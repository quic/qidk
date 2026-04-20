//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.ai_assistant_v1;


import android.annotation.SuppressLint;
import android.content.Context;
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.URLSpan;
import android.text.util.Linkify;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.List;

public class MessageListAdapter extends RecyclerView.Adapter<MessageListAdapter.ViewHolder> {
    private Context context;
    private Handler handler;
    private final List<Message> messages;

    public MessageListAdapter() {
        messages = new ArrayList<>();
    }

    public void setContext(Context context) {
        this.context = context;
    }



    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        Message message = messages.get(viewType);
        View view = null;
        if (message.getTextType() == MessageType.QUERY) {
            view = LayoutInflater.from(context).inflate(R.layout.layout_query_message, parent, false);
        } else if (message.getTextType() == MessageType.RESPONSE) {

            view = LayoutInflater.from(context).inflate(R.layout.layout_assistant_message, parent, false);

        } else if (message.getTextType() == MessageType.START_CHAT) {
            view = LayoutInflater.from(context).inflate(R.layout.layout_start_chat_message, parent, false);
        } else if (message.getTextType() == MessageType.END_CHAT) {
            view = LayoutInflater.from(context).inflate(R.layout.layout_end_chat_message, parent, false);
        }

        return new ViewHolder(view, message);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        Message message = messages.get(position);
        if (message.getTextType() == MessageType.RESPONSE &&
                holder.messageProgressBar.getVisibility() == View.VISIBLE &&
                !message.getText().isEmpty()) {
            holder.messageProgressBar.setLayoutParams(new LinearLayout.LayoutParams(0, 0));
            holder.messageProgressBar.setVisibility(View.INVISIBLE);
        }

        // We only set message text if Message Type is a QUERY or RESPONSE:
        else if (message.getTextType() == MessageType.QUERY || message.getTextType() == MessageType.RESPONSE) {
            // Set message:

            //TODO Here I need to remove the old text and then add the current text
            holder.messageText.setText(message.getText());
        }
    }

    @Override
    public int getItemCount() {
        return messages.size();
    }

    @Override
    public int getItemViewType(int position) {
        return position;
    }

    public void addMessage(Message message) {

        messages.add(message);
        notifyItemChanged(messages.size() - 1);
    }

    @SuppressLint("NotifyDataSetChanged")
    public void appendToLatestMessage(String newWord) {
        //TODO Here I need to check how can i add new message and delete the old message
        messages.get(messages.size() - 1).appendText(newWord);
        notifyDataSetChanged();
    }

    public Message getLatestMessage() {
        return messages.get(messages.size() - 1);
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        protected TextView messageText;
        protected ProgressBar messageProgressBar;

        public ViewHolder(View itemView, Message message) {
            super(itemView);
            // The only Messages with TextViews are QUERY and RESPONSE:
            if (message.getTextType() == MessageType.QUERY) {
                messageText = itemView.findViewById(R.id.text_user_message);
            } else if (message.getTextType() == MessageType.RESPONSE) {
                    messageText = itemView.findViewById(R.id.text_assistant_message);
                    messageProgressBar = itemView.findViewById(R.id.text_assistant_loading);

            }
        }
    }

    private class TravelLinkSpan extends URLSpan {
        private TravelLinkSpan(String url) {
            super(url);
        }

        @Override
        public void onClick(View widget) {
            String url = getURL();
            if (handler != null) {
                handler.displayWebView(url);
            }
        }
    }

    public interface Handler {
        void displayWebView(String url);
    }
}
