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

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/log.h>
#include <hidlmemory/mapping.h>
#include <utility>

using android::hardware::hidl_memory;
using android::hidl::allocator::V1_0::IAllocator;
using android::hidl::memory::V1_0::IMemory;

namespace android {
namespace hardware {
namespace soundtrigger {
namespace V2_2 {
namespace implementation {

/**
 * According to the HIDL C++ Users Guide: client and server implementations
 * should never directly refer to anything other than the interface header
 * generated from the HIDL definition file (ie. ISoundTriggerHw.hal), so
 * this V2_2 implementation copies the V2_0 and V2_1 implementations and
 * then adds the new V2_2 implementation.
 */

// Begin V2_0 implementation, copied from
// hardware/interfaces/soundtrigger/2.0/default/SoundTriggerHalImpl.cpp

// static
void soundModelCallback_(struct sound_trigger_model_event* halEvent, void* cookie) {
    if (halEvent == NULL) {
        ALOGW("soundModelCallback called with NULL event");
        return;
    }
    sp<SoundTriggerHw::SoundModelClient> client =
        wp<SoundTriggerHw::SoundModelClient>(static_cast<SoundTriggerHw::SoundModelClient*>(cookie))
            .promote();
    if (client == 0) {
        ALOGW("soundModelCallback called on stale client");
        return;
    }
    if (halEvent->model != client->getHalHandle()) {
        ALOGW("soundModelCallback call with wrong handle %d on client with handle %d",
              (int)halEvent->model, (int)client->getHalHandle());
        return;
    }

    client->soundModelCallback(halEvent);
}

// static
void recognitionCallback_(struct sound_trigger_recognition_event* halEvent, void* cookie) {
    if (halEvent == NULL) {
        ALOGW("recognitionCallback call NULL event");
        return;
    }
    sp<SoundTriggerHw::SoundModelClient> client =
        wp<SoundTriggerHw::SoundModelClient>(static_cast<SoundTriggerHw::SoundModelClient*>(cookie))
            .promote();
    if (client == 0) {
        ALOGW("recognitionCallback called on stale client");
        return;
    }

    client->recognitionCallback(halEvent);
}

Return<void> SoundTriggerHw::getProperties(ISoundTriggerHw::getProperties_cb _hidl_cb) {
    ALOGV("getProperties() mHwDevice %p", mHwDevice);
    int ret;
    struct sound_trigger_properties halProperties;
    ISoundTriggerHw::Properties properties;

    if (mHwDevice == NULL) {
        ret = -ENODEV;
        goto exit;
    }

    ret = mHwDevice->get_properties(mHwDevice, &halProperties);

    convertPropertiesFromHal(&properties, &halProperties);

    ALOGV("getProperties implementor %s recognitionModes %08x", properties.implementor.c_str(),
          properties.recognitionModes);

exit:
    _hidl_cb(ret, properties);
    return Void();
}

int SoundTriggerHw::doLoadSoundModel(const V2_0::ISoundTriggerHw::SoundModel& soundModel,
                                     sp<SoundTriggerHw::SoundModelClient> client) {
    int32_t ret = 0;
    struct sound_trigger_sound_model* halSoundModel;

    ALOGV("doLoadSoundModel() data size %zu", soundModel.data.size());

    if (mHwDevice == NULL) {
        ret = -ENODEV;
        goto exit;
    }

    halSoundModel = convertSoundModelToHal(&soundModel);
    if (halSoundModel == NULL) {
        ret = -EINVAL;
        goto exit;
    }

    sound_model_handle_t halHandle;
    ret = mHwDevice->load_sound_model(mHwDevice, halSoundModel, soundModelCallback_, client.get(),
                                      &halHandle);

    free(halSoundModel);

    if (ret != 0) {
        goto exit;
    }

    client->setHalHandle(halHandle);
    {
        AutoMutex lock(mLock);
        mClients.add(client->getId(), client);
    }

exit:
    return ret;
}

Return<void> SoundTriggerHw::loadSoundModel(const V2_0::ISoundTriggerHw::SoundModel& soundModel,
                                            const sp<V2_0::ISoundTriggerHwCallback>& callback,
                                            V2_0::ISoundTriggerHwCallback::CallbackCookie cookie,
                                            ISoundTriggerHw::loadSoundModel_cb _hidl_cb) {
    sp<SoundTriggerHw::SoundModelClient> client =
        new SoundModelClient_2_0(nextUniqueModelId(), cookie, callback);
    _hidl_cb(doLoadSoundModel(soundModel, client), client->getId());
    return Void();
}

Return<void> SoundTriggerHw::loadPhraseSoundModel(
    const V2_0::ISoundTriggerHw::PhraseSoundModel& soundModel,
    const sp<V2_0::ISoundTriggerHwCallback>& callback,
    V2_0::ISoundTriggerHwCallback::CallbackCookie cookie,
    ISoundTriggerHw::loadPhraseSoundModel_cb _hidl_cb) {
    sp<SoundTriggerHw::SoundModelClient> client =
        new SoundModelClient_2_0(nextUniqueModelId(), cookie, callback);
    _hidl_cb(doLoadSoundModel((const V2_0::ISoundTriggerHw::SoundModel&)soundModel, client),
             client->getId());
    return Void();
}

Return<int32_t> SoundTriggerHw::unloadSoundModel(int32_t modelHandle) {
    int32_t ret;
    sp<SoundTriggerHw::SoundModelClient> client;

    if (mHwDevice == NULL) {
        ret = -ENODEV;
        goto exit;
    }

    {
        AutoMutex lock(mLock);
        client = mClients.valueFor(modelHandle);
        if (client == 0) {
            ret = -ENOSYS;
            goto exit;
        }
    }

    ret = mHwDevice->unload_sound_model(mHwDevice, client->getHalHandle());

    mClients.removeItem(modelHandle);

exit:
    return ret;
}

Return<int32_t> SoundTriggerHw::startRecognition(
    int32_t modelHandle, const V2_0::ISoundTriggerHw::RecognitionConfig& config,
    const sp<V2_0::ISoundTriggerHwCallback>& /* callback */, int32_t /* cookie */) {
    int32_t ret;
    sp<SoundTriggerHw::SoundModelClient> client;
    struct sound_trigger_recognition_config* halConfig;

    if (mHwDevice == NULL) {
        ret = -ENODEV;
        goto exit;
    }

    {
        AutoMutex lock(mLock);
        client = mClients.valueFor(modelHandle);
        if (client == 0) {
            ret = -ENOSYS;
            goto exit;
        }
    }

    halConfig =
        convertRecognitionConfigToHal((const V2_0::ISoundTriggerHw::RecognitionConfig*)&config);

    if (halConfig == NULL) {
        ret = -EINVAL;
        goto exit;
    }
    ret = mHwDevice->start_recognition(mHwDevice, client->getHalHandle(), halConfig,
                                       recognitionCallback_, client.get());

    free(halConfig);

exit:
    return ret;
}

Return<int32_t> SoundTriggerHw::stopRecognition(int32_t modelHandle) {
    int32_t ret;
    sp<SoundTriggerHw::SoundModelClient> client;
    if (mHwDevice == NULL) {
        ret = -ENODEV;
        goto exit;
    }

    {
        AutoMutex lock(mLock);
        client = mClients.valueFor(modelHandle);
        if (client == 0) {
            ret = -ENOSYS;
            goto exit;
        }
    }

    ret = mHwDevice->stop_recognition(mHwDevice, client->getHalHandle());

exit:
    return ret;
}

Return<int32_t> SoundTriggerHw::stopAllRecognitions() {
    int32_t ret;
    if (mHwDevice == NULL) {
        ret = -ENODEV;
        goto exit;
    }

    if (mHwDevice->common.version >= SOUND_TRIGGER_DEVICE_API_VERSION_1_1 &&
        mHwDevice->stop_all_recognitions) {
        ret = mHwDevice->stop_all_recognitions(mHwDevice);
    } else {
        ret = -ENOSYS;
    }
exit:
    return ret;
}

SoundTriggerHw::SoundTriggerHw() : mModuleName("primary"), mHwDevice(NULL), mNextModelId(1) {}

void SoundTriggerHw::onFirstRef() {
    const hw_module_t* mod;
    int rc;

    rc = hw_get_module_by_class(SOUND_TRIGGER_HARDWARE_MODULE_ID, mModuleName, &mod);
    if (rc != 0) {
        ALOGE("couldn't load sound trigger module %s.%s (%s)", SOUND_TRIGGER_HARDWARE_MODULE_ID,
              mModuleName, strerror(-rc));
        return;
    }
    rc = sound_trigger_hw_device_open(mod, &mHwDevice);
    if (rc != 0) {
        ALOGE("couldn't open sound trigger hw device in %s.%s (%s)",
              SOUND_TRIGGER_HARDWARE_MODULE_ID, mModuleName, strerror(-rc));
        mHwDevice = NULL;
        return;
    }
    if (mHwDevice->common.version < SOUND_TRIGGER_DEVICE_API_VERSION_1_0 ||
        mHwDevice->common.version > SOUND_TRIGGER_DEVICE_API_VERSION_CURRENT) {
        ALOGE("wrong sound trigger hw device version %04x", mHwDevice->common.version);
        sound_trigger_hw_device_close(mHwDevice);
        mHwDevice = NULL;
        return;
    }

    ALOGI("onFirstRef() mModuleName %s mHwDevice %p", mModuleName, mHwDevice);
}

SoundTriggerHw::~SoundTriggerHw() {
    if (mHwDevice != NULL) {
        sound_trigger_hw_device_close(mHwDevice);
    }
}

uint32_t SoundTriggerHw::nextUniqueModelId() {
    uint32_t modelId = 0;
    {
        AutoMutex lock(mLock);
        do {
            modelId =
                atomic_fetch_add_explicit(&mNextModelId, (uint_fast32_t)1, memory_order_acq_rel);
        } while (mClients.valueFor(modelId) != 0 && modelId != 0);
    }
    LOG_ALWAYS_FATAL_IF(modelId == 0, "wrap around in sound model IDs, num loaded models %zu",
                        mClients.size());
    return modelId;
}

void SoundTriggerHw::convertUuidFromHal(Uuid* uuid, const sound_trigger_uuid_t* halUuid) {
    uuid->timeLow = halUuid->timeLow;
    uuid->timeMid = halUuid->timeMid;
    uuid->versionAndTimeHigh = halUuid->timeHiAndVersion;
    uuid->variantAndClockSeqHigh = halUuid->clockSeq;
    memcpy(&uuid->node[0], &halUuid->node[0], 6);
}

void SoundTriggerHw::convertUuidToHal(sound_trigger_uuid_t* halUuid, const Uuid* uuid) {
    halUuid->timeLow = uuid->timeLow;
    halUuid->timeMid = uuid->timeMid;
    halUuid->timeHiAndVersion = uuid->versionAndTimeHigh;
    halUuid->clockSeq = uuid->variantAndClockSeqHigh;
    memcpy(&halUuid->node[0], &uuid->node[0], 6);
}

void SoundTriggerHw::convertPropertiesFromHal(
    ISoundTriggerHw::Properties* properties, const struct sound_trigger_properties* halProperties) {
    properties->implementor = halProperties->implementor;
    properties->description = halProperties->description;
    properties->version = halProperties->version;
    convertUuidFromHal(&properties->uuid, &halProperties->uuid);
    properties->maxSoundModels = halProperties->max_sound_models;
    properties->maxKeyPhrases = halProperties->max_key_phrases;
    properties->maxUsers = halProperties->max_users;
    properties->recognitionModes = halProperties->recognition_modes;
    properties->captureTransition = halProperties->capture_transition;
    properties->maxBufferMs = halProperties->max_buffer_ms;
    properties->concurrentCapture = halProperties->concurrent_capture;
    properties->triggerInEvent = halProperties->trigger_in_event;
    properties->powerConsumptionMw = halProperties->power_consumption_mw;
}

void SoundTriggerHw::convertTriggerPhraseToHal(struct sound_trigger_phrase* halTriggerPhrase,
                                               const ISoundTriggerHw::Phrase* triggerPhrase) {
    halTriggerPhrase->id = triggerPhrase->id;
    halTriggerPhrase->recognition_mode = triggerPhrase->recognitionModes;
    unsigned int i;

    halTriggerPhrase->num_users =
        std::min((int)triggerPhrase->users.size(), SOUND_TRIGGER_MAX_USERS);
    for (i = 0; i < halTriggerPhrase->num_users; i++) {
        halTriggerPhrase->users[i] = triggerPhrase->users[i];
    }

    strlcpy(halTriggerPhrase->locale, triggerPhrase->locale.c_str(), SOUND_TRIGGER_MAX_LOCALE_LEN);
    strlcpy(halTriggerPhrase->text, triggerPhrase->text.c_str(), SOUND_TRIGGER_MAX_STRING_LEN);
}

struct sound_trigger_sound_model* SoundTriggerHw::convertSoundModelToHal(
    const V2_0::ISoundTriggerHw::SoundModel* soundModel) {
    struct sound_trigger_sound_model* halModel = NULL;
    if (soundModel->type == V2_0::SoundModelType::KEYPHRASE) {
        size_t allocSize =
            sizeof(struct sound_trigger_phrase_sound_model) + soundModel->data.size();
        struct sound_trigger_phrase_sound_model* halKeyPhraseModel =
            static_cast<struct sound_trigger_phrase_sound_model*>(malloc(allocSize));
        LOG_ALWAYS_FATAL_IF(halKeyPhraseModel == NULL,
                            "malloc failed for size %zu in convertSoundModelToHal PHRASE",
                            allocSize);

        const V2_0::ISoundTriggerHw::PhraseSoundModel* keyPhraseModel =
            reinterpret_cast<const V2_0::ISoundTriggerHw::PhraseSoundModel*>(soundModel);

        size_t i;
        for (i = 0; i < keyPhraseModel->phrases.size() && i < SOUND_TRIGGER_MAX_PHRASES; i++) {
            convertTriggerPhraseToHal(&halKeyPhraseModel->phrases[i], &keyPhraseModel->phrases[i]);
        }
        halKeyPhraseModel->num_phrases = (unsigned int)i;
        halModel = reinterpret_cast<struct sound_trigger_sound_model*>(halKeyPhraseModel);
        halModel->data_offset = sizeof(struct sound_trigger_phrase_sound_model);
    } else {
        size_t allocSize = sizeof(struct sound_trigger_sound_model) + soundModel->data.size();
        halModel = static_cast<struct sound_trigger_sound_model*>(malloc(allocSize));
        LOG_ALWAYS_FATAL_IF(halModel == NULL,
                            "malloc failed for size %zu in convertSoundModelToHal GENERIC",
                            allocSize);

        halModel->data_offset = sizeof(struct sound_trigger_sound_model);
    }
    halModel->type = (sound_trigger_sound_model_type_t)soundModel->type;
    convertUuidToHal(&halModel->uuid, &soundModel->uuid);
    convertUuidToHal(&halModel->vendor_uuid, &soundModel->vendorUuid);
    halModel->data_size = soundModel->data.size();
    uint8_t* dst = reinterpret_cast<uint8_t*>(halModel) + halModel->data_offset;
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&soundModel->data[0]);
    memcpy(dst, src, soundModel->data.size());

