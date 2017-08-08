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
#define LOG_TAG "VtsOffloadControlV1_0TargetTest"

#include <android-base/unique_fd.h>
#include <android/hardware/tetheroffload/config/1.0/IOffloadConfig.h>
#include <android/hardware/tetheroffload/control/1.0/IOffloadControl.h>
#include <android/hardware/tetheroffload/control/1.0/types.h>
#include <linux/netfilter/nfnetlink.h>
#include <linux/netlink.h>
#include <log/log.h>
#include <set>
#include <sys/socket.h>
#include <unistd.h>
#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>

using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using android::hardware::tetheroffload::control::V1_0::IOffloadControl;
using android::hardware::tetheroffload::control::V1_0::IPv4AddrPortPair;
using android::hardware::tetheroffload::control::V1_0::ITetheringOffloadCallback;
using android::hardware::tetheroffload::control::V1_0::OffloadCallbackEvent;
using android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate;
using android::hardware::tetheroffload::control::V1_0::NetworkProtocol;
using android::hardware::Void;
using android::sp;

inline const sockaddr * asSockaddr(const sockaddr_nl *nladdr) {
    return reinterpret_cast<const sockaddr *>(nladdr);
}

int conntrackSocket(unsigned groups) {
    android::base::unique_fd s(socket(AF_NETLINK, SOCK_DGRAM, NETLINK_NETFILTER));
    if (s.get() < 0) {
        return -errno;
    }

    const struct sockaddr_nl bind_addr = {
        .nl_family = AF_NETLINK,
        .nl_pad = 0,
        .nl_pid = 0,
        .nl_groups = groups,
    };
    if (::bind(s.get(), asSockaddr(&bind_addr), sizeof(bind_addr)) < 0) {
        return -errno;
    }

    const struct sockaddr_nl kernel_addr = {
        .nl_family = AF_NETLINK,
        .nl_pad = 0,
        .nl_pid = 0,
        .nl_groups = groups,
    };
    if (connect(s.get(), asSockaddr(&kernel_addr), sizeof(kernel_addr)) != 0) {
        return -errno;
    }

    return s.release();
}

constexpr char kCallbackOnEvent[] = "onEvent";
constexpr char kCallbackUpdateTimeout[] = "updateTimeout";

class TetheringOffloadCallbackArgs {
   public:
    OffloadCallbackEvent last_event;
    NatTimeoutUpdate last_params;
};

class OffloadControlHidlTest : public testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {
        control = testing::VtsHalHidlTargetTestBase::getService<IOffloadControl>();
        ASSERT_NE(nullptr, control.get()) << "Could not get HIDL instance";

        control_cb = new TetheringOffloadCallback();
        ASSERT_NE(nullptr, control.get()) << "Could not get get offload callback";

        /*
         * Config must be set with correct socket options in order for
         * any control options to be set.
         */
        config = testing::VtsHalHidlTargetTestBase::getService<IOffloadConfig>();
        ASSERT_NE(nullptr, control.get()) << "Could not get HIDL instance";

        android::base::unique_fd
            fd1(conntrackSocket(NFNLGRP_CONNTRACK_NEW | NFNLGRP_CONNTRACK_DESTROY)),
            fd2(conntrackSocket(NFNLGRP_CONNTRACK_UPDATE | NFNLGRP_CONNTRACK_DESTROY));

        native_handle_t* nativeHandle1 = native_handle_create(1, 0);
        nativeHandle1->data[0] = fd1;
        hidl_handle h1 = hidl_handle(nativeHandle1);

        native_handle_t* nativeHandle2 = native_handle_create(1, 0);
        nativeHandle2->data[0] = fd2;
        hidl_handle h2 = hidl_handle(nativeHandle2);

        auto config_cb = [&](bool config_success, std::string errMsg) {
            if(!config_success) {
                ALOGI("Config CB Error message: %s", errMsg.c_str());
            }
            ASSERT_TRUE(config_success);
        };

        Return<void> ret = config->setHandles(h1, h2, config_cb);
        ASSERT_TRUE(ret.isOk());

        auto init_cb = [&](bool success, std::string errMsg) {
                  if(!success) {
                      ALOGI("Error message: %s", errMsg.c_str());
                  }
                  ASSERT_TRUE(success);
              };
        ret = control->initOffload(control_cb, init_cb);
        ASSERT_TRUE(ret.isOk());
    }

    virtual void TearDown() override {
        auto cb = [&](bool success, const hidl_string& errMsg) {
                  if(!success) {
                      ALOGI("Error message: %s", errMsg.c_str());
                  }
                  ASSERT_TRUE(success);
              };
        Return<void> ret = control->stopOffload(cb);
        ASSERT_TRUE(ret.isOk());

        control_cb.clear();
    }

    /* Callback class for data & Event. */
    class TetheringOffloadCallback
        : public testing::VtsHalHidlTargetCallbackBase<TetheringOffloadCallbackArgs>,
          public ITetheringOffloadCallback {
       public:
        TetheringOffloadCallback(){};

        virtual ~TetheringOffloadCallback() = default;

        /* onEvent callback function - Called when an asynchronous
         * event is generated by the hardware management process.
         **/
        Return<void> onEvent(OffloadCallbackEvent event) override {
            TetheringOffloadCallbackArgs args;
            args.last_event = event;
            NotifyFromCallback(kCallbackOnEvent, args);
            return Void();
        };

        Return<void> updateTimeout(const NatTimeoutUpdate &params) override {
            TetheringOffloadCallbackArgs args;
            args.last_params = params;
            NotifyFromCallback(kCallbackUpdateTimeout, args);
            return Void();
        };
    };

    sp<IOffloadConfig> config;
    sp<IOffloadControl> control;
    sp<TetheringOffloadCallback> control_cb;
};

