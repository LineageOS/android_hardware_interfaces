/*
 * Copyright 2020 The Android Open Source Project
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

#include "FrontendTests.h"

Return<void> FrontendCallback::onEvent(FrontendEventType /*frontendEventType*/) {
    return Void();
}

Return<void> FrontendCallback::onScanMessage(FrontendScanMessageType /*type*/,
                                             const FrontendScanMessage& /*message*/) {
    return Void();
}

AssertionResult FrontendTests::getFrontendIds() {
    Result status;
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        mFeIds = frontendIds;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::getFrontendInfo(uint32_t frontendId) {
    Result status;
    mService->getFrontendInfo(frontendId, [&](Result result, const FrontendInfo& frontendInfo) {
        mFrontendInfo = frontendInfo;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::openFrontendById(uint32_t frontendId) {
    Result status;
    mService->openFrontendById(frontendId, [&](Result result, const sp<IFrontend>& frontend) {
        mFrontend = frontend;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FrontendTests::setFrontendCallback() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    mFrontendCallback = new FrontendCallback();
    auto callbackStatus = mFrontend->setCallback(mFrontendCallback);
    return AssertionResult(callbackStatus.isOk());
}

AssertionResult FrontendTests::closeFrontend() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontendById first.";
    Result status;
    status = mFrontend->close();
    mFrontend = nullptr;
    mFrontendCallback = nullptr;
    return AssertionResult(status == Result::SUCCESS);
}

void FrontendTests::getFrontendIdByType(FrontendType feType, uint32_t& feId) {
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);
    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != feType) {
            continue;
        }
        feId = mFeIds[i];
        return;
    }
    feId = INVALID_ID;
}