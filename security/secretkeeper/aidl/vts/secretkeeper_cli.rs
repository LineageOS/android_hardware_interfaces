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

//! Command line test tool for interacting with Secretkeeper.

use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::{
    ISecretkeeper::ISecretkeeper, SecretId::SecretId,
};
use anyhow::{anyhow, bail, Context, Result};
use authgraph_boringssl::BoringSha256;
use authgraph_core::traits::Sha256;
use clap::{Args, Parser, Subcommand};
use coset::CborSerializable;
use dice_policy_builder::{
    policy_for_dice_chain, CertIndex, ConstraintSpec, ConstraintType, MissingAction,
    WILDCARD_FULL_ARRAY,
};

use secretkeeper_client::{dice::OwnedDiceArtifactsWithExplicitKey, SkSession};
use secretkeeper_comm::data_types::{
    error::SecretkeeperError,
    packet::{ResponsePacket, ResponseType},
    request::Request,
    request_response_impl::{GetSecretRequest, GetSecretResponse, StoreSecretRequest},
    response::Response,
    {Id, Secret},
};
use secretkeeper_test::{
    dice_sample::make_explicit_owned_dice, AUTHORITY_HASH, CONFIG_DESC, MODE, SECURITY_VERSION,
    SUBCOMPONENT_AUTHORITY_HASH, SUBCOMPONENT_DESCRIPTORS, SUBCOMPONENT_SECURITY_VERSION,
};
use std::io::Write;

#[derive(Parser, Debug)]
#[command(about = "Interact with Secretkeeper HAL")]
#[command(version = "0.1")]
#[command(propagate_version = true)]
struct Cli {
    #[command(subcommand)]
    command: Command,

    /// Secretkeeper instance to connect to.
    #[arg(long, short)]
    instance: Option<String>,

    /// Security version in leaf DICE node.
    #[clap(default_value_t = 100)]
    #[arg(long, short = 'v')]
    dice_version: u64,

    /// Show hex versions of secrets and their IDs.
    #[clap(default_value_t = false)]
    #[arg(long, short = 'v')]
    hex: bool,
}

#[derive(Subcommand, Debug)]
enum Command {
    /// Store a secret value.
    Store(StoreArgs),
    /// Get a secret value.
    Get(GetArgs),
    /// Delete a secret value.
    Delete(DeleteArgs),
    /// Delete all secret values.
    DeleteAll(DeleteAllArgs),
}

#[derive(Args, Debug)]
struct StoreArgs {
    /// Identifier for the secret, as either a short (< 32 byte) string, or as 32 bytes of hex.
    id: String,
    /// Value to use as the secret value. If specified as 32 bytes of hex, the decoded value
    /// will be used as-is; otherwise, a string (less than 31 bytes in length) will be encoded
    /// as the secret.
    value: String,
}

#[derive(Args, Debug)]
struct GetArgs {
    /// Identifier for the secret, as either a short (< 32 byte) string, or as 32 bytes of hex.
    id: String,
}

#[derive(Args, Debug)]
struct DeleteArgs {
    /// Identifier for the secret, as either a short (< 32 byte) string, or as 32 bytes of hex.
    id: String,
}

#[derive(Args, Debug)]
struct DeleteAllArgs {
    /// Confirm deletion of all secrets.
    yes: bool,
}

const SECRETKEEPER_SERVICE: &str = "android.hardware.security.secretkeeper.ISecretkeeper";

/// Secretkeeper client information.
struct SkClient {
    sk: binder::Strong<dyn ISecretkeeper>,
    session: SkSession,
    dice_artifacts: OwnedDiceArtifactsWithExplicitKey,
}

impl SkClient {
    fn new(instance: &str, dice_artifacts: OwnedDiceArtifactsWithExplicitKey) -> Self {
        let sk: binder::Strong<dyn ISecretkeeper> =
            binder::get_interface(&format!("{SECRETKEEPER_SERVICE}/{instance}")).unwrap();
        let session = SkSession::new(sk.clone(), &dice_artifacts, None).unwrap();
        Self { sk, session, dice_artifacts }
    }

    fn secret_management_request(&mut self, req_data: &[u8]) -> Result<Vec<u8>> {
        self.session
            .secret_management_request(req_data)
            .map_err(|e| anyhow!("secret management: {e:?}"))
    }