/**
 * InitOffloadNotOk:
 * Calls initOffload() again. Check that initOffload returns
 * false, since it was already called from SetUp().
 */
TEST_F(OffloadControlHidlTest, InitOffloadNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success){
            ALOGI("Error message: %s", errMsg.c_str());
        }
        ASSERT_FALSE(success);
    };
    Return<void> ret = control->initOffload(control_cb, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * StopOffload:
 * Calls StopOffload()
 * Check that stopOffload after init is successful.
 */
TEST_F(OffloadControlHidlTest, StopOffload) {
    /* Empty function tested as part of tearDown */
}

/**
 * SetLocalPrefixesIPv4:
 * Calls setLocalPrefixes(). Test setting one ipv4 prefix
 * returns true.
 */
TEST_F(OffloadControlHidlTest, SetLocalPrefixesIPv4) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        ASSERT_TRUE(success);
    };
    vector<hidl_string> prefixes;
    prefixes.push_back(hidl_string("192.0.2.1"));
    Return<void> ret = control->setLocalPrefixes(prefixes, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetLocalPrefixesIPv6:
 * Calls setLocalPrefixes(). Test setting one ipv6 prefix
 * returns true.
 */
TEST_F(OffloadControlHidlTest, SetLocalPrefixesIPv6) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        ASSERT_TRUE(success);
    };
    vector<hidl_string> prefixes;
    prefixes.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setLocalPrefixes(prefixes, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetLocalPrefixesIPv4v6:
 * Calls setLocalPrefixes(). Test setting one ipv4 and one ipv6
 * prefix returns true.
 */
TEST_F(OffloadControlHidlTest, SetLocalPrefixesIPv4v6) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        ASSERT_TRUE(success);
    };
    vector<hidl_string> prefixes;
    prefixes.push_back(hidl_string("192.0.2.1"));
    prefixes.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setLocalPrefixes(prefixes, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetLocalPrefixesEmptyNotOk:
 * Calls setLocalPrefixes(). Test that setting no prefixes
 * returns false.
 */
TEST_F(OffloadControlHidlTest, SetLocalPrefixesEmptyNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        ASSERT_FALSE(success);
    };
    vector<hidl_string> prefixes;
    Return<void> ret = control->setLocalPrefixes(prefixes, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetLocalPrefixesInvalidNotOk:
 * Calls setLocalPrefixes(). Set the 2nd prefix of 2 to
 * "invalid", this is expected to return false.
 */
TEST_F(OffloadControlHidlTest, SetLocalPrefixesInvalidNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        ASSERT_FALSE(success);
    };
    vector<hidl_string> prefixes;
    prefixes.push_back(hidl_string("192.0.2.1"));
    prefixes.push_back(hidl_string("invalid"));
    Return<void> ret = control->setLocalPrefixes(prefixes, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * getForwardedStats:
 * Calls getForwardedStats(). Stats should always be 0
 * since there is no data traffic.
 */
TEST_F(OffloadControlHidlTest, GetForwardedStats) {
    auto cb = [&](uint64_t rxBytes, uint64_t txBytes) {
        EXPECT_EQ((uint64_t) 0, rxBytes);
        EXPECT_EQ((uint64_t) 0, txBytes);
    };

    hidl_string upstream("invalid");
    Return<void> ret = control->getForwardedStats(upstream, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * GetForwardedStatsDummyIface:
 * Calls getForwardedStats(). Stats should always be 0
 * since there is no data traffic.
 */
TEST_F(OffloadControlHidlTest, GetForwardedStatsDummyIface) {
    auto cb = [&](uint64_t rxBytes, uint64_t txBytes) {
        EXPECT_EQ((uint64_t) 0, rxBytes);
        EXPECT_EQ((uint64_t) 0, txBytes);
    };

    hidl_string upstream("dummy0");
    Return<void> ret = control->getForwardedStats(upstream, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetDataLimitEmptyNotOk:
 * Calls setDataLimit(). Set data limit with an empty upstream
 * parameter. Expectation is this returns false.
 */
TEST_F(OffloadControlHidlTest, SetDataLimitEmptyNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string upstream("");
    uint64_t limit = 5000;
    Return<void> ret = control->setDataLimit(upstream, limit, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParameters:
 * Calls setUpstreamParameters(). Valid parameters should return
 * true.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParameters) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_TRUE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersEmptyNotOk:
 * Calls setUpstreamParameters(). Empty parameters should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersEmptyNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("");
    hidl_string v4Addr("");
    hidl_string v4Gw("");
    vector<hidl_string> v6Gws;
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersIfaceInvalidNotOk:
 * Calls setUpstreamParameters(). Invalid iface should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersIfaceInvalidNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("invalid");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersIfaceEmptyNotOk:
 * Calls setUpstreamParameters(). Empty iface should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersIfaceEmptyNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4AddrEmpty: Calls
 * setUpstreamParameters(). Empty v4 address should return true,
 * v6 address will be added.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4AddrEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_TRUE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersV4AddrInvalidNotOk:
 * Calls setUpstreamParameters(). Invalid v4 address should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4AddrInvalidNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("invalid");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersV4AddrInvalid2NotOk:
 * Calls setUpstreamParameters(). Invalid v4 address (missing 1 octet)
 * should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4AddrInvalid2NotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4GwEmpty: Calls
 * setUpstreamParameters(). Empty ipv4 gateway should return
 * true, v6 address will be added.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4GwEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_TRUE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersV4GwInvalidNotOk:
 * Calls setUpstreamParameters(). Invalid ipv4 gateway should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4GwInvalidNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("invalid");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersV4GwInvalid2NotOk:
 * Calls setUpstreamParameters(). Invalid v4 gateway (missing 1 octet)
 * should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4GwInvalid2NotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("192.0.2");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersV6GwEmptyNotOk:
 * Calls setUpstreamParameters(). Invalid IPv6 gateway should
 * return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV6GwEmptyNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string(""));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersV6GwInvalidNotOk:
 * Calls setUpstreamParameters(). Invalid IPv6 gateway should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV6GwInvalidNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("invalid"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * SetUpstreamParametersV6GwInvalid2NotOk:
 * Calls setUpstreamParameters(). Invalid IPv6 gateway (too
 * short)should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV6GwInvalid2NotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4OnlyValid:
 * Calls setUpstreamParameters(). Invalid v6 gateway, should
 * still pass since a valid v4 gateway is passed in.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4Only) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_TRUE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr("192.0.2.0/24");
    hidl_string v4Gw("192.0.2.1");
    vector<hidl_string> v6Gws;
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV6Only:
 * Calls setUpstreamParameters(). Invalid v4 parameters, should
 * still pass since a valid v6 gateway is passed in.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV6Only) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_TRUE(success);
    };

    hidl_string iface("dummy0");
    hidl_string v4Addr;
    hidl_string v4Gw;
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("fe80:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * AddDownstream:
 * Calls addDownstream(). Valid parameters should return true.
 */
TEST_F(OffloadControlHidlTest, AddDownstream) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        ASSERT_TRUE(success);
    };

    hidl_string iface("dummy0");
    hidl_string prefix("192.0.2.0/24");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * AddDownstreamEmptyNotOk:
 * Calls addDownstream(). Empty parameters should return false.
 */
TEST_F(OffloadControlHidlTest, AddDownstreamEmptyNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("");
    hidl_string prefix("");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * AddDownstreamPrefixEmptyNotOk:
 * Calls addDownstream(). Empty prefix should return false.
 */
TEST_F(OffloadControlHidlTest, AddDownstreamPrefixEmptyNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string prefix("");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * AddDownstreamInvalidNotOk:
 * Calls addDownstream(). Invalid parameters should return false.
 */
TEST_F(OffloadControlHidlTest, AddDownstreamInvalidNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("invalid");
    hidl_string prefix("192.0.2");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * AddDownstreamInvalid2NotOk:
 * Calls addDownstream(). Invalid parameters should return false. Prefix
 * is too short, missing 1 octet.
 */
TEST_F(OffloadControlHidlTest, AddDownstreamInvalid2NotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string prefix("192.0.2");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * RemoveDownstream:
 * Calls removeDownstream(). Valid parameters should return true.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstream) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        ASSERT_TRUE(success);
    };

    hidl_string iface("dummy0");
    hidl_string prefix("192.0.2.0/24");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * RemoveDownstreamEmptyNotOk:
 * Calls removeDownstream(). Empty parameters should return false.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstreamEmptyNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("");
    hidl_string prefix("");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * RemoveDownstreamEmpty2NotOk:
 * Calls removeDownstream(). Empty prefix should return
 * false.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstreamEmpty2NotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string prefix("");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * RemoveDownstreamInvalidNotOk:
 * Calls removeDownstream(). Invalid parameters should return false.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstreamInvalidNotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("invalid");
    hidl_string prefix("192.0.2.1");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * Negative testcase
 * RemoveDownstreamInvalid2NotOk:
 * Calls removeDownstream(). Invalid parameters should return false. Prefix
 * is too short, missing 1 octet.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstreamInvalid2NotOk) {
    auto cb = [&](bool success, std::string errMsg) {
        if(!success) {
            ALOGI("Error message: %s", errMsg.c_str());
        }
        EXPECT_FALSE(success);
    };

    hidl_string iface("dummy0");
    hidl_string prefix("192.0.2");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGE("Test result with status=%d", status);
    return status;
}
