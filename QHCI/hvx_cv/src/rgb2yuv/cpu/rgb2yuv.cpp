//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include "QhciBase.hpp"

#define FEATURE_NAME "rgb2yuv"

// Step0. Define reference code here
typedef enum CvtcolorMethod
{
    CVTCOLOR_RGB2NV12,
    CVTCOLOR_RGB2NV21,
    CVTCOLOR_NV122RGB,
    CVTCOLOR_NV212RGB,
} CvtcolorMethod;

#ifndef _MIN
#define _MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef _MAX
#define _MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

static inline int _Round(double value)
{
    double intpart, fractpart;
    fractpart = modf(value, &intpart);
    if ((fabs(fractpart) != 0.5) || ((((int)intpart) & 1) != 0))
        return (int)(value + (value >= 0 ? 0.5 : -0.5));
    else
        return (int)intpart;
}

static int CvtcolorRgbToNv12(unsigned char *src, int src_w, int src_h, int src_stride,
                             unsigned char *dstY, unsigned char *dstUV, int dst_w, int dst_h, int dst_stride)
{
    if (NULL == src || NULL == dstY || NULL == dstUV)
    {
        printf("null ptr!\n");
        return -1;
    }

    if ((src_w != dst_w) || (src_h != dst_h))
    {
        printf("src and dst are not the same size!");
        return -1;
    }

    if (0 != (dst_h % 2) || 0 != (dst_w % 2))
    {
        printf("width and height need to align to 2!\n");
        return -1;
    }

    int ret = 0;

    const int shift_value = 11;
    const int multiplicator = 2048;

    const short int r2y = (short int)_Round(0.257 * multiplicator);
    const short int g2y = (short int)_Round(0.504 * multiplicator);
    const short int b2y = (short int)_Round(0.098 * multiplicator);
    const short int r2u = (short int)_Round(-0.148 * multiplicator);
    const short int g2u = (short int)_Round(-0.291 * multiplicator);
    const short int b2u = (short int)_Round(0.439 * multiplicator);
    const short int r2v = (short int)_Round(0.439 * multiplicator);
    const short int g2v = (short int)_Round(-0.368 * multiplicator);
    const short int b2v = (short int)_Round(-0.071 * multiplicator);

    const int yc = _Round(16.5 * multiplicator);
    const int uc = _Round(128.5 * multiplicator);

    unsigned char *input_line0 = (unsigned char *)src;
    unsigned char *input_line1 = (unsigned char *)src + src_stride;

    unsigned char *output_y0 = (unsigned char *)dstY;
    unsigned char *output_y1 = (unsigned char *)dstY + dst_stride;
    unsigned char *output_u = (unsigned char *)dstUV;
    unsigned char *output_v = (unsigned char *)dstUV + 1;

    int y00, y01, y10, y11;
    int u00, v00;

    for (int j = 0; j < src_h; j += 2)
    {
        unsigned char *_line0 = input_line0 + (j * src_stride);
        unsigned char *_line1 = input_line1 + (j * src_stride);

        unsigned char *_y0 = output_y0 + (j * dst_stride);
        unsigned char *_y1 = output_y1 + (j * dst_stride);
        unsigned char *_u = output_u + (j / 2 * dst_stride);
        unsigned char *_v = output_v + (j / 2 * dst_stride);

        for (int i = 0; i < src_w; i += 2)
        {
            y00 = yc + r2y * _line0[0] + g2y * _line0[1] + b2y * _line0[2];
            y01 = yc + r2y * _line0[3] + g2y * _line0[4] + b2y * _line0[5];
            y10 = yc + r2y * _line1[0] + g2y * _line1[1] + b2y * _line1[2];
            y11 = yc + r2y * _line1[3] + g2y * _line1[4] + b2y * _line1[5];

            u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];

            // store to dst
            _y0[0] = (unsigned char)(y00 >> shift_value);
            _y0[1] = (unsigned char)(y01 >> shift_value);
            _y1[0] = (unsigned char)(y10 >> shift_value);
            _y1[1] = (unsigned char)(y11 >> shift_value);
            _u[0] = (unsigned char)(u00 >> shift_value);
            _v[0] = (unsigned char)(v00 >> shift_value);
            _line0 += 6;
            _line1 += 6;

            _y0 += 2;
            _y1 += 2;
            _u += 2;
            _v += 2;
        }
    }

    return ret;
}

