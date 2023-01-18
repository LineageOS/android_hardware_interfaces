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

#include <numeric>
#include <string>

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <netdb.h>

#include "Offload.h"

namespace aidl::android::hardware::tetheroffload::impl::example {

using ::android::base::Join;

ndk::ScopedAStatus Offload::addDownstream(const std::string& in_iface,
                                          const std::string& in_prefix) {
    LOG(VERBOSE) << __func__ << " Interface: " << in_iface << ", Prefix: " << in_prefix;
    if (!isInitialized()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                EX_ILLEGAL_STATE, "Tetheroffload HAL not initialized");
    }
    if (!isValidInterface(in_iface)) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid interface name");
    }
    if (!isValidIpPrefix(in_prefix)) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid IP prefix");
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::getForwardedStats(const std::string& in_upstream,
                                              ForwardedStats* _aidl_return) {
    LOG(VERBOSE) << __func__ << " Upstream: " << in_upstream;
    ForwardedStats stats;
    stats.rxBytes = 0;
    stats.txBytes = 0;
    *_aidl_return = std::move(stats);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::initOffload(const ndk::ScopedFileDescriptor& in_fd1,
                                        const ndk::ScopedFileDescriptor& in_fd2,
                                        const std::shared_ptr<ITetheringOffloadCallback>& in_cb) {
    LOG(VERBOSE) << __func__ << " FileDescriptor1: " << std::to_string(in_fd1.get())
                 << ", FileDescriptor2: " << std::to_string(in_fd2.get())
                 << ", ITetheringOffloadCallback: " << in_cb;
    if (isInitialized()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                EX_ILLEGAL_STATE, "Tetheroffload HAL already initialized");
    }
    int fd1 = in_fd1.get();
    int fd2 = in_fd2.get();
    if (fd1 < 0 || fd2 < 0) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid file descriptors");
    }
    mFd1 = ndk::ScopedFileDescriptor(dup(fd1));
    mFd2 = ndk::ScopedFileDescriptor(dup(fd2));
    mInitialized = true;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::removeDownstream(const std::string& in_iface,
                                             const std::string& in_prefix) {
    LOG(VERBOSE) << __func__ << " Interface: " << in_iface << ", Prefix: " << in_prefix;
    if (!isInitialized()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                EX_ILLEGAL_STATE, "Tetheroffload HAL not initialized");
    }
    if (!isValidIpPrefix(in_prefix)) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid IP prefix");
    }
    if (!isValidInterface(in_iface)) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid interface name");
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::setDataWarningAndLimit(const std::string& in_upstream,
                                                   int64_t in_warningBytes, int64_t in_limitBytes) {
    LOG(VERBOSE) << __func__ << " Upstream: " << in_upstream
                 << ", WarningBytes: " << in_warningBytes << ", LimitBytes: " << in_limitBytes;
    if (!isInitialized()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                EX_ILLEGAL_STATE, "Tetheroffload HAL not initialized");
    }
    if (!isValidInterface(in_upstream)) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid interface name");
    }
    if (in_warningBytes < 0 || in_limitBytes < 0) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Threshold must be non-negative");
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::setLocalPrefixes(const std::vector<std::string>& in_prefixes) {
    LOG(VERBOSE) << __func__ << " Prefixes: " << Join(in_prefixes, ',');
    if (!isInitialized()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                EX_ILLEGAL_STATE, "Tetheroffload HAL not initialized");
    }
    if (in_prefixes.empty()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "No IP prefix");
    }
    for (std::string prefix : in_prefixes) {
        if (!isValidIpPrefix(prefix)) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Invalid IP prefix");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::setUpstreamParameters(const std::string& in_iface,
                                                  const std::string& in_v4Addr,
                                                  const std::string& in_v4Gw,
                                                  const std::vector<std::string>& in_v6Gws) {
    LOG(VERBOSE) << __func__ << " Interface: " << in_iface << ", IPv4Address: " << in_v4Addr
                 << ", IPv4Gateway: " << in_v4Gw << ", IPv6Gateways: " << Join(in_v6Gws, ',');
    if (!isInitialized()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                EX_ILLEGAL_STATE, "Tetheroffload HAL not initialized");
    }
    if (!isValidInterface(in_iface)) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid interface name");
    }
    if (in_v4Addr.empty() && in_v4Gw.empty() && in_v6Gws.empty()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "No upstream IP address");
    }
    if (!in_v4Addr.empty() && !in_v4Gw.empty()) {
        if (!isValidIpv4Address(in_v4Addr) || !isValidIpv4Address(in_v4Gw)) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Invalid IP address");
        }
    }
    for (std::string ip : in_v6Gws) {
        if (!isValidIpv6Address(ip)) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Invalid IP address");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::stopOffload() {
    LOG(VERBOSE) << __func__;
    if (!isInitialized()) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                EX_ILLEGAL_STATE, "Tetheroffload HAL not initialized");
    }
    mInitialized = false;
    return ndk::ScopedAStatus::ok();
};

bool Offload::isInitialized() {
    return (mInitialized == true);
}

bool Offload::isValidInterface(const std::string& iface) {
    return !iface.empty() && iface != "invalid";
}

bool Offload::isValidIpv4Address(const std::string& repr) {
    return validateIpAddressOrPrefix(repr, AF_INET, false);
}

bool Offload::isValidIpv4Prefix(const std::string& repr) {
    return validateIpAddressOrPrefix(repr, AF_INET, true);
}

bool Offload::isValidIpv6Address(const std::string& repr) {
    return validateIpAddressOrPrefix(repr, AF_INET6, false);
}

bool Offload::isValidIpv6Prefix(const std::string& repr) {
    return validateIpAddressOrPrefix(repr, AF_INET6, true);
}

bool Offload::isValidIpAddress(const std::string& repr) {
    return isValidIpv4Address(repr) || isValidIpv6Address(repr);
}

bool Offload::isValidIpPrefix(const std::string& repr) {
    return isValidIpv4Prefix(repr) || isValidIpv6Prefix(repr);
}

// Refer to libnetdutils's IPAddress and IPPrefix classes.
// Can't use them directly because libnetdutils is not "vendor_available".
bool Offload::validateIpAddressOrPrefix(const std::string& repr, const int expectedFamily,
                                        const bool isPrefix) {
    const addrinfo hints = {
            .ai_flags = AI_NUMERICHOST | AI_NUMERICSERV,
    };
    addrinfo* res;
    size_t index = repr.find('/');
    if (isPrefix && index == std::string::npos) return false;

    // Parse the IP address.
    const std::string ipAddress = isPrefix ? repr.substr(0, index) : repr;
    const int ret = getaddrinfo(ipAddress.c_str(), nullptr, &hints, &res);
    if (ret != 0) return false;

    // Check the address family.
    int family = res[0].ai_family;
    freeaddrinfo(res);
    if (family != expectedFamily) return false;
    if (!isPrefix) return true;

    // Parse the prefix length.
    const char* prefixString = repr.c_str() + index + 1;
    if (!isdigit(*prefixString)) return false;
    char* endptr;
    unsigned long prefixlen = strtoul(prefixString, &endptr, 10);
    if (*endptr != '\0') return false;

    uint8_t maxlen = (family == AF_INET) ? 32 : 128;
    if (prefixlen > maxlen) return false;

    return true;
}

}  // namespace aidl::android::hardware::tetheroffload::impl::example
