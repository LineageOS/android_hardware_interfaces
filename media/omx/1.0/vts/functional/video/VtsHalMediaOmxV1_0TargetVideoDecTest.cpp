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

#define LOG_TAG "media_omx_hidl_video_dec_test"
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
#include <media_video_hidl_test_common.h>
#include <fstream>

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

class VideoDecHidlTest : public ::testing::VtsHalHidlTargetTestBase {
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
        struct StringToName {
            const char* Name;
            standardComp CompName;
        };
        const StringToName kStringToName[] = {
            {"h263", h263}, {"avc", avc}, {"mpeg2", mpeg2}, {"mpeg4", mpeg4},
            {"hevc", hevc}, {"vp8", vp8}, {"vp9", vp9},
        };
        const size_t kNumStringToName =
            sizeof(kStringToName) / sizeof(kStringToName[0]);
        const char* pch;
        char substring[OMX_MAX_STRINGNAME_SIZE];
        strcpy(substring, gEnv->getRole().c_str());
        pch = strchr(substring, '.');
        ASSERT_NE(pch, nullptr);
        compName = unknown_comp;
        for (size_t i = 0; i < kNumStringToName; ++i) {
            if (!strcasecmp(pch + 1, kStringToName[i].Name)) {
                compName = kStringToName[i].CompName;
                break;
            }
        }
        ASSERT_NE(compName, unknown_comp);
        struct CompToCompression {
            standardComp CompName;
            OMX_VIDEO_CODINGTYPE eCompressionFormat;
        };
        static const CompToCompression kCompToCompression[] = {
            {h263, OMX_VIDEO_CodingH263},   {avc, OMX_VIDEO_CodingAVC},
            {mpeg2, OMX_VIDEO_CodingMPEG2}, {mpeg4, OMX_VIDEO_CodingMPEG4},
            {hevc, OMX_VIDEO_CodingHEVC},   {vp8, OMX_VIDEO_CodingVP8},
            {vp9, OMX_VIDEO_CodingVP9},
        };
        static const size_t kNumCompToCompression =
            sizeof(kCompToCompression) / sizeof(kCompToCompression[0]);
        size_t i;
        for (i = 0; i < kNumCompToCompression; ++i) {
            if (kCompToCompression[i].CompName == compName) {
                eCompressionFormat = kCompToCompression[i].eCompressionFormat;
                break;
            }
        }
        ASSERT_NE(i, kNumCompToCompression);
    }

    virtual void TearDown() override {
        if (omxNode != nullptr) {
            EXPECT_TRUE((omxNode->freeNode()).isOk());
            omxNode = nullptr;
        }
    }

    enum standardComp {
        h263,
        avc,
        mpeg2,
        mpeg4,
        hevc,
        vp8,
        vp9,
        unknown_comp,
    };

    sp<IOmx> omx;
    sp<CodecObserver> observer;
    sp<IOmxNode> omxNode;
    standardComp compName;
    OMX_VIDEO_CODINGTYPE eCompressionFormat;

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }
};

// Set Default port param.
void setDefaultPortParam(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                         OMX_VIDEO_CODINGTYPE eCompressionFormat,
                         OMX_COLOR_FORMATTYPE eColorFormat,
                         OMX_U32 nFrameWidth = 352, OMX_U32 nFrameHeight = 288,
                         OMX_U32 nBitrate = 0,
                         OMX_U32 xFramerate = (24U << 16)) {
    switch ((int)eCompressionFormat) {
        case OMX_VIDEO_CodingUnused:
            setupRAWPort(omxNode, portIndex, nFrameWidth, nFrameHeight,
                         nBitrate, xFramerate, eColorFormat);
            break;
        default:
            break;
    }
}

// In decoder components, often the input port parameters get updated upon
// parsing the header of elementary stream. Client needs to collect this
// information to reconfigure other ports that share data with this input
// port.
void getInputChannelInfo(sp<IOmxNode> omxNode, OMX_U32 kPortIndexInput,
                         uint32_t* nFrameWidth, uint32_t* nFrameHeight,
                         uint32_t* xFramerate) {
    android::hardware::media::omx::V1_0::Status status;
    *nFrameWidth = 352;
    *nFrameHeight = 288;
    *xFramerate = (24U << 16);

    OMX_PARAM_PORTDEFINITIONTYPE portDef;
    status = getPortParam(omxNode, OMX_IndexParamPortDefinition,
                          kPortIndexInput, &portDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        *nFrameWidth = portDef.format.video.nFrameWidth;
        *nFrameHeight = portDef.format.video.nFrameHeight;
        *xFramerate = portDef.format.video.xFramerate;
    }
}

// LookUpTable of clips and metadata for component testing
void GetURLForComponent(VideoDecHidlTest::standardComp comp, const char** mURL,
                        const char** info) {
    struct CompToURL {
        VideoDecHidlTest::standardComp comp;
        const char* mURL;
        const char* info;
    };
    static const CompToURL kCompToURL[] = {
        {VideoDecHidlTest::standardComp::avc,
         "/sdcard/media/bbb_avc_1920x1080_5000kbps_30fps.h264",
         "/sdcard/media/bbb_avc_1920x1080_5000kbps_30fps.info"},
    };

    *mURL = *info = nullptr;
    for (size_t i = 0; i < sizeof(kCompToURL) / sizeof(kCompToURL[0]); ++i) {
        if (kCompToURL[i].comp == comp) {
            *mURL = kCompToURL[i].mURL;
            *info = kCompToURL[i].info;
            return;
        }
    }
}

