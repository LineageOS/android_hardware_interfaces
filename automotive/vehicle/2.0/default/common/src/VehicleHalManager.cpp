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

#define LOG_TAG "automotive.vehicle@2.0-impl"

#include "VehicleHalManager.h"

#include <cmath>
#include <fstream>
#include <unordered_set>

#include <android-base/parsedouble.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android/hardware/automotive/vehicle/2.0/BpHwVehicleCallback.h>
#include <android/log.h>

#include <hwbinder/IPCThreadState.h>
#include <private/android_filesystem_config.h>
#include <utils/SystemClock.h>

#include "VehicleUtils.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

using namespace std::placeholders;

using ::android::base::EqualsIgnoreCase;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;

namespace {

constexpr std::chrono::milliseconds kHalEventBatchingTimeWindow(10);

const VehiclePropValue kEmptyValue{};

// A list of supported options for "--set" command.
const std::unordered_set<std::string> kSetPropOptions = {
        // integer.
        "-i",
        // 64bit integer.
        "-i64",
        // float.
        "-f",
        // string.
        "-s",
        // bytes in hex format, e.g. 0xDEADBEEF.
        "-b",
        // Area id in integer.
        "-a"};

}  // namespace

/**
 * Indicates what's the maximum size of hidl_vec<VehiclePropValue> we want
 * to store in reusable object pool.
 */
constexpr auto kMaxHidlVecOfVehiclePropValuePoolSize = 20;

Return<void> VehicleHalManager::getAllPropConfigs(getAllPropConfigs_cb _hidl_cb) {
    ALOGI("getAllPropConfigs called");
    hidl_vec<VehiclePropConfig> hidlConfigs;
    auto& halConfig = mConfigIndex->getAllConfigs();

    hidlConfigs.setToExternal(
            const_cast<VehiclePropConfig *>(halConfig.data()),
            halConfig.size());

    _hidl_cb(hidlConfigs);

    return Void();
}

Return<void> VehicleHalManager::getPropConfigs(const hidl_vec<int32_t> &properties,
                                               getPropConfigs_cb _hidl_cb) {
    std::vector<VehiclePropConfig> configs;
    for (size_t i = 0; i < properties.size(); i++) {
        auto prop = properties[i];
        if (mConfigIndex->hasConfig(prop)) {
            configs.push_back(mConfigIndex->getConfig(prop));
        } else {
            ALOGW("Requested config for undefined property: 0x%x", prop);
            _hidl_cb(StatusCode::INVALID_ARG, hidl_vec<VehiclePropConfig>());
            return Void();
        }
    }

    _hidl_cb(StatusCode::OK, configs);

    return Void();
}

Return<void> VehicleHalManager::get(const VehiclePropValue& requestedPropValue, get_cb _hidl_cb) {
    const auto* config = getPropConfigOrNull(requestedPropValue.prop);
    if (config == nullptr) {
        ALOGE("Failed to get value: config not found, property: 0x%x",
              requestedPropValue.prop);
        _hidl_cb(StatusCode::INVALID_ARG, kEmptyValue);
        return Void();
    }

    if (!checkReadPermission(*config)) {
        _hidl_cb(StatusCode::ACCESS_DENIED, kEmptyValue);
        return Void();
    }

    StatusCode status;
    auto value = mHal->get(requestedPropValue, &status);
    _hidl_cb(status, value.get() ? *value : kEmptyValue);


    return Void();
}

Return<StatusCode> VehicleHalManager::set(const VehiclePropValue &value) {
    auto prop = value.prop;
    const auto* config = getPropConfigOrNull(prop);
    if (config == nullptr) {
        ALOGE("Failed to set value: config not found, property: 0x%x", prop);
        return StatusCode::INVALID_ARG;
    }

    if (!checkWritePermission(*config)) {
        return StatusCode::ACCESS_DENIED;
    }

    handlePropertySetEvent(value);

    auto status = mHal->set(value);

    return Return<StatusCode>(status);
}

