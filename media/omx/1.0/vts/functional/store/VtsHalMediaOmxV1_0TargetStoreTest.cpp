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

#define LOG_TAG "media_omx_hidl_store_test"
#ifdef __LP64__
#define OMX_ANDROID_COMPILE_AS_32BIT_ON_64BIT_PLATFORMS
#endif

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <android/api-level.h>

#include <VtsCoreUtil.h>
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
#include <media/stagefright/omx/OMXUtils.h>

using ::android::sp;
using ::android::base::Join;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::media::omx::V1_0::CodecBuffer;
using ::android::hardware::media::omx::V1_0::IOmx;
using ::android::hardware::media::omx::V1_0::IOmxNode;
using ::android::hardware::media::omx::V1_0::IOmxObserver;
using ::android::hardware::media::omx::V1_0::IOmxStore;
using ::android::hardware::media::omx::V1_0::Message;
using ::android::hardware::media::omx::V1_0::PortMode;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMapper;
using ::android::hidl::memory::V1_0::IMemory;

#include <getopt.h>
#include <media_hidl_test_common.h>

class StoreHidlTest : public ::testing::TestWithParam<std::string> {
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

struct AttributePattern {
    const testing::internal::RE key;
    const testing::internal::RE value;
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

void validateAttributes(
        const std::map<const std::string, const testing::internal::RE>& knownPatterns,
        const std::vector<const struct AttributePattern>& unknownPatterns,
        hidl_vec<IOmxStore::Attribute> attributes) {
    std::set<const std::string> attributeKeys;
    for (const auto& attr : attributes) {
        // Make sure there are no duplicates
        const auto [nodeIter, inserted] = attributeKeys.insert(attr.key);
        EXPECT_EQ(inserted, true) << "Attribute \"" << attr.key << "\" has duplicates.";

        // Check the value against the corresponding regular
        // expression.
        const auto knownPattern = knownPatterns.find(attr.key);
        if (knownPattern != knownPatterns.end()) {
            EXPECT_EQ(testing::internal::RE::FullMatch(attr.value, knownPattern->second), true)
                    << "Attribute \"" << attr.key << "\" has invalid value \"" << attr.value << ".";
            ;
        } else {
            // Failed to find exact attribute, check against
            // possible patterns.
            bool keyFound = false;
            for (const auto& unknownPattern : unknownPatterns) {
                if (testing::internal::RE::PartialMatch(attr.key, unknownPattern.key)) {
                    keyFound = true;
                    EXPECT_EQ(testing::internal::RE::FullMatch(attr.value, unknownPattern.value),
                              true)
                            << "Attribute \"" << attr.key << "\" has invalid value \"" << attr.value
                            << ".";
                }
            }
            if (!keyFound) {
                std::cout << "Warning, Unrecognized attribute \"" << attr.key << "\" with value \""
                          << attr.value << "\"." << std::endl;
            }
        }
    }
}

// Make sure IOmx and IOmxStore have the same set of instances.
TEST(StoreHidlTest, instanceMatchValidation) {
    auto omxInstances = android::hardware::getAllHalInstanceNames(IOmx::descriptor);
    auto omxStoreInstances = android::hardware::getAllHalInstanceNames(IOmxStore::descriptor);
    ASSERT_EQ(omxInstances.size(), omxInstances.size());
    for (const std::string& omxInstance : omxInstances) {
        EXPECT_TRUE(std::find(omxStoreInstances.begin(), omxStoreInstances.end(), omxInstance) !=
                    omxStoreInstances.end());
    }
}

// list service attributes and verify expected formats
TEST_P(StoreHidlTest, ListServiceAttr) {
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
    if (attributes.size() == 0) {
        std::cout << "Warning, Attribute list empty" << std::endl;
    } else {
        /*
         * knownPatterns is a map whose keys are the known "key" for a service
         * attribute pair (see IOmxStore::Attribute), and whose values are the
         * corresponding regular expressions that will have to match with the
         * "value" of the attribute pair. If listServiceAttributes() returns an
         * attribute that has a matching key but an unmatched value, the test
         * will fail.
         */
        const std::map<const std::string, const testing::internal::RE> knownPatterns = {
                {"max-video-encoder-input-buffers", "0|[1-9][0-9]*"},
                {"supports-multiple-secure-codecs", "0|1"},
                {"supports-secure-with-non-secure-codec", "0|1"},
        };
        /*
         * unknownPatterns is a vector of pairs of regular expressions.
         * For each attribute whose key is not known (i.e., does not match any
         * of the keys in the "knownPatterns" variable defined above), that key will be
         * tried for a match with the first element of each pair of the variable
         * "unknownPatterns". If a match occurs, the value of that same attribute will be
         * tried for a match with the second element of the pair. If this second
         * match fails, the test will fail.
         */
        const std::vector<const struct AttributePattern> unknownPatterns = {
                {"supports-[a-z0-9-]*", "0|1"}};

        validateAttributes(knownPatterns, unknownPatterns, attributes);
    }
}

// get node prefix
TEST_P(StoreHidlTest, getNodePrefix) {
    description("get node prefix");
    hidl_string prefix;
    omxStore->getNodePrefix(
        [&prefix](hidl_string const& _nl) { prefix = _nl; });
    if (prefix.empty()) std::cout << "Warning, Node Prefix empty" << std::endl;
}

// list roles and validate all RoleInfo objects
TEST_P(StoreHidlTest, ListRoles) {
    description("list roles");
    hidl_vec<IOmxStore::RoleInfo> roleList;
    omxStore->listRoles([&roleList](hidl_vec<IOmxStore::RoleInfo> const& _nl) {
        roleList = _nl;
    });
    if (roleList.size() == 0) {
        GTEST_SKIP() << "Warning, RoleInfo list empty";
        return;
    }

    // Basic patterns for matching
    const std::string toggle = "(0|1)";
    const std::string string = "(.*)";
    const std::string num = "(0|([1-9][0-9]*))";
    const std::string size = "(" + num + "x" + num + ")";
    const std::string ratio = "(" + num + ":" + num + ")";
    const std::string range_num = "((" + num + "-" + num + ")|" + num + ")";
    const std::string range_size = "((" + size + "-" + size + ")|" + size + ")";
    const std::string range_ratio = "((" + ratio + "-" + ratio + ")|" + ratio + ")";
    const std::string list_range_num = "(" + range_num + "(," + range_num + ")*)";

    // Matching rules for node attributes with fixed keys
    const std::map<const std::string, const testing::internal::RE> knownPatterns = {
            {"alignment", size},
            {"bitrate-range", range_num},
            {"block-aspect-ratio-range", range_ratio},
            {"block-count-range", range_num},
            {"block-size", size},
            {"blocks-per-second-range", range_num},
            {"complexity-default", num},
            {"complexity-range", range_num},
            {"feature-adaptive-playback", toggle},
            {"feature-bitrate-control", "(VBR|CBR|CQ)[,(VBR|CBR|CQ)]*"},
            {"feature-can-swap-width-height", toggle},
            {"feature-intra-refresh", toggle},
            {"feature-partial-frame", toggle},
            {"feature-secure-playback", toggle},
            {"feature-tunneled-playback", toggle},
            {"frame-rate-range", range_num},
            {"max-channel-count", num},
            {"max-concurrent-instances", num},
            {"max-supported-instances", num},
            {"pixel-aspect-ratio-range", range_ratio},
            {"quality-default", num},
            {"quality-range", range_num},
            {"quality-scale", string},
            {"sample-rate-ranges", list_range_num},
            {"size-range", range_size},
    };

    // Strings for matching rules for node attributes with key patterns
    const std::vector<const struct AttributePattern> unknownPatterns = {
            {"measured-frame-rate-" + size + "-range", range_num},
            {"feature-[a-zA-Z0-9_-]+", string},
    };

    // Matching rules for node names and owners
    const testing::internal::RE nodeNamePattern = "[a-zA-Z0-9._-]+";
    const testing::internal::RE nodeOwnerPattern = "[a-zA-Z0-9._-]+";

    std::set<const std::string> roleKeys;
    std::map<const std::string, std::set<const std::string>> nodeToRoles;
    std::map<const std::string, std::set<const std::string>> ownerToNodes;
    for (const IOmxStore::RoleInfo& role : roleList) {
        // Make sure there are no duplicates
        const auto [roleIter, inserted] = roleKeys.insert(role.role);
        EXPECT_EQ(inserted, true) << "Role \"" << role.role << "\" has duplicates.";

        // Make sure role name follows expected format based on type and
        // isEncoder
        const char* role_name = ::android::GetComponentRole(role.isEncoder, role.type.c_str());
        if (role_name != nullptr) {
            EXPECT_EQ(std::string(role_name), role.role)
                    << "Role \"" << role.role << "\" does not match "
                    << (role.isEncoder ? "an encoder " : "a decoder ") << "for media type \""
                    << role.type << ".";
        }

        // Check the nodes for this role
        std::set<const std::string> nodeKeys;
        for (const IOmxStore::NodeInfo& node : role.nodes) {
            // Make sure there are no duplicates
            const auto [nodeIter, inserted] = nodeKeys.insert(node.name);
            EXPECT_EQ(inserted, true) << "Node \"" << node.name << "\" has duplicates.";

            // Check the format of node name
            EXPECT_EQ(testing::internal::RE::FullMatch(node.name, nodeNamePattern), true)
                    << "Node name \"" << node.name << " is invalid.";
            // Check the format of node owner
            EXPECT_EQ(testing::internal::RE::FullMatch(node.owner, nodeOwnerPattern), true)
                    << "Node owner \"" << node.owner << " is invalid.";

            validateAttributes(knownPatterns, unknownPatterns, node.attributes);

            ownerToNodes[node.owner].insert(node.name);
            nodeToRoles[node.name].insert(role.role);
        }
    }

    // Verify the information with IOmx::listNodes().
    // IOmxStore::listRoles() and IOmx::listNodes() should give consistent
    // information about nodes and roles.
    for (const auto& [owner, nodes] : ownerToNodes) {
        // Obtain the IOmx instance for each "owner"
        const sp<IOmx> omx = omxStore->getOmx(owner);
        EXPECT_NE(nullptr, omx);

        // Invoke IOmx::listNodes()
        android::hardware::media::omx::V1_0::Status status;
        hidl_vec<IOmx::ComponentInfo> nodeList;
        EXPECT_TRUE(
                omx->listNodes([&status, &nodeList](android::hardware::media::omx::V1_0::Status _s,
                                                    hidl_vec<IOmx::ComponentInfo> const& _nl) {
                       status = _s;
                       nodeList = _nl;
                   }).isOk());
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

        // Verify that roles for each node match with the information from
        // IOmxStore::listRoles().
        std::set<const std::string> nodeKeys;
        for (IOmx::ComponentInfo node : nodeList) {
            // Make sure there are no duplicates
            const auto [nodeIter, inserted] = nodeKeys.insert(node.mName);
            EXPECT_EQ(inserted, true)
                    << "IOmx::listNodes() lists duplicate nodes \"" << node.mName << "\".";

            // Skip "hidden" nodes, i.e. those that are not advertised by
            // IOmxStore::listRoles().
            if (nodes.find(node.mName) == nodes.end()) {
                std::cout << "Warning, IOmx::listNodes() lists unknown node \"" << node.mName
                          << "\" for IOmx instance \"" << owner << "\"." << std::endl;
                continue;
            }

            // All the roles advertised by IOmxStore::listRoles() for this
            // node must be included in roleKeys.
            std::set<const std::string> difference;
            std::set_difference(nodeToRoles[node.mName].begin(), nodeToRoles[node.mName].end(),
                                roleKeys.begin(), roleKeys.end(),
                                std::inserter(difference, difference.begin()));
            EXPECT_EQ(difference.empty(), true) << "IOmx::listNodes() for IOmx "
                                                   "instance \""
                                                << owner
                                                << "\" does not report some "
                                                   "expected nodes: "
                                                << android::base::Join(difference, ", ") << ".";
        }
        // Check that all nodes obtained from IOmxStore::listRoles() are
        // supported by the their corresponding IOmx instances.
        std::set<const std::string> difference;
        std::set_difference(nodes.begin(), nodes.end(), nodeKeys.begin(), nodeKeys.end(),
                            std::inserter(difference, difference.begin()));
        EXPECT_EQ(difference.empty(), true) << "IOmx::listNodes() for IOmx "
                                               "instance \""
                                            << owner
                                            << "\" does not report some "
                                               "expected nodes: "
                                            << android::base::Join(difference, ", ") << ".";
    }

    if (!nodeToRoles.empty()) {
        // Check that the prefix is a sensible string.
        hidl_string prefix;
        omxStore->getNodePrefix([&prefix](hidl_string const& _nl) { prefix = _nl; });
        EXPECT_EQ(testing::internal::RE::PartialMatch(prefix, nodeNamePattern), true)
                << "\"" << prefix << "\" is not a valid prefix for node names.";

        // Check that all node names have the said prefix.
        for (const auto& node : nodeToRoles) {
            EXPECT_NE(node.first.rfind(prefix, 0), std::string::npos)
                    << "Node \"" << node.first << "\" does not start with prefix \"" << prefix
                    << "\".";
        }
    }
}

static bool isTV() {
    return testing::deviceSupportsFeature("android.software.leanback");
}

// list components and roles.
TEST_P(StoreHidlTest, OmxCodecAllowedTest) {
    static int sBoardFirstApiLevel = android::base::GetIntProperty("ro.board.first_api_level", 0);
    if (sBoardFirstApiLevel == 0) {
        GTEST_SKIP() << "board first API level not detected";
    }
    hidl_vec<IOmx::ComponentInfo> componentInfos = getComponentInfoList(omx);
    for (IOmx::ComponentInfo info : componentInfos) {
        for (std::string role : info.mRoles) {
            if (role.find("video_decoder") != std::string::npos ||
                role.find("video_encoder") != std::string::npos) {
                // Codec2 is not mandatory on Android TV devices that launched with Android S
                if (isTV()) {
                    ASSERT_LT(sBoardFirstApiLevel, __ANDROID_API_T__)
                            << " Component: " << info.mName.c_str() << " Role: " << role.c_str()
                            << " not allowed for devices launching with Android T and above";
                } else {
                    std::string codecName = info.mName;
                    bool isAndroidCodec = (codecName.rfind("OMX.google", 0) != std::string::npos);
                    if (isAndroidCodec && (sBoardFirstApiLevel <= __ANDROID_API_S__)) {
                        // refer b/230582620
                        // S AOSP build did not remove the OMX.google video codecs
                        // so it is infeasible to require no OMX.google.* video codecs
                        // on S launching devices
                    } else {
                        ASSERT_LT(sBoardFirstApiLevel, __ANDROID_API_S__)
                                << " Component: " << info.mName.c_str() << " Role: " << role.c_str()
                                << " not allowed for devices launching with Android S and above";
                    }
                }
            }
            if (role.find("audio_decoder") != std::string::npos ||
                role.find("audio_encoder") != std::string::npos) {
                ASSERT_LT(sBoardFirstApiLevel, __ANDROID_API_T__)
                        << " Component: " << info.mName.c_str() << " Role: " << role.c_str()
                        << " not allowed for devices launching with Android T and above";
            }
        }
    }
}

// list components and roles.
TEST_P(StoreHidlTest, ListNodes) {
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
        std::cout << "Warning, ComponentInfo list empty" << std::endl;
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

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(StoreHidlTest);
INSTANTIATE_TEST_CASE_P(
        PerInstance, StoreHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IOmxStore::descriptor)),
        android::hardware::PrintInstanceNameToString);