    /// Construct a sealing policy on the DICE chain with constraints:
    /// 1. `ExactMatch` on `AUTHORITY_HASH` (non-optional).
    /// 2. `ExactMatch` on `MODE` (non-optional).
    /// 3. `GreaterOrEqual` on `SECURITY_VERSION` (optional).
    fn sealing_policy(&self) -> Result<Vec<u8>> {
        let dice =
            self.dice_artifacts.explicit_key_dice_chain().context("extract explicit DICE chain")?;

        let constraint_spec = [
            ConstraintSpec::new(
                ConstraintType::ExactMatch,
                vec![AUTHORITY_HASH],
                MissingAction::Fail,
                CertIndex::All,
            ),
            ConstraintSpec::new(
                ConstraintType::ExactMatch,
                vec![MODE],
                MissingAction::Fail,
                CertIndex::All,
            ),
            ConstraintSpec::new(
                ConstraintType::GreaterOrEqual,
                vec![CONFIG_DESC, SECURITY_VERSION],
                MissingAction::Ignore,
                CertIndex::All,
            ),
            // Constraints on sub components in the second last DiceChainEntry
            ConstraintSpec::new(
                ConstraintType::GreaterOrEqual,
                vec![
                    CONFIG_DESC,
                    SUBCOMPONENT_DESCRIPTORS,
                    WILDCARD_FULL_ARRAY,
                    SUBCOMPONENT_SECURITY_VERSION,
                ],
                MissingAction::Fail,
                CertIndex::FromEnd(1),
            ),
            ConstraintSpec::new(
                ConstraintType::ExactMatch,
                vec![
                    CONFIG_DESC,
                    SUBCOMPONENT_DESCRIPTORS,
                    WILDCARD_FULL_ARRAY,
                    SUBCOMPONENT_AUTHORITY_HASH,
                ],
                MissingAction::Fail,
                CertIndex::FromEnd(1),
            ),
        ];
        policy_for_dice_chain(dice, &constraint_spec)
            .unwrap()
            .to_vec()
            .context("serialize DICE policy")
    }

    fn store(&mut self, id: &Id, secret: &Secret) -> Result<()> {
        let store_request = StoreSecretRequest {
            id: id.clone(),
            secret: secret.clone(),
            sealing_policy: self.sealing_policy().context("build sealing policy")?,
        };
        let store_request =
            store_request.serialize_to_packet().to_vec().context("serialize StoreSecretRequest")?;

        let store_response = self.secret_management_request(&store_request)?;
        let store_response =
            ResponsePacket::from_slice(&store_response).context("deserialize ResponsePacket")?;
        let response_type = store_response.response_type().unwrap();
        if response_type == ResponseType::Success {
            Ok(())
        } else {
            let err = *SecretkeeperError::deserialize_from_packet(store_response).unwrap();
            Err(anyhow!("STORE failed: {err:?}"))
        }
    }

    fn get(&mut self, id: &Id) -> Result<Option<Secret>> {
        let get_request = GetSecretRequest { id: id.clone(), updated_sealing_policy: None }
            .serialize_to_packet()
            .to_vec()
            .context("serialize GetSecretRequest")?;

        let get_response = self.secret_management_request(&get_request).context("secret mgmt")?;
        let get_response =
            ResponsePacket::from_slice(&get_response).context("deserialize ResponsePacket")?;

        if get_response.response_type().unwrap() == ResponseType::Success {
            let get_response = *GetSecretResponse::deserialize_from_packet(get_response).unwrap();
            Ok(Some(Secret(get_response.secret.0)))
        } else {
            // Only expect a not-found failure.
            let err = *SecretkeeperError::deserialize_from_packet(get_response).unwrap();
            if err == SecretkeeperError::EntryNotFound {
                Ok(None)
            } else {
                Err(anyhow!("GET failed: {err:?}"))
            }
        }
    }

    /// Helper method to delete secrets.
    fn delete(&self, ids: &[&Id]) -> Result<()> {
        let ids: Vec<SecretId> = ids.iter().map(|id| SecretId { id: id.0 }).collect();
        self.sk.deleteIds(&ids).context("deleteIds")
    }

    /// Helper method to delete everything.
    fn delete_all(&self) -> Result<()> {
        self.sk.deleteAll().context("deleteAll")
    }
}

/// Convert a string input into an `Id`.  Input can be 64 bytes of hex, or a string
/// that will be hashed to give the `Id` value. Returns the `Id` and a display string.
fn string_to_id(s: &str, show_hex: bool) -> (Id, String) {
    if let Ok(data) = hex::decode(s) {
        if data.len() == 64 {
            // Assume something that parses as 64 bytes of hex is it.
            return (Id(data.try_into().unwrap()), s.to_string().to_lowercase());
        }
    }
    // Create a secret ID by repeating the SHA-256 hash of the string twice.
    let hash = BoringSha256.compute_sha256(s.as_bytes()).unwrap();
    let mut id = Id([0; 64]);
    id.0[..32].copy_from_slice(&hash);
    id.0[32..].copy_from_slice(&hash);
    if show_hex {
        let hex_id = hex::encode(&id.0);
        (id, format!("'{s}' (as {hex_id})"))
    } else {
        (id, format!("'{s}'"))
    }
}

