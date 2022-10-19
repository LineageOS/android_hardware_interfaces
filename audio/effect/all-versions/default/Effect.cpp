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

#include <memory.h>

#define LOG_TAG "EffectHAL"
#define ATRACE_TAG ATRACE_TAG_AUDIO

#include "Effect.h"
#include "common/all-versions/default/EffectMap.h"

#define ATRACE_TAG ATRACE_TAG_AUDIO
#include <HidlUtils.h>
#include <android/log.h>
#include <media/EffectsFactoryApi.h>
#include <mediautils/ScopedStatistics.h>
#include <util/EffectUtils.h>
#include <utils/Trace.h>

#include "VersionUtils.h"

namespace android {
namespace hardware {
namespace audio {
namespace effect {
namespace CPP_VERSION {
namespace implementation {

#if MAJOR_VERSION <= 6
using ::android::hardware::audio::common::COMMON_TYPES_CPP_VERSION::implementation::
        AudioChannelBitfield;
#endif
using ::android::hardware::audio::common::COMMON_TYPES_CPP_VERSION::implementation::HidlUtils;

namespace {

#define SCOPED_STATS()                                                       \
    ::android::mediautils::ScopedStatistics scopedStatistics {               \
        std::string("EffectHal::").append(__func__), mEffectHal->mStatistics \
    }

class ProcessThread : public Thread {
   public:
    // ProcessThread's lifespan never exceeds Effect's lifespan.
     ProcessThread(std::atomic<bool>* stop, effect_handle_t effect,
                   std::atomic<audio_buffer_t*>* inBuffer, std::atomic<audio_buffer_t*>* outBuffer,
                   Effect::StatusMQ* statusMQ, EventFlag* efGroup, Effect* effectHal)
         : Thread(false /*canCallJava*/),
           mStop(stop),
           mEffect(effect),
           mHasProcessReverse((*mEffect)->process_reverse != NULL),
           mInBuffer(inBuffer),
           mOutBuffer(outBuffer),
           mStatusMQ(statusMQ),
           mEfGroup(efGroup),
           mEffectHal(effectHal) {}
     virtual ~ProcessThread() {}

   private:
    std::atomic<bool>* mStop;
    effect_handle_t mEffect;
    bool mHasProcessReverse;
    std::atomic<audio_buffer_t*>* mInBuffer;
    std::atomic<audio_buffer_t*>* mOutBuffer;
    Effect::StatusMQ* mStatusMQ;
    EventFlag* mEfGroup;
    Effect* const mEffectHal;

