/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "mediacas_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/cas/BnCasListener.h>
#include <aidl/android/hardware/cas/IMediaCasService.h>
#include <aidl/android/hardware/cas/Status.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/ParcelFileDescriptor.h>
#include <cutils/ashmem.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>

#define CLEAR_KEY_SYSTEM_ID 0xF6D8
#define INVALID_SYSTEM_ID 0
#define WAIT_TIMEOUT 3000000000

#define PROVISION_STR                                      \
    "{                                                   " \
    "  \"id\": 21140844,                                 " \
    "  \"name\": \"Test Title\",                         " \
    "  \"lowercase_organization_name\": \"Android\",     " \
    "  \"asset_key\": {                                  " \
    "  \"encryption_key\": \"nezAr3CHFrmBR9R8Tedotw==\"  " \
    "  },                                                " \
    "  \"cas_type\": 1,                                  " \
    "  \"track_types\": [ ]                              " \
    "}                                                   "

using aidl::android::hardware::common::Ashmem;
using android::Mutex;
using namespace aidl::android::hardware::cas;
using namespace ndk;
using namespace std;
using namespace testing;

const uint8_t kEcmBinaryBuffer[] = {
        0x00, 0x00, 0x01, 0xf0, 0x00, 0x50, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x46, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x27, 0x10, 0x02, 0x00,
        0x01, 0x77, 0x01, 0x42, 0x95, 0x6c, 0x0e, 0xe3, 0x91, 0xbc, 0xfd, 0x05, 0xb1, 0x60, 0x4f,
        0x17, 0x82, 0xa4, 0x86, 0x9b, 0x23, 0x56, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x27, 0x10, 0x02, 0x00, 0x01, 0x77, 0x01, 0x42, 0x95, 0x6c, 0xd7, 0x43, 0x62, 0xf8, 0x1c,
        0x62, 0x19, 0x05, 0xc7, 0x3a, 0x42, 0xcd, 0xfd, 0xd9, 0x13, 0x48,
};

const SubSample kSubSamples[] = {{162, 0}, {0, 184}, {0, 184}};

const uint8_t kInBinaryBuffer[] = {
        0x00, 0x00, 0x00, 0x01, 0x09, 0xf0, 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1e, 0xdb,
        0x01, 0x40, 0x16, 0xec, 0x04, 0x40, 0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 0x0f, 0x03,
        0xc5, 0x8b, 0xb8, 0x00, 0x00, 0x00, 0x01, 0x68, 0xca, 0x8c, 0xb2, 0x00, 0x00, 0x01, 0x06,
        0x05, 0xff, 0xff, 0x70, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8,
        0x20, 0xd9, 0x23, 0xee, 0xef, 0x78, 0x32, 0x36, 0x34, 0x20, 0x2d, 0x20, 0x63, 0x6f, 0x72,
        0x65, 0x20, 0x31, 0x34, 0x32, 0x20, 0x2d, 0x20, 0x48, 0x2e, 0x32, 0x36, 0x34, 0x2f, 0x4d,
        0x50, 0x45, 0x47, 0x2d, 0x34, 0x20, 0x41, 0x56, 0x43, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x63,
        0x20, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x6c, 0x65, 0x66, 0x74, 0x20, 0x32, 0x30, 0x30,
        0x33, 0x2d, 0x32, 0x30, 0x31, 0x34, 0x20, 0x2d, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
        0x2f, 0x77, 0x77, 0x77, 0x2e, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x6c, 0x61, 0x6e, 0x2e, 0x6f,
        0x72, 0x67, 0x2f, 0x78, 0x32, 0x36, 0x34, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x6e, 0x45, 0x21,
        0x82, 0x38, 0xf0, 0x9d, 0x7d, 0x96, 0xe6, 0x94, 0xae, 0xe2, 0x87, 0x8f, 0x04, 0x49, 0xe5,
        0xf6, 0x8c, 0x8b, 0x9a, 0x10, 0x18, 0xba, 0x94, 0xe9, 0x22, 0x31, 0x04, 0x7e, 0x60, 0x5b,
        0xc4, 0x24, 0x00, 0x90, 0x62, 0x0d, 0xdc, 0x85, 0x74, 0x75, 0x78, 0xd0, 0x14, 0x08, 0xcb,
        0x02, 0x1d, 0x7d, 0x9d, 0x34, 0xe8, 0x81, 0xb9, 0xf7, 0x09, 0x28, 0x79, 0x29, 0x8d, 0xe3,
        0x14, 0xed, 0x5f, 0xca, 0xaf, 0xf4, 0x1c, 0x49, 0x15, 0xe1, 0x80, 0x29, 0x61, 0x76, 0x80,
        0x43, 0xf8, 0x58, 0x53, 0x40, 0xd7, 0x31, 0x6d, 0x61, 0x81, 0x41, 0xe9, 0x77, 0x9f, 0x9c,
        0xe1, 0x6d, 0xf2, 0xee, 0xd9, 0xc8, 0x67, 0xd2, 0x5f, 0x48, 0x73, 0xe3, 0x5c, 0xcd, 0xa7,
        0x45, 0x58, 0xbb, 0xdd, 0x28, 0x1d, 0x68, 0xfc, 0xb4, 0xc6, 0xf6, 0x92, 0xf6, 0x30, 0x03,
        0xaa, 0xe4, 0x32, 0xf6, 0x34, 0x51, 0x4b, 0x0f, 0x8c, 0xf9, 0xac, 0x98, 0x22, 0xfb, 0x49,
        0xc8, 0xbf, 0xca, 0x8c, 0x80, 0x86, 0x5d, 0xd7, 0xa4, 0x52, 0xb1, 0xd9, 0xa6, 0x04, 0x4e,
        0xb3, 0x2d, 0x1f, 0xb8, 0x35, 0xcc, 0x45, 0x6d, 0x9c, 0x20, 0xa7, 0xa4, 0x34, 0x59, 0x72,
        0xe3, 0xae, 0xba, 0x49, 0xde, 0xd1, 0xaa, 0xee, 0x3d, 0x77, 0xfc, 0x5d, 0xc6, 0x1f, 0x9d,
        0xac, 0xc2, 0x15, 0x66, 0xb8, 0xe1, 0x54, 0x4e, 0x74, 0x93, 0xdb, 0x9a, 0x24, 0x15, 0x6e,
        0x20, 0xa3, 0x67, 0x3e, 0x5a, 0x24, 0x41, 0x5e, 0xb0, 0xe6, 0x35, 0x87, 0x1b, 0xc8, 0x7a,
        0xf9, 0x77, 0x65, 0xe0, 0x01, 0xf2, 0x4c, 0xe4, 0x2b, 0xa9, 0x64, 0x96, 0x96, 0x0b, 0x46,
        0xca, 0xea, 0x79, 0x0e, 0x78, 0xa3, 0x5f, 0x43, 0xfc, 0x47, 0x6a, 0x12, 0xfa, 0xc4, 0x33,
        0x0e, 0x88, 0x1c, 0x19, 0x3a, 0x00, 0xc3, 0x4e, 0xb5, 0xd8, 0xfa, 0x8e, 0xf1, 0xbc, 0x3d,
        0xb2, 0x7e, 0x50, 0x8d, 0x67, 0xc3, 0x6b, 0xed, 0xe2, 0xea, 0xa6, 0x1f, 0x25, 0x24, 0x7c,
        0x94, 0x74, 0x50, 0x49, 0xe3, 0xc6, 0x58, 0x2e, 0xfd, 0x28, 0xb4, 0xc6, 0x73, 0xb1, 0x53,
        0x74, 0x27, 0x94, 0x5c, 0xdf, 0x69, 0xb7, 0xa1, 0xd7, 0xf5, 0xd3, 0x8a, 0x2c, 0x2d, 0xb4,
        0x5e, 0x8a, 0x16, 0x14, 0x54, 0x64, 0x6e, 0x00, 0x6b, 0x11, 0x59, 0x8a, 0x63, 0x38, 0x80,
        0x76, 0xc3, 0xd5, 0x59, 0xf7, 0x3f, 0xd2, 0xfa, 0xa5, 0xca, 0x82, 0xff, 0x4a, 0x62, 0xf0,
        0xe3, 0x42, 0xf9, 0x3b, 0x38, 0x27, 0x8a, 0x89, 0xaa, 0x50, 0x55, 0x4b, 0x29, 0xf1, 0x46,
        0x7c, 0x75, 0xef, 0x65, 0xaf, 0x9b, 0x0d, 0x6d, 0xda, 0x25, 0x94, 0x14, 0xc1, 0x1b, 0xf0,
        0xc5, 0x4c, 0x24, 0x0e, 0x65,
};

