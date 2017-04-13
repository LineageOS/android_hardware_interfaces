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

    void setQuirks(int _quirks) { quirks = _quirks; }

    const hidl_string getInstance() const { return instance; }

    const hidl_string getComponent() const { return component; }

    const hidl_string getRole() const { return role; }

    int getQuirks() const { return quirks; }

    int initFromOptions(int argc, char** argv) {
        static struct option options[] = {
            {"instance", required_argument, 0, 'I'},
            {"component", required_argument, 0, 'C'},
            {"role", required_argument, 0, 'R'},
            {"quirks", required_argument, 0, 'Q'},
            {0, 0, 0, 0}};

        while (true) {
            int index = 0;
            int c = getopt_long(argc, argv, "I:C:Q:R:", options, &index);
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
                case 'Q':
                    setQuirks(atoi(optarg));
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
                    "-R, --Role: OMX component Role\n"
                    "-Q, --quirks: Component quirks\n",
                    argv[optind ?: 1], argv[0]);
            return 2;
        }
        return 0;
    }

   private:
    hidl_string instance;
    hidl_string component;
    hidl_string role;
    // to be removed when IOmxNode::setQuirks is removed
    int quirks;
};

static ComponentTestEnvironment* gEnv = nullptr;

class ComponentHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        android::hardware::media::omx::V1_0::Status status;
        omx = ::testing::VtsHalHidlTargetTestBase::getService<IOmx>(
            gEnv->getInstance());
        ASSERT_NE(omx, nullptr);
        observer = new CodecObserver();
        ASSERT_NE(observer, nullptr);
        ASSERT_EQ(strncmp(gEnv->getComponent().c_str(), "OMX.", 4), 0)
            << "Invalid Component Name";
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
        ASSERT_NE(compClass, unknown_class) << "Invalid Component Class";

        allocator = ::testing::VtsHalHidlTargetTestBase::getService<IAllocator>(
            "ashmem");
        ASSERT_NE(allocator, nullptr);
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
    sp<IAllocator> allocator;
    standardCompClass compClass;
};

#define RANDOM_INDEX 1729

void allocatePortBuffers(sp<IOmxNode> omxNode, sp<IAllocator> allocator,
                         android::Vector<BufferInfo>* buffArray,
                         OMX_U32 portIndex) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_PARAM_PORTDEFINITIONTYPE portDef;

    buffArray->clear();

    status = getPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    for (size_t i = 0; i < portDef.nBufferCountActual; i++) {
        BufferInfo buffer;
        buffer.owner = client;
        buffer.omxBuffer.type = CodecBuffer::Type::SHARED_MEM;
        buffer.omxBuffer.attr.preset.rangeOffset = 0;
        buffer.omxBuffer.attr.preset.rangeLength = 0;
        bool success;
        allocator->allocate(
            portDef.nBufferSize,
            [&success, &buffer](bool _s,
                                ::android::hardware::hidl_memory const& mem) {
                success = _s;
                buffer.omxBuffer.sharedMemory = mem;
            });
        ASSERT_EQ(success, true);

        omxNode->useBuffer(
            portIndex, buffer.omxBuffer,
            [&status, &buffer](android::hardware::media::omx::V1_0::Status _s,
                               uint32_t id) {
                status = _s;
                buffer.id = id;
            });
        buffArray->push(buffer);
        ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    }
}

