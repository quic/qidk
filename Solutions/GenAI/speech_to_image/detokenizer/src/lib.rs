//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use tokenizers::Tokenizer;

#[unsafe(no_mangle)]
pub extern "C" fn detokenize(
    token_ids_ptr: *const c_char, 
    tokenizer_path_ptr: *const c_char
) -> *const c_char {
    if token_ids_ptr.is_null() || tokenizer_path_ptr.is_null() {
        return std::ptr::null();
    }

    let token_ids_str = unsafe { CStr::from_ptr(token_ids_ptr) }
        .to_str()
        .unwrap_or("");
    
    let tokenizer_path = unsafe { CStr::from_ptr(tokenizer_path_ptr) }
        .to_str()
        .unwrap_or("");

    let token_ids: Vec<u32> = token_ids_str
        .split(',')
        .filter_map(|s| s.trim().parse().ok())
        .collect();

    let tokenizer = match Tokenizer::from_file(tokenizer_path) {
        Ok(t) => t,
        Err(_) => return std::ptr::null_mut(),
    };

    let decoded_text = match tokenizer.decode(&token_ids, true) {
        Ok(text) => text,
        Err(_) => return std::ptr::null_mut(),
    };

    let c_string = CString::new(decoded_text).unwrap();
    c_string.into_raw()
}