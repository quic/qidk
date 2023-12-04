//=============================================================================
//
//  Copyright (c) 2022 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//=============================================================================

//==============================================================================
// Include Files
//==============================================================================
#include "qhci.h"
#include "AEEStdErr.h"
#include "HAP_power.h"
#include "HAP_farf.h"
#include "HAP_perf.h"
#include "remote.h"
#include "q6cache.h"
#include <assert.h>
#include "hexagon_types.h"
#include "hexagon_protos.h"
#include "HAP_vtcm_mgr.h"
#include "worker_pool.h"

/*===========================================================================
    DEFINITIONS
===========================================================================*/
// #define PROFILING_ON
#define _MIN(x, y) ((x) < (y) ? (x) : (y))
#define _MAX(x, y) ((x) > (y) ? (x) : (y))
typedef long HEXAGON_Vect1024_UN __attribute__((__vector_size__(128))) __attribute__((aligned(4)));
#define vmemu(A) *((HEXAGON_Vect1024_UN *)(A))

// (128-byte is only mode supported in this example)
#define VLEN 128
#define shift_value_yuv2bgr 7

/*===========================================================================
    DECLARATIONS
===========================================================================*/
// desired perm
// 0 ,1 ,2 ,96,3 ,4 ,5 ,97,6 ,7 ,8 ,98,9 ,10,11,99,12,13,14,100,15,16,17,101,18,19,20,102,21,22,23,103,24,25,26,104,27,28,29,105,30,31,32,
// 6,33,34,35,107,36,37,38,108,39,40,41,109,42,43,44,110,45,46,47,111,48,49,50,112,51,52,53,113,54,55,56,114,57,58,59,115,60,61,62,116,63,64,65,117,66,67,68,118,69,70,71,119,72,73,74,120,75,76,77,121,78,79,80,122,81,82,83,123,84,85,86,124,87,88,89,125,90,91,92,126,93,94,95,127,
unsigned char CtrlPerm[128 * 2] __attribute__((aligned(128))) = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x40,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x1F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,
    0x5F,

    0x00,
    0x00,
    0x00,
    0x04,
    0x01,
    0x03,
    0x09,
    0x0F,
    0x02,
    0x06,
    0x06,
    0x06,
    0x13,
    0x15,
    0x1F,
    0x1D,
    0x04,
    0x04,
    0x0C,
    0x08,
    0x0D,
    0x0F,
    0x0D,
    0x0B,
    0x26,
    0x22,
    0x2A,
    0x2A,
    0x3F,
    0x39,
    0x3B,
    0x39,
    0x08,
    0x08,
    0x08,
    0x0C,
    0x19,
    0x1B,
    0x11,
    0x17,
    0x1A,
    0x1E,
    0x1E,
    0x1E,
    0x1B,
    0x1D,
    0x17,
    0x15,
    0x0C,
    0x0C,
    0x04,
    0x00,
    0x15,
    0x17,
    0x15,
    0x13,
    0x3E,
    0x3A,
    0x32,
    0x32,
    0x37,
    0x31,
    0x33,
    0x31,
    0x10,
    0x10,
    0x10,
    0x14,
    0x11,
    0x13,
    0x19,
    0x1F,
    0x32,
    0x36,
    0x36,
    0x36,
    0x23,
    0x25,
    0x2F,
    0x2D,
    0x34,
    0x34,
    0x3C,
    0x38,
    0x3D,
    0x3F,
    0x3D,
    0x3B,
    0x36,
    0x32,
    0x3A,
    0x3A,
    0x2F,
    0x29,
    0x2B,
    0x29,
    0x18,
    0x18,
    0x18,
    0x1C,
    0x09,
    0x0B,
    0x01,
    0x07,
    0x2A,
    0x2E,
    0x2E,
    0x2E,
    0x2B,
    0x2D,
    0x27,
    0x25,
    0x3C,
    0x3C,
    0x34,
    0x30,
    0x25,
    0x27,
    0x25,
    0x23,
    0x2E,
    0x2A,
    0x22,
    0x22,
    0x27,
    0x21,
    0x23,
    0x21,
};

// desired perm
// 0 ,1 ,2 ,4 ,5 ,6 ,8 ,9 ,10,12,13,14,16,17,18,20,21,22,24,25,26,28,29,30,32,33,34,36,37,38,40,41,42,44,45,46,48,49,50,52,53,54,56,57,58,60,61,62,64,65,66,68,69,70,72,73,74,76,77,78,80,81,82,84,85,86,88,89,90,92,93,94,96,97,98,100,101,102,104,105,106,108,109,110,112,113,114,116,117,118,120,121,122,124,125,126,3 ,7 ,11,15,19,23,27,31,35,39,43,47,51,55,59,63,67,71,75,79,83,87,91,95,99,103,107,111,115,119,123,127,
unsigned char CtrlPerm_YUV2RGB[128 * 2] __attribute__((aligned(128))) = {
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x03,
    0x41,
    0x03,
    0x02,
    0x42,
    0x06,
    0x02,
    0x43,
    0x01,
    0x07,
    0x01,
    0x04,
    0x08,
    0x44,
    0x04,
    0x0D,
    0x03,
    0x05,
    0x47,
    0x46,
    0x0E,
    0x02,
    0x02,
    0x0F,
    0x45,
    0x03,
    0x01,
    0x08,
    0x48,
    0x10,
    0x08,
    0x49,
    0x0B,
    0x09,
    0x13,
    0x1A,
    0x0A,
    0x06,
    0x4A,
    0x0B,
    0x19,
    0x4F,
    0x01,
    0x4C,
    0x00,
    0x1C,
    0x04,
    0x05,
    0x4B,
    0x05,
    0x1F,
    0x1E,
    0x06,
    0x4A,
    0x02,
    0x07,
    0x1D,
    0x03,
    0x49,
    0x10,
    0x20,
    0x10,
    0x50,
    0x21,
    0x13,
    0x51,
    0x13,
    0x12,
    0x52,
    0x16,
    0x22,
    0x53,
    0x11,
    0x27,
    0x11,
    0x34,
    0x08,
    0x54,
    0x14,
    0x0D,
    0x33,
    0x15,
    0x57,
    0x56,
    0x1E,
    0x32,
    0x02,
    0x1F,
    0x55,
    0x03,
    0x31,
    0x18,
    0x78,
    0x00,
    0x08,
    0x79,
    0x1B,
    0x09,
    0x03,
    0x0A,
    0x0A,
    0x16,
    0x7A,
    0x0B,
    0x09,
    0x7F,
    0x11,
    0x7C,
    0x10,
    0x0C,
    0x04,
    0x15,
    0x7B,
    0x05,
    0x0F,
    0x0E,
    0x06,
    0x7A,
    0x12,
    0x07,
    0x0D,
    0x13,
    0x79,

    0x00,
    0x00,
    0x00,
    0x3C,
    0x00,
    0x00,
    0x38,
    0x04,
    0x00,
    0x34,
    0x00,
    0x0C,
    0x30,
    0x04,
    0x08,
    0x04,
    0x00,
    0x04,
    0x28,
    0x18,
    0x00,
    0x04,
    0x18,
    0x28,
    0x20,
    0x10,
    0x08,
    0x0C,
    0x10,
    0x20,
    0x08,
    0x0C,
    0x00,
    0x14,
    0x08,
    0x34,
    0x10,
    0x04,
    0x30,
    0x0C,
    0x00,
    0x34,
    0x08,
    0x10,
    0x30,
    0x04,
    0x10,
    0x08,
    0x00,
    0x10,
    0x20,
    0x1C,
    0x10,
    0x00,
    0x18,
    0x24,
    0x20,
    0x14,
    0x00,
    0x1C,
    0x10,
    0x24,
    0x18,
    0x04,
    0x00,
    0x14,
    0x28,
    0x28,
    0x10,
    0x04,
    0x28,
    0x28,
    0x20,
    0x20,
    0x08,
    0x1C,
    0x20,
    0x20,
    0x18,
    0x0C,
    0x00,
    0x14,
    0x28,
    0x24,
    0x10,
    0x04,
    0x20,
    0x2C,
    0x20,
    0x24,
    0x08,
    0x10,
    0x20,
    0x24,
    0x10,
    0x08,
    0x1F,
    0x0B,
    0x37,
    0x23,
    0x0F,
    0x1B,
    0x27,
    0x33,
    0x3F,
    0x2B,
    0x17,
    0x03,
    0x2F,
    0x3B,
    0x07,
    0x13,
    0x1F,
    0x0B,
    0x37,
    0x23,
    0x0F,
    0x1B,
    0x27,
    0x33,
    0x3F,
    0x2B,
    0x17,
    0x03,
    0x2F,
    0x3B,
    0x07,
    0x13,
};
/*===========================================================================
    TYPEDEF
===========================================================================*/
typedef struct
{
    unsigned int numThreads;
} hvx_config_t;

typedef struct
{
    worker_synctoken_t *token; // worker pool token
    unsigned int rowsPerJob;   // number of rows to process per multi-threaded job
    hvx_config_t *hvxInfo;     // HVX configuration information
    unsigned int threadCount;  // thread counter shared by all workers
    unsigned char *src;        // source image pointer
    unsigned int srcWidth;     // source image width
    unsigned int height;       // number of output rows
    unsigned int srcstride;    // input image height stride
    unsigned int dststride;    // output image height stride
    unsigned char *dst;        // destination image pointer

} rgb2yuv_callback_t;

typedef struct
{
    worker_synctoken_t *token; // worker pool token
    unsigned int rowsPerJob;   // number of rows to process per multi-threaded job
    hvx_config_t *hvxInfo;     // HVX configuration information
    unsigned int threadCount;  // thread counter shared by all workers
    unsigned char *src;        // source image pointer
    unsigned int srcWidth;     // source image width
    unsigned int height;       // number of output rows
    unsigned int srcstride;    // input image height stride
    unsigned int dststride;    // output image height stride
    unsigned char *dst;        // destination image pointer

} yuv2rgb_callback_t;