const uint8_t kOutRefBinaryBuffer[] = {
        0x00, 0x00, 0x00, 0x01, 0x09, 0xf0, 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1e, 0xdb,
        0x01, 0x40, 0x16, 0xec, 0x04, 0x40, 0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 0x0f, 0x03,
        0xc5, 0x8b, 0xb8, 0x00, 0x00, 0x00, 0x01, 0x68, 0xca, 0x8c, 0xb2, 0x00, 0x00, 0x01, 0x06,
        0x05, 0xff, 0xff, 0x70, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8,
        0x20, 0xd9, 0x23, 0xee, 0xef, 0x78, 0x32, 0x36, 0x34, 0x20, 0x2d, 0x20, 0x63, 0x6f, 0x72,
        0x65, 0x20, 0x31, 0x34, 0x32, 0x20, 0x2d, 0x20, 0x48, 0x2e, 0x32, 0x36, 0x34, 0x2f, 0x4d,
        0x50, 0x45, 0x47, 0x2d, 0x34, 0x20, 0x41, 0x56, 0x43, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x63,
        0x20, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x6c, 0x65, 0x66, 0x74, 0x20, 0x32, 0x30, 0x30,
        0x33, 0x2d, 0x32, 0x30, 0x31, 0x34, 0x20, 0x2d, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
        0x2f, 0x77, 0x77, 0x77, 0x2e, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x6c, 0x61, 0x6e, 0x2e, 0x6f,
        0x72, 0x67, 0x2f, 0x78, 0x32, 0x36, 0x34, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x20, 0x2d, 0x20,
        0x6f, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x3a, 0x20, 0x63, 0x61, 0x62, 0x61, 0x63, 0x3d,
        0x30, 0x20, 0x72, 0x65, 0x66, 0x3d, 0x32, 0x20, 0x64, 0x65, 0x62, 0x6c, 0x6f, 0x63, 0x6b,
        0x3d, 0x31, 0x3a, 0x30, 0x3a, 0x30, 0x20, 0x61, 0x6e, 0x61, 0x6c, 0x79, 0x73, 0x65, 0x3d,
        0x30, 0x78, 0x31, 0x3a, 0x30, 0x78, 0x31, 0x31, 0x31, 0x20, 0x6d, 0x65, 0x3d, 0x68, 0x65,
        0x78, 0x20, 0x73, 0x75, 0x62, 0x6d, 0x65, 0x3d, 0x37, 0x20, 0x70, 0x73, 0x79, 0x3d, 0x31,
        0x20, 0x70, 0x73, 0x79, 0x5f, 0x72, 0x64, 0x3d, 0x31, 0x2e, 0x30, 0x30, 0x3a, 0x30, 0x2e,
        0x30, 0x30, 0x20, 0x6d, 0x69, 0x78, 0x65, 0x64, 0x5f, 0x72, 0x65, 0x66, 0x3d, 0x31, 0x20,
        0x6d, 0x65, 0x5f, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x3d, 0x31, 0x36, 0x20, 0x63, 0x68, 0x72,
        0x6f, 0x6d, 0x61, 0x5f, 0x6d, 0x65, 0x3d, 0x31, 0x20, 0x74, 0x72, 0x65, 0x6c, 0x6c, 0x69,
        0x73, 0x3d, 0x31, 0x20, 0x38, 0x78, 0x38, 0x64, 0x63, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x71,
        0x6d, 0x3d, 0x30, 0x20, 0x64, 0x65, 0x61, 0x64, 0x7a, 0x6f, 0x6e, 0x65, 0x3d, 0x32, 0x31,
        0x2c, 0x31, 0x31, 0x20, 0x66, 0x61, 0x73, 0x74, 0x5f, 0x70, 0x73, 0x6b, 0x69, 0x70, 0x3d,
        0x31, 0x20, 0x63, 0x68, 0x72, 0x6f, 0x6d, 0x61, 0x5f, 0x71, 0x70, 0x5f, 0x6f, 0x66, 0x66,
        0x73, 0x65, 0x74, 0x3d, 0x2d, 0x32, 0x20, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d,
        0x36, 0x30, 0x20, 0x6c, 0x6f, 0x6f, 0x6b, 0x61, 0x68, 0x65, 0x61, 0x64, 0x5f, 0x74, 0x68,
        0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x35, 0x20, 0x73, 0x6c, 0x69, 0x63, 0x65, 0x64, 0x5f,
        0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x30, 0x20, 0x6e, 0x72, 0x3d, 0x30, 0x20,
        0x64, 0x65, 0x63, 0x69, 0x6d, 0x61, 0x74, 0x65, 0x3d, 0x31, 0x20, 0x69, 0x6e, 0x74, 0x65,
        0x72, 0x6c, 0x61, 0x63, 0x65, 0x64, 0x3d, 0x30, 0x20, 0x62, 0x6c, 0x75, 0x72, 0x61, 0x79,
        0x5f, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74,
        0x72, 0x61, 0x69, 0x6e, 0x65, 0x64, 0x5f, 0x69, 0x6e, 0x74, 0x72, 0x61, 0x3d, 0x30, 0x20,
        0x62, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x73, 0x3d, 0x30, 0x20, 0x77, 0x65, 0x69, 0x67, 0x68,
        0x74, 0x70, 0x3d, 0x30, 0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x3d, 0x32, 0x35, 0x30,
        0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x5f, 0x6d, 0x69, 0x6e, 0x3d, 0x32, 0x35, 0x20,
        0x73, 0x63, 0x65, 0x6e, 0x65,
};