Return<StatusCode> VehicleHalManager::subscribe(const sp<IVehicleCallback> &callback,
                                                const hidl_vec<SubscribeOptions> &options) {
    hidl_vec<SubscribeOptions> verifiedOptions(options);
    for (size_t i = 0; i < verifiedOptions.size(); i++) {
        SubscribeOptions& ops = verifiedOptions[i];
        auto prop = ops.propId;

        const auto* config = getPropConfigOrNull(prop);
        if (config == nullptr) {
            ALOGE("Failed to subscribe: config not found, property: 0x%x",
                  prop);
            return StatusCode::INVALID_ARG;
        }

        if (ops.flags == SubscribeFlags::UNDEFINED) {
            ALOGE("Failed to subscribe: undefined flag in options provided");
            return StatusCode::INVALID_ARG;
        }

        if (!isSubscribable(*config, ops.flags)) {
            ALOGE("Failed to subscribe: property 0x%x is not subscribable",
                  prop);
            return StatusCode::INVALID_ARG;
        }

        ops.sampleRate = checkSampleRate(*config, ops.sampleRate);
    }

    std::list<SubscribeOptions> updatedOptions;
    auto res = mSubscriptionManager.addOrUpdateSubscription(getClientId(callback),
                                                            callback, verifiedOptions,
                                                            &updatedOptions);
    if (StatusCode::OK != res) {
        ALOGW("%s failed to subscribe, error code: %d", __func__, res);
        return res;
    }

    for (auto opt : updatedOptions) {
        mHal->subscribe(opt.propId, opt.sampleRate);
    }

    return StatusCode::OK;
}

Return<StatusCode> VehicleHalManager::unsubscribe(const sp<IVehicleCallback>& callback,
                                                  int32_t propId) {
    mSubscriptionManager.unsubscribe(getClientId(callback), propId);
    return StatusCode::OK;
}

Return<void> VehicleHalManager::debugDump(IVehicle::debugDump_cb _hidl_cb) {
    _hidl_cb("");
    return Void();
}

Return<void> VehicleHalManager::debug(const hidl_handle& fd, const hidl_vec<hidl_string>& options) {
    if (fd.getNativeHandle() == nullptr || fd->numFds == 0) {
        ALOGE("Invalid parameters passed to debug()");
        return Void();
    }

    bool shouldContinue = mHal->dump(fd, options);
    if (!shouldContinue) {
        ALOGI("Dumped HAL only");
        return Void();
    }

    // Do our dump
    cmdDump(fd->data[0], options);
    return Void();
}

void VehicleHalManager::cmdDump(int fd, const hidl_vec<hidl_string>& options) {
    if (options.size() == 0) {
        cmdDumpAllProperties(fd);
        return;
    }
    std::string option = options[0];
    if (EqualsIgnoreCase(option, "--help")) {
        cmdHelp(fd);
    } else if (EqualsIgnoreCase(option, "--list")) {
        cmdListAllProperties(fd);
    } else if (EqualsIgnoreCase(option, "--get")) {
        cmdDumpSpecificProperties(fd, options);
    } else if (EqualsIgnoreCase(option, "--set")) {
        if (!checkCallerHasWritePermissions(fd)) {
            dprintf(fd, "Caller does not have write permission\n");
            return;
        }
        // Ignore the return value for this.
        cmdSetOneProperty(fd, options);
    } else {
        dprintf(fd, "Invalid option: %s\n", option.c_str());
    }
}

bool VehicleHalManager::checkCallerHasWritePermissions(int fd) {
    // Double check that's only called by root - it should be be blocked at the HIDL debug() level,
    // but it doesn't hurt to make sure...
    if (hardware::IPCThreadState::self()->getCallingUid() != AID_ROOT) {
        dprintf(fd, "Must be root\n");
        return false;
    }
    return true;
}

bool VehicleHalManager::checkArgumentsSize(int fd, const hidl_vec<hidl_string>& options,
                                           size_t minSize) {
    size_t size = options.size();
    if (size >= minSize) {
        return true;
    }
    dprintf(fd, "Invalid number of arguments: required at least %zu, got %zu\n", minSize, size);
    return false;
}

template <typename T>
bool VehicleHalManager::safelyParseInt(int fd, int index, const std::string& s, T* out) {
    if (!android::base::ParseInt(s, out)) {
        dprintf(fd, "non-integer argument at index %d: %s\n", index, s.c_str());
        return false;
    }
    return true;
}

bool VehicleHalManager::safelyParseFloat(int fd, int index, const std::string& s, float* out) {
    if (!android::base::ParseFloat(s, out)) {
        dprintf(fd, "non-float argument at index %d: %s\n", index, s.c_str());
        return false;
    }
    return true;
}

