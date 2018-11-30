/*
 * Copyright 2017 The Android Open Source Project
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

#ifndef ANDROID_SF_HWC2_ON_FB_ADAPTER_H
#define ANDROID_SF_HWC2_ON_FB_ADAPTER_H

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11

struct framebuffer_device_t;

namespace android {

class HWC2OnFbAdapter : public hwc2_device_t {
public:
    HWC2OnFbAdapter(framebuffer_device_t* fbDevice);

    static HWC2OnFbAdapter& cast(hw_device_t* device);
    static HWC2OnFbAdapter& cast(hwc2_device_t* device);

    static hwc2_display_t getDisplayId();
    static hwc2_config_t getConfigId();

    void close();

    struct Info {
        std::string name;
        uint32_t width;
        uint32_t height;
        int format;
        int vsync_period_ns;
        int xdpi_scaled;
        int ydpi_scaled;
    };
    const Info& getInfo() const;

    void updateDebugString();
    const std::string& getDebugString() const;

    enum class State {
        MODIFIED,
        VALIDATED_WITH_CHANGES,
        VALIDATED,
    };
    void setState(State state);
    State getState() const;

    hwc2_layer_t addLayer();
    bool removeLayer(hwc2_layer_t layer);
    bool hasLayer(hwc2_layer_t layer) const;
    bool markLayerDirty(hwc2_layer_t layer, bool dirty);
    const std::unordered_set<hwc2_layer_t>& getDirtyLayers() const;
    void clearDirtyLayers();

    void setBuffer(buffer_handle_t buffer);
    bool postBuffer();

    void setVsyncCallback(HWC2_PFN_VSYNC callback, hwc2_callback_data_t data);
    void enableVsync(bool enable);
    void getCapabilities(uint32_t* outCount, int32_t* outCapabilities);

private:
    framebuffer_device_t* mFbDevice{nullptr};
    Info mFbInfo{};

    std::string mDebugString;

    State mState{State::MODIFIED};

    uint64_t mNextLayerId{0};
    std::unordered_set<hwc2_layer_t> mLayers;
    std::unordered_set<hwc2_layer_t> mDirtyLayers;

    buffer_handle_t mBuffer{nullptr};

    std::unordered_set<HWC2::Capability> mCapabilities;

    class VsyncThread {
    public:
        static int64_t now();
        static bool sleepUntil(int64_t t);

        void start(int64_t first, int64_t period);
        void stop();
        void setCallback(HWC2_PFN_VSYNC callback, hwc2_callback_data_t data);
        void enableCallback(bool enable);

    private:
        void vsyncLoop();
        bool waitUntilNextVsync();

        std::thread mThread;
        int64_t mNextVsync{0};
        int64_t mPeriod{0};

        std::mutex mMutex;
        std::condition_variable mCondition;
        bool mStarted{false};
        HWC2_PFN_VSYNC mCallback{nullptr};
        hwc2_callback_data_t mCallbackData{nullptr};
        bool mCallbackEnabled{false};
    };
    VsyncThread mVsyncThread;
};

} // namespace android

#endif // ANDROID_SF_HWC2_ON_FB_ADAPTER_H