class MediaCasListener : public BnCasListener {
  public:
    virtual ScopedAStatus onEvent(int32_t event, int32_t arg,
                                  const vector<uint8_t>& data) override {
        Mutex::Autolock autoLock(mMsgLock);
        mEvent = event;
        mEventArg = arg;
        mEventData = data;

        mEventReceived = true;
        mMsgCondition.signal();
        return ScopedAStatus::ok();
    }

    virtual ScopedAStatus onSessionEvent(const vector<uint8_t>& sessionId, int32_t event,
                                         int32_t arg, const vector<uint8_t>& data) override {
        Mutex::Autolock autoLock(mMsgLock);
        mSessionId = sessionId;
        mEvent = event;
        mEventArg = arg;
        mEventData = data;

        mEventReceived = true;
        mMsgCondition.signal();
        return ScopedAStatus::ok();
    }

    virtual ScopedAStatus onStatusUpdate(StatusEvent event, int32_t arg) override {
        Mutex::Autolock autoLock(mMsgLock);
        mStatusEvent = event;
        mEventArg = arg;

        mEventReceived = true;
        mMsgCondition.signal();
        return ScopedAStatus::ok();
    }

    void testEventEcho(shared_ptr<ICas>& mediaCas, int32_t& event, int32_t& eventArg,
                       vector<uint8_t>& eventData);

    void testSessionEventEcho(shared_ptr<ICas>& mediaCas, const vector<uint8_t>& sessionId,
                              int32_t& event, int32_t& eventArg, vector<uint8_t>& eventData);

    void testStatusUpdate(shared_ptr<ICas>& mediaCas, vector<uint8_t>* sessionId,
                          SessionIntent intent, ScramblingMode mode);

  private:
    int32_t mEvent = -1;
    int32_t mEventArg = -1;
    StatusEvent mStatusEvent;
    bool mEventReceived = false;
    vector<uint8_t> mEventData;
    vector<uint8_t> mSessionId;
    Mutex mMsgLock;
    android::Condition mMsgCondition;
};

void MediaCasListener::testEventEcho(shared_ptr<ICas>& mediaCas, int32_t& event, int32_t& eventArg,
                                     vector<uint8_t>& eventData) {
    mEventReceived = false;
    auto returnStatus = mediaCas->sendEvent(event, eventArg, eventData);
    EXPECT_TRUE(returnStatus.isOk());

    Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            ADD_FAILURE() << "event not received within timeout";
            return;
        }
    }

    EXPECT_EQ(mEvent, event);
    EXPECT_EQ(mEventArg, eventArg);
    EXPECT_TRUE(mEventData == eventData);
}

