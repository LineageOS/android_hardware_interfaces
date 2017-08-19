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

#define LOG_TAG "media_omx_hidl_component_test"
#include <android-base/logging.h>

#include <android/hardware/media/omx/1.0/IOmx.h>
#include <android/hardware/media/omx/1.0/IOmxNode.h>
#include <android/hardware/media/omx/1.0/IOmxObserver.h>
#include <android/hardware/media/omx/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMapper.h>
#include <android/hidl/memory/1.0/IMemory.h>

using ::android::hardware::media::omx::V1_0::IOmx;
using ::android::hardware::media::omx::V1_0::IOmxObserver;
using ::android::hardware::media::omx::V1_0::IOmxNode;
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

#include <VtsHalHidlTargetTestBase.h>
#include <getopt.h>
#include <media_hidl_test_common.h>

// A class for test environment setup
class ComponentTestEnvironment : public ::testing::Environment {
   public:
    virtual void SetUp() {}
    virtual void TearDown() {}

    ComponentTestEnvironment() : instance("default") {}

    void setInstance(const char* _instance) { instance = _instance; }

    void setComponent(const char* _component) { component = _component; }

    void setRole(const char* _role) { role = _role; }

    const hidl_string getInstance() const { return instance; }

    const hidl_string getComponent() const { return component; }

    const hidl_string getRole() const { return role; }

    int initFromOptions(int argc, char** argv) {
        static struct option options[] = {
            {"instance", required_argument, 0, 'I'},
            {"component", required_argument, 0, 'C'},
            {"role", required_argument, 0, 'R'},
            {0, 0, 0, 0}};

        while (true) {
            int index = 0;
            int c = getopt_long(argc, argv, "I:C:R:", options, &index);
            if (c == -1) {
                break;
            }

            switch (c) {
                case 'I':
                    setInstance(optarg);
                    break;
                case 'C':
                    setComponent(optarg);
                    break;
                case 'R':
                    setRole(optarg);
                    break;
                case '?':
                    break;
            }
        }

        if (optind < argc) {
            fprintf(stderr,
                    "unrecognized option: %s\n\n"
                    "usage: %s <gtest options> <test options>\n\n"
                    "test options are:\n\n"
                    "-I, --instance: HAL instance to test\n"
                    "-C, --component: OMX component to test\n"
                    "-R, --Role: OMX component Role\n",
                    argv[optind ?: 1], argv[0]);
            return 2;
        }
        return 0;
    }

   private:
    hidl_string instance;
    hidl_string component;
    hidl_string role;
};

static ComponentTestEnvironment* gEnv = nullptr;

