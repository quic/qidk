QUANT_DIR=qnn_libs

compile_udo(){


	cd htpoppackage/qhci/
	rm -rf build/x86_64-linux-clang
	rm -rf build/
	make htp_x86
	
	make htp_v73
	cd ../../
}

compile_model(){

	
	qnn-onnx-converter \
		--input_network model.onnx \
		--op_package_config input_pack.xml \
		--output_path ${QUANT_DIR}/model_quant.cpp \
		--float_bw 16
	
	qnn-model-lib-generator \
		-c ${QUANT_DIR}/model_quant.cpp \
		-o ${QUANT_DIR}/libs \
		-l model_quant \
		-t x86_64-linux-clang 
		
	qnn-net-run \
		--backend libQnnHtp.so \
		--model ${QUANT_DIR}/libs/x86_64-linux-clang/libmodel_quant.so \
		--input_list input_list.txt \
		--op_packages htpoppackage/qhci/build/x86_64-linux-clang/libQnnqhci.so:qhciInterfaceProvider \
		--config_file be_htp.json
	#	#--log_level error
	#	
	qnn-context-binary-generator \
		--backend libQnnHtp.so \
		--model ${QUANT_DIR}/libs/x86_64-linux-clang/libmodel_quant.so \
		--op_packages htpoppackage/qhci/build/x86_64-linux-clang/libQnnqhci.so:qhciInterfaceProvider \
		--binary_file model.bin \
		--output_dir output \
		--config_file be_htp.json
}


compile_udo
compile_model
