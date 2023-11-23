/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! Tests of individual AuthGraph role (source or sink) functionality.

#![cfg(test)]

use authgraph_vts_test as vts;
use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    IAuthGraphKeyExchange::IAuthGraphKeyExchange,
};
use binder::StatusCode;

const AUTH_GRAPH_NONSECURE: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyExchange/nonsecure";

/// Retrieve the /nonsecure instance of AuthGraph, which supports both sink and source roles.
fn get_nonsecure() -> Option<binder::Strong<dyn IAuthGraphKeyExchange>> {
    match binder::get_interface(AUTH_GRAPH_NONSECURE) {
        Ok(ag) => Some(ag),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get AuthGraph/nonsecure: {e:?}"),
    }
}

/// Macro to require availability of a /nonsecure instance of AuthGraph.
///
/// Note that this macro triggers `return` if not found.
macro_rules! require_nonsecure {
    {} => {
        match get_nonsecure() {
            Some(v) => v,
            None => {
                eprintln!("Skipping test as no /nonsecure impl found");
                return;
            }
        }
    }
}

#[test]
fn test_nonsecure_source_mainline() {
    let mut sink = vts::test_ag_participant().expect("failed to create a local sink");
    vts::source::test_mainline(&mut sink, require_nonsecure!());
}
#[test]
fn test_nonsecure_source_corrupt_sig() {
    let mut sink = vts::test_ag_participant().expect("failed to create a local sink");
    vts::source::test_corrupt_sig(&mut sink, require_nonsecure!());
}
#[test]
fn test_nonsecure_source_corrupt_keys() {
    let mut sink = vts::test_ag_participant().expect("failed to create a local sink");
    vts::source::test_corrupt_key(&mut sink, require_nonsecure!());
}
#[test]
fn test_nonsecure_sink_mainline() {
    let mut source = vts::test_ag_participant().expect("failed to create a local source");
    vts::sink::test_mainline(&mut source, require_nonsecure!());
}
#[test]
fn test_nonsecure_sink_corrupt_sig() {
    let mut source = vts::test_ag_participant().expect("failed to create a local source");
    vts::sink::test_corrupt_sig(&mut source, require_nonsecure!());
}
#[test]
fn test_nonsecure_sink_corrupt_keys() {
    let mut source = vts::test_ag_participant().expect("failed to create a local source");
    vts::sink::test_corrupt_keys(&mut source, require_nonsecure!());
}