// generic component test fixture class
class ComponentHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        disableTest = false;
        android::hardware::media::omx::V1_0::Status status;
        omx = ::testing::VtsHalHidlTargetTestBase::getService<IOmx>(
            gEnv->getInstance());
        ASSERT_NE(omx, nullptr);
        observer = new CodecObserver(nullptr);
        ASSERT_NE(observer, nullptr);
        if (strncmp(gEnv->getComponent().c_str(), "OMX.", 4) != 0)
            disableTest = true;
        EXPECT_TRUE(omx->allocateNode(
                           gEnv->getComponent(), observer,
                           [&](android::hardware::media::omx::V1_0::Status _s,
                               sp<IOmxNode> const& _nl) {
                               status = _s;
                               this->omxNode = _nl;
                           })
                        .isOk());
        ASSERT_NE(omxNode, nullptr);
        ASSERT_NE(gEnv->getRole().empty(), true) << "Invalid Component Role";
        struct StringToClass {
            const char* Class;
            standardCompClass CompClass;
        };
        const StringToClass kStringToClass[] = {
            {"audio_decoder", audio_decoder},
            {"audio_encoder", audio_encoder},
            {"video_decoder", video_decoder},
            {"video_encoder", video_encoder},
        };
        const size_t kNumStringToClass =
            sizeof(kStringToClass) / sizeof(kStringToClass[0]);
        const char* pch;
        char substring[OMX_MAX_STRINGNAME_SIZE];
        strcpy(substring, gEnv->getRole().c_str());
        pch = strchr(substring, '.');
        ASSERT_NE(pch, nullptr) << "Invalid Component Role";
        substring[pch - substring] = '\0';
        compClass = unknown_class;
        for (size_t i = 0; i < kNumStringToClass; ++i) {
            if (!strcasecmp(substring, kStringToClass[i].Class)) {
                compClass = kStringToClass[i].CompClass;
                break;
            }
        }
        if (compClass == unknown_class) disableTest = true;
        isSecure = false;
        size_t suffixLen = strlen(".secure");
        if (strlen(gEnv->getComponent().c_str()) >= suffixLen) {
            isSecure =
                !strcmp(gEnv->getComponent().c_str() +
                            strlen(gEnv->getComponent().c_str()) - suffixLen,
                        ".secure");
        }
        if (disableTest) std::cerr << "[          ] Warning !  Test Disabled\n";
    }

    virtual void TearDown() override {
        if (omxNode != nullptr) {
            EXPECT_TRUE((omxNode->freeNode()).isOk());
            omxNode = nullptr;
        }
    }

    enum standardCompClass {
        audio_decoder,
        audio_encoder,
        video_decoder,
        video_encoder,
        unknown_class,
    };

    sp<IOmx> omx;
    sp<CodecObserver> observer;
    sp<IOmxNode> omxNode;
    standardCompClass compClass;
    bool isSecure;
    bool disableTest;

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }
};

// Random Index used for monkey testing while get/set parameters
#define RANDOM_INDEX 1729

void initPortMode(PortMode* pm, bool isSecure,
                  ComponentHidlTest::standardCompClass compClass) {
    pm[0] = PortMode::PRESET_BYTE_BUFFER;
    pm[1] = PortMode::PRESET_BYTE_BUFFER;
    if (isSecure) {
        switch (compClass) {
            case ComponentHidlTest::video_decoder:
                pm[0] = PortMode::PRESET_SECURE_BUFFER;
                break;
            case ComponentHidlTest::video_encoder:
                pm[1] = PortMode::PRESET_SECURE_BUFFER;
                break;
            default:
                break;
        }
    }
    return;
}

// test dispatch message API call
TEST_F(ComponentHidlTest, dispatchMsg) {
    description("test dispatch message API call");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    Message msgin, msgout;

    msgin.type = Message::Type::EVENT;
    msgin.data.eventData.event = OMX_EventError;
    msgin.data.eventData.data1 = 0xdeaf;
    msgin.data.eventData.data2 = 0xd00d;
    msgin.data.eventData.data3 = 0x01ce;
    msgin.data.eventData.data4 = 0xfa11;
    status = omxNode->dispatchMessage(msgin);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msgout, DEFAULT_TIMEOUT);
    EXPECT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    EXPECT_EQ(msgout.type, msgin.type);
    EXPECT_EQ(msgout.data.eventData.event, msgin.data.eventData.event);
    EXPECT_EQ(msgout.data.eventData.data1, msgin.data.eventData.data1);
    EXPECT_EQ(msgout.data.eventData.data2, msgin.data.eventData.data2);
    EXPECT_EQ(msgout.data.eventData.data3, msgin.data.eventData.data3);
    EXPECT_EQ(msgout.data.eventData.data4, msgin.data.eventData.data4);
}

