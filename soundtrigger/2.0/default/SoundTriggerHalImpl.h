/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_SOUNDTRIGGER_V2_0_IMPLEMENTATION_H
#define ANDROID_HARDWARE_SOUNDTRIGGER_V2_0_IMPLEMENTATION_H

#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHwCallback.h>
#include <hardware/sound_trigger.h>
#include <hidl/Status.h>
#include <stdatomic.h>
#include <system/sound_trigger.h>
#include <utils/KeyedVector.h>
#include <utils/threads.h>

namespace android {
namespace hardware {
namespace soundtrigger {
namespace V2_0 {
namespace implementation {

using ::android::hardware::audio::common::V2_0::Uuid;
using ::android::hardware::soundtrigger::V2_0::ISoundTriggerHwCallback;

class SoundTriggerHalImpl : public RefBase {
   public:
    SoundTriggerHalImpl();
    ISoundTriggerHw* getInterface() { return new TrampolineSoundTriggerHw_2_0(this); }

   protected:
    class SoundModelClient : public RefBase {
       public:
        SoundModelClient(uint32_t id, ISoundTriggerHwCallback::CallbackCookie cookie)
            : mId(id), mCookie(cookie) {}
        virtual ~SoundModelClient() {}

        uint32_t getId() const { return mId; }
        sound_model_handle_t getHalHandle() const { return mHalHandle; }
        void setHalHandle(sound_model_handle_t handle) { mHalHandle = handle; }

        virtual void recognitionCallback(struct sound_trigger_recognition_event* halEvent) = 0;
        virtual void soundModelCallback(struct sound_trigger_model_event* halEvent) = 0;

       protected:
        const uint32_t mId;
        sound_model_handle_t mHalHandle;
        ISoundTriggerHwCallback::CallbackCookie mCookie;
    };

    static void convertPhaseRecognitionEventFromHal(
        ISoundTriggerHwCallback::PhraseRecognitionEvent* event,
        const struct sound_trigger_phrase_recognition_event* halEvent);
    static void convertRecognitionEventFromHal(
        ISoundTriggerHwCallback::RecognitionEvent* event,
        const struct sound_trigger_recognition_event* halEvent);
    static void convertSoundModelEventFromHal(ISoundTriggerHwCallback::ModelEvent* event,
                                              const struct sound_trigger_model_event* halEvent);

    virtual ~SoundTriggerHalImpl();

    Return<void> getProperties(ISoundTriggerHw::getProperties_cb _hidl_cb);
    Return<void> loadSoundModel(const ISoundTriggerHw::SoundModel& soundModel,
                                const sp<ISoundTriggerHwCallback>& callback,
                                ISoundTriggerHwCallback::CallbackCookie cookie,
                                ISoundTriggerHw::loadSoundModel_cb _hidl_cb);
    Return<void> loadPhraseSoundModel(const ISoundTriggerHw::PhraseSoundModel& soundModel,
                                      const sp<ISoundTriggerHwCallback>& callback,
                                      ISoundTriggerHwCallback::CallbackCookie cookie,
                                      ISoundTriggerHw::loadPhraseSoundModel_cb _hidl_cb);
    Return<int32_t> unloadSoundModel(SoundModelHandle modelHandle);
    Return<int32_t> startRecognition(SoundModelHandle modelHandle,
                                     const ISoundTriggerHw::RecognitionConfig& config);
    Return<int32_t> stopRecognition(SoundModelHandle modelHandle);
    Return<int32_t> stopAllRecognitions();

    uint32_t nextUniqueModelId();
    int doLoadSoundModel(const ISoundTriggerHw::SoundModel& soundModel,
                         sp<SoundModelClient> client);

    // RefBase
    void onFirstRef() override;

   private:
    struct TrampolineSoundTriggerHw_2_0 : public ISoundTriggerHw {
        explicit TrampolineSoundTriggerHw_2_0(sp<SoundTriggerHalImpl> impl) : mImpl(impl) {}