void portReconfiguration(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                         android::Vector<BufferInfo>* iBuffer,
                         android::Vector<BufferInfo>* oBuffer,
                         OMX_U32 kPortIndexInput, OMX_U32 kPortIndexOutput,
                         Message msg) {
    android::hardware::media::omx::V1_0::Status status;

    if (msg.data.eventData.event == OMX_EventPortSettingsChanged) {
        ASSERT_EQ(msg.data.eventData.data1, kPortIndexOutput);
        if (msg.data.eventData.data2 == 0) {
            status = omxNode->sendCommand(
                toRawCommandType(OMX_CommandPortDisable), kPortIndexOutput);
            ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

            status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer,
                                              oBuffer);
            if (status ==
                android::hardware::media::omx::V1_0::Status::TIMED_OUT) {
                for (size_t i = 0; i < oBuffer->size(); ++i) {
                    // test if client got all its buffers back
                    EXPECT_EQ((*oBuffer)[i].owner, client);
                    // free the buffers
                    status =
                        omxNode->freeBuffer(kPortIndexOutput, (*oBuffer)[i].id);
                    ASSERT_EQ(status,
                              android::hardware::media::omx::V1_0::Status::OK);
                }
                status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                                  iBuffer, oBuffer);
                ASSERT_EQ(status,
                          android::hardware::media::omx::V1_0::Status::OK);
                ASSERT_EQ(msg.type, Message::Type::EVENT);
                ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
                ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortDisable);
                ASSERT_EQ(msg.data.eventData.data2, kPortIndexOutput);

                // set Port Params
                uint32_t nFrameWidth, nFrameHeight, xFramerate;
                OMX_COLOR_FORMATTYPE eColorFormat =
                    OMX_COLOR_FormatYUV420Planar;
                getInputChannelInfo(omxNode, kPortIndexInput, &nFrameWidth,
                                    &nFrameHeight, &xFramerate);
                setDefaultPortParam(omxNode, kPortIndexOutput,
                                    OMX_VIDEO_CodingUnused, eColorFormat,
                                    nFrameWidth, nFrameHeight, 0, xFramerate);

                // If you can disable a port, then you should be able to
                // enable
                // it as well
                status = omxNode->sendCommand(
                    toRawCommandType(OMX_CommandPortEnable), kPortIndexOutput);
                ASSERT_EQ(status,
                          android::hardware::media::omx::V1_0::Status::OK);

                // do not enable the port until all the buffers are supplied
                status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                                  iBuffer, oBuffer);
                ASSERT_EQ(
                    status,
                    android::hardware::media::omx::V1_0::Status::TIMED_OUT);

                allocatePortBuffers(omxNode, oBuffer, kPortIndexOutput);
                status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT,
                                                  iBuffer, oBuffer);
                ASSERT_EQ(status,
                          android::hardware::media::omx::V1_0::Status::OK);
                ASSERT_EQ(msg.type, Message::Type::EVENT);
                ASSERT_EQ(msg.data.eventData.data1, OMX_CommandPortEnable);
                ASSERT_EQ(msg.data.eventData.data2, kPortIndexOutput);

                // dispatch output buffers
                for (size_t i = 0; i < oBuffer->size(); i++) {
                    dispatchOutputBuffer(omxNode, oBuffer, i);
                }
            } else {
                ASSERT_TRUE(false);
            }
        } else if (msg.data.eventData.data2 ==
                   OMX_IndexConfigCommonOutputCrop) {
            std::cerr << "[          ] Warning ! OMX_EventPortSettingsChanged/ "
                         "OMX_IndexConfigCommonOutputCrop not handled \n";
        } else if (msg.data.eventData.data2 == OMX_IndexVendorStartUnused + 3) {
            std::cerr << "[          ] Warning ! OMX_EventPortSettingsChanged/ "
                         "kDescribeColorAspectsIndex not handled \n";
        }
    } else if (msg.data.eventData.event == OMX_EventError) {
        std::cerr << "[          ] Warning ! OMX_EventError/ "
                     "Decode Frame Call might be failed \n";
        return;
    } else {
        // something unexpected happened
        ASSERT_TRUE(false);
    }
}