/*===========================================================================
    FUNCTION
===========================================================================*/
void RGB2YUV12_lastthread(unsigned char *src, int src_w, int src_h, int src_stride, unsigned char *dst, unsigned char *dstuv, int height, int dst_stride)
{

    int i, j;
    unsigned char *L2FETCH_ADDR = src + 2 * src_stride;

    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA = CreateL2pfParam(src_stride, src_w * 3, 2, 1);
    HVX_Vector src00, src01, src02, vrdelta0, vrdelta1, vrdelta2, vrdelta3;
    HEXAGON_Vect32 uc = 0x40400, yc = 0x8400;
    HVX_Vector V_uc = Q6_V_vsplat_R(uc);     // uc
    HVX_Vector V_yc = Q6_V_vsplat_R(yc);     // yc
    int g2y_r2y = 0x408020E;                 // g2y| r2y
    int b2y = 0xc9;                          // 0| b2y
    int g2v_r2v = 0xFD0E0383;                // g2v| r2v
    int b2v = 0xFF6F;                        // 0| b2v
    int g2u_r2u = 0xFDACFED1;                // g2u| r2u
    int b2u = 0x383;                         // 0| b2u
    HVX_VectorPred q1 = Q6_Q_vsetq_R(src_w); //
    HVX_Vector *psCtrl = (HVX_Vector *)CtrlPerm;
    HVX_Vector sCtrlMSB_A = psCtrl[0];
    HVX_Vector sCtrlMSB_B = psCtrl[1];
    // printf("out_height_stride = %d\n",out_height_stride);
    int loop_w = src_w >> 7;
    for (j = 0; j < src_h - 2; j += 2)
    {
        if (j + 2 < src_h)
        {
            L2fetch((unsigned int)(L2FETCH_ADDR), L2FETCH_PARA);
            L2FETCH_ADDR += 2 * src_stride;
        }
        HEXAGON_Vect1024_UN *inp0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inp1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *outpY0 = (HEXAGON_Vect1024_UN *)(dst + j * dst_stride);
        HEXAGON_Vect1024_UN *outpY1 = (HEXAGON_Vect1024_UN *)(dst + (j + 1) * dst_stride);
        HEXAGON_Vect1024_UN *outpUV = (HEXAGON_Vect1024_UN *)(dstuv + (j >> 1) * dst_stride);
        for (i = src_w; i > VLEN; i -= VLEN) // src_w =src_R_w =src_G_w=src_B_w
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V0, outVU_U1, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            *outpY0++ = Q6_V_lo_W(out_tmp23);
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V2, outVU_U3, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // U 0 16 32 48 V 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpUV++ = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpY1++ = Q6_V_lo_W(out_tmp01);
        }
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V0, outVU_U1, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            if ((src_w & 127) == 0)
            {
                *outpY0 = Q6_V_lo_W(out_tmp23);
            }
            else
            {
                HVX_Vector v128_out = *outpY0;
                *outpY0 = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp23), v128_out);
            }
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V2, outVU_U3, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // U 0 16 32 48 V 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            if ((src_w & 127) == 0)
            {
                *outpUV = Q6_V_lo_W(out_tmp01);
            }
            else
            {
                HVX_Vector v128_out = *outpUV;
                *outpUV = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp01), v128_out);
            }

            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            if ((src_w & 127) == 0)
            {
                *outpY1 = Q6_V_lo_W(out_tmp01);
            }
            else
            {
                HVX_Vector v128_out = *outpY1;
                *outpY1 = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp01), v128_out);
            }
        }
    }
    {

        HEXAGON_Vect1024_UN *inp0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inp1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *outpY0 = (HEXAGON_Vect1024_UN *)(dst + j * dst_stride);
        HEXAGON_Vect1024_UN *outpY1 = (HEXAGON_Vect1024_UN *)(dst + (j + 1) * dst_stride);
        HEXAGON_Vect1024_UN *outpUV = (HEXAGON_Vect1024_UN *)(dstuv + (j >> 1) * dst_stride);
        for (i = loop_w; i > 0; i -= 1) // src_w =src_R_w =src_G_w=src_B_w
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V0, outVU_U1, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            *outpY0++ = Q6_V_lo_W(out_tmp23);
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V2, outVU_U3, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // U 0 16 32 48 U 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpUV++ = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpY1++ = Q6_V_lo_W(out_tmp01);
        }
        int w_rest = src_w & 127;
        unsigned char *_line0 = (unsigned char *)inp0;
        unsigned char *_line1 = (unsigned char *)inp1;
        unsigned char *_y0 = (unsigned char *)outpY0;
        unsigned char *_y1 = (unsigned char *)outpY1;
        unsigned char *_u = (unsigned char *)outpUV;
        unsigned char *_v = _u + 1;
        if (w_rest >= 86) // last
        {
            src00 = *inp0++; // check the data right or not. and check the address ++?
            w_rest = w_rest - 64;
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            unsigned char *y00_ptr = (unsigned char *)&out0;
            for (int l = 0; l < 16; l++)
                for (int k = 0; k < 4; k++)
                {
                    _y0[k * 16 + l] = *y00_ptr;
                    y00_ptr += 2;
                }

            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            unsigned char *v_ptr = (unsigned char *)&outVU_V0;
            for (int l = 0; l < 16; l = l + 2)
            {
                for (int k = 0; k < 4; k++)
                {
                    _v[k * 16 + l] = *v_ptr;
                    v_ptr += 2;
                }
                v_ptr += 8;
            }
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63

            unsigned char *u_ptr = (unsigned char *)&outVU_U1;
            for (int l = 0; l < 16; l = l + 2)
            {
                for (int k = 0; k < 4; k++)
                {
                    _u[k * 16 + l] = *u_ptr;
                    u_ptr += 2;
                }
                u_ptr += 8;
            }
            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            unsigned char *y01_ptr = (unsigned char *)&out0;
            for (int l = 0; l < 16; l++)
                for (int k = 0; k < 4; k++)
                {
                    _y1[k * 16 + l] = *y01_ptr;
                    y01_ptr += 2;
                }
            _line0 += 192;
            _line1 += 192;
            _y0 += 64;
            _y1 += 64;
            _u += 64;
            _v += 64;
        }
        const int shift_value = 11;
        const short int r2y = 526;
        const short int g2y = 1032;
        const short int b2y = 201;
        const short int r2u = -303;
        const short int g2u = -596;
        const short int b2u = 899;
        const short int r2v = 899;
        const short int g2v = -754;
        const short int b2v = -145;
        const int yc = 33792;
        const int uc = 263168;

        int y00, y01, y10, y11;
        int u00, v00;

        for (int i = 0; i < w_rest; i += 2)
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
}

void RGB2YUV12(unsigned char *src, int src_w, int src_h, int src_stride, unsigned char *dst, unsigned char *dstuv, int height, int dst_stride)
{
    int i, j;
    unsigned char *L2FETCH_ADDR = src + 2 * src_stride;

    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA = CreateL2pfParam(src_stride, src_w * 3, 2, 1);
    HVX_Vector src00, src01, src02, vrdelta0, vrdelta1, vrdelta2, vrdelta3;
    HEXAGON_Vect32 uc = 0x40400, yc = 0x8400;
    HVX_Vector V_uc = Q6_V_vsplat_R(uc);     // uc
    HVX_Vector V_yc = Q6_V_vsplat_R(yc);     // yc
    int g2y_r2y = 0x408020E;                 // g2y| r2y
    int b2y = 0xc9;                          // 0| b2y
    int g2v_r2v = 0xFD0E0383;                // g2v| r2v
    int b2v = 0xFF6F;                        // 0| b2v
    int g2u_r2u = 0xFDACFED1;                // g2u| r2u
    int b2u = 0x383;                         // 0| b2u
    HVX_VectorPred q1 = Q6_Q_vsetq_R(src_w); //
    HVX_Vector *psCtrl = (HVX_Vector *)CtrlPerm;
    HVX_Vector sCtrlMSB_A = psCtrl[0];
    HVX_Vector sCtrlMSB_B = psCtrl[1];
    // printf("out_height_stride = %d\n",out_height_stride);
    for (j = 0; j < src_h; j += 2)
    {
        if (j + 2 < src_h)
        {
            L2fetch((unsigned int)(L2FETCH_ADDR), L2FETCH_PARA);
            L2FETCH_ADDR += 2 * src_stride;
        }
        HEXAGON_Vect1024_UN *inp0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inp1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *outpY0 = (HEXAGON_Vect1024_UN *)(dst + j * dst_stride);
        HEXAGON_Vect1024_UN *outpY1 = (HEXAGON_Vect1024_UN *)(dst + (j + 1) * dst_stride);
        HEXAGON_Vect1024_UN *outpUV = (HEXAGON_Vect1024_UN *)(dstuv + (j >> 1) * dst_stride);
        for (i = src_w; i > VLEN; i -= VLEN) // src_w =src_R_w =src_G_w=src_B_w
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V0, outVU_U1, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            *outpY0++ = Q6_V_lo_W(out_tmp23);
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V2, outVU_U3, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :u 0 16 32 48 V 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // U 0 16 32 48 V 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpUV++ = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////

            src00 = *inp1++;
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpY1++ = Q6_V_lo_W(out_tmp01);
        }
        {

            src00 = *inp0++;

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V0, outVU_U1, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            if ((src_w & 127) == 0)
            {
                *outpY0 = Q6_V_lo_W(out_tmp23);
            }
            else
            {
                HVX_Vector v128_out = *outpY0;
                *outpY0 = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp23), v128_out);
            }
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_V2, outVU_U3, 8);                        // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :U 0 16 32 48 V 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // U 0 16 32 48 V 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            if ((src_w & 127) == 0)
            {
                *outpUV = Q6_V_lo_W(out_tmp01);
            }
            else
            {
                HVX_Vector v128_out = *outpUV;
                *outpUV = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp01), v128_out);
            }

            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            if ((src_w & 127) == 0)
            {
                *outpY1 = Q6_V_lo_W(out_tmp01);
            }
            else
            {
                HVX_Vector v128_out = *outpY1;
                *outpY1 = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp01), v128_out);
            }
        }
    }
}