/// Convert a string input into a `Secret`.  Input can be 32 bytes of hex, or a short string
/// that will be encoded as the `Secret` value. Returns the `Secret` and a display string.
fn value_to_secret(s: &str, show_hex: bool) -> Result<(Secret, String)> {
    if let Ok(data) = hex::decode(s) {
        if data.len() == 32 {
            // Assume something that parses as 32 bytes of hex is it.
            return Ok((Secret(data.try_into().unwrap()), s.to_string().to_lowercase()));
        }
    }
    let data = s.as_bytes();
    if data.len() > 31 {
        return Err(anyhow!("secret too long"));
    }
    let mut secret = Secret([0; 32]);
    secret.0[0] = data.len() as u8;
    secret.0[1..1 + data.len()].copy_from_slice(data);
    Ok(if show_hex {
        let hex_secret = hex::encode(&secret.0);
        (secret, format!("'{s}' (as {hex_secret})"))
    } else {
        (secret, format!("'{s}'"))
    })
}

/// Convert a `Secret` into a displayable string. If the secret looks like an encoded
/// string, show that, otherwise show the value in hex.
fn secret_to_value_display(secret: &Secret, show_hex: bool) -> String {
    let hex = hex::encode(&secret.0);
    secret_to_value(secret)
        .map(|s| if show_hex { format!("'{s}' (from {hex})") } else { format!("'{s}'") })
        .unwrap_or_else(|_e| format!("{hex}"))
}

/// Attempt to convert a `Secret` back to a string.
fn secret_to_value(secret: &Secret) -> Result<String> {
    let len = secret.0[0] as usize;
    if len > 31 {
        return Err(anyhow!("too long"));
    }
    std::str::from_utf8(&secret.0[1..1 + len]).map(|s| s.to_string()).context("not UTF-8 string")
}

fn main() -> Result<()> {
    let cli = Cli::parse();

    // Figure out which Secretkeeper instance is desired, and connect to it.
    let instance = if let Some(instance) = &cli.instance {
        // Explicitly specified.
        instance.clone()
    } else {
        // If there's only one instance, use that.
        let instances: Vec<String> = binder::get_declared_instances(SECRETKEEPER_SERVICE)
            .unwrap_or_default()
            .into_iter()
            .collect();
        match instances.len() {
            0 => bail!("No Secretkeeper instances available on device!"),
            1 => instances[0].clone(),
            _ => {
                bail!(
                    concat!(
                        "Multiple Secretkeeper instances available on device: {}\n",
                        "Use --instance <instance> to specify one."
                    ),
                    instances.join(", ")
                );
            }
        }
    };
    let dice = make_explicit_owned_dice(cli.dice_version);
    let mut sk_client = SkClient::new(&instance, dice);

    match cli.command {
        Command::Get(args) => {
            let (id, display_id) = string_to_id(&args.id, cli.hex);
            print!("GET key {display_id}: ");
            match sk_client.get(&id).context("GET") {
                Ok(None) => println!("not found"),
                Ok(Some(s)) => println!("{}", secret_to_value_display(&s, cli.hex)),
                Err(e) => {
                    println!("failed!");
                    return Err(e);
                }
            }
        }
        Command::Store(args) => {
            let (id, display_id) = string_to_id(&args.id, cli.hex);
            let (secret, display_secret) = value_to_secret(&args.value, cli.hex)?;
            println!("STORE key {display_id}: {display_secret}");
            sk_client.store(&id, &secret).context("STORE")?;
        }
        Command::Delete(args) => {
            let (id, display_id) = string_to_id(&args.id, cli.hex);
            println!("DELETE key {display_id}");
            sk_client.delete(&[&id]).context("DELETE")?;
        }
        Command::DeleteAll(args) => {
            if !args.yes {
                // Request confirmation.
                println!("Confirm delete all secrets: [y/N]");
                let _ = std::io::stdout().flush();
                let mut input = String::new();
                std::io::stdin().read_line(&mut input)?;
                let c = input.chars().next();
                if c != Some('y') && c != Some('Y') {
                    bail!("DELETE_ALL not confirmed");
                }
            }
            println!("DELETE_ALL");
            sk_client.delete_all().context("DELETE_ALL")?;
        }
    }
    Ok(())
}