void MediaCasListener::testSessionEventEcho(shared_ptr<ICas>& mediaCas,
                                            const vector<uint8_t>& sessionId, int32_t& event,
                                            int32_t& eventArg, vector<uint8_t>& eventData) {
    mEventReceived = false;
    EXPECT_TRUE(mediaCas->sendSessionEvent(sessionId, event, eventArg, eventData).isOk());

    Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            ADD_FAILURE() << "event not received within timeout";
            return;
        }
    }

    EXPECT_TRUE(mSessionId == sessionId);
    EXPECT_EQ(mEvent, event);
    EXPECT_EQ(mEventArg, eventArg);
    EXPECT_TRUE(mEventData == eventData);
}

void MediaCasListener::testStatusUpdate(shared_ptr<ICas>& mediaCas, vector<uint8_t>* sessionId,
                                        SessionIntent intent, ScramblingMode mode) {
    mEventReceived = false;
    EXPECT_TRUE(mediaCas->openSession(intent, mode, sessionId).isOk());

    Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            ADD_FAILURE() << "event not received within timeout";
            return;
        }
    }
    EXPECT_EQ(mStatusEvent, static_cast<StatusEvent>(intent));
    EXPECT_EQ(mEventArg, static_cast<int32_t>(mode));
}

class MediaCasAidlTest : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            mService = IMediaCasService::fromBinder(binder);
        } else {
            mService = nullptr;
        }
        ASSERT_NE(mService, nullptr);
    }

    shared_ptr<IMediaCasService> mService = nullptr;

  protected:
    static void description(const string& description) {
        RecordProperty("description", description);
    }

    shared_ptr<ICas> mMediaCas;
    shared_ptr<IDescrambler> mDescrambler;
    shared_ptr<MediaCasListener> mCasListener;
    typedef struct _OobInputTestParams {
        const SubSample* subSamples;
        int32_t numSubSamples;
        int64_t imemSizeActual;
        int64_t imemOffset;
        int64_t imemSize;
        int64_t srcOffset;
        int64_t dstOffset;
    } OobInputTestParams;

    AssertionResult createCasPlugin(int32_t caSystemId);
    AssertionResult openCasSessionDefault(vector<uint8_t>* sessionId);
    AssertionResult openCasSession(vector<uint8_t>* sessionId, SessionIntent intent,
                                   ScramblingMode mode);
    AssertionResult descrambleTestInputBuffer(const shared_ptr<IDescrambler>& descrambler,
                                              ScopedAStatus& descrambleStatus, uint8_t*& inMemory);
    AssertionResult descrambleTestOobInput(const shared_ptr<IDescrambler>& descrambler,
                                           ScopedAStatus& descrambleStatus,
                                           const OobInputTestParams& params);
};

AssertionResult MediaCasAidlTest::createCasPlugin(int32_t caSystemId) {
    bool isSystemIdSupported;
    auto status = mService->isSystemIdSupported(caSystemId, &isSystemIdSupported);
    bool skipDescrambler = false;
    if (!status.isOk() || !isSystemIdSupported) {
        return AssertionFailure();
    }
    bool isDescramblerSupported;
    status = mService->isDescramblerSupported(caSystemId, &isDescramblerSupported);
    if (!status.isOk() || !isDescramblerSupported) {
        ALOGI("Skip Descrambler test since it's not required in cas.");
        mDescrambler = nullptr;
        skipDescrambler = true;
    }

    mCasListener = SharedRefBase::make<MediaCasListener>();
    status = mService->createPlugin(caSystemId, mCasListener, &mMediaCas);
    if (!status.isOk()) {
        return AssertionFailure();
    }
    if (mMediaCas == nullptr) {
        return AssertionFailure();
    }

    if (skipDescrambler) {
        return AssertionSuccess();
    }

    status = mService->createDescrambler(caSystemId, &mDescrambler);
    if (!status.isOk()) {
        return AssertionFailure();
    }

    return AssertionResult(mDescrambler != nullptr);
}

AssertionResult MediaCasAidlTest::openCasSessionDefault(vector<uint8_t>* sessionId) {
    return AssertionResult(mMediaCas->openSessionDefault(sessionId).isOk());
}

AssertionResult MediaCasAidlTest::openCasSession(vector<uint8_t>* sessionId, SessionIntent intent,
                                                 ScramblingMode mode) {
    return AssertionResult(mMediaCas->openSession(intent, mode, sessionId).isOk());
}

