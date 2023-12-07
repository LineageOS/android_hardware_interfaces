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

use authgraph_hal::service;
use authgraph_nonsecure::LocalTa;
use log::{error, info};

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
    let local_ta = LocalTa::new().map_err(|e| format!("Failed to create the TA because: {e:?}"))?;
    let service = service::AuthGraphService::new_as_binder(local_ta);
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
