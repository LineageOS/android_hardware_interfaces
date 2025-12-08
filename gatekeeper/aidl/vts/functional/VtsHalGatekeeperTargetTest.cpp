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

#define LOG_TAG "gatekeeper_aidl_hal_test"

#include <inttypes.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/gatekeeper/GatekeeperEnrollResponse.h>
#include <aidl/android/hardware/gatekeeper/GatekeeperVerifyResponse.h>
#include <aidl/android/hardware/gatekeeper/IGatekeeper.h>
#include <aidl/android/hardware/security/keymint/HardwareAuthToken.h>
#include <android-base/endian.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <hardware/hw_auth_token.h>

#include <log/log.h>

using aidl::android::hardware::gatekeeper::GatekeeperEnrollResponse;
using aidl::android::hardware::gatekeeper::GatekeeperVerifyResponse;
using aidl::android::hardware::gatekeeper::IGatekeeper;
using aidl::android::hardware::security::keymint::HardwareAuthToken;
using Status = ::ndk::ScopedAStatus;

struct GatekeeperRequest {
    uint32_t uid;
    uint64_t challenge;
    std::vector<uint8_t> curPwdHandle;
    std::vector<uint8_t> curPwd;
    std::vector<uint8_t> newPwd;
    GatekeeperRequest() : uid(0), challenge(0) {}
};

// ASSERT_* macros generate return "void" internally
// we have to use EXPECT_* if we return anything but "void"
static void verifyAuthToken(GatekeeperVerifyResponse& rsp) {
    uint32_t auth_type = static_cast<uint32_t>(rsp.hardwareAuthToken.authenticatorType);
    uint64_t auth_tstamp = static_cast<uint64_t>(rsp.hardwareAuthToken.timestamp.milliSeconds);

    EXPECT_EQ(HW_AUTH_PASSWORD, auth_type);
    EXPECT_NE(UINT64_C(~0), auth_tstamp);
    ALOGI("Secure user ID:   %016" PRIX64, rsp.hardwareAuthToken.userId);
    EXPECT_NE(UINT32_C(0), rsp.hardwareAuthToken.userId);
}

// The main test class for Gatekeeper AIDL HAL.
class GatekeeperAidlTest : public ::testing::TestWithParam<std::string> {
  protected:
    void setUid(uint32_t uid) { uid_ = uid; }

    Status doEnroll(const GatekeeperRequest& req, GatekeeperEnrollResponse& rsp) {
        Status ret;
        while (true) {
            ret = gatekeeper_->enroll(uid_, req.curPwdHandle, req.curPwd, req.newPwd, &rsp);
            if (ret.isOk()) break;
            if (getReturnStatusCode(ret) != IGatekeeper::ERROR_RETRY_TIMEOUT) break;
            ALOGI("%s: got retry code; retrying in 1 sec", __func__);
            sleep(1);
        }
        return ret;
    }

    Status doVerify(const GatekeeperRequest& req, GatekeeperVerifyResponse& rsp) {
        Status ret;
        while (true) {
            ret = gatekeeper_->verify(uid_, req.challenge, req.curPwdHandle, req.newPwd, &rsp);
            if (ret.isOk()) break;
            if (getReturnStatusCode(ret) != IGatekeeper::ERROR_RETRY_TIMEOUT) break;
            ALOGI("%s: got retry code; retrying in 1 sec", __func__);
            sleep(1);
        }
        return ret;
    }

    Status doDeleteUser() { return gatekeeper_->deleteUser(uid_); }

    Status doDeleteAllUsers() { return gatekeeper_->deleteAllUsers(); }

    std::vector<uint8_t> generatePassword(uint8_t seed) {
        std::vector<uint8_t> password;
        password.resize(16);
        memset(password.data(), seed, password.size());
        return password;
    }

    void checkEnroll(const GatekeeperEnrollResponse& rsp, const Status& ret, bool expectSuccess) {
        if (expectSuccess) {
            EXPECT_TRUE(ret.isOk());
            EXPECT_EQ(IGatekeeper::STATUS_OK, rsp.statusCode);
            EXPECT_NE(nullptr, rsp.data.data());
            EXPECT_GT(rsp.data.size(), UINT32_C(0));
            EXPECT_NE(UINT32_C(0), rsp.secureUserId);
        } else {
            EXPECT_EQ(IGatekeeper::ERROR_GENERAL_FAILURE, getReturnStatusCode(ret));
            EXPECT_EQ(UINT32_C(0), rsp.data.size());
        }
    }

    void enrollNewPasswordFails(const std::vector<uint8_t>& password) {
        enrollNewPassword(password, /* expectSuccess= */ false);
    }