void RGB2YUV21_lastthread(unsigned char *src, int src_w, int src_h, int src_stride, unsigned char *dst, unsigned char *dstuv, int height, int dst_stride)
{

    /*
    r2y= 526    0x20E
    g2y= 1032   0x408
    b2y= 201    0xc9
    r2u= -303	0xFED1
    g2u= -596	0xFDAC
    b2u= 899 	0x‭383‬
    r2v= 899 	0x‭383‬
    g2v= -754	0xFD0E
    b2v= -145	0xFF6F
    yc= 33792	 0x8400
    uc= 263168	  0x4 0400
    */

    int i, j;
    unsigned char *L2FETCH_ADDR = src + 2 * src_stride;

    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA = CreateL2pfParam(src_stride, src_w * 3, 2, 1);
    HVX_Vector src00, src01, src02, vrdelta0, vrdelta1, vrdelta2, vrdelta3;
    HEXAGON_Vect32 uc = 0x40400, yc = 0x8400;
    HVX_Vector V_uc = Q6_V_vsplat_R(uc);     // uc
    HVX_Vector V_yc = Q6_V_vsplat_R(yc);     // yc
    int g2y_r2y = 0x408020E;                 // g2y| r2y
    int b2y = 0xc9;                          // 0| b2y
    int g2v_r2v = 0xFD0E0383;                // g2v| r2v
    int b2v = 0xFF6F;                        // 0| b2v
    int g2u_r2u = 0xFDACFED1;                // g2u| r2u
    int b2u = 0x383;                         // 0| b2u
    HVX_VectorPred q1 = Q6_Q_vsetq_R(src_w); //
    HVX_Vector *psCtrl = (HVX_Vector *)CtrlPerm;
    HVX_Vector sCtrlMSB_A = psCtrl[0];
    HVX_Vector sCtrlMSB_B = psCtrl[1];
    // printf("out_height_stride = %d\n",out_height_stride);
    int loop_w = src_w >> 7;
    for (j = 0; j < src_h - 2; j += 2)
    {
        if (j + 2 < src_h)
        {
            L2fetch((unsigned int)(L2FETCH_ADDR), L2FETCH_PARA);
            L2FETCH_ADDR += 2 * src_stride;
        }
        HEXAGON_Vect1024_UN *inp0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inp1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *outpY0 = (HEXAGON_Vect1024_UN *)(dst + j * dst_stride);
        HEXAGON_Vect1024_UN *outpY1 = (HEXAGON_Vect1024_UN *)(dst + (j + 1) * dst_stride);
        HEXAGON_Vect1024_UN *outpUV = (HEXAGON_Vect1024_UN *)(dstuv + (j >> 1) * dst_stride);
        for (i = src_w; i > VLEN; i -= VLEN) // src_w =src_R_w =src_G_w=src_B_w
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U1, outVU_V0, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            *outpY0++ = Q6_V_lo_W(out_tmp23);
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U3, outVU_V2, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // V 0 16 32 48 U 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpUV++ = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpY1++ = Q6_V_lo_W(out_tmp01);
        }
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U1, outVU_V0, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            if ((src_w & 127) == 0)
            {
                *outpY0 = Q6_V_lo_W(out_tmp23);
            }
            else
            {
                HVX_Vector v128_out = *outpY0;
                *outpY0 = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp23), v128_out);
            }
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U3, outVU_V2, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // V 0 16 32 48 U 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            if ((src_w & 127) == 0)
            {
                *outpUV = Q6_V_lo_W(out_tmp01);
            }
            else
            {
                HVX_Vector v128_out = *outpUV;
                *outpUV = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp01), v128_out);
            }

            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            if ((src_w & 127) == 0)
            {
                *outpY1 = Q6_V_lo_W(out_tmp01);
            }
            else
            {
                HVX_Vector v128_out = *outpY1;
                *outpY1 = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp01), v128_out);
            }
        }
    }
    {

        HEXAGON_Vect1024_UN *inp0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inp1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *outpY0 = (HEXAGON_Vect1024_UN *)(dst + j * dst_stride);
        HEXAGON_Vect1024_UN *outpY1 = (HEXAGON_Vect1024_UN *)(dst + (j + 1) * dst_stride);
        HEXAGON_Vect1024_UN *outpUV = (HEXAGON_Vect1024_UN *)(dstuv + (j >> 1) * dst_stride);
        for (i = loop_w; i > 0; i -= 1) // src_w =src_R_w =src_G_w=src_B_w
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U1, outVU_V0, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            *outpY0++ = Q6_V_lo_W(out_tmp23);
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U3, outVU_V2, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // V 0 16 32 48 U 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpUV++ = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpY1++ = Q6_V_lo_W(out_tmp01);
        }
        int w_rest = src_w & 127;
        unsigned char *_line0 = (unsigned char *)inp0;
        unsigned char *_line1 = (unsigned char *)inp1;
        unsigned char *_y0 = (unsigned char *)outpY0;
        unsigned char *_y1 = (unsigned char *)outpY1;
        unsigned char *_v = (unsigned char *)outpUV;
        unsigned char *_u = _v + 1;
        if (w_rest >= 86) // last
        {
            src00 = *inp0++; // check the data right or not. and check the address ++?
            w_rest = w_rest - 64;

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            unsigned char *y00_ptr = (unsigned char *)&out0;
            for (int l = 0; l < 16; l++)
                for (int k = 0; k < 4; k++)
                {
                    _y0[k * 16 + l] = *y00_ptr;
                    y00_ptr += 2;
                }

            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            unsigned char *v_ptr = (unsigned char *)&outVU_V0;
            for (int l = 0; l < 16; l = l + 2)
            {
                for (int k = 0; k < 4; k++)
                {
                    _v[k * 16 + l] = *v_ptr;
                    v_ptr += 2;
                }
                v_ptr += 8;
            }
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63

            unsigned char *u_ptr = (unsigned char *)&outVU_U1;
            for (int l = 0; l < 16; l = l + 2)
            {
                for (int k = 0; k < 4; k++)
                {
                    _u[k * 16 + l] = *u_ptr;
                    u_ptr += 2;
                }
                u_ptr += 8;
            }
            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            unsigned char *y01_ptr = (unsigned char *)&out0;
            for (int l = 0; l < 16; l++)
                for (int k = 0; k < 4; k++)
                {
                    _y1[k * 16 + l] = *y01_ptr;
                    y01_ptr += 2;
                }
            _line0 += 192;
            _line1 += 192;
            _y0 += 64;
            _y1 += 64;
            _u += 64;
            _v += 64;
        }
        const int shift_value = 11;
        const short int r2y = 526;
        const short int g2y = 1032;
        const short int b2y = 201;
        const short int r2u = -303;
        const short int g2u = -596;
        const short int b2u = 899;
        const short int r2v = 899;
        const short int g2v = -754;
        const short int b2v = -145;
        const int yc = 33792;
        const int uc = 263168;

        int y00, y01, y10, y11;
        int u00, v00;

        for (int i = 0; i < w_rest; i += 2)
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
}

void RGB2YUV21(unsigned char *src, int src_w, int src_h, int src_stride, unsigned char *dst, unsigned char *dstuv, int height, int dst_stride)
{
    int i, j;
    unsigned char *L2FETCH_ADDR = src + 2 * src_stride;

    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA = CreateL2pfParam(src_stride, src_w * 3, 2, 1);
    HVX_Vector src00, src01, src02, vrdelta0, vrdelta1, vrdelta2, vrdelta3;
    HEXAGON_Vect32 uc = 0x40400, yc = 0x8400;
    HVX_Vector V_uc = Q6_V_vsplat_R(uc);     // uc
    HVX_Vector V_yc = Q6_V_vsplat_R(yc);     // yc
    int g2y_r2y = 0x408020E;                 // g2y| r2y
    int b2y = 0xc9;                          // 0| b2y
    int g2v_r2v = 0xFD0E0383;                // g2v| r2v
    int b2v = 0xFF6F;                        // 0| b2v
    int g2u_r2u = 0xFDACFED1;                // g2u| r2u
    int b2u = 0x383;                         // 0| b2u
    HVX_VectorPred q1 = Q6_Q_vsetq_R(src_w); //
    HVX_Vector *psCtrl = (HVX_Vector *)CtrlPerm;
    HVX_Vector sCtrlMSB_A = psCtrl[0];
    HVX_Vector sCtrlMSB_B = psCtrl[1];
    // printf("out_height_stride = %d\n",out_height_stride);
    for (j = 0; j < src_h; j += 2)
    {
        if (j + 2 < src_h)
        {
            L2fetch((unsigned int)(L2FETCH_ADDR), L2FETCH_PARA);
            L2FETCH_ADDR += 2 * src_stride;
        }
        HEXAGON_Vect1024_UN *inp0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inp1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *outpY0 = (HEXAGON_Vect1024_UN *)(dst + j * dst_stride);
        HEXAGON_Vect1024_UN *outpY1 = (HEXAGON_Vect1024_UN *)(dst + (j + 1) * dst_stride);
        HEXAGON_Vect1024_UN *outpUV = (HEXAGON_Vect1024_UN *)(dstuv + (j >> 1) * dst_stride);
        for (i = src_w; i > VLEN; i -= VLEN) // src_w =src_R_w =src_G_w=src_B_w
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U1, outVU_V0, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            *outpY0++ = Q6_V_lo_W(out_tmp23);
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U3, outVU_V2, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // V 0 16 32 48 U 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpUV++ = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            *outpY1++ = Q6_V_lo_W(out_tmp01);
        }
        {

            src00 = *inp0++; // check the data right or not. and check the address ++?

            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            HVX_VectorPair out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_VectorPair SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));    // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01));               // 2 3 130 131 6 7 134 135
            HVX_Vector out0 = V_yc;
            HVX_Vector out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V0 = V_uc;
            HVX_Vector outVU_V1 = V_uc;
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V0, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V1, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V0 = Q6_Vh_vasr_VwVwR(outVU_V1, outVU_V0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U0 = V_uc;
            HVX_Vector outVU_U1 = V_uc;
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U0 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U0, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U1 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U1, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U1 = Q6_Vh_vasr_VwVwR(outVU_U1, outVU_U0, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U1, outVU_V0, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V0 = Q6_V_lo_W(out_tmp01);
            ///////////////////////////////////////
            src02 = *inp0++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            HVX_VectorPair out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            HVX_Vector out2 = V_yc;
            HVX_Vector out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp23 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp23 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp23), Q6_V_lo_W(out_tmp23), 64);
            if ((src_w & 127) == 0)
            {
                *outpY0 = Q6_V_lo_W(out_tmp23);
            }
            else
            {
                HVX_Vector v128_out = *outpY0;
                *outpY0 = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp23), v128_out);
            }
            /////////////////////////////////////////////////////////////////////////////
            // v00 = uc + r2v * _line0[0] + g2v * _line0[1] + b2v * _line0[2];
            // u00 = uc + r2u * _line0[0] + g2u * _line0[1] + b2u * _line0[2];
            HVX_Vector outVU_V2 = V_uc;
            HVX_Vector outVU_V3 = V_uc;
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V2, Q6_V_lo_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(SRC_h), g2v_r2v); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_V3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_V3, Q6_V_hi_W(out_tmp01), b2v); // y00  0 32 1 33.... 15 47
            outVU_V2 = Q6_Vh_vasr_VwVwR(outVU_V3, outVU_V2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            HVX_Vector outVU_U2 = V_uc;
            HVX_Vector outVU_U3 = V_uc;
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U2 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U2, Q6_V_lo_W(out_tmp01), b2u); // y00  0 32 1 33.... 15 47
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(SRC_h), g2u_r2u); // yc + r2y * _line0[0] + g2y * _line0[1]
            outVU_U3 = Q6_Vw_vdmpyacc_VwVhRh_sat(outVU_U3, Q6_V_hi_W(out_tmp01), b2u); // y00  16 48 17 49.... 31 63
            outVU_U3 = Q6_Vh_vasr_VwVwR(outVU_U3, outVU_U2, 11);                       // 0 16 32 48 1 17 33 49 ...15 31 47 63
            out_tmp01 = Q6_W_vshuff_VVR(outVU_U3, outVU_V2, 8);                        // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63
            outVU_V2 = Q6_V_lo_W(out_tmp01);                                           // LOW :V 0 16 32 48 U 0 16 32 48  ...15 31 47 63

            out0 = Q6_Vb_vpacke_VhVh(outVU_V2, outVU_V0); // V 0 16 32 48 U 0 16 32 48 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 32 1 33... 15 47  16 48 ... 31 63
            out0 = Q6_Vb_vdeal_Vb(out0);                  // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            if ((src_w & 127) == 0)
            {
                *outpUV = Q6_V_lo_W(out_tmp01);
            }
            else
            {
                HVX_Vector v128_out = *outpUV;
                *outpUV = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp01), v128_out);
            }

            ///////////////////////////////////////

            src00 = *inp1++; // check the data right or not. and check the address ++?
            // 0 ,1 ,2 ,3 ,64,4 ,5 ,6 ,7 ,66,8 ,9 ,10,11,68,12,13,14,15,70,16,17,18,19,72,20,21,22,23,74,24,25,26,27,
            // 76,28,29,30,31,78,32,33,34,35,80,36,37,38,39,82,40,41,42,43,84,44,45,46,47,86,48,49,50,51,88,52,53,54,
            // 55,90,56,57,58,59,92,60,61,62,63,94,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,96,97,98,99,100,101,
            // 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
            vrdelta0 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta0 = Q6_V_vdelta_VV(vrdelta0, sCtrlMSB_B); // mpy 0 - 95
            src01 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src01, src00, 32); //
            vrdelta1 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta1 = Q6_V_vdelta_VV(vrdelta1, sCtrlMSB_B);
            out_tmp01 = Q6_Wh_vshuffoe_VhVh(vrdelta1, vrdelta0);  // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp01));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp01)); // 2 3 130 131 6 7 134 135
            out0 = V_yc;
            out1 = V_yc;
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out0 = Q6_Vw_vdmpyacc_VwVhRh_sat(out0, Q6_V_lo_W(out_tmp01), b2y); // y00  0 32 1 33.... 15 47
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out1 = Q6_Vw_vdmpyacc_VwVhRh_sat(out1, Q6_V_hi_W(out_tmp01), b2y); // y00  16 48 17 49.... 31 63
            out0 = Q6_Vh_vasr_VwVwR(out1, out0, 11);                           // 0 16 32 48 1 17 33 49 ...15 31 47 63
            src02 = *inp1++;
            src00 = Q6_V_vlalign_VVR(src02, src01, 64);
            vrdelta2 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta2 = Q6_V_vdelta_VV(vrdelta2, sCtrlMSB_B);
            src00 = Q6_V_vror_VR(src02, 32);
            vrdelta3 = Q6_V_vrdelta_VV(src00, sCtrlMSB_A);
            vrdelta3 = Q6_V_vdelta_VV(vrdelta3, sCtrlMSB_B);
            out_tmp23 = Q6_Wh_vshuffoe_VhVh(vrdelta3, vrdelta2); // 0 1 128 129 4 5 132 133 | 2 3 130 131 6 7 134 135
            out2 = V_yc;
            out3 = V_yc;
            SRC_h = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(out_tmp23));     // 0 1 128 129 4 5 132 133
            out_tmp01 = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(out_tmp23)); // 2 3 130 131 6 7 134 135

            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out2 = Q6_Vw_vdmpyacc_VwVhRh_sat(out2, Q6_V_lo_W(out_tmp01), b2y); // y00	64 96 65 97.... 79 111
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(SRC_h), g2y_r2y); // yc + r2y * _line0[0] + g2y * _line0[1]
            out3 = Q6_Vw_vdmpyacc_VwVhRh_sat(out3, Q6_V_hi_W(out_tmp01), b2y); // y00	80 112 82 113.... 95 127
            out2 = Q6_Vh_vasr_VwVwR(out3, out2, 11);                           // 64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vpacke_VhVh(out2, out0);                              // 0 16 32 48 1 17 33 49 ...15 31 47 63  64 80 96 112 65 81 97 113 ...79 95 111 127
            out0 = Q6_Vb_vdeal_Vb(out0);                                       // 0 32 1 33... 15 47  16 48 ... 31 63
            // src00 = Q6_V_vror_VR(out0, 64);// ABCD -> CDAB
            // out_tmp01 = Q6_W_vshuff_VVR(src00, out0, 32);// ABCD -> BDDB | ACCA
            // out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);// ABCD -> BDDB | ACCA
            // out0=Q6_Vb_vdeal_Vb(Q6_V_lo_W(out_tmp01)); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out0 = Q6_Vb_vdeal_Vb(out0); // 0 1 2 3 .. 15  16 ...31   32  ...47 48 ...63   ...127
            out_tmp01 = Q6_W_vdeal_VVR(out0, out0, -16);
            out_tmp01 = Q6_W_vshuff_VVR(Q6_V_hi_W(out_tmp01), Q6_V_lo_W(out_tmp01), 64);
            if ((src_w & 127) == 0)
            {
                *outpY1 = Q6_V_lo_W(out_tmp01);
            }
            else
            {
                HVX_Vector v128_out = *outpY1;
                *outpY1 = Q6_V_vmux_QVV(q1, Q6_V_lo_W(out_tmp01), v128_out);
            }
        }
    }
}

