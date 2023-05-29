package com.qc.objectdetection;

import java.util.ArrayList;
/**
 * RectangleBox class defines the property associated with each box like coordinates
 * labels, confidence etc.
 * Can also create copy of boxes.
 */
public class RectangleBox {

    public float top;
    public float bottom;
    public float left;
    public float right;
    public String label;
    public int label_index;
    public float confidence;
    public int fps,processing_time;

    void createCopy(RectangleBox b) {
        b.top = top;
        b.bottom = bottom;
        b.left = left;
        b.right = right;
        b.fps = fps;
        b.label = label;
        b.processing_time = processing_time;
        b.label_index = label_index;
        b.confidence = confidence;
    }

    public static ArrayList<RectangleBox> createBoxes(int num) {
        final ArrayList<RectangleBox> boxes;
        boxes = new ArrayList<>();
        for (int i = 0; i < num; ++i) {
            boxes.add(new RectangleBox());
        }
        return boxes;
    }
}
