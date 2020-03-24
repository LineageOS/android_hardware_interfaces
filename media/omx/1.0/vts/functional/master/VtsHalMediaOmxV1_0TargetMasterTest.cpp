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

#define LOG_TAG "media_omx_hidl_master_test"
#ifdef __LP64__
#define OMX_ANDROID_COMPILE_AS_32BIT_ON_64BIT_PLATFORMS
#endif

#include <android-base/logging.h>

#include <android/hardware/media/omx/1.0/IOmx.h>
#include <android/hardware/media/omx/1.0/IOmxNode.h>
#include <android/hardware/media/omx/1.0/IOmxObserver.h>
#include <android/hardware/media/omx/1.0/IOmxStore.h>
#include <android/hardware/media/omx/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMapper.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::hardware::media::omx::V1_0::IOmx;
using ::android::hardware::media::omx::V1_0::IOmxObserver;
using ::android::hardware::media::omx::V1_0::IOmxNode;
using ::android::hardware::media::omx::V1_0::IOmxStore;
using ::android::hardware::media::omx::V1_0::Message;
using ::android::hardware::media::omx::V1_0::CodecBuffer;
using ::android::hardware::media::omx::V1_0::PortMode;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hidl::memory::V1_0::IMapper;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

#include <getopt.h>
#include <media_hidl_test_common.h>

class MasterHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        omxStore = IOmxStore::getService(GetParam());
        ASSERT_NE(omxStore, nullptr);
        omx = IOmx::getService(GetParam());
        ASSERT_NE(omx, nullptr);
    }

    sp<IOmxStore> omxStore;
    sp<IOmx> omx;

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }
};

void displayComponentInfo(hidl_vec<IOmx::ComponentInfo>& nodeList) {
    for (size_t i = 0; i < nodeList.size(); i++) {
        printf("%s | ", nodeList[i].mName.c_str());
        for (size_t j = 0; j < ((nodeList[i]).mRoles).size(); j++) {
            printf("%s ", nodeList[i].mRoles[j].c_str());
        }
        printf("\n");
    }
}

// Make sure IOmx and IOmxStore have the same set of instances.
TEST(MasterHidlTest, instanceMatchValidation) {
    auto omxInstances = android::hardware::getAllHalInstanceNames(IOmx::descriptor);
    auto omxStoreInstances = android::hardware::getAllHalInstanceNames(IOmxStore::descriptor);
    ASSERT_EQ(omxInstances.size(), omxInstances.size());
    for (const std::string& omxInstance : omxInstances) {
        EXPECT_TRUE(std::find(omxStoreInstances.begin(), omxStoreInstances.end(), omxInstance) !=
                    omxStoreInstances.end());
    }
}

// list service attributes
TEST_P(MasterHidlTest, ListServiceAttr) {
    description("list service attributes");
    android::hardware::media::omx::V1_0::Status status;
    hidl_vec<IOmxStore::Attribute> attributes;
    EXPECT_TRUE(omxStore
                    ->listServiceAttributes([&status, &attributes](
                        android::hardware::media::omx::V1_0::Status _s,
                        hidl_vec<IOmxStore::Attribute> const& _nl) {
                        status = _s;
                        attributes = _nl;
                    })
                    .isOk());
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    if (attributes.size() == 0) ALOGV("Warning, Attribute list empty");
}

// get node prefix
TEST_P(MasterHidlTest, getNodePrefix) {
    description("get node prefix");
    hidl_string prefix;
    omxStore->getNodePrefix(
        [&prefix](hidl_string const& _nl) { prefix = _nl; });
    if (prefix.empty()) ALOGV("Warning, Node Prefix empty");
}

// list roles
TEST_P(MasterHidlTest, ListRoles) {
    description("list roles");
    hidl_vec<IOmxStore::RoleInfo> roleList;
    omxStore->listRoles([&roleList](hidl_vec<IOmxStore::RoleInfo> const& _nl) {
        roleList = _nl;
    });
    if (roleList.size() == 0) ALOGV("Warning, RoleInfo list empty");
}

// list components and roles.
TEST_P(MasterHidlTest, ListNodes) {
    description("enumerate component and roles");
    android::hardware::media::omx::V1_0::Status status;
    hidl_vec<IOmx::ComponentInfo> nodeList;
    bool isPass = true;
    EXPECT_TRUE(
        omx->listNodes([&status, &nodeList](
                           android::hardware::media::omx::V1_0::Status _s,
                           hidl_vec<IOmx::ComponentInfo> const& _nl) {
               status = _s;
               nodeList = _nl;
           })
            .isOk());
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    if (nodeList.size() == 0)
        ALOGV("Warning, ComponentInfo list empty");
    else {
        // displayComponentInfo(nodeList);
        for (size_t i = 0; i < nodeList.size(); i++) {
            sp<CodecObserver> observer = nullptr;
            sp<IOmxNode> omxNode = nullptr;
            observer = new CodecObserver(nullptr);
            ASSERT_NE(observer, nullptr);
            EXPECT_TRUE(
                omx->allocateNode(
                       nodeList[i].mName, observer,
                       [&](android::hardware::media::omx::V1_0::Status _s,
                           sp<IOmxNode> const& _nl) {
                           status = _s;
                           omxNode = _nl;
                       })
                    .isOk());
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
            if (omxNode == nullptr) {
                isPass = false;
                std::cerr << "[    !OK   ] " << nodeList[i].mName.c_str()
                          << "\n";
            } else {
                EXPECT_TRUE((omxNode->freeNode()).isOk());
                omxNode = nullptr;
                // std::cout << "[     OK   ] " << nodeList[i].mName.c_str() <<
                // "\n";
            }
        }
    }
    EXPECT_TRUE(isPass);
}

INSTANTIATE_TEST_CASE_P(
        PerInstance, MasterHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IOmxStore::descriptor)),
        android::hardware::PrintInstanceNameToString);