// set component role
TEST_F(ComponentHidlTest, SetRole) {
    description("Test Set Component Role");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

// port indices enumeration
TEST_F(ComponentHidlTest, DISABLED_GetPortIndices) {
    description("Test Component on Mandatory Port Parameters (Port Indices)");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_PORT_PARAM_TYPE params;

    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    // Get Number of Ports and their Indices for all Domains
    // (Audio/Video/Image/Other)
    // All standard OMX components shall support following OMX Index types
    status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = getParam(omxNode, OMX_IndexParamImageInit, &params);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = getParam(omxNode, OMX_IndexParamOtherInit, &params);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

// port format enumeration
TEST_F(ComponentHidlTest, DISABLED_EnumeratePortFormat) {
    description("Test Component on Mandatory Port Parameters (Port Format)");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;

    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }

    OMX_COLOR_FORMATTYPE eColorFormat = OMX_COLOR_FormatYUV420Planar;
    OMX_U32 xFramerate = 24U << 16;

    // Enumerate Port Format
    if (compClass == audio_encoder) {
        status =
            setAudioPortFormat(omxNode, kPortIndexInput, OMX_AUDIO_CodingPCM);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        status = setAudioPortFormat(omxNode, kPortIndexOutput,
                                    OMX_AUDIO_CodingAutoDetect);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    } else if (compClass == audio_decoder) {
        status = setAudioPortFormat(omxNode, kPortIndexInput,
                                    OMX_AUDIO_CodingAutoDetect);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        status =
            setAudioPortFormat(omxNode, kPortIndexOutput, OMX_AUDIO_CodingPCM);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    } else if (compClass == video_encoder) {
        status =
            setVideoPortFormat(omxNode, kPortIndexInput, OMX_VIDEO_CodingUnused,
                               eColorFormat, xFramerate);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        status = setVideoPortFormat(omxNode, kPortIndexOutput,
                                    OMX_VIDEO_CodingAutoDetect,
                                    OMX_COLOR_FormatUnused, 0U);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    } else {
        status = setVideoPortFormat(omxNode, kPortIndexInput,
                                    OMX_VIDEO_CodingAutoDetect,
                                    OMX_COLOR_FormatUnused, 0U);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        status = setVideoPortFormat(omxNode, kPortIndexOutput,
                                    OMX_VIDEO_CodingUnused, eColorFormat,
                                    xFramerate);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    }
}

// get/set default port settings of a component
TEST_F(ComponentHidlTest, DISABLED_SetDefaultPortParams) {
    description(
        "Test Component on Mandatory Port Parameters (Port Definition)");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;

    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }

    for (size_t i = kPortIndexInput; i < kPortIndexOutput; i++) {
        OMX_PARAM_PORTDEFINITIONTYPE portDef;
        status =
            getPortParam(omxNode, OMX_IndexParamPortDefinition, i, &portDef);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        if (status == android::hardware::media::omx::V1_0::Status::OK) {
            EXPECT_EQ(portDef.eDir, i - kPortIndexInput);  // OMX_DirInput
            EXPECT_EQ(portDef.bEnabled, OMX_TRUE);
            EXPECT_EQ(portDef.bPopulated, OMX_FALSE);
            EXPECT_GE(portDef.nBufferCountMin, 1U);
            EXPECT_GE(portDef.nBufferCountActual, portDef.nBufferCountMin);
            if (compClass == audio_encoder || compClass == audio_decoder) {
                EXPECT_EQ(portDef.eDomain, OMX_PortDomainAudio);
            } else if (compClass == video_encoder ||
                       compClass == video_decoder) {
                EXPECT_EQ(portDef.eDomain, OMX_PortDomainVideo);
            }
            OMX_PARAM_PORTDEFINITIONTYPE mirror = portDef;

            // nBufferCountActual >= nBufferCountMin
            portDef.nBufferCountActual = portDef.nBufferCountMin - 1;
            status = setPortParam(omxNode, OMX_IndexParamPortDefinition, i,
                                  &portDef);
            EXPECT_NE(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);

            // Edit Read-Only fields.
            portDef = mirror;
            portDef.eDir = static_cast<OMX_DIRTYPE>(RANDOM_INDEX);
            setPortParam(omxNode, OMX_IndexParamPortDefinition, i, &portDef);
            getPortParam(omxNode, OMX_IndexParamPortDefinition, i, &portDef);
            EXPECT_EQ(portDef.eDir, mirror.eDir);
            setPortParam(omxNode, OMX_IndexParamPortDefinition, i, &mirror);

            portDef = mirror;
            portDef.nBufferSize >>= 1;
            setPortParam(omxNode, OMX_IndexParamPortDefinition, i, &portDef);
            getPortParam(omxNode, OMX_IndexParamPortDefinition, i, &portDef);
            EXPECT_EQ(portDef.nBufferSize, mirror.nBufferSize);
            setPortParam(omxNode, OMX_IndexParamPortDefinition, i, &mirror);

            portDef = mirror;
            portDef.nBufferCountMin += 1;
            setPortParam(omxNode, OMX_IndexParamPortDefinition, i, &portDef);
            getPortParam(omxNode, OMX_IndexParamPortDefinition, i, &portDef);
            EXPECT_EQ(portDef.nBufferCountMin, mirror.nBufferCountMin);
            setPortParam(omxNode, OMX_IndexParamPortDefinition, i, &mirror);

            portDef = mirror;
            portDef.nBufferCountActual += 1;
            status = setPortParam(omxNode, OMX_IndexParamPortDefinition, i,
                                  &portDef);
            if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
                status = getPortParam(omxNode, OMX_IndexParamPortDefinition, i,
                                      &portDef);
                EXPECT_EQ(portDef.nBufferCountActual,
                          mirror.nBufferCountActual + 1);
            }

            portDef = mirror;
            portDef.nBufferSize = mirror.nBufferSize << 1;
            status = setPortParam(omxNode, OMX_IndexParamPortDefinition, i,
                                  &portDef);
            if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
                status = getPortParam(omxNode, OMX_IndexParamPortDefinition, i,
                                      &portDef);
                if (portDef.nBufferSize != mirror.nBufferSize) {
                    std::cout
                        << "[          ] Warning ! Component input port does "
                           "not  preserve Read-Only fields \n";
                }
            }
        }
    }
}

