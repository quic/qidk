from transformers import TFAutoModel, AutoTokenizer
from pysnpe_utils import pysnpe
from pysnpe_utils.pysnpe_enums import *

SEQ_LEN = 384
model_name = 'sentence-transformers/bert-large-nli-stsb-mean-tokens'
model = TFAutoModel.from_pretrained(model_name)

input_map = [ InputMap('input_ids', (1,SEQ_LEN), tf.int32), InputMap('attention_mask', (1,SEQ_LEN), tf.int32) ]

# Generate SNPE DLC
snpe_context = pysnpe.export_to_tf_keras_model(model, input_map, 
                                                "bert_large_semantic_search",
                                                frozen_graph_path='bert_large_semantic_search.pb'
                                               ).to_dlc()\
                                                   .gen_dsp_graph_cache(DlcType.FLOAT)\
                                                       .visualize_dlc()

tokenizer = AutoTokenizer.from_pretrained(model_name)

def preprocessor(message):
    encoded = tokenizer.encode_plus(
        message,
        return_tensors="np",
        padding='max_length',
        return_length=True,
        max_length=SEQ_LEN,
    )
    return encoded.input_ids, encoded.attention_mask

def gen_msg_dataset_embeddings(messages): 
    # Tokenize and encode all messages
    message_embeddings = []
    for message in messages:
        input_ids, attention_mask = preprocessor(message)
        message_last_hidden_state = model(input_ids, attention_mask=attention_mask).last_hidden_state[:, 0]
#         message_last_hidden_state = post_process_mean_pooling(message_last_hidden_state, attention_mask)
        message_embeddings.append(message_last_hidden_state)

    message_embeddings = tf.concat(message_embeddings, axis=0)
    return message_embeddings

def semantic_search(query, message_embeddings, messages, top_n=3):
    input_ids, attention_mask = preprocessor(query)
    query_last_hidden_state = model(input_ids, attention_mask=attention_mask).last_hidden_state[:, 0]
#     query_last_hidden_state = post_process_mean_pooling(query_last_hidden_state, attention_mask)
    
    similarities = tf.keras.losses.cosine_similarity(query_last_hidden_state, message_embeddings, axis=1)

    # Sort messages by similarity and return top n results
    indices = tf.argsort(similarities, direction='DESCENDING').numpy()
    top_messages = [messages[i] for i in indices[:top_n]]

    print(f"Top {top_n} search result : \n")
    for msg in top_messages:
        print("==> ", msg)
    return top_messages

chat_msgs = """
1. Person A: Hey guys, I was thinking we should do something fun as a team. Anyone up for a team outing?
2. Person B: Yeah, that sounds like a great idea. What did you have in mind?
3. Person C: How about a picnic in the park? We could bring some food and games.
4. Person A: I like that idea. We could also do a potluck-style feast and everyone brings a dish.
5. Person D: That sounds good to me. What kind of food should we bring?
6. Person B: How about we each bring a dish from our cultural background? That way we can all try something new.
7. Person C: I love that idea! I can bring some samosas and chutney.
8. Person A: I can bring some Mexican-style grilled chicken and rice.
9. Person D: I can bring some Filipino-style adobo.
10. Person B: I'll bring some Chinese-style dumplings and dipping sauce.
11. Person A: Sounds like we've got a great feast planned! What about activities?
12. Person C: We could bring some lawn games like frisbee or bocce ball.
13. Person D: And we could also do a team scavenger hunt in the park.
14. Person B: That sounds like a lot of fun! When should we plan this for?
15. Person A: How about next Saturday, around noon?
16. Person C: That works for me. Should we make a list of all the food and supplies we need to bring?
17. Person D: Good idea. I can make a Google sheet for us to fill in.
18. Person B: And I'll bring some extra blankets and chairs in case we need them.
19. Person A: Great, let's get this team outing and feast planned! Can't wait to try everyone's food and play some games.
"""
messages = chat_msgs.strip().split("\n")

message_embeddings = gen_msg_dataset_embeddings(messages)

query = "What are the suggestions for the team outing ?"

top_msgs = semantic_search(query, message_embeddings, messages, top_n=5)