AssertionResult MediaCasAidlTest::descrambleTestInputBuffer(
        const shared_ptr<IDescrambler>& descrambler, ScopedAStatus& descrambleStatus,
        uint8_t*& sharedMemory) {
    vector<SubSample> subSample(kSubSamples,
                                kSubSamples + (sizeof(kSubSamples) / sizeof(SubSample)));

    int size = sizeof(kInBinaryBuffer);
    auto fd = ashmem_create_region("vts-cas", size);
    if (fd < 0) {
        ALOGE("ashmem_create_region failed");
        return AssertionFailure();
    }

    sharedMemory =
            static_cast<uint8_t*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (sharedMemory == reinterpret_cast<uint8_t*>(MAP_FAILED)) {
        ALOGE("mmap failed");
        return AssertionFailure();
    }

    memcpy(sharedMemory, kInBinaryBuffer, size);

    auto dupFd = dup(fd);

    SharedBuffer srcBuffer = {.heapBase.fd = ScopedFileDescriptor(std::move(fd)),
                              .heapBase.size = size,
                              .offset = 0,
                              .size = size};

    SharedBuffer dupBuffer = {.heapBase.fd = ScopedFileDescriptor(dupFd),
                              .heapBase.size = size,
                              .offset = 0,
                              .size = size};

    DestinationBuffer dstBuffer;
    dstBuffer.set<DestinationBuffer::nonsecureMemory>(std::move(dupBuffer));

    int32_t outBytes;
    descrambleStatus = descrambler->descramble(ScramblingControl::EVENKEY /*2*/, subSample,
                                               srcBuffer, 0, dstBuffer, 0, &outBytes);
    if (!descrambleStatus.isOk()) {
        ALOGI("descramble failed, status=%d, outBytes=%u, error=%s", descrambleStatus.getStatus(),
              outBytes, descrambleStatus.getDescription().c_str());
    }
    return AssertionResult(descrambleStatus.isOk());
}

AssertionResult MediaCasAidlTest::descrambleTestOobInput(
        const shared_ptr<IDescrambler>& descrambler, ScopedAStatus& descrambleStatus,
        const OobInputTestParams& params) {
    vector<SubSample> subSample(params.subSamples, params.subSamples + params.numSubSamples);

    auto fd = ashmem_create_region("vts-cas", params.imemSizeActual);
    if (fd < 0) {
        ALOGE("ashmem_create_region failed");
        return AssertionFailure();
    }

    auto dupFd = dup(fd);

    SharedBuffer srcBuffer = {.heapBase.fd = ScopedFileDescriptor(std::move(fd)),
                              .heapBase.size = params.imemSizeActual,
                              .offset = params.imemOffset,
                              .size = params.imemSize};

    SharedBuffer dupBuffer = {.heapBase.fd = ScopedFileDescriptor(dupFd),
                              .heapBase.size = params.imemSizeActual,
                              .offset = params.imemOffset,
                              .size = params.imemSize};

    DestinationBuffer dstBuffer;
    dstBuffer.set<DestinationBuffer::nonsecureMemory>(std::move(dupBuffer));

    int32_t outBytes;
    descrambleStatus =
            descrambler->descramble(ScramblingControl::EVENKEY /*2*/, subSample, srcBuffer,
                                    params.srcOffset, dstBuffer, params.dstOffset, &outBytes);
    if (!descrambleStatus.isOk()) {
        ALOGI("descramble failed, status=%d, outBytes=%u, error=%s", descrambleStatus.getStatus(),
              outBytes, descrambleStatus.getDescription().c_str());
    }
    return AssertionResult(descrambleStatus.isOk());
}

TEST_P(MediaCasAidlTest, EnumeratePlugins) {
    description("Test enumerate plugins");

    vector<AidlCasPluginDescriptor> descriptors;
    EXPECT_TRUE(mService->enumeratePlugins(&descriptors).isOk());

    if (descriptors.size() == 0) {
        ALOGW("[   WARN   ] enumeratePlugins list empty");
        return;
    }

    for (size_t i = 0; i < descriptors.size(); i++) {
        int32_t caSystemId = descriptors[i].caSystemId;

        ASSERT_TRUE(createCasPlugin(caSystemId));
    }
}

TEST_P(MediaCasAidlTest, TestInvalidSystemIdFails) {
    description("Test failure for invalid system ID");

    bool isSystemIdSupported;
    auto status = mService->isSystemIdSupported(INVALID_SYSTEM_ID, &isSystemIdSupported);

    EXPECT_TRUE(status.isOk());
    ASSERT_FALSE(isSystemIdSupported);

    bool isDescramblerSupported;
    status = mService->isDescramblerSupported(INVALID_SYSTEM_ID, &isDescramblerSupported);

    EXPECT_TRUE(status.isOk());
    ASSERT_FALSE(isDescramblerSupported);

    shared_ptr<ICas> mediaCas;
    shared_ptr<MediaCasListener> casListener = SharedRefBase::make<MediaCasListener>();
    status = mService->createPlugin(INVALID_SYSTEM_ID, casListener, &mediaCas);
    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(mediaCas, nullptr);

    shared_ptr<IDescrambler> descrambler;
    status = mService->createDescrambler(INVALID_SYSTEM_ID, &descrambler);
    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(descrambler, nullptr);
}

TEST_P(MediaCasAidlTest, TestClearKeyPluginInstalled) {
    description("Test if ClearKey plugin is installed");

    vector<AidlCasPluginDescriptor> descriptors;
    EXPECT_TRUE(mService->enumeratePlugins(&descriptors).isOk());

    if (descriptors.size() == 0) {
        ALOGW("[   WARN   ] enumeratePlugins list empty");
    }

    for (size_t i = 0; i < descriptors.size(); i++) {
        int32_t caSystemId = descriptors[i].caSystemId;
        if (CLEAR_KEY_SYSTEM_ID == caSystemId) {
            return;
        }
    }

    ADD_FAILURE() << "ClearKey plugin not installed";
}

