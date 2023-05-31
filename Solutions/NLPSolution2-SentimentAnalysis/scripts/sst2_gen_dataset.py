from datasets import load_dataset
from transformers import AutoTokenizer, TFAutoModelForSequenceClassification
import numpy as np
import os
SEQ_LEN = 128
MODEL_NAME = "textattack/bert-base-uncased-SST-2"
tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME, model_max_length=SEQ_LEN)

os.system("rm -r input_ids")
os.system("rm -r attention_mask")
os.system("rm -r dataset_logits_argmax_score")
os.system('mkdir -p input_ids')
os.system('mkdir -p attention_mask')
os.system('mkdir -p dataset_logits_argmax_score')

# dataset = load_dataset('glue', 'sst2', split="test")
dataset = load_dataset('glue', 'sst2', split="validation")

score = np.zeros((len(dataset)), np.float32) # init

for i in range(len(dataset)):
    items = dataset[i]
    input_encodings = tokenizer(
            items["sentence"],
            # return_tensors=TensorType.TENSORFLOW,
            return_tensors="np",
            padding='max_length',
            truncation="longest_first",
            return_length=True,
            max_length=SEQ_LEN,
    )
    inp_ids = input_encodings.input_ids
    print(inp_ids.shape)
    with open("input_ids/inp_ids_"+str(i)+".raw", 'w') as f:
        inp_ids.astype(np.float32).tofile(f)
    
    mask = input_encodings.attention_mask
    print(mask.shape)
    with open("attention_mask/attn_mask_"+str(i)+".raw", 'w') as f:
        mask.astype(np.float32).tofile(f)

    score[i] = items["label"]
    print(score[i])

with open("dataset_logits_argmax_score/dataset_logits_predicted_labels.raw", 'w') as f:
    # score = np.asarray(score, dtype=np.float32)
    score.tofile(f)
    


