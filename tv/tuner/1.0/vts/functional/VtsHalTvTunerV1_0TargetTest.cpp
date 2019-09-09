/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "Tuner_hidl_hal_test"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/tv/tuner/1.0/IDemux.h>
#include <android/hardware/tv/tuner/1.0/IDemuxCallback.h>
#include <android/hardware/tv/tuner/1.0/IDescrambler.h>
#include <android/hardware/tv/tuner/1.0/IFrontend.h>
#include <android/hardware/tv/tuner/1.0/IFrontendCallback.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <binder/MemoryDealer.h>
#include <fmq/MessageQueue.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>

#define WAIT_TIMEOUT 3000000000

using android::Condition;
using android::IMemory;
using android::IMemoryHeap;
using android::MemoryDealer;
using android::Mutex;
using android::sp;
using android::hardware::EventFlag;
using android::hardware::fromHeap;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::HidlMemory;
using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tv::tuner::V1_0::DemuxFilterEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterPesEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterSectionEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterStatus;
using android::hardware::tv::tuner::V1_0::DemuxFilterType;
using android::hardware::tv::tuner::V1_0::DemuxQueueNotifyBits;
using android::hardware::tv::tuner::V1_0::FrontendAtscModulation;
using android::hardware::tv::tuner::V1_0::FrontendAtscSettings;
using android::hardware::tv::tuner::V1_0::FrontendDvbtSettings;
using android::hardware::tv::tuner::V1_0::FrontendEventType;
using android::hardware::tv::tuner::V1_0::FrontendId;
using android::hardware::tv::tuner::V1_0::FrontendInnerFec;
using android::hardware::tv::tuner::V1_0::FrontendSettings;
using android::hardware::tv::tuner::V1_0::IDemux;
using android::hardware::tv::tuner::V1_0::IDemuxCallback;
using android::hardware::tv::tuner::V1_0::IDescrambler;
using android::hardware::tv::tuner::V1_0::IFrontend;
using android::hardware::tv::tuner::V1_0::IFrontendCallback;
using android::hardware::tv::tuner::V1_0::ITuner;
using android::hardware::tv::tuner::V1_0::Result;

namespace {

using FilterMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using FilterMQDesc = MQDescriptorSync<uint8_t>;

const std::vector<uint8_t> goldenDataInputBuffer{
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

const uint16_t FMQ_SIZE_4K = 0x1000;
// Equal to SECTION_WRITE_COUNT on the HAL impl side
// The HAL impl will repeatedly write to the FMQ the count times
const uint16_t SECTION_READ_COUNT = 10;

class FrontendCallback : public IFrontendCallback {
  public:
    virtual Return<void> onEvent(FrontendEventType frontendEventType) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mEventReceived = true;
        mEventType = frontendEventType;
        mMsgCondition.signal();
        return Void();
    }

    virtual Return<void> onDiseqcMessage(const hidl_vec<uint8_t>& diseqcMessage) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mDiseqcMessageReceived = true;
        mEventMessage = diseqcMessage;
        mMsgCondition.signal();
        return Void();
    }

    void testOnEvent(sp<IFrontend>& frontend, FrontendSettings settings);
    void testOnDiseqcMessage(sp<IFrontend>& frontend, FrontendSettings settings);

  private:
    bool mEventReceived = false;
    bool mDiseqcMessageReceived = false;
    FrontendEventType mEventType;
    hidl_vec<uint8_t> mEventMessage;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
};

void FrontendCallback::testOnEvent(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);

    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "event not received within timeout";
            return;
        }
    }
}

void FrontendCallback::testOnDiseqcMessage(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);

    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mDiseqcMessageReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "diseqc message not received within timeout";
            return;
        }
    }
}

class DemuxCallback : public IDemuxCallback {
  public:
    virtual Return<void> onFilterEvent(const DemuxFilterEvent& filterEvent) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mFilterEventReceived = true;
        mFilterEvent = filterEvent;
        mMsgCondition.signal();
        return Void();
    }

    virtual Return<void> onFilterStatus(uint32_t /*filterId*/,
                                        const DemuxFilterStatus /*status*/) override {
        return Void();
    }

    void testOnFilterEvent(uint32_t filterId);
    void testOnSectionFilterEvent(sp<IDemux>& demux, uint32_t filterId,
                                  FilterMQDesc& filterMQDescriptor);
    void testOnPesFilterEvent(sp<IDemux>& demux, uint32_t filterId,
                              FilterMQDesc& filterMQDescriptor);
    void readAndCompareSectionEventData();
    void readAndComparePesEventData();

  private:
    bool mFilterEventReceived = false;
    std::vector<uint8_t> mDataOutputBuffer;
    std::unique_ptr<FilterMQ> mFilterMQ;
    uint16_t mDataLength = 0;
    DemuxFilterEvent mFilterEvent;
    android::Mutex mMsgLock;
    android::Mutex mReadLock;
    android::Condition mMsgCondition;
    EventFlag* mFilterMQEventFlag;
};

