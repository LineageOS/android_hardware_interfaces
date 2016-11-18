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

#define LOG_TAG "DeviceHAL"

#include <algorithm>
#include <memory.h>
#include <string.h>

#include <utils/Log.h>

#include "Conversions.h"
#include "Device.h"
#include "StreamIn.h"
#include "StreamOut.h"

namespace android {
namespace hardware {
namespace audio {
namespace V2_0 {
namespace implementation {

Device::Device(audio_hw_device_t* device)
        : mDevice(device) {
}

Device::~Device() {
    int status = audio_hw_device_close(mDevice);
    ALOGW_IF(status, "Error closing audio hw device %p: %s", mDevice, strerror(-status));
    mDevice = nullptr;
}

// static
void Device::audioConfigToHal(const AudioConfig& config, audio_config_t* halConfig) {
    memset(halConfig, 0, sizeof(audio_config_t));
    halConfig->sample_rate = config.sampleRateHz;
    halConfig->channel_mask = static_cast<audio_channel_mask_t>(config.channelMask);
    halConfig->format = static_cast<audio_format_t>(config.format);
    audioOffloadInfoToHal(config.offloadInfo, &halConfig->offload_info);
    halConfig->frame_count = config.frameCount;
}

// static
void Device::audioGainConfigFromHal(
        const struct audio_gain_config& halConfig, AudioGainConfig* config) {
    config->index = halConfig.index;
    config->mode = AudioGainMode(halConfig.mode);
    config->channelMask = AudioChannelMask(halConfig.channel_mask);
    for (size_t i = 0; i < sizeof(audio_channel_mask_t) * 8; ++i) {
        config->values[i] = halConfig.values[i];
    }
    config->rampDurationMs = halConfig.ramp_duration_ms;
}

// static
void Device::audioGainConfigToHal(
        const AudioGainConfig& config, struct audio_gain_config* halConfig) {
    halConfig->index = config.index;
    halConfig->mode = static_cast<audio_gain_mode_t>(config.mode);
    halConfig->channel_mask = static_cast<audio_channel_mask_t>(config.channelMask);
    memset(halConfig->values, 0, sizeof(halConfig->values));
    for (size_t i = 0; i < sizeof(audio_channel_mask_t) * 8; ++i) {
        halConfig->values[i] = config.values[i];
    }
    halConfig->ramp_duration_ms = config.rampDurationMs;
}

// static
void Device::audioGainFromHal(const struct audio_gain& halGain, AudioGain* gain) {
    gain->mode = AudioGainMode(halGain.mode);
    gain->channelMask = AudioChannelMask(halGain.channel_mask);
    gain->minValue = halGain.min_value;
    gain->maxValue = halGain.max_value;
    gain->defaultValue = halGain.default_value;
    gain->stepValue = halGain.step_value;
    gain->minRampMs = halGain.min_ramp_ms;
    gain->maxRampMs = halGain.max_ramp_ms;
}

// static
void Device::audioGainToHal(const AudioGain& gain, struct audio_gain* halGain) {
    halGain->mode = static_cast<audio_gain_mode_t>(gain.mode);
    halGain->channel_mask = static_cast<audio_channel_mask_t>(gain.channelMask);
    halGain->min_value = gain.minValue;
    halGain->max_value = gain.maxValue;
    halGain->default_value = gain.defaultValue;
    halGain->step_value = gain.stepValue;
    halGain->min_ramp_ms = gain.minRampMs;
    halGain->max_ramp_ms = gain.maxRampMs;
}

// static
void Device::audioOffloadInfoToHal(
        const AudioOffloadInfo& offload, audio_offload_info_t* halOffload) {
    *halOffload = AUDIO_INFO_INITIALIZER;
    halOffload->sample_rate = offload.sampleRateHz;
    halOffload->channel_mask = static_cast<audio_channel_mask_t>(offload.channelMask);
    halOffload->stream_type = static_cast<audio_stream_type_t>(offload.streamType);
    halOffload->bit_rate = offload.bitRatePerSecond;
    halOffload->duration_us = offload.durationMicroseconds;
    halOffload->has_video = offload.hasVideo;
    halOffload->is_streaming = offload.isStreaming;
}

// static
void Device::audioPortConfigFromHal(
        const struct audio_port_config& halConfig, AudioPortConfig* config) {
    config->id = halConfig.id;
    config->role = AudioPortRole(halConfig.role);
    config->type = AudioPortType(halConfig.type);
    config->configMask = AudioPortConfigMask(halConfig.config_mask);
    config->sampleRateHz = halConfig.sample_rate;
    config->channelMask = AudioChannelMask(halConfig.channel_mask);
    config->format = AudioFormat(halConfig.format);
    audioGainConfigFromHal(halConfig.gain, &config->gain);
    switch (halConfig.type) {
        case AUDIO_PORT_TYPE_NONE: break;
        case AUDIO_PORT_TYPE_DEVICE: {
            config->ext.device.hwModule = halConfig.ext.device.hw_module;
            config->ext.device.type = AudioDevice(halConfig.ext.device.type);
            memcpy(config->ext.device.address.data(),
                    halConfig.ext.device.address,
                    AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        }
        case AUDIO_PORT_TYPE_MIX: {
            config->ext.mix.hwModule = halConfig.ext.mix.hw_module;
            config->ext.mix.ioHandle = halConfig.ext.mix.handle;
            if (halConfig.role == AUDIO_PORT_ROLE_SOURCE) {
                config->ext.mix.useCase.source = AudioSource(halConfig.ext.mix.usecase.source);
            } else if (halConfig.role == AUDIO_PORT_ROLE_SINK) {
                config->ext.mix.useCase.stream = AudioStreamType(halConfig.ext.mix.usecase.stream);
            }
            break;
        }
        case AUDIO_PORT_TYPE_SESSION: {
            config->ext.session.session = halConfig.ext.session.session;
            break;
        }
    }
}

// static
void Device::audioPortConfigToHal(
        const AudioPortConfig& config, struct audio_port_config* halConfig) {
    memset(halConfig, 0, sizeof(audio_port_config));
    halConfig->id = config.id;
    halConfig->role = static_cast<audio_port_role_t>(config.role);
    halConfig->type = static_cast<audio_port_type_t>(config.type);
    halConfig->config_mask = static_cast<unsigned int>(config.configMask);
    halConfig->sample_rate = config.sampleRateHz;
    halConfig->channel_mask = static_cast<audio_channel_mask_t>(config.channelMask);
    halConfig->format = static_cast<audio_format_t>(config.format);
    audioGainConfigToHal(config.gain, &halConfig->gain);
    switch (config.type) {
        case AudioPortType::NONE: break;
        case AudioPortType::DEVICE: {
            halConfig->ext.device.hw_module = config.ext.device.hwModule;
            halConfig->ext.device.type = static_cast<audio_devices_t>(config.ext.device.type);
            memcpy(halConfig->ext.device.address,
                    config.ext.device.address.data(),
                    AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        }
        case AudioPortType::MIX: {
            halConfig->ext.mix.hw_module = config.ext.mix.hwModule;
            halConfig->ext.mix.handle = config.ext.mix.ioHandle;
            if (config.role == AudioPortRole::SOURCE) {
                halConfig->ext.mix.usecase.source =
                        static_cast<audio_source_t>(config.ext.mix.useCase.source);
            } else if (config.role == AudioPortRole::SINK) {
                halConfig->ext.mix.usecase.stream =
                        static_cast<audio_stream_type_t>(config.ext.mix.useCase.stream);
            }
            break;
        }
        case AudioPortType::SESSION: {
            halConfig->ext.session.session =
                    static_cast<audio_session_t>(config.ext.session.session);
            break;
        }
    }
}

// static
std::unique_ptr<audio_port_config[]> Device::audioPortConfigsToHal(
        const hidl_vec<AudioPortConfig>& configs) {
    std::unique_ptr<audio_port_config[]> halConfigs(new audio_port_config[configs.size()]);
    for (size_t i = 0; i < configs.size(); ++i) {
        audioPortConfigToHal(configs[i], &halConfigs[i]);
    }
    return halConfigs;
}

// static
void Device::audioPortFromHal(const struct audio_port& halPort, AudioPort* port) {
    port->id = halPort.id;
    port->role = AudioPortRole(halPort.role);
    port->type = AudioPortType(halPort.type);
    port->name.setToExternal(halPort.name, strlen(halPort.name));
    port->sampleRates.resize(halPort.num_sample_rates);
    for (size_t i = 0; i < halPort.num_sample_rates; ++i) {
        port->sampleRates[i] = halPort.sample_rates[i];
    }
    port->channelMasks.resize(halPort.num_channel_masks);
    for (size_t i = 0; i < halPort.num_channel_masks; ++i) {
        port->channelMasks[i] = AudioChannelMask(halPort.channel_masks[i]);
    }
    port->formats.resize(halPort.num_formats);
    for (size_t i = 0; i < halPort.num_formats; ++i) {
        port->formats[i] = AudioFormat(halPort.formats[i]);
    }
    port->gains.resize(halPort.num_gains);
    for (size_t i = 0; i < halPort.num_gains; ++i) {
        audioGainFromHal(halPort.gains[i], &port->gains[i]);
    }
    audioPortConfigFromHal(halPort.active_config, &port->activeConfig);
    switch (halPort.type) {
        case AUDIO_PORT_TYPE_NONE: break;
        case AUDIO_PORT_TYPE_DEVICE: {
            port->ext.device.hwModule = halPort.ext.device.hw_module;
            port->ext.device.type = AudioDevice(halPort.ext.device.type);
            memcpy(port->ext.device.address.data(),
                    halPort.ext.device.address,
                    AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        }
        case AUDIO_PORT_TYPE_MIX: {
            port->ext.mix.hwModule = halPort.ext.mix.hw_module;
            port->ext.mix.ioHandle = halPort.ext.mix.handle;
            port->ext.mix.latencyClass = AudioMixLatencyClass(halPort.ext.mix.latency_class);
            break;
        }
        case AUDIO_PORT_TYPE_SESSION: {
            port->ext.session.session = halPort.ext.session.session;
            break;
        }
    }
}

// static
void Device::audioPortToHal(const AudioPort& port, struct audio_port* halPort) {
    memset(halPort, 0, sizeof(audio_port));
    halPort->id = port.id;
    halPort->role = static_cast<audio_port_role_t>(port.role);
    halPort->type = static_cast<audio_port_type_t>(port.type);
    memcpy(halPort->name,
            port.name.c_str(),
            std::min(port.name.size(), static_cast<size_t>(AUDIO_PORT_MAX_NAME_LEN)));
    halPort->num_sample_rates =
            std::min(port.sampleRates.size(), static_cast<size_t>(AUDIO_PORT_MAX_SAMPLING_RATES));
    for (size_t i = 0; i < halPort->num_sample_rates; ++i) {
        halPort->sample_rates[i] = port.sampleRates[i];
    }
    halPort->num_channel_masks =
            std::min(port.channelMasks.size(), static_cast<size_t>(AUDIO_PORT_MAX_CHANNEL_MASKS));
    for (size_t i = 0; i < halPort->num_channel_masks; ++i) {
        halPort->channel_masks[i] = static_cast<audio_channel_mask_t>(port.channelMasks[i]);
    }
    halPort->num_formats =
            std::min(port.formats.size(), static_cast<size_t>(AUDIO_PORT_MAX_FORMATS));
    for (size_t i = 0; i < halPort->num_formats; ++i) {
        halPort->formats[i] = static_cast<audio_format_t>(port.formats[i]);
    }
    halPort->num_gains = std::min(port.gains.size(), static_cast<size_t>(AUDIO_PORT_MAX_GAINS));
    for (size_t i = 0; i < halPort->num_gains; ++i) {
        audioGainToHal(port.gains[i], &halPort->gains[i]);
    }
    audioPortConfigToHal(port.activeConfig, &halPort->active_config);
    switch (port.type) {
        case AudioPortType::NONE: break;
        case AudioPortType::DEVICE: {
            halPort->ext.device.hw_module = port.ext.device.hwModule;
            halPort->ext.device.type = static_cast<audio_devices_t>(port.ext.device.type);
            memcpy(halPort->ext.device.address,
                    port.ext.device.address.data(),
                    AUDIO_DEVICE_MAX_ADDRESS_LEN);
            break;
        }
        case AudioPortType::MIX: {
            halPort->ext.mix.hw_module = port.ext.mix.hwModule;
            halPort->ext.mix.handle = port.ext.mix.ioHandle;
            halPort->ext.mix.latency_class =
                    static_cast<audio_mix_latency_class_t>(port.ext.mix.latencyClass);
            break;
        }
        case AudioPortType::SESSION: {
            halPort->ext.session.session = static_cast<audio_session_t>(port.ext.session.session);
            break;
        }
    }
}

Result Device::analyzeStatus(const char* funcName, int status) {
    if (status != 0) {
        ALOGW("Device %p %s: %s", mDevice, funcName, strerror(-status));
    }
    switch (status) {
        case 0: return Result::OK;
        case -EINVAL: return Result::INVALID_ARGUMENTS;
        case -ENODATA: return Result::INVALID_STATE;
        case -ENODEV: return Result::NOT_INITIALIZED;
        case -ENOSYS: return Result::NOT_SUPPORTED;
        default: return Result::INVALID_STATE;
    }
}

char* Device::halGetParameters(const char* keys) {
    return mDevice->get_parameters(mDevice, keys);
}

int Device::halSetParameters(const char* keysAndValues) {
    return mDevice->set_parameters(mDevice, keysAndValues);
}

// Methods from ::android::hardware::audio::V2_0::IDevice follow.
Return<Result> Device::initCheck()  {
    return analyzeStatus("init_check", mDevice->init_check(mDevice));
}

Return<Result> Device::setMasterVolume(float volume)  {
    Result retval(Result::NOT_SUPPORTED);
    if (mDevice->set_master_volume != NULL) {
        retval = analyzeStatus("set_master_volume", mDevice->set_master_volume(mDevice, volume));
    }
    return retval;
}

Return<void> Device::getMasterVolume(getMasterVolume_cb _hidl_cb)  {
    Result retval(Result::NOT_SUPPORTED);
    float volume = 0;
    if (mDevice->get_master_volume != NULL) {
        retval = analyzeStatus("get_master_volume", mDevice->get_master_volume(mDevice, &volume));
    }
    _hidl_cb(retval, volume);
    return Void();
}

Return<Result> Device::setMicMute(bool mute)  {
    return analyzeStatus("set_mic_mute", mDevice->set_mic_mute(mDevice, mute));
}

Return<void> Device::getMicMute(getMicMute_cb _hidl_cb)  {
    bool mute = false;
    Result retval = analyzeStatus("get_mic_mute", mDevice->get_mic_mute(mDevice, &mute));
    _hidl_cb(retval, mute);
    return Void();
}

Return<Result> Device::setMasterMute(bool mute)  {
    Result retval(Result::NOT_SUPPORTED);
    if (mDevice->set_master_mute != NULL) {
        retval = analyzeStatus("set_master_mute", mDevice->set_master_mute(mDevice, mute));
    }
    return retval;
}

Return<void> Device::getMasterMute(getMasterMute_cb _hidl_cb)  {
    Result retval(Result::NOT_SUPPORTED);
    bool mute = false;
    if (mDevice->get_master_mute != NULL) {
        retval = analyzeStatus("get_master_mute", mDevice->get_master_mute(mDevice, &mute));
    }
    _hidl_cb(retval, mute);
    return Void();
}

Return<void> Device::getInputBufferSize(
        const AudioConfig& config, getInputBufferSize_cb _hidl_cb)  {
    audio_config_t halConfig;
    audioConfigToHal(config, &halConfig);
    size_t halBufferSize = mDevice->get_input_buffer_size(mDevice, &halConfig);
    Result retval(Result::INVALID_ARGUMENTS);
    uint64_t bufferSize = 0;
    if (halBufferSize != 0) {
        retval = Result::OK;
        bufferSize = halBufferSize;
    }
    _hidl_cb(retval, bufferSize);
    return Void();
}

Return<void> Device::openOutputStream(
        int32_t ioHandle,
        const DeviceAddress& device,
        const AudioConfig& config,
        AudioOutputFlag flags,
        openOutputStream_cb _hidl_cb)  {
    audio_config_t halConfig;
    audioConfigToHal(config, &halConfig);
    audio_stream_out_t *halStream;
    int status = mDevice->open_output_stream(
            mDevice,
            ioHandle,
            static_cast<audio_devices_t>(device.device),
            static_cast<audio_output_flags_t>(flags),
            &halConfig,
            &halStream,
            deviceAddressToHal(device).c_str());
    sp<IStreamOut> streamOut;
    if (status == OK) {
        streamOut = new StreamOut(mDevice, halStream);
    }
    _hidl_cb(analyzeStatus("open_output_stream", status), streamOut);
    return Void();
}

Return<void> Device::openInputStream(
        int32_t ioHandle,
        const DeviceAddress& device,
        const AudioConfig& config,
        AudioInputFlag flags,
        AudioSource source,
        openInputStream_cb _hidl_cb)  {
    audio_config_t halConfig;
    audioConfigToHal(config, &halConfig);
    audio_stream_in_t *halStream;
    int status = mDevice->open_input_stream(
            mDevice,
            ioHandle,
            static_cast<audio_devices_t>(device.device),
            &halConfig,
            &halStream,
            static_cast<audio_input_flags_t>(flags),
            deviceAddressToHal(device).c_str(),
            static_cast<audio_source_t>(source));
    sp<IStreamIn> streamIn;
    if (status == OK) {
        streamIn = new StreamIn(mDevice, halStream);
    }
    _hidl_cb(analyzeStatus("open_input_stream", status), streamIn);
    return Void();
}

Return<void> Device::createAudioPatch(
        const hidl_vec<AudioPortConfig>& sources,
        const hidl_vec<AudioPortConfig>& sinks,
        createAudioPatch_cb _hidl_cb)  {
    Result retval(Result::NOT_SUPPORTED);
    AudioPatchHandle patch = 0;
    if (version() >= AUDIO_DEVICE_API_VERSION_3_0) {
        std::unique_ptr<audio_port_config[]> halSources(audioPortConfigsToHal(sources));
        std::unique_ptr<audio_port_config[]> halSinks(audioPortConfigsToHal(sinks));
        audio_patch_handle_t halPatch;
        retval = analyzeStatus(
                "create_audio_patch",
                mDevice->create_audio_patch(
                        mDevice,
                        sources.size(), &halSources[0],
                        sinks.size(), &halSinks[0],
                        &halPatch));
        if (retval == Result::OK) {
            patch = static_cast<AudioPatchHandle>(halPatch);
        }
    }
    _hidl_cb(retval, patch);
    return Void();
}

Return<Result> Device::releaseAudioPatch(int32_t patch)  {
    if (version() >= AUDIO_DEVICE_API_VERSION_3_0) {
        return analyzeStatus(
                "release_audio_patch",
                mDevice->release_audio_patch(mDevice, static_cast<audio_patch_handle_t>(patch)));
    }
    return Result::NOT_SUPPORTED;
}

Return<void> Device::getAudioPort(const AudioPort& port, getAudioPort_cb _hidl_cb)  {
    audio_port halPort;
    audioPortToHal(port, &halPort);
    Result retval = analyzeStatus("get_audio_port", mDevice->get_audio_port(mDevice, &halPort));
    AudioPort resultPort = port;
    if (retval == Result::OK) {
        audioPortFromHal(halPort, &resultPort);
    }
    _hidl_cb(retval, resultPort);
    return Void();
}

Return<Result> Device::setAudioPortConfig(const AudioPortConfig& config)  {
    if (version() >= AUDIO_DEVICE_API_VERSION_3_0) {
        struct audio_port_config halPortConfig;
        audioPortConfigToHal(config, &halPortConfig);
        return analyzeStatus(
                "set_audio_port_config", mDevice->set_audio_port_config(mDevice, &halPortConfig));
    }
    return Result::NOT_SUPPORTED;
}

Return<AudioHwSync> Device::getHwAvSync()  {
    int halHwAvSync;
    Result retval = getParam(AudioParameter::keyHwAvSync, &halHwAvSync);
    return retval == Result::OK ? halHwAvSync : AUDIO_HW_SYNC_INVALID;
}

Return<Result> Device::setScreenState(bool turnedOn)  {
    return setParam(AudioParameter::keyScreenState, turnedOn);
}

Return<void> Device::getParameters(const hidl_vec<hidl_string>& keys, getParameters_cb _hidl_cb)  {
    getParametersImpl(keys, _hidl_cb);
    return Void();
}

Return<Result> Device::setParameters(const hidl_vec<ParameterValue>& parameters)  {
    return setParametersImpl(parameters);
}

Return<void> Device::debugDump(const hidl_handle& fd)  {
    if (fd->numFds == 1) {
        analyzeStatus("dump", mDevice->dump(mDevice, fd->data[0]));
    }
    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace hardware
}  // namespace android