    std::pair<std::vector<uint8_t>, int64_t> enrollNewPassword(const std::vector<uint8_t>& password,
                                                               bool expectSuccess = true) {
        GatekeeperRequest req;
        req.newPwd = password;
        GatekeeperEnrollResponse rsp;
        Status ret = doEnroll(req, rsp);
        checkEnroll(rsp, ret, expectSuccess);
        return std::make_pair(rsp.data, rsp.secureUserId);
    }

    void verifyPasswordSucceeds(const std::vector<uint8_t>& password,
                                const std::vector<uint8_t>& passwordHandle, uint64_t challenge,
                                int64_t expectedSid, GatekeeperVerifyResponse& verifyRsp) {
        verifyPassword(password, passwordHandle, challenge, verifyRsp, expectedSid,
                       /* expectSuccess= */ true);
    }

    void verifyPasswordFails(const std::vector<uint8_t>& password,
                             const std::vector<uint8_t>& passwordHandle, uint64_t challenge) {
        GatekeeperVerifyResponse verifyRsp;
        verifyPassword(password, passwordHandle, challenge, verifyRsp,
                       /* expectedSid= */ 0,
                       /* expectSuccess= */ false);
    }

    void verifyPassword(const std::vector<uint8_t>& password,
                        const std::vector<uint8_t>& passwordHandle, uint64_t challenge,
                        GatekeeperVerifyResponse& verifyRsp, int64_t expectedSid,
                        bool expectSuccess) {
        // Assemble the arguments into the verify request.
        GatekeeperRequest verifyReq;
        verifyReq.newPwd = password;
        verifyReq.curPwdHandle = passwordHandle;
        verifyReq.challenge = challenge;

        Status ret = doVerify(verifyReq, verifyRsp);
        if (expectSuccess) {
            EXPECT_TRUE(ret.isOk());
            EXPECT_GE(verifyRsp.statusCode, IGatekeeper::STATUS_OK);
            EXPECT_LE(verifyRsp.statusCode, IGatekeeper::STATUS_REENROLL);

            verifyAuthToken(verifyRsp);
            EXPECT_EQ(challenge, verifyRsp.hardwareAuthToken.challenge);
            EXPECT_EQ(expectedSid, verifyRsp.hardwareAuthToken.userId);
        } else {
            EXPECT_EQ(IGatekeeper::ERROR_GENERAL_FAILURE, getReturnStatusCode(ret));
        }
    }

    int32_t getReturnStatusCode(const Status& result) {
        if (!result.isOk()) {
            if (result.getExceptionCode() == EX_SERVICE_SPECIFIC) {
                return result.getServiceSpecificError();
            }
            return IGatekeeper::ERROR_GENERAL_FAILURE;
        }
        return IGatekeeper::STATUS_OK;
    }

    // Check that a verification attempt fails, and return any retry interval in milliseconds.
    int32_t verifyPasswordFailDelay(const std::vector<uint8_t>& password,
                                    const std::vector<uint8_t>& passwordHandle,
                                    uint64_t challenge) {
        GatekeeperRequest verifyReq;
        verifyReq.newPwd = password;
        verifyReq.curPwdHandle = passwordHandle;
        verifyReq.challenge = challenge;

        GatekeeperVerifyResponse verifyRsp;
        Status ret = doVerify(verifyReq, verifyRsp);
        if (ret.isOk()) {
            // An OK Binder response (when verification failure is expected)
            // should be an indication that the verification wasn't attempted
            // because a retry interval is pending.
            EXPECT_EQ(verifyRsp.statusCode, IGatekeeper::ERROR_RETRY_TIMEOUT);
            return verifyRsp.timeoutMs;
        } else {
            // Failed attempt to verify.
            return 0;
        }
    }

  protected:
    std::shared_ptr<IGatekeeper> gatekeeper_;
    uint32_t uid_;

  public:
    GatekeeperAidlTest() : uid_(0) {}
    virtual void SetUp() override {
        gatekeeper_ = IGatekeeper::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(nullptr, gatekeeper_.get());
        doDeleteAllUsers();
    }

    virtual void TearDown() override { doDeleteAllUsers(); }
};

/**
 * Ensure we can enroll new password
 */
TEST_P(GatekeeperAidlTest, EnrollSuccess) {
    ALOGI("Testing Enroll (expected success)");
    std::vector<uint8_t> password = generatePassword(0);
    enrollNewPassword(password);
    ALOGI("Testing Enroll done");
}

/**
 * Ensure we can not enroll empty password
 */
