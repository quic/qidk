#include "hexagon_types.h"
#include "hexagon_protos.h"         // part of Q6 tools, contains intrinsics definitions

#ifndef _DEBUG
#define _DEBUG
#endif
#include "HAP_farf.h"


/* ======================================================================== */
/*  Intrinsic C version of Gaussian5x5u16().                                 */
/* ======================================================================== */
void Gaussian5x5u16PerRow_intrinsic(
    uint16_t **pSrc,
    int            width,
    uint16_t *dst,
    int VLEN
    )
{
#if (__HEXAGON_ARCH__ >= 60)
    int i;
    HEXAGON_Vect32 c1c1c1c1,c2c2c2c2,c2c2;

    HVX_Vector sLine0, sLine1, sLine2, sLine3, sLine4;
    HVX_Vector sXV0e, sXV0o, sXV1e, sXV1o;
    HVX_Vector sX_1, sX_2,  sX0, sX1, sX2, sX3; 
    HVX_Vector sX1X_1, sX2X_2,  sX2X0, sX3X_1;

    HVX_VectorPair dVsum;
	HVX_Vector dSum02, dSum13;

    HVX_Vector *iptr0 = (HVX_Vector *)(pSrc[0]);
    HVX_Vector *iptr1 = (HVX_Vector *)(pSrc[1]);
    HVX_Vector *iptr2 = (HVX_Vector *)(pSrc[2]);
    HVX_Vector *iptr3 = (HVX_Vector *)(pSrc[3]);
    HVX_Vector *iptr4 = (HVX_Vector *)(pSrc[4]);

    HVX_Vector *optr  = (HVX_Vector *)dst;

    //c0c1c0c1 = 0x01060106;
   // c2c3c2c3 = 0x0F140F14;
    c1c1c1c1 = 0x06060606;
    c2c2c2c2 = 0x0F0F0F0F;
    c2c2 = 0x000F000F;
    sXV0e = Q6_V_vzero();
    sXV0o = Q6_V_vzero();

    sLine0 = *iptr0++;
    sLine1 = *iptr1++;
    sLine2 = *iptr2++;
    sLine3 = *iptr3++;
    sLine4 = *iptr4++;

    dVsum = Q6_Ww_vadd_VuhVuh(sLine0,sLine4);
    dVsum = Q6_Ww_vmpaacc_WwWuhRb(dVsum,Q6_W_vcombine_VV(sLine1,sLine3),c1c1c1c1);
    dVsum = Q6_Wuw_vmpyacc_WuwVuhRuh(dVsum,sLine2,c2c2);
	
    sXV1e = Q6_V_lo_W(dVsum);
    sXV1o = Q6_V_hi_W(dVsum);

    for ( i=width; i>VLEN/2; i-=VLEN/2 )
    {
        sLine0 = *iptr0++;
        sLine1 = *iptr1++;
        sLine2 = *iptr2++;
        sLine3 = *iptr3++;
        sLine4 = *iptr4++;
  
        dVsum = Q6_Ww_vadd_VuhVuh(sLine0,sLine4);
        dVsum = Q6_Ww_vmpaacc_WwWuhRb(dVsum,Q6_W_vcombine_VV(sLine1,sLine3),c1c1c1c1);
        dVsum = Q6_Wuw_vmpyacc_WuwVuhRuh(dVsum,sLine2,c2c2);
//////////
        sX_1 = Q6_V_vlalign_VVI(sXV1o,sXV0o,4);
        sX_2 = Q6_V_vlalign_VVI(sXV1e,sXV0e,4);
       

        sXV0e = sXV1e;
        sXV0o = sXV1o;
        sXV1e = Q6_V_lo_W(dVsum);
        sXV1o = Q6_V_hi_W(dVsum);

        sX0 = sXV0e;
        sX1 = sXV0o;
        sX2 = Q6_V_valign_VVI(sXV1e,sXV0e,4);
        sX3 = Q6_V_valign_VVI(sXV1o,sXV0o,4);
       
        sX1X_1 = Q6_Vw_vadd_VwVw(sX1,sX_1);
        sX2X_2 = Q6_Vw_vadd_VwVw(sX2,sX_2);
        

        sX2X0  = Q6_Vw_vadd_VwVw(sX2,sX0);
        sX3X_1 = Q6_Vw_vadd_VwVw(sX3,sX_1);
        

       // dSum02 = Q6_Ww_vmpa_WhRb(Q6_W_vcombine_VV(sX1X_1,sX0),c2c3c2c3);
		//dSum02 = Q6_Ww_vmpaacc_WwWhRb(dSum02,Q6_W_vcombine_VV(sX3X_3,sX2X_2),c0c1c0c1);
		dSum02 = Q6_Vw_vmpyiacc_VwVwRub(sX2X_2,sX0, c2c2c2c2);
	    dSum02 = Q6_Vw_vmpyiacc_VwVwRub(dSum02,sX1X_1,c1c1c1c1);
		//dSum13 = Q6_Ww_vmpa_WhRb(Q6_W_vcombine_VV(sX2X0,sX1),c2c3c2c3);
        //dSum13 = Q6_Ww_vmpaacc_WwWhRb(dSum13,Q6_W_vcombine_VV(sX4X_2,sX3X_1),c0c1c0c1);
		dSum13 = Q6_Vw_vmpyiacc_VwVwRub(sX3X_1,sX1, c2c2c2c2);
	    dSum13 = Q6_Vw_vmpyiacc_VwVwRub(dSum13,sX2X0,c1c1c1c1);
		
        *optr++ = Q6_Vuh_vasr_VwVwR_sat(dSum13,dSum02,12);
        
      
		//////
    }


    {   
        sX_1 = Q6_V_vlalign_VVI(sXV1o,sXV0o,4);
        sX_2 = Q6_V_vlalign_VVI(sXV1e,sXV0e,4);
     

        sXV0e = sXV1e;
        sXV0o = sXV1o;
        sXV1e = Q6_V_lo_W(dVsum);
        sXV1o = Q6_V_hi_W(dVsum);

        sX0 = sXV0e;
        sX1 = sXV0o;
        sX2 = Q6_V_valign_VVI(sXV1e,sXV0e,4);
        sX3 = Q6_V_valign_VVI(sXV1o,sXV0o,4);

        sX1X_1 = Q6_Vw_vadd_VwVw(sX1,sX_1);
        sX2X_2 = Q6_Vw_vadd_VwVw(sX2,sX_2);

        sX2X0  = Q6_Vw_vadd_VwVw(sX2,sX0);
        sX3X_1 = Q6_Vw_vadd_VwVw(sX3,sX_1);


       // dSum02 = Q6_Ww_vmpa_WhRb(Q6_W_vcombine_VV(sX1X_1,sX0),c2c3c2c3);
		//dSum02 = Q6_Ww_vmpaacc_WwWhRb(dSum02,Q6_W_vcombine_VV(sX3X_3,sX2X_2),c0c1c0c1);
		dSum02 = Q6_Vw_vmpyiacc_VwVwRub(sX2X_2,sX0, c2c2c2c2);
	    dSum02 = Q6_Vw_vmpyiacc_VwVwRub(dSum02,sX1X_1,c1c1c1c1);
		//dSum13 = Q6_Ww_vmpa_WhRb(Q6_W_vcombine_VV(sX2X0,sX1),c2c3c2c3);
        //dSum13 = Q6_Ww_vmpaacc_WwWhRb(dSum13,Q6_W_vcombine_VV(sX4X_2,sX3X_1),c0c1c0c1);
		dSum13 = Q6_Vw_vmpyiacc_VwVwRub(sX3X_1,sX1, c2c2c2c2);
		dSum13 = Q6_Vw_vmpyiacc_VwVwRub(dSum13,sX2X0,c1c1c1c1);

        *optr++ = Q6_Vuh_vasr_VwVwR_sat(dSum13,dSum02,12);
    }
#endif
}
