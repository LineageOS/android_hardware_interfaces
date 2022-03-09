/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <composer-vts/2.1/GraphicsComposerCallback.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace vts {

void GraphicsComposerCallback::setVsyncAllowed(bool allowed) {
    std::lock_guard<std::mutex> lock(mMutex);
    mVsyncAllowed = allowed;
}

std::vector<Display> GraphicsComposerCallback::getDisplays() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mDisplays;
}

int GraphicsComposerCallback::getInvalidHotplugCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidHotplugCount;
}

int GraphicsComposerCallback::getInvalidRefreshCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidRefreshCount;
}

int GraphicsComposerCallback::getInvalidVsyncCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidVsyncCount;
}

Return<void> GraphicsComposerCallback::onHotplug(Display display, Connection connection) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find(mDisplays.begin(), mDisplays.end(), display);
    if (connection == Connection::CONNECTED) {
        if (it == mDisplays.end()) {
            mDisplays.push_back(display);
        } else {
            mInvalidHotplugCount++;
        }
    } else if (connection == Connection::DISCONNECTED) {
        if (it != mDisplays.end()) {
            mDisplays.erase(it);
        } else {
            mInvalidHotplugCount++;
        }
    }

    return Void();
}

Return<void> GraphicsComposerCallback::onRefresh(Display display) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find(mDisplays.begin(), mDisplays.end(), display);
    if (it == mDisplays.end()) {
        mInvalidRefreshCount++;
    }

    return Void();
}

Return<void> GraphicsComposerCallback::onVsync(Display display, int64_t) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find(mDisplays.begin(), mDisplays.end(), display);
    if (!mVsyncAllowed || it == mDisplays.end()) {
        mInvalidVsyncCount++;
    }

    return Void();
}

}  // namespace vts
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