Return<android::hardware::media::omx::V1_0::Status> setVideoPortFormat(
    sp<IOmxNode> omxNode, OMX_U32 portIndex,
    OMX_VIDEO_CODINGTYPE compressionFormat, OMX_COLOR_FORMATTYPE colorFormat,
    OMX_U32 frameRate) {
    OMX_U32 index = 0;
    OMX_VIDEO_PARAM_PORTFORMATTYPE portFormat;
    std::vector<OMX_COLOR_FORMATTYPE> eColorFormat;
    std::vector<OMX_VIDEO_CODINGTYPE> eCompressionFormat;
    android::hardware::media::omx::V1_0::Status status;

    while (1) {
        portFormat.nIndex = index;
        status = getPortParam(omxNode, OMX_IndexParamVideoPortFormat, portIndex,
                              &portFormat);
        if (status != ::android::hardware::media::omx::V1_0::Status::OK) break;
        if (compressionFormat == OMX_VIDEO_CodingUnused)
            eColorFormat.push_back(portFormat.eColorFormat);
        else
            eCompressionFormat.push_back(portFormat.eCompressionFormat);
        index++;
        if (index == 512) {
            EXPECT_LE(index, 512U)
                << "Expecting OMX_ErrorNoMore but not received";
            break;
        }
    }
    if (!index) return status;
    if (compressionFormat == OMX_VIDEO_CodingUnused) {
        for (index = 0; index < eColorFormat.size(); index++) {
            if (eColorFormat[index] == colorFormat) {
                portFormat.eColorFormat = eColorFormat[index];
                break;
            }
        }
        if (index == eColorFormat.size()) {
            ALOGI("setting default color format");
            portFormat.eColorFormat = eColorFormat[0];
        }
        portFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;
    } else {
        for (index = 0; index < eCompressionFormat.size(); index++) {
            if (eCompressionFormat[index] == compressionFormat) {
                portFormat.eCompressionFormat = eCompressionFormat[index];
                break;
            }
        }
        if (index == eCompressionFormat.size()) {
            ALOGI("setting default compression format");
            portFormat.eCompressionFormat = eCompressionFormat[0];
        }
        portFormat.eColorFormat = OMX_COLOR_FormatUnused;
    }
    // In setParam call nIndex shall be ignored as per omx-il specification.
    // see how this holds up by corrupting nIndex
    portFormat.nIndex = RANDOM_INDEX;
    portFormat.xFramerate = frameRate;
    status = setPortParam(omxNode, OMX_IndexParamVideoPortFormat, portIndex,
                          &portFormat);
    return status;
}

Return<android::hardware::media::omx::V1_0::Status> setAudioPortFormat(
    sp<IOmxNode> omxNode, OMX_U32 portIndex, OMX_AUDIO_CODINGTYPE encoding) {
    OMX_U32 index = 0;
    OMX_AUDIO_PARAM_PORTFORMATTYPE portFormat;
    std::vector<OMX_AUDIO_CODINGTYPE> eEncoding;
    android::hardware::media::omx::V1_0::Status status;

    while (1) {
        portFormat.nIndex = index;
        status = getPortParam(omxNode, OMX_IndexParamAudioPortFormat, portIndex,
                              &portFormat);
        if (status != ::android::hardware::media::omx::V1_0::Status::OK) break;
        eEncoding.push_back(portFormat.eEncoding);
        index++;
        if (index == 512) {
            EXPECT_LE(index, 512U)
                << "Expecting OMX_ErrorNoMore but not received";
            break;
        }
    }
    if (!index) return status;
    for (index = 0; index < eEncoding.size(); index++) {
        if (eEncoding[index] == encoding) {
            portFormat.eEncoding = eEncoding[index];
            break;
        }
    }
    if (index == eEncoding.size()) {
        ALOGI("setting default Port format");
        portFormat.eEncoding = eEncoding[0];
    }
    // In setParam call nIndex shall be ignored as per omx-il specification.
    // see how this holds up by corrupting nIndex
    portFormat.nIndex = RANDOM_INDEX;
    status = setPortParam(omxNode, OMX_IndexParamAudioPortFormat, portIndex,
                          &portFormat);
    return status;
}

Return<android::hardware::media::omx::V1_0::Status> setRole(
    sp<IOmxNode> omxNode, const char* role) {
    OMX_PARAM_COMPONENTROLETYPE params;
    strcpy((char*)params.cRole, role);
    return setParam(omxNode, OMX_IndexParamStandardComponentRole, &params);
}

// Set Component Role
TEST_F(ComponentHidlTest, SetRole) {
    android::hardware::media::omx::V1_0::Status status;
    status = setRole(omxNode, gEnv->getRole().c_str());
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

// Get Number of Ports and their Indices for all Domains
// (Audio/Video/Image/Other)
TEST_F(ComponentHidlTest, GetPortIndices) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_PORT_PARAM_TYPE params;

    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

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

// Enumerate Port Format
TEST_F(ComponentHidlTest, EnumeratePortFormat) {
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

    OMX_COLOR_FORMATTYPE colorFormat = OMX_COLOR_FormatYUV420Planar;
    OMX_U32 frameRate = 24 << 16;

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
                               colorFormat, frameRate);
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
        status =
            setVideoPortFormat(omxNode, kPortIndexOutput,
                               OMX_VIDEO_CodingUnused, colorFormat, frameRate);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    }
}

