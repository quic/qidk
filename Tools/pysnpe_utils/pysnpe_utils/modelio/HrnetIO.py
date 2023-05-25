"""
Python Helper for Hrnet model pre and post processing
"""
# @Author and Maintainer: Pradeep Pant (ppant)


import cv2
import numpy as np
import os
from datetime import datetime
import torchvision.transforms as transforms
import torch
import math
from pysnpe_utils.logger_config import logger
from .ModelIO import ModelIO

class HrnetIO(ModelIO):
    NUM_KPTS = 17

    SKELETON = [
        [1,3],[1,0],[2,4],[2,0],[0,5],[0,6],[5,7],[7,9],[6,8],[8,10],[5,11],[6,12],[11,12],[11,13],[13,15],[12,14],[14,16]
    ]

    CocoColors = [[255, 0, 0], [255, 85, 0], [255, 170, 0], [255, 255, 0], [170, 255, 0], [85, 255, 0], [0, 255, 0],
                  [0, 255, 85], [0, 255, 170], [0, 255, 255], [0, 170, 255], [0, 85, 255], [0, 0, 255], [85, 0, 255],
                  [170, 0, 255], [255, 0, 255], [255, 0, 170], [255, 0, 85]]

    COCO_KEYPOINT_INDEXES = {
        0: 'nose',
        1: 'left_eye',
        2: 'right_eye',
        3: 'left_ear',
        4: 'right_ear',
        5: 'left_shoulder',
        6: 'right_shoulder',
        7: 'left_elbow',
        8: 'right_elbow',
        9: 'left_wrist',
        10: 'right_wrist',
        11: 'left_hip',
        12: 'right_hip',
        13: 'left_knee',
        14: 'right_knee',
        15: 'left_ankle',
        16: 'right_ankle'
    }
        
    def preprocess(img_path):
        image_bgr = cv2.resize(cv2.imread(img_path),(192, 256),interpolation=cv2.INTER_CUBIC)
        img = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB)
        center, scale = getcenterscale(img, 192, 256)
        rotation = 0
        trans = get_affine_transform(center, scale, rotation, [192, 256])
        model_input = cv2.warpAffine(
            img,
            trans,
            (192,256),
            flags=cv2.INTER_LINEAR)
        transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406],
                                 std=[0.229, 0.224, 0.225]),
        ])    
        model_input = transform(model_input).unsqueeze(0)
        model_input = model_input.cpu().detach().numpy()
        # model_input = model_input.transpose(0,2,3,1) 
        return model_input
    
    def postprocess(self,img_path, output):
        output = output["3672"].astype(np.float32).reshape(1,64,48,17) 
        output = output.transpose(0,3,1,2)
        output = torch.from_numpy(output)
        image_bgr = cv2.resize(cv2.imread(img_path),(192, 256),interpolation=cv2.INTER_CUBIC)
        img = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB)
        center, scale = getcenterscale(img, 192, 256)
        logger.debug("Output in dlc output"+{output.shape})    
        logger.debug("center"+{center})
        logger.debug("scale"+{scale})
        
        preds, _ = get_final_preds(
            output.clone().cpu().numpy(),
            np.asarray([center]),
            np.asarray([scale]))
        img_actual = cv2.imread(img_path)
        for key_pt in preds:
            draw_pose(key_pt,img_actual) # draw the poses
        if not os.path.exists("hrnet_output"):
            os.mkdir("hrnet_output")    
        output_name = "hrnet_output/hrnet_"+datetime.now().strftime('%Y-%m-%d-%H-%M-%S')+".jpg"
        cv2.imwrite(output_name, img_actual)
        logger.debug("output_name"+{output_name})
        return
    
    def draw_pose(keypoints,img):
        """draw the keypoints and the skeletons.
        :params keypoints: the shape should be equal to [17,2]
        :params img:
        """
        assert keypoints.shape == (NUM_KPTS,2)
        for i in range(len(SKELETON)):
            kpt_a, kpt_b = SKELETON[i][0], SKELETON[i][1]
            x_a, y_a = keypoints[kpt_a][0]*(img.shape[1]/192),keypoints[kpt_a][1]*(img.shape[0]/256)
            x_b, y_b = keypoints[kpt_b][0]*(img.shape[1]/192),keypoints[kpt_b][1]*(img.shape[0]/256)
            logger.debug("Point "+{i}+": "+{x_a}+", "+{y_a}+", "+{x_b}+", "+{y_b}+", "+{COCO_KEYPOINT_INDEXES[kpt_a]}+", "+{COCO_KEYPOINT_INDEXES[kpt_b]})
            cv2.circle(img, (int(x_a), int(y_a)), 2, CocoColors[i], -1)
            cv2.circle(img, (int(x_b), int(y_b)), 2, CocoColors[i], -1)
            cv2.line(img, (int(x_a), int(y_a)), (int(x_b), int(y_b)), CocoColors[i], 3)

    def get_3rd_point(a, b):
        direct = a - b
        return b + np.array([-direct[1], direct[0]], dtype=np.float32)
    def get_dir(src_point, rot_rad):
        sn, cs = np.sin(rot_rad), np.cos(rot_rad)

        src_result = [0, 0]
        src_result[0] = src_point[0] * cs - src_point[1] * sn
        src_result[1] = src_point[0] * sn + src_point[1] * cs

        return src_result
        
    def get_affine_transform(
            center, scale, rot, output_size,
            shift=np.array([0, 0], dtype=np.float32), inv=0):
        if not isinstance(scale, np.ndarray) and not isinstance(scale, list):
            logger.debug(scale)
            scale = np.array([scale, scale])

        scale_tmp = scale * 200.0
        src_w = scale_tmp[0]
        dst_w = output_size[0]
        dst_h = output_size[1]

        rot_rad = np.pi * rot / 180
        src_dir = get_dir([0, src_w * -0.5], rot_rad)
        dst_dir = np.array([0, dst_w * -0.5], np.float32)

        src = np.zeros((3, 2), dtype=np.float32)
        dst = np.zeros((3, 2), dtype=np.float32)
        src[0, :] = center + scale_tmp * shift
        src[1, :] = center + src_dir + scale_tmp * shift
        dst[0, :] = [dst_w * 0.5, dst_h * 0.5]
        dst[1, :] = np.array([dst_w * 0.5, dst_h * 0.5]) + dst_dir

        src[2:, :] = get_3rd_point(src[0, :], src[1, :])
        dst[2:, :] = get_3rd_point(dst[0, :], dst[1, :])

        if inv:
            trans = cv2.getAffineTransform(np.float32(dst), np.float32(src))
        else:
            trans = cv2.getAffineTransform(np.float32(src), np.float32(dst))

        return trans
        
    def getcenterscale(image, model_image_width, model_image_height):
        center = np.zeros((2), dtype=np.float32)
        bottom_left_corner = [0,0]
        top_right_corner = [image.shape[1],image.shape[0]]
        box_width = top_right_corner[0]-bottom_left_corner[0]
        box_height = top_right_corner[1]-bottom_left_corner[1]
        bottom_left_x = bottom_left_corner[0]
        bottom_left_y = bottom_left_corner[1]
        center[0] = bottom_left_x + box_width * 0.5
        center[1] = bottom_left_y + box_height * 0.5
        aspect_ratio = model_image_width * 1.0 / model_image_height
        pixel_std = 200

        if box_width > aspect_ratio * box_height:
            logger.debug("width is big")
            box_width = box_height * aspect_ratio
        elif box_width < aspect_ratio * box_height:
            logger.debug("height is big")
            box_height = box_width * 1.0 / aspect_ratio

        scale = np.array(
            [box_width * 1.0 / pixel_std, box_height * 1.0 / pixel_std],
            dtype=np.float32)
        if center[0] != -1:
            scale = scale * 1.25
        return center, scale


    def get_max_preds(batch_heatmaps):
        '''
        get predictions from score maps
        heatmaps: numpy.ndarray([batch_size, num_joints, height, width])
        '''
        assert isinstance(batch_heatmaps, np.ndarray), \
            'batch_heatmaps should be numpy.ndarray'
        assert batch_heatmaps.ndim == 4, 'batch_images should be 4-ndim'

        batch_size = batch_heatmaps.shape[0]
        num_joints = batch_heatmaps.shape[1]
        width = batch_heatmaps.shape[3]
        heatmaps_reshaped = batch_heatmaps.reshape((batch_size, num_joints, -1))
        idx = np.argmax(heatmaps_reshaped, 2)
        maxvals = np.amax(heatmaps_reshaped, 2)

        maxvals = maxvals.reshape((batch_size, num_joints, 1))
        idx = idx.reshape((batch_size, num_joints, 1))

        preds = np.tile(idx, (1, 1, 2)).astype(np.float32)

        preds[:, :, 0] = (preds[:, :, 0]) % width
        preds[:, :, 1] = np.floor((preds[:, :, 1]) / width)

        pred_mask = np.tile(np.greater(maxvals, 0.0), (1, 1, 2))
        pred_mask = pred_mask.astype(np.float32)

        preds *= pred_mask
        return preds, maxvals


    def get_final_preds( batch_heatmaps, center, scale):
        coords, maxvals = get_max_preds(batch_heatmaps)

        heatmap_height = batch_heatmaps.shape[2]
        heatmap_width = batch_heatmaps.shape[3]

        # post-processing
        if True:
            for n in range(coords.shape[0]):
                for p in range(coords.shape[1]):
                    hm = batch_heatmaps[n][p]
                    px = int(math.floor(coords[n][p][0] + 0.5))
                    py = int(math.floor(coords[n][p][1] + 0.5))
                    if 1 < px < heatmap_width-1 and 1 < py < heatmap_height-1:
                        diff = np.array(
                            [
                                hm[py][px+1] - hm[py][px-1],
                                hm[py+1][px]-hm[py-1][px]
                            ]
                        )
                        coords[n][p] += np.sign(diff) * .25

        preds = coords.copy()

        # Transform back
        for i in range(coords.shape[0]):
            preds[i] = transform_preds(
                coords[i], center[i], scale[i], [heatmap_width, heatmap_height]
            )

        return preds, maxvals

    def transform_preds(coords, center, scale, output_size):
        target_coords = np.zeros(coords.shape)
        trans = get_affine_transform(center, scale, 0, output_size, inv=1)
        for p in range(coords.shape[0]):
            target_coords[p, 0:2] = affine_transform(coords[p, 0:2], trans)
        return target_coords
        
    def affine_transform(pt, t):
        new_pt = np.array([pt[0], pt[1], 1.]).T
        new_pt = np.dot(t, new_pt)
        return new_pt[:2]