TEST_P(GatekeeperAidlTest, EnrollNoPassword) {
    ALOGI("Testing Enroll(empty) (expected failure)");
    std::vector<uint8_t> password;
    enrollNewPasswordFails(password);
    ALOGI("Testing Enroll done");
}

/**
 * Ensure we can successfully verify previously enrolled password
 */
TEST_P(GatekeeperAidlTest, VerifySuccess) {
    ALOGI("Testing Enroll+Verify (expected success)");
    std::vector<uint8_t> password = generatePassword(0);

    const auto [passwordHandle, sid] = enrollNewPassword(password);
    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 1, sid, verifyRsp);

    ALOGI("Testing unenrolled password doesn't verify");
    std::vector<uint8_t> wrongPassword = generatePassword(1);
    verifyPasswordFails(wrongPassword, passwordHandle, 1);
    ALOGI("Testing Enroll+Verify done");
}

/**
 * Ensure that passwords containing a NUL byte aren't truncated
 */
TEST_P(GatekeeperAidlTest, PasswordIsBinaryData) {
    ALOGI("Testing Enroll+Verify of password with embedded NUL (expected success)");
    std::vector<uint8_t> rightPassword = {'A', 'B', 'C', '\0', 'D', 'E', 'F'};
    const auto [passwordHandle, sid] = enrollNewPassword(rightPassword);
    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(rightPassword, passwordHandle, 1, sid, verifyRsp);

    ALOGI("Testing Verify of wrong password (expected failure)");
    std::vector<uint8_t> wrongPassword = {'A', 'B', 'C', '\0', '\0', '\0', '\0'};
    verifyPasswordFails(wrongPassword, passwordHandle, 1);

    ALOGI("PasswordIsBinaryData test done");
}

/**
 * Ensure that long passwords aren't truncated
 */
TEST_P(GatekeeperAidlTest, LongPassword) {
    ALOGI("Testing Enroll+Verify of long password (expected success)");
    std::vector<uint8_t> password;
    password.resize(64);  // maximum length used by Android
    memset(password.data(), 'A', password.size());

    const auto [passwordHandle, sid] = enrollNewPassword(password);
    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 1, sid, verifyRsp);

    ALOGI("Testing Verify of wrong password (expected failure)");
    password[password.size() - 1] ^= 1;
    verifyPasswordFails(password, passwordHandle, 1);

    ALOGI("LongPassword test done");
}

/**
 * Ensure we can securely update password (keep the same
 * secure user_id) if we prove we know old password
 */
TEST_P(GatekeeperAidlTest, TrustedReenroll) {
    ALOGI("Testing Trusted Reenroll (expected success)");

    std::vector<uint8_t> password = generatePassword(0);

    const auto [passwordHandle, sid] = enrollNewPassword(password);

    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 0, sid, verifyRsp);
    ALOGI("Primary Enroll+Verify done");
    verifyAuthToken(verifyRsp);

    std::vector<uint8_t> newPassword = generatePassword(1);
    GatekeeperRequest reenrollReq;
    reenrollReq.newPwd = newPassword;
    reenrollReq.curPwd = password;
    reenrollReq.curPwdHandle = passwordHandle;

    GatekeeperEnrollResponse reenrollRsp;
    Status ret = doEnroll(reenrollReq, reenrollRsp);
    checkEnroll(reenrollRsp, ret, true);
    std::vector<uint8_t> newPasswordHandle = reenrollRsp.data;

    GatekeeperVerifyResponse reenrollVerifyRsp;
    verifyPasswordSucceeds(newPassword, newPasswordHandle, 0, sid, reenrollVerifyRsp);
    ALOGI("Trusted ReEnroll+Verify done");
    verifyAuthToken(reenrollVerifyRsp);
    EXPECT_EQ(verifyRsp.hardwareAuthToken.userId, reenrollVerifyRsp.hardwareAuthToken.userId);
    ALOGI("Testing Trusted Reenroll done");
}

/**
 * Ensure we can update password (and get new secure user_id) if we don't know old password
 */
TEST_P(GatekeeperAidlTest, UntrustedReenroll) {
    ALOGI("Testing Untrusted Reenroll (expected success)");
    std::vector<uint8_t> password = generatePassword(0);
    const auto [passwordHandle, sid] = enrollNewPassword(password);
    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 0, sid, verifyRsp);
    verifyAuthToken(verifyRsp);
    ALOGI("Primary Enroll+Verify done");

    std::vector<uint8_t> newPassword = generatePassword(1);
    const auto [newPasswordHandle, newSid] = enrollNewPassword(newPassword);
    EXPECT_NE(newSid, sid);

    GatekeeperVerifyResponse reenrollVerifyRsp;
    verifyPasswordSucceeds(newPassword, newPasswordHandle, 0, newSid, reenrollVerifyRsp);
    ALOGI("Untrusted ReEnroll+Verify done");

    verifyAuthToken(reenrollVerifyRsp);
    EXPECT_NE(verifyRsp.hardwareAuthToken.userId, reenrollVerifyRsp.hardwareAuthToken.userId);
    ALOGI("Testing Untrusted Reenroll done");
}