static int CvtcolorRgbToNv21(unsigned char *src, int src_w, int src_h, int src_stride,
                             unsigned char *dstY, unsigned char *dstUV, int dst_w, int dst_h, int dst_stride)
{
    if (NULL == src || NULL == dstY || NULL == dstUV)
    {
        printf("null ptr!\n");
        return -1;
    }

    if ((src_w != dst_w) || (src_h != dst_h))
    {
        printf("src and dst are not the same size!src_w = %d , dst_w = %d ,src_h = %d  , dst_h = %d  \n", src_w, dst_w, src_h, dst_h);
        return -1;
    }

    if (0 != (dst_h % 2) || 0 != (dst_w % 2))
    {
        printf("width and height need to align to 2!\n");
        return -1;
    }

    int ret = 0;

    const int shift_value = 11;
    const int multiplicator = 2048;

    const short int r2y = (short int)_Round(0.257 * multiplicator);
    // printf("r2y= %d\n",r2y);
    const short int g2y = (short int)_Round(0.504 * multiplicator);
    // printf("g2y= %d\n",g2y);
    const short int b2y = (short int)_Round(0.098 * multiplicator);
    // printf("b2y= %d\n",b2y);
    const short int r2u = (short int)_Round(-0.148 * multiplicator);
    // printf("r2u= %d\n",r2u);
    const short int g2u = (short int)_Round(-0.291 * multiplicator);
    // printf("g2u= %d\n",g2u);
    const short int b2u = (short int)_Round(0.439 * multiplicator);
    // printf("b2u= %d\n",b2u);
    const short int r2v = (short int)_Round(0.439 * multiplicator);
    // printf("r2v= %d\n",r2v);
    const short int g2v = (short int)_Round(-0.368 * multiplicator);
    // printf("g2v= %d\n",g2v);
    const short int b2v = (short int)_Round(-0.071 * multiplicator);
    // printf("b2v= %d\n",b2v);
    const int yc = _Round(16.5 * multiplicator);
    // printf("yc= %d\n",yc);
    const int uc = _Round(128.5 * multiplicator);
    // printf("uc= %d\n",uc);

    unsigned char *input_line0 = (unsigned char *)src;
    unsigned char *input_line1 = (unsigned char *)src + src_stride;

    unsigned char *output_y0 = (unsigned char *)dstY;
    unsigned char *output_y1 = (unsigned char *)dstY + dst_stride;
    unsigned char *output_v = (unsigned char *)dstUV;
    unsigned char *output_u = (unsigned char *)dstUV + 1;

    int y00, y01, y10, y11;
    int v00, u00;

    for (int j = 0; j < src_h; j += 2)
    {
        unsigned char *_line0 = input_line0 + (j * src_stride);
        unsigned char *_line1 = input_line1 + (j * src_stride);

        unsigned char *_y0 = output_y0 + (j * dst_stride);
        unsigned char *_y1 = output_y1 + (j * dst_stride);
        unsigned char *_v = output_v + (j / 2 * dst_stride);
        unsigned char *_u = output_u + (j / 2 * dst_stride);

        for (int i = 0; i < src_w; i += 2)
        {
            y00 = yc + r2y * _line0[0] + g2y * _line0[1] + b2y * _line0[2];
            y01 = yc + r2y * _line0[3] + g2y * _line0[4] + b2y * _line0[5];
            y10 = yc + r2y * _line1[0] + g2y * _line1[1] + b2y * _line1[2];
            y11 = yc + r2y * _line1[3] + g2y * _line1[4] + b2y * _line1[5];

            v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];

            // store to dst
            _y0[0] = (unsigned char)(y00 >> shift_value);
            _y0[1] = (unsigned char)(y01 >> shift_value);
            _y1[0] = (unsigned char)(y10 >> shift_value);
            _y1[1] = (unsigned char)(y11 >> shift_value);
            _v[0] = (unsigned char)(v00 >> shift_value);
            _u[0] = (unsigned char)(u00 >> shift_value);

            _line0 += 6;
            _line1 += 6;

            _y0 += 2;
            _y1 += 2;
            _v += 2;
            _u += 2;
        }
    }

    return ret;
}

