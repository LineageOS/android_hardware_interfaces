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

#define LOG_TAG "BcRadio.vts"
#define LOG_NDEBUG 0
#define EGMOCK_VERBOSE 1

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android/hardware/broadcastradio/2.0/IBroadcastRadio.h>
#include <android/hardware/broadcastradio/2.0/ITunerCallback.h>
#include <android/hardware/broadcastradio/2.0/ITunerSession.h>
#include <android/hardware/broadcastradio/2.0/types.h>
#include <broadcastradio-utils-2x/Utils.h>
#include <broadcastradio-vts-utils/call-barrier.h>
#include <broadcastradio-vts-utils/environment-utils.h>
#include <broadcastradio-vts-utils/mock-timeout.h>
#include <broadcastradio-vts-utils/pointer-utils.h>
#include <cutils/bitops.h>
#include <gmock/gmock.h>

#include <chrono>
#include <optional>
#include <regex>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V2_0 {
namespace vts {

using namespace std::chrono_literals;

using std::unordered_set;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::ByMove;
using testing::DoAll;
using testing::Invoke;
using testing::SaveArg;

using broadcastradio::vts::BroadcastRadioHidlEnvironment;
using broadcastradio::vts::CallBarrier;
using broadcastradio::vts::clearAndWait;
using utils::make_identifier;
using utils::make_selector_amfm;

namespace timeout {

static constexpr auto tune = 30s;
static constexpr auto programListScan = 5min;

}  // namespace timeout

static constexpr auto gTuneWorkaround = 200ms;

static const ConfigFlag gConfigFlagValues[] = {
    ConfigFlag::FORCE_MONO,
    ConfigFlag::FORCE_ANALOG,
    ConfigFlag::FORCE_DIGITAL,
    ConfigFlag::RDS_AF,
    ConfigFlag::RDS_REG,
    ConfigFlag::DAB_DAB_LINKING,
    ConfigFlag::DAB_FM_LINKING,
    ConfigFlag::DAB_DAB_SOFT_LINKING,
    ConfigFlag::DAB_FM_SOFT_LINKING,
};

class TunerCallbackMock : public ITunerCallback {
   public:
    TunerCallbackMock();

    MOCK_METHOD2(onTuneFailed, Return<void>(Result, const ProgramSelector&));
    MOCK_TIMEOUT_METHOD1(onCurrentProgramInfoChanged_, Return<void>(const ProgramInfo&));
    virtual Return<void> onCurrentProgramInfoChanged(const ProgramInfo& info);
    Return<void> onProgramListUpdated(const ProgramListChunk& chunk);
    MOCK_METHOD1(onAntennaStateChange, Return<void>(bool connected));
    MOCK_METHOD1(onParametersUpdated, Return<void>(const hidl_vec<VendorKeyValue>& parameters));

    MOCK_TIMEOUT_METHOD0(onProgramListReady, void());

    std::mutex mLock;
    utils::ProgramInfoSet mProgramList;
};

struct AnnouncementListenerMock : public IAnnouncementListener {
    MOCK_METHOD1(onListUpdated, Return<void>(const hidl_vec<Announcement>&));
};

static BroadcastRadioHidlEnvironment<IBroadcastRadio>* gEnv = nullptr;

class BroadcastRadioHalTest : public ::testing::VtsHalHidlTargetTestBase {
   protected:
    virtual void SetUp() override;
    virtual void TearDown() override;

    bool openSession();
    bool getAmFmRegionConfig(bool full, AmFmRegionConfig* config);
    std::optional<utils::ProgramInfoSet> getProgramList();

