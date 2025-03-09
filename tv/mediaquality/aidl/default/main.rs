/*
 * Copyright (C) 2024 The Android Open Source Project
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
//! This implements the MediaQuality Example Service.
use android_hardware_tv_mediaquality::aidl::android::hardware::tv::mediaquality::IMediaQuality::{BnMediaQuality, IMediaQuality};
use binder::BinderFeatures;

mod hal;
use hal::media_quality_hal_impl::MediaQualityService;

const LOG_TAG: &str = "mediaquality_service_example_rust";

use log::LevelFilter;

fn main() {

    android_logger::init_once(
        android_logger::Config::default()
            .with_tag(LOG_TAG)
            .with_max_level(LevelFilter::Info),
    );

    let media_quality_service = MediaQualityService::new();
    let media_quality_service_binder = BnMediaQuality::new_binder(media_quality_service, BinderFeatures::default());

    let service_name = format!("{}/default", MediaQualityService::get_descriptor());
    binder::add_service(&service_name, media_quality_service_binder.as_binder())
        .expect("Failed to register service");

    log::info!("MediaQualityHal service is running...");

    binder::ProcessState::join_thread_pool();
}