    return halModel;
}

void SoundTriggerHw::convertPhraseRecognitionExtraToHal(
    struct sound_trigger_phrase_recognition_extra* halExtra,
    const V2_0::PhraseRecognitionExtra* extra) {
    halExtra->id = extra->id;
    halExtra->recognition_modes = extra->recognitionModes;
    halExtra->confidence_level = extra->confidenceLevel;

    unsigned int i;
    for (i = 0; i < extra->levels.size() && i < SOUND_TRIGGER_MAX_USERS; i++) {
        halExtra->levels[i].user_id = extra->levels[i].userId;
        halExtra->levels[i].level = extra->levels[i].levelPercent;
    }
    halExtra->num_levels = i;
}

struct sound_trigger_recognition_config* SoundTriggerHw::convertRecognitionConfigToHal(
    const V2_0::ISoundTriggerHw::RecognitionConfig* config) {
    size_t allocSize = sizeof(struct sound_trigger_recognition_config) + config->data.size();
    struct sound_trigger_recognition_config* halConfig =
        static_cast<struct sound_trigger_recognition_config*>(malloc(allocSize));

    LOG_ALWAYS_FATAL_IF(halConfig == NULL,
                        "malloc failed for size %zu in convertRecognitionConfigToHal", allocSize);

    halConfig->capture_handle = (audio_io_handle_t)config->captureHandle;
    halConfig->capture_device = (audio_devices_t)config->captureDevice;
    halConfig->capture_requested = config->captureRequested;

    unsigned int i;
    for (i = 0; i < config->phrases.size() && i < SOUND_TRIGGER_MAX_PHRASES; i++) {
        convertPhraseRecognitionExtraToHal(&halConfig->phrases[i], &config->phrases[i]);
    }
    halConfig->num_phrases = i;

    halConfig->data_offset = sizeof(struct sound_trigger_recognition_config);
    halConfig->data_size = config->data.size();
    uint8_t* dst = reinterpret_cast<uint8_t*>(halConfig) + halConfig->data_offset;
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&config->data[0]);
    memcpy(dst, src, config->data.size());
    return halConfig;
}

// static
void SoundTriggerHw::convertSoundModelEventFromHal(
    V2_0::ISoundTriggerHwCallback::ModelEvent* event,
    const struct sound_trigger_model_event* halEvent) {
    event->status = (V2_0::ISoundTriggerHwCallback::SoundModelStatus)halEvent->status;
    // event->model to be remapped by called
    event->data.setToExternal(
        const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(halEvent)) + halEvent->data_offset,
        halEvent->data_size);
}

// static
void SoundTriggerHw::convertPhaseRecognitionEventFromHal(
    V2_0::ISoundTriggerHwCallback::PhraseRecognitionEvent* event,
    const struct sound_trigger_phrase_recognition_event* halEvent) {
    event->phraseExtras.resize(halEvent->num_phrases);
    for (unsigned int i = 0; i < halEvent->num_phrases; i++) {
        convertPhraseRecognitionExtraFromHal(&event->phraseExtras[i], &halEvent->phrase_extras[i]);
    }
    convertRecognitionEventFromHal(&event->common, &halEvent->common);
}

// static
void SoundTriggerHw::convertRecognitionEventFromHal(
    V2_0::ISoundTriggerHwCallback::RecognitionEvent* event,
    const struct sound_trigger_recognition_event* halEvent) {
    event->status = static_cast<V2_0::ISoundTriggerHwCallback::RecognitionStatus>(halEvent->status);
    event->type = static_cast<V2_0::SoundModelType>(halEvent->type);
    // event->model to be remapped by called
    event->captureAvailable = halEvent->capture_available;
    event->captureSession = halEvent->capture_session;
    event->captureDelayMs = halEvent->capture_delay_ms;
    event->capturePreambleMs = halEvent->capture_preamble_ms;
    event->triggerInData = halEvent->trigger_in_data;
    event->audioConfig.sampleRateHz = halEvent->audio_config.sample_rate;
    event->audioConfig.channelMask =
        (audio::common::V2_0::AudioChannelMask)halEvent->audio_config.channel_mask;
    event->audioConfig.format = (audio::common::V2_0::AudioFormat)halEvent->audio_config.format;
    event->data.setToExternal(
        const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(halEvent)) + halEvent->data_offset,
        halEvent->data_size);
}

// static
void SoundTriggerHw::convertPhraseRecognitionExtraFromHal(
    V2_0::PhraseRecognitionExtra* extra,
    const struct sound_trigger_phrase_recognition_extra* halExtra) {
    extra->id = halExtra->id;
    extra->recognitionModes = halExtra->recognition_modes;
    extra->confidenceLevel = halExtra->confidence_level;

    extra->levels.resize(halExtra->num_levels);
    for (unsigned int i = 0; i < halExtra->num_levels; i++) {
        extra->levels[i].userId = halExtra->levels[i].user_id;
        extra->levels[i].levelPercent = halExtra->levels[i].level;
    }
}

void SoundTriggerHw::SoundModelClient_2_0::recognitionCallback(
    struct sound_trigger_recognition_event* halEvent) {
    if (halEvent->type == SOUND_MODEL_TYPE_KEYPHRASE) {
        V2_0::ISoundTriggerHwCallback::PhraseRecognitionEvent event;
        convertPhaseRecognitionEventFromHal(
            &event, reinterpret_cast<sound_trigger_phrase_recognition_event*>(halEvent));
        event.common.model = mId;
        mCallback->phraseRecognitionCallback(event, mCookie);
    } else {
        V2_0::ISoundTriggerHwCallback::RecognitionEvent event;
        convertRecognitionEventFromHal(&event, halEvent);
        event.model = mId;
        mCallback->recognitionCallback(event, mCookie);
    }
}

void SoundTriggerHw::SoundModelClient_2_0::soundModelCallback(
    struct sound_trigger_model_event* halEvent) {
    V2_0::ISoundTriggerHwCallback::ModelEvent event;
    convertSoundModelEventFromHal(&event, halEvent);
    event.model = mId;
    mCallback->soundModelCallback(event, mCookie);
}

// Begin V2_1 implementation, copied from
// hardware/interfaces/soundtrigger/2.1/default/SoundTriggerHw.cpp

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
    int32_t modelHandle, const V2_1::ISoundTriggerHw::RecognitionConfig& config,
    const sp<V2_1::ISoundTriggerHwCallback>& callback, int32_t cookie) {
    // It is assumed that legacy data vector is empty, thus making copy is cheap.
    V2_0::ISoundTriggerHw::RecognitionConfig config_2_0(config.header);
    auto result = memoryAsVector(config.data, &config_2_0.data);
    return result.first ? startRecognition(modelHandle, config_2_0, callback, cookie)
                        : Return<int32_t>(-ENOMEM);
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

// Begin V2_2 implementation

Return<int32_t> SoundTriggerHw::getModelState(int32_t modelHandle) {
    sp<SoundModelClient> client;
    if (mHwDevice == NULL) {
        return -ENODEV;
    }

    {
        AutoMutex lock(mLock);
        client = mClients.valueFor(modelHandle);
        if (client == 0) {
            return -ENOSYS;
        }
    }

    if (mHwDevice->get_model_state == NULL) {
        ALOGE("Failed to get model state from device, no such method");
        return -ENODEV;
    }

    return mHwDevice->get_model_state(mHwDevice, client->getHalHandle());
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

ISoundTriggerHw* HIDL_FETCH_ISoundTriggerHw(const char* /* name */) {
    return new SoundTriggerHw();
}

}  // namespace implementation
}  // namespace V2_2
}  // namespace soundtrigger
}  // namespace hardware
}  // namespace android
