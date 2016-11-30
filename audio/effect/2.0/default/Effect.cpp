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

#include <memory>
#include <memory.h>

#define LOG_TAG "EffectHAL"
#include <media/EffectsFactoryApi.h>
#include <android/log.h>

#include "Conversions.h"
#include "Effect.h"
#include "EffectMap.h"

namespace android {
namespace hardware {
namespace audio {
namespace effect {
namespace V2_0 {
namespace implementation {

using ::android::hardware::audio::common::V2_0::AudioChannelMask;
using ::android::hardware::audio::common::V2_0::AudioFormat;

// static
const char *Effect::sContextResultOfCommand = "returned status";
const char *Effect::sContextCallToCommand = "error";
const char *Effect::sContextCallFunction = sContextCallToCommand;

Effect::Effect(effect_handle_t handle) : mHandle(handle) {
}

Effect::~Effect() {
    int status = EffectRelease(mHandle);
    ALOGW_IF(status, "Error releasing effect %p: %s", mHandle, strerror(-status));
    EffectMap::getInstance().remove(mHandle);
    mHandle = 0;
}

// static
template<typename T> size_t Effect::alignedSizeIn(size_t s) {
    return (s + sizeof(T) - 1) / sizeof(T);
}

// static
template<typename T> void Effect::hidlVecToHal(
        const hidl_vec<T>& vec, uint32_t* halDataSize, void** halData) {
    *halDataSize = static_cast<T>(vec.size() * sizeof(T));
    *halData = static_cast<void*>(const_cast<T*>(&vec[0]));
}

// static
void Effect::effectAuxChannelsConfigFromHal(
        const channel_config_t& halConfig, EffectAuxChannelsConfig* config) {
    config->mainChannels = AudioChannelMask(halConfig.main_channels);
    config->auxChannels = AudioChannelMask(halConfig.aux_channels);
}

// static
void Effect::effectAuxChannelsConfigToHal(
        const EffectAuxChannelsConfig& config, channel_config_t* halConfig) {
    halConfig->main_channels = static_cast<audio_channel_mask_t>(config.mainChannels);
    halConfig->aux_channels = static_cast<audio_channel_mask_t>(config.auxChannels);
}

// static
void Effect::effectBufferConfigFromHal(
        const buffer_config_t& halConfig, EffectBufferConfig* config) {
    // TODO(mnaganov): Use FMQ instead of AudioBuffer.
    (void)halConfig.buffer.frameCount;
    (void)halConfig.buffer.raw;
    config->samplingRateHz = halConfig.samplingRate;
    config->channels = AudioChannelMask(halConfig.channels);
    config->format = AudioFormat(halConfig.format);
    config->accessMode = EffectBufferAccess(halConfig.accessMode);
    config->mask = EffectConfigParameters(halConfig.mask);
}

// static
void Effect::effectBufferConfigToHal(const EffectBufferConfig& config, buffer_config_t* halConfig) {
    // TODO(mnaganov): Use FMQ instead of AudioBuffer.
    halConfig->buffer.frameCount = 0;
    halConfig->buffer.raw = NULL;
    halConfig->samplingRate = config.samplingRateHz;
    halConfig->channels = static_cast<uint32_t>(config.channels);
    // TODO(mnaganov): As the calling code does not use BP for now, implement later.
    halConfig->bufferProvider.cookie = NULL;
    halConfig->bufferProvider.getBuffer = NULL;
    halConfig->bufferProvider.releaseBuffer = NULL;
    halConfig->format = static_cast<uint8_t>(config.format);
    halConfig->accessMode = static_cast<uint8_t>(config.accessMode);
    halConfig->mask = static_cast<uint8_t>(config.mask);
}

// static
void Effect::effectConfigFromHal(const effect_config_t& halConfig, EffectConfig* config) {
    effectBufferConfigFromHal(halConfig.inputCfg, &config->inputCfg);
    effectBufferConfigFromHal(halConfig.outputCfg, &config->outputCfg);
}

// static
void Effect::effectConfigToHal(const EffectConfig& config, effect_config_t* halConfig) {
    effectBufferConfigToHal(config.inputCfg, &halConfig->inputCfg);
    effectBufferConfigToHal(config.outputCfg, &halConfig->outputCfg);
}

// static
void Effect::effectOffloadParamToHal(
        const EffectOffloadParameter& offload, effect_offload_param_t* halOffload) {
    halOffload->isOffload = offload.isOffload;
    halOffload->ioHandle = offload.ioHandle;
}

// static
std::vector<uint8_t> Effect::parameterToHal(
        uint32_t paramSize,
        const void* paramData,
        uint32_t valueSize,
        const void** valueData) {
    size_t valueOffsetFromData = alignedSizeIn<uint32_t>(paramSize) * sizeof(uint32_t);
    size_t halParamBufferSize = sizeof(effect_param_t) + valueOffsetFromData + valueSize;
    std::vector<uint8_t> halParamBuffer(halParamBufferSize, 0);
    effect_param_t *halParam = reinterpret_cast<effect_param_t*>(&halParamBuffer[0]);
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
    return halParamBuffer;
}

Result Effect::analyzeCommandStatus(const char* commandName, const char* context, status_t status) {
    return analyzeStatus("command", commandName, context, status);
}

Result Effect::analyzeStatus(
        const char* funcName,
        const char* subFuncName,
        const char* contextDescription,
        status_t status) {
    if (status != OK) {
        ALOGW("Effect %p %s %s %s: %s",
                mHandle, funcName, subFuncName, contextDescription, strerror(-status));
    }
    switch (status) {
        case OK: return Result::OK;
        case -EINVAL: return Result::INVALID_ARGUMENTS;
        case -ENODATA: return Result::INVALID_STATE;
        case -ENODEV: return Result::NOT_INITIALIZED;
        case -ENOMEM: return Result::RESULT_TOO_BIG;
        case -ENOSYS: return Result::NOT_SUPPORTED;
        default: return Result::INVALID_STATE;
    }
}

void Effect::getConfigImpl(int commandCode, const char* commandName, GetConfigCallback cb) {
    uint32_t halResultSize = sizeof(effect_config_t);
    effect_config_t halConfig;
    status_t status = (*mHandle)->command(
            mHandle, commandCode, 0, NULL, &halResultSize, &halConfig);
    EffectConfig config;
    if (status == OK) {
        effectConfigFromHal(halConfig, &config);
    }
    cb(analyzeCommandStatus(commandName, sContextCallToCommand, status), config);
}

Result Effect::getCurrentConfigImpl(
        uint32_t featureId, uint32_t configSize, GetCurrentConfigSuccessCallback onSuccess) {
    uint32_t halCmd = featureId;
    uint32_t halResult[alignedSizeIn<uint32_t>(sizeof(uint32_t) + configSize)];
    memset(halResult, 0, sizeof(halResult));
    uint32_t halResultSize = 0;
    return sendCommandReturningStatusAndData(
            EFFECT_CMD_GET_FEATURE_CONFIG, "GET_FEATURE_CONFIG",
            sizeof(uint32_t), &halCmd,
            &halResultSize, halResult,
            sizeof(uint32_t),
            [&]{ onSuccess(&halResult[1]); });
}

Result Effect::getParameterImpl(
        uint32_t paramSize,
        const void* paramData,
        uint32_t valueSize,
        GetParameterSuccessCallback onSuccess) {
    // As it is unknown what method HAL uses for copying the provided parameter data,
    // it is safer to make sure that input and output buffers do not overlap.
    std::vector<uint8_t> halCmdBuffer =
            parameterToHal(paramSize, paramData, valueSize, nullptr);
    const void *valueData = nullptr;
    std::vector<uint8_t> halParamBuffer =
            parameterToHal(paramSize, paramData, valueSize, &valueData);
    uint32_t halParamBufferSize = halParamBuffer.size();

    return sendCommandReturningStatusAndData(
            EFFECT_CMD_GET_PARAM, "GET_PARAM",
            halCmdBuffer.size(), &halCmdBuffer[0],
            &halParamBufferSize, &halParamBuffer[0],
            sizeof(effect_param_t),
            [&]{
                effect_param_t *halParam = reinterpret_cast<effect_param_t*>(&halParamBuffer[0]);
                onSuccess(halParam->vsize, valueData);
            });
}

Result Effect::getSupportedConfigsImpl(
        uint32_t featureId,
        uint32_t maxConfigs,
        uint32_t configSize,
        GetSupportedConfigsSuccessCallback onSuccess) {
    uint32_t halCmd[2] = { featureId, maxConfigs };
    uint32_t halResultSize = 2 * sizeof(uint32_t) + maxConfigs * sizeof(configSize);
    uint8_t halResult[halResultSize];
    memset(&halResult[0], 0, halResultSize);
    return sendCommandReturningStatusAndData(
            EFFECT_CMD_GET_FEATURE_SUPPORTED_CONFIGS, "GET_FEATURE_SUPPORTED_CONFIGS",
            sizeof(halCmd), halCmd,
            &halResultSize, &halResult[0],
            2 * sizeof(uint32_t),
            [&]{
                uint32_t *halResult32 = reinterpret_cast<uint32_t*>(&halResult[0]);
                uint32_t supportedConfigs = *(++halResult32); // skip status field
                if (supportedConfigs > maxConfigs) supportedConfigs = maxConfigs;
                onSuccess(supportedConfigs, ++halResult32);
            });
}

void Effect::processImpl(
        ProcessFunction process,
        const char* funcName,
        const AudioBuffer& inBuffer,
        uint32_t outFrameSize,
        ProcessCallback cb) {
    audio_buffer_t halInBuffer;
    halInBuffer.frameCount = inBuffer.frameCount;
    halInBuffer.u8 = const_cast<uint8_t*>(&inBuffer.data[0]);
    audio_buffer_t halOutBuffer;
    halOutBuffer.frameCount = halInBuffer.frameCount;
    // TODO(mnaganov): Consider stashing the buffer to avoid reallocating it every time.
    std::unique_ptr<uint8_t[]> halOutBufferData(
            new uint8_t[halOutBuffer.frameCount * outFrameSize]);
    halOutBuffer.u8 = &halOutBufferData[0];
    status_t status = process(mHandle, &halInBuffer, &halOutBuffer);
    Result retval = analyzeStatus(funcName, "", sContextCallFunction, status);
    AudioBuffer outBuffer;
    if (status == OK) {
        outBuffer.frameCount = halOutBuffer.frameCount;
        outBuffer.data.setToExternal(halOutBuffer.u8, halOutBuffer.frameCount * outFrameSize);
    } else {
        outBuffer.frameCount = 0;
    }
    cb(retval, outBuffer);
}

Result Effect::sendCommand(int commandCode, const char* commandName) {
    return sendCommand(commandCode, commandName, 0, NULL);
}

Result Effect::sendCommand(
        int commandCode, const char* commandName, uint32_t size, void* data) {
    status_t status = (*mHandle)->command(mHandle, commandCode, size, data, 0, NULL);
    return analyzeCommandStatus(commandName, sContextCallToCommand, status);
}

Result Effect::sendCommandReturningData(
        int commandCode, const char* commandName,
        uint32_t* replySize, void* replyData) {
    return sendCommandReturningData(commandCode, commandName, 0, NULL, replySize, replyData);
}

Result Effect::sendCommandReturningData(
        int commandCode, const char* commandName,
        uint32_t size, void* data,
        uint32_t* replySize, void* replyData) {
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

Result Effect::sendCommandReturningStatus(
        int commandCode, const char* commandName, uint32_t size, void* data) {
    uint32_t replyCmdStatus;
    uint32_t replySize = sizeof(uint32_t);
    return sendCommandReturningStatusAndData(
            commandCode, commandName, size, data, &replySize, &replyCmdStatus, replySize, []{});
}

Result Effect::sendCommandReturningStatusAndData(
        int commandCode, const char* commandName,
        uint32_t size, void* data,
        uint32_t* replySize, void* replyData,
        uint32_t minReplySize,
        CommandSuccessCallback onSuccess) {
    status_t status =
            (*mHandle)->command(mHandle, commandCode, size, data, replySize, replyData);
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

Result Effect::setConfigImpl(
        int commandCode, const char* commandName,
        const EffectConfig& config,
        const sp<IEffectBufferProviderCallback>& inputBufferProvider,
        const sp<IEffectBufferProviderCallback>& outputBufferProvider) {
    effect_config_t halConfig;
    effectConfigToHal(config, &halConfig);
    if (inputBufferProvider != 0) {
        LOG_FATAL("Using input buffer provider is not supported");
    }
    if (outputBufferProvider != 0) {
        LOG_FATAL("Using output buffer provider is not supported");
    }
    return sendCommandReturningStatus(
            commandCode, commandName, sizeof(effect_config_t), &halConfig);
}


Result Effect::setParameterImpl(
        uint32_t paramSize, const void* paramData, uint32_t valueSize, const void* valueData) {
    std::vector<uint8_t> halParamBuffer = parameterToHal(
            paramSize, paramData, valueSize, &valueData);
    return sendCommandReturningStatus(
            EFFECT_CMD_SET_PARAM, "SET_PARAM", halParamBuffer.size(), &halParamBuffer[0]);
}

// Methods from ::android::hardware::audio::effect::V2_0::IEffect follow.
Return<Result> Effect::init()  {
    return sendCommandReturningStatus(EFFECT_CMD_INIT, "INIT");
}

Return<Result> Effect::setConfig(
        const EffectConfig& config,
        const sp<IEffectBufferProviderCallback>& inputBufferProvider,
        const sp<IEffectBufferProviderCallback>& outputBufferProvider)  {
    return setConfigImpl(
            EFFECT_CMD_SET_CONFIG, "SET_CONFIG", config, inputBufferProvider, outputBufferProvider);
}

Return<Result> Effect::reset()  {
    return sendCommand(EFFECT_CMD_RESET, "RESET");
}

Return<Result> Effect::enable()  {
    return sendCommandReturningStatus(EFFECT_CMD_ENABLE, "ENABLE");
}

Return<Result> Effect::disable()  {
    return sendCommandReturningStatus(EFFECT_CMD_DISABLE, "DISABLE");
}

Return<Result> Effect::setDevice(AudioDevice device)  {
    uint32_t halDevice = static_cast<uint32_t>(device);
    return sendCommand(EFFECT_CMD_SET_DEVICE, "SET_DEVICE", sizeof(uint32_t), &halDevice);
}

Return<void> Effect::setAndGetVolume(
        const hidl_vec<uint32_t>& volumes, setAndGetVolume_cb _hidl_cb)  {
    uint32_t halDataSize;
    void *halData;
    hidlVecToHal(volumes, &halDataSize, &halData);
    uint32_t halResultSize = halDataSize;
    uint32_t halResult[volumes.size()];
    Result retval = sendCommandReturningData(
            EFFECT_CMD_SET_VOLUME, "SET_VOLUME", halDataSize, halData, &halResultSize, halResult);
    hidl_vec<uint32_t> result;
    if (retval == Result::OK) {
        result.setToExternal(&halResult[0], halResultSize);
    }
    _hidl_cb(retval, result);
    return Void();
}

Return<Result> Effect::setAudioMode(AudioMode mode)  {
    uint32_t halMode = static_cast<uint32_t>(mode);
    return sendCommand(
            EFFECT_CMD_SET_AUDIO_MODE, "SET_AUDIO_MODE", sizeof(uint32_t), &halMode);
}

Return<Result> Effect::setConfigReverse(
        const EffectConfig& config,
        const sp<IEffectBufferProviderCallback>& inputBufferProvider,
        const sp<IEffectBufferProviderCallback>& outputBufferProvider)  {
    return setConfigImpl(EFFECT_CMD_SET_CONFIG_REVERSE, "SET_CONFIG_REVERSE",
            config, inputBufferProvider, outputBufferProvider);
}

Return<Result> Effect::setInputDevice(AudioDevice device)  {
    uint32_t halDevice = static_cast<uint32_t>(device);
    return sendCommand(
            EFFECT_CMD_SET_INPUT_DEVICE, "SET_INPUT_DEVICE", sizeof(uint32_t), &halDevice);
}

Return<void> Effect::getConfig(getConfig_cb _hidl_cb)  {
    getConfigImpl(EFFECT_CMD_GET_CONFIG, "GET_CONFIG", _hidl_cb);
    return Void();
}

Return<void> Effect::getConfigReverse(getConfigReverse_cb _hidl_cb)  {
    getConfigImpl(EFFECT_CMD_GET_CONFIG_REVERSE, "GET_CONFIG_REVERSE", _hidl_cb);
    return Void();
}

Return<void> Effect::getSupportedAuxChannelsConfigs(
        uint32_t maxConfigs, getSupportedAuxChannelsConfigs_cb _hidl_cb)  {
    hidl_vec<EffectAuxChannelsConfig> result;
    Result retval = getSupportedConfigsImpl(
            EFFECT_FEATURE_AUX_CHANNELS,
            maxConfigs,
            sizeof(channel_config_t),
            [&] (uint32_t supportedConfigs, void* configsData) {
                result.resize(supportedConfigs);
                channel_config_t *config = reinterpret_cast<channel_config_t*>(configsData);
                for (size_t i = 0; i < result.size(); ++i) {
                    effectAuxChannelsConfigFromHal(*config++, &result[i]);
                }
            });
    _hidl_cb(retval, result);
    return Void();
}

Return<void> Effect::getAuxChannelsConfig(getAuxChannelsConfig_cb _hidl_cb)  {
    uint32_t halCmd = EFFECT_FEATURE_AUX_CHANNELS;
    uint32_t halResult[alignedSizeIn<uint32_t>(sizeof(uint32_t) + sizeof(channel_config_t))];
    memset(halResult, 0, sizeof(halResult));
    uint32_t halResultSize = 0;
    EffectAuxChannelsConfig result;
    Result retval = getCurrentConfigImpl(
            EFFECT_FEATURE_AUX_CHANNELS,
            sizeof(channel_config_t),
            [&] (void* configData) {
                effectAuxChannelsConfigFromHal(
                        *reinterpret_cast<channel_config_t*>(configData), &result);
            });
    _hidl_cb(retval, result);
    return Void();
}

Return<Result> Effect::setAuxChannelsConfig(const EffectAuxChannelsConfig& config)  {
    uint32_t halCmd[alignedSizeIn<uint32_t>(sizeof(uint32_t) + sizeof(channel_config_t))];
    halCmd[0] = EFFECT_FEATURE_AUX_CHANNELS;
    effectAuxChannelsConfigToHal(config, reinterpret_cast<channel_config_t*>(&halCmd[1]));
    return sendCommandReturningStatus(EFFECT_CMD_SET_FEATURE_CONFIG,
            "SET_FEATURE_CONFIG AUX_CHANNELS", sizeof(halCmd), halCmd);
}

Return<Result> Effect::setAudioSource(AudioSource source)  {
    uint32_t halSource = static_cast<uint32_t>(source);
    return sendCommand(
            EFFECT_CMD_SET_AUDIO_SOURCE, "SET_AUDIO_SOURCE", sizeof(uint32_t), &halSource);
}

Return<Result> Effect::offload(const EffectOffloadParameter& param)  {
    effect_offload_param_t halParam;
    effectOffloadParamToHal(param, &halParam);
    return sendCommandReturningStatus(
            EFFECT_CMD_OFFLOAD, "OFFLOAD", sizeof(effect_offload_param_t), &halParam);
}

Return<void> Effect::getDescriptor(getDescriptor_cb _hidl_cb)  {
    effect_descriptor_t halDescriptor;
    memset(&halDescriptor, 0, sizeof(effect_descriptor_t));
    status_t status = (*mHandle)->get_descriptor(mHandle, &halDescriptor);
    EffectDescriptor descriptor;
    if (status == OK) {
        effectDescriptorFromHal(halDescriptor, &descriptor);
    }
    _hidl_cb(analyzeStatus("get_descriptor", "", sContextCallFunction, status), descriptor);
    return Void();
}

Return<void> Effect::process(
        const AudioBuffer& inBuffer, uint32_t outFrameSize, process_cb _hidl_cb)  {
    processImpl((*mHandle)->process, "process", inBuffer, outFrameSize, _hidl_cb);
    return Void();
}

Return<void> Effect::processReverse(
        const AudioBuffer& inBuffer, uint32_t outFrameSize, processReverse_cb _hidl_cb)  {
    if ((*mHandle)->process_reverse != NULL) {
        processImpl(
                (*mHandle)->process_reverse, "process_reverse", inBuffer, outFrameSize, _hidl_cb);
    } else {
        _hidl_cb(Result::NOT_SUPPORTED, AudioBuffer());
    }
    return Void();
}

Return<void> Effect::command(
        uint32_t commandId,
        const hidl_vec<uint8_t>& data,
        uint32_t resultMaxSize,
        command_cb _hidl_cb)  {
    uint32_t halDataSize;
    void *halData;
    hidlVecToHal(data, &halDataSize, &halData);
    uint32_t halResultSize = resultMaxSize;
    std::unique_ptr<uint8_t[]> halResult(new uint8_t[halResultSize]);
    memset(&halResult[0], 0, halResultSize);
    status_t status = (*mHandle)->command(
            mHandle, commandId, halDataSize, halData, &halResultSize, &halResult[0]);
    hidl_vec<uint8_t> result;
    if (status == OK) {
        result.setToExternal(&halResult[0], halResultSize);
    }
    _hidl_cb(status, result);
    return Void();
}

Return<Result> Effect::setParameter(
        const hidl_vec<uint8_t>& parameter, const hidl_vec<uint8_t>& value)  {
    return setParameterImpl(parameter.size(), &parameter[0], value.size(), &value[0]);
}

Return<void> Effect::getParameter(
        const hidl_vec<uint8_t>& parameter, uint32_t valueMaxSize, getParameter_cb _hidl_cb)  {
    hidl_vec<uint8_t> value;
    Result retval = getParameterImpl(
            parameter.size(),
            &parameter[0],
            valueMaxSize,
            [&] (uint32_t valueSize, const void* valueData) {
                value.setToExternal(
                        reinterpret_cast<uint8_t*>(const_cast<void*>(valueData)), valueSize);
            });
    _hidl_cb(retval, value);
    return Void();
}

Return<void> Effect::getSupportedConfigsForFeature(
        uint32_t featureId,
        uint32_t maxConfigs,
        uint32_t configSize,
        getSupportedConfigsForFeature_cb _hidl_cb)  {
    uint32_t configCount = 0;
    hidl_vec<uint8_t> result;
    Result retval = getSupportedConfigsImpl(
            featureId,
            maxConfigs,
            configSize,
            [&] (uint32_t supportedConfigs, void* configsData) {
                configCount = supportedConfigs;
                result.resize(configCount * configSize);
                memcpy(&result[0], configsData, result.size());
            });
    _hidl_cb(retval, configCount, result);
    return Void();
}

Return<void> Effect::getCurrentConfigForFeature(
        uint32_t featureId, uint32_t configSize, getCurrentConfigForFeature_cb _hidl_cb)  {
    hidl_vec<uint8_t> result;
    Result retval = getCurrentConfigImpl(
            featureId,
            configSize,
            [&] (void* configData) {
                result.resize(configSize);
                memcpy(&result[0], configData, result.size());
            });
    _hidl_cb(retval, result);
    return Void();
}

Return<Result> Effect::setCurrentConfigForFeature(
        uint32_t featureId, const hidl_vec<uint8_t>& configData)  {
    uint32_t halCmd[alignedSizeIn<uint32_t>(sizeof(uint32_t) + configData.size())];
    memset(halCmd, 0, sizeof(halCmd));
    halCmd[0] = featureId;
    memcpy(&halCmd[1], &configData[0], configData.size());
    return sendCommandReturningStatus(
            EFFECT_CMD_SET_FEATURE_CONFIG, "SET_FEATURE_CONFIG", sizeof(halCmd), halCmd);
}

} // namespace implementation
}  // namespace V2_0
}  // namespace effect
}  // namespace audio
}  // namespace hardware
}  // namespace android
