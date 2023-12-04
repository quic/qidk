//==============================================================================
// Auto Generated Code for qhci
//==============================================================================

#include "HTP/core/constraints.h"
#include "HTP/core/op_package_feature_support.h"
#include "HTP/core/op_register_ext.h"
#include "HTP/core/optimize.h"
#include "QnnOpPackage.h"
#include "HTP/core/simple_reg.h"


BEGIN_PKG_OP_DEFINITION(PKG_InputPack);



DEF_OPTIM(CLEANUP_GRAPH+130, relaxed_precision_flag,
		Op("InputPack", "x1","mask"),
          AND(IS_FLOAT32("x1"), IS_FLOAT32("*")),
          MAKE_OP_FP16_AND_INSERT_CAST(
			Op("Inputpack", CAST_TO_FP16("x1"),CAST_TO_FP16("mask"))
		  )
)

DEF_OPTIM(CLEANUP_GRAPH+130, relaxed_precision_flag,
		Op("InputPack", "x1","mask"),
        AND(IS_FLOAT16("x1"), IS_FLOAT16("*")),
		Op("Inputpack", "x1","mask")
		  
)

DEF_PACKAGE_OPTIMIZATION(TCM_MIGRATION + 110,
                         Op("q::crouton_to_vtcm",Op("Inputpack","x1","mask")),
                         OK,
						 Op("Inputpack.tcm.crouton",  
								WITH_SAME_OUTPUT("x1",Op("q::crouton_to_vtcm","x1")),
								"mask"
							)
)

//
DEF_PACKAGE_OPTIMIZATION(TCM_MIGRATION + 110,
                         Op("Inputpack",Op("q::crouton_from_vtcm","x1"),"mask"),
                         OK,
						 Op("q::crouton_from_vtcm",
							Op("Inputpack.tcm.crouton","x1","mask")
							)
						
)

DEF_PACKAGE_OPTIMIZATION(TCM_MIGRATION + 130,
                         Op("q::flat_to_vtcm",Op("Inputpack","x1","mask")),
                         OK,
						 Op("Inputpack.tcm.flat",  
								WITH_SAME_OUTPUT("x1",Op("q::flat_to_vtcm","x1")),
								"mask"
						)
)

DEF_PACKAGE_OPTIMIZATION(TCM_MIGRATION + 130,
                         Op("Inputpack",Op("q::flat_from_vtcm","x1"),Op("q::flat_from_vtcm","mask")),
                         OK,
						 Op("q::flat_from_vtcm",
							Op("Inputpack.tcm.flat","x1","mask"))
						
)

#define VALID_VALUE   0x3f800000
#define INVALID_VALUE 0

/* execute functions for ops */

template<typename T,  typename src_T = uint8_t> 
void hvx_debug(T data,const char* oem_str = nullptr){
#define DEBUG
#ifdef DEBUG

	using value_t = src_T;
	constexpr int value_size = sizeof(src_T);
	
	char* tmp =  (char*)&data;
	
	printf("%s \n",oem_str);
	for(size_t i = 0; i < 128;i+= value_size){
		auto value = *(value_t*)(tmp + i);
		if constexpr(std::is_floating_point<value_t>::value){
			printf("%f ",value);
		}else{
			printf("%d ",value);
		}
	}
	printf("\n");
	printf("\n");
#endif
}

template<typename TensorType,typename MaskType >
void check_results(TensorType& out_0,const TensorType &data,MaskType& mask_in){
	int valid = 0;
	auto [b,h,w,valid_number] = mask_in.dims();
	auto mask = mask_in.get_raw_addr(0,0,0,0);
	for(int i =0;i < valid_number;i++){
		if(mask[i] != INVALID_VALUE){
			for(int c = 0; c < 128;c++){
				if(out_0(0,0,valid,c) != data(0,0,i,c)){
					printf("error at %d %d valid %d\n",i,c,valid);
					assert(0);
				}
			}
			valid++;
		}
	}
}

	
template<typename TensorType,typename MaskType>
GraphStatus inputpackImpl(TensorType& out_0,
                          const TensorType &data,
						  const MaskType& mask_in)

{
		   
	auto [b_out,h_out,w_out,d_out] = out_0.dims();
	//printf("%d %d %d %d\n",b_out,h_out,w_out,d_out);
	
	
	int valid = 0;
	
	int leftover = d_out%64;
	int full_vector = d_out/64;
	
	auto mask = mask_in.get_raw_addr(0,0,0,0);

	//h is always 1,
	for(int j = 0; j < w_out;j++){
		if(mask[j] != INVALID_VALUE){
			int m = 0;
			for(; m < full_vector;m++){
				int c = m << 6;
				HVX_Vector* data_addr = (HVX_Vector*)data.get_raw_addr(0,0,j, c );
				HVX_Vector* dst_addr  = (HVX_Vector*)out_0.get_raw_addr(0,0,valid,c);
				*dst_addr = *data_addr;
			
			}
			//leftover
			if(leftover > 0){
				int c = m << 6;	
				q6op_vstu_variable_ARV(out_0.get_raw_addr(0,0,valid,c), leftover * 2, *(HVX_Vector*)out_0.get_raw_addr(0,0,j,c));
			}
			valid++;
		}
		//do nothing is not valid
	}
	//check_results(out_0,data,mask_in);
	return GraphStatus::Success;
}