void YUV12ToRGB_lastthread(unsigned char *src, unsigned char *srcuv, int src_w, int src_h, int src_stride, unsigned char *dst, int height, int dst_stride)
{
    // FARF(ERROR,"enter YUV12ToRGB_lastthread");
    int i, j;
    unsigned char *L2FETCH_ADDR_src = src + 2 * src_stride;
    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA_src = CreateL2pfParam(src_stride, src_w, 2, 1);

    unsigned char *L2FETCH_ADDR_srcuv = srcuv + src_stride;
    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA_srcuv = CreateL2pfParam(src_stride, src_w, 1, 1);
    HVX_Vector V_srcY0, V_srcuv;
    int y2rgb = 0x00950095; // 149;  //0x0095
    int u2b = 0x1020102;    // 258;   //0x102
    int u2g = 0xFFCEFFCE;   //-50;   //0xFFCE
    int v2g = 0xFF98FF98;   //-104;  //0xFF98‬
    int v2r = 0xcc00cc;     // 204;   //0xcc
    HVX_Vector v_max_255 = Q6_V_vsplat_R(0x7F80);
    /*
    int v2r_y2rgb = 0xcc0095 ;
    int v2g_y2rgb = 0xFF980095;
    int u2b_y2rgb = 0x1020095;
    int u2g_y2rgb = 0xFFCE0095;
    */
    int src_w_rest = src_w & 127;
    int src_w_rest_3 = src_w_rest * 3;
    HVX_VectorPred q1 = Q6_Q_vsetq_R(96); //
    HVX_Vector *psCtrl = (HVX_Vector *)CtrlPerm_YUV2RGB;
    HVX_Vector sCtrlMSB_A = psCtrl[0];
    HVX_Vector sCtrlMSB_B = psCtrl[1];
    HVX_Vector v_128 = Q6_V_vsplat_R(0x80808080);
    HVX_Vector v_16 = Q6_V_vsplat_R(0x10101010);
    HVX_Vector V_1_shift_value = Q6_V_vsplat_R(64); // 64 (1 << (shift_value_yuv2bgr - 1))
    // printf("out_height_stride = %d\n",out_height_stride);
    for (j = 0; j < src_h - 2; j += 2)
    {
        if (j + 2 < src_h)
        {
            L2fetch((unsigned int)(L2FETCH_ADDR_src), L2FETCH_PARA_src);
            L2FETCH_ADDR_src += 2 * src_stride;
            L2fetch((unsigned int)(L2FETCH_ADDR_srcuv), L2FETCH_PARA_srcuv);
            L2FETCH_ADDR_srcuv += src_stride;
        }
        HEXAGON_Vect1024_UN *inpY0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inpY1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *inpuv = (HEXAGON_Vect1024_UN *)(srcuv + (j >> 1) * src_stride);
        unsigned char *outp0 = (unsigned char *)(dst + j * dst_stride);
        unsigned char *outp1 = (unsigned char *)(dst + (j + 1) * dst_stride);
        // unsigned char * ori_ptr = (unsigned char *)(dst+ dst_stride);
        // unsigned char * ori_odd_ptr = (unsigned char *)(dst+(j+1)*dst_stride);
        // FARF(ERROR,"%p %p %p %d",outp0,outp1,ori_ptr,dst_stride);
        for (i = src_w; i > VLEN; i -= VLEN) // src_w =src_R_w =src_G_w=src_B_w
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low u high v
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            // FARF(ERROR,"j %d j+1 %d",j,j%2);
            // if ((j+1)%2 == 1){
            //	FARF(ERROR,"input0");
            //	v_print_char(*inpY0);
            // }

            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_hi_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_hi_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_lo_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_lo_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);

            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;

            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////
            // if ((j+1)%2 ==1){
            //	FARF(ERROR,"input1");
            //	v_print_char(*inpY1);
            // }
            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            // HVX_Vector debug =  Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            // if ((j+1)%2 ==1){
            //	FARF(ERROR,"out0");
            //	v_print_char(debug);
            // }
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;

            // int debug_count = outp1 - ori_ptr;
            // FARF(ERROR,"debug count %d",debug_count);
            // if (debug_count ==  96 * 4){
            // for(int debug_loop = 0; debug_loop < debug_count;debug_loop ++){
            //		printf("0x%p %d\n",ori_ptr + debug_loop,*(ori_ptr +debug_loop));
            //	}
            // }
        }

        // what's next code mean, get the &127
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low u high v
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_hi_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_hi_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_lo_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_lo_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2); // RGB
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            // FARF(ERROR,"src_w_rest %d",src_w_rest);

            if (0 == src_w_rest)
            {

                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                ;
                out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                HVX_Vector v128_out = vmemu(outp0);
                vmemu(outp0) = Q6_V_vmux_QVV(q1, out0, v128_out);
                outp0 += 96;
            }
            else
            {
                if (src_w_rest < 32)
                {
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_3); //
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 64)
                {
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 96;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 96)
                {

                    // FARF(ERROR,"addr %p",outp0);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B); //+ 128
                    outp0 += 96;                                     // +96

                    ////ruzhongl solution 1,store odd data first, then write back
                    // HVX_Vector overflow = vmemu(ori_odd_ptr); //read as backup
                    ////ruzhongl end of solution 1

                    // ruzhongl solution 2,store odd data first, then write back
                    HVX_Vector overflow = vmemu(outp0); // keep 96 + 32
                    // ruzhong2 end of solution 1

                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B); // croupt here

                    ////ruzhongl solution 1,store odd data first, then write back
                    // vmemu(ori_odd_ptr) = overflow;
                    ////ruzhongl end of solution 1

                    outp0 += 96; // try to odd line fist 128 bytes,as the first 32 bytes are corrupted,use overflow instead
                    // int src_w_rest_q0 = src_w_rest_3-192; // 64 * 3 - 192
                    // int src_w_rest_q0 = 96; // 64 * 3 - 192

                    overflow = Q6_V_vror_VR(overflow, 96); // move high 32 bytes to low 32 bytes
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(32);  // set mutxt
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, overflow, v128_out);

                    // outp0 +=96;// try to recover?
                    // int src_w_rest_q0 = src_w_rest_3-192; // 64 * 3 - 192
                    // HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0); // q0 = 0
                    // outtmp0 = Q6_W_vshuff_VVR( Q6_V_hi_W(V_b),Q6_V_hi_W(rg04), -2); // rgb
                    // out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    // out0= Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    // HVX_Vector v128_out = vmemu(outp0);
                    // vmemu(outp0)  = Q6_V_vmux_QVV(q0,out0, v128_out);
                }

                else if (src_w_rest < 128)
                {

                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 288;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
            }
            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            // FARF(ERROR,"ouputp1 addr %p",outp1);

            if (0 == src_w_rest)
            {

                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                ;
                out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                HVX_Vector v128_out = vmemu(outp1);
                vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
            }
            else
            {
                if (src_w_rest < 32)
                {
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_3); //
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 64)
                {
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 96;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 96)
                {
                    // FARF(ERROR,"enter overleft");
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 192;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }

                else if (src_w_rest < 128)
                {

                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
                    outp1 += 96;

                    int src_w_rest_q0 = src_w_rest_3 - 288;

                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
            }
        }
    }
    {
        HEXAGON_Vect1024_UN *inpY0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inpY1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *inpuv = (HEXAGON_Vect1024_UN *)(srcuv + (j >> 1) * src_stride);
        unsigned char *outp0 = (unsigned char *)(dst + j * dst_stride);
        unsigned char *outp1 = (unsigned char *)(dst + (j + 1) * dst_stride);
        int loop_w = src_w >> 7;
        for (i = loop_w; i > 0; i -= 1) // src_w =src_R_w =src_G_w=src_B_w
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low u high v
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_hi_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_hi_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_lo_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_lo_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);

            out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            HVX_Vector v128_out = vmemu(outp0);
            vmemu(outp0) = Q6_V_vmux_QVV(q1, out0, v128_out);
            outp0 += 96;
            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            v128_out = vmemu(outp1);
            vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
            outp1 += 96;
        }

        int r00, r01, r10, r11;
        int g00, g01, g10, g11;
        int b00, b01, b10, b11;

        int y2rgb_r = 149; // 0x0095
        int u2b_r = 258;   // 0x102
        int u2g_r = -50;   // 0xFFCE
        int v2g_r = -104;  // 0xFF98‬
        int v2r_r = 204;   // 0xcc
        int max_255 = 0x7F80;

        int rv, gv, gu, bu;
        int y00_mult, y01_mult, y10_mult, y11_mult;
        unsigned char *_y0 = (unsigned char *)inpY0;
        unsigned char *_y1 = (unsigned char *)inpY1;
        unsigned char *_uv = (unsigned char *)inpuv;

        unsigned char *_r0 = outp0;
        unsigned char *_g0 = _r0 + 1;
        unsigned char *_b0 = _r0 + 2;

        unsigned char *_r1 = outp1;
        unsigned char *_g1 = _r1 + 1;
        unsigned char *_b1 = _r1 + 2;

        for (int i = 0; i < src_w_rest; i += 2)
        {

            rv = (_uv[1] - 128) * v2r_r + (1 << (shift_value_yuv2bgr - 1));
            gv = (_uv[1] - 128) * v2g_r + (1 << (shift_value_yuv2bgr - 1));
            gu = (_uv[0] - 128) * u2g_r;
            bu = (_uv[0] - 128) * u2b_r + (1 << (shift_value_yuv2bgr - 1));
            y00_mult = _MAX((_y0[0] - 16), 0) * y2rgb_r;

            y01_mult = _MAX((_y0[1] - 16), 0) * y2rgb_r;
            y10_mult = _MAX((_y1[0] - 16), 0) * y2rgb_r;
            y11_mult = _MAX((_y1[1] - 16), 0) * y2rgb_r;

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
            _r0[0] = (unsigned char)(r00 >> shift_value_yuv2bgr);
            _r0[3] = (unsigned char)(r01 >> shift_value_yuv2bgr);
            _r1[0] = (unsigned char)(r10 >> shift_value_yuv2bgr);
            _r1[3] = (unsigned char)(r11 >> shift_value_yuv2bgr);

            _g0[0] = (unsigned char)(g00 >> shift_value_yuv2bgr);
            _g0[3] = (unsigned char)(g01 >> shift_value_yuv2bgr);
            _g1[0] = (unsigned char)(g10 >> shift_value_yuv2bgr);
            _g1[3] = (unsigned char)(g11 >> shift_value_yuv2bgr);

            _b0[0] = (unsigned char)(b00 >> shift_value_yuv2bgr);
            _b0[3] = (unsigned char)(b01 >> shift_value_yuv2bgr);
            _b1[0] = (unsigned char)(b10 >> shift_value_yuv2bgr);
            _b1[3] = (unsigned char)(b11 >> shift_value_yuv2bgr);

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
}