// populate port test
TEST_F(ComponentHidlTest, DISABLED_PopulatePort) {
    description("Verify bPopulated field of a component port");
    if (disableTest || isSecure) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_U32 portBase = 0;

    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        portBase = params.nStartPortNumber;
    }

    sp<IAllocator> allocator = IAllocator::getService("ashmem");
    EXPECT_NE(allocator.get(), nullptr);

    OMX_PARAM_PORTDEFINITIONTYPE portDef;
    status =
        getPortParam(omxNode, OMX_IndexParamPortDefinition, portBase, &portDef);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(portDef.bPopulated, OMX_FALSE);

    android::Vector<BufferInfo> pBuffer;
    pBuffer.clear();
    uint32_t nBufferSize = portDef.nBufferSize >> 1;

    for (size_t i = 0; i < portDef.nBufferCountActual; i++) {
        BufferInfo buffer;
        buffer.owner = client;
        buffer.omxBuffer.type = CodecBuffer::Type::SHARED_MEM;
        buffer.omxBuffer.attr.preset.rangeOffset = 0;
        buffer.omxBuffer.attr.preset.rangeLength = 0;
        bool success = false;
        allocator->allocate(
            nBufferSize,
            [&success, &buffer](bool _s,
                                ::android::hardware::hidl_memory const& mem) {
                success = _s;
                buffer.omxBuffer.sharedMemory = mem;
            });
        ASSERT_EQ(success, true);
        ASSERT_EQ(buffer.omxBuffer.sharedMemory.size(), nBufferSize);

        omxNode->useBuffer(
            portBase, buffer.omxBuffer,
            [&status, &buffer](android::hardware::media::omx::V1_0::Status _s,
                               uint32_t id) {
                status = _s;
                buffer.id = id;
            });
        pBuffer.push(buffer);
        ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    }

    status =
        getPortParam(omxNode, OMX_IndexParamPortDefinition, portBase, &portDef);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    // A port is populated when all of the buffers indicated by
    // nBufferCountActual with a size of at least nBufferSizehave been
    // allocated on the port.
    ASSERT_EQ(portDef.bPopulated, OMX_FALSE);
}