void VehicleHalManager::cmdHelp(int fd) const {
    dprintf(fd, "Usage: \n\n");
    dprintf(fd, "[no args]: dumps (id and value) all supported properties \n");
    dprintf(fd, "--help: shows this help\n");
    dprintf(fd, "--list: lists the ids of all supported properties\n");
    dprintf(fd, "--get <PROP1> [PROP2] [PROPN]: dumps the value of specific properties \n");
    dprintf(fd,
            "--set <PROP> [-i INT_VALUE [INT_VALUE ...]] [-i64 INT64_VALUE [INT64_VALUE ...]] "
            "[-f FLOAT_VALUE [FLOAT_VALUE ...]] [-s STR_VALUE] "
            "[-b BYTES_VALUE] [-a AREA_ID] : sets the value of property PROP. "
            "Notice that the string, bytes and area value can be set just once, while the other can"
            " have multiple values (so they're used in the respective array), "
            "BYTES_VALUE is in the form of 0xXXXX, e.g. 0xdeadbeef.\n");
}

void VehicleHalManager::cmdListAllProperties(int fd) const {
    auto& halConfig = mConfigIndex->getAllConfigs();
    size_t size = halConfig.size();
    if (size == 0) {
        dprintf(fd, "no properties to list\n");
        return;
    }
    int i = 0;
    dprintf(fd, "listing %zu properties\n", size);
    for (const auto& config : halConfig) {
        dprintf(fd, "%d: %d\n", ++i, config.prop);
    }
}

void VehicleHalManager::cmdDumpAllProperties(int fd) {
    auto& halConfig = mConfigIndex->getAllConfigs();
    size_t size = halConfig.size();
    if (size == 0) {
        dprintf(fd, "no properties to dump\n");
        return;
    }
    int rowNumber = 0;
    dprintf(fd, "dumping %zu properties\n", size);
    for (auto& config : halConfig) {
        cmdDumpOneProperty(fd, ++rowNumber, config);
    }
}

void VehicleHalManager::cmdDumpOneProperty(int fd, int rowNumber, const VehiclePropConfig& config) {
    size_t numberAreas = config.areaConfigs.size();
    if (numberAreas == 0) {
        if (rowNumber > 0) {
            dprintf(fd, "%d: ", rowNumber);
        }
        cmdDumpOneProperty(fd, config.prop, /* areaId= */ 0);
        return;
    }
    for (size_t j = 0; j < numberAreas; ++j) {
        if (rowNumber > 0) {
            if (numberAreas > 1) {
                dprintf(fd, "%d/%zu: ", rowNumber, j);
            } else {
                dprintf(fd, "%d: ", rowNumber);
            }
        }
        cmdDumpOneProperty(fd, config.prop, config.areaConfigs[j].areaId);
    }
}

void VehicleHalManager::cmdDumpSpecificProperties(int fd, const hidl_vec<hidl_string>& options) {
    if (!checkArgumentsSize(fd, options, 2)) return;

    // options[0] is the command itself...
    int rowNumber = 0;
    size_t size = options.size();
    for (size_t i = 1; i < size; ++i) {
        int prop;
        if (!safelyParseInt(fd, i, options[i], &prop)) return;
        const auto* config = getPropConfigOrNull(prop);
        if (config == nullptr) {
            dprintf(fd, "No property %d\n", prop);
            continue;
        }
        if (size > 2) {
            // Only show row number if there's more than 1
            rowNumber++;
        }
        cmdDumpOneProperty(fd, rowNumber, *config);
    }
}

void VehicleHalManager::cmdDumpOneProperty(int fd, int32_t prop, int32_t areaId) {
    VehiclePropValue input;
    input.prop = prop;
    input.areaId = areaId;
    auto callback = [&fd, &prop](StatusCode status, const VehiclePropValue& output) {
        if (status == StatusCode::OK) {
            dprintf(fd, "%s\n", toString(output).c_str());
        } else {
            dprintf(fd, "Could not get property %d. Error: %s\n", prop, toString(status).c_str());
        }
    };

    StatusCode status;
    auto value = mHal->get(input, &status);
    callback(status, value.get() ? *value : kEmptyValue);
}

