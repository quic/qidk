//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <streambuf>
#include <string>
#include <android/log.h>
#include <iostream>
#include "AndroidLogger.hpp"


// Custom stream buffer
class AndroidLogBuffer : public std::streambuf {
public:
    AndroidLogBuffer(int priority, const char* tag) : priority_(priority), tag_(tag) {}

protected:
    int overflow(int c) override {
        if (c == EOF) {
            return !EOF;
        }
        buffer_ += static_cast<char>(c);
        if (static_cast<char>(c) == '\n') {
            flush();
        }
        return c;
    }

    int sync() override {
        flush();
        return 0;
    }

private:
    void flush() {
        if (!buffer_.empty()) {
            __android_log_write(priority_, tag_, buffer_.c_str());
            buffer_.clear();
        }
    }

    int priority_;
    const char* tag_;
    std::string buffer_;
};

// Custom stream
class AndroidLogStream : public std::ostream {
public:
    AndroidLogStream(int priority, const char* tag) : std::ostream(&buffer_), buffer_(priority, tag) {}

private:
    AndroidLogBuffer buffer_;
};

// Global instances of the custom streams
AndroidLogStream androidCout(ANDROID_LOG_INFO, "SpeechToImage-cout");
AndroidLogStream androidCerr(ANDROID_LOG_ERROR, "SpeechToImage-cerr");

// Function to redirect cout and cerr
void redirect_cout_cerr_to_logcat() {
    std::cout.rdbuf(androidCout.rdbuf());
    std::cerr.rdbuf(androidCerr.rdbuf());
}