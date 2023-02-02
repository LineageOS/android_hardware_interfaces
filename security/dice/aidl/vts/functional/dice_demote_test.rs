// Copyright 2021, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use diced_sample_inputs;
use diced_utils;
use std::convert::TryInto;

mod utils;
use utils::with_connection;

// This test calls derive with an empty argument vector, then demotes the HAL using
// a set of three input values, and then calls derive with empty argument vector again.
// It then performs the same three derivation steps on the result of the former and compares
// the result to the result of the latter.
#[test]
fn demote_test() {
    with_connection(|device| {
        let input_values = diced_sample_inputs::get_input_values_vector();
        let former = device.derive(&[]).expect("Trying to call derive.");
        device
            .demote(&input_values)
            .expect("Trying to call demote with input values.");

        let latter = device
            .derive(&[])
            .expect("Trying to call derive after demote.");

        let artifacts = diced_utils::ResidentArtifacts::new(
            former.cdiAttest[..].try_into().unwrap(),
            former.cdiSeal[..].try_into().unwrap(),
            &former.bcc.data,
        )
        .unwrap();

        let artifacts = artifacts.execute_steps(input_values.iter()).unwrap();
        let (cdi_attest, cdi_seal, bcc) = artifacts.into_tuple();
        let from_former = diced_utils::make_bcc_handover(
            cdi_attest[..].try_into().unwrap(),
            cdi_seal[..].try_into().unwrap(),
            &bcc,
        )
        .unwrap();
        // TODO b/204938506 when we have a parser/verifier, check equivalence rather
        // than bit by bit equality.
        assert_eq!(latter, from_former);
        Ok(())
    })
}