// r/w default i/o port parameters
TEST_F(ComponentHidlTest, SetDefaultPortParams) {
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

    OMX_PARAM_PORTDEFINITIONTYPE iPortDef;
    status = getPortParam(omxNode, OMX_IndexParamPortDefinition,
                          kPortIndexInput, &iPortDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    if (status == android::hardware::media::omx::V1_0::Status::OK) {
        EXPECT_EQ(iPortDef.eDir, OMX_DirInput);
        EXPECT_EQ(iPortDef.bEnabled, OMX_TRUE);
        EXPECT_EQ(iPortDef.bPopulated, 0);
        EXPECT_GE(iPortDef.nBufferCountMin, 1U);
        EXPECT_GE(iPortDef.nBufferCountActual, iPortDef.nBufferCountMin);
        if (compClass == audio_encoder || compClass == audio_decoder) {
            EXPECT_EQ(iPortDef.eDomain, OMX_PortDomainAudio);
            if (compClass == audio_decoder) {
                iPortDef.format.audio.bFlagErrorConcealment = OMX_TRUE;
                status = setPortParam(omxNode, OMX_IndexParamPortDefinition,
                                      kPortIndexInput, &iPortDef);
            }
            EXPECT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
        } else if (compClass == video_encoder || compClass == video_decoder) {
            EXPECT_EQ(iPortDef.eDomain, OMX_PortDomainVideo);
        }
        OMX_PARAM_PORTDEFINITIONTYPE dummy = iPortDef;
        iPortDef.nBufferCountActual = iPortDef.nBufferCountMin - 1;
        status = setPortParam(omxNode, OMX_IndexParamPortDefinition,
                              kPortIndexInput, &iPortDef);
        EXPECT_NE(status, ::android::hardware::media::omx::V1_0::Status::OK);
        // Edit Read-Only fields.
        iPortDef.eDir = OMX_DirOutput;  // Read Only field
        iPortDef.nBufferCountActual = dummy.nBufferCountActual << 1;
        iPortDef.nBufferCountMin = dummy.nBufferCountMin
                                   << 1;                // Read Only field
        iPortDef.nBufferSize = dummy.nBufferSize << 1;  // Read Only field
        status = setPortParam(omxNode, OMX_IndexParamPortDefinition,
                              kPortIndexInput, &iPortDef);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        status = getPortParam(omxNode, OMX_IndexParamPortDefinition,
                              kPortIndexInput, &iPortDef);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        EXPECT_EQ(iPortDef.nBufferCountActual, dummy.nBufferCountActual << 1);
        if ((iPortDef.eDir != OMX_DirInput) ||
            (iPortDef.nBufferCountMin != dummy.nBufferCountMin) ||
            (iPortDef.nBufferSize != dummy.nBufferSize)) {
            ALOGI(
                "Warning, Component does not seem to preserve Read-Only "
                "fields");
            printf(
                "Warning, Component does not seem to preserve Read-Only fields "
                "\n");
        }
    }

    OMX_PARAM_PORTDEFINITIONTYPE oPortDef;
    status = getPortParam(omxNode, OMX_IndexParamPortDefinition,
                          kPortIndexOutput, &oPortDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        EXPECT_EQ(oPortDef.eDir, OMX_DirOutput);
        EXPECT_EQ(oPortDef.bEnabled, OMX_TRUE);
        EXPECT_EQ(oPortDef.bPopulated, 0);
        EXPECT_GE(oPortDef.nBufferCountMin, 1U);
        EXPECT_GE(oPortDef.nBufferCountActual, oPortDef.nBufferCountMin);
        if (compClass == audio_encoder || compClass == audio_decoder) {
            EXPECT_EQ(oPortDef.eDomain, OMX_PortDomainAudio);
            if (compClass == audio_encoder) {
                oPortDef.format.audio.bFlagErrorConcealment = OMX_TRUE;
                status = setPortParam(omxNode, OMX_IndexParamPortDefinition,
                                      kPortIndexOutput, &oPortDef);
            }
            EXPECT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
        } else if (compClass == video_encoder || compClass == video_decoder) {
            EXPECT_EQ(oPortDef.eDomain, OMX_PortDomainVideo);
        }
        OMX_PARAM_PORTDEFINITIONTYPE dummy = oPortDef;
        oPortDef.nBufferCountActual = oPortDef.nBufferCountMin - 1;
        status = setPortParam(omxNode, OMX_IndexParamPortDefinition,
                              kPortIndexOutput, &oPortDef);
        EXPECT_NE(status, ::android::hardware::media::omx::V1_0::Status::OK);
        // Edit Read-Only fields.
        oPortDef.eDir = OMX_DirInput;  // Read Only field
        oPortDef.nBufferCountActual = dummy.nBufferCountActual << 1;
        oPortDef.nBufferCountMin = dummy.nBufferCountMin
                                   << 1;                // Read Only field
        oPortDef.nBufferSize = dummy.nBufferSize << 1;  // Read Only field
        status = setPortParam(omxNode, OMX_IndexParamPortDefinition,
                              kPortIndexOutput, &oPortDef);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        status = getPortParam(omxNode, OMX_IndexParamPortDefinition,
                              kPortIndexOutput, &oPortDef);
        EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
        EXPECT_EQ(oPortDef.nBufferCountActual, dummy.nBufferCountActual << 1);
        if ((oPortDef.eDir != OMX_DirOutput) ||
            (oPortDef.nBufferCountMin != dummy.nBufferCountMin) ||
            (oPortDef.nBufferSize != dummy.nBufferSize)) {
            ALOGI(
                "Warning, Component does not seem to preserve Read-Only "
                "fields");
            printf(
                "Warning, Component does not seem to preserve Read-Only fields "
                "\n");
        }
    }
}

// Test State Transitions & Flush
TEST_F(ComponentHidlTest, StateTransitions_Flush) {
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

    // set state to idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    // Dont switch states until the ports got their buffers
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    // allocate buffers on input port
    allocatePortBuffers(omxNode, allocator, &iBuffer, kPortIndexInput);

    // Dont switch states until the ports got their buffers
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    // allocate buffers on output port
    allocatePortBuffers(omxNode, allocator, &oBuffer, kPortIndexOutput);

    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

    // set state to executing
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateExecuting);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateExecuting);

    CodecBuffer t;
    t.sharedMemory = android::hardware::hidl_memory();
    t.nativeHandle = android::hardware::hidl_handle();
    t.type = CodecBuffer::Type::PRESET;
    t.attr.preset.rangeOffset = 0;
    t.attr.preset.rangeLength = 0;
    for (size_t i = 0; i < oBuffer.size(); ++i) {
        native_handle_t* fenceNh = native_handle_create(0, 0);
        status = omxNode->fillBuffer(oBuffer[i].id, t, fenceNh);
        native_handle_close(fenceNh);
        native_handle_delete(fenceNh);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
        oBuffer.editItemAt(i).owner = component;
    }

    // set state to Idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

    // test if client got all its buffers back
    for (size_t i = 0; i < oBuffer.size(); ++i) {
        EXPECT_EQ(oBuffer[i].owner, client);
    }

    // set state to executing
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateExecuting);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateExecuting);

    for (size_t i = 0; i < oBuffer.size(); i += 2) {
        native_handle_t* fenceNh = native_handle_create(0, 0);
        status = omxNode->fillBuffer(oBuffer[i].id, t, fenceNh);
        native_handle_close(fenceNh);
        native_handle_delete(fenceNh);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
        oBuffer.editItemAt(i).owner = component;
    }

    status = omxNode->sendCommand(toRawCommandType(OMX_CommandFlush),
                                  kPortIndexOutput);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandFlush);
    ASSERT_EQ(msg.data.eventData.data2, kPortIndexOutput);

    // test if client got all its buffers back
    for (size_t i = 0; i < oBuffer.size(); ++i) {
        EXPECT_EQ(oBuffer[i].owner, client);
    }

    // set state to Idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

    // set state to Loaded
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateLoaded);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    // dont change state until all buffers are freed
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    for (size_t i = 0; i < iBuffer.size(); ++i) {
        status = omxNode->freeBuffer(kPortIndexInput, iBuffer[i].id);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    }

    // dont change state until all buffers are freed
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    for (size_t i = 0; i < oBuffer.size(); ++i) {
        status = omxNode->freeBuffer(kPortIndexOutput, oBuffer[i].id);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    }

    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateLoaded);
}