TEST_P(MediaCasAidlTest, TestClearKeyDefaultSessionClosedAfterRelease) {
    description("Test that all sessions are closed after a MediaCas object is released");

    ASSERT_TRUE(createCasPlugin(CLEAR_KEY_SYSTEM_ID));

    EXPECT_TRUE(mMediaCas->provision(PROVISION_STR).isOk());

    vector<uint8_t> sessionId;
    ASSERT_TRUE(openCasSessionDefault(&sessionId));

    vector<uint8_t> streamSessionId;
    ASSERT_TRUE(openCasSessionDefault(&streamSessionId));

    EXPECT_TRUE(mMediaCas->release().isOk());

    if (mDescrambler != nullptr) {
        auto status = mDescrambler->setMediaCasSession(sessionId);
        EXPECT_FALSE(status.isOk());
        EXPECT_EQ(Status::ERROR_CAS_SESSION_NOT_OPENED, status.getServiceSpecificError());

        status = mDescrambler->setMediaCasSession(streamSessionId);
        EXPECT_FALSE(status.isOk());
        EXPECT_EQ(Status::ERROR_CAS_SESSION_NOT_OPENED, status.getServiceSpecificError());
    }
}

TEST_P(MediaCasAidlTest, TestClearKeySessionClosedAfterRelease) {
    description("Test that all sessions are closed after a MediaCas object is released");

    ASSERT_TRUE(createCasPlugin(CLEAR_KEY_SYSTEM_ID));

    EXPECT_TRUE(mMediaCas->provision(PROVISION_STR).isOk());

    SessionIntent intent = SessionIntent::LIVE;
    ScramblingMode mode = ScramblingMode::DVB_CSA1;

    vector<uint8_t> sessionId;
    ASSERT_TRUE(openCasSession(&sessionId, intent, mode));

    vector<uint8_t> streamSessionId;
    ASSERT_TRUE(openCasSession(&streamSessionId, intent, mode));

    EXPECT_TRUE(mMediaCas->release().isOk());

    if (mDescrambler != nullptr) {
        auto status = mDescrambler->setMediaCasSession(sessionId);
        EXPECT_FALSE(status.isOk());
        EXPECT_EQ(Status::ERROR_CAS_SESSION_NOT_OPENED, status.getServiceSpecificError());

        status = mDescrambler->setMediaCasSession(streamSessionId);
        EXPECT_FALSE(status.isOk());
        EXPECT_EQ(Status::ERROR_CAS_SESSION_NOT_OPENED, status.getServiceSpecificError());
    }
}

TEST_P(MediaCasAidlTest, TestClearKeyErrors) {
    description("Test that invalid call sequences fail with expected error codes");

    ASSERT_TRUE(createCasPlugin(CLEAR_KEY_SYSTEM_ID));

    /*
     * Test MediaCas error codes
     */
    // Provision should fail with an invalid asset string
    auto returnStatus = mMediaCas->provision("invalid asset string");
    EXPECT_FALSE(returnStatus.isOk());
    EXPECT_EQ(Status::ERROR_CAS_NO_LICENSE, returnStatus.getServiceSpecificError());

    SessionIntent intent = SessionIntent::LIVE;
    ScramblingMode mode = ScramblingMode::DVB_CSA1;

    // Open a session, then close it so that it should become invalid
    vector<uint8_t> invalidSessionId;
    ASSERT_TRUE(openCasSession(&invalidSessionId, intent, mode));
    EXPECT_TRUE(mMediaCas->closeSession(invalidSessionId).isOk());

    // processEcm should fail with an invalid session id
    vector<uint8_t> ecm(kEcmBinaryBuffer, kEcmBinaryBuffer + sizeof(kEcmBinaryBuffer));
    returnStatus = mMediaCas->processEcm(invalidSessionId, ecm);
    EXPECT_FALSE(returnStatus.isOk());
    EXPECT_EQ(Status::ERROR_CAS_SESSION_NOT_OPENED, returnStatus.getServiceSpecificError());

    vector<uint8_t> sessionId;
    ASSERT_TRUE(openCasSession(&sessionId, intent, mode));

    // processEcm should fail without provisioning
    returnStatus = mMediaCas->processEcm(sessionId, ecm);
    EXPECT_FALSE(returnStatus.isOk());
    EXPECT_EQ(Status::ERROR_CAS_NOT_PROVISIONED, returnStatus.getServiceSpecificError());

    EXPECT_TRUE(mMediaCas->provision(PROVISION_STR).isOk());

    // processEcm should fail with ecm with bad descriptor count
    ecm[17] = 0x03;  // change the descriptor count field to 3 (invalid)
    returnStatus = mMediaCas->processEcm(sessionId, ecm);
    EXPECT_FALSE(returnStatus.isOk());
    EXPECT_EQ(Status::ERROR_CAS_UNKNOWN, returnStatus.getServiceSpecificError());

    // processEcm should fail with ecm buffer that's too short
    ecm.resize(8);
    returnStatus = mMediaCas->processEcm(sessionId, ecm);
    EXPECT_FALSE(returnStatus.isOk());
    EXPECT_EQ(Status::BAD_VALUE, returnStatus.getServiceSpecificError());

    if (mDescrambler != nullptr) {
        /*
         * Test MediaDescrambler error codes
         */
        // setMediaCasSession should fail with an invalid session id
        returnStatus = mDescrambler->setMediaCasSession(invalidSessionId);
        EXPECT_FALSE(returnStatus.isOk());
        EXPECT_EQ(Status::ERROR_CAS_SESSION_NOT_OPENED, returnStatus.getServiceSpecificError());

        // descramble should fail without a valid session
        ScopedAStatus descrambleStatus = ScopedAStatus::ok();
        uint8_t* sharedBuffer = nullptr;

        ASSERT_FALSE(descrambleTestInputBuffer(mDescrambler, descrambleStatus, sharedBuffer));
        EXPECT_EQ(Status::ERROR_CAS_DECRYPT_UNIT_NOT_INITIALIZED,
                  descrambleStatus.getServiceSpecificError());

        // Now set a valid session, should still fail because no valid ecm is processed
        EXPECT_TRUE(mDescrambler->setMediaCasSession(sessionId).isOk());
        ASSERT_FALSE(descrambleTestInputBuffer(mDescrambler, descrambleStatus, sharedBuffer));
        EXPECT_EQ(Status::ERROR_CAS_DECRYPT, descrambleStatus.getServiceSpecificError());

        // Verify that requiresSecureDecoderComponent handles empty mime
        bool requiresSecureDecoderComponent = true;
        EXPECT_TRUE(
                mDescrambler->requiresSecureDecoderComponent("", &requiresSecureDecoderComponent)
                        .isOk());
        EXPECT_FALSE(requiresSecureDecoderComponent);

        // Verify that requiresSecureDecoderComponent handles invalid mime
        requiresSecureDecoderComponent = true;
        EXPECT_TRUE(
                mDescrambler->requiresSecureDecoderComponent("bad", &requiresSecureDecoderComponent)
                        .isOk());
        EXPECT_FALSE(requiresSecureDecoderComponent);
    }
}