void DemuxCallback::testOnFilterEvent(uint32_t filterId) {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mFilterEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "filter event not received within timeout";
            return;
        }
    }
    // Reset the filter event recieved flag
    mFilterEventReceived = false;
    // Check if filter id match
    EXPECT_TRUE(filterId == mFilterEvent.filterId) << "filter id match";
}

void DemuxCallback::testOnSectionFilterEvent(sp<IDemux>& demux, uint32_t filterId,
                                             FilterMQDesc& filterMQDescriptor) {
    Result status;
    // Create MQ to read the output into the local buffer
    mFilterMQ = std::make_unique<FilterMQ>(filterMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mFilterMQ);
    // Create the EventFlag that is used to signal the HAL impl that data have been
    // read the Filter FMQ
    EXPECT_TRUE(EventFlag::createEventFlag(mFilterMQ->getEventFlagWord(), &mFilterMQEventFlag) ==
                android::OK);
    // Start filter
    status = demux->startFilter(filterId);
    EXPECT_EQ(status, Result::SUCCESS);
    // Test start filter and receive callback event
    for (int i = 0; i < SECTION_READ_COUNT; i++) {
        testOnFilterEvent(filterId);
        // checksum of mDataOutputBuffer and Input golden input
        readAndCompareSectionEventData();
    }
}

void DemuxCallback::testOnPesFilterEvent(sp<IDemux>& demux, uint32_t filterId,
                                         FilterMQDesc& filterMQDescriptor) {
    Result status;
    // Create MQ to read the output into the local buffer
    mFilterMQ = std::make_unique<FilterMQ>(filterMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mFilterMQ);
    // Create the EventFlag that is used to signal the HAL impl that data have been
    // read the Filter FMQ
    EXPECT_TRUE(EventFlag::createEventFlag(mFilterMQ->getEventFlagWord(), &mFilterMQEventFlag) ==
                android::OK);
    // Start filter
    status = demux->startFilter(filterId);
    EXPECT_EQ(status, Result::SUCCESS);
    // Test start filter and receive callback event
    testOnFilterEvent(filterId);
    // checksum of mDataOutputBuffer and Input golden input
    readAndComparePesEventData();
}

void DemuxCallback::readAndCompareSectionEventData() {
    bool result = false;
    for (int i = 0; i < mFilterEvent.events.size(); i++) {
        DemuxFilterSectionEvent event = mFilterEvent.events[i].section();
        mDataLength = event.dataLength;
        EXPECT_TRUE(mDataLength == goldenDataInputBuffer.size()) << "buffer size does not match";

        mDataOutputBuffer.resize(mDataLength);
        result = mFilterMQ->read(mDataOutputBuffer.data(), mDataLength);
        EXPECT_TRUE(result) << "can't read from Filter MQ";

        for (int i = 0; i < mDataLength; i++) {
            EXPECT_TRUE(goldenDataInputBuffer[i] == mDataOutputBuffer[i]) << "data does not match";
        }
    }
    if (result) {
        mFilterMQEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_CONSUMED));
    }
}

void DemuxCallback::readAndComparePesEventData() {
    // TODO handle multiple events in one filter callback event
    DemuxFilterPesEvent event = mFilterEvent.events[0].pes();
    mDataLength = event.dataLength;
    EXPECT_TRUE(mDataLength == goldenDataInputBuffer.size()) << "buffer size does not match";

    mDataOutputBuffer.resize(mDataLength);
    bool result = mFilterMQ->read(mDataOutputBuffer.data(), mDataLength);
    EXPECT_TRUE(result) << "can't read from Filter MQ";

    if (result) {
        mFilterMQEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_CONSUMED));
    }

    for (int i = 0; i < mDataLength; i++) {
        EXPECT_TRUE(goldenDataInputBuffer[i] == mDataOutputBuffer[i]) << "data does not match";
    }
}