/**
 * Ensure we don't get successful verify with invalid data
 */
TEST_P(GatekeeperAidlTest, VerifyNoData) {
    ALOGI("Testing Verify (expected failure)");
    std::vector<uint8_t> password;
    std::vector<uint8_t> passwordHandle;
    verifyPasswordFails(password, passwordHandle, 0);
    ALOGI("Testing Verify done");
}

/**
 * Ensure we can not verify password after we enrolled it and then deleted user
 */
TEST_P(GatekeeperAidlTest, DeleteUserTest) {
    ALOGI("Testing deleteUser (expected success)");
    setUid(10001);
    std::vector<uint8_t> password = generatePassword(0);
    const auto [passwordHandle, sid] = enrollNewPassword(password);

    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 0, sid, verifyRsp);
    ALOGI("Enroll+Verify done");

    auto result = doDeleteUser();
    EXPECT_TRUE(result.isOk() ||
                (getReturnStatusCode(result) == IGatekeeper::ERROR_NOT_IMPLEMENTED));
    ALOGI("DeleteUser done");
    if (result.isOk()) {
        verifyPasswordFails(password, passwordHandle, 0);
        ALOGI("Verify after Delete done (must fail)");
    }
    ALOGI("Testing deleteUser done: rsp=%" PRIi32, getReturnStatusCode(result));
}

/**
 * Ensure we can not delete a user that does not exist
 */
TEST_P(GatekeeperAidlTest, DeleteInvalidUserTest) {
    ALOGI("Testing deleteUser (expected failure)");
    setUid(10002);
    std::vector<uint8_t> password = generatePassword(0);
    const auto [passwordHandle, sid] = enrollNewPassword(password);
    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 0, sid, verifyRsp);
    ALOGI("Enroll+Verify done");

    // Delete the user
    Status result1 = doDeleteUser();
    EXPECT_TRUE(result1.isOk() ||
                (getReturnStatusCode(result1) == IGatekeeper::ERROR_NOT_IMPLEMENTED));

    // Delete the user again
    Status result2 = doDeleteUser();
    int32_t retCode2 = getReturnStatusCode(result2);
    EXPECT_TRUE((retCode2 == IGatekeeper::ERROR_NOT_IMPLEMENTED) ||
                (retCode2 == IGatekeeper::ERROR_GENERAL_FAILURE));
    ALOGI("DeleteUser done");
    ALOGI("Testing deleteUser done: rsp=%" PRIi32, retCode2);
}

/**
 * Ensure we can not verify passwords after we enrolled them and then deleted
 * all users
 */
TEST_P(GatekeeperAidlTest, DeleteAllUsersTest) {
    struct UserData {
        uint32_t userId;
        std::vector<uint8_t> password;
        std::vector<uint8_t> passwordHandle;
        int64_t sid;
        GatekeeperVerifyResponse verifyRsp;
        UserData(int id) { userId = id; }
    } users[3]{10001, 10002, 10003};
    ALOGI("Testing deleteAllUsers (expected success)");

    // enroll multiple users
    for (size_t i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
        setUid(users[i].userId);
        users[i].password = generatePassword((i % 255) + 1);
        auto [passwordHandle, sid] = enrollNewPassword(users[i].password);
        users[i].passwordHandle = passwordHandle;
        users[i].sid = sid;
    }
    ALOGI("Multiple users enrolled");

    // verify multiple users
    for (size_t i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
        setUid(users[i].userId);
        verifyPasswordSucceeds(users[i].password, users[i].passwordHandle, 0, users[i].sid,
                               users[i].verifyRsp);
    }
    ALOGI("Multiple users verified");

    Status result = doDeleteAllUsers();
    EXPECT_TRUE(result.isOk() ||
                (getReturnStatusCode(result) == IGatekeeper::ERROR_NOT_IMPLEMENTED));
    ALOGI("All users deleted");

    if (result.isOk()) {
        // verify multiple users after they are deleted; all must fail
        for (size_t i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
            setUid(users[i].userId);
            verifyPasswordFails(users[i].password, users[i].passwordHandle, 0);
        }
        ALOGI("Multiple users verified after delete (all must fail)");
    }

    ALOGI("Testing deleteAllUsers done: rsp=%" PRIi32, getReturnStatusCode(result));
}