    sp<IBroadcastRadio> mModule;
    Properties mProperties;
    sp<ITunerSession> mSession;
    sp<TunerCallbackMock> mCallback = new TunerCallbackMock();
};

static void printSkipped(std::string msg) {
    std::cout << "[  SKIPPED ] " << msg << std::endl;
}

MATCHER_P(InfoHasId, id,
          std::string(negation ? "does not contain" : "contains") + " " + toString(id)) {
    auto ids = utils::getAllIds(arg.selector, utils::getType(id));
    return ids.end() != find(ids.begin(), ids.end(), id.value);
}

TunerCallbackMock::TunerCallbackMock() {
    EXPECT_TIMEOUT_CALL(*this, onCurrentProgramInfoChanged_, _).Times(AnyNumber());

    // we expect the antenna is connected through the whole test
    EXPECT_CALL(*this, onAntennaStateChange(false)).Times(0);
}

Return<void> TunerCallbackMock::onCurrentProgramInfoChanged(const ProgramInfo& info) {
    for (auto&& id : info.selector) {
        EXPECT_NE(IdentifierType::INVALID, utils::getType(id));
    }

    auto logically = utils::getType(info.logicallyTunedTo);
    /* This field is required for currently tuned program and should be INVALID
     * for entries from the program list.
     */
    EXPECT_TRUE(
        logically == IdentifierType::AMFM_FREQUENCY || logically == IdentifierType::RDS_PI ||
        logically == IdentifierType::HD_STATION_ID_EXT ||
        logically == IdentifierType::DAB_SID_EXT || logically == IdentifierType::DRMO_SERVICE_ID ||
        logically == IdentifierType::SXM_SERVICE_ID ||
        (logically >= IdentifierType::VENDOR_START && logically <= IdentifierType::VENDOR_END) ||
        logically > IdentifierType::SXM_CHANNEL);

    auto physically = utils::getType(info.physicallyTunedTo);
    // ditto (see "logically" above)
    EXPECT_TRUE(
        physically == IdentifierType::AMFM_FREQUENCY ||
        physically == IdentifierType::DAB_ENSEMBLE ||
        physically == IdentifierType::DRMO_FREQUENCY || physically == IdentifierType::SXM_CHANNEL ||
        (physically >= IdentifierType::VENDOR_START && physically <= IdentifierType::VENDOR_END) ||
        physically > IdentifierType::SXM_CHANNEL);

    if (logically == IdentifierType::AMFM_FREQUENCY) {
        auto ps = utils::getMetadataString(info, MetadataKey::RDS_PS);
        if (ps.has_value()) {
            EXPECT_NE("", android::base::Trim(*ps))
                << "Don't use empty RDS_PS as an indicator of missing RSD PS data.";
        }
    }

    return onCurrentProgramInfoChanged_(info);
}

Return<void> TunerCallbackMock::onProgramListUpdated(const ProgramListChunk& chunk) {
    std::lock_guard<std::mutex> lk(mLock);

    updateProgramList(mProgramList, chunk);

    if (chunk.complete) onProgramListReady();

    return {};
}

void BroadcastRadioHalTest::SetUp() {
    EXPECT_EQ(nullptr, mModule.get()) << "Module is already open";

    // lookup HIDL service (radio module)
    mModule = getService<IBroadcastRadio>(gEnv->getServiceName<IBroadcastRadio>());
    ASSERT_NE(nullptr, mModule.get()) << "Couldn't find broadcast radio HAL implementation";

    // get module properties
    auto propResult = mModule->getProperties([&](const Properties& p) { mProperties = p; });
    ASSERT_TRUE(propResult.isOk());

    EXPECT_FALSE(mProperties.maker.empty());
    EXPECT_FALSE(mProperties.product.empty());
    EXPECT_GT(mProperties.supportedIdentifierTypes.size(), 0u);
}

void BroadcastRadioHalTest::TearDown() {
    mSession.clear();
    mModule.clear();
    clearAndWait(mCallback, 1s);
}

bool BroadcastRadioHalTest::openSession() {
    EXPECT_EQ(nullptr, mSession.get()) << "Session is already open";

    Result halResult = Result::UNKNOWN_ERROR;
    auto openCb = [&](Result result, const sp<ITunerSession>& session) {
        halResult = result;
        if (result != Result::OK) return;
        mSession = session;
    };
    auto hidlResult = mModule->openSession(mCallback, openCb);

    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, halResult);
    EXPECT_NE(nullptr, mSession.get());

    return nullptr != mSession.get();
}

bool BroadcastRadioHalTest::getAmFmRegionConfig(bool full, AmFmRegionConfig* config) {
    auto halResult = Result::UNKNOWN_ERROR;
    auto cb = [&](Result result, AmFmRegionConfig configCb) {
        halResult = result;
        if (config) *config = configCb;
    };

    auto hidlResult = mModule->getAmFmRegionConfig(full, cb);
    EXPECT_TRUE(hidlResult.isOk());

    if (halResult == Result::NOT_SUPPORTED) return false;

    EXPECT_EQ(Result::OK, halResult);
    return halResult == Result::OK;
}

