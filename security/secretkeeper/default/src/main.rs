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

//! Non-secure implementation of the Secretkeeper HAL.

use log::{error, info, LevelFilter};
use secretkeeper_hal::SecretkeeperService;
use secretkeeper_nonsecure::{AuthGraphChannel, SecretkeeperChannel, LocalTa};
use std::sync::{Arc, Mutex};
use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::{
    BpSecretkeeper, ISecretkeeper,
};

fn main() {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("NonSecureSecretkeeper")
            .with_max_level(LevelFilter::Info)
            .with_log_buffer(android_logger::LogId::System),
    );
    // Redirect panic messages to logcat.
    std::panic::set_hook(Box::new(|panic_info| {
        error!("{}", panic_info);
    }));

    let ta = Arc::new(Mutex::new(LocalTa::new()));
    let ag_channel = AuthGraphChannel(ta.clone());
    let sk_channel = SecretkeeperChannel(ta.clone());

    let service = SecretkeeperService::new_as_binder(sk_channel, ag_channel);
    let service_name = format!(
        "{}/nonsecure",
        <BpSecretkeeper as ISecretkeeper>::get_descriptor()
    );
    binder::add_service(&service_name, service.as_binder()).unwrap_or_else(|e| {
        panic!("Failed to register service {service_name} because of {e:?}.",);
    });
    info!("Registered Binder service, joining threadpool.");
    binder::ProcessState::join_thread_pool();
}