// Flush test
TEST_F(ComponentHidlTest, Flush) {
    description("Test Flush");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    Message msg;

    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }

    android::Vector<BufferInfo> iBuffer, oBuffer;

    // set port mode
    PortMode portMode[2];
    initPortMode(portMode, isSecure, compClass);
    status = omxNode->setPortMode(kPortIndexInput, portMode[0]);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = omxNode->setPortMode(kPortIndexOutput, portMode[1]);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    // set state to idle
    changeStateLoadedtoIdle(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput, portMode);
    // set state to executing
    changeStateIdletoExecute(omxNode, observer);
    // dispatch buffers
    for (size_t i = 0; i < oBuffer.size(); i++) {
        dispatchOutputBuffer(omxNode, &oBuffer, i, portMode[1]);
    }
    // flush port
    flushPorts(omxNode, observer, &iBuffer, &oBuffer, kPortIndexInput,
               kPortIndexOutput);
    // TODO: Sending empty input buffers is slightly tricky.
    // Components sometimes process input buffers even when output buffers are
    // not dispatched. For instance Parsing sequence header does not require
    // output buffers. In such instances sending 0 size input buffers might
    // make component to send error events. so lets skip this aspect of testing.
    // dispatch buffers
    //    for (size_t i = 0; i < iBuffer.size(); i++) {
    //        dispatchInputBuffer(omxNode, &iBuffer, i, 0, 0, 0, portMode[0]);
    //    }
    //    // flush ports
    //    flushPorts(omxNode, observer, &iBuffer, &oBuffer, kPortIndexInput,
    //               kPortIndexOutput);
    // set state to idle
    changeStateExecutetoIdle(omxNode, observer, &iBuffer, &oBuffer);
    // set state to loaded
    changeStateIdletoLoaded(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput);
}

// state transitions test
TEST_F(ComponentHidlTest, StateTransitions) {
    description("Test State Transitions Loaded<->Idle<->Execute");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    Message msg;

    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }

    android::Vector<BufferInfo> iBuffer, oBuffer;

    // set port mode
    PortMode portMode[2];
    initPortMode(portMode, isSecure, compClass);
    status = omxNode->setPortMode(kPortIndexInput, portMode[0]);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = omxNode->setPortMode(kPortIndexOutput, portMode[1]);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    // set state to idle
    changeStateLoadedtoIdle(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput, portMode);
    // set state to executing
    changeStateIdletoExecute(omxNode, observer);
    // dispatch buffers
    for (size_t i = 0; i < oBuffer.size(); i++) {
        dispatchOutputBuffer(omxNode, &oBuffer, i, portMode[1]);
    }
    // set state to idle
    changeStateExecutetoIdle(omxNode, observer, &iBuffer, &oBuffer);
    //    // set state to executing
    //    changeStateIdletoExecute(omxNode, observer);
    //    // TODO: Sending empty input buffers is slightly tricky.
    //    // dispatch buffers
    //    for (size_t i = 0; i < iBuffer.size(); i++) {
    //        dispatchInputBuffer(omxNode, &iBuffer, i, 0, 0, 0, portMode[0]);
    //    }
    //    // set state to idle
    //    changeStateExecutetoIdle(omxNode, observer, &iBuffer, &oBuffer);
    // set state to loaded
    changeStateIdletoLoaded(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput);
}

