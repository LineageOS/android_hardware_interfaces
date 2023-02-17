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

use diced_open_dice::DiceArtifacts;
use diced_sample_inputs;
use diced_utils;

mod utils;
use utils::with_connection;

static TEST_MESSAGE: &[u8] = &[
    // "My test message!"
    0x4d, 0x79, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x21,
    0x0a,
];

// This test calls derive with an empty argument vector and with a set of three input values.
// It then performs the same three derivation steps on the result of the former and compares
// the result to the result of the latter.
#[test]
fn equivalence_test() {
    with_connection(|device| {
        let input_values = diced_sample_inputs::get_input_values_vector();
        let former = device.derive(&[]).expect("Trying to call derive.");
        let latter = device
            .derive(&input_values)
            .expect("Trying to call derive with input values.");
        let artifacts = diced_utils::ResidentArtifacts::new(
            former.cdiAttest[..].try_into().unwrap(),
            former.cdiSeal[..].try_into().unwrap(),
            &former.bcc.data,
        )
        .unwrap();

        let artifacts = artifacts.execute_steps(input_values.iter()).unwrap();
        let from_former = diced_utils::make_bcc_handover(
            artifacts.cdi_attest(),
            artifacts.cdi_seal(),
            artifacts.bcc().expect("bcc is none"),
        )
        .unwrap();
        // TODO b/204938506 when we have a parser/verifier, check equivalence rather
        // than bit by bit equality.
        assert_eq!(latter, from_former);
        Ok(())
    })
}

#[test]
fn sign_and_verify() {
    with_connection(|device| {
        let _signature = device
            .sign(&[], TEST_MESSAGE)
            .expect("Trying to call sign.");

        let _bcc = device
            .getAttestationChain(&[])
            .expect("Trying to call getAttestationChain.");
        // TODO b/204938506 check the signature with the bcc when the verifier is available.
        Ok(())
    })
}