std::optional<utils::ProgramInfoSet> BroadcastRadioHalTest::getProgramList() {
    EXPECT_TIMEOUT_CALL(*mCallback, onProgramListReady).Times(AnyNumber());

    auto startResult = mSession->startProgramListUpdates({});
    if (startResult == Result::NOT_SUPPORTED) {
        printSkipped("Program list not supported");
        return nullopt;
    }
    EXPECT_EQ(Result::OK, startResult);
    if (startResult != Result::OK) return nullopt;

    EXPECT_TIMEOUT_CALL_WAIT(*mCallback, onProgramListReady, timeout::programListScan);

    auto stopResult = mSession->stopProgramListUpdates();
    EXPECT_TRUE(stopResult.isOk());

    return mCallback->mProgramList;
}

/**
 * Test session opening.
 *
 * Verifies that:
 *  - the method succeeds on a first and subsequent calls;
 *  - the method succeeds when called for the second time without
 *    closing previous session.
 */
TEST_F(BroadcastRadioHalTest, OpenSession) {
    // simply open session for the first time
    ASSERT_TRUE(openSession());

    // drop (without explicit close) and re-open the session
    mSession.clear();
    ASSERT_TRUE(openSession());

    // open the second session (the first one should be forcibly closed)
    auto secondSession = mSession;
    mSession.clear();
    ASSERT_TRUE(openSession());
}

static bool isValidAmFmFreq(uint64_t freq) {
    auto id = utils::make_identifier(IdentifierType::AMFM_FREQUENCY, freq);
    return utils::isValid(id);
}

static void validateRange(const AmFmBandRange& range) {
    EXPECT_TRUE(isValidAmFmFreq(range.lowerBound));
    EXPECT_TRUE(isValidAmFmFreq(range.upperBound));
    EXPECT_LT(range.lowerBound, range.upperBound);
    EXPECT_GT(range.spacing, 0u);
    EXPECT_EQ(0u, (range.upperBound - range.lowerBound) % range.spacing);
}

static bool supportsFM(const AmFmRegionConfig& config) {
    for (auto&& range : config.ranges) {
        if (utils::getBand(range.lowerBound) == utils::FrequencyBand::FM) return true;
    }
    return false;
}

/**
 * Test fetching AM/FM regional configuration.
 *
 * Verifies that:
 *  - AM/FM regional configuration is either set at startup or not supported at all by the hardware;
 *  - there is at least one AM/FM band configured;
 *  - FM Deemphasis and RDS are correctly configured for FM-capable radio;
 *  - all channel grids (frequency ranges and spacings) are valid;
 *  - seek spacing is a multiple of the manual spacing value.
 */
TEST_F(BroadcastRadioHalTest, GetAmFmRegionConfig) {
    AmFmRegionConfig config;
    bool supported = getAmFmRegionConfig(false, &config);
    if (!supported) {
        printSkipped("AM/FM not supported");
        return;
    }

    EXPECT_GT(config.ranges.size(), 0u);
    EXPECT_LE(popcountll(config.fmDeemphasis), 1);
    EXPECT_LE(popcountll(config.fmRds), 1);

    for (auto&& range : config.ranges) {
        validateRange(range);
        EXPECT_EQ(0u, range.scanSpacing % range.spacing);
        EXPECT_GE(range.scanSpacing, range.spacing);
    }

    if (supportsFM(config)) {
        EXPECT_EQ(popcountll(config.fmDeemphasis), 1);
    }
}

/**
 * Test fetching AM/FM regional capabilities.
 *
 * Verifies that:
 *  - AM/FM regional capabilities are either available or not supported at all by the hardware;
 *  - there is at least one AM/FM range supported;
 *  - there is at least one de-emphasis filter mode supported for FM-capable radio;
 *  - all channel grids (frequency ranges and spacings) are valid;
 *  - seek spacing is not set.
 */
TEST_F(BroadcastRadioHalTest, GetAmFmRegionConfigCapabilities) {
    AmFmRegionConfig config;
    bool supported = getAmFmRegionConfig(true, &config);
    if (!supported) {
        printSkipped("AM/FM not supported");
        return;
    }

    EXPECT_GT(config.ranges.size(), 0u);

    for (auto&& range : config.ranges) {
        validateRange(range);
        EXPECT_EQ(0u, range.scanSpacing);
    }

    if (supportsFM(config)) {
        EXPECT_GE(popcountll(config.fmDeemphasis), 1);
    }
}