int rgb2yuv_ref(unsigned char *src, int src_w, int src_h, int src_stride,
                unsigned char *dstY, unsigned char *dstUV, int dst_w, int dst_h, int dst_stride, CvtcolorMethod method)
{
    if ((NULL == src) || (NULL == dstY) || (NULL == dstUV))
    {
        printf("null ptr!\n");
        return -1;
    }
    // printf("rgb2yuv_ref start !\n");
    int ret = 0;

    if (method == CVTCOLOR_RGB2NV12)
    {
        printf("start CVTCOLOR_RGB2NV12\n");
        for (int loops = 0; loops < LOOPS; loops++)
        {
        ret |= CvtcolorRgbToNv12(src, src_w, src_h, src_stride, dstY, dstUV, dst_w, dst_h, dst_stride);
        }
    }
    else if (method == CVTCOLOR_RGB2NV21)
    {
        printf("start CVTCOLOR_RGB2NV21\n");
        for (int loops = 0; loops < LOOPS; loops++)
        {
        ret |= CvtcolorRgbToNv21(src, src_w, src_h, src_stride, dstY, dstUV, dst_w, dst_h, dst_stride);
        }
    }
    else
    {
        printf("This cvtcolor method is not supported!\n");
        return -1;
    }

    return ret;
}

