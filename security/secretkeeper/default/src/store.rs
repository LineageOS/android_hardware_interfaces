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
use secretkeeper_comm::data_types::error::Error;
use secretkeeper_core::store::KeyValueStore;
use std::collections::HashMap;

/// An in-memory implementation of KeyValueStore. Please note that this is entirely for
/// testing purposes. Refer to the documentation of `PolicyGatedStorage` & Secretkeeper HAL for
/// persistence requirements.
#[derive(Default)]
pub struct InMemoryStore(HashMap<Vec<u8>, Vec<u8>>);
impl KeyValueStore for InMemoryStore {
    fn store(&mut self, key: &[u8], val: &[u8]) -> Result<(), Error> {
        // This will overwrite the value if key is already present.
        let _ = self.0.insert(key.to_vec(), val.to_vec());
        Ok(())
    }

    fn get(&self, key: &[u8]) -> Result<Option<Vec<u8>>, Error> {
        let optional_val = self.0.get(key);
        Ok(optional_val.cloned())
    }
}
