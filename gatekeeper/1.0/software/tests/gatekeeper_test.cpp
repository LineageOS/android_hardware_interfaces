/*
 * Copyright 2015 The Android Open Source Project
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

#include <arpa/inet.h>
#include <iostream>

#include <gtest/gtest.h>
#include <hardware/hw_auth_token.h>

#include "../SoftGateKeeper.h"

using ::gatekeeper::EnrollRequest;
using ::gatekeeper::EnrollResponse;
using ::gatekeeper::secure_id_t;
using ::gatekeeper::SizedBuffer;
using ::gatekeeper::SoftGateKeeper;
using ::gatekeeper::VerifyRequest;
using ::gatekeeper::VerifyResponse;
using ::testing::Test;

static SizedBuffer makePasswordBuffer(int init = 0) {
    constexpr const uint32_t pw_buffer_size = 16;
    auto pw_buffer = new uint8_t[pw_buffer_size];
    memset(pw_buffer, init, pw_buffer_size);

    return {pw_buffer, pw_buffer_size};
}

static SizedBuffer makeAndInitializeSizedBuffer(const uint8_t* data, uint32_t size) {
    auto buffer = new uint8_t[size];
    memcpy(buffer, data, size);
    return {buffer, size};
}

static SizedBuffer copySizedBuffer(const SizedBuffer& rhs) {
    return makeAndInitializeSizedBuffer(rhs.Data<uint8_t>(), rhs.size());
}

static void do_enroll(SoftGateKeeper& gatekeeper, EnrollResponse* response) {
    EnrollRequest request(0, {}, makePasswordBuffer(), {});

    gatekeeper.Enroll(request, response);
}

TEST(GateKeeperTest, EnrollSuccess) {
    SoftGateKeeper gatekeeper;
    EnrollResponse response;
    do_enroll(gatekeeper, &response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, response.error);
}

TEST(GateKeeperTest, EnrollBogusData) {
    SoftGateKeeper gatekeeper;
    EnrollResponse response;

    EnrollRequest request(0, {}, {}, {});

    gatekeeper.Enroll(request, &response);

    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_INVALID, response.error);
}

TEST(GateKeeperTest, VerifySuccess) {
    SoftGateKeeper gatekeeper;
    EnrollResponse enroll_response;

    do_enroll(gatekeeper, &enroll_response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, enroll_response.error);
    VerifyRequest request(0, 1, std::move(enroll_response.enrolled_password_handle),
                          makePasswordBuffer());
    VerifyResponse response;

    gatekeeper.Verify(request, &response);

    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, response.error);

    auto auth_token = response.auth_token.Data<hw_auth_token_t>();

    ASSERT_NE(nullptr, auth_token);
    ASSERT_EQ((uint32_t)HW_AUTH_PASSWORD, ntohl(auth_token->authenticator_type));
    ASSERT_EQ((uint64_t)1, auth_token->challenge);
    ASSERT_NE(~((uint32_t)0), auth_token->timestamp);
    ASSERT_NE((uint64_t)0, auth_token->user_id);
    ASSERT_NE((uint64_t)0, auth_token->authenticator_id);
}

TEST(GateKeeperTest, TrustedReEnroll) {
    SoftGateKeeper gatekeeper;
    EnrollResponse enroll_response;

    // do_enroll enrolls an all 0 password
    do_enroll(gatekeeper, &enroll_response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, enroll_response.error);

    // verify first password
    VerifyRequest request(0, 0, copySizedBuffer(enroll_response.enrolled_password_handle),
                          makePasswordBuffer());
    VerifyResponse response;
    gatekeeper.Verify(request, &response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, response.error);
    auto auth_token = response.auth_token.Data<hw_auth_token_t>();
    ASSERT_NE(nullptr, auth_token);

    secure_id_t secure_id = auth_token->user_id;

    // enroll new password
    EnrollRequest enroll_request(0, std::move(enroll_response.enrolled_password_handle),
                                 makePasswordBuffer(1) /* new password */,
                                 makePasswordBuffer() /* old password */);
    gatekeeper.Enroll(enroll_request, &enroll_response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, enroll_response.error);

    // verify new password
    VerifyRequest new_request(0, 0, std::move(enroll_response.enrolled_password_handle),
                              makePasswordBuffer(1));
    gatekeeper.Verify(new_request, &response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, response.error);
    ASSERT_NE(nullptr, response.auth_token.Data<hw_auth_token_t>());
    ASSERT_EQ(secure_id, response.auth_token.Data<hw_auth_token_t>()->user_id);
}

TEST(GateKeeperTest, UntrustedReEnroll) {
    SoftGateKeeper gatekeeper;
    SizedBuffer provided_password;
    EnrollResponse enroll_response;

    // do_enroll enrolls an all 0 password
    provided_password = makePasswordBuffer();
    do_enroll(gatekeeper, &enroll_response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, enroll_response.error);

    // verify first password
    VerifyRequest request(0, 0, std::move(enroll_response.enrolled_password_handle),
                          std::move(provided_password));
    VerifyResponse response;
    gatekeeper.Verify(request, &response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, response.error);
    auto auth_token = response.auth_token.Data<hw_auth_token_t>();
    ASSERT_NE(nullptr, auth_token);

    secure_id_t secure_id = auth_token->user_id;

    EnrollRequest enroll_request(0, {}, makePasswordBuffer(1), {});
    gatekeeper.Enroll(enroll_request, &enroll_response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, enroll_response.error);

    // verify new password
    VerifyRequest new_request(0, 0, std::move(enroll_response.enrolled_password_handle),
                              makePasswordBuffer(1));
    gatekeeper.Verify(new_request, &response);
    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_NONE, response.error);
    ASSERT_NE(nullptr, response.auth_token.Data<hw_auth_token_t>());
    ASSERT_NE(secure_id, response.auth_token.Data<hw_auth_token_t>()->user_id);
}

TEST(GateKeeperTest, VerifyBogusData) {
    SoftGateKeeper gatekeeper;
    VerifyResponse response;

    VerifyRequest request(0, 0, {}, {});

    gatekeeper.Verify(request, &response);

    ASSERT_EQ(::gatekeeper::gatekeeper_error_t::ERROR_INVALID, response.error);
}