// Test environment for Tuner HIDL HAL.
class TunerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static TunerHidlEnvironment* Instance() {
        static TunerHidlEnvironment* instance = new TunerHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<ITuner>(); }
};

class TunerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  public:
    virtual void SetUp() override {
        mService = ::testing::VtsHalHidlTargetTestBase::getService<ITuner>(
                TunerHidlEnvironment::Instance()->getServiceName<ITuner>());
        ASSERT_NE(mService, nullptr);
    }

    sp<ITuner> mService;

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<IFrontend> mFrontend;
    sp<FrontendCallback> mFrontendCallback;
    sp<IDescrambler> mDescrambler;
    sp<IDemux> mDemux;
    sp<DemuxCallback> mDemuxCallback;
    FilterMQDesc mFilterMQDescriptor;
    uint32_t mDemuxId;
    uint32_t mFilterId;

    ::testing::AssertionResult createFrontend(int32_t frontendId);
    ::testing::AssertionResult tuneFrontend(int32_t frontendId);
    ::testing::AssertionResult stopTuneFrontend(int32_t frontendId);
    ::testing::AssertionResult closeFrontend(int32_t frontendId);
    ::testing::AssertionResult createDemux();
    ::testing::AssertionResult createDemuxWithFrontend(int32_t frontendId);
    ::testing::AssertionResult addSectionFilterToDemux();
    ::testing::AssertionResult addPesFilterToDemux();
    ::testing::AssertionResult getFilterMQDescriptor(sp<IDemux>& demux, const uint32_t filterId);
    ::testing::AssertionResult readSectionFilterDataOutput();
    ::testing::AssertionResult readPesFilterDataOutput();
    ::testing::AssertionResult closeDemux();
    ::testing::AssertionResult createDescrambler();
    ::testing::AssertionResult closeDescrambler();
};

::testing::AssertionResult TunerHidlTest::createFrontend(int32_t frontendId) {
    Result status;

    mService->openFrontendById(frontendId, [&](Result result, const sp<IFrontend>& frontend) {
        mFrontend = frontend;
        status = result;
    });
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    mFrontendCallback = new FrontendCallback();
    auto callbackStatus = mFrontend->setCallback(mFrontendCallback);

    return ::testing::AssertionResult(callbackStatus.isOk());
}

::testing::AssertionResult TunerHidlTest::tuneFrontend(int32_t frontendId) {
    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Frontend Settings for testing
    FrontendSettings frontendSettings;
    FrontendAtscSettings frontendAtscSettings{
            .frequency = 0,
            .modulation = FrontendAtscModulation::UNDEFINED,
    };
    frontendSettings.atsc() = frontendAtscSettings;
    mFrontendCallback->testOnEvent(mFrontend, frontendSettings);

    FrontendDvbtSettings frontendDvbtSettings{
            .frequency = 0,
            .modulation = FrontendAtscModulation::UNDEFINED,
            .fec = FrontendInnerFec::FEC_UNDEFINED,
    };
    frontendSettings.dvbt(frontendDvbtSettings);
    mFrontendCallback->testOnEvent(mFrontend, frontendSettings);

    return ::testing::AssertionResult(true);
}