    bool threadLoop() override;
};

bool ProcessThread::threadLoop() {
    // This implementation doesn't return control back to the Thread until it decides to stop,
    // as the Thread uses mutexes, and this can lead to priority inversion.
    while (!std::atomic_load_explicit(mStop, std::memory_order_acquire)) {
        uint32_t efState = 0;
        mEfGroup->wait(static_cast<uint32_t>(MessageQueueFlagBits::REQUEST_PROCESS_ALL), &efState);
        if (!(efState & static_cast<uint32_t>(MessageQueueFlagBits::REQUEST_PROCESS_ALL)) ||
            (efState & static_cast<uint32_t>(MessageQueueFlagBits::REQUEST_QUIT))) {
            continue;  // Nothing to do or time to quit.
        }
        Result retval = Result::OK;
        if (efState & static_cast<uint32_t>(MessageQueueFlagBits::REQUEST_PROCESS_REVERSE) &&
            !mHasProcessReverse) {
            retval = Result::NOT_SUPPORTED;
        }

        if (retval == Result::OK) {
            // affects both buffer pointers and their contents.
            std::atomic_thread_fence(std::memory_order_acquire);
            int32_t processResult;
            audio_buffer_t* inBuffer =
                std::atomic_load_explicit(mInBuffer, std::memory_order_relaxed);
            audio_buffer_t* outBuffer =
                std::atomic_load_explicit(mOutBuffer, std::memory_order_relaxed);
            if (inBuffer != nullptr && outBuffer != nullptr) {
                // Time this effect process
                SCOPED_STATS();

                if (efState & static_cast<uint32_t>(MessageQueueFlagBits::REQUEST_PROCESS)) {
                    processResult = (*mEffect)->process(mEffect, inBuffer, outBuffer);
                } else {
                    processResult = (*mEffect)->process_reverse(mEffect, inBuffer, outBuffer);
                }
                std::atomic_thread_fence(std::memory_order_release);
            } else {
                ALOGE("processing buffers were not set before calling 'process'");
                processResult = -ENODEV;
            }
            switch (processResult) {
                case 0:
                    retval = Result::OK;
                    break;
                case -ENODATA:
                    retval = Result::INVALID_STATE;
                    break;
                case -EINVAL:
                    retval = Result::INVALID_ARGUMENTS;
                    break;
                default:
                    retval = Result::NOT_INITIALIZED;
            }
        }
        if (!mStatusMQ->write(&retval)) {
            ALOGW("status message queue write failed");
        }
        mEfGroup->wake(static_cast<uint32_t>(MessageQueueFlagBits::DONE_PROCESSING));
    }

    return false;
}

}  // namespace

// static
const char* Effect::sContextResultOfCommand = "returned status";
const char* Effect::sContextCallToCommand = "error";
const char* Effect::sContextCallFunction = sContextCallToCommand;
const char* Effect::sContextConversion = "conversion";

Effect::Effect(bool isInput, effect_handle_t handle)
    : mIsInput(isInput), mHandle(handle), mEfGroup(nullptr), mStopProcessThread(false) {
    (void)mIsInput;  // prevent 'unused field' warnings in pre-V7 versions.
}

Effect::~Effect() {
    ATRACE_CALL();
    (void)close();
    if (mProcessThread.get()) {
        ATRACE_NAME("mProcessThread->join");
        status_t status = mProcessThread->join();
        ALOGE_IF(status, "processing thread exit error: %s", strerror(-status));
    }
    if (mEfGroup) {
        status_t status = EventFlag::deleteEventFlag(&mEfGroup);
        ALOGE_IF(status, "processing MQ event flag deletion error: %s", strerror(-status));
    }
    mInBuffer.clear();
    mOutBuffer.clear();
#if MAJOR_VERSION <= 5
    int status = EffectRelease(mHandle);
    ALOGW_IF(status, "Error releasing effect %p: %s", mHandle, strerror(-status));
#endif
    EffectMap::getInstance().remove(mHandle);
    mHandle = 0;
}

// static
template <typename T>
size_t Effect::alignedSizeIn(size_t s) {
    return (s + sizeof(T) - 1) / sizeof(T);
}

// static
template <typename T>
std::unique_ptr<uint8_t[]> Effect::hidlVecToHal(const hidl_vec<T>& vec, uint32_t* halDataSize) {
    // Due to bugs in HAL, they may attempt to write into the provided
    // input buffer. The original binder buffer is r/o, thus it is needed
    // to create a r/w version.
    *halDataSize = vec.size() * sizeof(T);
    std::unique_ptr<uint8_t[]> halData(new uint8_t[*halDataSize]);
    memcpy(&halData[0], &vec[0], *halDataSize);
    return halData;
}

#if MAJOR_VERSION <= 6

void Effect::effectAuxChannelsConfigFromHal(const channel_config_t& halConfig,
                                            EffectAuxChannelsConfig* config) {
    config->mainChannels = AudioChannelBitfield(halConfig.main_channels);
    config->auxChannels = AudioChannelBitfield(halConfig.aux_channels);
}

// static
void Effect::effectAuxChannelsConfigToHal(const EffectAuxChannelsConfig& config,
                                          channel_config_t* halConfig) {
    halConfig->main_channels = static_cast<audio_channel_mask_t>(config.mainChannels);
    halConfig->aux_channels = static_cast<audio_channel_mask_t>(config.auxChannels);
}

#else  // MAJOR_VERSION <= 6

void Effect::effectAuxChannelsConfigFromHal(const channel_config_t& halConfig,
                                            EffectAuxChannelsConfig* config) {
    (void)HidlUtils::audioChannelMaskFromHal(halConfig.main_channels, mIsInput,
                                             &config->mainChannels);
    (void)HidlUtils::audioChannelMaskFromHal(halConfig.aux_channels, mIsInput,
                                             &config->auxChannels);
}

// static
void Effect::effectAuxChannelsConfigToHal(const EffectAuxChannelsConfig& config,
                                          channel_config_t* halConfig) {
    (void)HidlUtils::audioChannelMaskToHal(config.mainChannels, &halConfig->main_channels);
    (void)HidlUtils::audioChannelMaskToHal(config.auxChannels, &halConfig->aux_channels);
}

#endif  // MAJOR_VERSION <= 6

// static
void Effect::effectOffloadParamToHal(const EffectOffloadParameter& offload,
                                     effect_offload_param_t* halOffload) {
    halOffload->isOffload = offload.isOffload;
    halOffload->ioHandle = offload.ioHandle;
}

// static
bool Effect::parameterToHal(uint32_t paramSize, const void* paramData, uint32_t valueSize,
                            const void** valueData, std::vector<uint8_t>* halParamBuffer) {
    constexpr size_t kMaxSize = EFFECT_PARAM_SIZE_MAX - sizeof(effect_param_t);
    if (paramSize > kMaxSize) {
        ALOGE("%s: Parameter size is too big: %" PRIu32, __func__, paramSize);
        return false;
    }
    size_t valueOffsetFromData = alignedSizeIn<uint32_t>(paramSize) * sizeof(uint32_t);
    if (valueOffsetFromData > kMaxSize) {
        ALOGE("%s: Aligned parameter size is too big: %zu", __func__, valueOffsetFromData);
        return false;
    }
    if (valueSize > kMaxSize - valueOffsetFromData) {
        ALOGE("%s: Value size is too big: %" PRIu32 ", max size is %zu", __func__, valueSize,
              kMaxSize - valueOffsetFromData);
        android_errorWriteLog(0x534e4554, "237291425");
        return false;
    }
    size_t halParamBufferSize = sizeof(effect_param_t) + valueOffsetFromData + valueSize;
    halParamBuffer->resize(halParamBufferSize, 0);
    effect_param_t* halParam = reinterpret_cast<effect_param_t*>(halParamBuffer->data());
    halParam->psize = paramSize;
    halParam->vsize = valueSize;
    memcpy(halParam->data, paramData, paramSize);
    if (valueData) {
        if (*valueData) {
            // Value data is provided.
            memcpy(halParam->data + valueOffsetFromData, *valueData, valueSize);
        } else {
            // The caller needs the pointer to the value data location.
            *valueData = halParam->data + valueOffsetFromData;
        }
    }
    return true;
}

Result Effect::analyzeCommandStatus(const char* commandName, const char* context, status_t status) {
    return analyzeStatus("command", commandName, context, status);
}

Result Effect::analyzeStatus(const char* funcName, const char* subFuncName,
                             const char* contextDescription, status_t status) {
    if (status != OK) {
        ALOGW("Effect %p %s %s %s: %s", mHandle, funcName, subFuncName, contextDescription,
              strerror(-status));
    }
    switch (status) {
        case OK:
            return Result::OK;
        case -EINVAL:
            return Result::INVALID_ARGUMENTS;
        case -ENODATA:
            return Result::INVALID_STATE;
        case -ENODEV:
            return Result::NOT_INITIALIZED;
        case -ENOMEM:
            return Result::RESULT_TOO_BIG;
        case -ENOSYS:
            return Result::NOT_SUPPORTED;
        default:
            return Result::INVALID_STATE;
    }
}

void Effect::getConfigImpl(int commandCode, const char* commandName, GetConfigCallback cb) {
    uint32_t halResultSize = sizeof(effect_config_t);
    effect_config_t halConfig{};
    status_t status =
        (*mHandle)->command(mHandle, commandCode, 0, NULL, &halResultSize, &halConfig);
    EffectConfig config;
    if (status == OK) {
        status = EffectUtils::effectConfigFromHal(halConfig, mIsInput, &config);
    }
    cb(analyzeCommandStatus(commandName, sContextCallToCommand, status), config);
}

Result Effect::getCurrentConfigImpl(uint32_t featureId, uint32_t configSize,
                                    GetCurrentConfigSuccessCallback onSuccess) {
    if (configSize > kMaxDataSize - sizeof(uint32_t)) {
        ALOGE("%s: Config size is too big: %" PRIu32, __func__, configSize);
        android_errorWriteLog(0x534e4554, "240266798");
        return Result::INVALID_ARGUMENTS;
    }
    uint32_t halCmd = featureId;
    std::vector<uint32_t> halResult(alignedSizeIn<uint32_t>(sizeof(uint32_t) + configSize), 0);
    uint32_t halResultSize = 0;
    return sendCommandReturningStatusAndData(
            EFFECT_CMD_GET_FEATURE_CONFIG, "GET_FEATURE_CONFIG", sizeof(uint32_t), &halCmd,
            &halResultSize, &halResult[0], sizeof(uint32_t), [&] { onSuccess(&halResult[1]); });
}

Result Effect::getParameterImpl(uint32_t paramSize, const void* paramData,
                                uint32_t requestValueSize, uint32_t replyValueSize,
                                GetParameterSuccessCallback onSuccess) {
    // As it is unknown what method HAL uses for copying the provided parameter data,
    // it is safer to make sure that input and output buffers do not overlap.
    std::vector<uint8_t> halCmdBuffer;
    if (!parameterToHal(paramSize, paramData, requestValueSize, nullptr, &halCmdBuffer)) {
        return Result::INVALID_ARGUMENTS;
    }
    const void* valueData = nullptr;
    std::vector<uint8_t> halParamBuffer;
    if (!parameterToHal(paramSize, paramData, replyValueSize, &valueData, &halParamBuffer)) {
        return Result::INVALID_ARGUMENTS;
    }
    uint32_t halParamBufferSize = halParamBuffer.size();

    return sendCommandReturningStatusAndData(
        EFFECT_CMD_GET_PARAM, "GET_PARAM", halCmdBuffer.size(), &halCmdBuffer[0],
        &halParamBufferSize, &halParamBuffer[0], sizeof(effect_param_t), [&] {
            effect_param_t* halParam = reinterpret_cast<effect_param_t*>(&halParamBuffer[0]);
            onSuccess(halParam->vsize, valueData);
        });
}

Result Effect::getSupportedConfigsImpl(uint32_t featureId, uint32_t maxConfigs, uint32_t configSize,
                                       GetSupportedConfigsSuccessCallback onSuccess) {
    if (maxConfigs != 0 && configSize > (kMaxDataSize - 2 * sizeof(uint32_t)) / maxConfigs) {
        ALOGE("%s: Config size is too big: %" PRIu32, __func__, configSize);
        return Result::INVALID_ARGUMENTS;
    }
    uint32_t halCmd[2] = {featureId, maxConfigs};
    uint32_t halResultSize = 2 * sizeof(uint32_t) + maxConfigs * configSize;
    std::vector<uint8_t> halResult(static_cast<size_t>(halResultSize), 0);
    return sendCommandReturningStatusAndData(
        EFFECT_CMD_GET_FEATURE_SUPPORTED_CONFIGS, "GET_FEATURE_SUPPORTED_CONFIGS", sizeof(halCmd),
        halCmd, &halResultSize, &halResult[0], 2 * sizeof(uint32_t), [&] {
            uint32_t* halResult32 = reinterpret_cast<uint32_t*>(&halResult[0]);
            uint32_t supportedConfigs = *(++halResult32);  // skip status field
            if (supportedConfigs > maxConfigs) supportedConfigs = maxConfigs;
            onSuccess(supportedConfigs, ++halResult32);
        });
}

Return<void> Effect::prepareForProcessing(prepareForProcessing_cb _hidl_cb) {
    status_t status;
    // Create message queue.
    if (mStatusMQ) {
        ALOGE("the client attempts to call prepareForProcessing_cb twice");
        _hidl_cb(Result::INVALID_STATE, StatusMQ::Descriptor());
        return Void();
    }
    std::unique_ptr<StatusMQ> tempStatusMQ(new StatusMQ(1, true /*EventFlag*/));
    if (!tempStatusMQ->isValid()) {
        ALOGE_IF(!tempStatusMQ->isValid(), "status MQ is invalid");
        _hidl_cb(Result::INVALID_ARGUMENTS, StatusMQ::Descriptor());
        return Void();
    }
    status = EventFlag::createEventFlag(tempStatusMQ->getEventFlagWord(), &mEfGroup);
    if (status != OK || !mEfGroup) {
        ALOGE("failed creating event flag for status MQ: %s", strerror(-status));
        _hidl_cb(Result::INVALID_ARGUMENTS, StatusMQ::Descriptor());
        return Void();
    }

    // Create and launch the thread.
    mProcessThread = new ProcessThread(&mStopProcessThread, mHandle, &mHalInBufferPtr,
                                       &mHalOutBufferPtr, tempStatusMQ.get(), mEfGroup, this);
    status = mProcessThread->run("effect", PRIORITY_URGENT_AUDIO);
    if (status != OK) {
        ALOGW("failed to start effect processing thread: %s", strerror(-status));
        _hidl_cb(Result::INVALID_ARGUMENTS, MQDescriptorSync<Result>());
        return Void();
    }

    mStatusMQ = std::move(tempStatusMQ);
    _hidl_cb(Result::OK, *mStatusMQ->getDesc());
    return Void();
}

Return<Result> Effect::setProcessBuffers(const AudioBuffer& inBuffer,
                                         const AudioBuffer& outBuffer) {
    AudioBufferManager& manager = AudioBufferManager::getInstance();
    sp<AudioBufferWrapper> tempInBuffer, tempOutBuffer;
    if (!manager.wrap(inBuffer, &tempInBuffer)) {
        ALOGE("Could not map memory of the input buffer");
        return Result::INVALID_ARGUMENTS;
    }
    if (!manager.wrap(outBuffer, &tempOutBuffer)) {
        ALOGE("Could not map memory of the output buffer");
        return Result::INVALID_ARGUMENTS;
    }
    mInBuffer = tempInBuffer;
    mOutBuffer = tempOutBuffer;
    // The processing thread only reads these pointers after waking up by an event flag,
    // so it's OK to update the pair non-atomically.
    mHalInBufferPtr.store(mInBuffer->getHalBuffer(), std::memory_order_release);
    mHalOutBufferPtr.store(mOutBuffer->getHalBuffer(), std::memory_order_release);
    return Result::OK;
}

Result Effect::sendCommand(int commandCode, const char* commandName) {
    return sendCommand(commandCode, commandName, 0, NULL);
}

Result Effect::sendCommand(int commandCode, const char* commandName, uint32_t size, void* data) {
    status_t status = (*mHandle)->command(mHandle, commandCode, size, data, 0, NULL);
    return analyzeCommandStatus(commandName, sContextCallToCommand, status);
}

Result Effect::sendCommandReturningData(int commandCode, const char* commandName,
                                        uint32_t* replySize, void* replyData) {
    return sendCommandReturningData(commandCode, commandName, 0, NULL, replySize, replyData);
}

Result Effect::sendCommandReturningData(int commandCode, const char* commandName, uint32_t size,
                                        void* data, uint32_t* replySize, void* replyData) {
    uint32_t expectedReplySize = *replySize;
    status_t status = (*mHandle)->command(mHandle, commandCode, size, data, replySize, replyData);
    if (status == OK && *replySize != expectedReplySize) {
        status = -ENODATA;
    }
    return analyzeCommandStatus(commandName, sContextCallToCommand, status);
}

Result Effect::sendCommandReturningStatus(int commandCode, const char* commandName) {
    return sendCommandReturningStatus(commandCode, commandName, 0, NULL);
}

Result Effect::sendCommandReturningStatus(int commandCode, const char* commandName, uint32_t size,
                                          void* data) {
    uint32_t replyCmdStatus;
    uint32_t replySize = sizeof(uint32_t);
    return sendCommandReturningStatusAndData(commandCode, commandName, size, data, &replySize,
                                             &replyCmdStatus, replySize, [] {});
}

Result Effect::sendCommandReturningStatusAndData(int commandCode, const char* commandName,
                                                 uint32_t size, void* data, uint32_t* replySize,
                                                 void* replyData, uint32_t minReplySize,
                                                 CommandSuccessCallback onSuccess) {
    status_t status = (*mHandle)->command(mHandle, commandCode, size, data, replySize, replyData);
    Result retval;
    if (status == OK && minReplySize >= sizeof(uint32_t) && *replySize >= minReplySize) {
        uint32_t commandStatus = *reinterpret_cast<uint32_t*>(replyData);
        retval = analyzeCommandStatus(commandName, sContextResultOfCommand, commandStatus);
        if (commandStatus == OK) {
            onSuccess();
        }
    } else {
        retval = analyzeCommandStatus(commandName, sContextCallToCommand, status);
    }
    return retval;
}

Result Effect::setConfigImpl(int commandCode, const char* commandName, const EffectConfig& config,
                             const sp<IEffectBufferProviderCallback>& inputBufferProvider,
                             const sp<IEffectBufferProviderCallback>& outputBufferProvider) {
    effect_config_t halConfig;
    EffectUtils::effectConfigToHal(config, &halConfig);
    if (inputBufferProvider != 0) {
        LOG_FATAL("Using input buffer provider is not supported");
    }
    if (outputBufferProvider != 0) {
        LOG_FATAL("Using output buffer provider is not supported");
    }
    return sendCommandReturningStatus(commandCode, commandName, sizeof(effect_config_t),
                                      &halConfig);
}

Result Effect::setParameterImpl(uint32_t paramSize, const void* paramData, uint32_t valueSize,
                                const void* valueData) {
    std::vector<uint8_t> halParamBuffer;
    if (!parameterToHal(paramSize, paramData, valueSize, &valueData, &halParamBuffer)) {
        return Result::INVALID_ARGUMENTS;
    }
    return sendCommandReturningStatus(EFFECT_CMD_SET_PARAM, "SET_PARAM", halParamBuffer.size(),
                                      &halParamBuffer[0]);
}

// Methods from ::android::hardware::audio::effect::CPP_VERSION::IEffect follow.
Return<Result> Effect::init() {
    return sendCommandReturningStatus(EFFECT_CMD_INIT, "INIT");
}

Return<Result> Effect::setConfig(const EffectConfig& config,
                                 const sp<IEffectBufferProviderCallback>& inputBufferProvider,
                                 const sp<IEffectBufferProviderCallback>& outputBufferProvider) {
    return setConfigImpl(EFFECT_CMD_SET_CONFIG, "SET_CONFIG", config, inputBufferProvider,
                         outputBufferProvider);
}

Return<Result> Effect::reset() {
    return sendCommand(EFFECT_CMD_RESET, "RESET");
}

Return<Result> Effect::enable() {
    return sendCommandReturningStatus(EFFECT_CMD_ENABLE, "ENABLE");
}

Return<Result> Effect::disable() {
    return sendCommandReturningStatus(EFFECT_CMD_DISABLE, "DISABLE");
}

Return<Result> Effect::setAudioSource(
#if MAJOR_VERSION <= 6
        AudioSource source
#else
        const AudioSource& source
#endif
) {
    audio_source_t halSource;
    if (status_t status = HidlUtils::audioSourceToHal(source, &halSource); status == NO_ERROR) {
        uint32_t halSourceParam = static_cast<uint32_t>(halSource);
        return sendCommand(EFFECT_CMD_SET_AUDIO_SOURCE, "SET_AUDIO_SOURCE", sizeof(uint32_t),
                           &halSourceParam);
    } else {
        return analyzeStatus(__func__, "audioSourceToHal", sContextConversion, status);
    }
}

#if MAJOR_VERSION <= 6

Return<Result> Effect::setDevice(AudioDeviceBitfield device) {
    uint32_t halDevice = static_cast<uint32_t>(device);
    return sendCommand(EFFECT_CMD_SET_DEVICE, "SET_DEVICE", sizeof(uint32_t), &halDevice);
}

Return<Result> Effect::setInputDevice(AudioDeviceBitfield device) {
    uint32_t halDevice = static_cast<uint32_t>(device);
    return sendCommand(EFFECT_CMD_SET_INPUT_DEVICE, "SET_INPUT_DEVICE", sizeof(uint32_t),
                       &halDevice);
}

#else  // MAJOR_VERSION <= 6

Return<Result> Effect::setDevice(const DeviceAddress& device) {
    audio_devices_t halDevice;
    char halDeviceAddress[AUDIO_DEVICE_MAX_ADDRESS_LEN];
    if (status_t status = HidlUtils::deviceAddressToHal(device, &halDevice, halDeviceAddress);
        status == NO_ERROR) {
        uint32_t halDeviceParam = static_cast<uint32_t>(halDevice);
        return sendCommand(EFFECT_CMD_SET_DEVICE, "SET_DEVICE", sizeof(uint32_t), &halDeviceParam);
    } else {
        return analyzeStatus(__func__, "deviceAddressToHal", sContextConversion, status);
    }
}

Return<Result> Effect::setInputDevice(const DeviceAddress& device) {
    audio_devices_t halDevice;
    char halDeviceAddress[AUDIO_DEVICE_MAX_ADDRESS_LEN];
    if (status_t status = HidlUtils::deviceAddressToHal(device, &halDevice, halDeviceAddress);
        status == NO_ERROR) {
        uint32_t halDeviceParam = static_cast<uint32_t>(halDevice);
        return sendCommand(EFFECT_CMD_SET_INPUT_DEVICE, "SET_INPUT_DEVICE", sizeof(uint32_t),
                           &halDeviceParam);
    } else {
        return analyzeStatus(__func__, "deviceAddressToHal", sContextConversion, status);
    }
}

#endif  // MAJOR_VERSION <= 6

Return<void> Effect::setAndGetVolume(const hidl_vec<uint32_t>& volumes,
                                     setAndGetVolume_cb _hidl_cb) {
    uint32_t halDataSize;
    std::unique_ptr<uint8_t[]> halData = hidlVecToHal(volumes, &halDataSize);
    uint32_t halResultSize = halDataSize;
    std::vector<uint32_t> halResult(volumes.size(), 0);
    Result retval = sendCommandReturningData(EFFECT_CMD_SET_VOLUME, "SET_VOLUME", halDataSize,
                                             &halData[0], &halResultSize, &halResult[0]);
    hidl_vec<uint32_t> result;
    if (retval == Result::OK) {
        result.setToExternal(&halResult[0], halResultSize);
    }
    _hidl_cb(retval, result);
    return Void();
}

Return<Result> Effect::volumeChangeNotification(const hidl_vec<uint32_t>& volumes) {
    uint32_t halDataSize;
    std::unique_ptr<uint8_t[]> halData = hidlVecToHal(volumes, &halDataSize);
    return sendCommand(EFFECT_CMD_SET_VOLUME, "SET_VOLUME", halDataSize, &halData[0]);
}

Return<Result> Effect::setAudioMode(AudioMode mode) {
    uint32_t halMode = static_cast<uint32_t>(mode);
    return sendCommand(EFFECT_CMD_SET_AUDIO_MODE, "SET_AUDIO_MODE", sizeof(uint32_t), &halMode);
}

Return<Result> Effect::setConfigReverse(
    const EffectConfig& config, const sp<IEffectBufferProviderCallback>& inputBufferProvider,
    const sp<IEffectBufferProviderCallback>& outputBufferProvider) {
    return setConfigImpl(EFFECT_CMD_SET_CONFIG_REVERSE, "SET_CONFIG_REVERSE", config,
                         inputBufferProvider, outputBufferProvider);
}

Return<void> Effect::getConfig(getConfig_cb _hidl_cb) {
    getConfigImpl(EFFECT_CMD_GET_CONFIG, "GET_CONFIG", _hidl_cb);
    return Void();
}

Return<void> Effect::getConfigReverse(getConfigReverse_cb _hidl_cb) {
    getConfigImpl(EFFECT_CMD_GET_CONFIG_REVERSE, "GET_CONFIG_REVERSE", _hidl_cb);
    return Void();
}

Return<void> Effect::getSupportedAuxChannelsConfigs(uint32_t maxConfigs,
                                                    getSupportedAuxChannelsConfigs_cb _hidl_cb) {
    hidl_vec<EffectAuxChannelsConfig> result;
    Result retval = getSupportedConfigsImpl(
        EFFECT_FEATURE_AUX_CHANNELS, maxConfigs, sizeof(channel_config_t),
        [&](uint32_t supportedConfigs, void* configsData) {
            result.resize(supportedConfigs);
            channel_config_t* config = reinterpret_cast<channel_config_t*>(configsData);
            for (size_t i = 0; i < result.size(); ++i) {
                effectAuxChannelsConfigFromHal(*config++, &result[i]);
            }
        });
    _hidl_cb(retval, result);
    return Void();
}

Return<void> Effect::getAuxChannelsConfig(getAuxChannelsConfig_cb _hidl_cb) {
    EffectAuxChannelsConfig result;
    Result retval = getCurrentConfigImpl(
        EFFECT_FEATURE_AUX_CHANNELS, sizeof(channel_config_t), [&](void* configData) {
            effectAuxChannelsConfigFromHal(*reinterpret_cast<channel_config_t*>(configData),
                                           &result);
        });
    _hidl_cb(retval, result);
    return Void();
}

Return<Result> Effect::setAuxChannelsConfig(const EffectAuxChannelsConfig& config) {
    std::vector<uint32_t> halCmd(
            alignedSizeIn<uint32_t>(sizeof(uint32_t) + sizeof(channel_config_t)), 0);
    halCmd[0] = EFFECT_FEATURE_AUX_CHANNELS;
    effectAuxChannelsConfigToHal(config, reinterpret_cast<channel_config_t*>(&halCmd[1]));
    return sendCommandReturningStatus(EFFECT_CMD_SET_FEATURE_CONFIG,
                                      "SET_FEATURE_CONFIG AUX_CHANNELS", halCmd.size(), &halCmd[0]);
}

Return<Result> Effect::offload(const EffectOffloadParameter& param) {
    effect_offload_param_t halParam;
    effectOffloadParamToHal(param, &halParam);
    return sendCommandReturningStatus(EFFECT_CMD_OFFLOAD, "OFFLOAD", sizeof(effect_offload_param_t),
                                      &halParam);
}

Return<void> Effect::getDescriptor(getDescriptor_cb _hidl_cb) {
    effect_descriptor_t halDescriptor;
    memset(&halDescriptor, 0, sizeof(effect_descriptor_t));
    status_t status = (*mHandle)->get_descriptor(mHandle, &halDescriptor);
    EffectDescriptor descriptor;
    if (status == OK) {
        status = EffectUtils::effectDescriptorFromHal(halDescriptor, &descriptor);
    }
    _hidl_cb(analyzeStatus("get_descriptor", "", sContextCallFunction, status), descriptor);
    return Void();
}

Return<void> Effect::command(uint32_t commandId, const hidl_vec<uint8_t>& data,
                             uint32_t resultMaxSize, command_cb _hidl_cb) {
    uint32_t halDataSize;
    std::unique_ptr<uint8_t[]> halData = hidlVecToHal(data, &halDataSize);
    uint32_t halResultSize = resultMaxSize;
    std::unique_ptr<uint8_t[]> halResult(new uint8_t[halResultSize]);
    memset(&halResult[0], 0, halResultSize);

    void* dataPtr = halDataSize > 0 ? &halData[0] : NULL;
    void* resultPtr = halResultSize > 0 ? &halResult[0] : NULL;
    status_t status = BAD_VALUE;
    switch (commandId) {
        case 'gtid':  // retrieve the tid, used for spatializer priority boost
            if (halDataSize == 0 && resultMaxSize == sizeof(int32_t)) {
                auto ptid = (int32_t*)resultPtr;
                ptid[0] = mProcessThread ? mProcessThread->getTid() : -1;
                status = OK;
                break;  // we have handled 'gtid' here.
            }
            [[fallthrough]];  // allow 'gtid' overload (checked halDataSize and resultMaxSize).
        default:
            status = (*mHandle)->command(mHandle, commandId, halDataSize, dataPtr, &halResultSize,
                                         resultPtr);
            break;
    }
    hidl_vec<uint8_t> result;
    if (status == OK && resultPtr != NULL) {
        result.setToExternal(&halResult[0], halResultSize);
    }
    _hidl_cb(status, result);
    return Void();
}

Return<Result> Effect::setParameter(const hidl_vec<uint8_t>& parameter,
                                    const hidl_vec<uint8_t>& value) {
    return setParameterImpl(parameter.size(), &parameter[0], value.size(), &value[0]);
}

Return<void> Effect::getParameter(const hidl_vec<uint8_t>& parameter, uint32_t valueMaxSize,
                                  getParameter_cb _hidl_cb) {
    hidl_vec<uint8_t> value;
    Result retval = getParameterImpl(
        parameter.size(), &parameter[0], valueMaxSize,
        [&](uint32_t valueSize, const void* valueData) {
            value.setToExternal(reinterpret_cast<uint8_t*>(const_cast<void*>(valueData)),
                                valueSize);
        });
    _hidl_cb(retval, value);
    return Void();
}

Return<void> Effect::getSupportedConfigsForFeature(uint32_t featureId, uint32_t maxConfigs,
                                                   uint32_t configSize,
                                                   getSupportedConfigsForFeature_cb _hidl_cb) {
    uint32_t configCount = 0;
    hidl_vec<uint8_t> result;
    Result retval = getSupportedConfigsImpl(featureId, maxConfigs, configSize,
                                            [&](uint32_t supportedConfigs, void* configsData) {
                                                configCount = supportedConfigs;
                                                result.resize(configCount * configSize);
                                                memcpy(&result[0], configsData, result.size());
                                            });
    _hidl_cb(retval, configCount, result);
    return Void();
}

Return<void> Effect::getCurrentConfigForFeature(uint32_t featureId, uint32_t configSize,
                                                getCurrentConfigForFeature_cb _hidl_cb) {
    hidl_vec<uint8_t> result;
    Result retval = getCurrentConfigImpl(featureId, configSize, [&](void* configData) {
        result.resize(configSize);
        memcpy(&result[0], configData, result.size());
    });
    _hidl_cb(retval, result);
    return Void();
}

Return<Result> Effect::setCurrentConfigForFeature(uint32_t featureId,
                                                  const hidl_vec<uint8_t>& configData) {
    std::vector<uint32_t> halCmd(alignedSizeIn<uint32_t>(sizeof(uint32_t) + configData.size()), 0);
    halCmd[0] = featureId;
    memcpy(&halCmd[1], &configData[0], configData.size());
    return sendCommandReturningStatus(EFFECT_CMD_SET_FEATURE_CONFIG, "SET_FEATURE_CONFIG",
                                      halCmd.size(), &halCmd[0]);
}

Return<Result> Effect::close() {
    if (mStopProcessThread.load(std::memory_order_relaxed)) {  // only this thread modifies
        return Result::INVALID_STATE;
    }
    mStopProcessThread.store(true, std::memory_order_release);
    if (mEfGroup) {
        mEfGroup->wake(static_cast<uint32_t>(MessageQueueFlagBits::REQUEST_QUIT));
    }
#if MAJOR_VERSION <= 5
    return Result::OK;
#elif MAJOR_VERSION >= 6
    // No need to join the processing thread, it is part of the API contract that the client
    // must finish processing before closing the effect.
    Result retval =
            analyzeStatus("EffectRelease", "", sContextCallFunction, EffectRelease(mHandle));
    EffectMap::getInstance().remove(mHandle);
    return retval;
#endif
}

Return<void> Effect::debug(const hidl_handle& fd, const hidl_vec<hidl_string>& /* options */) {
    if (fd.getNativeHandle() != nullptr && fd->numFds == 1) {
        uint32_t cmdData = fd->data[0];
        (void)sendCommand(EFFECT_CMD_DUMP, "DUMP", sizeof(cmdData), &cmdData);
        const std::string s = mStatistics->dump();
        if (s.size() != 0) write(cmdData, s.c_str(), s.size());
    }
    return Void();
}

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace effect
}  // namespace audio
}  // namespace hardware
}  // namespace android