void YUV12ToRGB(unsigned char *src, unsigned char *srcuv, int src_w, int src_h, int src_stride, unsigned char *dst, int height, int dst_stride)
{
    // FARF(ERROR,"enter YUV12ToRGB");
    int i, j;
    unsigned char *L2FETCH_ADDR_src = src + 2 * src_stride;
    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA_src = CreateL2pfParam(src_stride, src_w, 2, 1);

    unsigned char *L2FETCH_ADDR_srcuv = srcuv + src_stride;
    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA_srcuv = CreateL2pfParam(src_stride, src_w, 1, 1);
    HVX_Vector V_srcY0, V_srcuv;
    int y2rgb = 0x00950095; // 149;  //0x0095
    int u2b = 0x1020102;    // 258;   //0x102
    int u2g = 0xFFCEFFCE;   //-50;   //0xFFCE
    int v2g = 0xFF98FF98;   //-104;  //0xFF98‬
    int v2r = 0xcc00cc;     // 204;   //0xcc
    HVX_Vector v_max_255 = Q6_V_vsplat_R(0x7F80);
    /*
    int v2r_y2rgb = 0xcc0095 ;
    int v2g_y2rgb = 0xFF980095;
    int u2b_y2rgb = 0x1020095;
    int u2g_y2rgb = 0xFFCE0095;
    */

    int src_w_rest = src_w & 127;
    int src_w_rest_3 = src_w_rest * 3;
    HVX_VectorPred q1 = Q6_Q_vsetq_R(96); //
    HVX_Vector *psCtrl = (HVX_Vector *)CtrlPerm_YUV2RGB;
    HVX_Vector sCtrlMSB_A = psCtrl[0];
    HVX_Vector sCtrlMSB_B = psCtrl[1];
    HVX_Vector v_128 = Q6_V_vsplat_R(0x80808080);
    HVX_Vector v_16 = Q6_V_vsplat_R(0x10101010);
    HVX_Vector V_1_shift_value = Q6_V_vsplat_R(64); // 64 (1 << (shift_value_yuv2bgr - 1))
    // printf("out_height_stride = %d\n",out_height_stride);
    for (j = 0; j < src_h; j += 2)
    {
        if (j + 2 < src_h)
        {
            L2fetch((unsigned int)(L2FETCH_ADDR_src), L2FETCH_PARA_src);
            L2FETCH_ADDR_src += 2 * src_stride;
            L2fetch((unsigned int)(L2FETCH_ADDR_srcuv), L2FETCH_PARA_srcuv);
            L2FETCH_ADDR_srcuv += src_stride;
        }
        HEXAGON_Vect1024_UN *inpY0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inpY1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *inpuv = (HEXAGON_Vect1024_UN *)(srcuv + (j >> 1) * src_stride);
        unsigned char *outp0 = (unsigned char *)(dst + j * dst_stride);
        unsigned char *outp1 = (unsigned char *)(dst + (j + 1) * dst_stride);

        for (i = src_w; i > VLEN; i -= VLEN) // src_w =src_R_w =src_G_w=src_B_w
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low u high v
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_hi_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_hi_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_lo_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_lo_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);

            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;

            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
        }
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low u high v
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_hi_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_hi_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_lo_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_lo_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            if (0 == src_w_rest)
            {
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                ;
                out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                HVX_Vector v128_out = vmemu(outp0);
                vmemu(outp0) = Q6_V_vmux_QVV(q1, out0, v128_out);
                outp0 += 96;
            }
            else
            {
                if (src_w_rest < 32)
                {
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_3); //
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 64)
                {
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 96;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 96)
                {
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;

                    // ruzhongl solution 2,store odd data first, then write back
                    HVX_Vector overflow = vmemu(outp0); // keep 96 + 32
                    // ruzhong2 end of solution 1

                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);

                    outp0 += 96;
                    overflow = Q6_V_vror_VR(overflow, 96); // move high 32 bytes to low 32 bytes
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(32);  // set mutxt
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, overflow, v128_out);

                    // outp0 +=96;
                    // int src_w_rest_q0 = src_w_rest_3-192;
                    // HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    // outtmp0 = Q6_W_vshuff_VVR( Q6_V_hi_W(V_b),Q6_V_hi_W(rg04), -2);;
                    // out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    // out0= Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    // HVX_Vector v128_out = vmemu(outp0);
                    // vmemu(outp0)  = Q6_V_vmux_QVV(q0,out0, v128_out);
                }

                else if (src_w_rest < 128)
                {

                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 288;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
            }
            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);

            if (0 == src_w_rest)
            {

                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                ;
                out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                HVX_Vector v128_out = vmemu(outp1);
                vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
            }
            else
            {
                if (src_w_rest < 32)
                {
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_3); //
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 64)
                {
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 96;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 96)
                {
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;

                    // ruzhongl solution 2,store odd data first, then write back
                    HVX_Vector overflow = vmemu(outp1); // keep 96 + 32
                    // ruzhong2 end of solution 1

                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);

                    outp1 += 96;
                    overflow = Q6_V_vror_VR(overflow, 96); // move high 32 bytes to low 32 bytes
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(32);  // set mutxt
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, overflow, v128_out);

                    // outp1 +=96;
                    // int src_w_rest_q0  = src_w_rest_3-192;
                    // HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    // outtmp0 = Q6_W_vshuff_VVR( Q6_V_hi_W(V_b),Q6_V_hi_W(rg04), -2);;
                    // out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    // out0= Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    // HVX_Vector v128_out = vmemu(outp1);
                    // vmemu(outp1)  = Q6_V_vmux_QVV(q0,out0, v128_out);
                }

                else if (src_w_rest < 128)
                {

                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
                    outp1 += 96;

                    int src_w_rest_q0 = src_w_rest_3 - 288;

                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
            }
        }
    }
}