bool VehicleHalManager::cmdSetOneProperty(int fd, const hidl_vec<hidl_string>& options) {
    if (!checkArgumentsSize(fd, options, 4)) {
        dprintf(fd, "Requires at least 4 options, see help\n");
        return false;
    }

    VehiclePropValue prop = {};
    if (!parseSetPropOptions(fd, options, &prop)) {
        return false;
    }
    ALOGD("Setting prop %s", toString(prop).c_str());

    // Do not use VehicleHalManager::set here because we don't want to check write permission.
    // Caller should be able to use the debug interface to set read-only properties.
    handlePropertySetEvent(prop);
    auto status = mHal->set(prop);

    if (status == StatusCode::OK) {
        dprintf(fd, "Set property %s\n", toString(prop).c_str());
        return true;
    }
    dprintf(fd, "Failed to set property %s: %s\n", toString(prop).c_str(),
            toString(status).c_str());
    return false;
}

void VehicleHalManager::init() {
    ALOGI("VehicleHalManager::init");

    mHidlVecOfVehiclePropValuePool.resize(kMaxHidlVecOfVehiclePropValuePoolSize);

    mBatchingConsumer.run(&mEventQueue,
                          kHalEventBatchingTimeWindow,
                          std::bind(&VehicleHalManager::onBatchHalEvent,
                                    this, _1));

    mHal->init(&mValueObjectPool,
               std::bind(&VehicleHalManager::onHalEvent, this, _1),
               std::bind(&VehicleHalManager::onHalPropertySetError, this,
                         _1, _2, _3));

    // Initialize index with vehicle configurations received from VehicleHal.
    auto supportedPropConfigs = mHal->listProperties();
    mConfigIndex.reset(new VehiclePropConfigIndex(supportedPropConfigs));

    std::vector<int32_t> supportedProperties(
        supportedPropConfigs.size());
    for (const auto& config : supportedPropConfigs) {
        supportedProperties.push_back(config.prop);
    }
}

VehicleHalManager::~VehicleHalManager() {
    mBatchingConsumer.requestStop();
    mEventQueue.deactivate();
    // We have to wait until consumer thread is fully stopped because it may
    // be in a state of running callback (onBatchHalEvent).
    mBatchingConsumer.waitStopped();
    ALOGI("VehicleHalManager::dtor");
}

void VehicleHalManager::onHalEvent(VehiclePropValuePtr v) {
    mEventQueue.push(std::move(v));
}

void VehicleHalManager::onHalPropertySetError(StatusCode errorCode,
                                              int32_t property,
                                              int32_t areaId) {
    const auto& clients =
        mSubscriptionManager.getSubscribedClients(property, SubscribeFlags::EVENTS_FROM_CAR);

    for (const auto& client : clients) {
        client->getCallback()->onPropertySetError(errorCode, property, areaId);
    }
}

void VehicleHalManager::onBatchHalEvent(const std::vector<VehiclePropValuePtr>& values) {
    const auto& clientValues =
        mSubscriptionManager.distributeValuesToClients(values, SubscribeFlags::EVENTS_FROM_CAR);

    for (const HalClientValues& cv : clientValues) {
        auto vecSize = cv.values.size();
        hidl_vec<VehiclePropValue> vec;
        if (vecSize < kMaxHidlVecOfVehiclePropValuePoolSize) {
            vec.setToExternal(&mHidlVecOfVehiclePropValuePool[0], vecSize);
        } else {
            vec.resize(vecSize);
        }

        int i = 0;
        for (VehiclePropValue* pValue : cv.values) {
            shallowCopy(&(vec)[i++], *pValue);
        }
        auto status = cv.client->getCallback()->onPropertyEvent(vec);
        if (!status.isOk()) {
            ALOGE("Failed to notify client %s, err: %s",
                  toString(cv.client->getCallback()).c_str(),
                  status.description().c_str());
        }
    }
}

bool VehicleHalManager::isSampleRateFixed(VehiclePropertyChangeMode mode) {
    return (mode & VehiclePropertyChangeMode::ON_CHANGE);
}