// Test State Transitions
TEST_F(ComponentHidlTest, StateTransitions_M) {
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
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    // allocate buffers on i/o port
    allocatePortBuffers(omxNode, allocator, &iBuffer, kPortIndexInput);
    allocatePortBuffers(omxNode, allocator, &oBuffer, kPortIndexOutput);

    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

    // set state to idle ; receive error OMX_ErrorSameState
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    EXPECT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // set state to executing
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateExecuting);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateExecuting);

    // set state to executing ; receive error OMX_ErrorSameState
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateExecuting);
    EXPECT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // set state to Loaded ; receive error OMX_ErrorIncorrectStateTransition
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateLoaded);
    EXPECT_NE(status, android::hardware::media::omx::V1_0::Status::OK);

    // set state to Idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

    // set state to Loaded
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateLoaded);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    for (size_t i = 0; i < iBuffer.size(); ++i) {
        status = omxNode->freeBuffer(kPortIndexInput, iBuffer[i].id);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    }
    for (size_t i = 0; i < oBuffer.size(); ++i) {
        status = omxNode->freeBuffer(kPortIndexOutput, oBuffer[i].id);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    }

    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateLoaded);
}

// Enable and Disable Input and Output ports when the component is in Loaded
// State
TEST_F(ComponentHidlTest, PortEnableDisable_Loaded) {
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
            ALOGI("Port %d Disabling failed with error %d", (int)i,
                  (int)msg.data.eventData.event);
        } else {
            // something unexpected happened
            ASSERT_TRUE(false);
        }
    }
}