void YUV21ToRGB_lastthread(unsigned char *src, unsigned char *srcuv, int src_w, int src_h, int src_stride, unsigned char *dst, int height, int dst_stride)
{
    // FARF(ERROR,"enter YUV21ToRGB_lastthread");
    int i, j;
    unsigned char *L2FETCH_ADDR_src = src + 2 * src_stride;
    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA_src = CreateL2pfParam(src_stride, src_w, 2, 1);

    unsigned char *L2FETCH_ADDR_srcuv = srcuv + src_stride;
    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA_srcuv = CreateL2pfParam(src_stride, src_w, 1, 1);
    HVX_Vector V_srcY0, V_srcuv;
    int y2rgb = 0x00950095; // 149;  //0x0095
    int u2b = 0x1020102;    // 258;   //0x102
    int u2g = 0xFFCEFFCE;   //-50;   //0xFFCE
    int v2g = 0xFF98FF98;   //-104;  //0xFF98‬
    int v2r = 0xcc00cc;     // 204;   //0xcc
    HVX_Vector v_max_255 = Q6_V_vsplat_R(0x7F80);
    /*
    int v2r_y2rgb = 0xcc0095 ;
    int v2g_y2rgb = 0xFF980095;
    int u2b_y2rgb = 0x1020095;
    int u2g_y2rgb = 0xFFCE0095;
    */

    int src_w_rest = src_w & 127;
    int src_w_rest_3 = src_w_rest * 3;
    HVX_VectorPred q1 = Q6_Q_vsetq_R(96); //
    HVX_Vector *psCtrl = (HVX_Vector *)CtrlPerm_YUV2RGB;
    HVX_Vector sCtrlMSB_A = psCtrl[0];
    HVX_Vector sCtrlMSB_B = psCtrl[1];
    HVX_Vector v_128 = Q6_V_vsplat_R(0x80808080);
    HVX_Vector v_16 = Q6_V_vsplat_R(0x10101010);
    HVX_Vector V_1_shift_value = Q6_V_vsplat_R(64); // 64 (1 << (shift_value_yuv2bgr - 1))
    // printf("out_height_stride = %d\n",out_height_stride);
    for (j = 0; j < src_h - 2; j += 2)
    {
        if (j + 2 < src_h)
        {
            L2fetch((unsigned int)(L2FETCH_ADDR_src), L2FETCH_PARA_src);
            L2FETCH_ADDR_src += 2 * src_stride;
            L2fetch((unsigned int)(L2FETCH_ADDR_srcuv), L2FETCH_PARA_srcuv);
            L2FETCH_ADDR_srcuv += src_stride;
        }
        HEXAGON_Vect1024_UN *inpY0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inpY1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *inpuv = (HEXAGON_Vect1024_UN *)(srcuv + (j >> 1) * src_stride);
        unsigned char *outp0 = (unsigned char *)(dst + j * dst_stride);
        unsigned char *outp1 = (unsigned char *)(dst + (j + 1) * dst_stride);

        for (i = src_w; i > VLEN; i -= VLEN) // src_w =src_R_w =src_G_w=src_B_w
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low v high u
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_lo_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_lo_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_hi_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_hi_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);

            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;

            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
        }
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low v high u
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_lo_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_lo_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_hi_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_hi_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            if (0 == src_w_rest)
            {

                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                ;
                out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                HVX_Vector v128_out = vmemu(outp0);
                vmemu(outp0) = Q6_V_vmux_QVV(q1, out0, v128_out);
                outp0 += 96;
            }
            else
            {
                if (src_w_rest < 32)
                {
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_3); //
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 64)
                {
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 96;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 96)
                {
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;

                    // ruzhongl solution 2,store odd data first, then write back
                    HVX_Vector overflow = vmemu(outp0); // keep 96 + 32
                    // ruzhong2 end of solution 1

                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);

                    outp0 += 96;
                    overflow = Q6_V_vror_VR(overflow, 96); // move high 32 bytes to low 32 bytes
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(32);  // set mutxt
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, overflow, v128_out);

                    // outp0 +=96;
                    // int src_w_rest_q0 = src_w_rest_3-192;
                    // HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    // outtmp0 = Q6_W_vshuff_VVR( Q6_V_hi_W(V_b),Q6_V_hi_W(rg04), -2);;
                    // out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    // out0= Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    // HVX_Vector v128_out = vmemu(outp0);
                    // vmemu(outp0)  = Q6_V_vmux_QVV(q0,out0, v128_out);
                }

                else if (src_w_rest < 128)
                {

                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 288;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
            }
            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);

            if (0 == src_w_rest)
            {

                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                ;
                out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                HVX_Vector v128_out = vmemu(outp1);
                vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
            }
            else
            {
                if (src_w_rest < 32)
                {
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_3); //
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 64)
                {
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 96;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 96)
                {
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 192;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }

                else if (src_w_rest < 128)
                {

                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
                    outp1 += 96;

                    int src_w_rest_q0 = src_w_rest_3 - 288;

                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
            }
        }
    }
    {

        HEXAGON_Vect1024_UN *inpY0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inpY1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *inpuv = (HEXAGON_Vect1024_UN *)(srcuv + (j >> 1) * src_stride);
        unsigned char *outp0 = (unsigned char *)(dst + j * dst_stride);
        unsigned char *outp1 = (unsigned char *)(dst + (j + 1) * dst_stride);

        int loop_w = src_w >> 7;
        for (i = loop_w; i > 0; i -= 1) // src_w =src_R_w =src_G_w=src_B_w
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low v high u
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_lo_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_lo_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_hi_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_hi_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);

            out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            HVX_Vector v128_out = vmemu(outp0);
            vmemu(outp0) = Q6_V_vmux_QVV(q1, out0, v128_out);
            outp0 += 96;

            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            v128_out = vmemu(outp1);
            vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
            outp1 += 96;
        }
        int r00, r01, r10, r11;
        int g00, g01, g10, g11;
        int b00, b01, b10, b11;

        int y2rgb_r = 149; // 0x0095
        int u2b_r = 258;   // 0x102
        int u2g_r = -50;   // 0xFFCE
        int v2g_r = -104;  // 0xFF98‬
        int v2r_r = 204;   // 0xcc
        int max_255 = 0x7F80;

        int rv, gv, gu, bu;
        int y00_mult, y01_mult, y10_mult, y11_mult;
        unsigned char *_y0 = (unsigned char *)inpY0;
        unsigned char *_y1 = (unsigned char *)inpY1;
        unsigned char *_uv = (unsigned char *)inpuv;

        unsigned char *_r0 = outp0;
        unsigned char *_g0 = _r0 + 1;
        unsigned char *_b0 = _r0 + 2;

        unsigned char *_r1 = outp1;
        unsigned char *_g1 = _r1 + 1;
        unsigned char *_b1 = _r1 + 2;

        for (int i = 0; i < src_w_rest; i += 2)
        {

            rv = (_uv[0] - 128) * v2r_r + (1 << (shift_value_yuv2bgr - 1));
            gv = (_uv[0] - 128) * v2g_r + (1 << (shift_value_yuv2bgr - 1));
            gu = (_uv[1] - 128) * u2g_r;
            bu = (_uv[1] - 128) * u2b_r + (1 << (shift_value_yuv2bgr - 1));

            y00_mult = _MAX((_y0[0] - 16), 0) * y2rgb_r;
            y01_mult = _MAX((_y0[1] - 16), 0) * y2rgb_r;
            y10_mult = _MAX((_y1[0] - 16), 0) * y2rgb_r;
            y11_mult = _MAX((_y1[1] - 16), 0) * y2rgb_r;

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
            _r0[0] = (unsigned char)(r00 >> shift_value_yuv2bgr);
            _r0[3] = (unsigned char)(r01 >> shift_value_yuv2bgr);
            _r1[0] = (unsigned char)(r10 >> shift_value_yuv2bgr);
            _r1[3] = (unsigned char)(r11 >> shift_value_yuv2bgr);

            _g0[0] = (unsigned char)(g00 >> shift_value_yuv2bgr);
            _g0[3] = (unsigned char)(g01 >> shift_value_yuv2bgr);
            _g1[0] = (unsigned char)(g10 >> shift_value_yuv2bgr);
            _g1[3] = (unsigned char)(g11 >> shift_value_yuv2bgr);

            _b0[0] = (unsigned char)(b00 >> shift_value_yuv2bgr);
            _b0[3] = (unsigned char)(b01 >> shift_value_yuv2bgr);
            _b1[0] = (unsigned char)(b10 >> shift_value_yuv2bgr);
            _b1[3] = (unsigned char)(b11 >> shift_value_yuv2bgr);

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
}

