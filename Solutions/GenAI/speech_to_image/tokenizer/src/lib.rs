//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

use tokenizers::tokenizer::Tokenizer;
use std::ffi::{CString, CStr};
use std::os::raw::c_char;

#[unsafe(no_mangle)]
pub extern "C" fn tokenize(input: *const c_char, tokenizer_path: *const c_char) -> *const c_char {
    if input.is_null() || tokenizer_path.is_null() {
        return std::ptr::null();
    }

    let c_str_input = unsafe { CStr::from_ptr(input) };
    let text = match c_str_input.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null(),
    };

    let c_str_path = unsafe { CStr::from_ptr(tokenizer_path) };
    let path = match c_str_path.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null(),
    };

    let tokenizer = match Tokenizer::from_file(path) {
        Ok(t) => t,
        Err(_) => return std::ptr::null(),
    };

    let encoding = match tokenizer.encode(text, true) {
        Ok(e) => e,
        Err(_) => return std::ptr::null(),
    };

    let token_ids = encoding.get_ids();

    let token_string = token_ids.iter()
                                .map(|id| id.to_string())
                                .collect::<Vec<String>>()
                                .join(",");

    let c_str = CString::new(token_string).unwrap();
    c_str.into_raw()
}