// state transitions test - monkeying
TEST_F(ComponentHidlTest, DISABLED_StateTransitions_M) {
    description("Test State Transitions monkeying");
    if (disableTest || isSecure) return;
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    Message msg;

    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }

    android::Vector<BufferInfo> iBuffer, oBuffer;

    // set state to loaded ; receive error OMX_ErrorSameState
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateLoaded);
    EXPECT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // set state to executing ; receive error OMX_ErrorIncorrectStateTransition
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateExecuting);
    EXPECT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // set state to idle
    changeStateLoadedtoIdle(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput);

    // set state to idle ; receive error OMX_ErrorSameState
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    EXPECT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // set state to executing
    changeStateIdletoExecute(omxNode, observer);

    // set state to executing ; receive error OMX_ErrorSameState
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateExecuting);
    EXPECT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // set state to Loaded ; receive error OMX_ErrorIncorrectStateTransition
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateLoaded);
    EXPECT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // set state to Idle
    changeStateExecutetoIdle(omxNode, observer, &iBuffer, &oBuffer);

    // set state to Loaded
    changeStateIdletoLoaded(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput);
}

// port enable disable test
TEST_F(ComponentHidlTest, DISABLED_PortEnableDisable_Loaded) {
    description("Test Port Enable and Disable (Component State :: Loaded)");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_U32 portBase = 0;
    Message msg;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        portBase = params.nStartPortNumber;
    }

    for (size_t i = portBase; i < portBase + 2; i++) {
        status =
            omxNode->sendCommand(toRawCommandType(OMX_CommandPortDisable), i);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
        status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
        ASSERT_EQ(msg.type, Message::Type::EVENT);
        if (msg.data.eventData.event == OMX_EventCmdComplete) {
            ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortDisable);
            ASSERT_EQ(msg.data.eventData.data2, i);
            // If you can disable a port, then you should be able to enable it
            // as well
            status = omxNode->sendCommand(
                toRawCommandType(OMX_CommandPortEnable), i);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
            status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
            ASSERT_EQ(msg.type, Message::Type::EVENT);
            ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortEnable);
            ASSERT_EQ(msg.data.eventData.data2, i);
        } else if (msg.data.eventData.event == OMX_EventError) {
            ALOGE("Port %d Disabling failed with error %d", (int)i,
                  (int)msg.data.eventData.event);
        } else {
            // something unexpected happened
            ASSERT_TRUE(false);
        }
    }
}

// port enable disable test
TEST_F(ComponentHidlTest, PortEnableDisable_Idle) {
    description("Test Port Enable and Disable (Component State :: Idle)");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    OMX_U32 portBase = 0;
    Message msg;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        portBase = params.nStartPortNumber;
    }
    kPortIndexInput = portBase;
    kPortIndexOutput = portBase + 1;

    // Component State :: Idle
    android::Vector<BufferInfo> pBuffer[2];

    // set port mode
    PortMode portMode[2];
    initPortMode(portMode, isSecure, compClass);
    status = omxNode->setPortMode(kPortIndexInput, portMode[0]);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = omxNode->setPortMode(kPortIndexOutput, portMode[1]);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    // set state to idle
    changeStateLoadedtoIdle(omxNode, observer, &pBuffer[0], &pBuffer[1],
                            kPortIndexInput, kPortIndexOutput, portMode);

    for (size_t i = portBase; i < portBase + 2; i++) {
        status =
            omxNode->sendCommand(toRawCommandType(OMX_CommandPortDisable), i);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

        status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &pBuffer[0],
                                          &pBuffer[1]);
        if (status == android::hardware::media::omx::V1_0::Status::OK) {
            ASSERT_EQ(msg.type, Message::Type::EVENT);
            if (msg.data.eventData.event == OMX_EventCmdComplete) {
                // do not disable the port until all the buffers are freed
                ASSERT_TRUE(false);
            } else if (msg.data.eventData.event == OMX_EventError) {
                ALOGE("Port %d Disabling failed with error %d", (int)i,
                      (int)msg.data.eventData.event);
            } else {
                // something unexpected happened
                ASSERT_TRUE(false);
            }
        } else if (status ==
                   android::hardware::media::omx::V1_0::Status::TIMED_OUT) {
            for (size_t j = 0; j < pBuffer[i - portBase].size(); ++j) {
                status = omxNode->freeBuffer(i, pBuffer[i - portBase][j].id);
                ASSERT_EQ(status,
                          android::hardware::media::omx::V1_0::Status::OK);
            }

            status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                              &pBuffer[0], &pBuffer[1]);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
            ASSERT_EQ(msg.type, Message::Type::EVENT);
            ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
            ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortDisable);
            ASSERT_EQ(msg.data.eventData.data2, i);

            // If you can disable a port, then you should be able to enable it
            // as well
            status = omxNode->sendCommand(
                toRawCommandType(OMX_CommandPortEnable), i);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

            // do not enable the port until all the buffers are supplied
            status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                              &pBuffer[0], &pBuffer[1]);
            ASSERT_EQ(status,
                      android::hardware::media::omx::V1_0::Status::TIMED_OUT);

            allocatePortBuffers(omxNode, &pBuffer[i - portBase], i,
                                portMode[i - portBase]);
            status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                              &pBuffer[0], &pBuffer[1]);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
            ASSERT_EQ(msg.type, Message::Type::EVENT);
            ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortEnable);
            ASSERT_EQ(msg.data.eventData.data2, i);
        } else {
            // something unexpected happened
            ASSERT_TRUE(false);
        }
    }

    // set state to Loaded
    changeStateIdletoLoaded(omxNode, observer, &pBuffer[0], &pBuffer[1],
                            kPortIndexInput, kPortIndexOutput);
}