void YUV21ToRGB(unsigned char *src, unsigned char *srcuv, int src_w, int src_h, int src_stride, unsigned char *dst, int height, int dst_stride)
{

    int i, j;
    unsigned char *L2FETCH_ADDR_src = src + 2 * src_stride;
    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA_src = CreateL2pfParam(src_stride, src_w, 2, 1);

    unsigned char *L2FETCH_ADDR_srcuv = srcuv + src_stride;
    // next prefetches will just add 8*VLEN/2*4 int 16
    long long L2FETCH_PARA_srcuv = CreateL2pfParam(src_stride, src_w, 1, 1);
    HVX_Vector V_srcY0, V_srcuv;
    int y2rgb = 0x00950095; // 149;  //0x0095
    int u2b = 0x1020102;    // 258;   //0x102
    int u2g = 0xFFCEFFCE;   //-50;   //0xFFCE
    int v2g = 0xFF98FF98;   //-104;  //0xFF98‬
    int v2r = 0xcc00cc;     // 204;   //0xcc
    HVX_Vector v_max_255 = Q6_V_vsplat_R(0x7F80);
    /*
    int v2r_y2rgb = 0xcc0095 ;
    int v2g_y2rgb = 0xFF980095;
    int u2b_y2rgb = 0x1020095;
    int u2g_y2rgb = 0xFFCE0095;
    */

    int src_w_rest = src_w & 127;
    int src_w_rest_3 = src_w_rest * 3;
    HVX_VectorPred q1 = Q6_Q_vsetq_R(96); //
    HVX_Vector *psCtrl = (HVX_Vector *)CtrlPerm_YUV2RGB;
    HVX_Vector sCtrlMSB_A = psCtrl[0];
    HVX_Vector sCtrlMSB_B = psCtrl[1];
    HVX_Vector v_128 = Q6_V_vsplat_R(0x80808080);
    HVX_Vector v_16 = Q6_V_vsplat_R(0x10101010);
    HVX_Vector V_1_shift_value = Q6_V_vsplat_R(64); // 64 (1 << (shift_value_yuv2bgr - 1))
    // printf("out_height_stride = %d\n",out_height_stride);
    for (j = 0; j < src_h; j += 2)
    {
        if (j + 2 < src_h)
        {
            L2fetch((unsigned int)(L2FETCH_ADDR_src), L2FETCH_PARA_src);
            L2FETCH_ADDR_src += 2 * src_stride;
            L2fetch((unsigned int)(L2FETCH_ADDR_srcuv), L2FETCH_PARA_srcuv);
            L2FETCH_ADDR_srcuv += src_stride;
        }
        HEXAGON_Vect1024_UN *inpY0 = (HEXAGON_Vect1024_UN *)(src + j * src_stride);
        HEXAGON_Vect1024_UN *inpY1 = (HEXAGON_Vect1024_UN *)(src + (j + 1) * src_stride);
        HEXAGON_Vect1024_UN *inpuv = (HEXAGON_Vect1024_UN *)(srcuv + (j >> 1) * src_stride);
        unsigned char *outp0 = (unsigned char *)(dst + j * dst_stride);
        unsigned char *outp1 = (unsigned char *)(dst + (j + 1) * dst_stride);

        for (i = src_w; i > VLEN; i -= VLEN) // src_w =src_R_w =src_G_w=src_B_w
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low v high u
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_lo_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_lo_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_hi_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_hi_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);

            vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp0 += 96;

            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
            out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
            vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
            outp1 += 96;
        }
        {
            V_srcuv = *inpuv++;
            HVX_VectorPair V_srcvu_tmp = Q6_Wh_vsub_VubVub(V_srcuv, v_128); // low v high u
            HVX_VectorPair v_rv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            HVX_VectorPair v_gv = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            V_srcY0 = *inpY0++;
            HVX_VectorPair v_bu = Q6_W_vcombine_VV(V_1_shift_value, V_1_shift_value);
            v_rv = Q6_Ww_vmpyacc_WwVhRh(v_rv, Q6_V_lo_W(V_srcvu_tmp), v2r); // rv low 024 high 135
            v_gv = Q6_Ww_vmpyacc_WwVhRh(v_gv, Q6_V_lo_W(V_srcvu_tmp), v2g); // gv low 024 high 135
            v_bu = Q6_Ww_vmpyacc_WwVhRh(v_bu, Q6_V_hi_W(V_srcvu_tmp), u2b); // bu low 024 high 135
            V_srcvu_tmp = Q6_Ww_vmpy_VhRh(Q6_V_hi_W(V_srcvu_tmp), u2g);     // gu low 024 high 135
            HVX_VectorPair V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16);  // low 024 high 135
            HVX_Vector V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            HVX_Vector V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            HVX_VectorPair y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            HVX_VectorPair y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            HVX_Vector r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            HVX_Vector r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            HVX_Vector r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            HVX_Vector r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            HVX_Vector g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            HVX_Vector g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            HVX_Vector g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            HVX_Vector g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            HVX_VectorPair rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            HVX_Vector b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            HVX_Vector b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            HVX_Vector b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            HVX_Vector b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            HVX_VectorPair V_b = Q6_Wuh_vunpack_Vub(b04);
            HVX_VectorPair outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            HVX_Vector out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
            if (0 == src_w_rest)
            {

                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                ;
                out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp0 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                HVX_Vector v128_out = vmemu(outp0);
                vmemu(outp0) = Q6_V_vmux_QVV(q1, out0, v128_out);
                outp0 += 96;
            }
            else
            {
                if (src_w_rest < 32)
                {
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_3); //
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 64)
                {
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 96;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 96)
                {
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;

                    // ruzhongl solution 2,store odd data first, then write back
                    HVX_Vector overflow = vmemu(outp0); // keep 96 + 32
                    // ruzhong2 end of solution 1

                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);

                    outp0 += 96;
                    overflow = Q6_V_vror_VR(overflow, 96); // move high 32 bytes to low 32 bytes
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(32);  // set mutxt
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, overflow, v128_out);

                    // outp0 +=96;
                    // int src_w_rest_q0 = src_w_rest_3-192;
                    // HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    // outtmp0 = Q6_W_vshuff_VVR( Q6_V_hi_W(V_b),Q6_V_hi_W(rg04), -2);;
                    // out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    // out0= Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    // HVX_Vector v128_out = vmemu(outp0);
                    // vmemu(outp0)  = Q6_V_vmux_QVV(q0,out0, v128_out);
                }

                else if (src_w_rest < 128)
                {

                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    ;
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp0) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp0 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 288;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp0);
                    vmemu(outp0) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
            }
            // V_srcY1 = *inpY1++;
            // HVX_VectorPair V_srcY1_tmp = Q6_Wh_vsub_VubVub(V_srcY1,v_16);  // low 024 high 135
            ///////////////////////////////////////////////for 1 ////

            V_srcY0 = *inpY1++;
            V_srcY0_tmp = Q6_Wh_vsub_VubVub(V_srcY0, v_16); // low 024 high 135
            V_srcY0_tmp_L = Q6_Vh_vmax_VhVh(Q6_V_lo_W(V_srcY0_tmp), Q6_V_vzero());
            V_srcY0_tmp_H = Q6_Vh_vmax_VhVh(Q6_V_hi_W(V_srcY0_tmp), Q6_V_vzero());

            y00_mult_02 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_L, y2rgb); // low 04 high 26
            y00_mult_13 = Q6_Ww_vmpy_VhRh(V_srcY0_tmp_H, y2rgb); // low 15 high 37
            r04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_rv));
            r04 = Q6_Vw_vmax_VwVw(r04, Q6_V_vzero());
            r04 = Q6_Vw_vmin_VwVw(r04, v_max_255);
            r26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_rv));
            r26 = Q6_Vw_vmax_VwVw(r26, Q6_V_vzero());
            r26 = Q6_Vw_vmin_VwVw(r26, v_max_255);
            r04 = Q6_Vh_vasr_VwVwR(r26, r04, shift_value_yuv2bgr);
            r15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_rv));
            r15 = Q6_Vw_vmax_VwVw(r15, Q6_V_vzero());
            r15 = Q6_Vw_vmin_VwVw(r15, v_max_255);
            r37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_rv));
            r37 = Q6_Vw_vmax_VwVw(r37, Q6_V_vzero());
            r37 = Q6_Vw_vmin_VwVw(r37, v_max_255);
            r15 = Q6_Vh_vasr_VwVwR(r37, r15, shift_value_yuv2bgr);
            r04 = Q6_Vb_vshuffe_VbVb(r15, r04);

            g04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_gv));
            g04 = Q6_Vw_vadd_VwVw(g04, Q6_V_lo_W(V_srcvu_tmp)); // add gu

            g04 = Q6_Vw_vmax_VwVw(g04, Q6_V_vzero());
            g04 = Q6_Vw_vmin_VwVw(g04, v_max_255);

            g26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_gv));
            g26 = Q6_Vw_vadd_VwVw(g26, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g26 = Q6_Vw_vmax_VwVw(g26, Q6_V_vzero());
            g26 = Q6_Vw_vmin_VwVw(g26, v_max_255);
            g04 = Q6_Vh_vasr_VwVwR(g26, g04, shift_value_yuv2bgr);
            g15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_gv));
            g15 = Q6_Vw_vadd_VwVw(g15, Q6_V_lo_W(V_srcvu_tmp)); // add gu
            g15 = Q6_Vw_vmax_VwVw(g15, Q6_V_vzero());
            g15 = Q6_Vw_vmin_VwVw(g15, v_max_255);
            g37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_gv));
            g37 = Q6_Vw_vadd_VwVw(g37, Q6_V_hi_W(V_srcvu_tmp)); // add gu
            g37 = Q6_Vw_vmax_VwVw(g37, Q6_V_vzero());
            g37 = Q6_Vw_vmin_VwVw(g37, v_max_255);
            g15 = Q6_Vh_vasr_VwVwR(g37, g15, shift_value_yuv2bgr);
            g04 = Q6_Vb_vshuffe_VbVb(g15, g04);
            rg04 = Q6_W_vshuff_VVR(g04, r04, -1); // rg

            b04 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_02), Q6_V_lo_W(v_bu));
            b04 = Q6_Vw_vmax_VwVw(b04, Q6_V_vzero());
            b04 = Q6_Vw_vmin_VwVw(b04, v_max_255);

            b26 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_02), Q6_V_hi_W(v_bu));
            b26 = Q6_Vw_vmax_VwVw(b26, Q6_V_vzero());
            b26 = Q6_Vw_vmin_VwVw(b26, v_max_255);

            b04 = Q6_Vh_vasr_VwVwR(b26, b04, shift_value_yuv2bgr);

            b15 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(y00_mult_13), Q6_V_lo_W(v_bu));
            b15 = Q6_Vw_vmax_VwVw(b15, Q6_V_vzero());
            b15 = Q6_Vw_vmin_VwVw(b15, v_max_255);

            b37 = Q6_Vw_vadd_VwVw(Q6_V_hi_W(y00_mult_13), Q6_V_hi_W(v_bu));

            b37 = Q6_Vw_vmax_VwVw(b37, Q6_V_vzero());
            b37 = Q6_Vw_vmin_VwVw(b37, v_max_255);

            b15 = Q6_Vh_vasr_VwVwR(b37, b15, shift_value_yuv2bgr);
            b04 = Q6_Vb_vshuffe_VbVb(b15, b04);

            V_b = Q6_Wuh_vunpack_Vub(b04);
            outtmp0 = Q6_W_vshuff_VVR(Q6_V_lo_W(V_b), Q6_V_lo_W(rg04), -2);
            ;
            out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);

            if (0 == src_w_rest)
            {

                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                ;
                out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                outp1 += 96;
                out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                HVX_Vector v128_out = vmemu(outp1);
                vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
            }
            else
            {
                if (src_w_rest < 32)
                {
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_3); //
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 64)
                {
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    int src_w_rest_q0 = src_w_rest_3 - 96;
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
                else if (src_w_rest < 96)
                {
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    // ruzhongl solution 2,store odd data first, then write back
                    HVX_Vector overflow = vmemu(outp1); // keep 96 + 32
                    // ruzhong2 end of solution 1

                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);

                    outp1 += 96;
                    overflow = Q6_V_vror_VR(overflow, 96); // move high 32 bytes to low 32 bytes
                    HVX_VectorPred q0 = Q6_Q_vsetq_R(32);  // set mutxt
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, overflow, v128_out);

                    // outp1 +=96;
                    // int src_w_rest_q0  = src_w_rest_3-192;
                    // HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    // outtmp0 = Q6_W_vshuff_VVR( Q6_V_hi_W(V_b),Q6_V_hi_W(rg04), -2);;
                    // out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    // out0= Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    // HVX_Vector v128_out = vmemu(outp1);
                    // vmemu(outp1)  = Q6_V_vmux_QVV(q0,out0, v128_out);
                }

                else if (src_w_rest < 128)
                {

                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    vmemu(outp1) = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    outp1 += 96;
                    outtmp0 = Q6_W_vshuff_VVR(Q6_V_hi_W(V_b), Q6_V_hi_W(rg04), -2);
                    out0 = Q6_V_vrdelta_VV(Q6_V_lo_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    HVX_Vector v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q1, out0, v128_out);
                    outp1 += 96;

                    int src_w_rest_q0 = src_w_rest_3 - 288;

                    HVX_VectorPred q0 = Q6_Q_vsetq_R(src_w_rest_q0);
                    out0 = Q6_V_vrdelta_VV(Q6_V_hi_W(outtmp0), sCtrlMSB_A);
                    out0 = Q6_V_vdelta_VV(out0, sCtrlMSB_B);
                    v128_out = vmemu(outp1);
                    vmemu(outp1) = Q6_V_vmux_QVV(q0, out0, v128_out);
                }
            }
        }
    }
}

/*===========================================================================
    CALLBACK
===========================================================================*/

static void rgb2nv12_callback(
    void *data)
{
    rgb2yuv_callback_t *dptr = (rgb2yuv_callback_t *)data;
    int tid = worker_pool_atomic_inc_return(&(dptr->threadCount)) - 1;
    int rowsPerJob = dptr->rowsPerJob;
    int numThreads = dptr->hvxInfo->numThreads;
    int length = dptr->rowsPerJob;
    // printf("length  %d\n",length);
    if (tid == numThreads - 1)
    {
        length = dptr->height - (numThreads - 1) * rowsPerJob;
    }

    // Set pointers to appropriate line of image for this stripe
    unsigned char *src = dptr->src + (dptr->srcstride * (rowsPerJob * tid));
    unsigned char *dstY = dptr->dst + (dptr->dststride * (rowsPerJob * tid));
    unsigned char *dstvu = dptr->dst + dptr->height * dptr->dststride + (dptr->dststride * ((rowsPerJob >> 1) * tid));
    // next prefetches will just add 1 row
    for (int loops = 0; loops < LOOPS; loops++)
    {
    // HVX-optimized implementation
    if (tid == numThreads - 1)
    {
        RGB2YUV12_lastthread(src, dptr->srcWidth, length, dptr->srcstride, dstY, dstvu, dptr->height, dptr->dststride);
    }
    else
    {
        RGB2YUV12(src, dptr->srcWidth, length, dptr->srcstride, dstY, dstvu, dptr->height, dptr->dststride);
    }
    }

    worker_pool_synctoken_jobdone(dptr->token);
}

static void rgb2nv21_callback(
    void *data)
{
    rgb2yuv_callback_t *dptr = (rgb2yuv_callback_t *)data;
    int tid = worker_pool_atomic_inc_return(&(dptr->threadCount)) - 1;
    int rowsPerJob = dptr->rowsPerJob;
    int numThreads = dptr->hvxInfo->numThreads;
    int length = dptr->rowsPerJob;
    // printf("length  %d\n",length);
    if (tid == numThreads - 1)
    {
        length = dptr->height - (numThreads - 1) * rowsPerJob;
    }

    // Set pointers to appropriate line of image for this stripe
    unsigned char *src = dptr->src + (dptr->srcstride * (rowsPerJob * tid));
    unsigned char *dstY = dptr->dst + (dptr->dststride * (rowsPerJob * tid));
    unsigned char *dstvu = dptr->dst + dptr->height * dptr->dststride + (dptr->dststride * ((rowsPerJob >> 1) * tid));
    // next prefetches will just add 1 row
    for (int loops = 0; loops < LOOPS; loops++)
    {
    // HVX-optimized implementation
    if (tid == numThreads - 1)
    {
        RGB2YUV21_lastthread(src, dptr->srcWidth, length, dptr->srcstride, dstY, dstvu, dptr->height, dptr->dststride);
    }
    else
    {
        RGB2YUV21(src, dptr->srcWidth, length, dptr->srcstride, dstY, dstvu, dptr->height, dptr->dststride);
    }
    }
    worker_pool_synctoken_jobdone(dptr->token);
}

static void nv12Torgb_callback(
    void *data)
{
    yuv2rgb_callback_t *dptr = (yuv2rgb_callback_t *)data;
    int tid = worker_pool_atomic_inc_return(&(dptr->threadCount)) - 1;
    int rowsPerJob = dptr->rowsPerJob;
    int numThreads = dptr->hvxInfo->numThreads;
    int length = dptr->rowsPerJob;
    // printf("length  %d\n",length);
    if (tid == numThreads - 1)
    {
        length = dptr->height - (numThreads - 1) * rowsPerJob;
    }
    // FARF(ERROR,"number works %d",length);

    // Set pointers to appropriate line of image for this stripe
    unsigned char *srcY = dptr->src + (dptr->srcstride * (rowsPerJob * tid));
    unsigned char *srcvu = dptr->src + dptr->height * dptr->srcstride + (dptr->srcstride * ((rowsPerJob >> 1) * tid));
    unsigned char *dst = dptr->dst + (dptr->dststride * (rowsPerJob * tid));
    for (int loops = 0; loops < LOOPS; loops++)
    {
    // HVX-optimized implementation
    if (tid == numThreads - 1)
    {
        // YUV12ToRGB_lastthread(srcY,srcvu, dptr->srcWidth, length, dptr->srcstride,dst,dptr->height, dptr->dststride);
        YUV12ToRGB_lastthread(srcY, srcvu, dptr->srcWidth, length, dptr->srcstride, dst, dptr->height, dptr->dststride);
    }
    else
    {
        YUV12ToRGB(srcY, srcvu, dptr->srcWidth, length, dptr->srcstride, dst, dptr->height, dptr->dststride);
    }
    }
    worker_pool_synctoken_jobdone(dptr->token);
}