static int CvtcolorNv12ToRgb(unsigned char *srcY, unsigned char *srcUV, int src_w, int src_h, int src_stride,
                             unsigned char *dst, int dst_w, int dst_h, int dst_stride)
{
    if ((NULL == srcY) || (NULL == srcUV) || (NULL == dst))
    {
        printf("null ptr!\n");
        return -1;
    }

    if ((src_w != dst_w) || (src_h != dst_h))
    {
        printf("src and dst are not the same size!");
        return -1;
    }

    if (0 != (src_h % 2) || 0 != (src_w % 2))
    {
        printf("width and height need to align to 2!\n");
        return -1;
    }

    int ret = 0;

    const int shift_value = 7;
    const int multiplicator = 128;

    const short int max_255 = 255 * multiplicator;
    // printf("max_255= %d\n",max_255);
    short int y2rgb = _Round(1.164 * multiplicator);
    // printf("y2rgb= %d\n",y2rgb);
    short int u2b = _Round(2.018 * multiplicator);
    // printf("u2b= %d\n",u2b);
    short int u2g = _Round(-0.391 * multiplicator);
    // printf("u2g= %d\n",u2g);
    short int v2g = _Round(-0.813 * multiplicator);
    // printf("v2g= %d\n",v2g);
    short int v2r = _Round(1.596 * multiplicator);
    // printf("v2r= %d\n",v2r);

    unsigned char *input_y0 = (unsigned char *)srcY;
    unsigned char *input_y1 = (unsigned char *)srcY + src_stride;
    unsigned char *input_uv = (unsigned char *)srcUV;

    unsigned char *output_r0 = (unsigned char *)dst;
    unsigned char *output_g0 = (unsigned char *)dst + 1;
    unsigned char *output_b0 = (unsigned char *)dst + 2;

    unsigned char *output_r1 = (unsigned char *)dst + dst_stride;
    unsigned char *output_g1 = (unsigned char *)dst + dst_stride + 1;
    unsigned char *output_b1 = (unsigned char *)dst + dst_stride + 2;

    int r00, r01, r10, r11;
    int g00, g01, g10, g11;
    int b00, b01, b10, b11;

    int rv, gv, gu, bu;
    int y00_mult, y01_mult, y10_mult, y11_mult;

    for (int j = 0; j < dst_h; j += 2)
    {
        unsigned char *_y0 = input_y0 + (j * src_stride);
        unsigned char *_y1 = input_y1 + (j * src_stride);
        unsigned char *_uv = input_uv + (j / 2 * src_stride);

        unsigned char *_r0 = output_r0 + (j * dst_stride);
        unsigned char *_g0 = output_g0 + (j * dst_stride);
        unsigned char *_b0 = output_b0 + (j * dst_stride);

        unsigned char *_r1 = output_r1 + (j * dst_stride);
        unsigned char *_g1 = output_g1 + (j * dst_stride);
        unsigned char *_b1 = output_b1 + (j * dst_stride);

        for (int i = 0; i < dst_w; i += 2)
        {

            rv = (_uv[1] - 128) * v2r + (1 << (shift_value - 1));
            gv = (_uv[1] - 128) * v2g + (1 << (shift_value - 1));
            gu = (_uv[0] - 128) * u2g;
            bu = (_uv[0] - 128) * u2b + (1 << (shift_value - 1));

            y00_mult = _MAX((_y0[0] - 16), 0) * y2rgb;

            y01_mult = _MAX((_y0[1] - 16), 0) * y2rgb;
            y10_mult = _MAX((_y1[0] - 16), 0) * y2rgb;
            y11_mult = _MAX((_y1[1] - 16), 0) * y2rgb;

            r00 = _MIN(max_255, _MAX(y00_mult + rv, 0));

            g00 = _MIN(max_255, _MAX(y00_mult + gv + gu, 0));
            b00 = _MIN(max_255, _MAX(y00_mult + bu, 0));
            //  if(j == 0 && (i < 128) )printf("b00= %d    ...\n",  b00);
            r01 = _MIN(max_255, _MAX(y01_mult + rv, 0));
            g01 = _MIN(max_255, _MAX(y01_mult + gv + gu, 0));
            b01 = _MIN(max_255, _MAX(y01_mult + bu, 0));
            // if(j == 0 && (i < 128) )printf("y01_mult + bu= %d    ...\n",  y01_mult + bu);
            r10 = _MIN(max_255, _MAX(y10_mult + rv, 0));
            g10 = _MIN(max_255, _MAX(y10_mult + gv + gu, 0));
            b10 = _MIN(max_255, _MAX(y10_mult + bu, 0));

            r11 = _MIN(max_255, _MAX(y11_mult + rv, 0));
            g11 = _MIN(max_255, _MAX(y11_mult + gv + gu, 0));
            b11 = _MIN(max_255, _MAX(y11_mult + bu, 0));

            // store to dst
            _r0[0] = (unsigned char)(r00 >> shift_value);
            _r0[3] = (unsigned char)(r01 >> shift_value);
            _r1[0] = (unsigned char)(r10 >> shift_value);
            _r1[3] = (unsigned char)(r11 >> shift_value);

            _g0[0] = (unsigned char)(g00 >> shift_value);
            _g0[3] = (unsigned char)(g01 >> shift_value);
            _g1[0] = (unsigned char)(g10 >> shift_value);
            _g1[3] = (unsigned char)(g11 >> shift_value);

            _b0[0] = (unsigned char)(b00 >> shift_value);
            _b0[3] = (unsigned char)(b01 >> shift_value);
            _b1[0] = (unsigned char)(b10 >> shift_value);
            _b1[3] = (unsigned char)(b11 >> shift_value);

            _y0 += 2;
            _y1 += 2;
            _uv += 2;

            _r0 += 6;
            _r1 += 6;
            _g0 += 6;
            _g1 += 6;
            _b0 += 6;
            _b1 += 6;
        }
    }

    return ret;
}