// port enable disable test
TEST_F(ComponentHidlTest, PortEnableDisable_Execute) {
    description("Test Port Enable and Disable (Component State :: Execute)");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    OMX_U32 portBase = 0;
    Message msg;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        portBase = params.nStartPortNumber;
    }
    kPortIndexInput = portBase;
    kPortIndexOutput = portBase + 1;

    // Component State :: Idle
    android::Vector<BufferInfo> pBuffer[2];

    // set port mode
    PortMode portMode[2];
    initPortMode(portMode, isSecure, compClass);
    status = omxNode->setPortMode(kPortIndexInput, portMode[0]);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = omxNode->setPortMode(kPortIndexOutput, portMode[1]);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    // set state to idle
    changeStateLoadedtoIdle(omxNode, observer, &pBuffer[0], &pBuffer[1],
                            kPortIndexInput, kPortIndexOutput, portMode);

    // set state to executing
    changeStateIdletoExecute(omxNode, observer);

    // dispatch buffers
    for (size_t i = 0; i < pBuffer[1].size(); i++) {
        dispatchOutputBuffer(omxNode, &pBuffer[1], i, portMode[1]);
    }

    for (size_t i = portBase; i < portBase + 2; i++) {
        status =
            omxNode->sendCommand(toRawCommandType(OMX_CommandPortDisable), i);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

        status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &pBuffer[0],
                                          &pBuffer[1]);
        if (status == android::hardware::media::omx::V1_0::Status::OK) {
            ASSERT_EQ(msg.type, Message::Type::EVENT);
            if (msg.data.eventData.event == OMX_EventCmdComplete) {
                // do not disable the port until all the buffers are freed
                ASSERT_TRUE(false);
            } else if (msg.data.eventData.event == OMX_EventError) {
                ALOGE("Port %d Disabling failed with error %d", (int)i,
                      (int)msg.data.eventData.event);
            } else {
                // something unexpected happened
                ASSERT_TRUE(false);
            }
        } else if (status ==
                   android::hardware::media::omx::V1_0::Status::TIMED_OUT) {
            for (size_t j = 0; j < pBuffer[i - portBase].size(); ++j) {
                // test if client got all its buffers back
                EXPECT_EQ(pBuffer[i - portBase][j].owner, client);
                // free the buffers
                status = omxNode->freeBuffer(i, pBuffer[i - portBase][j].id);
                ASSERT_EQ(status,
                          android::hardware::media::omx::V1_0::Status::OK);
            }

            status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                              &pBuffer[0], &pBuffer[1]);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
            ASSERT_EQ(msg.type, Message::Type::EVENT);
            ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
            ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortDisable);
            ASSERT_EQ(msg.data.eventData.data2, i);

            // If you can disable a port, then you should be able to enable it
            // as well
            status = omxNode->sendCommand(
                toRawCommandType(OMX_CommandPortEnable), i);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

            // do not enable the port until all the buffers are supplied
            status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                              &pBuffer[0], &pBuffer[1]);
            ASSERT_EQ(status,
                      android::hardware::media::omx::V1_0::Status::TIMED_OUT);

            allocatePortBuffers(omxNode, &pBuffer[i - portBase], i,
                                portMode[i - portBase]);
            status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                              &pBuffer[0], &pBuffer[1]);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
            ASSERT_EQ(msg.type, Message::Type::EVENT);
            ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortEnable);
            ASSERT_EQ(msg.data.eventData.data2, i);
        } else {
            // something unexpected happened
            ASSERT_TRUE(false);
        }
    }

    // set state to Idle
    changeStateExecutetoIdle(omxNode, observer, &pBuffer[0], &pBuffer[1]);

    // set state to Loaded
    changeStateIdletoLoaded(omxNode, observer, &pBuffer[0], &pBuffer[1],
                            kPortIndexInput, kPortIndexOutput);
}