        // Methods from ::android::hardware::soundtrigger::V2_0::ISoundTriggerHw follow.
        Return<void> getProperties(getProperties_cb _hidl_cb) override {
            return mImpl->getProperties(_hidl_cb);
        }
        Return<void> loadSoundModel(const ISoundTriggerHw::SoundModel& soundModel,
                                    const sp<ISoundTriggerHwCallback>& callback,
                                    ISoundTriggerHwCallback::CallbackCookie cookie,
                                    loadSoundModel_cb _hidl_cb) override {
            return mImpl->loadSoundModel(soundModel, callback, cookie, _hidl_cb);
        }
        Return<void> loadPhraseSoundModel(const ISoundTriggerHw::PhraseSoundModel& soundModel,
                                          const sp<ISoundTriggerHwCallback>& callback,
                                          ISoundTriggerHwCallback::CallbackCookie cookie,
                                          loadPhraseSoundModel_cb _hidl_cb) override {
            return mImpl->loadPhraseSoundModel(soundModel, callback, cookie, _hidl_cb);
        }
        Return<int32_t> unloadSoundModel(SoundModelHandle modelHandle) override {
            return mImpl->unloadSoundModel(modelHandle);
        }
        Return<int32_t> startRecognition(
            SoundModelHandle modelHandle, const ISoundTriggerHw::RecognitionConfig& config,
            const sp<ISoundTriggerHwCallback>& /*callback*/,
            ISoundTriggerHwCallback::CallbackCookie /*cookie*/) override {
            return mImpl->startRecognition(modelHandle, config);
        }
        Return<int32_t> stopRecognition(SoundModelHandle modelHandle) override {
            return mImpl->stopRecognition(modelHandle);
        }
        Return<int32_t> stopAllRecognitions() override { return mImpl->stopAllRecognitions(); }

       private:
        sp<SoundTriggerHalImpl> mImpl;
    };

    class SoundModelClient_2_0 : public SoundModelClient {
       public:
        SoundModelClient_2_0(uint32_t id, ISoundTriggerHwCallback::CallbackCookie cookie,
                             sp<ISoundTriggerHwCallback> callback)
            : SoundModelClient(id, cookie), mCallback(callback) {}

        void recognitionCallback(struct sound_trigger_recognition_event* halEvent) override;
        void soundModelCallback(struct sound_trigger_model_event* halEvent) override;

       private:
        sp<ISoundTriggerHwCallback> mCallback;
    };

    void convertUuidFromHal(Uuid* uuid, const sound_trigger_uuid_t* halUuid);
    void convertUuidToHal(sound_trigger_uuid_t* halUuid, const Uuid* uuid);
    void convertPropertiesFromHal(ISoundTriggerHw::Properties* properties,
                                  const struct sound_trigger_properties* halProperties);
    void convertTriggerPhraseToHal(struct sound_trigger_phrase* halTriggerPhrase,
                                   const ISoundTriggerHw::Phrase* triggerPhrase);
    // returned HAL sound model must be freed by caller
    struct sound_trigger_sound_model* convertSoundModelToHal(
        const ISoundTriggerHw::SoundModel* soundModel);
    void convertPhraseRecognitionExtraToHal(struct sound_trigger_phrase_recognition_extra* halExtra,
                                            const PhraseRecognitionExtra* extra);
    // returned recognition config must be freed by caller
    struct sound_trigger_recognition_config* convertRecognitionConfigToHal(
        const ISoundTriggerHw::RecognitionConfig* config);

    static void convertPhraseRecognitionExtraFromHal(
        PhraseRecognitionExtra* extra,
        const struct sound_trigger_phrase_recognition_extra* halExtra);

    static void soundModelCallback(struct sound_trigger_model_event* halEvent, void* cookie);
    static void recognitionCallback(struct sound_trigger_recognition_event* halEvent, void* cookie);

    const char* mModuleName;
    struct sound_trigger_hw_device* mHwDevice;
    volatile atomic_uint_fast32_t mNextModelId;
    DefaultKeyedVector<int32_t, sp<SoundModelClient> > mClients;
    Mutex mLock;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace soundtrigger
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SOUNDTRIGGER_V2_0_IMPLEMENTATION_H
