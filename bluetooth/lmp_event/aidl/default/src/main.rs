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
//! Implements LMP Event Example Service.


use android_hardware_bluetooth_lmp_event::aidl::android::hardware::bluetooth::lmp_event::IBluetoothLmpEvent::{
    IBluetoothLmpEvent, BnBluetoothLmpEvent
};

use binder::BinderFeatures;
use log::{info, LevelFilter};

mod lmp_event;

const LOG_TAG: &str = "lmp_event_service_example";

fn main() {
    info!("{LOG_TAG}: starting service");
    let logger_success = logger::init(
        logger::Config::default().with_tag_on_device(LOG_TAG).with_max_level(LevelFilter::Trace)
    );
    if !logger_success {
        panic!("{LOG_TAG}: Failed to start logger");
    }

    binder::ProcessState::set_thread_pool_max_thread_count(0);

    let lmp_event_service = lmp_event::LmpEvent::new();
    let lmp_event_service_binder = BnBluetoothLmpEvent::new_binder(lmp_event_service, BinderFeatures::default());

    let descriptor = format!("{}/default", lmp_event::LmpEvent::get_descriptor());
    if binder::is_declared(&descriptor).expect("Failed to check if declared") {
        binder::add_service(&descriptor, lmp_event_service_binder.as_binder()).expect("Failed to register service");
    } else {
        info!("{LOG_TAG}: Failed to register service. Not declared.");
    }
    binder::ProcessState::join_thread_pool()
}