// Enable and Disable Input and Output ports when the component is in Idle State
TEST_F(ComponentHidlTest, PortEnableDisable_Idle) {
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
    // set state to idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    // allocate buffers on input port
    allocatePortBuffers(omxNode, allocator, &pBuffer[0], kPortIndexInput);
    allocatePortBuffers(omxNode, allocator, &pBuffer[1], kPortIndexOutput);

    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &pBuffer[0],
                                      &pBuffer[1]);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

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
                ALOGI("Port %d Disabling failed with error %d", (int)i,
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

            allocatePortBuffers(omxNode, allocator, &pBuffer[i - portBase], i);
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
}

// Enable and Disable Input and Output ports when the component is in execute
TEST_F(ComponentHidlTest, PortEnableDisable_Execute) {
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
    // set state to idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    // allocate buffers on input port
    allocatePortBuffers(omxNode, allocator, &pBuffer[0], kPortIndexInput);
    allocatePortBuffers(omxNode, allocator, &pBuffer[1], kPortIndexOutput);

    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &pBuffer[0],
                                      &pBuffer[1]);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

    // Component State :: Execute
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateExecuting);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &pBuffer[0],
                                      &pBuffer[1]);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateExecuting);

    CodecBuffer t;
    t.sharedMemory = android::hardware::hidl_memory();
    t.nativeHandle = android::hardware::hidl_handle();
    t.type = CodecBuffer::Type::PRESET;
    t.attr.preset.rangeOffset = 0;
    t.attr.preset.rangeLength = 0;
    for (size_t i = 0; i < pBuffer[1].size(); ++i) {
        native_handle_t* fenceNh = native_handle_create(0, 0);
        status = omxNode->fillBuffer(pBuffer[1][i].id, t, fenceNh);
        native_handle_close(fenceNh);
        native_handle_delete(fenceNh);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
        pBuffer[1].editItemAt(i).owner = component;
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
                ALOGI("Port %d Disabling failed with error %d", (int)i,
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

            allocatePortBuffers(omxNode, allocator, &pBuffer[i - portBase], i);
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
}

// Enable and Disable Input and Output ports, (in loaded state)
TEST_F(ComponentHidlTest, PortEnableDisable_M) {
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
            ALOGI("Port %d Disabling failed with error %d", (int)i,
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
            ALOGI("Port %d Enabling failed with error %d", (int)i,
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
