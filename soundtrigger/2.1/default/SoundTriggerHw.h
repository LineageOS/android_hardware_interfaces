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

#ifndef ANDROID_HARDWARE_SOUNDTRIGGER_V2_1_SOUNDTRIGGERHW_H
#define ANDROID_HARDWARE_SOUNDTRIGGER_V2_1_SOUNDTRIGGERHW_H

#include <SoundTriggerHalImpl.h>
#include <android/hardware/soundtrigger/2.1/ISoundTriggerHw.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace soundtrigger {
namespace V2_1 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct SoundTriggerHw : public V2_0::implementation::SoundTriggerHalImpl {
    SoundTriggerHw() = default;
    ISoundTriggerHw* getInterface() { return new TrampolineSoundTriggerHw_2_1(this); }

   protected:
    virtual ~SoundTriggerHw() = default;

    Return<void> loadSoundModel_2_1(const V2_1::ISoundTriggerHw::SoundModel& soundModel,
                                    const sp<V2_1::ISoundTriggerHwCallback>& callback,
                                    int32_t cookie,
                                    V2_1::ISoundTriggerHw::loadSoundModel_2_1_cb _hidl_cb);
    Return<void> loadPhraseSoundModel_2_1(
        const V2_1::ISoundTriggerHw::PhraseSoundModel& soundModel,
        const sp<V2_1::ISoundTriggerHwCallback>& callback, int32_t cookie,
        V2_1::ISoundTriggerHw::loadPhraseSoundModel_2_1_cb _hidl_cb);
    Return<int32_t> startRecognition_2_1(int32_t modelHandle,
                                         const V2_1::ISoundTriggerHw::RecognitionConfig& config);

   private:
    struct TrampolineSoundTriggerHw_2_1 : public ISoundTriggerHw {
        explicit TrampolineSoundTriggerHw_2_1(sp<SoundTriggerHw> impl) : mImpl(impl) {}

        // Methods from ::android::hardware::soundtrigger::V2_0::ISoundTriggerHw follow.
        Return<void> getProperties(getProperties_cb _hidl_cb) override {
            return mImpl->getProperties(_hidl_cb);
        }
        Return<void> loadSoundModel(const V2_0::ISoundTriggerHw::SoundModel& soundModel,
                                    const sp<V2_0::ISoundTriggerHwCallback>& callback,
                                    int32_t cookie, loadSoundModel_cb _hidl_cb) override {
            return mImpl->loadSoundModel(soundModel, callback, cookie, _hidl_cb);
        }
        Return<void> loadPhraseSoundModel(const V2_0::ISoundTriggerHw::PhraseSoundModel& soundModel,
                                          const sp<V2_0::ISoundTriggerHwCallback>& callback,
                                          int32_t cookie,
                                          loadPhraseSoundModel_cb _hidl_cb) override {
            return mImpl->loadPhraseSoundModel(soundModel, callback, cookie, _hidl_cb);
        }
        Return<int32_t> unloadSoundModel(V2_0::SoundModelHandle modelHandle) override {
            return mImpl->unloadSoundModel(modelHandle);
        }
        Return<int32_t> startRecognition(int32_t modelHandle,
                                         const V2_0::ISoundTriggerHw::RecognitionConfig& config,
                                         const sp<V2_0::ISoundTriggerHwCallback>& /*callback*/,
                                         int32_t /*cookie*/) override {
            return mImpl->startRecognition(modelHandle, config);
        }
        Return<int32_t> stopRecognition(V2_0::SoundModelHandle modelHandle) override {
            return mImpl->stopRecognition(modelHandle);
        }
        Return<int32_t> stopAllRecognitions() override { return mImpl->stopAllRecognitions(); }

        // Methods from V2_1::ISoundTriggerHw follow.
        Return<void> loadSoundModel_2_1(const V2_1::ISoundTriggerHw::SoundModel& soundModel,
                                        const sp<V2_1::ISoundTriggerHwCallback>& callback,
                                        int32_t cookie, loadSoundModel_2_1_cb _hidl_cb) override {
            return mImpl->loadSoundModel_2_1(soundModel, callback, cookie, _hidl_cb);
        }
        Return<void> loadPhraseSoundModel_2_1(
            const V2_1::ISoundTriggerHw::PhraseSoundModel& soundModel,
            const sp<V2_1::ISoundTriggerHwCallback>& callback, int32_t cookie,
            loadPhraseSoundModel_2_1_cb _hidl_cb) override {
            return mImpl->loadPhraseSoundModel_2_1(soundModel, callback, cookie, _hidl_cb);
        }
        Return<int32_t> startRecognition_2_1(int32_t modelHandle,
                                             const V2_1::ISoundTriggerHw::RecognitionConfig& config,
                                             const sp<V2_1::ISoundTriggerHwCallback>& /*callback*/,
                                             int32_t /*cookie*/) override {
            return mImpl->startRecognition_2_1(modelHandle, config);
        }

       private:
        sp<SoundTriggerHw> mImpl;
    };

    class SoundModelClient_2_1 : public SoundModelClient {
       public:
        SoundModelClient_2_1(uint32_t id, V2_1::ISoundTriggerHwCallback::CallbackCookie cookie,
                             sp<V2_1::ISoundTriggerHwCallback> callback)
            : SoundModelClient(id, cookie), mCallback(callback) {}

        void recognitionCallback(struct sound_trigger_recognition_event* halEvent) override;
        void soundModelCallback(struct sound_trigger_model_event* halEvent) override;

       private:
        sp<V2_1::ISoundTriggerHwCallback> mCallback;
    };
};

extern "C" ISoundTriggerHw* HIDL_FETCH_ISoundTriggerHw(const char* name);

}  // namespace implementation
}  // namespace V2_1
}  // namespace soundtrigger
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SOUNDTRIGGER_V2_1_SOUNDTRIGGERHW_H