TEST_P(MediaCasAidlTest, TestClearKeyApisWithSession) {
    description("Test that valid call sequences with SessionEvent send and receive");

    ASSERT_TRUE(createCasPlugin(CLEAR_KEY_SYSTEM_ID));

    EXPECT_TRUE(mMediaCas->provision(PROVISION_STR).isOk());

    vector<uint8_t> pvtData;
    pvtData.resize(256);
    EXPECT_TRUE(mMediaCas->setPrivateData(pvtData).isOk());

    SessionIntent intent = SessionIntent::LIVE;
    ScramblingMode mode = ScramblingMode::DVB_CSA1;

    vector<uint8_t> sessionId;
    ASSERT_TRUE(openCasSession(&sessionId, intent, mode));
    EXPECT_TRUE(mMediaCas->setSessionPrivateData(sessionId, pvtData).isOk());

    vector<uint8_t> streamSessionId;
    ASSERT_TRUE(openCasSession(&streamSessionId, intent, mode));
    EXPECT_TRUE(mMediaCas->setSessionPrivateData(streamSessionId, pvtData).isOk());

    if (mDescrambler != nullptr) {
        EXPECT_TRUE(mDescrambler->setMediaCasSession(sessionId).isOk());
        EXPECT_TRUE(mDescrambler->setMediaCasSession(streamSessionId).isOk());
    }

    vector<uint8_t> nullPtrVector(0);
    EXPECT_TRUE(mMediaCas->refreshEntitlements(3, nullPtrVector).isOk());

    vector<uint8_t> refreshData{0, 1, 2, 3};
    EXPECT_TRUE(mMediaCas->refreshEntitlements(10, refreshData).isOk());

    int32_t eventID = 1;
    int32_t eventArg = 2;
    mCasListener->testEventEcho(mMediaCas, eventID, eventArg, nullPtrVector);
    mCasListener->testSessionEventEcho(mMediaCas, sessionId, eventID, eventArg, nullPtrVector);

    eventID = 3;
    eventArg = 4;
    vector<uint8_t> eventData{'e', 'v', 'e', 'n', 't', 'd', 'a', 't', 'a'};
    mCasListener->testEventEcho(mMediaCas, eventID, eventArg, eventData);
    mCasListener->testSessionEventEcho(mMediaCas, sessionId, eventID, eventArg, eventData);

    mCasListener->testStatusUpdate(mMediaCas, &sessionId, intent, mode);

    vector<uint8_t> clearKeyEmmData{'c', 'l', 'e', 'a', 'r', 'k', 'e', 'y', 'e', 'm', 'm'};
    EXPECT_TRUE(mMediaCas->processEmm(clearKeyEmmData).isOk());

    vector<uint8_t> ecm(kEcmBinaryBuffer, kEcmBinaryBuffer + sizeof(kEcmBinaryBuffer));
    EXPECT_TRUE(mMediaCas->processEcm(sessionId, ecm).isOk());
    EXPECT_TRUE(mMediaCas->processEcm(streamSessionId, ecm).isOk());

    if (mDescrambler != nullptr) {
        bool requiresSecureDecoderComponent = true;
        EXPECT_TRUE(mDescrambler
                            ->requiresSecureDecoderComponent("video/avc",
                                                             &requiresSecureDecoderComponent)
                            .isOk());
        EXPECT_FALSE(requiresSecureDecoderComponent);

        ScopedAStatus descrambleStatus = ScopedAStatus::ok();
        uint8_t* sharedBuffer = nullptr;

        ASSERT_TRUE(descrambleTestInputBuffer(mDescrambler, descrambleStatus, sharedBuffer));

        int compareResult =
                memcmp(static_cast<const void*>(sharedBuffer),
                       static_cast<const void*>(kOutRefBinaryBuffer), sizeof(kOutRefBinaryBuffer));
        EXPECT_EQ(0, compareResult);

        EXPECT_TRUE(mDescrambler->release().isOk());
    }

    EXPECT_TRUE(mMediaCas->release().isOk());
}