/**
 * Test fetching DAB regional configuration.
 *
 * Verifies that:
 *  - DAB regional configuration is either set at startup or not supported at all by the hardware;
 *  - all channel labels match correct format;
 *  - all channel frequencies are in correct range.
 */
TEST_F(BroadcastRadioHalTest, GetDabRegionConfig) {
    Result halResult;
    hidl_vec<DabTableEntry> config;
    auto cb = [&](Result result, hidl_vec<DabTableEntry> configCb) {
        halResult = result;
        config = configCb;
    };
    auto hidlResult = mModule->getDabRegionConfig(cb);
    ASSERT_TRUE(hidlResult.isOk());

    if (halResult == Result::NOT_SUPPORTED) {
        printSkipped("DAB not supported");
        return;
    }
    ASSERT_EQ(Result::OK, halResult);

    std::regex re("^[A-Z0-9][A-Z0-9 ]{0,5}[A-Z0-9]$");
    // double-check correctness of the test
    ASSERT_TRUE(std::regex_match("5A", re));
    ASSERT_FALSE(std::regex_match("5a", re));
    ASSERT_FALSE(std::regex_match("1234ABCD", re));
    ASSERT_TRUE(std::regex_match("CN 12D", re));
    ASSERT_FALSE(std::regex_match(" 5A", re));

    for (auto&& entry : config) {
        EXPECT_TRUE(std::regex_match(std::string(entry.label), re));

        auto id = utils::make_identifier(IdentifierType::DAB_FREQUENCY, entry.frequency);
        EXPECT_TRUE(utils::isValid(id));
    }
}

/**
 * Test tuning with FM selector.
 *
 * Verifies that:
 *  - if AM/FM selector is not supported, the method returns NOT_SUPPORTED;
 *  - if it is supported, the method succeeds;
 *  - after a successful tune call, onCurrentProgramInfoChanged callback is
 *    invoked carrying a proper selector;
 *  - program changes exactly to what was requested.
 */
TEST_F(BroadcastRadioHalTest, FmTune) {
    ASSERT_TRUE(openSession());

    uint64_t freq = 100100;  // 100.1 FM
    auto sel = make_selector_amfm(freq);

    /* TODO(b/69958777): there is a race condition between tune() and onCurrentProgramInfoChanged
     * callback setting infoCb, because egmock cannot distinguish calls with different matchers
     * (there is one here and one in callback constructor).
     *
     * This sleep workaround will fix default implementation, but the real HW tests will still be
     * flaky. We probably need to implement egmock alternative based on actions.
     */
    std::this_thread::sleep_for(gTuneWorkaround);

    // try tuning
    ProgramInfo infoCb = {};
    EXPECT_TIMEOUT_CALL(*mCallback, onCurrentProgramInfoChanged_,
                        InfoHasId(utils::make_identifier(IdentifierType::AMFM_FREQUENCY, freq)))
        .Times(AnyNumber())
        .WillOnce(DoAll(SaveArg<0>(&infoCb), testing::Return(ByMove(Void()))));
    auto result = mSession->tune(sel);

    // expect a failure if it's not supported
    if (!utils::isSupported(mProperties, sel)) {
        EXPECT_EQ(Result::NOT_SUPPORTED, result);
        return;
    }

    // expect a callback if it succeeds
    EXPECT_EQ(Result::OK, result);
    EXPECT_TIMEOUT_CALL_WAIT(*mCallback, onCurrentProgramInfoChanged_, timeout::tune);

    ALOGD("current program info: %s", toString(infoCb).c_str());

    // it should tune exactly to what was requested
    auto freqs = utils::getAllIds(infoCb.selector, IdentifierType::AMFM_FREQUENCY);
    EXPECT_NE(freqs.end(), find(freqs.begin(), freqs.end(), freq));
}

/**
 * Test tuning with invalid selectors.
 *
 * Verifies that:
 *  - if the selector is not supported, it's ignored;
 *  - if it is supported, an invalid value results with INVALID_ARGUMENTS;
 */
