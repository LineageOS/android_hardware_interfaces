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

#include <composer-vts/2.4/TestCommandReader.h>

#include <gtest/gtest.h>

namespace android::hardware::graphics::composer::V2_4::vts {

void TestCommandReader::parseSingleCommand(int32_t commandRaw, uint16_t length) {
    IComposerClient::Command command = static_cast<IComposerClient::Command>(commandRaw);

    switch (command) {
        case IComposerClient::Command::SET_CLIENT_TARGET_PROPERTY:
            ASSERT_EQ(2, length);
            read();
            close(readFence());
            break;
        default:
            return android::hardware::graphics::composer::V2_1::vts::TestCommandReader::
                    parseSingleCommand(commandRaw, length);
    }
}

}  // namespace android::hardware::graphics::composer::V2_4::vts
