/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : div_integer_signed                                      |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Produces a result which is the integer division of var1 by var2; var1   |
 |   and var2 must be signed integer; The result is the signed integer       |
 |   partial result obtained by dividing var1 by var2                        |
 |                                  |
 |                                                                           |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    var1                                                                   |
 |             16 bit short signed integer (Word16)                          |
 |                                                                           |
 |    var2                                                                   |
 |             16 bit short signed integer (Word16)                          |
 |                                                                           |
 |   Outputs :                                                               |
 |                                                                           |
 |    none                                                                   |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    var_out                                                                |
 |             16 bit short signed integer (Word16)                          |
 |___________________________________________________________________________|
*/
#define LOG2VLEN             7     //if mode == 128byte LOG2VLEN = 7   if mode == 64byte LOG2VLEN = 6
#define SumFPtr                 R0  
#define Denomptr             R1
#define quotPtr                 R2
#define len                   R3
#define const1               R4
#define const4000             R5
#define const40004000       R6
#define const8000            R7
#define const0            R8
#define sDenom                V1:0
#define sDenome               V0
#define sDenomo                V1
#define sSumF                 V3:2
#define sSumFe                V2
#define sSumFo                V3
#define sResulte              V4
#define sResulto              V5
#define sBit                 V6
#define singed                 V7
#define Vconst0          V8
#define Vconst1                  V9
   .text
   .p2align 4                          // ensures 16-byte alignment of first packet
    .globl div_integer_signed
    .type    div_integer_signed, @function
div_integer_signed:
    {
	  len = LSR(len,#(LOG2VLEN-1))   
	  const1 = ##0x40000001
    }
    {
	LOOP1(.div_integer_signed_len_loop,len) 
	R5:4 = PACKHL(const1,const1) 
	Vconst0 = ##0x0
    }
    {
	const8000 = ##0x8000
	Vconst1 = VSPLAT(const1)
    }
.falign
.div_integer_signed_len_loop:		
    {   
        sSumFe = vmem(SumFPtr++#1)
        sBit = VSPLAT(const8000) 
        sResulte = #0               //    
    }
    {	
        sDenome = vmem(Denomptr++#1)              //
        sResulto = #0  
        LOOP0(.division_LOOP,#15)  
        sSumFo.h= vabs(sSumFe.h):sat
        
    }
    {
	singed=vxor(sSumFe,sDenome)
	 sDenome.h= vabs(sDenome.h):sat
        sSumF.uw  = vzxt(sSumFo.uh)         //V3:2 ：    1 3 5...........0 2 4.........   
    }
    {
    	Q2=vcmp.gt(singed.h,Vconst0.h)
	
	sDenom.w = VMPY(sDenome.h,const4000.h)   //V1:0 ：    1 3 5...........0 2 4.........  
    }
    {
	Q2 |=vcmp.eq(singed.h,Vconst0.h)
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
	sResulte.h = vsub(Vconst0.h,sResulto.h)
    }
    {
	VTMP=vmux(Q2,sResulto,sResulte)
	VMEM(quotPtr++#1) = VTMP.new
    }:endloop1
	
    { 
       
        JUMPR R31
    }
    .size    div_integer_signed, .-div_integer_signed

