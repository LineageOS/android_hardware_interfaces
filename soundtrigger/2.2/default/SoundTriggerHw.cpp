/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "SoundTriggerHw"

#include "SoundTriggerHw.h"

#include <android/log.h>
#include <utility>

namespace android {
namespace hardware {
namespace soundtrigger {
namespace V2_2 {
namespace implementation {

Return<void> SoundTriggerHw::getModelState_2_2(int32_t modelHandle, getModelState_cb hidl_cb) {
    sp<SoundModelClient> client;
    if (mHwDevice == NULL) {
        hidl_cb(-ENODEV, NULL);
        return Void();
    }

    {
        AutoMutex lock(mLock);
        client = mClients.valueFor(modelHandle);
        if (client == 0) {
            hidl_cb(-ENOSYS, NULL);
            return Void();
        }
    }

    if (mHwDevice->get_model_state == NULL) {
        ALOGE("Failed to get model state from device, no such method");
        hidl_cb(-ENODEV, NULL);
        return Void();
    }

    // Get the state from the device (as a recognition event)
    struct sound_trigger_recognition_event* event =
        mHwDevice->get_model_state(mHwDevice, client->getHalHandle());
    if (event == NULL) {
        ALOGE("Failed to get model state from device");
        hidl_cb(-ENODEV, NULL);
        return Void();
    }

    // Allocate shared memory to return to the client
    sp<IAllocator> alloc = IAllocator::getService("ashmem");
    if (alloc == 0) {
        ALOGE("Failed to retrieve ashmem allocator service");
        free(event);
        hidl_cb(-ENOMEM, NULL);
        return Void();
    }
    // Note: Only generic recognition events are currently supported
    int n_bytes = sizeof(struct sound_trigger_generic_recognition_event);
    bool success = false;
    const hidl_memory& mem;
    Return<void> r = ashmem->allocate(n_bytes, [&](bool s, const hidl_memory& m) {
        success = s;
        if (success) mem = m;
    });
    if (r.isOk() && success) {
        // Copy the event data to the shared memory
        sp<IMemory> memory = mapMemory(mem);
        if (memory != 0) {
            struct sound_trigger_generic_recognition_event* data =
                (struct sound_trigger_generic_recognition_event*)memory->getPointer();
            memory->update();
            *data = *event;
            memory->commit();

            // Return the event memory via this callback
            hidl_cb(0, memory);
        } else {
            ALOGE("Failed to map memory for recognition event");
            hidl_cb(-ENOMEM, NULL);
        }
    } else {
        ALOGE("Failed to allocate %d bytes from ashmem allocator service", n_bytes);
        hidl_cb(-ENOMEM, NULL);
    }

    free(event);
    return Void();
}

ISoundTriggerHw* HIDL_FETCH_ISoundTriggerHw(const char* /* name */) {
    return (new SoundTriggerHw())->getInterface();
}

}  // namespace implementation
}  // namespace V2_2
}  // namespace soundtrigger
}  // namespace hardware
}  // namespace android
