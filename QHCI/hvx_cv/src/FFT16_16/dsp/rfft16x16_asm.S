 
.file "rfft16x16.s"

#include "hexagon_hvx.h"

#if __hexagon__

.text

/**************************************************************/
/* Function:    RFFT16x16_4_asm                                     */
/**************************************************************/
/* Reg Inputs:  R0 = complex_t *pOutput                       */
/*              R1 = uint16_t nOutputStride     not use       */
/*              R2 = real_t *pInput                           */
/*              R3 = uint16_t nInputStride                    */
/* Reg Outputs: None                                          */
/* SideEffects: R0-R12,M0,M1,V0-V31                            */
/* Stack Usage: 0                                             */
/* Notes:       Callable from C                               */
/* Cycles:                                                  */
/**************************************************************/

#define COS_PI_8 30274
#define SIN_PI_8 12540
#define COS_PI_4 23170

.p2align 4
.globl RFFT16x16_4_asm
.type RFFT16x16_4_asm, @function

RFFT16x16_4_asm:
	 {   
		allocframe(#256)                              // tmp On stack	
		R3 = ADD(R3,R3)            // nInputStride*sizeof(real_t)	
    }
	{ M1 = R3                    // nInputStride*sizeof(real_t)
	}
	{ 
		V0 = VMEMU(R2++M1)   // RG
	}
	{
      V8 = VMEMU(R2++M1)   // GB 
	  R13:12 = COMBINE(#64,#32)
     // R1 = ASL(R1,#(LOG2_SIZEOF_HALFWORD+1))
                                 // nOutputStride*sizeof(complex_t)
	}
	{
		V0.h=VDEAL(V0.h)
	}	
	
	{
		V8.h=VDEAL(V8.h) 
	}
	{
		V1 = VMEMU(R2++M1)   // RG
		
	}
	{	 
	  R5:4 = COMBINE(#SIZEOF_WORD,#SIZEOF_HALFWORD)
      V9 = VMEMU(R2++M1)   // RG                          // 4|2 
      R8 = #COS_PI_4       // COS_PI_4
   }
   {
	  V1.h=VDEAL(V1.h)
   }
   {
       V9.h=VDEAL(V9.h)
    }
   {
     
      R7:6 = COMBINE(#-SIZEOF_HALFWORD,#SIZEOF_DOUBLEWORD)
						  // -2|8
	  VSHUFF(V8,V0,R12)  //V0 0 R G(2) G(1) B
	}
	{
      R8 = COMBINE(R8.l,R8.l)    // COS_PI_4|COS_PI_4
      R9 = #SIN_PI_8      // SIN_PI_8
	  VSHUFF(V9,V1,R12)    //V1 1 R G(2) G(1) B
   }
   {
      //M0 = R1                    // nOutputStride*sizeof(complex_t)
      R9 = COMBINE(R9.l,R9.l)    // SIN_PI_8|SIN_PI_8
	  V2 = VMEMU(R2++M1) 
	}
	{	
      V12 = VMEMU(R2++M1) 
      //R10 = #SIZEOF_QUADRUPLEWORD  //16
	  R10 = #16
   } 
	{
	    V2.h=VDEAL(V2.h)
	}
	 {
       V12.h=VDEAL(V12.h)
    }
	{
       V3 = VMEMU(R2++M1)      // x37|x36|x35|x34|x33|x32|x31|x30
   }
    {
       V13= VMEMU(R2++M1)      // x37|x36|x35|x34|x33|x32|x31|x30
    }
   
	{
	    V3.h=VDEAL(V3.h)
	}
	{
       V13.h=VDEAL(V13.h)
    }
   
   {
      VSHUFF(V12,V2,R12)          // x27|x26|x25|x24|x23|x22|x21|x20 
   }
   { 
	   VSHUFF(V13,V3,R12)    //V3  R G(2) G(1) B
   }
    {
      V4 = VMEMU(R2++M1)          // x47|x46|x45|x44|x43|x42|x41|x40
	  V1:0.h = VSHUFFOE(V1.h,V0.h)    // x17|x07|x15|x05|x13|x03|x11|x01|x16|x06|x14|x04|x12|x02|x10|x00 
	  R11 = #COS_PI_8             //COS_PI_8
    }
    {
      V12 = VMEMU(R2++M1)          // x47|x46|x45|x44|x43|x42|x41|x40
      R11 = COMBINE(R11.l,R11.l)    //COS_PI_8|COS_PI_8
    }
	{
	    V4.h=VDEAL(V4.h)
	}
	{
      V5 = VMEMU(R2++M1)      // x57|x56|x55|x54|x53|x52|x51|x50
	}
    {
       V12.h=VDEAL(V12.h)
    }
	{
      V13 = VMEMU(R2++M1)      // x57|x56|x55|x54|x53|x52|x51|x50
	}
	{
	    V5.h=VDEAL(V5.h)
	}
	
    {
       V13.h=VDEAL(V13.h)
    }
	{
      V6 = VMEMU(R2++M1)          // x67|x66|x65|x64|x63|x62|x61|x60
	  V3:2.h = VSHUFFOE(V3.h,V2.h)          // x37|x27|x35|x25|x33|x23|x31|x21|x36|x26|x34|x24|x32|x22|x30|x20
    }
	{
      V8 = VMEMU(R2++M1)      // x57|x56|x55|x54|x53|x52|x51|x50
	}
	{
	    V6.h=VDEAL(V6.h)
	}
	{
		V7 = VMEMU(R2++M1)      // x77|x76|x75|x74|x73|x72|x71|x70
	}
    {
       V8.h=VDEAL(V8.h)
    }
	{
		V9 = VMEMU(R2++M1)      // x77|x76|x75|x74|x73|x72|x71|x70
	}
	{
       V7.h=VDEAL(V7.h)
    }
	{
       V9.h=VDEAL(V9.h)
    }
	{ 
	   VSHUFF(V13,V5,R12)    //V5  R G(2) G(1) B
	}
   	{ 
	   VSHUFF(V12,V4,R12)    //V4  R G(2) G(1) B
    }
	{ 
	   VSHUFF(V8,V6,R12)    //V6  R G(2) G(1) B
    }
	{ 
	   VSHUFF(V9,V7,R12)    //V6  R G(2) G(1) B
    }
	{
      VSHUFF(V2,V0,R5)           // x36|x26|x16|x06|x32|x22|x12|x02|x34|x24|x14|x04|x30|x20|x10|x00
	   V5:4.h = VSHUFFOE(V5.h,V4.h)    // x57|x47|x55|x45|x53|x43|x51|x41|x56|x46|x54|x44|x52|x42|x50|x40
    }
    {
     
      V24 = VMEMU(R2++M1)          // x87|x86|x85|x84|x83|x82|x81|x80
     
   }
	{
      VSHUFF(V3,V1,R5)           // x37|x27|x17|x07|x33|x23|x13|x03|x35|x25|x15|x05|x31|x21|x11|x01
      V10 = #0                    
   }
     {
     
      V8 = VMEMU(R2++M1)          // x87|x86|x85|x84|x83|x82|x81|x80
     
   }
   {
       V24.h=VDEAL(V24.h)
    }
	{
       V8.h=VDEAL(V8.h)
    }
	{
		V25 = VMEMU(R2++M1)      // x97|x96|x95|x94|x93|x92|x91|x90
		V7:6.h = VSHUFFOE(V7.h,V6.h)  // x77|x67|x75|x65|x73|x63|x71|x61|x76|x66|x74|x64|x72|x62|x70|x60
	}
    {
		V9 = VMEMU(R2++M1)      // x97|x96|x95|x94|x93|x92|x91|x90
	}
	  {
       V25.h=VDEAL(V25.h)
    }
	{
       V9.h=VDEAL(V9.h)
    }
	{
      V26 = VMEMU(R2++M1)          // x10 7|x10 6|x10 5|x10 4|x10 3|x10 2|x10 1|x10 0
   }
   { 
	   VSHUFF(V8,V24,R12)    //V24  R G(2) G(1) B
    }
	{
      V12 = VMEMU(R2++M1)          // x10 7|x10 6|x10 5|x10 4|x10 3|x10 2|x10 1|x10 0
   }
   {
       V26.h=VDEAL(V26.h)
    }
	{
		V27 = VMEMU(R2++M1)      // x11 7|x11 6|x11 5|x11 4|x11 3|x11 2|x11 1|x11 0
	}
	{
       V12.h=VDEAL(V12.h)
    }
	{
      V13 = VMEMU(R2++M1)          // x10 7|x10 6|x10 5|x10 4|x10 3|x10 2|x10 1|x10 0
   }{
       V27.h=VDEAL(V27.h)
    }
	{
       V13.h=VDEAL(V13.h)
    }
	{
		V28 = VMEMU(R2++M1)          // x12 7|x12 6|x12 5|x12 4|x12 3|x12 2|x12 1|x12 0
	}
	{ 
	   VSHUFF(V9,V25,R12)    //V25  R G(2) G(1) B
    }
	{
		V8 = VMEMU(R2++M1)          // x12 7|x12 6|x12 5|x12 4|x12 3|x12 2|x12 1|x12 0
	}
	{
       V28.h=VDEAL(V28.h)
    }
	{
       V8.h=VDEAL(V8.h)
    }
	{
		V29 = VMEMU(R2++M1)      // x13 7|x13 6|x13 5|x13 4|x13 3|x13 2|x13 1|x13 0
	}
	{ 
	   VSHUFF(V13,V27,R12)    //V27  R G(2) G(1) B
    }
	{
		V9 = VMEMU(R2++M1)    
	}
	{
       V29.h=VDEAL(V29.h)
    }
	{
       V9.h=VDEAL(V9.h)
    }
	{
	    V30 = VMEMU(R2++M1)          // x14 7|x14 6|x14 5|x14 4|x14 3|x14 2|x14 1|x14 0
	}
	{ 
	   VSHUFF(V12,V26,R12)    //V26  R G(2) G(1) B
    }
	{
	    V13 = VMEMU(R2++M1)          // x14 7|x14 6|x14 5|x14 4|x14 3|x14 2|x14 1|x14 0
	}
	{
       V30.h=VDEAL(V30.h)
    }
	{
       V13.h=VDEAL(V13.h)
    }
	{ 
	   VSHUFF(V8,V28,R12)    //V28  R G(2) G(1) B
    }
	{ 
	   VSHUFF(V9,V29,R12)    //V29  R G(2) G(1) B
    }
	{
		V31 = VMEMU(R2++M1)      // x15 7|x15 6|x15 5|x15 4|x15 3|x15 2|x15 1|x15 0
	}
	{ 
	   VSHUFF(V13,V30,R12)    //V30  R G(2) G(1) B
    }
	{
		V12 = VMEMU(R2++M1)      // x15 7|x15 6|x15 5|x15 4|x15 3|x15 2|x15 1|x15 0
	}
	{
       V31.h=VDEAL(V31.h)
    }
	{
       V12.h=VDEAL(V12.h)
    }
    {
      VSHUFF(V7,V5,R5)           // x77|x67|x57|x47|x73|x63|x53|x43|x75|x65|x55|x45|x71|x61|x51|x41
      
   }
   { 
	   VSHUFF(V12,V31,R12)    //V31  R G(2) G(1) B
    }
   {
      VSHUFF(V5,V1,R6)           // X5|X1=x75|x65|x55|x45|x35|x25|x15|x05|x71|x61|x51|x41|x31|x21|x11|x01
      V27:26.h = VSHUFFOE(V27.h,V26.h) // x11 7|x10 7|x11 5|x10 5|x11 3|x10 3|x11 1|x10 1|x11 6|x10 6|x11 4|x10 4|x11 2|x10 2|x11 0|x10 0
   }
   {
      VSHUFF(V7,V3,R6)           // X7|X3=x77|x67|x57|x47|x37|x27|x17|x07|x73|x63|x53|x43|x33|x23|x13|x03
      V25:24.h = VSHUFFOE(V25.h,V24.h)  // x97|x87|x95|x85|x93|x83|x91|x81|x96|x86|x94|x84|x92|x82|x90|x80
   }
   { 
      VSHUFF(V26,V24,R5)           // x11 6|x10 6|x96|x86|x11 2|x10 2|x92|x82|x11 4|x10 4|x94|x84|x11 0|x10 0|x90|x80
	  V29:28.h = VSHUFFOE(V29.h,V28.h)  // x13 7|x12 7|x13 5|x12 5|x13 3|x12 3|x13 1|x12 1|x13 6|x12 6|x13 4|x12 4|x13 2|x12 2|x13 0|x12 0
   }
   {  
      VSHUFF(V27,V25,R5)           // x11 7|x10 7|x97|x87|x11 3|x10 3|x93|x8 3|x11 5|x10 5|x95|x85|x11 1|x10 1|x91|x81
	  V31:30.h = VSHUFFOE(V31.h,V30.h)  // x15 7|x14 7|x15 5|x14 5|x15 3|x14 3|x15 1|x14 1|x15 6|x14 6|x15 4|x14 4|x15 2|x14 2|x15 0|x14 0
   }
    {
      VSHUFF(V6,V4,R5)           // x76|x66|x56|x46|x72|x62|x52|x42|x74|x64|x54|x44|x70|x60|x50|x40
   	}
   {
      VSHUFF(V31,V29,R5)           // x15 7|x14 7|x13 7|x12 7|x73|x14 3|x13 3|x12 3|x15 5|x14 5|x13 5|x12 5|x15 1|x14 1|x13 1|x12 1
      V11 = #0 
   }
   {
      VSHUFF(V29,V25,R6)           // X5|X1=x15 5|x14 5|x13 5|x12 5|x11 5|x10 5|x95|x85|x15 1|x14 1|x13 1|x12 1|x11 1|x10 1|x91|x81

   }
   
	 {
		VSHUFF(V30,V28,R5)		   // x15 6|x14 6|x13 6|x12 6|x15 2|x14 2|x13 2|x12 2|x15 4|x14 4|x13 4|x12 4|x15 0|x14 0|x13 0|x12 0
	
	 }
	 {
		VSHUFF(V30,V26,R6)		   // X6|X2=x15 6|x14 6|x13 6|x12 6|x11 6|x10 6|x96|x86|x15 2|x14 2|x13 2|x12 2|x11 2|x10 2|x92|x82

	 }
	 {
	 	VSHUFF(V28,V24,R6)           // X4|X0=x74|x64|x54|x44|x34|x24|x14|x04|x70|x60|x50|x40|x30|x20|x10|x00}
	 }

   {
      VSHUFF(V6,V2,R6)           // X6|X2=x76|x66|x56|x46|x36|x26|x16|x06|x72|x62|x52|x42|x32|x22|x12|x02
   	}
   	{
   		VSHUFF(V4,V0,R6)           // X4|X0=x74|x64|x54|x44|x34|x24|x14|x04|x70|x60|x50|x40|x30|x20|x10|x00
   	}
    {
      VSHUFF(V31,V27,R6)           // X7|X3=x77|x67|x57|x47|x37|x27|x17|x07|x73|x63|x53|x43|x33|x23|x13|x03
   	}
   	{
		VSHUFF(V24,V0,R10)         //X8|X0 = x15 8|....|x58|x48|x38|x28|x18|x08|x15 0|...|x50|x40|x30|x20|x10|x00}
	}

	{
		VSHUFF(V28,V4,R10)         //X12|X4 = x15 12|....|x5 12|x4 12|x3 12|x2 12|x1 12|x0 12|x15 4|...|x54|x44|x34|x24|x14|x04}
		V0.h = VADD(V0.h,V24.h)     // A00=X0+X8
      	V24.h = VSUB(V0.h,V24.h)     // anTmp01r = B01r = A01=X0-X8
	}
	{
		VSHUFF(V25,V1,R10)        //X9|X1 
		V4.h = VADD(V4.h,V28.h)     // A02=X4+X12
      	V28.h = VSUB(V28.h,V4.h)     // anTmp01i =  B01i = A03=X12 - X4
		
	}
	{
		VSHUFF(V29,V5,R10)         //X13|X5 = x15 13|....|x5 13|x4 13|x3 13|x2 13|x1 13|x0 13|x15 5|...|x55|x45|x35|x25|x15|x05}
		V0.h = VADD(V0.h,V4.h)     //B00r = A00+A02
		V4.h = VSUB(V0.h,V4.h)     //D12r = C02r = anTmp02r = B02r = A00-A02
	}
	{
		VSHUFF(V26,V2,R10)         //X10|X2 = x15 10|....|x5 10|x4 10|x3 10|x2 10|x1 10|x0 10|x15 2|...|x52|x42|x32|x22|x12|x02}
		V1.h = VADD(V1.h,V25.h)     // A10=X1+X9
      	V25.h = VSUB(V1.h,V25.h)     // B11r = A11=X1-X9
	}
	{
	
		VSHUFF(V30,V6,R10)         //X14|X6 = x15 14|....|x5 14|x4 14|x3 14|x2 14|x1 14|x0 14|x15 6|...|x56|x46|x36|x26|x16|x06}
		V5.h = VADD(V5.h,V29.h)     // A12=X5+X13
      	V29.h = VSUB(V29.h,V5.h)     // B11i = A13=X13 - X5
	}
	{
	
		VSHUFF(V27,V3,R10)         //X11|X3 = x15 11|....|x5 11|x4 11|x3 11|x2 11|x1 11|x0 11|x15 3|...|x53|x43|x33|x23|x13|x03}
		V2.h = VADD(V2.h,V26.h) 	// A20=X2+X10
		V26.h = VSUB(V2.h,V26.h)	 // B21r = A21=X2-X10
	}
	{
	
		VSHUFF(V31,V7,R10)         //X15|X7 = x15 15|....|x5 15|x4 15|x3 15|x2 15|x1 15|x0 15|x15 7|...|x57|x47|x37|x27|x17|x07}
		
		V6.h = VADD(V6.h,V30.h)     // A22=X6+X14
      	V30.h = VSUB(V30.h, V6.h)     // B21i = A23=X14 - X6
	}

	{
		V17.h = VMPY(V25.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B11r
		V3.h = VADD(V3.h,V27.h)     // A30=X3+X11
      	V27.h = VSUB(V3.h,V27.h)     // B31r = A31=X3-X11
	}
	{
		V18.h = VMPY(V29.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B11i
	 	V7.h = VADD(V7.h,V31.h)     // A32=X7+X15
    	V31.h = VSUB(V31.h,V7.h)     // B31i = A23=X15 - X7
    }
	{
		V25.h = VMPY(V25.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B11r
		V1.h = VADD(V1.h,V5.h)	   //B10r = A10+A12
		V5.h = VSUB(V1.h,V5.h)	   //B12r = A10-A12
	}
	{
		V29.h = VMPY(V29.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B11i
		V2.h = VADD(V2.h,V6.h)	   //B20r = A20+A22
		V6.h = VSUB(V2.h,V6.h)	   //C12i = C02i = anTmp22i= B22r = A20-A22

	}
	{
		V30.h = VMPY(V30.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B21i	
		V25.h = VADD(V17.h,V18.h)	   //anTmp11r = COS_PI_8 * B11r + SIN_PI_8 * B11i
		V29.h = VSUB(V29.h,V25.h)	   //anTmp11i = COS_PI_8 * B11i - SIN_PI_8 * B11r  
	}
   	{
		V26.h = VMPY(V26.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B21r
        V3.h = VADD(V3.h,V7.h)      //B30r = A30+A32
        V7.h = VSUB(V3.h,V7.h)      //B32r = A30-A32
    }
	{
		V17.h = VMPY(V27.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B31r
		V26.h = VADD(V30.h,V26.h)	   //anTmp21r = COS_PI_4 * B21r + COS_PI_4 * B21i
		V30.h = VSUB(V30.h,V26.h)	   //anTmp21i = COS_PI_4 * B21i - COS_PI_4 * B21r 
	}
 	{
		V18.h = VMPY(V31.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B31i
		V0.h = VADD(V0.h,V2.h)	   // C00r= B00r+B20r
		V2.h = VSUB(V0.h,V2.h)	   // out1_4r = C10r= B00r-B20r
	}
	{
		V27.h = VMPY(V27.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B31r
		V1.h  = VADD(V1.h,V3.h)	   // C20r= B10r+B30r
		V3.h = VSUB(V3.h, V1.h)	   // out1_4i = -C30r= -(B10r-B30r)
	}
    {
		V31.h = VMPY(V31.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B31i
	    V0.h = VADD(V0.h,V1.h)	  // out1_0r= C00r+C20r
	    V1.h  = VSUB(V0.h,V1.h)	  // out1_8r= C00r-C20r 
    }
	{
		V5.h = VMPY(V5.h,R8.h):<<1:rnd:sat   //anTmp12i = anTmp12r = COS_PI_4 * B12r
		V28.h = VADD(V28.h,V30.h)	   //C01i = anTmp01i * anTmp21i
		V30.h = VSUB(V28.h,V30.h)	   //C11i = anTmp01i - anTmp21i
	}
	{
		V7.h = VMPY(V7.h,R8.h):<<1:rnd:sat   //anTmp32i = anTmp32r = COS_PI_4 * B32r
		V27.h = VADD(V18.h,V17.h)	   //anTmp31r = COS_PI_8 * B31i + SIN_PI_8 * B31r
		V31.h = VSUB(V31.h,V27.h)	   //anTmp31i = SIN_PI_8 * B31i - COS_PI_8 * B31r
	}
	{	
		V29.h = VADD(V29.h,V31.h)	   //C21i = anTmp11i + anTmp31i
		V31.h = VSUB(V29.h,V31.h)	   //C31i = anTmp11i - anTmp31i
		
	}
	{
		V17.h = VADD(V7.h,V5.h)	   //C22i = anTmp32i + anTmp12i
		V18.h = VSUB(V7.h,V5.h)	   //C32i = anTmp32i - anTmp12i 
		V22.h = VADD(V28.h,V29.h)	   //out1_1i = C01i + C21i
		V23.h = VADD(V25.h,V27.h)	   //C21r = anTmp11r + anTmp31r
	}
	{
		V27.h = VSUB(V25.h,V27.h)	   //C31r = anTmp11r - anTmp31r
		V19.h = VADD(V6.h,V17.h)	      //-out1_2i = C02i + C22i
		V5.h = VADD(V5.h,V7.h)	       //C32r = anTmp12r + anTmp32r
		V7.h = VSUB(V5.h,V7.h)	       //C22r = anTmp12r - anTmp32r	
	}
	{
		VSHUFF(V22,V10,R4)             //y15 1|y15 0|...|...|y31|y30|y11|y10|y14 1|y14 0|...|...|y21|y20|y01|y00                                            					
		V27.h = VSUB(V30.h,V27.h)	   //out1_5i = C11i - C31r
		V30.h = VADD(V30.h,V27.h)	   //- y3i = C11i + C31r
	}
	{	
		V30.h =  VSUB(V11.h,V30.h)     ////  out1_3i V11 = 0
		V29.h = VSUB(V29.h,V28.h)	   //out1_7i = C21i - C01i	
		V6.h = VSUB(V6.h,V5.h)	   //out1_6i = C12i - C32r
		V19.h =  VSUB(V11.h,V19.h)   //out1_2i
	}
	{	
		V24.h = VADD(V24.h,V26.h)	   //C01r = anTmp01r + anTmp21r
		V26.h = VSUB(V24.h,V26.h)	   //C11r = anTmp01r - anTmp21r
	}
	{                                                 
		VSHUFF(V30,V19,R4)				//...|...|y33|y32|y13|y12........|y23|y22|y03|y02 
		V20.h = VADD(V24.h,V23.h)	   //out1_1r = C01r + C21r
		V7.h = VADD(V4.h,V7.h)	       //out1_2r = C02r + C22r			
	}
	{                                                 
		VSHUFF(V27,V3,R4)              //....|y35|y34|y15|y14|...|y25|y24|y05|y04 
		V31.h = VADD(V26.h,V31.h)	   //out1_5r = C11r + C31i
		V26.h = VSUB(V26.h,V31.h)	   //out1_3r = C11r - C31i	
	}
	{                                                 
		VSHUFF(V29,V6,R4)              //...|y37|y36|y17|y16|....|y27|y26|y07|y06 
		V21.h = VSUB(V24.h,V23.h)	   //out1_7r = C01r - C21r 
		V18.h = VADD(V4.h,V18.h)	   //out1_6r = C12r + C32i
	}
	{
		VSHUFF(V19,V10,R5)            //y63|y62|y61|y60|y23|y22|y21|y20|...|y43|y42|y41|y40|y03|y02|y01|y00  
		V23 = #0                   
        V28 = #0                    
	}
	{
		VSHUFF(V30,V22,R5)         //...|y73|y72||y71|y70|y33|y32|y31|y30|...|y53|y52|y51|y50|y13|y12|y11|y10 
		V5:4 = #0
		
	}
	{
		VSHUFF(V6,V3,R5)           //|y67|y66|y65|y64|y27|y26|y25|y24|y47|y46|y45|y44|y07|y06|y05|y04   
		V9:8 = #0
	}
	{
		VSHUFF(V29,V27,R5)        //.....|y77|y76|y75|y74|y37|y36|y35|y34|...|y57|y56|y55|y54|y17|y16|y15|y14 
		V12 = #0
	}
	{
		VSHUFF(V3,V10,R6)        //y47|y46|y45|y44|y43|y42|y41|y40|y07|y06|y05|y04|y03|y02|y01|y00
		V13 =#0
		V14 =#0
	}
	{
		VSHUFF(V27,V22,R6)     //y57|y56|y55|y54|y53|y52|y51|y50|y17|y16|y15|y14|y13|y12|y11|y10   	
		V15 =#0
		V17 =#0	
	}
	{           
		VSHUFF(V4,V10,R10)    // y8i | y0i 
		
		V16 = #0        
	}
	
	{                 
		VSHUFF(V5,V3,R10)    // y12i | y4i  
		V4.h = VADD(V10.h,V4.h)     // D00i =y0i+y8i	
		V10.h = VSUB(V10.h,V4.h)     // D01i =y0i-y8i		
	} 
	{
		VSHUFF(V20,V0,R4)             //r...|...|y31|y30|y11|y10|...|...|y21|y20|y01|y00                                            					
		V25:24.uw = VZXT(V1.uh)		    ////r y15 9|y15 8|...|...|y39|y38|y19|y18|y14 9|y14 8|...|...|y29|y28|y09|y08 
	}	
	{                                                 
		VSHUFF(V26,V7,R4)				//r ...|...|y33|y32|y13|y12........|y23|y22|y03|y02 
		V1 = #0 
	}
	{
		VSHUFF(V8,V22,R10)   // y9i | y1i 
		V3.h = VADD(V3.h,V5.h)     // D02i =y4i+y12i	
		V5.h = VSUB(V3.h,V5.h)     // D03i =y4i-y12i		
	}
	{          
		VSHUFF(V6,V19,R6)       //y67|y66|y65|y64|y63|y62|y61|y60|y27|y26|y25|y24|y23|y22|y21|y20   
	}
	{

		VSHUFF(V13,V24,R5)         //r  y611|y610|y69|y68|y211|y210|y29|y28|...|y411|y410|y49|y48|y011|y010|y09|y08 
		V22.h = VADD(V22.h,V8.h)     // D10i =y1i+y9i	
		V8.h = VSUB(V22.h,V8.h)     // D11i =y1i-y9i			
	}
	{          
		VSHUFF(V12,V6,R10)     // y14i | y6i
		V3.h = VAVG(V4.h,V3.h):rnd      //E00i = D00i + D02i 
		V4.h = VNAVG(V4.h,V3.h)       //E02i = D00i - D02i anTmp2_02i
	}
	{	  
		VSHUFF(V23,V19,R10)    // y10i | y2i 
		V6.h = VADD(V6.h,V12.h)     // D22i =y6i+y14i	
		V12.h = VSUB(V6.h,V12.h)     // D23i =y6i-y14i
	}
	{                                                 
		VSHUFF(V31,V2,R4)              //r ....|y35|y34|y15|y14|...|y25|y24|y05|y04 
		V19.h = VADD(V19.h,V23.h)     // D20i =y2i+y10i	
		V23.h = VSUB(V19.h,V23.h)     // D21i =y2i-y10i
	}
		
	{
		VSHUFF(V9,V27,R10)      // y13i | y5i	
		V6.h = VAVG(V19.h,V6.h):rnd      //E20i = D20i + D22i 
		V19.h = VNAVG(V19.h,V6.h)       //E22i = D20i - D22i	anTmp2_22r 
	} 
	{	    
		VSHUFF(V29,V30,R6)        //y77|y76|y75|y74|y73|y72|y71|y70|y37|y36|y35|y34|y33|y32|y31|y30  
		V27.h = VADD(V27.h,V9.h)     // D12i =y5i+y13i	
		V9.h = VSUB(V27.h,V9.h)     // D13i =y5i-y13i 
	}
	{
		VSHUFF(V15,V24,R6)        //r  y415|y414|y413|y412|y411|y410|y49|y48|y015|y014|y013|y012|y011|y010|y09|y08 
		V27.h = VAVG(V22.h,V27.h):rnd      //E10i = D10i + D12i 
		V22.h = VNAVG(V22.h,V27.h)       //E12i = D10i - D12i  		
	}
	{
			   
		VSHUFF(V11,V30,R10)    // y11i | y3i
		V3.h = VAVG(V3.h,V6.h):rnd      //F00i = E00i + E20i 
		V6.h = VNAVG(V3.h,V6.h)       //F10i = E00i - E20i   		
	}
	{
		VSHUFF(V28,V29,R10)    // y15i | y7i   
		V30.h = VADD(V30.h,V11.h)      // D30i =y3i+y11i	
		V11.h = VSUB(V30.h,V11.h)     // D31i =y3i-y11i
	}
	{                                                 
		VSHUFF(V21,V18,R4)              //r  ...|y37|y36|y17|y16|....|y27|y26|y07|y06 
		V29.h = VADD(V29.h,V28.h)      // D32i =y7i+y15i	
		V28.h = VSUB(V29.h,V28.h)     // D33i =y7i-y15i	
	}
	{
		VSHUFF(V7,V0,R5)            //r  y63|y62|y61|y60|y23|y22|y21|y20|...|y43|y42|y41|y40|y03|y02|y01|y00  
		V29.h = VAVG(V30.h,V29.h):rnd      //E30i = D30i + D32i 
		V30.h = VNAVG(V30.h,V29.h)       //E32i = D30i - D32i               
	}
	{
		VSHUFF(V18,V2,R5)           //r  |y67|y66|y65|y64|y27|y26|y25|y24|y47|y46|y45|y44|y07|y06|y05|y04 
		V27.h = VAVG(V27.h,V29.h):rnd      //F20i = E10i + E30i 
		V29.h = VNAVG(V27.h,V29.h)       //F30i = E10i - E30i 
	}

	{
		VSHUFF(V2,V0,R6)        //r   y47|y46|y45|y44|y43|y42|y41|y40|y07|y06|y05|y04|y03|y02|y01|y00 
		V3.h = VAVG(V3.h,V27.h):rnd      // G00i = F00i + F20i 
		V27.h = VNAVG(V3.h,V27.h)         // G20i = F00i - F20i
	}
	{	
		VSHUFF(V24,V0,R10)    // y8r | y0r 
		
	}

	{
		VSHUFF(V21,V31,R5)        //r  .....|y77|y76|y75|y74|y37|y36|y35|y34|...|y57|y56|y55|y54|y17|y16|y15|y14
		V24.h = VADD(V0.h,V24.h)     // D00r =y0r+y8r	
		V0.h = VSUB(V0.h,V24.h)     // D01r =y0r-y8r        
	}
	{
		VSHUFF(V15,V2,R10)    // y12r | y4r   
		V0.h = VAVG(V0.h,V5.h):rnd      //E01r =D01r + D03i   =anTmp2_01r
		V5.h = VNAVG(V0.h,V5.h)       // E03r = D01r - D03i  = anTmp2_03r
	}
	{
		VSHUFF(V26,V20,R5)         //r  ...|y73|y72||y71|y70|y33|y32|y31|y30|...|y53|y52|y51|y50|y13|y12|y11|y10 
		V15.h = VADD(V2.h,V15.h)     // D02r =y4r+y12r	
		V2.h = VSUB(V2.h,V15.h)     // D03r =y4r-y12r  
	}
	{
		VSHUFF(V18,V7,R6)     //r  y67|y66|y65|y64|y63|y62|y61|y60|y27|y26|y25|y24|y23|y22|y21|y20 
		V15.h = VAVG(V24.h,V15.h):rnd      //E00r = D00r + D02r 
		V24.h = VNAVG(V24.h,V15.h)       //E02r = D00r - D02r    anTmp2_02r		
	}
	{	
		VSHUFF(V17,V13,R6)     //r  y615|y614|y613|y612|y611|y610|y69|y68|y215|y214|y213|y212|y211|y210|y29|y28 		
		V2.h = VAVG(V10.h,V2.h):rnd     // E03i =D01i+D03r	+= anTmp2_03i
		V10.h = VNAVG(V10.h,V2.h)     // E01i=D01i-D03r  =anTmp2_01i
	}
	
	{
		VSHUFF(V31,V20,R6)   //r  y57|y56|y55|y54|y53|y52|y51|y50|y17|y16|y15|y14|y13|y12|y11|y10 
		V22.h = VMPY(V22.h,R8.h):<<1:rnd:sat   //COS_PI_4 * E12i
	}
	{	
		VSHUFF(V13,V7,R10)    // y10r | y2r 
		
	}
	{
		VSHUFF(V17,V18,R10)    // y14r | y6r   
		V7.h = VADD(V7.h,V13.h)     // D20r =y2r+y10r	
		V13.h = VSUB(V7.h,V13.h)     // D21r =y2r-y10r	
	}
	{
		VSHUFF(V14,V25,R5)         //r  y711|y710|y79|y78|y311|y310|y39|y38|...|y511|y510|y59|y58|y111|y110|y19|y18
   		V18.h = VADD(V18.h,V17.h)     // D22r =y6r+y14r	
		V17.h = VSUB(V18.h,V17.h)     // D23r =y6r-y14r
	}

	
	{
		VSHUFF(V21,V26,R6)    //r  y77|y76|y75|y74|y73|y72|y71|y70|y37|y36|y35|y34|y33|y32|y31|y30 
		V18.h = VAVG(V7.h,V18.h):rnd      //E20r = D20r + D22r 
		V7.h = VNAVG(V7.h,V18.h)       //E22r = D20r - D22r	 anTmp2_22i
	}
	{
		VSHUFF(V1,V25,R6)   //r y515|y514|y513|y512|y511|y510|y59|y58|y115|y114|y113|y112|y111|y110|y19|y18   
		V15.h = VAVG(V15.h,V18.h):rnd      //F00r = E00r + E20r 
		V18.h = VNAVG(V15.h,V18.h)       //F10r = E00r - E20r 	
	}
	{
		VSHUFF(V16,V14,R6)     //r y715|y714|y713|y712|y711|y710|y79|y78|y315|y314|y313|y312|y311|y310|y39|y38
		V18.h = VAVG(V18.h,V29.h):rnd      //G10r = F10r + F30i 
		V29.h = VNAVG(V18.h,V29.h)       //G30r = F10r - F30i 	
	}
	{
		VSHUFF(V25,V20,R10)    //r  y9r | y1r 
	        
	}

	{
		VSHUFF(V1,V31,R10)    // y13r | y5r  
		V20.h = VADD(V20.h,V25.h)     // D10r =y1r+y9r	
		V25.h = VSUB(V20.h,V25.h)     // D11r =y1r-y9r
	}
	{
		VSHUFF(V14,V26,R10)    // y11r | y3r 
		V31.h = VADD(V31.h,V1.h)     // D12r =y5r+y13r	
		V1.h = VSUB(V31.h,V1.h)     // D13r =y5r-y13r	
	}
	{
		VSHUFF(V16,V21,R10)    // y15r | y7r   
		V31.h = VAVG(V20.h,V31.h):rnd      //E10r = D10r + D12r 
		V20.h = VNAVG(V20.h,V31.h)       //E12r = D10r - D12r	
	}
	{
		VMEMU(SP) = V22       //STORE COS_PI_4 * E12i
		V26.h = VADD(V26.h,V14.h)      // D30r =y3r+y11r	
		V14.h = VSUB(V26.h,V14.h)     // D31r =y3r-y11r	
	}
	{
		V20.h = VMPY(V20.h,R8.h):<<1:rnd:sat   //COS_PI_4 * E12r
		V21.h = VADD(V21.h,V16.h)      // D32r =y7r+y15r	
		V16.h = VSUB(V21.h,V16.h)     // D33r =y7r-y15r	
	}
	
	{
	    VMEMU(SP+#1) = V20       //STOR COS_PI_4 * E12r
		V1.h = VAVG(V8.h,V1.h):rnd     //E13i = D11i + D13r
		V8.h = VNAVG(V8.h,V1.h)      //E11i = D11i - D13r           
	}
	{
		V17.h = VAVG(V23.h,V17.h):rnd     //E23i = D21i + D23r
		V23.h = VNAVG(V23.h,V17.h)      //E21i = D21i - D23r
		  
	}
	{
		V20.h = VMPY(V8.h,R11.h):<<1:rnd:sat   //COS_PI_8 * E11i 
		V9.h = VAVG(V25.h,V9.h):rnd     //E11r = D11r + D13i	     
		V25.h = VNAVG(V25.h,V9.h)		//E13r = D11r - D13i	                    
									   
	}
	{
		V23.h = VMPY(V23.h,R8.h):<<1:rnd:sat   //COS_PI_4 * E21i   
		V12.h = VAVG(V13.h,V12.h):rnd     //E21r = D21r + D23i	     
		V13.h = VNAVG(V13.h,V12.h)		//E23r = D21r - D23i	                    
									   
	}
	{
	    V22.h = VMPY(V9.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * E11r   
		V21.h = VAVG(V26.h,V21.h):rnd      //E30r = D30r + D32r 
		V26.h = VNAVG(V26.h,V21.h)       //E32r = D30r - D32r  
	
	}
	{
		V12.h = VMPY(V12.h,R8.h):<<1:rnd:sat   //COS_PI_4 * E21r
		V31.h = VAVG(V31.h,V21.h):rnd      //F20r = E10r + E30r 
		V21.h = VNAVG(V31.h,V21.h)       //F30r = E10r - E30r 
	}	
	{
	    V9.h = VMPY(V9.h,R11.h):<<1:rnd:sat   //COS_PI_8 * E11r  
		V28.h = VAVG(V14.h,V28.h):rnd     // E31r = D31r + D33i
		V14.h = VNAVG(V14.h,V28.h)       // E33r = D31r - D33i
	}
	{
		V8.h = VMPY(V8.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * E11i 
		V16.h = VAVG(V11.h,V16.h):rnd     //E33i = D31i + D33r
		V11.h = VNAVG(V11.h,V16.h)      //E31i = D31i - D33r	
	}
	{
		V20.h = VMPY(V28.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * E31r 		
		V8.h = VADD(V8.h,V9.h)      // anTmp2_11r =SIN_PI_8 * E11i + COS_PI_8 * E11r 	
		V9.h = VSUB(V20.h,V22.h)     // anTmp2_11i =COS_PI_8 * E11i - 	SIN_PI_8* E11r
	}
	{ 
		V28.h = VMPY(V28.h,R11.h):<<1:rnd:sat   //COS_PI_8 * E31r 
	
		V31.h = VAVG(V15.h,V31.h):rnd      //G00r = F00r + F20r 
		V15.h = VNAVG(V15.h,V31.h)         //G20r = F00r - F20r 			
	}

	{
		V22.h = VMPY(V11.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * E31i 	
		V21.h = VAVG(V6.h,V21.h):rnd        //G30i = F10i + F30r
		V6.h = VNAVG(V6.h,V21.h)        //G10i = F10i - F30r
		
	}
	{
		V11.h = VMPY(V11.h,R11.h):<<1:rnd:sat   //COS_PI_8 * E31i 	
		V23.h = VADD(V23.h,V12.h)      // anTmp2_21r =COS_PI_4 * E21i + COS_PI_4 * E21r 	
		V12.h = VSUB(V23.h,V12.h)     // anTmp2_21i =COS_PI_4 * E21i - 	COS_PI_4* E21r
	}
	{
		VSHUFF(V6,V18,R7)         // Z4=Z4i/Z4r  G10i G10r
		V11.h = VADD(V20.h,V11.h)      // anTmp2_31r =SIN_PI_8 * E31r + COS_PI_8 * E31i 	
		V28.h = VSUB(V22.h,V28.h)     // anTmp2_31i =SIN_PI_8 * E31i - 	COS_PI_8* E31r
	}
	{
		V22.h = VMPY(V1.h,R11.h):<<1:rnd:sat   //COS_PI_8 * E13i
		V23.h = VAVG(V0.h,V23.h):rnd        //F01r = anTmp2_01r + anTmp2_21r
		V0.h = VNAVG(V0.h,V23.h)        //F11r = anTmp2_01r - anTmp2_21r
	}
	{
		V1.h = VMPY(V1.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * E13i
		V12.h = VAVG(V10.h,V12.h):rnd        //F01i = anTmp2_01i + anTmp2_21i
		V10.h = VNAVG(V10.h,V12.h)        //F11i = anTmp2_01i - anTmp2_21i
	}
	{
		V20.h = VMPY(V25.h,R11.h):<<1:rnd:sat   //COS_PI_8 * E13r
		V11.h = VAVG(V8.h,V11.h):rnd        //F21r = anTmp2_11r + anTmp2_31r
		V8.h = VNAVG(V8.h,V11.h)        //F31r = anTmp2_11r - anTmp2_31r

	}
	{
		V25.h = VMPY(V25.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * E13r 
		V28.h = VAVG(V9.h,V28.h):rnd        //F21i = anTmp2_11i + anTmp2_31i
		V9.h = VNAVG(V9.h,V28.h)        //F31i = anTmp2_11i - anTmp2_31i
	}
	{
		V13.h = VMPY(V13.h,R8.h):<<1:rnd:sat     //COS_PI_4 * E23r 
		V11.h = VAVG(V23.h,V11.h):rnd    //G01r = F01r + F21r
		V23.h = VNAVG(V23.h,V11.h)        //G21r = F01r - F21r
	}
	{
		V17.h = VMPY(V17.h,R8.h):<<1:rnd:sat     //COS_PI_4 * E23i
		V28.h = VAVG(V12.h,V28.h):rnd    //G01i = F01i + F21i
		V12.h = VNAVG(V12.h,V28.h)        //G21i = F01i - F21i
	}
	{
		V20.h = VMPY(V16.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * E33i 
		V25.h = VADD(V22.h,V25.h)     //anTmp2_13r = COS_PI_8 * E13i + SIN_PI_8 * E13r	
		V1.h = VSUB(V1.h,V20.h)   //anTmp2_13i = SIN_PI_8 * E13i  -COS_PI_8 * E13r		
	}
	{
		V22.h = VMPY(V14.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * E33r 
		V9.h = VAVG(V0.h,V9.h):rnd    //G11r = F11r + F31i
		V0.h = VNAVG(V0.h,V9.h)        //G31r = F11r - F31i
	}
	{
		V16.h = VMPY(V16.h,R11.h):<<1:rnd:sat   //COS_PI_8 * E33i 
		V8.h = VAVG(V10.h,V8.h):rnd    //G31i = F11i + F31r
		V10.h = VNAVG(V10.h,V8.h)        //G11i = F11i - F31r
	}
	

	{
		V14.h = VMPY(V14.h,R11.h):<<1:rnd:sat   //COS_PI_8 * E33r 
		V17.h = VADD(V17.h,V13.h)     //anTmp2_23i = COS_PI_4 * E23i  + COS_PI_4 * E23r
		V13.h = VSUB(V17.h,V13.h)   //anTmp2_23r = COS_PI_4 * E23i  - COS_PI_4 * E23r
	}
   
	{
		VSHUFF(V3,V31,R7)         // Z0=Z0i/Z0r  G00i G00r
		V14.h = VADD(V14.h,V20.h)      //anTmp2_33r = COS_PI_8 *E33r + SIN_PI_8 * E33i
		V16.h = VSUB(V22.h,V16.h)    //anTmp2_33i = SIN_PI_8 * E33r - COS_PI_8 *E33i
		VMEM(R0+#1) = V3.new      // store Z0 H
	}
	
	{
		V20 = VMEMU(SP+#1)      //COS_PI_4 * E12r 
		V13.h = VAVG(V5.h,V13.h):rnd        //F03r = anTmp2_03r + anTmp2_23r
		V5.h = VNAVG(V5.h,V13.h)        //F13r = anTmp2_03r - anTmp2_23r
	}
	{
		V22 = VMEMU(SP)      //COS_PI_4 * E12i 
		V17.h = VAVG(V2.h,V17.h):rnd        //F13i = anTmp2_03i + anTmp2_23i
		V2.h = VNAVG(V2.h,V17.h)        //F03i = anTmp2_03i - anTmp2_23i
	}
	{
		V26.h = VMPY(V26.h,R8.h):<<1:rnd:sat   //COS_PI_4 * E32r
		V14.h = VAVG(V25.h,V14.h):rnd        //F33r = anTmp2_13r + anTmp2_33r
		V25.h = VNAVG(V25.h,V14.h)        //F23r = anTmp2_13r - anTmp2_33r
	}
	{
		V30.h = VMPY(V30.h,R8.h):<<1:rnd:sat   //COS_PI_4 * E32i
		V16.h = VAVG(V1.h,V16.h):rnd        //F23i = anTmp2_13i + anTmp2_33i
		V1.h = VNAVG(V1.h,V16.h)        //F33i = anTmp2_13i - anTmp2_33i
	}
	{
		VSHUFF(V21,V29,R7)         // Z12=Z12i/Z12r  G30i G30r
		V20.h = VADD(V22.h,V20.h)       //anTmp2_12r = COS_PI_4*E12i + COS_PI_4 * E12r
		V22.h = VSUB(V22.h,V20.h)        //anTmp2_12i =  COS_PI_4*E12i - COS_PI_4 * E12r
	}
	{
		VSHUFF(V27,V15,R7)                 // Z8=Z8i/Z8r  G20i G20r
		V26.h = VADD(V30.h,V26.h)       //anTmp2_32i = COS_PI_4*E32i + COS_PI_4 * E32r
		V30.h = VSUB(V30.h,V26.h)        //anTmp2_32r =  COS_PI_4*E32i - COS_PI_4 * E32r
	}
	{
		VSHUFF(V28,V11,R7)                 // Z1=Z1i/Z1r  G01i G01r
		V25.h = VAVG(V13.h,V25.h):rnd    //G03r = F03r + F23r
		V13.h = VNAVG(V13.h,V25.h)        //G23r = F03r - F23r
	}
	{
		VSHUFF(V10,V9,R7)                 // Z5=Z5i/Z5r  G11i G11r
		V16.h = VAVG(V2.h,V16.h):rnd    //G03i = F03i + F23i
		V2.h = VNAVG(V2.h,V16.h)        //G23i = F03i - F23i
		
	}
	{
		VSHUFF(V12,V23,R7)                 // Z9=Z9i/Z9r  G21i G21r
		V19.h = VAVG(V24.h,V19.h):rnd        //F02r = anTmp2_02r + anTmp2_22r
		V24.h = VNAVG(V24.h,V19.h)        //F12r = anTmp2_02r - anTmp2_22r
	}
	{
		VSHUFF(V16,V25,R7)                 // Z3=Z3i/Z3r  G03i G03r
		V30.h = VAVG(V20.h,V30.h):rnd        //F22r = anTmp2_12r + anTmp2_32r
		V20.h = VNAVG(V20.h,V30.h)        //F32r = anTmp2_12r - anTmp2_32r
	}
	{
		VSHUFF(V2,V13,R7)                 // Z11=Z11i/Z11r  G23i G23r
		V26.h = VAVG(V22.h,V26.h):rnd        //F32i = anTmp2_12i + anTmp2_32i
		V22.h = VNAVG(V22.h,V26.h)        //F22i = anTmp2_12i - anTmp2_32i	
		
	}
	{
		VSHUFF(V8,V0,R7)                 // Z13=Z13i/Z13r  G31i G31r
		V30.h = VAVG(V19.h,V30.h):rnd        //G02r = F02r + F22r
		V19.h = VNAVG(V19.h,V30.h)        //G22r = F02r - F22r		
	}
	{
		VSHUFF(V11,V31,R5)             ////l...|...|Z31|Z30|Z11|Z10|...|...|Z21|Z20|Z01|Z00 
		V7.h = VAVG(V4.h,V7.h):rnd        //F12i = anTmp2_02i + anTmp2_22i
		V4.h = VNAVG(V4.h,V7.h)        //F02i = anTmp2_02i - anTmp2_22i				
	}
	{
		VSHUFF(V28,V3,R5)             ////H...|...|Z31|Z30|Z11|Z10|...|...|Z21|Z20|Z01|Z00 
		V22.h = VAVG(V4.h,V22.h):rnd        //G02i = F02i + F22i
		V4.h = VNAVG(V4.h,V22.h)        //G22i = F02i - F22i		
	}
		
	{
		
		V26.h = VAVG(V24.h,V26.h):rnd        //G12r = F12r + F32i
		V24.h = VNAVG(V24.h,V26.h)        //G32r = F12r - F32i
	}
	{
		VSHUFF(V22,V30,R7)                 // Z2=Z2i/Z2r  G02i G02r
		V20.h = VAVG(V7.h,V20.h):rnd        //G32i = F12i + F32r
		V7.h = VNAVG(V7.h,V20.h)        //G12i = F12i - F32r
		
	}
	{
		VSHUFF(V4,V19,R7)                 // Z10=Z10i/Z10r  G22i G22r
		V1.h = VAVG(V5.h,V1.h):rnd    //G13r = F13r + F33i
		V5.h = VNAVG(V5.h,V1.h)        //G33r = F13r - F33i
	}
	{
		VSHUFF(V25,V30,R5)             ////L...|...|Z33|Z32|Z13|Z12|...|...|Z23|Z22|Z03|Z02 
		V14.h = VAVG(V17.h,V14.h):rnd    //G33i = F13i + F33r
		V17.h = VNAVG(V17.h,V14.h)        //G13i = F13i - F33r
	}
	
	{
		VSHUFF(V16,V22,R5)             ////H...|...|Z33|Z32|Z13|Z12|...|...|Z23|Z22|Z03|Z02 
	}
	{
		VSHUFF(V7,V26,R7)                 // Z6=Z6i/Z6r  G12i G12r
		
	}
	{
		VSHUFF(V17,V1,R7)                 // Z7=Z7i/Z7r  G13i G13r
		
		
	}
	{
		VSHUFF(V20,V24,R7)                 // Z14=Z14i/Z14r  G32i G32r
		
	}
	{
		VSHUFF(V14,V5,R7)                 // Z15=Z15i/Z15r  G33i G33r
		
	}
	{
		VSHUFF(V9,V18,R5)             ////L...|Z ROW COLUMN  ...|Z35|Z34|Z15|Z14|...|...|Z25|Z24|Z05|Z04 
	}
	{
		VSHUFF(V10,V6,R5)             ////H...|...|Z35|Z34|Z15|Z14|...|...|Z25|Z24|Z05|Z04 
	}
	{
		VSHUFF(V1,V26,R5)             ////L...|...|Z37|Z36|Z17|Z16|...|...|Z27|Z26|Z07|Z06 
	}
	{
		VSHUFF(V17,V7,R5)             ////H...|...|Z37|Z36|Z17|Z16|...|...|Z27|Z26|Z07|Z06 
	}
	{
		VSHUFF(V23,V15,R5)             ////L...|...|Z39|Z38|Z19|Z18|...|...|Z29|Z28|Z09|Z08 
	}
	{
		VSHUFF(V12,V27,R5)             ////H...|...|Z39|Z38|Z19|Z18|...|...|Z29|Z28|Z09|Z08 
	}
	{
		VSHUFF(V13,V19,R5)             ////L...|...|Z311|Z310|Z111|Z110|...|...|Z211|Z210|Z011|Z010 
	}{
		VSHUFF(V2,V4,R5)             ////H...|...|Z311|Z310|Z111|Z110|...|...|Z211|Z210|Z011|Z010
	}
	{
		VSHUFF(V0,V29,R5)             ////L...|...|Z313|Z312|Z113|Z112|...|...|Z213|Z212|Z012|Z012 
	}{
		VSHUFF(V8,V21,R5)             ////H...|...|Z313|Z312|Z113|Z112|...|...|Z213|Z212|Z012|Z012 
	}
	{
		VSHUFF(V5,V24,R5)            ////L...|...|Z33|Z32|Z13|Z12|...|...|Z23|Z22|Z03|Z02 
	}{
		VSHUFF(V14,V20,R5)              ////H...|...|Z33|Z32|Z13|Z12|...|...|Z23|Z22|Z03|Z02 
	}
	
	
	{
		VSHUFF(V30,V31,R6)             ////L...|...|Z63|Z62|Z61|Z60|Z23|Z22|Z21|Z20|...|...|Z43|Z42|Z41|Z40|Z03|Z02|Z01|Z00 
	}
	{
		VSHUFF(V22,V3,R6)             ////H...|...|Z63|Z62|Z61|Z60|Z23|Z22|Z21|Z20|...|...|Z43|Z42|Z41|Z40|Z03|Z02|Z01|Z00 
	}
	{
		VSHUFF(V25,V11,R6)             ////L....|...|Z73|Z72|Z71|Z70|Z33|Z32|Z31|Z30|...|...|Z53|Z52|Z51|Z50|Z13|Z12|Z11|Z10 
	}
	{
		VSHUFF(V16,V28,R6)             ////H...|...|Z73|Z72|Z71|Z70|Z33|Z32|Z31|Z30|...|...|Z53|Z52|Z51|Z50|Z13|Z12|Z11|Z10 
	}
	{
		VSHUFF(V26,V18,R6)             ////L...|...|Z67|Z66|Z65|Z64|Z27|Z26|Z25|Z24|...|...|Z47|Z46|Z45|Z44|Z07|Z06|Z05|Z04 
	}
	{
		VSHUFF(V7,V6,R6)             ////H...|...|Z67|Z66|Z65|Z64|Z27|Z26|Z25|Z24|...|...|Z47|Z46|Z45|Z44|Z07|Z06|Z05|Z04 
	}
	{
		VSHUFF(V1,V9,R6)             ////L...|...|Z77|Z76|Z75|Z74|Z37|Z36|Z35|Z34|...|...|Z57|Z56|Z55|Z54|Z17|Z16|Z15|Z14 
	}
	{
		VSHUFF(V17,V10,R6)             ////H...|...|Z77|Z76|Z75|Z74|Z37|Z36|Z35|Z34|...|...|Z57|Z56|Z55|Z54|Z17|Z16|Z15|Z14  
	}
	{
		VSHUFF(V19,V15,R6)             ////L...|...|Z611|Z610|Z69|Z68|Z211|Z210|Z29|Z28|...|...|Z411|Z410|Z49|Z48|Z011|Z010|Z09|Z08  
	}{
		VSHUFF(V4,V27,R6)             ////H...|....|Z611|Z610|Z69|Z68|Z211|Z210|Z29|Z28|...|...|Z411|Z410|Z49|Z48|Z011|Z010|Z09|Z08 
	}
	{
		VSHUFF(V13,V23,R6)             ////L...|....|Z711|Z710|Z79|Z78|Z311|Z310|Z39|Z38|...|...|Z511|Z510|Z59|Z58|Z111|Z110|Z19|Z18 
	}{
		VSHUFF(V2,V12,R6)             ////H...|...|Z711|Z710|Z79|Z78|Z311|Z310|Z39|Z38|...|...|Z511|Z510|Z59|Z58|Z111|Z110|Z19|Z18 
	}
	{
		VSHUFF(V24,V29,R6)            ////L...|....|Z615|Z614|Z613|Z612|Z215|Z214|Z213|Z212...|...|Z415|Z414|Z413|Z412|Z015|Z014|Z013|Z012
	}
	{
		VSHUFF(V20,V21,R6)             ////H..|....|Z615|Z614|Z613|Z612|Z215|Z214|Z213|Z212...|...|Z415|Z414|Z413|Z412|Z015|Z014|Z013|Z012
	}
	{
		VSHUFF(V5,V0,R6)              ////L..|....|Z715|Z714|Z713|Z712|Z315|Z314|Z313|Z312...|...|Z515|Z514|Z513|Z512|Z115|Z114|Z113|Z112
	}
	{
		VSHUFF(V14,V8,R6)              ////H...|...|Z715|Z714|Z713|Z712|Z315|Z314|Z313|Z312...|...|Z515|Z514|Z513|Z512|Z115|Z114|Z113|Z112
	}
	{
		VSHUFF(V18,V31,R10)             ////L...|...|Z47|Z46|Z45|Z44|Z43|Z42|Z41|Z40|...|...Z07|Z06|Z05|Z04|Z03|Z02|Z01|Z00
	}
	{
		VSHUFF(V6,V3,R10)             ////H...|..|...|Z47|Z46|Z45|Z44|Z43|Z42|Z41|Z40|...|...Z07|Z06|Z05|Z04|Z03|Z02|Z01|Z00
	}
	{
		VSHUFF(V26,V30,R10)             ////L....|...|Z67|Z66|Z65|Z64|Z63|Z62|Z61|Z60|...|...Z27|Z26|Z25|Z24|Z23|Z22|Z21|Z20
	}
	{
		VSHUFF(V7,V22,R10)             ////H...|...|Z67|Z66|Z65|Z64|Z63|Z62|Z61|Z60|...|...Z27|Z26|Z25|Z24|Z23|Z22|Z21|Z20
	}
	{
		VSHUFF(V9,V11,R10)             ////L...|...|Z57|Z56|Z55|Z54|Z53|Z52|Z51|Z50|...|...Z17|Z16|Z15|Z14|Z13|Z12|Z11|Z10
	}
	{
		VSHUFF(V10,V28,R10)             ////H...|...|Z57|Z56|Z55|Z54|Z53|Z52|Z51|Z50|...|...Z17|Z16|Z15|Z14|Z13|Z12|Z11|Z10
	}
	{
		VSHUFF(V1,V25,R10)             ////L...|...|Z77|Z76|Z75|Z74|Z73|Z72|Z71|Z70|...|...Z37|Z36|Z35|Z34|Z33|Z32|Z31|Z30
	}
	{
		VSHUFF(V17,V16,R10)             ////H...|...|Z77|Z76|Z75|Z74|Z73|Z72|Z71|Z70|...|...Z37|Z36|Z35|Z34|Z33|Z32|Z31|Z30
	}
	{
		VSHUFF(V29,V15,R10)             ////L...|....Z415|Z414|Z413|Z412|Z411|Z410|Z49|Z48...|...Z015|Z014|Z013|Z012|Z011|Z010|Z09|Z08
	}{
		VSHUFF(V21,V27,R10)             ////H...|....Z415|Z414|Z413|Z412|Z411|Z410|Z49|Z48...|...Z015|Z014|Z013|Z012|Z011|Z010|Z09|Z08
	}
	{
		VSHUFF(V24,V19,R10)             ////L...6 | 2   15 14 13 12 11 10 9 8
	}{
		VSHUFF(V20,V4,R10)             ////H....6 | 2   15 14 13 12 11 10 9 8
	}
	{
		VSHUFF(V0,V23,R10)            ////L.....5 | 1   15 14 13 12 11 10 9 8
	}
	{
		VSHUFF(V8,V12,R10)             ////H....5 | 1   15 14 13 12 11 10 9 8
	}
	{
		VSHUFF(V5,V13,R10)              ////L....7 | 3   15 14 13 12 11 10 9 8
	}
	{
		VSHUFF(V14,V2,R10)              ////H.....7 | 3   15 14 13 12 11 10 9 8
	}
	/////////////
	{
		VSHUFF(V15,V31,R12)             ////L..8 | 0  15~0  R12=32
	}
	{
		VSHUFF(V27,V3,R12)             ////H...8 | 0  15~0
	}
	{
		VSHUFF(V29,V18,R12)             ////L..12 | 4  15~0
	}
	{
		VSHUFF(V21,V6,R12)             ////H...12 | 4 15~0
	}
	{
		VSHUFF(V19,V30,R12)             ////L.....10 | 2 15~0
	}
	{
		VSHUFF(V4,V22,R12)             ////H....10 | 2 15~0
	}
	{
		VSHUFF(V24,V26,R12)             ////L....14 | 6 15~0
	}
	{
		VSHUFF(V20,V7,R12)             ////H....14 | 6 15~0
	}
	{
		VSHUFF(V23,V11,R12)             ////L....9| 1 15~0
	}{
		VSHUFF(V12,V28,R12)             ////H....9| 1 15~0
	}
	{
		VSHUFF(V0,V9,R12)             ////L.....13 | 5 15~0
	}{
		VSHUFF(V8,V10,R12)             ////H.....13 | 5 15~0
	}
	{
		VSHUFF(V13,V25,R12)            ////L.......11 | 3 15~0
	}
	{
		VSHUFF(V2,V16,R12)             ////H.....11 | 3 15~0
	}
	{
		VSHUFF(V5,V1,R12)              ////L.....15 | 7 15~0
	}
	{
		VSHUFF(V14,V17,R12)              ////H....15 | 7 15~0
	}
	/////////////////////////////////////////////////////
	{
		VSHUFF(V11,V31,R13)            ////L.......11 | 3 15~0
		
	}
	{
		VMEM(R0++#1) = V31         // store Z1 | Z0 R
		VSHUFF(V25,V30,R13)            
	}
	{
		VMEM(R0++#1) = V30         // store Z3 | Z2 R
		VSHUFF(V9,V18,R13)             
	}
	{
		VMEM(R0++#1) = V18         // store Z5 | Z4 R
		VSHUFF(V1,V26,R13)             
	}
	{
		VMEM(R0++#1) = V26         // store Z7 | Z6 R
		VSHUFF(V3,V15,R13)             
	}
	
	{
		VMEM(R0++#1) = V15         // store Z0 G1 | Z8 R
		VSHUFF(V22,V28,R13)            
	}
	{
		VMEM(R0++#1) = V28         // store Z2 | Z1 G1
		VSHUFF(V6,V16,R13)              
	}
	{
		VMEM(R0++#1) = V16         // store Z4 | Z3 G1
		VSHUFF(V7,V10,R13)              
	}
	{
		VMEM(R0++#1) = V10         // store Z6 | Z5 G1
		VSHUFF(V27,V17,R13)              
	}
	{
		VMEM(R0++#1) = V17         // store Z8 | Z7 G1
		
	}
	{
		VMEM(R0++#1) = V11         // store Z1 | Z0 G2
		
	}
	{
		VMEM(R0++#1) = V25         // store Z3 | Z2 G2
		
	}
	{
		VMEM(R0++#1) = V9         // store Z7 | Z6 G2
		
	}
	{
		VMEM(R0++#1) = V1         // store Z7 | Z6 G2
		
	}
	{
		VMEM(R0++#1) = V3         // store Z7 B | Z6 G2
		
	}
	{
		VMEM(R0++#1) = V22         // store Z7 | Z6 R
		
	}
	
	{
		VMEM(R0++#1) = V6         // store Z7 | Z6 R
		
	}
	{
		VMEM(R0++#1) = V7         // store Z7 | Z6 R
		
	}
	{
		VMEM(R0++#1) = V27         // store Z7 | Z6 R
		
	}
	{
      dealloc_return                               //Q
   }

.size RFFT16x16_4_asm, .-RFFT16x16_4_asm

#endif

#if __hexagon__

.text

/**************************************************************/
/* Function:    RIFFT16x16_4_asm                                    */
/**************************************************************/
/* Reg Inputs:  R0 = real_t *pOutput                          */
/*              R1 = uint16_t nOutputStride                   */
/*              R2 = complex_t *pInput                        */
/* Reg Outputs: None                                          */
/* SideEffects: R0-R14,M0,M1,V0-V31                          */
/* Stack Usage: 0                                             */
/* Notes:       Callable from C                               */
/* Cycles:                                                 */
/**************************************************************/

#define COS_PI_8 30274
#define SIN_PI_8 12540
#define COS_PI_4 23170
#define nInputStride_18 1152    //18*64
.p2align 4
.globl RIFFT16x16_4_asm
.type RIFFT16x16_4_asm, @function

RIFFT16x16_4_asm:
    {   
		allocframe(#256)                              // tmp On stack	
		R3 = ADD(R2,#nInputStride_18 )              // G2   base + 64*18
    }
    {
	   V0 = VMEM(R2++#1)      //R 1 0 
       R8 = #COS_PI_4       // COS_PI_4
	   R13:12 = COMBINE(#64,#32)
    }
	
	{ 
	  R5:4 = COMBINE(#SIZEOF_WORD,#SIZEOF_HALFWORD)  // 4 | 2
      V2.tmp = VMEM(R3++#1)      //G2 1 0 
	  VSHUFF(V2,V0,R13)       // 1 RG | 0 RG
	}

	{
		R7:6 = COMBINE(#-SIZEOF_HALFWORD,#SIZEOF_DOUBLEWORD)   // -2|8
		V4 = VMEM(R2++#1)      //R 3 2
	}	
	
	{
		R8 = COMBINE(R8.l,R8.l)    // COS_PI_4|COS_PI_4
        R9 = #SIN_PI_8      // SIN_PI_8
		V6.tmp = VMEM(R3++#1)      //G2 3 2
	    VSHUFF(V6,V4,R13)       // 3 R G2 | 2 R G2
	}
	{
		R9 = COMBINE(R9.l,R9.l)    // SIN_PI_8|SIN_PI_8
		V8 = VMEM(R2++#1)      //R 5 4
	  
	}
	{	
        //R10 = #SIZEOF_QUADRUPLEWORD  //16
		R10 = #16
	    V10.tmp = VMEM(R3++#1)      //G2 5 4
	    VSHUFF(V10,V8,R13)       // 5 RG2 | 4 RG2
    } 
    { 
	    R11 = #COS_PI_8             //COS_PI_8
	    V12 = VMEM(R2++#1)      //R 7 6
    }
    {
        R11 = COMBINE(R11.l,R11.l)    //COS_PI_8|COS_PI_8
	    V14.tmp = VMEM(R3++#1)      //G2 7 6
	    VSHUFF(V14,V12,R13)       // 7 RG2 | 6 RG2
    }
	{
		V16 = VMEM(R2++#1)      //R 8 G1 0
    }
	{
	    V1.tmp = VMEM(R3++#1)      //G2 8 B 0
	    VSHUFF(V1,V16,R13)       // 0 G1B | 8 RG2
	}
	{
		V3 = VMEM(R2++#1)      //G1 2 G1 1
		V1:0.h = VSHUFFOE(V1.h,V0.h)   // v1 i V0 r R G2 G1 B  x0
    }
	
	
	{
	    V5.tmp = VMEM(R3++#1)      //B 2 B 1
	    VSHUFF(V5,V3,R13)       // 2 G1B | 1 G1B 
	}
	{
		V7 = VMEM(R2++#1)      //G1 G1 0
		V3:2.h = VSHUFFOE(V3.h,V2.h)   // v3 i V2 r R G2 G1 B  x1
    }
	{
	
		V0.h=VDEAL(V0.h)
	}		
	{
		V1.h=VDEAL(V1.h) 
	}
	{
	
		V2.h=VDEAL(V2.h)
	}	
	{
		V3.h=VDEAL(V3.h) 
	}
	{
	    V9.tmp = VMEM(R3++#1)      //B 8 B 0
	    VSHUFF(V9,V7,R13)       // 4 G1B | 3 G1B
		V5:4.h = VSHUFFOE(V5.h,V4.h)   // v5 i V4 r R G2 G1 B x2
	}
	
	{
		V11 = VMEM(R2++#1)      //G1 8 G1 0
		V7:6.h = VSHUFFOE(V7.h,V6.h)   // v7 i V6 r R G2 G1 B x3
    }
	{
		V4.h=VDEAL(V4.h)
	}	
	
	{
		V5.h=VDEAL(V5.h) 
	}
	{
	
		V6.h=VDEAL(V6.h)
	}	
	{
		V7.h=VDEAL(V7.h) 
	}
	{
	    V13.tmp = VMEM(R3++#1)      //B 8 B 0
	    VSHUFF(V13,V11,R13)       // 6 G1B | 5 G1B
		V9:8.h = VSHUFFOE(V9.h,V8.h)   // v9 i V8 r R G2 G1 B x4
	}
	
	{
		V15 = VMEM(R2++#1)      //G1 8 G1 0
		V11:10.h = VSHUFFOE(V11.h,V10.h)   // v11 i V10 r R G2 G1 B x5
    }
	{
	
		V8.h=VDEAL(V8.h)
	}	
	
	{
		V9.h=VDEAL(V9.h) 
	}
	{
	
		V10.h=VDEAL(V10.h)
	}	
	
	{
		V11.h=VDEAL(V11.h) 
	}
	{
	    V17.tmp = VMEM(R3++#1)      //B 8 B 0
	    VSHUFF(V17,V15,R13)       // 8 G1B | 7 G1B
		V13:12.h = VSHUFFOE(V13.h,V12.h)   // v13 i V12 r R G2 G1 B x6
	}
	
	
	{	 
	  V15:14.h = VSHUFFOE(V15.h,V14.h)   // V15 i V14 r R G2 G1 B x7
	  VSHUFF(V3,V1,R4)             //x15 1|x15 0|...|...|x31|x30|x11|x10|x14 1|x14 0|...|...|x21|x20|x01|x00      
   }
   	{
	
		V12.h=VDEAL(V12.h)
	}	
	
	{
		V13.h=VDEAL(V13.h) 
	}
	{
	
		V14.h=VDEAL(V14.h)
	}	
	
	{
		V15.h=VDEAL(V15.h) 
	}
   {
      V17:16.h = VSHUFFOE(V17.h,V16.h)    // V17 i V16 r R G2 G1 B x8
	  VSHUFF(V7,V5,R4)				//...|...|x33|x32|x13|x12........|x23|x22|x03|x02 		     
   }
	{                                                 
		VSHUFF(V11,V9,R4)              //....|x35|x34|x15|x14|...|x25|x24|x05|x04 
		V21:20 = #0
	}
	{                                                 
		VSHUFF(V15,V13,R4)              //...|x37|x36|x17|x16|....|x27|x26|x07|x06
		V23:22 = #0		
	}
	{
	
		V16.h=VDEAL(V16.h)
	}	
	
	{
		V17.h=VDEAL(V17.h) 
	}
	{
		VSHUFF(V5,V1,R5)            //x63|x62|x61|x60|x23|x22|x21|x20|...|x43|x42|x41|x40|x03|x02|x01|x00  
		V25:24 = #0                 
	}
	{
		VSHUFF(V7,V3,R5)         //...|x73|x72||x71|x70|x33|x32|x31|x30|...|x53|x52|x51|x50|x13|x12|x11|x10 
		V27:26 = #0
		
	}
	{
		VSHUFF(V13,V9,R5)           //|x67|x66|x65|x64|x27|x26|x25|x24|x47|x46|x45|x44|x07|x06|x05|x04   
		
	}
	{
		VSHUFF(V15,V11,R5)        //.....|x77|x76|x75|x74|x37|x36|x35|x34|...|x57|x56|x55|x54|x17|x16|x15|x14 
		
	}
	{
		VSHUFF(V9,V1,R6)        //x47|x46|x45|x44|x43|x42|x41|x40|x07|x06|x05|x04|x03|x02|x01|x00
		
		V19:18.uw = VZXT(V17.uh)		    ////  x15 9|x15 8|...|...|x39|x38|x19|x18|x14 9|x14 8|...|...|x29|x28|x09|x08 
	}
	{
		VSHUFF(V11,V3,R6)     //x57|x56|x55|x54|x53|x52|x51|x50|x17|x16|x15|x14|x13|x12|x11|x10 
		V29:28 = #0  		
	}
	{
		VSHUFF(V20,V18,R5)        //   x611|x610|x69|x68|x211|x210|x29|x28|...|x411|x410|x49|x48|x011|x010|x09|x08 
		V31:30 = #0		
	}
	{
		VSHUFF(V21,V19,R5)        //  x711|x710|x79|x78|x311|x310|x39|x38|...|x511|x510|x59|x58|x111|x110|x19|x18
		V17:16.uw = VZXT(V16.uh)		    ////r  x15 9|x15 8|...|...|x39|x38|x19|x18|x14 9|x14 8|...|...|x29|x28|x09|x08 		
	}
	
	{
		VSHUFF(V22,V18,R6)        //   x415|x414|x413|x412|x411|x410|x49|x48|x015|x014|x013|x012|x011|x010|x09|x08 
	
	}
	{	
		VSHUFF(V24,V20,R6)     //   x615|x614|x613|x612|x611|x610|x69|x68|x215|x214|x213|x212|x211|x210|x29|x28 		
		
	}
	{
		VSHUFF(V23,V19,R6)   //  x515|x514|x513|x512|x511|x510|x59|x58|x115|x114|x113|x112|x111|x110|x19|x18   
		
	}
	{
		VSHUFF(V25,V21,R6)     //  x715|x714|x713|x712|x711|x710|x79|x78|x315|x314|x313|x312|x311|x310|x39|x38	
	}
	{           
		VSHUFF(V18,V1,R10)    // x8i | x0i        
	}	 
	
	{                 
		VSHUFF(V19,V3,R10)    // x9i | x1i  
		V1.h = VAVG(V1.h,V18.h):rnd // A00i=x0i+x8i
		V18.h = VNAVG(V1.h,V18.h)    // A01i=x0i-x8i	
	} 
	
	{          
		VSHUFF(V13,V5,R6)       //x67|x66|x65|x64|x63|x62|x61|x60|x27|x26|x25|x24|x23|x22|x21|x20
		V3.h = VAVG(V3.h,V19.h):rnd // A10i=x1i+x9i
		V19.h = VNAVG(V3.h,V19.h)    // A11i=x1i-x9i		
	}
	{	    
		VSHUFF(V15,V7,R6)        //x77|x76|x75|x74|x73|x72|x71|x70|x37|x36|x35|x34|x33|x32|x31|x30  
	}
	{
		VSHUFF(V20,V5,R10)   // x10i | x2i	
	}
	{          
		VSHUFF(V21,V7,R10)     // x11i | x3i
		V5.h = VAVG(V5.h,V20.h):rnd // A20i=x2i+x10i
		V20.h = VNAVG(V5.h,V20.h)    // A21i=x2i-x10i
	}
	{	  
		VSHUFF(V22,V9,R10)    // x12i | x4i
		V7.h = VAVG(V7.h,V21.h):rnd // A30i=x3i+x11i
		V21.h = VNAVG(V7.h,V21.h)    // A31i=x3i-x11i
	}	
	{
		VSHUFF(V23,V11,R10)     // x13i | x5i 
		V9.h = VAVG(V9.h,V22.h):rnd // A02i=x4i+x12i
		V22.h = VNAVG(V9.h,V22.h)    // A03i=x4i-x12i	
	} 
	{			   
		VSHUFF(V24,V13,R10)    // x14i | x6i  
		V11.h = VAVG(V11.h,V23.h):rnd // A12i=x5i+x13i
		V23.h = VNAVG(V11.h,V23.h)    // A13i=x5i-x13i			
	}
	{
		VSHUFF(V25,V15,R10)    // x15i | x7i  
		V13.h = VAVG(V13.h,V24.h):rnd // A22i=x6i+x14i
		V24.h = VNAVG(V13.h,V24.h)    // A23i=x6i-x14i		
	}
   	{
		VSHUFF(V2,V0,R4)             //r  x15 1|x15 0|...|...|x31|x30|x11|x10|x14 1|x14 0|...|...|x21|x20|x01|x00                                            					
		V15.h = VAVG(V15.h,V25.h):rnd // A32i=x7i+x15i
		V25.h = VNAVG(V15.h,V25.h)    // A33i=x7i-x15i	
	}
	{                                                 
		VSHUFF(V6,V4,R4)				//r  ...|...|x33|x32|x13|x12........|x23|x22|x03|x02
		V1.h = VAVG(V1.h,V9.h):rnd // B00i=A00i+A02i
		V9.h = VNAVG(V1.h,V9.h)    //anTmp1_02i B02i=A00i-A02i
	}
	{                                                 
		VSHUFF(V10,V8,R4)              //r  ....|x35|x34|x15|x14|...|x25|x24|x05|x04 
		V3.h = VAVG(V3.h,V11.h):rnd // B10i=A10i+A12i
		V11.h = VNAVG(V3.h,V11.h)    // B12i=A10i-A12i
	}
	{                                                 
		VSHUFF(V14,V12,R4)              //r  ...|x37|x36|x17|x16|....|x27|x26|x07|x06 
		V5.h = VAVG(V5.h,V13.h):rnd // B20i=A20i+A22i
		V13.h = VNAVG(V5.h,V13.h)    // anTmp1_22r B22i=A20i-A22i
	}
	{
		VSHUFF(V4,V0,R5)            //r  x63|x62|x61|x60|x23|x22|x21|x20|...|x43|x42|x41|x40|x03|x02|x01|x00  
		V7.h = VAVG(V7.h,V15.h):rnd // B30i=A30i+A32i
		V15.h = VNAVG(V7.h,V15.h)    // B32i=A30i-A32i
		                 
	}
	{
		
		VSHUFF(V12,V8,R5)           //r  |x67|x66|x65|x64|x27|x26|x25|x24|x47|x46|x45|x44|x07|x06|x05|x04   		
		V1.h = VAVG(V1.h,V5.h):rnd // C00i=B00i+B20i
		V5.h = VNAVG(V1.h,V5.h)    // C10i=B00i-B20i
		
	}
	{
		VSHUFF(V26,V16,R5)        // r  x611|x610|x69|x68|x211|x210|x29|x28|...|x411|x410|x49|x48|x011|x010|x09|x08 
		V3.h = VAVG(V3.h,V7.h):rnd // C20i=B10i+B30i
		V7.h = VNAVG(V3.h,V7.h)    // C30i=B10i-B30i
	}

	{
		VSHUFF(V8,V0,R6)        //r  x47|x46|x45|x44|x43|x42|x41|x40|x07|x06|x05|x04|x03|x02|x01|x00
		V1.h = VAVG(V1.h,V3.h):rnd // out1_0i=C00i+C20i
		V3.h = VNAVG(V1.h,V3.h)    // out1_8i=C00i-C20i
	}
	{
		
		VSHUFF(V28,V16,R6)        // r  x415|x414|x413|x412|x411|x410|x49|x48|x015|x014|x013|x012|x011|x010|x09|x08 
		V15.h = VMPY(V15.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B32i 
	}
	{
		VSHUFF(V6,V2,R5)         //r  ...|x73|x72||x71|x70|x33|x32|x31|x30|...|x53|x52|x51|x50|x13|x12|x11|x10
		V11.h = VMPY(V11.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B12i
		
	}
	{           
		VSHUFF(V16,V0,R10)    // x8r | x0r        
	}
	{
		VSHUFF(V14,V10,R5)        //r .....|x77|x76|x75|x74|x37|x36|x35|x34|...|x57|x56|x55|x54|x17|x16|x15|x14 
		V0.h = VAVG(V0.h,V16.h):rnd // A00r=x0r+x8r
		V16.h = VNAVG(V0.h,V16.h)    // A01r=x0r+x8r	
	}

	{
		VSHUFF(V27,V17,R5)        // r x711|x710|x79|x78|x311|x310|x39|x38|...|x511|x510|x59|x58|x111|x110|x19|x18
		V16.h = VAVG(V16.h,V22.h):rnd // anTmp1_03r = B03r=A01r+A03i
		V22.h = VNAVG(V16.h,V22.h)    //anTmp1_01r = B01r=A01r-A03i	
	}
	{
		VSHUFF(V10,V2,R6)     //r  x57|x56|x55|x54|x53|x52|x51|x50|x17|x16|x15|x14|x13|x12|x11|x10   	
		
	}
	
	{
		VSHUFF(V29,V17,R6)   //r  x515|x514|x513|x512|x511|x510|x59|x58|x115|x114|x113|x112|x111|x110|x19|x18   
		
	}
	{
		VSHUFF(V31,V27,R6)     //r  x715|x714|x713|x712|x711|x710|x79|x78|x315|x314|x313|x312|x311|x310|x39|x38	
	}
	{                 
		VSHUFF(V17,V2,R10)    // x9r | x1r  		
	} 
	
	{	
		VSHUFF(V30,V26,R6)     // r  x615|x614|x613|x612|x611|x610|x69|x68|x215|x214|x213|x212|x211|x210|x29|x28 		
		V2.h = VAVG(V2.h,V17.h):rnd // A10r=x1r+x9r
		V17.h = VNAVG(V2.h,V17.h)    // A11r=x1r+x9r
	}
	{          
		VSHUFF(V12,V4,R6)       //r x67|x66|x65|x64|x63|x62|x61|x60|x27|x26|x25|x24|x23|x22|x21|x20
		V17.h = VAVG(V17.h,V23.h):rnd // B13r=A11r+A13i
		V23.h = VNAVG(V17.h,V23.h)    // B11r=A11r-A13i			
	}
	{	    
		VSHUFF(V14,V6,R6)        //r x77|x76|x75|x74|x73|x72|x71|x70|x37|x36|x35|x34|x33|x32|x31|x30  
	}
	{
		VSHUFF(V26,V4,R10)   // x10r | x2r	
	}
	
	{          
		VSHUFF(V27,V6,R10)     // x11r | x3r
		V4.h = VAVG(V4.h,V26.h):rnd // A20r=x2r+x10r
		V26.h = VNAVG(V4.h,V26.h)    // A21r=x2r-x10r
	}
	
	{	  
		VSHUFF(V28,V8,R10)    // x12r | x4r
		V6.h = VAVG(V6.h,V27.h):rnd // A30r=x3r+x11r
		V27.h = VNAVG(V6.h,V27.h)    // A31r=x3r-x11r
	}	
	{
		VSHUFF(V29,V10,R10)     // x13r | x5r 
		V8.h = VAVG(V8.h,V28.h):rnd // A02r=x4r+x12r
		V28.h = VNAVG(V8.h,V28.h)    // A03r=x4r-x12r	
	} 

	{			   
		VSHUFF(V30,V12,R10)    // x14r | x6r
		V10.h = VAVG(V10.h,V29.h):rnd // A12r=x5r+x13r
		V29.h = VNAVG(V10.h,V29.h)    // A13r=x5r-x13r	  		
	}
	{
		VSHUFF(V31,V14,R10)    // x15r | x7r
		V19.h = VAVG(V19.h,V29.h):rnd   // B11i=A11i+A13r
		V29.h = VNAVG(V19.h,V29.h)     // B13i=A11i-A13r
	}
	{
		VMEMU(SP) = V15        //COS_PI_4 * B32i 
		V14.h = VAVG(V14.h,V31.h):rnd // A32r=x7r+x15r
		V31.h = VNAVG(V14.h,V31.h)    // A33r=x7r-x15r				
	}
	/////////////////////////////////////////
	
	{
		//SIN_PI_8, anStage1[1*4+1].i
		V15.h = VMPY(V19.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B11i 
		V6.h = VAVG(V6.h,V14.h):rnd // B30r=A30r+A32r
		V14.h = VNAVG(V6.h,V14.h)   // B32r=A30r-A32r	
	}	
	{
		//COS_PI_8, anStage1[1*4+1].i
		V19.h = VMPY(V19.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B11i 
		V2.h = VAVG(V2.h,V10.h):rnd // B10r=A10r+A12r
		V10.h = VNAVG(V2.h,V10.h)    //B12r=A10r-A12r                 
	}
	{                                                 
		V14.h = VMPY(V14.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B32r 
		V0.h = VAVG(V0.h,V8.h):rnd // B00r=A00r+A02r
		V8.h = VNAVG(V0.h,V8.h)    // anTmp1_02r B02r=A00r-A02r
	}
	{
		VMEMU(SP+#1) = V14        //COS_PI_4 * B32r 
		V12.h = VAVG(V12.h,V30.h):rnd // A22r=x6r+x14r
		V30.h = VNAVG(V12.h,V30.h)    // A23r=x6r-x14r
	}
	{                                                 
		V14.h = VMPY(V23.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B11r 
		V26.h = VAVG(V26.h,V24.h):rnd // B23r=A21r+A23i
		V24.h = VNAVG(V26.h,V24.h)    // B21r=A21r-A23i
		
	}
	{
		V23.h = VMPY(V23.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B11r 
		V20.h = VAVG(V20.h,V30.h):rnd   // B21i=A21i+A23r
		V30.h = VNAVG(V20.h,V30.h)     //  B23i=A21i-A23r
	}
	{	
		V24.h = VMPY(V24.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B21r
		V21.h = VAVG(V21.h,V31.h):rnd   // B31i=A31i+A33r
		V31.h = VNAVG(V21.h,V31.h)      // B33i=A31i-A33r		
	}
	{
		V20.h = VMPY(V20.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B21i 
		V4.h = VAVG(V4.h,V12.h):rnd // B20r=A20r+A22r
		V12.h = VNAVG(V4.h,V12.h)   //anTmp1_22i B22r=A20r-A22r
	}	
	{
		V14.h = VMPY(V21.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B31i 
		V19.h = VSUB(V23.h,V15.h)     // anTmp1_11r =COS_PI_8 * B11r - 	SIN_PI_8* B11i
		V23.h = VADD(V14.h,V19.h)      // anTmp1_11i =SIN_PI_8 * B11r + COS_PI_8 * B11i
	}
	{
		V21.h = VMPY(V21.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B31i 
		V27.h = VAVG(V27.h,V25.h):rnd // B33r=A31r+A33i
		V25.h = VNAVG(V27.h,V25.h)    // B31r=A31r-A33i		
	}
	{		
		V10.h = VMPY(V10.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B12r
		V0.h = VAVG(V0.h,V4.h):rnd // C00r=B00r+B20r
		V4.h = VNAVG(V0.h,V4.h)    // C10r=B00r-B20r
	}
	{	
		V15.h = VMPY(V25.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B31r 
		V18.h = VAVG(V18.h,V28.h):rnd   // anTmp1_01i = B01i=A01i+A03r
		V28.h = VNAVG(V18.h,V28.h)     // anTmp1_03i = B03i=A01i-A03r
		
	}
	{
		V25.h = VMPY(V25.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B31r 
		V2.h = VAVG(V2.h,V6.h):rnd // C20r=B10r+B30r
		V6.h = VNAVG(V2.h,V6.h)    // C30r=B10r-B30r
		
	}
	    
	{
		V26.h = VMPY(V26.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B23r 
		V21.h = VSUB(V15.h,V21.h)     // anTmp1_31r =SIN_PI_8 * B31r - 	COS_PI_8* B31i
		V25.h = VADD(V25.h,V14.h)      // anTmp1_31i =COS_PI_8 * B31r + SIN_PI_8 * B31i 	
		
		
	}
	{
		V30.h = VMPY(V30.h,R8.h):<<1:rnd:sat   //COS_PI_4 * B23i 
		V0.h = VAVG(V0.h,V2.h):rnd // out1_0r=C00r+C20r
		V2.h = VNAVG(V0.h,V2.h)    // out1_8r=C00r-C20r
	}
	
	
	
	{
		V14.h = VMPY(V17.h,R9.h):<<1:rnd:sat   //SIN_PI_8*B13r
		V4.h = VAVG(V4.h,V7.h):rnd // out1_12r=C10r+C30i
		V7.h = VNAVG(V4.h,V7.h)    // out1_4r=C10r-C30i
	}
	{
		V17.h = VMPY(V17.h,R11.h):<<1:rnd:sat   //COS_PI_8*B13r
		V5.h = VAVG(V5.h,V6.h):rnd // out1_4i=C10i+C30r
		V6.h = VNAVG(V5.h,V6.h)    // out1_12i=C10i-C30r		
		
	}

	{ 	
		V15.h = VMPY(V29.h,R9.h):<<1:rnd:sat   //SIN_PI_8*B13i 
		V20.h = VSUB(V24.h,V20.h)     // anTmp1_21r =COS_PI_4 * B21r - 	COS_PI_4* B21i
		V24.h = VADD(V24.h,V20.h)      // anTmp1_21i =COS_PI_4 * B21r + COS_PI_4 * B21i		
		
	}
	{
		V29.h = VMPY(V29.h,R11.h):<<1:rnd:sat   //COS_PI_8*B13i
		V20.h = VAVG(V22.h,V20.h):rnd // C01r=anTmp1_01r+anTmp1_21r
		V22.h = VNAVG(V22.h,V20.h)    // C11r=anTmp1_01r-anTmp1_21r
	}
		
	
	{ 	
		V14.h = VMPY(V31.h,R11.h):<<1:rnd:sat   //COS_PI_8 * B33i
		V17.h = VSUB(V14.h,V29.h)     // anTmp1_13r =SIN_PI_8 * B13r - 	COS_PI_8* B13i
		V29.h = VADD(V17.h,V15.h)      // anTmp1_13i =COS_PI_8 * B13r + SIN_PI_8 * B13i			
	}
	{
		V31.h = VMPY(V31.h,R9.h):<<1:rnd:sat   //SIN_PI_8 * B33i
		V19.h = VAVG(V19.h,V21.h):rnd // C21r=anTmp1_11r+anTmp1_31r
		V21.h = VNAVG(V19.h,V21.h)    // C31r=anTmp1_11r-anTmp1_31r
	}
	{
		V15.h = VMPY(V27.h,R11.h):<<1:rnd:sat   //COS_PI_8* B33r
		V20.h = VAVG(V20.h,V19.h):rnd // out1_1r=C01r+C21r
		V19.h = VNAVG(V20.h,V19.h)    // out1_9r=C01r-C21r		
	}
	{
		V27.h = VMPY(V27.h,R9.h):<<1:rnd:sat   //SIN_PI_8* B33r
		V18.h = VAVG(V18.h,V24.h):rnd // C01i=anTmp1_01i+anTmp1_21i
		V24.h = VNAVG(V18.h,V24.h)    // C11i=anTmp1_01i-anTmp1_21i
	}
	{	
		VSHUFF(V20,V0,R4)    
		V31.h = VSUB(V31.h,V15.h)     // anTmp1_33r =SIN_PI_8 * B33i  - 	COS_PI_8* B33r
		V27.h = VADD(V14.h,V27.h)   // anTmp1_33i =COS_PI_8 * B33i + SIN_PI_8 * B33r	
	}
	
	{
		V14 = VMEMU(SP+#1)   //COS_PI_4 * B32r 
		V23.h = VAVG(V23.h,V25.h):rnd // C21i=anTmp1_11i+anTmp1_31i
		V25.h = VNAVG(V23.h,V25.h)    // C31i=anTmp1_11i-anTmp1_31i
	}
	{
		V15 = VMEMU(SP)    //COS_PI_4 * B32i 
		V18.h = VAVG(V18.h,V23.h):rnd // out1_1i=C01i+C21i
		V23.h = VNAVG(V18.h,V23.h)    // out1_9i=C01i-C21i		
	}
		
	{
		VSHUFF(V19,V2,R4)
		V22.h = VAVG(V22.h,V25.h):rnd // out1_13r=C11r+C31i
		V25.h = VNAVG(V22.h,V25.h)    // out1_5r=C11r-C31i		
	}
	{
		VSHUFF(V18,V1,R4)
		V24.h = VAVG(V24.h,V21.h):rnd // out1_5i=C11i+C31r
		V21.h = VNAVG(V24.h,V21.h)    // out1_13i=C11i-C31r		
	}
		
	{ 	
		VSHUFF(V23,V3,R4)
		V26.h = VADD(V26.h,V30.h)      // anTmp1_23r =COS_PI_4 * B23r + COS_PI_4 * B23i	
		V30.h = VSUB(V26.h,V30.h)     // anTmp1_23i =COS_PI_4 * B23r - 	COS_PI_4* B23i		
	}
	{
		VSHUFF(V24,V5,R4)
		V16.h = VAVG(V16.h,V26.h):rnd // C13r =anTmp1_03r+anTmp1_23r
		V26.h = VNAVG(V16.h,V26.h)    // C03r=anTmp1_03r-anTmp1_23r
	}
	{
		VSHUFF(V25,V7,R4)
		V17.h = VAVG(V17.h,V31.h):rnd // C23r=anTmp1_13r+anTmp1_33r
		V31.h = VNAVG(V17.h,V31.h)    // C33r=anTmp1_13r-anTmp1_33r
	}
	{
		VSHUFF(V21,V6,R4)
		V26.h = VAVG(V26.h,V17.h):rnd // out1_3r= C03r+C23r
		V17.h = VNAVG(V26.h,V17.h)    // out1_11r=C03r-C23r	
	}
	{
		VSHUFF(V22,V4,R4)	
		V13.h = VAVG(V8.h,V13.h):rnd // C12r= anTmp1_02r+anTmp1_22r
		V8.h = VNAVG(V8.h,V13.h)    // C02r= anTmp1_02r-anTmp1_22r	
	}
	{
		V14.h = VADD(V14.h,V15.h)      // anTmp1_32r =COS_PI_4 * B32r + COS_PI_4* B32i	
		V15.h = VSUB(V14.h,V15.h)      // anTmp1_32i =COS_PI_4 * B32r - COS_PI_4* B32i		
		V11.h = VSUB(V10.h,V11.h)     // anTmp1_12r =COS_PI_4 * B12r - 	COS_PI_4* B12i	
		V10.h = VADD(V10.h,V11.h)      // anTmp1_12i =COS_PI_4 * B12r + COS_PI_4 * B12i				
	}
	
	{		
		V11.h = VAVG(V11.h,V14.h):rnd // C32r= anTmp1_12r+anTmp1_32r
		V14.h = VNAVG(V11.h,V14.h)    // C22r= anTmp1_12r+anTmp1_32r
		V9.h = VAVG(V9.h,V12.h):rnd // C02i = anTmp1_02i+anTmp1_22i
		V12.h = VNAVG(V9.h,V12.h)    // C12i= anTmp1_02i-anTmp1_22i	
	}
	{
		V15.h = VAVG(V10.h,V15.h):rnd // C22i = anTmp1_12i+anTmp1_32i
		V10.h = VNAVG(V10.h,V15.h)    // C32i = anTmp1_12i+anTmp1_32i
		V8.h = VAVG(V8.h,V14.h):rnd // out1_2r= C02r+C22r
		V14.h = VNAVG(V8.h,V14.h)    // out1_10r=C02r-C22r		
	}
	{
		VSHUFF(V26,V8,R4)	
		V9.h = VAVG(V9.h,V15.h):rnd // out1_2i= C02i+C22i
		V15.h = VNAVG(V9.h,V15.h)    // out1_10i=C02i-C22i		
	}

	{
		VSHUFF(V17,V14,R4)	
		V29.h = VAVG(V29.h,V27.h):rnd // C33i =anTmp1_13i+anTmp1_33i
		V27.h = VNAVG(V29.h,V27.h)    // C23i =anTmp1_13i-anTmp1_33i	
	}
	{
		VSHUFF(V26,V20,R5)
		V28.h = VAVG(V28.h,V30.h):rnd // C03i =anTmp1_03i+anTmp1_23i
		V30.h = VNAVG(V28.h,V30.h)    // C13i =anTmp1_03i-anTmp1_23i
	}
	{
		VSHUFF(V8,V0,R5)
		V28.h = VAVG(V28.h,V27.h):rnd // out1_3i= C03i+C23i
		V27.h = VNAVG(V28.h,V27.h)    // out1_11i=C03i-C23i		
	}

	{
		VSHUFF(V28,V9,R4)
		V16.h = VAVG(V16.h,V29.h):rnd // out1_15r= C13r+C33i
		V29.h = VNAVG(V16.h,V29.h)    // out1_7r=C13r-C33i		
	}
	
	{
		VSHUFF(V27,V15,R4)
		V30.h = VAVG(V30.h,V31.h):rnd // out1_7i= C13i+C33r
		V31.h = VNAVG(V30.h,V31.h)    // out1_15i=C13i-C33r		
	}
	
		
	{
		VSHUFF(V9,V1,R5)
		V13.h = VAVG(V13.h,V10.h):rnd // out1_14r= C12r+C32i
		V10.h = VNAVG(V13.h,V10.h)    // out1_6r=  C12r-C32i		
	}
	{
		VSHUFF(V28,V18,R5)
		V12.h = VAVG(V12.h,V11.h):rnd // out1_6i= C12i+C32r
		V11.h = VNAVG(V12.h,V11.h)    // out1_14i=C12i-C32r		
	}
	
	{
		VSHUFF(V14,V2,R5)
	
	}
	{
		VSHUFF(V29,V10,R4)
	}
	{
		VSHUFF(V16,V13,R4)
	}
	{
		VSHUFF(V10,V7,R5)
	}
	{
		VSHUFF(V29,V25,R5)
	}
	{
		VSHUFF(V17,V19,R5)
	}
	{
		VSHUFF(V13,V4,R5)
	}
	{
		VSHUFF(V16,V22,R5)
	}
	{
		VSHUFF(V7,V0,R6)
	}
	{
		VSHUFF(V4,V2,R6)
	}
	{
		VSHUFF(V25,V20,R6)
	}
	{
		VSHUFF(V22,V19,R6)
	}
	{
		VSHUFF(V10,V8,R6)
	}
	{
		VSHUFF(V13,V14,R6)
	}
	{
		VSHUFF(V29,V26,R6)
	}
	{
		VSHUFF(V16,V17,R6)
	}
	{
		VSHUFF(V2,V0,R10)  // V2 | V0 = Y8r | Y0r
	}
	
	{
		VSHUFF(V4,V7,R10) // V7 = Y4r
		V0.h = VAVG(V0.h,V2.h):rnd // anTmp1_RIFFT1_00r= Y0r+Y8r
		V2.h = VNAVG(V0.h,V2.h)    // anTmp1_RIFFT1_01r= Y0r-Y8r
	}
	
	{
		VSHUFF(V19,V20,R10) //  V20 = Y1r
		V7.h = VAVG(V7.h,V7.h):rnd // anTmp1_RIFFT1_02r= Y4r+Y4r
	}
	
	{
		
		VSHUFF(V16,V29,R10)   // V29 = Y7r 
		V0.h = VADD(V0.h,V7.h):sat    //    D00r= anTmp1_RIFFT1_00r+anTmp1_RIFFT1_02r  anStage1
		V7.h = VSUB(V0.h,V7.h):sat    //   anTmp2_RIFFT1_02r D02r= anTmp1_RIFFT1_00r-anTmp1_RIFFT1_02r
	}
	
	{
		VSHUFF(V17,V26,R10) // V26 = Y3r
		
		V19.h = VAVG(V20.h,V29.h):rnd // anTmp1_RIFFT1_10r= Y1r+Y7r
		V16.h = VNAVG(V20.h,V29.h)    // anTmp1_RIFFT1_11r= Y1r-Y7r
	}
	
	
	{
		VSHUFF(V22,V25,R10) // V25 = Y5r
	}
	{
		VSHUFF(V13,V10,R10) // V10 = Y6r
		V4.h = VAVG(V25.h,V26.h):rnd // anTmp1_RIFFT1_12r= Y5r+Y3r
		V17.h = VNAVG(V25.h,V26.h)    // anTmp1_RIFFT1_13r= Y5r-Y3r
	}
	
	
	{
		VSHUFF(V14,V8,R10) // V8 =  Y2r
		V19.h = VADD(V19.h,V4.h):sat    //    D10r= anTmp1_RIFFT1_10r+anTmp1_RIFFT1_12r  anStage1
		V4.h = VSUB(V19.h,V4.h):sat    //    D12r= anTmp1_RIFFT1_10r-anTmp1_RIFFT1_12r
	}
	
	{
		VSHUFF(V31,V11,R4)
		V22.h = VAVG(V8.h,V10.h):rnd // anTmp1_RIFFT1_20r= Y2r+Y6r
		V13.h = VNAVG(V8.h,V10.h)    // anTmp1_RIFFT1_21r= Y2r-Y6r
	}
	{
		VSHUFF(V30,V12,R4)
		V8 = V22                     // anTmp1_RIFFT1_22r= Y6r+Y2r
		V10.h = VNAVG(V10.h,V8.h)    // anTmp1_RIFFT1_23r= Y6r-Y2r
	}
	
	{
		VSHUFF(V12,V5,R5)
		
		V22.h = VADD(V22.h,V8.h):sat    //    D20r= anTmp1_RIFFT1_20r+anTmp1_RIFFT1_22r  anStage1
	}
	{
		VSHUFF(V15,V3,R5)
		V25.h = VAVG(V25.h,V26.h):rnd                   // anTmp1_RIFFT1_30r= Y3r+Y5r
		V26.h = VNAVG(V26.h,V25.h)    // anTmp1_RIFFT1_31r= Y3r-Y5r
	}
	{
		VSHUFF(V11,V6,R5)
		V29.h = VAVG(V20.h,V29.h):rnd 	// anTmp1_RIFFT1_32r= Y7r+Y1r
		V20.h = VNAVG(V29.h,V20.h)    // anTmp1_RIFFT1_33r= Y7r-Y1r
	}	
	{
		VSHUFF(V5,V1,R6)
		V25.h = VADD(V25.h,V29.h):sat    //    D30r= anTmp1_RIFFT1_30r+anTmp1_RIFFT1_32r  anStage1
		V29.h = VSUB(V25.h,V29.h):sat    //     D32r= anTmp1_RIFFT1_30r-anTmp1_RIFFT1_32r
	}

	{
		VSHUFF(V6,V3,R6)
		V0.h = VADD(V0.h,V22.h):sat    //    E00 = D00r+D20r  anStage2
		V22.h = VSUB(V0.h,V22.h):sat    //   E10 = D00r-D20r
	}
		
	
	{
		VSHUFF(V30,V24,R5)
		V4.h = VMPY(V4.h,R8.h):<<1:rnd:sat    //COS_PI_4 * D12r
	}
	{
		VSHUFF(V6,V5,R10) // V5 = Y4i
		V29.h = VMPY(V29.h,R8.h):<<1:rnd:sat   //COS_PI_4 * D32r
	}
	{
		VSHUFF(V31,V21,R5)
		V5.h = VAVG(V5.h,V5.h):rnd   // anTmp1_RIFFT1_03i= Y4i-Y4i
	}
	{
		VSHUFF(V27,V23,R5)
		V2.h = VSUB(V2.h,V5.h):sat    //    anTmp2_RIFFT1_01r = D01r= anTmp1_RIFFT1_01r-anTmp1_RIFFT1_03i  anStage1
		V5.h = VADD(V2.h,V5.h):sat    //  anTmp2_RIFFT1_03r  D03r= anTmp1_RIFFT1_01r+anTmp1_RIFFT1_03i
		
	}

	{
		VSHUFF(V24,V18,R6)
	}
	{
		VSHUFF(V21,V23,R6)
	}

	{
		VSHUFF(V31,V27,R6)
	}
	{
		VSHUFF(V30,V28,R6)
	}
	{
		VSHUFF(V23,V18,R10) //  V18 = Y1i
	}
	{
		VSHUFF(V31,V30,R10)   // V30 = Y7i 
	}
	{
		VSHUFF(V12,V9,R6)
		V31.h = VAVG(V18.h,V30.h):rnd // anTmp1_RIFFT1_11i= Y1i+Y7i
		V14.h = VNAVG(V18.h,V30.h)    // anTmp1_RIFFT1_10i= Y1i-Y7i
	}
	{
		VSHUFF(V11,V15,R6)
		V31.h = VADD(V31.h,V17.h):sat    //    D11i= anTmp1_RIFFT1_11i+anTmp1_RIFFT1_13r  anStage1
		V17.h = VSUB(V31.h,V17.h):sat    //    D13i= anTmp1_RIFFT1_11i-anTmp1_RIFFT1_13r
	}
	{
		VSHUFF(V27,V28,R10) // V28 = Y3i
		
	}
	{
		VSHUFF(V21,V24,R10) // V24 = Y5i
	}
	{
		VSHUFF(V15,V9,R10) // V9 =  Y2i
		V27.h = VAVG(V24.h,V28.h):rnd // anTmp1_RIFFT1_13i= Y5i+Y3i
		V21.h = VNAVG(V24.h,V28.h)    // anTmp1_RIFFT1_12i= Y5i-Y3i
	}
	
	{
		VSHUFF(V11,V12,R10) // V12 = Y6i
		V14.h = VADD(V14.h,V21.h):sat    //    D10i= anTmp1_RIFFT1_10i+anTmp1_RIFFT1_12i  anStage1
		V21.h = VSUB(V14.h,V21.h):sat    //    D12i= anTmp1_RIFFT1_10i-anTmp1_RIFFT1_12i
	}
	
	{
		V11.h = VMPY(V31.h,R11.h):<<1:rnd:sat   ////COS_PI_8 * D11i
		V16.h = VADD(V16.h,V27.h):sat    //    D13r= anTmp1_RIFFT1_11r+anTmp1_RIFFT1_13i  anStage1
		V27.h = VSUB(V16.h,V27.h):sat    //    D11r= anTmp1_RIFFT1_11r-anTmp1_RIFFT1_13i
	}
	{
		V31.h = VMPY(V31.h,R9.h):<<1:rnd:sat   ////SIN_PI_8 * D11i
		V6.h = VAVG(V9.h,V12.h):rnd   //  anTmp1_RIFFT1_21i= Y2i+Y6i
		V23.h = VNAVG(V9.h,V12.h)    // anTmp1_RIFFT1_20i= Y2i-Y6i
	}
    {
	    V15.h = VMPY(V27.h,R9.h):<<1:rnd:sat         //SIN_PI_8 * D11r
		V9 = V6      // anTmp1_RIFFT1_23i
		V12.h = VNAVG(V12.h,V9.h)    // anTmp1_RIFFT1_22i= Y6i-Y2i
	}
    {
		V27.h = VMPY(V27.h,R11.h):<<1:rnd:sat         //COS_PI_8 * D11r
		V13.h = VADD(V13.h,V9.h):sat    //    D23r= anTmp1_RIFFT1_21r+anTmp1_RIFFT1_23i  anStage1
		V9.h = VSUB(V13.h,V9.h):sat    //     D21r= anTmp1_RIFFT1_21r-anTmp1_RIFFT1_23i
    }
	{
		
		V12.h = VSUB(V23.h,V12.h):sat    // anTmp2_RIFFT1_22r  D22i= anTmp1_RIFFT1_20i+anTmp1_RIFFT1_22i  anStage1
		V18.h = VAVG(V18.h,V30.h):rnd // anTmp1_RIFFT1_33i= Y7i+Y1i
		V30.h = VNAVG(V30.h,V18.h)   // anTmp1_RIFFT1_32i= Y7i-Y1i
	}
	{
		V9.h = VMPY(V9.h,R8.h):<<1:rnd:sat        //COS_PI_4 * D21r
		V26.h = VADD(V26.h,V18.h):sat    //    D33r= anTmp1_RIFFT1_31r+anTmp1_RIFFT1_33i  anStage1
		V18.h = VSUB(V26.h,V18.h):sat    //   D31r= anTmp1_RIFFT1_31r-anTmp1_RIFFT1_33i
	}
	{
		V21.h = VMPY(V21.h,R8.h):<<1:rnd:sat    //COS_PI_4 * D12i
		V24.h = VAVG(V24.h,V28.h):rnd                  // anTmp1_RIFFT1_31i= Y3i+Y5i
		V28.h = VNAVG(V28.h,V24.h)    // anTmp1_RIFFT1_30i= Y3i-Y5i
	}

	{
		V1.h = VMPY(V18.h,R11.h):<<1:rnd:sat        //COS_PI_8 * D31r
		V24.h = VADD(V24.h,V20.h):sat    //    D31i= anTmp1_RIFFT1_31i+anTmp1_RIFFT1_33r  anStage1
		V20.h = VSUB(V24.h,V20.h):sat    //   D33i= anTmp1_RIFFT1_31i-anTmp1_RIFFT1_33r
	}

	{
		V18.h = VMPY(V18.h,R9.h):<<1:rnd:sat        //SIN_PI_8 * D31r
		V6.h = VADD(V6.h,V10.h):sat    //    D21i= anTmp1_RIFFT1_21i+anTmp1_RIFFT1_23r  anStage1
		V10.h = VSUB(V6.h,V10.h):sat    //     D23i= anTmp1_RIFFT1_21i-anTmp1_RIFFT1_23r
	}
	{
		V3.h = VMPY(V24.h,R11.h):<<1:rnd:sat        //COS_PI_8 * D31i
		V28.h = VADD(V28.h,V30.h):sat    //    D30i= anTmp1_RIFFT1_30i+anTmp1_RIFFT1_32i  anStage1
		V30.h = VSUB(V28.h,V30.h):sat    //   D32i= anTmp1_RIFFT1_30i-anTmp1_RIFFT1_32i
	}
	
	{
		V24.h = VMPY(V24.h,R9.h):<<1:rnd:sat        //SIN_PI_8 * D31i
		V19.h = VADD(V19.h,V25.h):sat    //    E20= D10r+D30r  anStage2
		V14.h = VSUB(V14.h,V28.h):sat    //   E30= D10i-D30i	
	}
	
	
	{
		V6.h = VMPY(V6.h,R8.h):<<1:rnd:sat        //COS_PI_4 * D21i
		V0.h = VADD(V0.h,V19.h):sat    //  Z0 = E00+E20  
		V19.h = VSUB(V0.h,V19.h):sat    //  Z8 =E00-E20		
	}
	
	{
		V30.h = VMPY(V30.h,R8.h):<<1:rnd:sat        //COS_PI_4 * D32i
		V27.h = VADD(V15.h,V11.h)      //  anTmp2_RIFFT1_11i = SIN_PI_8 * D11r + COS_PI_8 * D11i
		V31.h = VSUB(V27.h,V31.h)     //  anTmp2_RIFFT1_11r = COS_PI_8 * D11r - SIN_PI_8 * D11i
	}

	{
		V13.h = VMPY(V13.h,R8.h):<<1:rnd:sat        //COS_PI_4 * D23r 
		V18.h = VADD(V1.h,V24.h)       //  anTmp2_RIFFT1_31i = COS_PI_8 * D31r + SIN_PI_8 * D31i
		V24.h = VSUB(V18.h,V3.h)     //  anTmp2_RIFFT1_31r = SIN_PI_8 * D31r - COS_PI_8 * D31i
	}  	
	{	
		V10.h = VMPY(V10.h,R8.h):<<1:rnd:sat        //COS_PI_4 * D23i
		V24.h = VADD(V31.h,V24.h):sat    //   E21= anTmp2_RIFFT1_11r+anTmp2_RIFFT1_31r  anStage2
		V18.h = VSUB(V27.h,V18.h):sat    //   E31= anTmp2_RIFFT1_11i-anTmp2_RIFFT1_31i
	}
	{
		V31.h = VMPY(V16.h,R11.h):<<1:rnd:sat   ////COS_PI_8 * D13r
		V9.h = VSUB(V9.h,V6.h)    //  anTmp2_RIFFT1_21r = COS_PI_4 * D21r - COS_PI_4 * D21i 
	    V13.h = VADD(V13.h,V10.h)   // anTmp2_RIFFT1_23r = COS_PI_4 * D23r + COS_PI_4 * D23i		
	}
	{	
		V16.h = VMPY(V16.h,R9.h):<<1:rnd:sat   ////SIN_PI_8 * D13r
		V2.h = VADD(V2.h,V9.h):sat    //   E01= anTmp2_RIFFT1_01r+anTmp2_RIFFT1_21r  anStage2
		V9.h = VSUB(V2.h,V9.h):sat    //   E11= anTmp2_RIFFT1_01r-anTmp2_RIFFT1_21r
	}
	
	{
		V27.h = VMPY(V17.h,R9.h):<<1:rnd:sat //SIN_PI_8 * D13i
		V2.h = VADD(V2.h,V24.h):sat    //  Z1 = E01+E21  
		V24.h = VSUB(V2.h,V24.h):sat    //  Z9 =E01-E21	
	}

	{
		V17.h = VMPY(V17.h,R11.h):<<1:rnd:sat //COS_PI_8 * D13i
		V9.h = VADD(V9.h,V18.h):sat    //  Z13 = E11+E31  
		V18.h = VSUB(V9.h,V18.h):sat    // Z5  = E11-E31	
	}
	{
		V6.h = VMPY(V26.h,R9.h):<<1:rnd:sat //SIN_PI_8 * D33r
		V22.h = VADD(V22.h,V14.h):sat    //  Z12 = E10+E30  
		V14.h = VSUB(V22.h,V14.h):sat    //  Z4 =E10-E30
	}
    {
		V26.h = VMPY(V26.h,R11.h):<<1:rnd:sat //COS_PI_8 * D33r
		V4.h = VADD(V4.h,V21.h)       //anTmp2_RIFFT1_12i =  COS_PI_4 * D12r + COS_PI_4 * D12i
		V21.h = VSUB(V4.h,V21.h)     //  anTmp2_RIFFT1_12r = COS_PI_4 * D12r - COS_PI_4 * D12i
	}
	{
		V10.h = VMPY(V20.h,R11.h):<<1:rnd:sat //COS_PI_8 * D33i
		V29.h = VADD(V29.h,V30.h)       //anTmp2_RIFFT1_32r =  COS_PI_4 * D32r + COS_PI_4 * D32i
		V30.h = VSUB(V29.h,V30.h)     //  anTmp2_RIFFT1_32i = COS_PI_4 * D32r - COS_PI_4 * D32i
	}
	{	
		V20.h = VMPY(V20.h,R9.h):<<1:rnd:sat //SIN_PI_8 * D33i
		V7.h = VADD(V7.h,V12.h):sat    //   E12= anTmp2_RIFFT1_02r+anTmp2_RIFFT1_22r  anStage2
		V12.h = VSUB(V7.h,V12.h):sat    //   E02= anTmp2_RIFFT1_02r-anTmp2_RIFFT1_22r
	}
	
	{	
		VSHUFF(V2,V0,R4)
		V21.h = VSUB(V21.h,V29.h):sat    //   E22= anTmp2_RIFFT1_12r-anTmp2_RIFFT1_32r  anStage2
		V4.h = VSUB(V4.h,V30.h):sat    //   E32= anTmp2_RIFFT1_12i-anTmp2_RIFFT1_32i
	}
	{
		VSHUFF(V24,V19,R4)
		V12.h = VADD(V12.h,V21.h):sat    //  Z2 = E02+E22  
		V21.h = VSUB(V12.h,V21.h):sat    // Z10 =E02-E22	
	}

	{
		VSHUFF(V18,V14,R4)
		V27.h = VADD(V31.h,V27.h)      //  anTmp2_RIFFT1_13i = COS_PI_8 * D13r + SIN_PI_8 * D13i
		V17.h = VSUB(V16.h,V17.h)     //  anTmp2_RIFFT1_13r = SIN_PI_8 * D13r -  COS_PI_8 * D13i
	}
	{
		VSHUFF(V9,V22,R4)
		V6.h = VADD(V6.h,V10.h)     //  anTmp2_RIFFT1_33i = SIN_PI_8 * D33r + COS_PI_8 * D33i
		V20.h = VSUB(V20.h,V26.h)     //  anTmp2_RIFFT1_33r = SIN_PI_8 * D33i - COS_PI_8 * D33r
	}
	{
		V5.h = VADD(V5.h,V13.h):sat    //   E13= anTmp2_RIFFT1_03r+anTmp2_RIFFT1_23r  anStage2
		V13.h = VSUB(V5.h,V13.h):sat    //   E03= anTmp2_RIFFT1_03r-anTmp2_RIFFT1_23r	
		V17.h = VADD(V17.h,V20.h):sat    //   E23= anTmp2_RIFFT1_13r-anTmp2_RIFFT1_33r  anStage2
		V6.h = VADD(V27.h,V6.h):sat     // E33= anTmp2_RIFFT1_13i-anTmp2_RIFFT1_33i
	}
	{	
		
		V13.h = VADD(V13.h,V17.h):sat    //  Z3 = E03+E23  
		V20.h = VSUB(V13.h,V17.h):sat    // Z11 =E03-E23	
	}

	{
		V7.h = VADD(V5.h,V6.h):sat    //  Z15 = E13+E33  
		V5.h = VSUB(V5.h,V6.h):sat    //  Z7  = E13-E33	
		V6.h = VADD(V7.h,V4.h):sat    //  Z14 = E12+E32  
		V4.h = VSUB(V7.h,V4.h):sat    //  Z6  = E12-E32	
	}
		
	{
		VSHUFF(V20,V21,R4)
		V13:12.h = VSHUFFOE(V13.h,V12.h) 
		
	}
	{
		V7:6.h = VSHUFFOE(V7.h,V6.h)
		V5:4.h = VSHUFFOE(V5.h,V4.h)
	}
	{
		VSHUFF(V12,V0,R5)
	}
	{
		VSHUFF(V13,V2,R5)
		
	}
	{
		VSHUFF(V4,V14,R5)
		
	}
	{
		VSHUFF(V5,V18,R5)
		
	}
	{
		VSHUFF(V21,V19,R5)
		
	}
	{
		VSHUFF(V20,V24,R5)
	}
	{
		VSHUFF(V6,V22,R5)
	}
	{
		VSHUFF(V7,V9,R5)
	}
	{
		VSHUFF(V14,V0,R6)
	}
	{
		VSHUFF(V18,V2,R6)
	}
	{
		VSHUFF(V4,V12,R6)
	}
	{
		VSHUFF(V5,V13,R6)
	}
	{
		VSHUFF(V22,V19,R6)
	}
	{
		VSHUFF(V9,V24,R6)
	}
	{
		VSHUFF(V6,V21,R6)
	}
	{
		VSHUFF(V7,V20,R6)
		
	}
	{
		VSHUFF(V19,V0,R10)  // V19 | V0 OUT8 | OUT0
	}
	{
		VSHUFF(V24,V2,R10) // V24 | V2 OUT9 | OUT1
	}
	{
		VSHUFF(V21,V12,R10)// V21 | V12 OUT10 | OUT2
	}
	{
		VSHUFF(V20,V13,R10)// V20 | V13 OUT11 | OUT3
	}
	{
		VSHUFF(V22,V14,R10)// V22 | V14 OUT12 | OUT4
	}
	{
		VSHUFF(V9,V18,R10)// V9 | V18 OUT13 | OUT5
		
	}
	{
		VSHUFF(V6,V4,R10)// V6 | V4 OUT14 | OUT6
		
	}
	{
		VSHUFF(V7,V5,R10)// V7 | V5 OUT15 | OUT7
	}
	{
		V0.h=VSHUFF(V0.h) 
		VMEM(R0++#1) = V0.new
	}
	{
		V2.h=VSHUFF(V2.h) //
		VMEM(R0++#1)  = V2.new
	}
	{
		V12.h=VSHUFF(V12.h) //
		VMEM(R0++#1)  = V12.new
	}
	{
		V13.h=VSHUFF(V13.h) //
		VMEM(R0++#1)  = V13.new
	}
	{
		V14.h=VSHUFF(V14.h) //
		VMEM(R0++#1) = V14.new
	}
	{
		V18.h=VSHUFF(V18.h) //
		VMEM(R0++#1) = V18.new
	}
	{
		V4.h=VSHUFF(V4.h) //
		VMEM(R0++#1) = V4.new
	}

	{
		V5.h=VSHUFF(V5.h) //
		VMEM(R0++#1) = V5.new
	}
	{
		V19.h=VSHUFF(V19.h) //
		VMEM(R0++#1) = V19.new
	}
	{
		V24.h=VSHUFF(V24.h) //
		VMEM(R0++#1) = V24.new
	}
	{
		V21.h=VSHUFF(V21.h) //
		VMEM(R0++#1) = V21.new
	}
	{
		V20.h=VSHUFF(V20.h) //
		VMEM(R0++#1) = V20.new
	}
	{
		V22.h=VSHUFF(V22.h) //
		VMEM(R0++#1) = V22.new
	}
	{
		V9.h=VSHUFF(V9.h) //
		VMEM(R0++#1) = V9.new
	}
	{
		V6.h=VSHUFF(V6.h) //
		VMEM(R0++#1) = V6.new
	}
	{
		V7.h=VSHUFF(V7.h) //
		VMEM(R0++#1) = V7.new
	}
 	{
       dealloc_return                               //Q
    }

.size RIFFT16x16_4_asm, .-RIFFT16x16_4_asm

#endif
