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

//! Default implementation of the AuthGraph key exchange HAL.
//!
//! This implementation of the HAL is only intended to allow testing and policy compliance.  A real
//! implementation of the AuthGraph HAL would be implemented in a secure environment, and would not
//! be independently registered with service manager (a secure component that uses AuthGraph would
//! expose an entrypoint that allowed retrieval of the specific IAuthGraphKeyExchange instance that
//! is correlated with the component).

use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    Arc::Arc, IAuthGraphKeyExchange::BnAuthGraphKeyExchange,
    IAuthGraphKeyExchange::IAuthGraphKeyExchange, Identity::Identity, KeInitResult::KeInitResult,
    Key::Key, PubKey::PubKey, SessionIdSignature::SessionIdSignature, SessionInfo::SessionInfo,
    SessionInitiationInfo::SessionInitiationInfo,
};
use authgraph_boringssl as boring;
use authgraph_core::{key::MillisecondsSinceEpoch, keyexchange as ke, traits};
use authgraph_hal::{err_to_binder, Innto, TryInnto};
use log::{error, info};
use std::ffi::CString;
use std::sync::Mutex;

static SERVICE_NAME: &str = "android.hardware.security.authgraph.IAuthGraphKeyExchange";
static SERVICE_INSTANCE: &str = "nonsecure";

/// Local error type for failures in the HAL service.
#[derive(Debug, Clone)]
struct HalServiceError(String);

impl From<String> for HalServiceError {
    fn from(s: String) -> Self {
        Self(s)
    }
}

fn main() {
    if let Err(e) = inner_main() {
        panic!("HAL service failed: {:?}", e);
    }
}

fn inner_main() -> Result<(), HalServiceError> {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("authgraph-hal-nonsecure")
            .with_min_level(log::Level::Info)
            .with_log_id(android_logger::LogId::System),
    );
    // Redirect panic messages to logcat.
    std::panic::set_hook(Box::new(|panic_info| {
        error!("{}", panic_info);
    }));

    info!("Insecure AuthGraph key exchange HAL service is starting.");

    info!("Starting thread pool now.");
    binder::ProcessState::start_thread_pool();

    // Register the service
    let service = AuthGraphService::new_as_binder();
    let service_name = format!("{}/{}", SERVICE_NAME, SERVICE_INSTANCE);
    binder::add_service(&service_name, service.as_binder()).map_err(|e| {
        format!(
            "Failed to register service {} because of {:?}.",
            service_name, e
        )
    })?;

    info!("Successfully registered AuthGraph HAL services.");
    binder::ProcessState::join_thread_pool();
    info!("AuthGraph HAL service is terminating."); // should not reach here
    Ok(())
}

/// Non-secure implementation of the AuthGraph key exchange service.
struct AuthGraphService {
    imp: Mutex<traits::TraitImpl>,
}

impl AuthGraphService {
    /// Create a new instance.
    fn new() -> Self {
        Self {
            imp: Mutex::new(traits::TraitImpl {
                aes_gcm: Box::new(boring::BoringAes),
                ecdh: Box::new(boring::BoringEcDh),
                ecdsa: Box::new(boring::BoringEcDsa),
                hmac: Box::new(boring::BoringHmac),
                hkdf: Box::new(boring::BoringHkdf),
                sha256: Box::new(boring::BoringSha256),
                rng: Box::new(boring::BoringRng),
                device: Box::<boring::test_device::AgDevice>::default(),
                clock: Some(Box::new(StdClock)),
            }),
        }
    }

    /// Create a new instance wrapped in a proxy object.
    pub fn new_as_binder() -> binder::Strong<dyn IAuthGraphKeyExchange> {
        BnAuthGraphKeyExchange::new_binder(Self::new(), binder::BinderFeatures::default())
    }
}

impl binder::Interface for AuthGraphService {}

