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

use android_hardware_security_dice::aidl::android::hardware::security::dice::IDiceDevice::IDiceDevice;
use anyhow::Result;
use binder::Strong;
use keystore2_vintf::get_aidl_instances;
use std::sync::Arc;

static DICE_DEVICE_SERVICE_NAME: &str = &"android.hardware.security.dice";
static DICE_DEVICE_INTERFACE_NAME: &str = &"IDiceDevice";

/// This function iterates through all announced IDiceDevice services and runs the given test
/// closure against connections to each of them. It also modifies the panic hook to indicate
/// on which instance the test failed in case the test closure panics.
pub fn with_connection<R, F>(test: F)
where
    F: Fn(&Strong<dyn IDiceDevice>) -> Result<R>,
{
    let instances = get_aidl_instances(DICE_DEVICE_SERVICE_NAME, 1, DICE_DEVICE_INTERFACE_NAME);
    let panic_hook = Arc::new(std::panic::take_hook());
    for i in instances.into_iter() {
        let panic_hook_clone = panic_hook.clone();
        let instance_clone = i.clone();
        std::panic::set_hook(Box::new(move |v| {
            println!("While testing instance: \"{}\"", instance_clone);
            panic_hook_clone(v)
        }));
        let connection: Strong<dyn IDiceDevice> = binder::get_interface(&format!(
            "{}.{}/{}",
            DICE_DEVICE_SERVICE_NAME, DICE_DEVICE_INTERFACE_NAME, i
        ))
        .unwrap();
        test(&connection).unwrap();
        drop(std::panic::take_hook());
    }
    // Cannot call unwrap here because the panic hook is not Debug.
    std::panic::set_hook(match Arc::try_unwrap(panic_hook) {
        Ok(hook) => hook,
        _ => panic!("Failed to unwrap and reset previous panic hook."),
    })
}