static void nv21Torgb_callback(
    void *data)
{
    yuv2rgb_callback_t *dptr = (yuv2rgb_callback_t *)data;
    int tid = worker_pool_atomic_inc_return(&(dptr->threadCount)) - 1;
    int rowsPerJob = dptr->rowsPerJob;
    int numThreads = dptr->hvxInfo->numThreads;
    // numThreads = 1;
    int length = dptr->rowsPerJob;
    // printf("length  %d\n",length);
    if (tid == numThreads - 1)
    {
        length = dptr->height - (numThreads - 1) * rowsPerJob;
    }

    // Set pointers to appropriate line of image for this stripe
    unsigned char *srcY = dptr->src + (dptr->srcstride * (rowsPerJob * tid));
    unsigned char *srcvu = dptr->src + dptr->height * dptr->srcstride + (dptr->srcstride * ((rowsPerJob >> 1) * tid));
    unsigned char *dst = dptr->dst + (dptr->dststride * (rowsPerJob * tid));
    for (int loops = 0; loops < LOOPS; loops++)
    {
    // HVX-optimized implementation
    if (tid == numThreads - 1)
    {
        // YUV12ToRGB_lastthread(srcY,srcvu, dptr->srcWidth, length, dptr->srcstride,dst,dptr->height, dptr->dststride);
        YUV21ToRGB_lastthread(srcY, srcvu, dptr->srcWidth, length, dptr->srcstride, dst, dptr->height, dptr->dststride);
    }
    else
    {
        YUV21ToRGB(srcY, srcvu, dptr->srcWidth, length, dptr->srcstride, dst, dptr->height, dptr->dststride);
    }
    }
    worker_pool_synctoken_jobdone(dptr->token);
}

/*===========================================================================
    GLOBAL FUNCTION
===========================================================================*/

AEEResult qhci_rgb2nv12(
    remote_handle64 handle,
    const uint8_t *imgSrc, // input buffer of unsigned 8-bit values
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
// only supporting HVX version in this example.
#if (__HEXAGON_ARCH__ < 60)
    return AEE_EUNSUPPORTED;
#endif

    // record start time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    int32 dspUsec = 0;
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    //
    if (!(imgSrc && imgDst && (srcWidth == dstWidth) && (srcHeight == dstHeight) && 0 == (srcHeight % 2) && 0 == (srcWidth % 2)))
    {
        FARF(ERROR, "please check the input parameter");
        return AEE_EBADPARM;
    }

    hvx_config_t hvxInfo;
    hvxInfo.numThreads = 4;
    int numWorkers = hvxInfo.numThreads;
    // numWorkers = 1;
    //  split src image into horizontal stripes, for multi-threading.
    worker_pool_job_t job;
    worker_synctoken_t token;

    rgb2yuv_callback_t dptr;
    dptr.token = &token;
    dptr.threadCount = 0;
    dptr.src = (unsigned char *)imgSrc;
    dptr.srcWidth = srcWidth;
    dptr.height = srcHeight;
    dptr.srcstride = srcStride;
    dptr.dststride = dstStride;
    dptr.dst = imgDst;
    dptr.hvxInfo = &hvxInfo;
    // dptr.rowsPerJob = ((dptr.height+ (3 * numWorkers - 1)) / (3 * numWorkers)) & ~1;
    dptr.rowsPerJob = ((dptr.height + (numWorkers - 1)) / (numWorkers)) & ~1;

    // printf("dptr.rowsPerJob   %d  dptr.height %d \n",dptr.rowsPerJob,dptr.height);
    job.dptr = (void *)&dptr;
    job.fptr = rgb2nv12_callback;

    worker_pool_synctoken_init(&token, numWorkers);
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;
    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        // for multi-threaded impl, use this line.
        (void)worker_pool_submit(*worker_pool_context, job);

        // This line can be used instead of the above to directly invoke the
        // callback function without dispatching to the worker pool.
        // job.fptr(job.dptr);
    }
    worker_pool_synctoken_wait(&token);
    // clean up hvx configuration - release temporary reservation (if any), turn off power, etc.

    // record end time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    dspUsec = (int)(endTime - startTime);
    FARF(ALWAYS, "qhci_rgb2nv12 profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(dspUsec), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif
    return AEE_SUCCESS;
}

AEEResult qhci_rgb2nv21(
    remote_handle64 handle,
    const uint8_t *imgSrc, // input buffer of unsigned 8-bit values
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
// only supporting HVX version in this example.
#if (__HEXAGON_ARCH__ < 60)
    return AEE_EUNSUPPORTED;
#endif

    // record start time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    int32 dspUsec = 0;
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    //
    if (!(imgSrc && imgDst && (srcWidth == dstWidth) && (srcHeight == dstHeight) && 0 == (srcHeight % 2) && 0 == (srcWidth % 2)))
    {
        FARF(ERROR, "please check the input parameter");
        return AEE_EBADPARM;
    }

    hvx_config_t hvxInfo;
    hvxInfo.numThreads = 4;
    int numWorkers = hvxInfo.numThreads;
    // numWorkers = 1;
    //  split src image into horizontal stripes, for multi-threading.
    worker_pool_job_t job;
    worker_synctoken_t token;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    rgb2yuv_callback_t dptr;
    dptr.token = &token;
    dptr.threadCount = 0;
    dptr.src = (unsigned char *)imgSrc;
    dptr.srcWidth = srcWidth;
    dptr.height = srcHeight;
    dptr.srcstride = srcStride;
    dptr.dststride = dstStride;
    dptr.dst = imgDst;
    dptr.hvxInfo = &hvxInfo;
    // dptr.rowsPerJob = ((dptr.height+ (3 * numWorkers - 1)) / (3 * numWorkers)) & ~1;
    dptr.rowsPerJob = ((dptr.height + (numWorkers - 1)) / (numWorkers)) & ~1;

    // printf("dptr.rowsPerJob   %d  dptr.height %d \n",dptr.rowsPerJob,dptr.height);
    job.dptr = (void *)&dptr;
    job.fptr = rgb2nv21_callback;

    worker_pool_synctoken_init(&token, numWorkers);
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;
    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        // for multi-threaded impl, use this line.
        (void)worker_pool_submit(*worker_pool_context, job);

        // This line can be used instead of the above to directly invoke the
        // callback function without dispatching to the worker pool.
        // job.fptr(job.dptr);
    }
    worker_pool_synctoken_wait(&token);
    // clean up hvx configuration - release temporary reservation (if any), turn off power, etc.

    // record end time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    dspUsec = (int)(endTime - startTime);
    FARF(ALWAYS, "qhci_rgb2nv21 profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(dspUsec), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif
    return AEE_SUCCESS;
}
AEEResult qhci_nv12Torgb(
    remote_handle64 handle,
    const uint8_t *imgSrc, //
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
// only supporting HVX version in this example.
#if (__HEXAGON_ARCH__ < 60)
    return AEE_EUNSUPPORTED;
#endif

    // record start time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    int32 dspUsec = 0;
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    //
    if (!(imgSrc && imgDst && (srcWidth == dstWidth) && (srcHeight == dstHeight) && 0 == (srcHeight % 2) && 0 == (srcWidth % 2)))
    {
        FARF(ERROR, "please check the input parameter");
        return AEE_EBADPARM;
    }

    hvx_config_t hvxInfo;
    hvxInfo.numThreads = 4;
    int numWorkers = hvxInfo.numThreads;
    // numWorkers = 1;
    //  split src image into horizontal stripes, for multi-threading.
    worker_pool_job_t job;
    worker_synctoken_t token;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    yuv2rgb_callback_t dptr;
    dptr.token = &token;
    dptr.threadCount = 0;
    dptr.src = (unsigned char *)imgSrc;
    dptr.srcWidth = srcWidth;
    dptr.height = srcHeight;
    dptr.srcstride = srcStride;
    dptr.dststride = dstStride;
    dptr.dst = imgDst;
    dptr.hvxInfo = &hvxInfo;
    // dptr.rowsPerJob = ((dptr.height+ (3 * numWorkers - 1)) / (3 * numWorkers)) & ~1;
    dptr.rowsPerJob = ((dptr.height + (numWorkers - 1)) / (numWorkers)) & ~1;

    // printf("dptr.rowsPerJob   %d  dptr.height %d \n",dptr.rowsPerJob,dptr.height);
    job.dptr = (void *)&dptr;
    job.fptr = nv12Torgb_callback;

    worker_pool_synctoken_init(&token, numWorkers);
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;
    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        // for multi-threaded impl, use this line.
        (void)worker_pool_submit(*worker_pool_context, job);

        // This line can be used instead of the above to directly invoke the
        // callback function without dispatching to the worker pool.
        // job.fptr(job.dptr);
    }
    worker_pool_synctoken_wait(&token);
    // record end time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    dspUsec = (int)(endTime - startTime);
    FARF(ALWAYS, "qhci_nv12Torgb profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(dspUsec), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif

    return AEE_SUCCESS;
}
AEEResult qhci_nv21Torgb(
    remote_handle64 handle,
    const uint8_t *imgSrc, // input buffer of unsigned 8-bit values
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
// only supporting HVX version in this example.
#if (__HEXAGON_ARCH__ < 60)
    return AEE_EUNSUPPORTED;
#endif

    // record start time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    int32 dspUsec = 0;
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    //
    if (!(imgSrc && imgDst && (srcWidth == dstWidth) && (srcHeight == dstHeight) && 0 == (srcHeight % 2) && 0 == (srcWidth % 2)))
    {
        FARF(ERROR, "please check the input parameter");
        return AEE_EBADPARM;
    }
    // Determine if it is safe (from an audio/voice/camera concurrency perspective) to run a compute function now

    hvx_config_t hvxInfo;
    hvxInfo.numThreads = 4;
    int numWorkers = hvxInfo.numThreads;

    worker_pool_job_t job;
    worker_synctoken_t token;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    yuv2rgb_callback_t dptr;
    dptr.token = &token;
    dptr.threadCount = 0;
    dptr.src = (unsigned char *)imgSrc;
    dptr.srcWidth = srcWidth;
    dptr.height = srcHeight;
    dptr.srcstride = srcStride;
    dptr.dststride = dstStride;
    dptr.dst = imgDst;
    dptr.hvxInfo = &hvxInfo;
    // dptr.rowsPerJob = ((dptr.height+ (3 * numWorkers - 1)) / (3 * numWorkers)) & ~1;
    dptr.rowsPerJob = ((dptr.height + (numWorkers - 1)) / (numWorkers)) & ~1;

    // printf("dptr.rowsPerJob   %d  dptr.height %d \n",dptr.rowsPerJob,dptr.height);
    job.dptr = (void *)&dptr;
    job.fptr = nv21Torgb_callback;

    worker_pool_synctoken_init(&token, numWorkers);
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;
    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        // for multi-threaded impl, use this line.
        (void)worker_pool_submit(*worker_pool_context, job);

        // This line can be used instead of the above to directly invoke the
        // callback function without dispatching to the worker pool.
        // job.fptr(job.dptr);
    }
    worker_pool_synctoken_wait(&token);
    // record end time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    dspUsec = (int)(endTime - startTime);
    FARF(ALWAYS, "qhci_nv21Torgb profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(dspUsec), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif

    return AEE_SUCCESS;
}
