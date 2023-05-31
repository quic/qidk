Step 1 : Generate Squad2 dataset :
=========================================================================
# Py deps :
pip install transformers
pip install datasets

$ python generate_representative_dataset_squadv2.py <model_name> <num_samples> <seq_len>
$ python generate_representative_dataset_squadv2.py deepset/bert-base-cased-squad2 1000 384

--model_name can be :
DB-Bert-base = deepset/bert-base-cased-squad2
DistillBert = distilbert-base-cased-distilled-squad
DB-Albert-base = twmkn9/albert-base-v2-squad2
DB-Electra-small = mrm8488/electra-small-finetuned-squadv2

Output will be:
-----------------------------
input_ids : input_ids/input_ids__00*.raw
attn_mask : attention_mask/attention_mask__00*.raw
tty_ids   : token_type_ids/token_type_ids__00*.raw

logits_from_golden_model  : Output Logits from Golden Pytorch model
golden_answers            : Contains textual answer & start-index of answer
golden_questions_n_context: Contains paragraph & question in textual format


Step 2 : Generate raw_input_list.txt for inference :
=========================================================================
$ python gen_raw_list.py <no. of inputs for inference>


Step 3 : Generate logits from TF-Golden model :
=========================================================================

python batch_tf_inf.py ./QA_models_TF/QA_DB_BERT_base_default_one/db-bert-base-cased-squad2.pb small_raw_list.txt input_ids:0,attention_mask:0,token_type_ids:0 Identity:0,Identity_1:0

Output will be:
-----------------------------
tf_out/infer_*___Identity.raw
tf_out/infer_*___Identity_1.raw

Above output can be used to Compare with QNN output logits with SQNR, PSNR, Cosine-Sim


Step 4 : Convert logits to Textual Answer & Cal. F1, EM scores :
=========================================================================
python logits_to_F1_score.py --model_name deepset/bert-base-cased-squad2 --sequence_len 512 --logits_dir tf_out --golden_answers_dir golden_answers --top_k 1


Extras : Model Comparison :
=========================================================================
python qa_model_comparator.py deepset/bert-base-cased-squad2 400

python qa_model_comparator.py twmkn9/albert-base-v2-squad2 400

python qa_model_comparator.py mrm8488/electra-small-finetuned-squadv2 400