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

//! Common functionality for non-secure/testing instance of AuthGraph.

use authgraph_boringssl as boring;
use authgraph_core::{
    error,
    key::MillisecondsSinceEpoch,
    keyexchange,
    ta::{AuthGraphTa, Role},
    traits,
};
use authgraph_hal::channel::SerializedChannel;
use std::sync::{Arc, Mutex};
use std::time::Instant;

/// Monotonic clock with an epoch that starts at the point of construction.
/// (This makes it unsuitable for use outside of testing, because the epoch
/// will not match that of any other component.)
pub struct StdClock(Instant);

impl Default for StdClock {
    fn default() -> Self {
        Self(Instant::now())
    }
}

impl traits::MonotonicClock for StdClock {
    fn now(&self) -> MillisecondsSinceEpoch {
        let millis: i64 = self
            .0
            .elapsed()
            .as_millis()
            .try_into()
            .expect("failed to fit timestamp in i64");
        MillisecondsSinceEpoch(millis)
    }
}

/// Implementation of the AuthGraph TA that runs locally in-process (and which is therefore
/// insecure).
pub struct LocalTa {
    ta: Arc<Mutex<AuthGraphTa>>,
}

impl LocalTa {
    /// Create a new instance.
    pub fn new() -> Result<Self, error::Error> {
        Ok(Self {
            ta: Arc::new(Mutex::new(AuthGraphTa::new(
                keyexchange::AuthGraphParticipant::new(
                    boring::crypto_trait_impls(),
                    Box::<boring::test_device::AgDevice>::default(),
                    keyexchange::MAX_OPENED_SESSIONS,
                )?,
                Role::Both,
            ))),
        })
    }
}

/// Pretend to be a serialized channel to the TA, but actually just directly invoke the TA with
/// incoming requests.
impl SerializedChannel for LocalTa {
    const MAX_SIZE: usize = usize::MAX;

    fn execute(&mut self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
        Ok(self.ta.lock().unwrap().process(req_data))
    }
}