static int CvtcolorNv21ToRgb(unsigned char *srcY, unsigned char *srcUV, int src_w, int src_h, int src_stride,
                             unsigned char *dst, int dst_w, int dst_h, int dst_stride)
{
    if ((NULL == srcY) || (NULL == srcUV) || (NULL == dst))
    {
        printf("null ptr!\n");
        return -1;
    }

    if ((src_w != dst_w) || (src_h != dst_h))
    {
        printf("src and dst are not the same size!");
        return -1;
    }

    if (0 != (src_h % 2) || 0 != (src_w % 2))
    {
        printf("width and height need to align to 2!\n");
        return -1;
    }

    int ret = 0;

    const int shift_value = 7;
    const int multiplicator = 128;

    const int max_255 = 255 * multiplicator;
    int y2rgb = _Round(1.164 * multiplicator);
    int u2b = _Round(2.018 * multiplicator);
    int u2g = _Round(-0.391 * multiplicator);
    int v2g = _Round(-0.813 * multiplicator);
    int v2r = _Round(1.596 * multiplicator);

    unsigned char *input_y0 = (unsigned char *)srcY;
    unsigned char *input_y1 = (unsigned char *)srcY + src_stride;
    unsigned char *input_uv = (unsigned char *)srcUV;

    unsigned char *output_r0 = (unsigned char *)dst;
    unsigned char *output_g0 = (unsigned char *)dst + 1;
    unsigned char *output_b0 = (unsigned char *)dst + 2;

    unsigned char *output_r1 = (unsigned char *)dst + dst_stride;
    unsigned char *output_g1 = (unsigned char *)dst + dst_stride + 1;
    unsigned char *output_b1 = (unsigned char *)dst + dst_stride + 2;

    int r00, r01, r10, r11;
    int g00, g01, g10, g11;
    int b00, b01, b10, b11;

    int rv, gv, gu, bu;
    int y00_mult, y01_mult, y10_mult, y11_mult;

    for (int j = 0; j < dst_h; j += 2)
    {
        unsigned char *_y0 = input_y0 + (j * src_stride);
        unsigned char *_y1 = input_y1 + (j * src_stride);
        unsigned char *_uv = input_uv + (j / 2 * src_stride);

        unsigned char *_r0 = output_r0 + (j * dst_stride);
        unsigned char *_g0 = output_g0 + (j * dst_stride);
        unsigned char *_b0 = output_b0 + (j * dst_stride);

        unsigned char *_r1 = output_r1 + (j * dst_stride);
        unsigned char *_g1 = output_g1 + (j * dst_stride);
        unsigned char *_b1 = output_b1 + (j * dst_stride);

        for (int i = 0; i < dst_w; i += 2)
        {
            rv = (_uv[0] - 128) * v2r + (1 << (shift_value - 1));
            gv = (_uv[0] - 128) * v2g + (1 << (shift_value - 1));
            gu = (_uv[1] - 128) * u2g;
            bu = (_uv[1] - 128) * u2b + (1 << (shift_value - 1));

            y00_mult = _MAX((_y0[0] - 16), 0) * y2rgb;
            y01_mult = _MAX((_y0[1] - 16), 0) * y2rgb;
            y10_mult = _MAX((_y1[0] - 16), 0) * y2rgb;
            y11_mult = _MAX((_y1[1] - 16), 0) * y2rgb;

            r00 = _MIN(max_255, _MAX(y00_mult + rv, 0));
            g00 = _MIN(max_255, _MAX(y00_mult + gv + gu, 0));
            b00 = _MIN(max_255, _MAX(y00_mult + bu, 0));

            r01 = _MIN(max_255, _MAX(y01_mult + rv, 0));
            g01 = _MIN(max_255, _MAX(y01_mult + gv + gu, 0));
            b01 = _MIN(max_255, _MAX(y01_mult + bu, 0));

            r10 = _MIN(max_255, _MAX(y10_mult + rv, 0));
            g10 = _MIN(max_255, _MAX(y10_mult + gv + gu, 0));
            b10 = _MIN(max_255, _MAX(y10_mult + bu, 0));

            r11 = _MIN(max_255, _MAX(y11_mult + rv, 0));
            g11 = _MIN(max_255, _MAX(y11_mult + gv + gu, 0));
            b11 = _MIN(max_255, _MAX(y11_mult + bu, 0));

            // store to dst
            _r0[0] = (unsigned char)(r00 >> shift_value);
            _r0[3] = (unsigned char)(r01 >> shift_value);
            _r1[0] = (unsigned char)(r10 >> shift_value);
            _r1[3] = (unsigned char)(r11 >> shift_value);

            _g0[0] = (unsigned char)(g00 >> shift_value);
            _g0[3] = (unsigned char)(g01 >> shift_value);
            _g1[0] = (unsigned char)(g10 >> shift_value);
            _g1[3] = (unsigned char)(g11 >> shift_value);

            _b0[0] = (unsigned char)(b00 >> shift_value);
            _b0[3] = (unsigned char)(b01 >> shift_value);
            _b1[0] = (unsigned char)(b10 >> shift_value);
            _b1[3] = (unsigned char)(b11 >> shift_value);

            _y0 += 2;
            _y1 += 2;
            _uv += 2;

            _r0 += 6;
            _r1 += 6;
            _g0 += 6;
            _g1 += 6;
            _b0 += 6;
            _b1 += 6;
        }
    }

    return ret;
}

