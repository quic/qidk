
/*___________________________________________________________________________
 |                                                                           |
 |   Function Name : div_fractional                                          |
 |                                                                           |
 |   Purpose :                                                               |
 |                                                                           |
 |   Produces a result which is the fractional integer division of var1  by  |
 |   var2; var1 and var2 must be positive and var2 must be greater or equal  |
 |   to var1; the result is positive (leading bit equal to 0) and truncated  |
 |   to 16 bits.                                                             |
 |   If var1 = var2 then div(var1,var2) = 32767.                             |
 |                                                                           |
 |   Complexity weight : 18                                                  |
 |                                                                           |
 |   Inputs :                                                                |
 |                                                                           |
 |    var1                                                                   |
 |             16 bit short unsigned integer (Word16) whose value falls in   |
 |             range : 0x0000 0000 <= var1 <= var2 and var2 != 0.            |
 |                                                                           |
 |    var2                                                                   |
 |             16 bit short unsigned integer (Word16) whose value falls in   |
 |             range : var1 <= var2 <= 0x0000 7fff and var2 != 0.            |
 |                                                                           |
 |   Outputs :                                                               |
 |                                                                           |
 |    none                                                                   |
 |                                                                           |
 |   Return Value :                                                          |
 |                                                                           |
 |    var_out                                                                |
 |             16 bit short unsigned integer (Word16) whose value falls in   |
 |             range : 0x0000 0000 <= var_out <= 0x0000 7fff.                |
 |             It is a Q15 value (point between b15 and b14).                |
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
#define sDenom                V1:0
#define sDenome               V0
#define sDenomo                V1
#define sSumF                 V3:2
#define sSumFe                V2
#define sSumFo                V3
#define sResulte              V4
#define sResulto              V5
#define sBit                 V6
   .text
   .p2align 4                          // ensures 16-byte alignment of first packet
    .globl div_fractional
    .type    div_fractional, @function
div_fractional:
{

  len = LSR(len,#(LOG2VLEN-1))
   
}
{
	LOOP1(.div_func_loop,len) 
}

.falign
.div_func_loop:		
    {   
        sSumFe = vmem(SumFPtr++#1)
        const1 = ##0x00000001
        sResulte = #0               //
    }
    {	
        sDenome = vmem(Denomptr++#1)              //
        sResulto = #0  
        sBit = VSPLAT(const1) 
    }
    {
	LOOP0(.division_LOOP,#15) 
        sDenom.uw = vzxt(sDenome.uh)   //V1:0  1 3 5...........0 2 4.........  
        sSumF.uw  = vzxt(sSumFe.uh)         //V3:2  1 3 5...........0 2 4......... 
    }
    {           //
 
      sSumFe.w = VASL(sSumFe.w,const1)          //
      }
 
.falign
.division_LOOP:
    
      {
      sSumFo.w = VASL(sSumFo.w,const1)  
      }
      {
      Q0 = VCMP.GT(sDenome.w,sSumFe.w)  
      sResulte.w = VASL(sResulte.w,const1)    
    }
      { sResulto.w = VASL(sResulto.w,const1)      // 
    }
    { 
     Q1 = VCMP.GT(sDenomo.w,sSumFo.w) 
      
      IF (!Q0) sSumFe.w -= sDenome.w              //
      IF (!Q0) sResulte.w += sBit.w                //
    }
  
    {  
      IF (!Q1) sSumFo.w -= sDenomo.w              //
      IF (!Q1) sResulto.w += sBit.w                //
      sSumFe.w = VASL(sSumFe.w,const1) 
    }:endloop0
    {
        VTMP.h=VSHUFFE(sResulto.h,sResulte.h) 
        VMEM(quotPtr++#1) = VTMP.new
    }:endloop1
	
    { 
        
        JUMPR R31
    }
    .size    div_fractional, .-div_fractional