TEST_F(BroadcastRadioHalTest, TuneFailsWithInvalid) {
    ASSERT_TRUE(openSession());

    vector<ProgramIdentifier> invalid = {
        make_identifier(IdentifierType::AMFM_FREQUENCY, 0),
        make_identifier(IdentifierType::RDS_PI, 0x10000),
        make_identifier(IdentifierType::HD_STATION_ID_EXT, 0x100000000),
        make_identifier(IdentifierType::DAB_SID_EXT, 0),
        make_identifier(IdentifierType::DRMO_SERVICE_ID, 0x100000000),
        make_identifier(IdentifierType::SXM_SERVICE_ID, 0x100000000),
    };

    for (auto&& id : invalid) {
        ProgramSelector sel{id, {}};

        auto result = mSession->tune(sel);

        if (utils::isSupported(mProperties, sel)) {
            EXPECT_EQ(Result::INVALID_ARGUMENTS, result);
        } else {
            EXPECT_EQ(Result::NOT_SUPPORTED, result);
        }
    }
}

/**
 * Test tuning with empty program selector.
 *
 * Verifies that:
 *  - tune fails with NOT_SUPPORTED when program selector is not initialized.
 */
TEST_F(BroadcastRadioHalTest, TuneFailsWithEmpty) {
    ASSERT_TRUE(openSession());

    // Program type is 1-based, so 0 will always be invalid.
    ProgramSelector sel = {};
    auto result = mSession->tune(sel);
    ASSERT_EQ(Result::NOT_SUPPORTED, result);
}

/**
 * Test seeking to next/prev station via ITunerSession::scan().
 *
 * Verifies that:
 *  - the method succeeds;
 *  - the program info is changed within timeout::tune;
 *  - works both directions and with or without skipping sub-channel.
 */
TEST_F(BroadcastRadioHalTest, Seek) {
    ASSERT_TRUE(openSession());

    // TODO(b/69958777): see FmTune workaround
    std::this_thread::sleep_for(gTuneWorkaround);

    EXPECT_TIMEOUT_CALL(*mCallback, onCurrentProgramInfoChanged_, _);
    auto result = mSession->scan(true /* up */, true /* skip subchannel */);
    EXPECT_EQ(Result::OK, result);
    EXPECT_TIMEOUT_CALL_WAIT(*mCallback, onCurrentProgramInfoChanged_, timeout::tune);

    EXPECT_TIMEOUT_CALL(*mCallback, onCurrentProgramInfoChanged_, _);
    result = mSession->scan(false /* down */, false /* don't skip subchannel */);
    EXPECT_EQ(Result::OK, result);
    EXPECT_TIMEOUT_CALL_WAIT(*mCallback, onCurrentProgramInfoChanged_, timeout::tune);
}

/**
 * Test step operation.
 *
 * Verifies that:
 *  - the method succeeds or returns NOT_SUPPORTED;
 *  - the program info is changed within timeout::tune if the method succeeded;
 *  - works both directions.
 */
TEST_F(BroadcastRadioHalTest, Step) {
    ASSERT_TRUE(openSession());

    // TODO(b/69958777): see FmTune workaround
    std::this_thread::sleep_for(gTuneWorkaround);

    EXPECT_TIMEOUT_CALL(*mCallback, onCurrentProgramInfoChanged_, _).Times(AnyNumber());
    auto result = mSession->step(true /* up */);
    if (result == Result::NOT_SUPPORTED) {
        printSkipped("step not supported");
        return;
    }
    EXPECT_EQ(Result::OK, result);
    EXPECT_TIMEOUT_CALL_WAIT(*mCallback, onCurrentProgramInfoChanged_, timeout::tune);

    EXPECT_TIMEOUT_CALL(*mCallback, onCurrentProgramInfoChanged_, _);
    result = mSession->step(false /* down */);
    EXPECT_EQ(Result::OK, result);
    EXPECT_TIMEOUT_CALL_WAIT(*mCallback, onCurrentProgramInfoChanged_, timeout::tune);
}

/**
 * Test tune cancellation.
 *
 * Verifies that:
 *  - the method does not crash after being invoked multiple times.
 */
TEST_F(BroadcastRadioHalTest, Cancel) {
    ASSERT_TRUE(openSession());

    for (int i = 0; i < 10; i++) {
        auto result = mSession->scan(true /* up */, true /* skip subchannel */);
        ASSERT_EQ(Result::OK, result);

        auto cancelResult = mSession->cancel();
        ASSERT_TRUE(cancelResult.isOk());
    }
}

