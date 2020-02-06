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

#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android/hardware/automotive/vehicle/2.0/BpHwVehicleCallback.h>
#include <android/log.h>

#include <hwbinder/IPCThreadState.h>

#include <utils/SystemClock.h>

#include "VehicleUtils.h"

// TODO: figure out how to include private/android_filesystem_config.h instead...
#define AID_ROOT 0 /* traditional unix root user */

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

using namespace std::placeholders;

using ::android::base::EqualsIgnoreCase;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;

constexpr std::chrono::milliseconds kHalEventBatchingTimeWindow(10);

const VehiclePropValue kEmptyValue{};

/**
 * Indicates what's the maximum size of hidl_vec<VehiclePropValue> we want
 * to store in reusable object pool.
 */
constexpr auto kMaxHidlVecOfVehiclPropValuePoolSize = 20;

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

bool VehicleHalManager::safelyParseInt(int fd, int index, std::string s, int* out) {
    if (!android::base::ParseInt(s, out)) {
        dprintf(fd, "non-integer argument at index %d: %s\n", index, s.c_str());
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
    // TODO: support other formats (int64, float, bytes)
    dprintf(fd,
            "--set <PROP> <i|s> <VALUE_1> [<i|s> <VALUE_N>] [a AREA_ID] : sets the value of "
            "property PROP, using arbitrary number of key/value parameters (i for int32, "
            "s for string) and an optional area.\n"
            "Notice that the string value can be set just once, while the other can have multiple "
            "values (so they're used in the respective array)\n");
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
    auto callback = [&](StatusCode status, const VehiclePropValue& output) {
        if (status == StatusCode::OK) {
            dprintf(fd, "%s\n", toString(output).c_str());
        } else {
            dprintf(fd, "Could not get property %d. Error: %s\n", prop, toString(status).c_str());
        }
    };
    get(input, callback);
}

void VehicleHalManager::cmdSetOneProperty(int fd, const hidl_vec<hidl_string>& options) {
    if (!checkCallerHasWritePermissions(fd) || !checkArgumentsSize(fd, options, 3)) return;

    size_t size = options.size();

    // Syntax is --set PROP Type1 Value1 TypeN ValueN, so number of arguments must be even
    if (size % 2 != 0) {
        dprintf(fd, "must pass even number of arguments (passed %zu)\n", size);
        return;
    }
    int numberValues = (size - 2) / 2;

    VehiclePropValue prop;
    if (!safelyParseInt(fd, 1, options[1], &prop.prop)) return;
    prop.timestamp = elapsedRealtimeNano();
    prop.status = VehiclePropertyStatus::AVAILABLE;

    // First pass: calculate sizes
    int sizeInt32 = 0;
    int stringIndex = 0;
    int areaIndex = 0;
    for (int i = 2, kv = 1; kv <= numberValues; kv++) {
        // iterate through the kv=1..n key/value pairs, accessing indexes i / i+1 at each step
        std::string type = options[i];
        std::string value = options[i + 1];
        if (EqualsIgnoreCase(type, "i")) {
            sizeInt32++;
        } else if (EqualsIgnoreCase(type, "s")) {
            if (stringIndex != 0) {
                dprintf(fd,
                        "defining string value (%s) again at index %d (already defined at %d=%s"
                        ")\n",
                        value.c_str(), i, stringIndex, options[stringIndex + 1].c_str());
                return;
            }
            stringIndex = i;
        } else if (EqualsIgnoreCase(type, "a")) {
            if (areaIndex != 0) {
                dprintf(fd,
                        "defining area value (%s) again at index %d (already defined at %d=%s"
                        ")\n",
                        value.c_str(), i, areaIndex, options[areaIndex + 1].c_str());
                return;
            }
            areaIndex = i;
        } else {
            dprintf(fd, "invalid (%s) type at index %d\n", type.c_str(), i);
            return;
        }
        i += 2;
    }
    prop.value.int32Values.resize(sizeInt32);

    // Second pass: populate it
    int indexInt32 = 0;
    for (int i = 2, kv = 1; kv <= numberValues; kv++) {
        // iterate through the kv=1..n key/value pairs, accessing indexes i / i+1 at each step
        int valueIndex = i + 1;
        std::string type = options[i];
        std::string value = options[valueIndex];
        if (EqualsIgnoreCase(type, "i")) {
            int safeInt;
            if (!safelyParseInt(fd, valueIndex, value, &safeInt)) return;
            prop.value.int32Values[indexInt32++] = safeInt;
        } else if (EqualsIgnoreCase(type, "s")) {
            prop.value.stringValue = value;
        } else if (EqualsIgnoreCase(type, "a")) {
            if (!safelyParseInt(fd, valueIndex, value, &prop.areaId)) return;
        }
        i += 2;
    }
    ALOGD("Setting prop %s", toString(prop).c_str());
    auto status = set(prop);
    if (status == StatusCode::OK) {
        dprintf(fd, "Set property %s\n", toString(prop).c_str());
    } else {
        dprintf(fd, "Failed to set property %s: %s\n", toString(prop).c_str(),
                toString(status).c_str());
    }
}

void VehicleHalManager::init() {
    ALOGI("VehicleHalManager::init");

    mHidlVecOfVehiclePropValuePool.resize(kMaxHidlVecOfVehiclPropValuePoolSize);


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
        if (vecSize < kMaxHidlVecOfVehiclPropValuePoolSize) {
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

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