int yuv2rgb_ref(unsigned char *srcY, unsigned char *srcUV, int src_w, int src_h, int src_stride,
                unsigned char *dst, int dst_w, int dst_h, int dst_stride, CvtcolorMethod method)
{
    if ((NULL == srcY) || (NULL == srcUV) || (NULL == dst))
    {
        printf("null ptr!\n");
        return -1;
    }

    int ret = 0;

    if (method == CVTCOLOR_NV122RGB)
    {
        printf("start CVTCOLOR_NV122RGB\n");
        for (int loops = 0; loops < LOOPS; loops++)
        {
        ret |= CvtcolorNv12ToRgb(srcY, srcUV, src_w, src_h, src_stride, dst, dst_w, dst_h, dst_stride);
        }
    }
    else if (method == CVTCOLOR_NV212RGB)
    {
        printf("start CVTCOLOR_NV212RGB\n");
        for (int loops = 0; loops < LOOPS; loops++)
        {
        ret |= CvtcolorNv21ToRgb(srcY, srcUV, src_w, src_h, src_stride, dst, dst_w, dst_h, dst_stride);
        }
    }
    else
    {
        printf("This cvtcolor method is not supported!\n");
        return -1;
    }

    return ret;
}

AEEResult qhci_rgb2nv12_ref(
    remote_handle64 handle,
    uint8_t *imgSrc, // input buffer of unsigned 8-bit values
    int imgSrcLen,
    uint32_t srcWidth,  // source width
    uint32_t srcHeight, // source height
    uint32_t srcStride, // source stride
    uint32_t dstWidth,  // dst width
    uint32_t dstHeight, // dst height
    uint32_t dstStride, // dst stride
    uint8_t *imgDst,    // output dst buffer
    int imgDstLen)
{
    CvtcolorMethod method = CVTCOLOR_RGB2NV12;
    unsigned char *dstY = imgDst;
    unsigned char *dstUV = imgDst + dstStride * dstHeight;
    rgb2yuv_ref(imgSrc, srcWidth, srcHeight, srcStride, dstY, dstUV, dstWidth, dstHeight, dstStride, method);
    return AEE_SUCCESS;
}

AEEResult qhci_rgb2nv21_ref(
    remote_handle64 handle,
    uint8_t *imgSrc, // input buffer of unsigned 8-bit values
    int imgSrcLen,
    uint32_t srcWidth,  // src width
    uint32_t srcHeight, //
    uint32_t srcStride, // src stride
    uint32_t dstWidth,  // dst width
    uint32_t dstHeight, // dst height
    uint32_t dstStride, // dst stride
    uint8_t *imgDst,    // output buffer of unsigned 8-bit values
    int imgDstLen)
{
    CvtcolorMethod method = CVTCOLOR_RGB2NV21;
    unsigned char *dstY = imgDst;
    unsigned char *dstUV = imgDst + dstStride * dstHeight;
    rgb2yuv_ref(imgSrc, srcWidth, srcHeight, srcStride, dstY, dstUV, dstWidth, dstHeight, dstStride, method);
    return AEE_SUCCESS;
}

AEEResult qhci_nv12Torgb_ref(
    remote_handle64 handle,
    uint8_t *imgSrc, //
    int imgSrcLen,
    uint32_t srcWidth,  // src width
    uint32_t srcHeight, // src height
    uint32_t srcStride, // src stride
    uint32_t dstWidth,  // dst width
    uint32_t dstHeight, // dst height
    uint32_t dstStride, // dst stride
    uint8_t *imgDst,    // output buffer of unsigned 8-bit values
    int imgDstLen)
{
    CvtcolorMethod method = CVTCOLOR_NV122RGB;
    unsigned char *srcY = imgSrc;
    unsigned char *srcUV = imgSrc + srcStride * srcHeight;
    yuv2rgb_ref(srcY, srcUV, srcWidth, srcHeight, srcStride, imgDst, dstWidth, dstHeight, dstStride, method);
    return AEE_SUCCESS;
}