float VehicleHalManager::checkSampleRate(const VehiclePropConfig &config,
                                         float sampleRate) {
    if (isSampleRateFixed(config.changeMode)) {
        if (std::abs(sampleRate) > std::numeric_limits<float>::epsilon()) {
            ALOGW("Sample rate is greater than zero for on change type. "
                      "Ignoring it.");
        }
        return 0.0;
    } else {
        if (sampleRate > config.maxSampleRate) {
            ALOGW("Sample rate %f is higher than max %f. Setting sampling rate "
                      "to max.", sampleRate, config.maxSampleRate);
            return config.maxSampleRate;
        }
        if (sampleRate < config.minSampleRate) {
            ALOGW("Sample rate %f is lower than min %f. Setting sampling rate "
                      "to min.", sampleRate, config.minSampleRate);
            return config.minSampleRate;
        }
    }
    return sampleRate;  // Provided sample rate was good, no changes.
}

bool VehicleHalManager::isSubscribable(const VehiclePropConfig& config,
                                       SubscribeFlags flags) {
    bool isReadable = config.access & VehiclePropertyAccess::READ;

    if (!isReadable && (SubscribeFlags::EVENTS_FROM_CAR & flags)) {
        ALOGW("Cannot subscribe, property 0x%x is not readable", config.prop);
        return false;
    }
    if (config.changeMode == VehiclePropertyChangeMode::STATIC) {
        ALOGW("Cannot subscribe, property 0x%x is static", config.prop);
        return false;
    }
    return true;
}

bool VehicleHalManager::checkWritePermission(const VehiclePropConfig &config) const {
    if (!(config.access & VehiclePropertyAccess::WRITE)) {
        ALOGW("Property 0%x has no write access", config.prop);
        return false;
    } else {
        return true;
    }
}

bool VehicleHalManager::checkReadPermission(const VehiclePropConfig &config) const {
    if (!(config.access & VehiclePropertyAccess::READ)) {
        ALOGW("Property 0%x has no read access", config.prop);
        return false;
    } else {
        return true;
    }
}

void VehicleHalManager::handlePropertySetEvent(const VehiclePropValue& value) {
    auto clients =
        mSubscriptionManager.getSubscribedClients(value.prop, SubscribeFlags::EVENTS_FROM_ANDROID);
    for (const auto& client : clients) {
        client->getCallback()->onPropertySet(value);
    }
}

const VehiclePropConfig* VehicleHalManager::getPropConfigOrNull(
        int32_t prop) const {
    return mConfigIndex->hasConfig(prop)
           ? &mConfigIndex->getConfig(prop) : nullptr;
}

void VehicleHalManager::onAllClientsUnsubscribed(int32_t propertyId) {
    mHal->unsubscribe(propertyId);
}

ClientId VehicleHalManager::getClientId(const sp<IVehicleCallback>& callback) {
    //TODO(b/32172906): rework this to get some kind of unique id for callback interface when this
    // feature is ready in HIDL.

    if (callback->isRemote()) {
        BpHwVehicleCallback* hwCallback = static_cast<BpHwVehicleCallback*>(callback.get());
        return static_cast<ClientId>(reinterpret_cast<intptr_t>(hwCallback->onAsBinder()));
    } else {
        return static_cast<ClientId>(reinterpret_cast<intptr_t>(callback.get()));
    }
}

std::vector<std::string> VehicleHalManager::getOptionValues(const hidl_vec<hidl_string>& options,
                                                            size_t* index) {
    std::vector<std::string> values;
    while (*index < options.size()) {
        std::string option = options[*index];
        if (kSetPropOptions.find(option) != kSetPropOptions.end()) {
            return std::move(values);
        }
        values.push_back(option);
        (*index)++;
    }
    return std::move(values);
}