/**
 * Ensure that multiple failed verify attempts induce a delay.
 *
 * Android CDD section 9.11:
 *
 * [C-0-2] The lock screen authentication MUST implement a time interval between failed
 * attempts. With n as the failed attempt count, the time interval MUST be at least 30 seconds for 9
 * < n < 30. For n > 29, the time interval value MUST be at least 30*2^floor((n-30)/10)) seconds or
 * at least 24 hours, whichever is smaller.
 */
TEST_P(GatekeeperAidlTest, FailedAttemptDelay) {
    // Limit test execution to a couple of minutes.
    const int32_t kMaxTestTimeMillis = 120000;

    ALOGI("Testing multiple failed verify");
    std::vector<uint8_t> password = generatePassword(0);

    const auto [passwordHandle, sid] = enrollNewPassword(password);
    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 1, sid, verifyRsp);

    std::vector<uint8_t> wrongPassword = generatePassword(1);

    auto testStart = std::chrono::steady_clock::now();
    int failureCount = 0;
    while (true) {
        int32_t waitMillis = verifyPasswordFailDelay(wrongPassword, passwordHandle, 0);
        ALOGI("Attempt to verify wrong password attempt %d requires %ldms delay", failureCount,
              (long)waitMillis);

        if (failureCount > 9) {
            // Allow a little leeway for rounding
            ASSERT_GT(waitMillis, 29000) << "failed verify attempt " << failureCount << " requires "
                                         << waitMillis << "ms retry interval but should be >30s";
        }
        failureCount++;

        if (waitMillis != 0) {
            // Round up slightly to be sure the retry interval has expired before next retry.
            waitMillis += 500;

            // Abandon the test if the next wait would make overall test execution too long.
            auto now = std::chrono::steady_clock::now();
            auto testMillis =
                    std::chrono::duration_cast<std::chrono::milliseconds>(now - testStart).count();
            if (testMillis + waitMillis > kMaxTestTimeMillis) {
                ALOGI("Abandoning test as total time taken is now %ldms", (long)testMillis);
                break;
            }

            ALOGI("Waiting %ld millis before retrying", (long)waitMillis);
            std::this_thread::sleep_for(std::chrono::milliseconds(waitMillis));
        }
    }
    ALOGI("Testing multiple failed verify done");
}

/**
 * Test that delays are enforced.
 **/
TEST_P(GatekeeperAidlTest, DelayEnforced) {
    ALOGI("Testing delay enforcement");
    std::vector<uint8_t> password = generatePassword(0);

    const auto [passwordHandle, sid] = enrollNewPassword(password);
    GatekeeperVerifyResponse verifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 0, sid, verifyRsp);

    std::vector<uint8_t> wrongPassword = generatePassword(1);

    // Repeatedly fail verification until we get a long retry delay.
    int32_t waitMillis = 0;
    int failureCount = 0;
    while (waitMillis < 20000) {
        waitMillis = verifyPasswordFailDelay(wrongPassword, passwordHandle, 0);
        ALOGI("Attempt to verify wrong password attempt %d requires %ldms delay", failureCount,
              (long)waitMillis);
        failureCount++;
    }

    // Wait for less than the required time.
    int32_t shortWaitMillis = waitMillis / 2;
    ALOGI("Waiting %ld millis (too soon) before retrying", (long)shortWaitMillis);
    std::this_thread::sleep_for(std::chrono::milliseconds(shortWaitMillis));

    // Presenting the correct password fails because a retry interval is still in force.
    int32_t remainingMillis = verifyPasswordFailDelay(password, passwordHandle, 0);
    EXPECT_GT(remainingMillis, 0);

    // Wait for rest of the required time (with some leeway).
    shortWaitMillis += 1000;
    ALOGI("Waiting %ld millis before retrying", (long)shortWaitMillis);
    std::this_thread::sleep_for(std::chrono::milliseconds(shortWaitMillis));

    GatekeeperVerifyResponse laterVerifyRsp;
    verifyPasswordSucceeds(password, passwordHandle, 0, sid, laterVerifyRsp);

    ALOGI("Testing delay enforcement done");
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GatekeeperAidlTest);
INSTANTIATE_TEST_SUITE_P(
    PerInstance, GatekeeperAidlTest,
    testing::ValuesIn(android::getAidlHalInstanceNames(IGatekeeper::descriptor)),
    android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
