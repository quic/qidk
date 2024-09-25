//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

//==============================================================================
// Auto Generated Code for RandomNormalLike
//==============================================================================

#include "HTP/core/constraints.h"
#include "HTP/core/op_package_feature_support.h"
#include "HTP/core/op_register_ext.h"
#include "HTP/core/optimize.h"
#include "QnnOpPackage.h"
#include "HTP/core/simple_reg.h"
#include "math.h"


BEGIN_PKG_OP_DEFINITION(PKG_RandomNormalLike);


// op execute function declarations
template<typename TensorType>
GraphStatus randomnormallikeImpl(TensorType& Out,
                                 const TensorType& In);

// forward declaration of sample cost function
static float randomnormallikeCostFunc(const Op *op);

/*
* method 1 for defining op, using default cost value (i.e. GLACIAL) and no flag
* syntax: DEF_PACKAGE_OP(F,OP)
* e.g. DEF_PACKAGE_OP((randomnormallikeImpl<Tensor>), "RandomNormalLike")
*/
DEF_PACKAGE_OP((randomnormallikeImpl<Tensor>), "RandomNormalLike")

/*
* method 2 for defining op with specified cost value (one of GLACIAL, SNAIL, FAST, FREE)
* and provided flags
* syntax: DEF_PACKAGE_OP_AND_COST_AND_FLAGS(F,OP,COST,...)
* can use zero (default) or more flags, FLAG options are IS_CONST, INHIBIT_CONST_PROP,
* RESOURCE_HVX, RESOURCE_HMX(not supported in external op packages)
* e.g. DEF_PACKAGE_OP_AND_COST_AND_FLAGS((randomnormallikeImpl<PlainFloatTensor>), "RandomNormalLike", SNAIL)
*/

/*
* method 3 for defining op with cost function pointer and provided flags
* cost function pointer type: typedef float (*cost_function) (const Op * op);
* syntax: DEF_PACKAGE_OP_AND_COST_F_AND_FLAGS(F,OP,COST_F,...)
* e.g. DEF_PACKAGE_OP_AND_COST_F_AND_FLAGS((randomnormallikeImpl<PlainFloatTensor>),
* "RandomNormalLike", randomnormallikeCostFunc, Flags::RESOURCE_HVX)
*/

/*
* optimization definitions
* need to be global in the package
* one definition per optimization
* syntax: DEF_PACKAGE_OPTIMIZATION(PRIORITY,MATCHCODE,CONSTRAINTCODE,REPLACECODE)
* PRIORITY predefined values include EARLY(2000), MIDDLE(3000), LATE(4000)
* HTP core provides some replacement functions for op package to use
* for more information about optimization rules, please refer to HTP core documentations
*/

/*
* op parameter order definitions
* need to be global in the package
* one definition per op, and this is optional
* syntax: DEF_PACKAGE_PARAM_ORDER(OP,PARAM1,MANDATORY1,DEFAULT1,PARAM2,MANDATORY2,DEFAULT2...)
* one or more parameters can be specified for each op
    * order of parameters listed determines the order of parameters passed into op execution functions
* if an op does not have a parameter order definition, parameter order passed into Qnn_addNode
*   will be passed into op execution functions
* if an op has a parameter order definition, any parameter passed into Qnn_addNode with unlisted
    *   name will be abandoned
* if two or more op packages with the same package name will be registered, they cannot list
*   conflicting parameter orders
* PARAM refers to parameter name as a string literal
* MANDATORY refers to whether this parameter is required to be provided at Qnn_addNode
* DEFAULT is used when MANDATORY is false
*     if provided as Qnn_Param_t*,
*       DEFAULT will be used for graph construction when this parameter is not provided at
*       Qnn_addNode
*     if provided as nullptr,
*       graph construction will skip this parameter when this parameter is not provided at
*       Qnn_addNode
*/


/* execute functions for ops */

float randn(float mu, float sigma) {
  float U1, U2, W, mult;
  static float X1, X2;
  static int call = 0;
  if (call == 1) {
    call = !call;
    return (mu + sigma * (float) X2);
  }

  do {
    U1 = -1 + ((float) rand () / (float)RAND_MAX) * 2;
    U2 = -1 + ((float) rand () / (float)RAND_MAX) * 2;
    W = pow (U1, 2) + pow (U2, 2);
  }
  while (W >= 1 || W == 0); 

  mult = sqrt ((-2 * log (W)) / W);
  X1 = U1 * mult;
  X2 = U2 * mult;

  call = !call;

  return (mu + sigma * (float) X1);
}

const float mean = 0;
const float scale = 1;
template<typename TensorType>
GraphStatus randomnormallikeImpl(TensorType& Out,
                                 const TensorType& In)

{
/*
* add code here
           * */
    auto [b_in, h_in, w_in, d_in] = In.dims();
    for(size_t b_it = 0 ; b_it < b_in; b_it++)
    for(size_t h_it = 0 ; h_it < h_in; h_it++)
    for(size_t w_it = 0 ; w_it < w_in; w_it++ )
        for(size_t d_it = 0 ; d_it < d_in; d_it++ ) {
        Out(b_it,h_it,w_it, d_it) = randn(mean, scale);
    }
    return GraphStatus::Success;
}

__attribute__((unused)) static float randomnormallikeCostFunc(const Op *op)
{
/*
* add code here
* */

float cost = 0.0;  // add cost computation here
return cost;
}





/* At the bottom of the op file, call END_PKG_OP_DEFINITION(<name>),
   where <name> is as BEGIN_PKG_OP_DEFINITION
*/
END_PKG_OP_DEFINITION(PKG_RandomNormalLike);