::testing::AssertionResult TunerHidlTest::stopTuneFrontend(int32_t frontendId) {
    Result status;
    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mFrontend->stopTune();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::closeFrontend(int32_t frontendId) {
    Result status;
    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mFrontend->close();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDemux() {
    Result status;

    mService->openDemux([&](Result result, uint32_t demuxId, const sp<IDemux>& demux) {
        mDemux = demux;
        mDemuxId = demuxId;
        status = result;
    });
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDemuxWithFrontend(int32_t frontendId) {
    Result status;

    if (createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDemux->setFrontendDataSource(frontendId);

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::addSectionFilterToDemux() {
    Result status;

    if (createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Create demux callback
    mDemuxCallback = new DemuxCallback();

    // Add section filter to the local demux
    mDemux->addFilter(DemuxFilterType::SECTION, FMQ_SIZE_4K, mDemuxCallback,
                      [&](Result result, uint32_t filterId) {
                          mFilterId = filterId;
                          status = result;
                      });

    // Add another section filter to the local demux
    mDemux->addFilter(DemuxFilterType::SECTION, FMQ_SIZE_4K, mDemuxCallback,
                      [&](Result result, uint32_t filterId) {
                          mFilterId = filterId;
                          status = result;
                      });

    // TODO Test configure the filter

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::addPesFilterToDemux() {
    Result status;

    if (createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Create demux callback
    mDemuxCallback = new DemuxCallback();

    // Add PES filter to the local demux
    mDemux->addFilter(DemuxFilterType::PES, FMQ_SIZE_4K, mDemuxCallback,
                      [&](Result result, uint32_t filterId) {
                          mFilterId = filterId;
                          status = result;
                      });

    // Add another PES filter to the local demux
    mDemux->addFilter(DemuxFilterType::PES, FMQ_SIZE_4K, mDemuxCallback,
                      [&](Result result, uint32_t filterId) {
                          mFilterId = filterId;
                          status = result;
                      });

    // TODO Test configure the filter

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::getFilterMQDescriptor(sp<IDemux>& demux,
                                                                const uint32_t filterId) {
    Result status;

    if (!demux) {
        return ::testing::AssertionFailure();
    }

    mDemux->getFilterQueueDesc(filterId, [&](Result result, const FilterMQDesc& filterMQDesc) {
        mFilterMQDescriptor = filterMQDesc;
        status = result;
    });

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::readSectionFilterDataOutput() {
    if (addSectionFilterToDemux() == ::testing::AssertionFailure() ||
        getFilterMQDescriptor(mDemux, mFilterId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Test start filter and read the output data
    mDemuxCallback->testOnSectionFilterEvent(mDemux, mFilterId, mFilterMQDescriptor);

    return ::testing::AssertionResult(true);
}

::testing::AssertionResult TunerHidlTest::readPesFilterDataOutput() {
    if (addPesFilterToDemux() == ::testing::AssertionFailure() ||
        getFilterMQDescriptor(mDemux, mFilterId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Test start filter and read the output data
    mDemuxCallback->testOnPesFilterEvent(mDemux, mFilterId, mFilterMQDescriptor);

    return ::testing::AssertionResult(true);
}

::testing::AssertionResult TunerHidlTest::closeDemux() {
    Result status;
    if (createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDemux->close();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDescrambler() {
    Result status;

    mService->openDescrambler([&](Result result, const sp<IDescrambler>& descrambler) {
        mDescrambler = descrambler;
        status = result;
    });
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    if (createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDescrambler->setDemuxSource(mDemuxId);
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    // Test if demux source can be set more than once.
    status = mDescrambler->setDemuxSource(mDemuxId);
    return ::testing::AssertionResult(status == Result::INVALID_STATE);
}

::testing::AssertionResult TunerHidlTest::closeDescrambler() {
    Result status;
    if (createDescrambler() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDescrambler->close();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

TEST_F(TunerHidlTest, CreateFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Create Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(createFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, TuneFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Tune Frontends and check callback onEvent");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(tuneFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, StopTuneFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("stopTune Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(stopTuneFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, CloseFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Close Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(closeFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, CreateDemux) {
    description("Create Demux");

    ASSERT_TRUE(createDemux());
}

TEST_F(TunerHidlTest, CreateDemuxWithFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Create Demux with Frontend");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(createDemuxWithFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, AddSectionFilterToDemux) {
    description("Add a section filter to a created demux");
    ASSERT_TRUE(addSectionFilterToDemux());
}

TEST_F(TunerHidlTest, AddPesFilterToDemux) {
    description("Add a pes filter to a created demux");
    ASSERT_TRUE(addPesFilterToDemux());
}

TEST_F(TunerHidlTest, GetFilterMQDescriptor) {
    description("Get MQ Descriptor from a created filter");
    ASSERT_TRUE(addSectionFilterToDemux());
    ASSERT_TRUE(getFilterMQDescriptor(mDemux, mFilterId));
}

TEST_F(TunerHidlTest, ReadSectionFilterOutput) {
    description("Read data output from FMQ of a Section Filter");
    ASSERT_TRUE(readSectionFilterDataOutput());
}

TEST_F(TunerHidlTest, ReadPesFilterOutput) {
    description("Read data output from FMQ of a PES Filter");
    ASSERT_TRUE(readPesFilterDataOutput());
}

TEST_F(TunerHidlTest, CloseDemux) {
    description("Close Demux");

    ASSERT_TRUE(closeDemux());
}

TEST_F(TunerHidlTest, CreateDescrambler) {
    description("Create Descrambler");

    ASSERT_TRUE(createDescrambler());
}

TEST_F(TunerHidlTest, CloseDescrambler) {
    description("Close Descrambler");

    ASSERT_TRUE(closeDescrambler());
}

}  // namespace

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(TunerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    TunerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