/// Extract (and require) an unsigned public key as bytes from a [`PubKey`].
fn unsigned_pub_key(pub_key: &PubKey) -> binder::Result<&[u8]> {
    match pub_key {
        PubKey::PlainKey(key) => Ok(&key.plainPubKey),
        PubKey::SignedKey(_) => Err(binder::Status::new_exception(
            binder::ExceptionCode::ILLEGAL_ARGUMENT,
            Some(&CString::new("expected unsigned public key").unwrap()),
        )),
    }
}

/// This nonsecure implementation of the AuthGraph HAL interface directly calls the AuthGraph
/// reference implementation library code; a real implementation requires the AuthGraph
/// code to run in a secure environment, not within Android.
impl IAuthGraphKeyExchange for AuthGraphService {
    fn create(&self) -> binder::Result<SessionInitiationInfo> {
        info!("create()");
        let mut imp = self.imp.lock().unwrap();
        let info = ke::create(&mut *imp).map_err(err_to_binder)?;
        Ok(info.innto())
    }
    fn init(
        &self,
        peer_pub_key: &PubKey,
        peer_id: &Identity,
        peer_nonce: &[u8],
        peer_version: i32,
    ) -> binder::Result<KeInitResult> {
        info!("init(v={peer_version})");
        let mut imp = self.imp.lock().unwrap();
        let peer_pub_key = unsigned_pub_key(peer_pub_key)?;
        let result = ke::init(
            &mut *imp,
            peer_pub_key,
            &peer_id.identity,
            &peer_nonce,
            peer_version,
        )
        .map_err(err_to_binder)?;
        Ok(result.innto())
    }

    fn finish(
        &self,
        peer_pub_key: &PubKey,
        peer_id: &Identity,
        peer_signature: &SessionIdSignature,
        peer_nonce: &[u8],
        peer_version: i32,
        own_key: &Key,
    ) -> binder::Result<SessionInfo> {
        info!("finish(v={peer_version})");
        let mut imp = self.imp.lock().unwrap();
        let peer_pub_key = unsigned_pub_key(peer_pub_key)?;
        let own_key: Key = own_key.clone();
        let own_key: authgraph_core::key::Key = own_key.try_innto()?;
        let session_info = ke::finish(
            &mut *imp,
            peer_pub_key,
            &peer_id.identity,
            &peer_signature.signature,
            &peer_nonce,
            peer_version,
            own_key,
        )
        .map_err(err_to_binder)?;
        Ok(session_info.innto())
    }

    fn authenticationComplete(
        &self,
        peer_signature: &SessionIdSignature,
        shared_keys: &[Arc; 2],
    ) -> binder::Result<[Arc; 2]> {
        info!("authComplete()");
        let mut imp = self.imp.lock().unwrap();
        let shared_keys = [shared_keys[0].arc.clone(), shared_keys[1].arc.clone()];
        let arcs = ke::authentication_complete(&mut *imp, &peer_signature.signature, shared_keys)
            .map_err(err_to_binder)?;
        Ok(arcs.map(|arc| Arc { arc }))
    }
}

/// Monotonic clock.
#[derive(Default)]
pub struct StdClock;

impl traits::MonotonicClock for StdClock {
    fn now(&self) -> authgraph_core::key::MillisecondsSinceEpoch {
        let mut time = libc::timespec {
            tv_sec: 0,  // libc::time_t
            tv_nsec: 0, // libc::c_long
        };
        let rc =
        // Safety: `time` is a valid structure.
            unsafe { libc::clock_gettime(libc::CLOCK_BOOTTIME, &mut time as *mut libc::timespec) };
        if rc < 0 {
            log::warn!("failed to get time!");
            return MillisecondsSinceEpoch(0);
        }
        // The types in `libc::timespec` may be different on different architectures,
        // so allow conversion to `i64`.
        #[allow(clippy::unnecessary_cast)]
        MillisecondsSinceEpoch((time.tv_sec as i64 * 1000) + (time.tv_nsec as i64 / 1000 / 1000))
    }
}
