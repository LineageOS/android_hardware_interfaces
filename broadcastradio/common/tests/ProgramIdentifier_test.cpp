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

#include <broadcastradio-utils-2x/Utils.h>
#include <gtest/gtest.h>

#include <optional>

namespace {

namespace utils = android::hardware::broadcastradio::utils;

TEST(ProgramIdentifierTest, hdRadioStationName) {
    auto verify = [](std::string name, uint64_t nameId) {
        auto id = utils::make_hdradio_station_name(name);
        EXPECT_EQ(nameId, id.value) << "Failed to convert '" << name << "'";
    };

    verify("", 0);
    verify("Abc", 0x434241);
    verify("Some Station 1", 0x54415453454d4f53);
    verify("Station1", 0x314e4f4954415453);
    verify("!@#$%^&*()_+", 0);
    verify("-=[]{};':\"0", 0x30);
}

}  // anonymous namespace
