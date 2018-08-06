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

#include <utility>

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/log.h>
#include <hidlmemory/mapping.h>

using android::hardware::hidl_memory;
using android::hidl::allocator::V1_0::IAllocator;
using android::hidl::memory::V1_0::IMemory;

namespace android {
namespace hardware {
namespace soundtrigger {
namespace V2_1 {
namespace implementation {

namespace {

// Backs up by the vector with the contents of shared memory.
// It is assumed that the passed hidl_vector is empty, so it's
// not cleared if the memory is a null object.
// The caller needs to keep the returned sp<IMemory> as long as
// the data is needed.
std::pair<bool, sp<IMemory>> memoryAsVector(const hidl_memory& m, hidl_vec<uint8_t>* vec) {
    sp<IMemory> memory;
    if (m.size() == 0) {
        return std::make_pair(true, memory);
    }
    memory = mapMemory(m);
    if (memory != nullptr) {
        memory->read();
        vec->setToExternal(static_cast<uint8_t*>(static_cast<void*>(memory->getPointer())),
                           memory->getSize());
        return std::make_pair(true, memory);
    }
    ALOGE("%s: Could not map HIDL memory to IMemory", __func__);
    return std::make_pair(false, memory);
}

// Moves the data from the vector into allocated shared memory,
// emptying the vector.
// It is assumed that the passed hidl_memory is a null object, so it's
// not reset if the vector is empty.
// The caller needs to keep the returned sp<IMemory> as long as
// the data is needed.
std::pair<bool, sp<IMemory>> moveVectorToMemory(hidl_vec<uint8_t>* v, hidl_memory* mem) {
    sp<IMemory> memory;
    if (v->size() == 0) {
        return std::make_pair(true, memory);
    }
    sp<IAllocator> ashmem = IAllocator::getService("ashmem");
    if (ashmem == 0) {
        ALOGE("Failed to retrieve ashmem allocator service");
        return std::make_pair(false, memory);
    }
    bool success = false;
    Return<void> r = ashmem->allocate(v->size(), [&](bool s, const hidl_memory& m) {
        success = s;
        if (success) *mem = m;
    });
    if (r.isOk() && success) {
        memory = hardware::mapMemory(*mem);
        if (memory != 0) {
            memory->update();
            memcpy(memory->getPointer(), v->data(), v->size());
            memory->commit();
            v->resize(0);
            return std::make_pair(true, memory);
        } else {
            ALOGE("Failed to map allocated ashmem");
        }
    } else {
        ALOGE("Failed to allocate %llu bytes from ashmem", (unsigned long long)v->size());
    }
    return std::make_pair(false, memory);
}

}  // namespace

Return<void> SoundTriggerHw::loadSoundModel_2_1(
    const V2_1::ISoundTriggerHw::SoundModel& soundModel,
    const sp<V2_1::ISoundTriggerHwCallback>& callback, int32_t cookie,
    V2_1::ISoundTriggerHw::loadSoundModel_2_1_cb _hidl_cb) {
    // It is assumed that legacy data vector is empty, thus making copy is cheap.
    V2_0::ISoundTriggerHw::SoundModel soundModel_2_0(soundModel.header);
    auto result = memoryAsVector(soundModel.data, &soundModel_2_0.data);
    if (result.first) {
        sp<SoundModelClient> client =
            new SoundModelClient_2_1(nextUniqueModelId(), cookie, callback);
        _hidl_cb(doLoadSoundModel(soundModel_2_0, client), client->getId());
        return Void();
    }
    _hidl_cb(-ENOMEM, 0);
    return Void();
}

Return<void> SoundTriggerHw::loadPhraseSoundModel_2_1(
    const V2_1::ISoundTriggerHw::PhraseSoundModel& soundModel,
    const sp<V2_1::ISoundTriggerHwCallback>& callback, int32_t cookie,
    V2_1::ISoundTriggerHw::loadPhraseSoundModel_2_1_cb _hidl_cb) {
    V2_0::ISoundTriggerHw::PhraseSoundModel soundModel_2_0;
    // It is assumed that legacy data vector is empty, thus making copy is cheap.
    soundModel_2_0.common = soundModel.common.header;
    // Avoid copying phrases data.
    soundModel_2_0.phrases.setToExternal(
        const_cast<V2_0::ISoundTriggerHw::Phrase*>(soundModel.phrases.data()),
        soundModel.phrases.size());
    auto result = memoryAsVector(soundModel.common.data, &soundModel_2_0.common.data);
    if (result.first) {
        sp<SoundModelClient> client =
            new SoundModelClient_2_1(nextUniqueModelId(), cookie, callback);
        _hidl_cb(doLoadSoundModel((const V2_0::ISoundTriggerHw::SoundModel&)soundModel_2_0, client),
                 client->getId());
        return Void();
    }
    _hidl_cb(-ENOMEM, 0);
    return Void();
}

Return<int32_t> SoundTriggerHw::startRecognition_2_1(
    int32_t modelHandle, const V2_1::ISoundTriggerHw::RecognitionConfig& config) {
    // It is assumed that legacy data vector is empty, thus making copy is cheap.
    V2_0::ISoundTriggerHw::RecognitionConfig config_2_0(config.header);
    auto result = memoryAsVector(config.data, &config_2_0.data);
    return result.first ? startRecognition(modelHandle, config_2_0) : Return<int32_t>(-ENOMEM);
}

void SoundTriggerHw::SoundModelClient_2_1::recognitionCallback(
    struct sound_trigger_recognition_event* halEvent) {
    if (halEvent->type == SOUND_MODEL_TYPE_KEYPHRASE) {
        V2_0::ISoundTriggerHwCallback::PhraseRecognitionEvent event_2_0;
        convertPhaseRecognitionEventFromHal(
            &event_2_0, reinterpret_cast<sound_trigger_phrase_recognition_event*>(halEvent));
        event_2_0.common.model = mId;
        V2_1::ISoundTriggerHwCallback::PhraseRecognitionEvent event;
        event.phraseExtras.setToExternal(event_2_0.phraseExtras.data(),
                                         event_2_0.phraseExtras.size());
        auto result = moveVectorToMemory(&event_2_0.common.data, &event.common.data);
        if (result.first) {
            // The data vector is now empty, thus copying is cheap.
            event.common.header = event_2_0.common;
            mCallback->phraseRecognitionCallback_2_1(event, mCookie);
        }
    } else {
        V2_1::ISoundTriggerHwCallback::RecognitionEvent event;
        convertRecognitionEventFromHal(&event.header, halEvent);
        event.header.model = mId;
        auto result = moveVectorToMemory(&event.header.data, &event.data);
        if (result.first) {
            mCallback->recognitionCallback_2_1(event, mCookie);
        }
    }
}

void SoundTriggerHw::SoundModelClient_2_1::soundModelCallback(
    struct sound_trigger_model_event* halEvent) {
    V2_1::ISoundTriggerHwCallback::ModelEvent event;
    convertSoundModelEventFromHal(&event.header, halEvent);
    event.header.model = mId;
    auto result = moveVectorToMemory(&event.header.data, &event.data);
    if (result.first) {
        mCallback->soundModelCallback_2_1(event, mCookie);
    }
}

ISoundTriggerHw* HIDL_FETCH_ISoundTriggerHw(const char* /* name */) {
    return (new SoundTriggerHw())->getInterface();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace soundtrigger
}  // namespace hardware
}  // namespace android