TEST_P(MediaCasAidlTest, TestClearKeyOobFails) {
    description("Test that oob descramble request fails with expected error");

    ASSERT_TRUE(createCasPlugin(CLEAR_KEY_SYSTEM_ID));
    EXPECT_TRUE(mMediaCas->provision(PROVISION_STR).isOk());

    SessionIntent intent = SessionIntent::LIVE;
    ScramblingMode mode = ScramblingMode::DVB_CSA1;

    vector<uint8_t> sessionId;
    ASSERT_TRUE(openCasSession(&sessionId, intent, mode));

    if (mDescrambler != nullptr) {
        EXPECT_TRUE(mDescrambler->setMediaCasSession(sessionId).isOk());
    }

    vector<uint8_t> ecm(kEcmBinaryBuffer, kEcmBinaryBuffer + sizeof(kEcmBinaryBuffer));
    EXPECT_TRUE(mMediaCas->processEcm(sessionId, ecm).isOk());

    if (mDescrambler != nullptr) {
        ScopedAStatus descrambleStatus = ScopedAStatus::ok();

        // test invalid src buffer offset
        ASSERT_FALSE(
                descrambleTestOobInput(mDescrambler, descrambleStatus,
                                       {.subSamples = kSubSamples,
                                        .numSubSamples = sizeof(kSubSamples) / sizeof(SubSample),
                                        .imemSizeActual = sizeof(kInBinaryBuffer),
                                        .imemOffset = 0xcccccc,
                                        .imemSize = sizeof(kInBinaryBuffer),
                                        .srcOffset = 0,
                                        .dstOffset = 0}));
        EXPECT_EQ(Status::BAD_VALUE, descrambleStatus.getServiceSpecificError());

        // test invalid src buffer size
        ASSERT_FALSE(
                descrambleTestOobInput(mDescrambler, descrambleStatus,
                                       {.subSamples = kSubSamples,
                                        .numSubSamples = sizeof(kSubSamples) / sizeof(SubSample),
                                        .imemSizeActual = sizeof(kInBinaryBuffer),
                                        .imemOffset = 0,
                                        .imemSize = 0xcccccc,
                                        .srcOffset = 0,
                                        .dstOffset = 0}));
        EXPECT_EQ(Status::BAD_VALUE, descrambleStatus.getServiceSpecificError());

        // test invalid src buffer size
        ASSERT_FALSE(
                descrambleTestOobInput(mDescrambler, descrambleStatus,
                                       {.subSamples = kSubSamples,
                                        .numSubSamples = sizeof(kSubSamples) / sizeof(SubSample),
                                        .imemSizeActual = sizeof(kInBinaryBuffer),
                                        .imemOffset = 1,
                                        .imemSize = -1,
                                        .srcOffset = 0,
                                        .dstOffset = 0}));
        EXPECT_EQ(Status::BAD_VALUE, descrambleStatus.getServiceSpecificError());

        // test invalid srcOffset
        ASSERT_FALSE(
                descrambleTestOobInput(mDescrambler, descrambleStatus,
                                       {.subSamples = kSubSamples,
                                        .numSubSamples = sizeof(kSubSamples) / sizeof(SubSample),
                                        .imemSizeActual = sizeof(kInBinaryBuffer),
                                        .imemOffset = 0,
                                        .imemSize = sizeof(kInBinaryBuffer),
                                        .srcOffset = 0xcccccc,
                                        .dstOffset = 0}));
        EXPECT_EQ(Status::BAD_VALUE, descrambleStatus.getServiceSpecificError());

        // test invalid dstOffset
        ASSERT_FALSE(
                descrambleTestOobInput(mDescrambler, descrambleStatus,
                                       {.subSamples = kSubSamples,
                                        .numSubSamples = sizeof(kSubSamples) / sizeof(SubSample),
                                        .imemSizeActual = sizeof(kInBinaryBuffer),
                                        .imemOffset = 0,
                                        .imemSize = sizeof(kInBinaryBuffer),
                                        .srcOffset = 0,
                                        .dstOffset = 0xcccccc}));
        EXPECT_EQ(Status::BAD_VALUE, descrambleStatus.getServiceSpecificError());

        // test detection of oob subsample sizes
        const SubSample invalidSubSamples1[] = {{162, 0}, {0, 184}, {0, 0xdddddd}};

        ASSERT_FALSE(descrambleTestOobInput(
                mDescrambler, descrambleStatus,
                {.subSamples = invalidSubSamples1,
                 .numSubSamples = sizeof(invalidSubSamples1) / sizeof(SubSample),
                 .imemSizeActual = sizeof(kInBinaryBuffer),
                 .imemOffset = 0,
                 .imemSize = sizeof(kInBinaryBuffer),
                 .srcOffset = 0,
                 .dstOffset = 0}));
        EXPECT_EQ(Status::BAD_VALUE, descrambleStatus.getServiceSpecificError());

        // test detection of overflowing subsample sizes
        const SubSample invalidSubSamples2[] = {{162, 0}, {0, 184}, {2, -1}};

        ASSERT_FALSE(descrambleTestOobInput(
                mDescrambler, descrambleStatus,
                {.subSamples = invalidSubSamples2,
                 .numSubSamples = sizeof(invalidSubSamples2) / sizeof(SubSample),
                 .imemSizeActual = sizeof(kInBinaryBuffer),
                 .imemOffset = 0,
                 .imemSize = sizeof(kInBinaryBuffer),
                 .srcOffset = 0,
                 .dstOffset = 0}));
        EXPECT_EQ(Status::BAD_VALUE, descrambleStatus.getServiceSpecificError());

        EXPECT_TRUE(mDescrambler->release().isOk());
    }
    EXPECT_TRUE(mMediaCas->release().isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MediaCasAidlTest);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, MediaCasAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IMediaCasService::descriptor)),
        android::PrintInstanceNameToString);

// Start thread pool to receive callbacks from AIDL service.
int main(int argc, char** argv) {
    InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