template<typename TensorType,typename MaskType>
GraphStatus inputpackImpl_Crouton(TensorType& out_0,
                          const TensorType &data,
						  const MaskType& mask_in)

{
		   
	//printf("data %s \n",typeid(out_0).name());
	//printf("data %s \n",typeid(data).name());
	//printf("mask_in %s \n",typeid(mask_in).name());
	
	
	auto [b_out,h_out,w_out,d_out] = out_0.dims();
	
	auto mask = mask_in.get_raw_addr(0,0,0,0);
	//dcfetch_block(mask,w_out * 2);
	l2pref(mask,1,w_out,1);
	
	union {
		float f;
		int32_t i;
	}first, second;
	
	if constexpr (std::is_base_of<LayoutCrouton_16, TensorType>::value) {
		/*
        uint32_t start = uint32_t(data.get_raw_addr(0, 0, 0, 0) - data.block_ptr(0, 0, 0, 0));
        uint8_t **inblocktab = (uint8_t **)data.blocktab_ptr();
        uint8_t **outblocktab = (uint8_t **)out_0.blocktab_ptr();

        const auto &[i_offset_t_bat, i_offset_t_row, i_offset_t_col, i_offset_t_depth] = data.tile_strides();
        const auto &[o_offset_t_bat, o_offset_t_row, o_offset_t_col, o_offset_t_depth] = out_0.tile_strides();
		printf("%d %d %d %d\n",i_offset_t_bat, i_offset_t_row, i_offset_t_col, i_offset_t_depth);
		*/
		for(int c = 0 ; c < d_out; c+=32){
				int w_wrtite_index = 0;
				for(int w = 0; w < w_out;w+= 2){
					first.f = mask[w];
					second.f = mask[w+1];
					if(first.i == INVALID_VALUE && second.i == INVALID_VALUE){
						continue;
					}
					HVX_Vector* in = (HVX_Vector*)data.get_raw_addr(0,0,w,c);
					//even index
					//if(w_wrtite_index % 2 == 0){
					if((w_wrtite_index & 1) == 0){
						HVX_Vector* out = (HVX_Vector*)out_0.get_raw_addr(0,0,w_wrtite_index,c);
						if(first.i == VALID_VALUE && second.i == VALID_VALUE){
							*out = *in;
							w_wrtite_index += 2;
						}
						else{
							//(0,1) or (1,0)
							if(first.i == VALID_VALUE){
								//(1,0)
								*out = *in;
							}else{
								//(0,1)
								HVX_Vector odd_w = Q6_V_vror_VR(*in,2);
								*out = odd_w;
							}
							
							w_wrtite_index += 1;
						}
					}
					//odd case
					else{
						HVX_Vector* out= (HVX_Vector*)out_0.get_raw_addr(0,0,w_wrtite_index-1,c);
						if(first.i == VALID_VALUE && second.i == VALID_VALUE){
								//write w
								HVX_VectorPair output = Q6_W_vshuff_VVR(*in,*out,2); //out in
								
								*out = Q6_V_lo_W(output);
								
								//write second w
								out= (HVX_Vector*)out_0.get_raw_addr(0,0,w_wrtite_index+1,c);
								*out = Q6_V_vror_VR(Q6_V_hi_W(output),2);
								
								w_wrtite_index += 2;
							
						}else{
							//(0,1) or (1,0)
							if(first.i == VALID_VALUE){
								//(1,0)
								HVX_VectorPair output = Q6_W_vshuff_VVR(*in,*out,2);
								*out = Q6_V_lo_W(output);
							}else{
								//(0,1)
								HVX_Vector odd_w = Q6_V_vror_VR(*in,2);
								HVX_VectorPair output = Q6_W_vshuff_VVR(odd_w,*out,2);
								
								*out = Q6_V_lo_W(output);
							}
							w_wrtite_index += 1;
						}
					}
				}
		}
		//check_results(out_0,data,mask_in);
		return GraphStatus::Success;
	}
	printf("using reference code");
	int valid = 0;
	for(int j = 0 ; j < w_out;j++){
		if(mask[j] != 0){
			for(int m = 0; m < d_out;m++){
				out_0(0,0,valid,m) = data(0,0,j,m);
			}
		}
		valid++;
	}
	return GraphStatus::Success;
}

__attribute__((unused)) static float inputpackCostFunc(const Op *op)
{
/*
* add code here
* */

float cost = 0.0;  // add cost computation here
return cost;
}





DEF_PACKAGE_OP_AND_COST_AND_FLAGS((inputpackImpl<PlainFloat16Tensor_TCM,PlainFloat16Tensor_TCM>), "Inputpack.tcm.flat",SNAIL,Flags::RESOURCE_HVX)

DEF_PACKAGE_OP_AND_COST_AND_FLAGS((inputpackImpl_Crouton<F16CroutonTensor_TCM,PlainFloat16Tensor_TCM>), "Inputpack.tcm.crouton",FAST,Flags::RESOURCE_HVX)
DEF_PACKAGE_OP_AND_COST_AND_FLAGS((inputpackImpl_Crouton<F16CroutonTensor_TCM,PlainFloat16Tensor>), "Inputpack.tcm.crouton",FAST,Flags::RESOURCE_HVX)

END_PKG_OP_DEFINITION(PKG_InputPack);