AEEResult qhci_nv21Torgb_ref(
    remote_handle64 handle,
    uint8_t *imgSrc, // input buffer of unsigned 8-bit values
    int imgSrcLen,
    uint32_t srcWidth,  // src width
    uint32_t srcHeight, // src height
    uint32_t srcStride, // src stride
    uint32_t dstWidth,  // dst width
    uint32_t dstHeight, // dst height
    uint32_t dstStride, // dst stride
    uint8_t *imgDst,    // output buffer of unsigned 8-bit values
    int imgDstLen)
{
    CvtcolorMethod method = CVTCOLOR_NV212RGB;
    unsigned char *srcY = imgSrc;
    unsigned char *srcUV = imgSrc + srcStride * srcHeight;
    yuv2rgb_ref(srcY, srcUV, srcWidth, srcHeight, srcStride, imgDst, dstWidth, dstHeight, dstStride, method);
    return AEE_SUCCESS;
}

class RGB2YUV : public QhciFeatureBase
{
public:
    using QhciFeatureBase::QhciFeatureBase;
    int Test(remote_handle64 handle) override
    {
        AEEResult nErr = 0;
        // Api teat
        {
            // Step1. Define param && allocate buffer
            uint32_t imgSrcLen = 1600 * 3 * 1300;
            uint8_t *imgSrc = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgSrcLen * sizeof(uint8_t));
            uint32_t srcWidth = 1600;
            uint32_t srcHeight = 1300;
            uint32_t srcStride = 1600 * 3;
            uint32_t dstWidth = 1600;
            uint32_t dstHeight = 1300;
            uint32_t dstStride = 1600;
            uint32_t imgDstLen = 1600 * 1300 * 3 / 2;
            uint8_t *imgDst_cpu = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgDstLen * sizeof(uint8_t));
            uint8_t *imgDst_dsp = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgDstLen * sizeof(uint8_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            GenerateRandomData((uint8_t *)imgSrc, imgSrcLen * sizeof(uint8_t));

            // Step3. CPU execute
            TIME_STAMP("Cpu", qhci_rgb2nv12_ref(handle, imgSrc, imgSrcLen, srcWidth, srcHeight, srcStride, dstWidth, dstHeight, dstStride, imgDst_cpu, imgDstLen));

            // Step4. DSP execute
            TIME_STAMP("DSP", qhci_rgb2nv12(handle, imgSrc, imgSrcLen, srcWidth, srcHeight, srcStride, dstWidth, dstHeight, dstStride, imgDst_dsp, imgDstLen));

            // Step5. Compare CPU & DSP results
            nErr = CompareBuffers(imgDst_cpu, imgDst_dsp, imgDstLen);
            CHECK(nErr == AEE_SUCCESS);

            // Step6. Free buffer
            if (imgSrc)
                rpcmem_free(imgSrc);
            if (imgDst_cpu)
                rpcmem_free(imgDst_cpu);
            if (imgDst_dsp)
                rpcmem_free(imgDst_dsp);
        }
        {
            // Step1. Define param && allocate buffer
            uint32_t imgSrcLen = 1600 * 3 * 1300;
            uint8_t *imgSrc = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgSrcLen * sizeof(uint8_t));
            uint32_t srcWidth = 1600;
            uint32_t srcHeight = 1300;
            uint32_t srcStride = 1600 * 3;
            uint32_t dstWidth = 1600;
            uint32_t dstHeight = 1300;
            uint32_t dstStride = 1600;
            uint32_t imgDstLen = 1600 * 1300 * 3 / 2;
            uint8_t *imgDst_cpu = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgDstLen * sizeof(uint8_t));
            uint8_t *imgDst_dsp = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgDstLen * sizeof(uint8_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            GenerateRandomData((uint8_t *)imgSrc, imgSrcLen * sizeof(uint8_t));

            // Step3. CPU execute
            TIME_STAMP("Cpu", qhci_rgb2nv21_ref(handle, imgSrc, imgSrcLen, srcWidth, srcHeight, srcStride, dstWidth, dstHeight, dstStride, imgDst_cpu, imgDstLen));

            // Step4. DSP execute
            TIME_STAMP("DSP", qhci_rgb2nv21(handle, imgSrc, imgSrcLen, srcWidth, srcHeight, srcStride, dstWidth, dstHeight, dstStride, imgDst_dsp, imgDstLen));

            // Step5. Compare CPU & DSP results
            nErr = CompareBuffers(imgDst_cpu, imgDst_dsp, imgDstLen);
            CHECK(nErr == AEE_SUCCESS);

            // Step6. Free buffer
            if (imgSrc)
                rpcmem_free(imgSrc);
            if (imgDst_cpu)
                rpcmem_free(imgDst_cpu);
            if (imgDst_dsp)
                rpcmem_free(imgDst_dsp);
        }
        {
            // Step1. Define param && allocate buffer
            uint32_t imgSrcLen = 1600 * 1300 * 3 / 2;
            uint8_t *imgSrc = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgSrcLen * sizeof(uint8_t));
            uint32_t srcWidth = 1600;
            uint32_t srcHeight = 1300;
            uint32_t srcStride = 1600;
            uint32_t dstWidth = 1600;
            uint32_t dstHeight = 1300;
            uint32_t dstStride = 1600 * 3;
            uint32_t imgDstLen = 1600 * 1300 * 3;
            uint8_t *imgDst_cpu = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgDstLen * sizeof(uint8_t));
            uint8_t *imgDst_dsp = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgDstLen * sizeof(uint8_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            GenerateRandomData((uint8_t *)imgSrc, imgSrcLen * sizeof(uint8_t));

            // Step3. CPU execute
            TIME_STAMP("Cpu", qhci_nv12Torgb_ref(handle, imgSrc, imgSrcLen, srcWidth, srcHeight, srcStride, dstWidth, dstHeight, dstStride, imgDst_cpu, imgDstLen));

            // Step4. DSP execute
            TIME_STAMP("DSP", qhci_nv12Torgb(handle, imgSrc, imgSrcLen, srcWidth, srcHeight, srcStride, dstWidth, dstHeight, dstStride, imgDst_dsp, imgDstLen));

            // Step5. Compare CPU & DSP results
            nErr = CompareBuffers(imgDst_cpu, imgDst_dsp, imgDstLen);
            CHECK(nErr == AEE_SUCCESS);

            // Step6. Free buffer
            if (imgSrc)
                rpcmem_free(imgSrc);
            if (imgDst_cpu)
                rpcmem_free(imgDst_cpu);
            if (imgDst_dsp)
                rpcmem_free(imgDst_dsp);
        }
        {
            // Step1. Define param && allocate buffer
            uint32_t imgSrcLen = 1600 * 1300 * 3 / 2;
            uint8_t *imgSrc = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgSrcLen * sizeof(uint8_t));
            uint32_t srcWidth = 1600;
            uint32_t srcHeight = 1300;
            uint32_t srcStride = 1600;
            uint32_t dstWidth = 1600;
            uint32_t dstHeight = 1300;
            uint32_t dstStride = 1600 * 3;
            uint32_t imgDstLen = 1600 * 1300 * 3;
            uint8_t *imgDst_cpu = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgDstLen * sizeof(uint8_t));
            uint8_t *imgDst_dsp = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, imgDstLen * sizeof(uint8_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            GenerateRandomData((uint8_t *)imgSrc, imgSrcLen * sizeof(uint8_t));

            // Step3. CPU execute
            TIME_STAMP("Cpu", qhci_nv21Torgb_ref(handle, imgSrc, imgSrcLen, srcWidth, srcHeight, srcStride, dstWidth, dstHeight, dstStride, imgDst_cpu, imgDstLen));

            // Step4. DSP execute
            TIME_STAMP("DSP", qhci_nv21Torgb(handle, imgSrc, imgSrcLen, srcWidth, srcHeight, srcStride, dstWidth, dstHeight, dstStride, imgDst_dsp, imgDstLen));

            // Step5. Compare CPU & DSP results
            nErr = CompareBuffers(imgDst_cpu, imgDst_dsp, imgDstLen);
            CHECK(nErr == AEE_SUCCESS);

            // Step6. Free buffer
            if (imgSrc)
                rpcmem_free(imgSrc);
            if (imgDst_cpu)
                rpcmem_free(imgDst_cpu);
            if (imgDst_dsp)
                rpcmem_free(imgDst_dsp);
        }

        return nErr;
    }
};

static RGB2YUV *_feature = new RGB2YUV(FEATURE_NAME);
static bool init = []
{
    QhciTest::GetInstance().current_map_func()[FEATURE_NAME] = _feature;
    return true;
}();
#undef FEATURE_NAME
