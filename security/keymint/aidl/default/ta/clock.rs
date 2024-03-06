//
// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Monotonic clock implementation.

use kmr_common::crypto;
use std::time::Instant;

/// Monotonic clock.
pub struct StdClock {
    start: Instant,
}

impl StdClock {
    /// Create new clock instance, holding time since construction.
    pub fn new() -> Self {
        Self {
            start: Instant::now(),
        }
    }
}

impl crypto::MonotonicClock for StdClock {
    fn now(&self) -> crypto::MillisecondsSinceEpoch {
        let duration = self.start.elapsed();
        crypto::MillisecondsSinceEpoch(duration.as_millis().try_into().unwrap())
    }
}