/**
 * Test IBroadcastRadio::get|setParameters() methods called with no parameters.
 *
 * Verifies that:
 *  - callback is called for empty parameters set.
 */
TEST_F(BroadcastRadioHalTest, NoParameters) {
    ASSERT_TRUE(openSession());

    hidl_vec<VendorKeyValue> halResults = {};
    bool wasCalled = false;
    auto cb = [&](hidl_vec<VendorKeyValue> results) {
        wasCalled = true;
        halResults = results;
    };

    auto hidlResult = mSession->setParameters({}, cb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_TRUE(wasCalled);
    ASSERT_EQ(0u, halResults.size());

    wasCalled = false;
    hidlResult = mSession->getParameters({}, cb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_TRUE(wasCalled);
    ASSERT_EQ(0u, halResults.size());
}

/**
 * Test IBroadcastRadio::get|setParameters() methods called with unknown parameters.
 *
 * Verifies that:
 *  - unknown parameters are ignored;
 *  - callback is called also for empty results set.
 */
TEST_F(BroadcastRadioHalTest, UnknownParameters) {
    ASSERT_TRUE(openSession());

    hidl_vec<VendorKeyValue> halResults = {};
    bool wasCalled = false;
    auto cb = [&](hidl_vec<VendorKeyValue> results) {
        wasCalled = true;
        halResults = results;
    };

    auto hidlResult = mSession->setParameters({{"com.google.unknown", "dummy"}}, cb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_TRUE(wasCalled);
    ASSERT_EQ(0u, halResults.size());

    wasCalled = false;
    hidlResult = mSession->getParameters({{"com.google.unknown*", "dummy"}}, cb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_TRUE(wasCalled);
    ASSERT_EQ(0u, halResults.size());
}

/**
 * Test session closing.
 *
 * Verifies that:
 *  - the method does not crash after being invoked multiple times.
 */
TEST_F(BroadcastRadioHalTest, Close) {
    ASSERT_TRUE(openSession());

    for (int i = 0; i < 10; i++) {
        auto cancelResult = mSession->close();
        ASSERT_TRUE(cancelResult.isOk());
    }
}

/**
 * Test geting image of invalid ID.
 *
 * Verifies that:
 * - getImage call handles argument 0 gracefully.
 */
TEST_F(BroadcastRadioHalTest, GetNoImage) {
    size_t len = 0;
    auto result = mModule->getImage(0, [&](hidl_vec<uint8_t> rawImage) { len = rawImage.size(); });

    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(0u, len);
}

/**
 * Test getting config flags.
 *
 * Verifies that:
 * - isConfigFlagSet either succeeds or ends with NOT_SUPPORTED or INVALID_STATE;
 * - call success or failure is consistent with setConfigFlag.
 */
TEST_F(BroadcastRadioHalTest, FetchConfigFlags) {
    ASSERT_TRUE(openSession());

    for (auto flag : gConfigFlagValues) {
        auto halResult = Result::UNKNOWN_ERROR;
        auto cb = [&](Result result, bool) { halResult = result; };
        auto hidlResult = mSession->isConfigFlagSet(flag, cb);
        EXPECT_TRUE(hidlResult.isOk());

        if (halResult != Result::NOT_SUPPORTED && halResult != Result::INVALID_STATE) {
            ASSERT_EQ(Result::OK, halResult);
        }

        // set must fail or succeed the same way as get
        auto setResult = mSession->setConfigFlag(flag, false);
        EXPECT_EQ(halResult, setResult);
        setResult = mSession->setConfigFlag(flag, true);
        EXPECT_EQ(halResult, setResult);
    }
}

/**
 * Test setting config flags.
 *
 * Verifies that:
 * - setConfigFlag either succeeds or ends with NOT_SUPPORTED or INVALID_STATE;
 * - isConfigFlagSet reflects the state requested immediately after the set call.
 */
TEST_F(BroadcastRadioHalTest, SetConfigFlags) {
    ASSERT_TRUE(openSession());

    auto get = [&](ConfigFlag flag) {
        auto halResult = Result::UNKNOWN_ERROR;
        bool gotValue = false;
        auto cb = [&](Result result, bool value) {
            halResult = result;
            gotValue = value;
        };
        auto hidlResult = mSession->isConfigFlagSet(flag, cb);
        EXPECT_TRUE(hidlResult.isOk());
        EXPECT_EQ(Result::OK, halResult);
        return gotValue;
    };

    for (auto flag : gConfigFlagValues) {
        auto result = mSession->setConfigFlag(flag, false);
        if (result == Result::NOT_SUPPORTED || result == Result::INVALID_STATE) {
            // setting to true must result in the same error as false
            auto secondResult = mSession->setConfigFlag(flag, true);
            EXPECT_EQ(result, secondResult);
            continue;
        }
        ASSERT_EQ(Result::OK, result);

        // verify false is set
        auto value = get(flag);
        EXPECT_FALSE(value);

        // try setting true this time
        result = mSession->setConfigFlag(flag, true);
        ASSERT_EQ(Result::OK, result);
        value = get(flag);
        EXPECT_TRUE(value);

        // false again
        result = mSession->setConfigFlag(flag, false);
        ASSERT_EQ(Result::OK, result);
        value = get(flag);
        EXPECT_FALSE(value);
    }
}

/**
 * Test getting program list.
 *
 * Verifies that:
 * - startProgramListUpdates either succeeds or returns NOT_SUPPORTED;
 * - the complete list is fetched within timeout::programListScan;
 * - stopProgramListUpdates does not crash.
 */
TEST_F(BroadcastRadioHalTest, GetProgramList) {
    ASSERT_TRUE(openSession());

    getProgramList();
}

/**
 * Test HD_STATION_NAME correctness.
 *
 * Verifies that if a program on the list contains HD_STATION_NAME identifier:
 *  - the program provides station name in its metadata;
 *  - the identifier matches the name;
 *  - there is only one identifier of that type.
 */
TEST_F(BroadcastRadioHalTest, HdRadioStationNameId) {
    ASSERT_TRUE(openSession());

    auto list = getProgramList();
    if (!list) return;

    for (auto&& program : *list) {
        auto nameIds = utils::getAllIds(program.selector, IdentifierType::HD_STATION_NAME);
        EXPECT_LE(nameIds.size(), 1u);
        if (nameIds.size() == 0) continue;

        auto name = utils::getMetadataString(program, MetadataKey::PROGRAM_NAME);
        if (!name) name = utils::getMetadataString(program, MetadataKey::RDS_PS);
        ASSERT_TRUE(name.has_value());

        auto expectedId = utils::make_hdradio_station_name(*name);
        EXPECT_EQ(expectedId.value, nameIds[0]);
    }
}

/**
 * Test announcement listener registration.
 *
 * Verifies that:
 *  - registerAnnouncementListener either succeeds or returns NOT_SUPPORTED;
 *  - if it succeeds, it returns a valid close handle (which is a nullptr otherwise);
 *  - closing handle does not crash.
 */
TEST_F(BroadcastRadioHalTest, AnnouncementListenerRegistration) {
    sp<AnnouncementListenerMock> listener = new AnnouncementListenerMock();

    Result halResult = Result::UNKNOWN_ERROR;
    sp<ICloseHandle> closeHandle = nullptr;
    auto cb = [&](Result result, const sp<ICloseHandle>& closeHandle_) {
        halResult = result;
        closeHandle = closeHandle_;
    };

    auto hidlResult =
        mModule->registerAnnouncementListener({AnnouncementType::EMERGENCY}, listener, cb);
    ASSERT_TRUE(hidlResult.isOk());

    if (halResult == Result::NOT_SUPPORTED) {
        ASSERT_EQ(nullptr, closeHandle.get());
        printSkipped("Announcements not supported");
        return;
    }

    ASSERT_EQ(Result::OK, halResult);
    ASSERT_NE(nullptr, closeHandle.get());

    closeHandle->close();
}

}  // namespace vts
}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using android::hardware::broadcastradio::V2_0::vts::gEnv;
    using android::hardware::broadcastradio::V2_0::IBroadcastRadio;
    using android::hardware::broadcastradio::vts::BroadcastRadioHidlEnvironment;
    gEnv = new BroadcastRadioHidlEnvironment<IBroadcastRadio>;
    ::testing::AddGlobalTestEnvironment(gEnv);
    ::testing::InitGoogleTest(&argc, argv);
    gEnv->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
