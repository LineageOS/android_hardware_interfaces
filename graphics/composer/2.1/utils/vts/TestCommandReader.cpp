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

#include <composer-vts/2.1/TestCommandReader.h>

#include <gtest/gtest.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace vts {

void TestCommandReader::parse() {
    mErrors.clear();
    mCompositionChanges.clear();
    while (!isEmpty()) {
        int32_t command;
        uint16_t length;
        ASSERT_TRUE(beginCommand(&command, &length));

        parseSingleCommand(command, length);

        endCommand();
    }
}

void TestCommandReader::parseSingleCommand(int32_t commandRaw, uint16_t length) {
    IComposerClient::Command command = static_cast<IComposerClient::Command>(commandRaw);

    switch (command) {
        case IComposerClient::Command::SELECT_DISPLAY:
            ASSERT_EQ(2, length);
            read64();  // display
            break;
        case IComposerClient::Command::SET_ERROR: {
            ASSERT_EQ(2, length);
            auto loc = read();
            auto err = readSigned();
            std::pair<uint32_t, uint32_t> error(loc, err);
            mErrors.push_back(error);
        } break;
        case IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES:
            ASSERT_EQ(0, length % 3);
            for (uint16_t count = 0; count < length / 3; ++count) {
                uint64_t layerId = read64();
                uint32_t composition = read();

                std::pair<uint64_t, uint32_t> compositionChange(layerId, composition);
                mCompositionChanges.push_back(compositionChange);
            }
            break;
        case IComposerClient::Command::SET_DISPLAY_REQUESTS:
            ASSERT_EQ(1, length % 3);
            read();  // displayRequests, ignored for now
            for (uint16_t count = 0; count < (length - 1) / 3; ++count) {
                read64();  // layer
                // silently eat requests to clear the client target, since we won't be testing
                // client composition anyway
                ASSERT_EQ(1u, read());
            }
            break;
        case IComposerClient::Command::SET_PRESENT_FENCE:
            ASSERT_EQ(1, length);
            close(readFence());
            break;
        case IComposerClient::Command::SET_RELEASE_FENCES:
            ASSERT_EQ(0, length % 3);
            for (uint16_t count = 0; count < length / 3; ++count) {
                read64();
                close(readFence());
            }
            break;
        default:
            GTEST_FAIL() << "unexpected return command " << std::hex << static_cast<int>(command);
            break;
    }
}

}  // namespace vts
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