// port enable disable test - monkeying
TEST_F(ComponentHidlTest, DISABLED_PortEnableDisable_M) {
    description(
        "Test Port Enable and Disable Monkeying (Component State :: Loaded)");
    if (disableTest || isSecure) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_U32 portBase = 0;
    Message msg;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    if (compClass == audio_decoder || compClass == audio_encoder) {
        status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    } else {
        status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    }
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        portBase = params.nStartPortNumber;
    }

    // disable invalid port, expecting OMX_ErrorBadPortIndex
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandPortDisable),
                                  RANDOM_INDEX);
    ASSERT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // enable invalid port, expecting OMX_ErrorBadPortIndex
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandPortEnable),
                                  RANDOM_INDEX);
    ASSERT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // disable all ports
    status =
        omxNode->sendCommand(toRawCommandType(OMX_CommandPortDisable), OMX_ALL);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    for (size_t i = 0; i < 2; i++) {
        status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
        ASSERT_EQ(msg.type, Message::Type::EVENT);
        if (msg.data.eventData.event == OMX_EventCmdComplete) {
            ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortDisable);
            if (msg.data.eventData.data2 != portBase ||
                msg.data.eventData.data2 != portBase + 1)
                EXPECT_TRUE(false);
        } else if (msg.data.eventData.event == OMX_EventError) {
            ALOGE("Port %d Disabling failed with error %d", (int)i,
                  (int)msg.data.eventData.event);
        } else {
            // something unexpected happened
            ASSERT_TRUE(false);
        }
    }

    // enable all ports
    status =
        omxNode->sendCommand(toRawCommandType(OMX_CommandPortEnable), OMX_ALL);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    for (size_t i = 0; i < 2; i++) {
        status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
        ASSERT_EQ(msg.type, Message::Type::EVENT);
        if (msg.data.eventData.event == OMX_EventCmdComplete) {
            ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortEnable);
            if (msg.data.eventData.data2 != portBase ||
                msg.data.eventData.data2 != portBase + 1)
                EXPECT_TRUE(false);
        } else if (msg.data.eventData.event == OMX_EventError) {
            ALOGE("Port %d Enabling failed with error %d", (int)i,
                  (int)msg.data.eventData.event);
        } else {
            // something unexpected happened
            ASSERT_TRUE(false);
        }
    }
}

int main(int argc, char** argv) {
    gEnv = new ComponentTestEnvironment();
    ::testing::AddGlobalTestEnvironment(gEnv);
    ::testing::InitGoogleTest(&argc, argv);
    int status = gEnv->initFromOptions(argc, argv);
    if (status == 0) {
        status = RUN_ALL_TESTS();
        ALOGI("Test result = %d", status);
    }
    return status;
}
