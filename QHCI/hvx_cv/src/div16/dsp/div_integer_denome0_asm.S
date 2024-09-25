//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : div_integer_denome0                                     |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Produces a result which is the integer division of var1 by var2; var1   |
 |   and var2 must be unsigned integer; The result is the integer partial    |
 |   result obtained by dividing var1 by var2                                |
 |   If var2 == 0 then div(var1,var2) = 0.                                   |
 |                                                                           |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    var1                                                                   |
 |             16 bit short unsigned integer (Word16)                        |
 |                                                                           |
 |    var2                                                                   |
 |             16 bit short unsigned integer (Word16)                        |
 |                                                                           |
 |   Outputs :                                                               |
 |                                                                           |
 |    none                                                                   |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    var_out                                                                |
 |             16 bit short unsigned integer (Word16)                        |
 |___________________________________________________________________________|
*/

#define LOG2VLEN             7
#define SumFPtr                 R0  
#define Denomptr             R1
#define quotPtr                 R2
#define len                   R3
#define const1               R4
#define const4000             R5
#define const40004000       R6
#define const8000            R7
#define sDenom                V1:0
#define sDenome               V0
#define sDenomo                V1
#define sSumF                 V3:2
#define sSumFe                V2
#define sSumFo                V3
#define sResulte              V4
#define sResulto              V5
#define sBit                 V6
#define  zero_const      V7
   .text
   .p2align 4                          // ensures 16-byte alignment of first packet
    .globl div_integer_denome0
    .type    div_integer_denome0, @function
div_integer_denome0:
{

  len = LSR(len,#(LOG2VLEN-1))
  zero_const = #0
  const1 = ##0x40000101
  
}
{
	LOOP1(.div_integer_denome0_len_loop,len) 
	R5:4 = PACKHL(const1,const1) 
	const8000 = ##0x8000 
}

.falign
.div_integer_denome0_len_loop:		
    {   
        sSumFe = vmem(SumFPtr++#1)
        sBit = VSPLAT(const8000) 
        sResulte = #0               //
        
    }
    {	
        sDenome = vmem(Denomptr++#1)              //
        sResulto = #0  
        LOOP0(.division_LOOP,#15)  
    }
	{
		Q2 = VCMP.EQ(sDenome.uh,zero_const.uh) 
    }
    {
        sDenom.w = VMPY(sDenome.h,const4000.h)   //V1:0 ：    1 3 5...........0 2 4.........  
        sSumF.uw  = vzxt(sSumFe.uh)         //V3:2 ：    1 3 5...........0 2 4.........
	}
.falign
.division_LOOP:
    { Q0 = VCMP.GT(sDenome.w,sSumFe.w)            //
      sBit.w = VASR(sBit.w,const1)                  // 
    }
    { 
     Q1 = VCMP.GT(sDenomo.w,sSumFo.w) 
      sDenome.w = VASR(sDenome.w,const1)          //
      IF (!Q0) sSumFe.w -= sDenome.w              //
      IF (!Q0) sResulte.w += sBit.w                //
    }
    { 
      sDenomo.w = VASR(sDenomo.w,const1)          //
      IF (!Q1) sSumFo.w -= sDenomo.w              //
      IF (!Q1) sResulto.w += sBit.w                //
    }:endloop0
	{
		sResulto.h=VSHUFFE(sResulto.h,sResulte.h)
	}
    {
         
		 VTMP=vmux(Q2,zero_const,sResulto)
         VMEM(quotPtr++#1) = VTMP.new
    }:endloop1
    { 
       
        JUMPR R31
    }
    .size    div_integer_denome0, .-div_integer_denome0

