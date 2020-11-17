/*
 * Copyright (C) 2020 The Android Open Source Project
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

#pragma once

#include <linux/netlink.h>
#include <sys/socket.h>

// We use #defines here so as to get local lamba captures and error message line numbers
#define ASSERT_TRUE_CALLBACK                                    \
    [&](bool success, std::string errMsg) {                     \
        ASSERT_TRUE(success) << "unexpected error: " << errMsg; \
    }

#define ASSERT_FALSE_CALLBACK \
    [&](bool success, std::string errMsg) { ASSERT_FALSE(success) << "expected error: " << errMsg; }

#define ASSERT_ZERO_BYTES_CALLBACK            \
    [&](uint64_t rxBytes, uint64_t txBytes) { \
        EXPECT_EQ(0ULL, rxBytes);             \
        EXPECT_EQ(0ULL, txBytes);             \
    }

inline const sockaddr* asSockaddr(const sockaddr_nl* nladdr);

int conntrackSocket(unsigned groups);