bool VehicleHalManager::parseSetPropOptions(int fd, const hidl_vec<hidl_string>& options,
                                            VehiclePropValue* prop) {
    // Options format:
    // --set PROP [-f f1 f2...] [-i i1 i2...] [-i64 i1 i2...] [-s s1 s2...] [-b b1 b2...] [-a a]
    size_t optionIndex = 1;
    int propValue;
    if (!safelyParseInt(fd, optionIndex, options[optionIndex], &propValue)) {
        dprintf(fd, "property value: \"%s\" is not a valid int\n", options[optionIndex].c_str());
        return false;
    }
    prop->prop = propValue;
    prop->timestamp = elapsedRealtimeNano();
    prop->status = VehiclePropertyStatus::AVAILABLE;
    optionIndex++;
    std::unordered_set<std::string> parsedOptions;

    while (optionIndex < options.size()) {
        std::string type = options[optionIndex];
        optionIndex++;
        size_t currentIndex = optionIndex;
        std::vector<std::string> values = getOptionValues(options, &optionIndex);
        if (parsedOptions.find(type) != parsedOptions.end()) {
            dprintf(fd, "duplicate \"%s\" options\n", type.c_str());
            return false;
        }
        parsedOptions.insert(type);
        if (EqualsIgnoreCase(type, "-i")) {
            if (values.size() == 0) {
                dprintf(fd, "no values specified when using \"-i\"\n");
                return false;
            }
            prop->value.int32Values.resize(values.size());
            for (size_t i = 0; i < values.size(); i++) {
                int32_t safeInt;
                if (!safelyParseInt(fd, currentIndex + i, values[i], &safeInt)) {
                    dprintf(fd, "value: \"%s\" is not a valid int\n", values[i].c_str());
                    return false;
                }
                prop->value.int32Values[i] = safeInt;
            }
        } else if (EqualsIgnoreCase(type, "-i64")) {
            if (values.size() == 0) {
                dprintf(fd, "no values specified when using \"-i64\"\n");
                return false;
            }
            prop->value.int64Values.resize(values.size());
            for (size_t i = 0; i < values.size(); i++) {
                int64_t safeInt;
                if (!safelyParseInt(fd, currentIndex + i, values[i], &safeInt)) {
                    dprintf(fd, "value: \"%s\" is not a valid int64\n", values[i].c_str());
                    return false;
                }
                prop->value.int64Values[i] = safeInt;
            }
        } else if (EqualsIgnoreCase(type, "-f")) {
            if (values.size() == 0) {
                dprintf(fd, "no values specified when using \"-f\"\n");
                return false;
            }
            prop->value.floatValues.resize(values.size());
            for (size_t i = 0; i < values.size(); i++) {
                float safeFloat;
                if (!safelyParseFloat(fd, currentIndex + i, values[i], &safeFloat)) {
                    dprintf(fd, "value: \"%s\" is not a valid float\n", values[i].c_str());
                    return false;
                }
                prop->value.floatValues[i] = safeFloat;
            }
        } else if (EqualsIgnoreCase(type, "-s")) {
            if (values.size() != 1) {
                dprintf(fd, "expect exact one value when using \"-s\"\n");
                return false;
            }
            prop->value.stringValue = values[0];
        } else if (EqualsIgnoreCase(type, "-b")) {
            if (values.size() != 1) {
                dprintf(fd, "expect exact one value when using \"-b\"\n");
                return false;
            }
            std::vector<uint8_t> bytes;
            if (!parseHexString(fd, values[0], &bytes)) {
                dprintf(fd, "value: \"%s\" is not a valid hex string\n", values[0].c_str());
                return false;
            }
            prop->value.bytes = bytes;
        } else if (EqualsIgnoreCase(type, "-a")) {
            if (values.size() != 1) {
                dprintf(fd, "expect exact one value when using \"-a\"\n");
                return false;
            }
            if (!safelyParseInt(fd, currentIndex, values[0], &(prop->areaId))) {
                dprintf(fd, "area ID: \"%s\" is not a valid int\n", values[0].c_str());
                return false;
            }
        } else {
            dprintf(fd, "unknown option: %s\n", type.c_str());
            return false;
        }
    }

    return true;
}

bool VehicleHalManager::parseHexString(int fd, const std::string& s, std::vector<uint8_t>* bytes) {
    if (s.size() % 2 != 0) {
        dprintf(fd, "invalid hex string: %s, should have even size\n", s.c_str());
        return false;
    }
    if (strncmp(s.substr(0, 2).c_str(), "0x", 2)) {
        dprintf(fd, "hex string should start with \"0x\", got %s\n", s.c_str());
        return false;
    }
    std::string subs = s.substr(2);
    std::transform(subs.begin(), subs.end(), subs.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    bool highDigit = true;
    for (size_t i = 0; i < subs.size(); i++) {
        char c = subs[i];
        uint8_t v;
        if (c >= '0' && c <= '9') {
            v = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            v = c - 'a' + 10;
        } else {
            dprintf(fd, "invalid character %c in hex string %s\n", c, subs.c_str());
            return false;
        }
        if (highDigit) {
            (*bytes).push_back(v * 16);
        } else {
            (*bytes)[bytes->size() - 1] += v;
        }
        highDigit = !highDigit;
    }
    return true;
}

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