// Decode N Frames
void decodeNFrames(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                   android::Vector<BufferInfo>* iBuffer,
                   android::Vector<BufferInfo>* oBuffer,
                   OMX_U32 kPortIndexInput, OMX_U32 kPortIndexOutput,
                   uint32_t nFrames, std::ifstream& eleStream,
                   std::ifstream& eleInfo) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;

    // dispatch output buffers
    for (size_t i = 0; i < oBuffer->size(); i++) {
        dispatchOutputBuffer(omxNode, oBuffer, i);
    }
    // dispatch input buffers
    int bytesCount = 0;
    uint32_t sampleflags = 0, flags = 0;
    uint64_t timestamp = 0;
    for (size_t i = 0; i < iBuffer->size() && nFrames != 0; i++) {
        char* ipBuffer = static_cast<char*>(
            static_cast<void*>((*iBuffer)[i].mMemory->getPointer()));
        if (!(eleInfo >> bytesCount)) break;
        ASSERT_LE(bytesCount,
                  static_cast<int>((*iBuffer)[i].mMemory->getSize()));
        eleStream.read(ipBuffer, bytesCount);
        ASSERT_EQ(eleStream.gcount(), bytesCount);
        eleInfo >> sampleflags;
        eleInfo >> timestamp;
        flags = 0;
        // Map MediaExtractor flags to OMX Buffer flags
        if (sampleflags & 1) flags |= OMX_BUFFERFLAG_SYNCFRAME;
        dispatchInputBuffer(omxNode, iBuffer, i, bytesCount, flags, timestamp);
        nFrames--;
    }

    while (1) {
        status =
            observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);

        // Port Reconfiguration
        if (status == android::hardware::media::omx::V1_0::Status::OK &&
            msg.type == Message::Type::EVENT) {
            portReconfiguration(omxNode, observer, iBuffer, oBuffer,
                                kPortIndexInput, kPortIndexOutput, msg);
        }

        if (nFrames == 0) break;

        // Dispatch input buffer
        size_t index = 0;
        if ((index = getEmptyBufferID(iBuffer)) < iBuffer->size()) {
            char* ipBuffer = static_cast<char*>(
                static_cast<void*>((*iBuffer)[index].mMemory->getPointer()));
            if (!(eleInfo >> bytesCount)) break;
            ASSERT_LE(bytesCount,
                      static_cast<int>((*iBuffer)[index].mMemory->getSize()));
            eleStream.read(ipBuffer, bytesCount);
            ASSERT_EQ(eleStream.gcount(), bytesCount);
            eleInfo >> sampleflags;
            eleInfo >> timestamp;
            flags = 0;
            // Map MediaExtractor flags to OMX Buffer flags
            if (sampleflags & 1) flags |= OMX_BUFFERFLAG_SYNCFRAME;
            dispatchInputBuffer(omxNode, iBuffer, index, bytesCount, flags,
                                timestamp);
            nFrames--;
        }
        if ((index = getEmptyBufferID(oBuffer)) < oBuffer->size()) {
            dispatchOutputBuffer(omxNode, oBuffer, index);
        }
    }
}

TEST_F(VideoDecHidlTest, SetRole) {
    description("Test Set Component Role");
    android::hardware::media::omx::V1_0::Status status;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

TEST_F(VideoDecHidlTest, EnumeratePortFormat) {
    description("Test Component on Mandatory Port Parameters (Port Format)");
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    OMX_COLOR_FORMATTYPE eColorFormat = OMX_COLOR_FormatYUV420Planar;
    OMX_U32 xFramerate = (24U << 16);
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }
    status = setVideoPortFormat(omxNode, kPortIndexInput, eCompressionFormat,
                                OMX_COLOR_FormatUnused, 0U);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status =
        setVideoPortFormat(omxNode, kPortIndexOutput, OMX_VIDEO_CodingUnused,
                           eColorFormat, xFramerate);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

TEST_F(VideoDecHidlTest, DecodeTest) {
    description("Tests Port Reconfiguration and Decode");
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }
    const char *mURL = nullptr, *info = nullptr;
    GetURLForComponent(compName, &mURL, &info);
    EXPECT_NE(mURL, nullptr);
    EXPECT_NE(info, nullptr);

    std::ifstream eleStream, eleInfo;
    eleStream.open(mURL, std::ifstream::binary);
    ASSERT_EQ(eleStream.is_open(), true);
    eleInfo.open(info);
    ASSERT_EQ(eleInfo.is_open(), true);

    // set Port Params
    uint32_t nFrameWidth, nFrameHeight, xFramerate;
    OMX_COLOR_FORMATTYPE eColorFormat = OMX_COLOR_FormatYUV420Planar;
    getInputChannelInfo(omxNode, kPortIndexInput, &nFrameWidth, &nFrameHeight,
                        &xFramerate);
    setDefaultPortParam(omxNode, kPortIndexOutput, OMX_VIDEO_CodingUnused,
                        eColorFormat, nFrameWidth, nFrameHeight, 0, xFramerate);

    android::Vector<BufferInfo> iBuffer, oBuffer;

    // set state to idle
    changeStateLoadedtoIdle(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput);
    // set state to executing
    changeStateIdletoExecute(omxNode, observer);
    // Port Reconfiguration
    decodeNFrames(omxNode, observer, &iBuffer, &oBuffer, kPortIndexInput,
                  kPortIndexOutput, 1024, eleStream, eleInfo);
    // set state to idle
    changeStateExecutetoIdle(omxNode, observer, &iBuffer, &oBuffer);
    // set state to executing
    changeStateIdletoLoaded(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput);

    eleInfo.close();
    eleStream.close();
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
