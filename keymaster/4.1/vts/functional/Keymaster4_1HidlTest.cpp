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

#include "Keymaster4_1HidlTest.h"

namespace android::hardware::keymaster::V4_1::test {

using std::string;

void Keymaster4_1HidlTest::SetUp() {
    keymaster41_ = IKeymasterDevice::getService(GetParam());
    InitializeKeymaster(keymaster41_);
}

auto Keymaster4_1HidlTest::ProcessMessage(const HidlBuf& key_blob, KeyPurpose operation,
                                          const string& message, const AuthorizationSet& in_params)
        -> std::tuple<ErrorCode, string, AuthorizationSet /* out_params */> {
    AuthorizationSet begin_out_params;
    V4_0::ErrorCode result = Begin(operation, key_blob, in_params, &begin_out_params, &op_handle_);
    AuthorizationSet out_params(std::move(begin_out_params));
    if (result != V4_0::ErrorCode::OK) {
        return {convert(result), {}, out_params};
    }

    string output;
    size_t consumed = 0;
    AuthorizationSet update_params;
    AuthorizationSet update_out_params;
    result = Update(op_handle_, update_params, message, &update_out_params, &output, &consumed);
    out_params.push_back(update_out_params);
    if (result != V4_0::ErrorCode::OK) {
        return {convert(result), output, out_params};
    }

    string unused;
    AuthorizationSet finish_params;
    AuthorizationSet finish_out_params;
    result = Finish(op_handle_, finish_params, message.substr(consumed), unused, &finish_out_params,
                    &output);
    op_handle_ = V4_0::test::kOpHandleSentinel;
    out_params.push_back(finish_out_params);

    return {convert(result), output, out_params};
}

}  // namespace android::hardware::keymaster::V4_1::test
