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
//! Implements LMP Event AIDL Interface.

use android_hardware_bluetooth_lmp_event::aidl::android::hardware::bluetooth::lmp_event::{
    Direction::Direction, AddressType::AddressType, IBluetoothLmpEvent::IBluetoothLmpEvent,
    IBluetoothLmpEventCallback::IBluetoothLmpEventCallback, LmpEventId::LmpEventId,
    Timestamp::Timestamp,
};

use binder::{Interface, Result, Strong};

use log::info;
use std::thread;
use std::time;


pub struct LmpEvent;

impl LmpEvent {
    pub fn new() -> Self {
        Self
    }
}

impl Interface for LmpEvent {}

impl IBluetoothLmpEvent for LmpEvent {
    fn registerForLmpEvents(&self,
                            callback: &Strong<dyn IBluetoothLmpEventCallback>,
                            address_type: AddressType,
                            address: &[u8; 6],
                            lmp_event_ids: &[LmpEventId]
    ) -> Result<()> {
        info!("registerForLmpEvents");

        let cb = callback.clone();
        let addr_type = address_type;
        let addr = address.clone();
        let lmp_event = lmp_event_ids.to_vec();

        let thread_handle = thread::spawn(move || {
            let ts = Timestamp {
                bluetoothTimeUs: 1000000,
                systemTimeUs: 2000000,
            };

            info!("sleep for 1000 ms");
            thread::sleep(time::Duration::from_millis(1000));

            info!("callback event");
            cb.onEventGenerated(&ts, addr_type, &addr, Direction::RX, lmp_event[0], 1)
                .expect("onEventGenerated failed");
        });

        info!("callback register");
        callback.onRegistered(true)?;

        thread_handle.join().expect("join failed");
        Ok(())
    }
    fn unregisterLmpEvents(&self, _address_type: AddressType, _address: &[u8; 6]) -> Result<()> {
        info!("unregisterLmpEvents");

        Ok(())
    }
}
