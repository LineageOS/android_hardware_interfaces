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

#define LOG_TAG "media_omx_hidl_video_enc_test"
#ifdef __LP64__
#define OMX_ANDROID_COMPILE_AS_32BIT_ON_64BIT_PLATFORMS
#endif

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
#include <media/hardware/HardwareAPI.h>
#include <media_hidl_test_common.h>
#include <media_video_hidl_test_common.h>
#include <fstream>

// A class for test environment setup
class ComponentTestEnvironment : public ::testing::Environment {
   public:
    virtual void SetUp() {}
    virtual void TearDown() {}

    ComponentTestEnvironment() : instance("default"), res("/sdcard/media/") {}

    void setInstance(const char* _instance) { instance = _instance; }

    void setComponent(const char* _component) { component = _component; }

    void setRole(const char* _role) { role = _role; }

    void setRes(const char* _res) { res = _res; }

    const hidl_string getInstance() const { return instance; }

    const hidl_string getComponent() const { return component; }

    const hidl_string getRole() const { return role; }

    const hidl_string getRes() const { return res; }

    int initFromOptions(int argc, char** argv) {
        static struct option options[] = {
            {"instance", required_argument, 0, 'I'},
            {"component", required_argument, 0, 'C'},
            {"role", required_argument, 0, 'R'},
            {"res", required_argument, 0, 'P'},
            {0, 0, 0, 0}};

        while (true) {
            int index = 0;
            int c = getopt_long(argc, argv, "I:C:R:P:", options, &index);
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
                case 'P':
                    setRes(optarg);
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
                    "-R, --role: OMX component Role\n"
                    "-P, --res: Resource files directory location\n",
                    argv[optind ?: 1], argv[0]);
            return 2;
        }
        return 0;
    }

   private:
    hidl_string instance;
    hidl_string component;
    hidl_string role;
    hidl_string res;
};

static ComponentTestEnvironment* gEnv = nullptr;

// video encoder test fixture class
class VideoEncHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        disableTest = false;
        android::hardware::media::omx::V1_0::Status status;
        omx = ::testing::VtsHalHidlTargetTestBase::getService<IOmx>(
            gEnv->getInstance());
        ASSERT_NE(omx, nullptr);
        observer = new CodecObserver([this](Message msg) { (void)msg; });
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
        struct StringToName {
            const char* Name;
            standardComp CompName;
        };
        const StringToName kStringToName[] = {
            {"h263", h263}, {"avc", avc}, {"mpeg4", mpeg4},
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
        if (compName == unknown_comp) disableTest = true;
        struct CompToCompression {
            standardComp CompName;
            OMX_VIDEO_CODINGTYPE eCompressionFormat;
        };
        static const CompToCompression kCompToCompression[] = {
            {h263, OMX_VIDEO_CodingH263},   {avc, OMX_VIDEO_CodingAVC},
            {mpeg4, OMX_VIDEO_CodingMPEG4}, {hevc, OMX_VIDEO_CodingHEVC},
            {vp8, OMX_VIDEO_CodingVP8},     {vp9, OMX_VIDEO_CodingVP9},
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
        if (i == kNumCompToCompression) disableTest = true;
        if (disableTest) std::cerr << "[          ] Warning !  Test Disabled\n";
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
    bool disableTest;

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }
};

// request VOP refresh
void requestIDR(sp<IOmxNode> omxNode, OMX_U32 portIndex) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_CONFIG_INTRAREFRESHVOPTYPE param;
    param.IntraRefreshVOP = OMX_TRUE;
    status = setPortConfig(omxNode, OMX_IndexConfigVideoIntraVOPRefresh,
                           portIndex, &param);
    if (status != ::android::hardware::media::omx::V1_0::Status::OK)
        std::cerr << "[          ] Warning ! unable to request IDR \n";
}

// modify bitrate
void changeBitrate(sp<IOmxNode> omxNode, OMX_U32 portIndex, uint32_t nBitrate) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_VIDEO_CONFIG_BITRATETYPE param;
    param.nEncodeBitrate = nBitrate;
    status =
        setPortConfig(omxNode, OMX_IndexConfigVideoBitrate, portIndex, &param);
    if (status != ::android::hardware::media::omx::V1_0::Status::OK)
        std::cerr << "[          ] Warning ! unable to change Bitrate \n";
}

// modify framerate
Return<android::hardware::media::omx::V1_0::Status> changeFrameRate(
    sp<IOmxNode> omxNode, OMX_U32 portIndex, uint32_t xFramerate) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_CONFIG_FRAMERATETYPE param;
    param.xEncodeFramerate = xFramerate;
    status = setPortConfig(omxNode, OMX_IndexConfigVideoFramerate, portIndex,
                           &param);
    if (status != ::android::hardware::media::omx::V1_0::Status::OK)
        std::cerr << "[          ] Warning ! unable to change Framerate \n";
    return status;
}

// modify intra refresh interval
void changeRefreshPeriod(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                         uint32_t nRefreshPeriod) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE param;
    param.nRefreshPeriod = nRefreshPeriod;
    status = setPortConfig(omxNode,
                           (OMX_INDEXTYPE)OMX_IndexConfigAndroidIntraRefresh,
                           portIndex, &param);
    if (status != ::android::hardware::media::omx::V1_0::Status::OK)
        std::cerr << "[          ] Warning ! unable to change Refresh Period\n";
}

// set intra refresh interval
void setRefreshPeriod(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                      uint32_t nRefreshPeriod) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_VIDEO_PARAM_INTRAREFRESHTYPE param;
    param.eRefreshMode = OMX_VIDEO_IntraRefreshCyclic;
    param.nCirMBs = 0;
    if (nRefreshPeriod == 0)
        param.nCirMBs = 0;
    else {
        OMX_PARAM_PORTDEFINITIONTYPE portDef;
        status = getPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                              &portDef);
        if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
            param.nCirMBs =
                ((portDef.format.video.nFrameWidth + 15) >>
                 4 * (portDef.format.video.nFrameHeight + 15) >> 4) /
                nRefreshPeriod;
        }
    }
    status = setPortParam(omxNode, OMX_IndexParamVideoIntraRefresh, portIndex,
                          &param);
    if (status != ::android::hardware::media::omx::V1_0::Status::OK)
        std::cerr << "[          ] Warning ! unable to set Refresh Period \n";
}

// Set Default port param.
void setDefaultPortParam(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                         OMX_VIDEO_CODINGTYPE eCompressionFormat,
                         OMX_U32 nBitrate, OMX_U32 xFramerate) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_PARAM_PORTDEFINITIONTYPE portDef;
    status = getPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    portDef.format.video.nBitrate = nBitrate;
    portDef.format.video.xFramerate = xFramerate;
    portDef.format.video.bFlagErrorConcealment = OMX_TRUE;
    portDef.format.video.eCompressionFormat = eCompressionFormat;
    portDef.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    status = setPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    std::vector<int32_t> arrProfile;
    std::vector<int32_t> arrLevel;
    enumerateProfileAndLevel(omxNode, portIndex, &arrProfile, &arrLevel);
    if (arrProfile.empty() == true || arrLevel.empty() == true)
        ASSERT_TRUE(false);
    int32_t profile = arrProfile[0];
    int32_t level = arrLevel[0];

    switch ((int)eCompressionFormat) {
        case OMX_VIDEO_CodingAVC:
            setupAVCPort(omxNode, portIndex,
                         static_cast<OMX_VIDEO_AVCPROFILETYPE>(profile),
                         static_cast<OMX_VIDEO_AVCLEVELTYPE>(level),
                         xFramerate);
            break;
        case OMX_VIDEO_CodingHEVC:
            setupHEVCPort(omxNode, portIndex,
                          static_cast<OMX_VIDEO_HEVCPROFILETYPE>(profile),
                          static_cast<OMX_VIDEO_HEVCLEVELTYPE>(level));
            break;
        case OMX_VIDEO_CodingH263:
            setupH263Port(omxNode, portIndex,
                          static_cast<OMX_VIDEO_H263PROFILETYPE>(profile),
                          static_cast<OMX_VIDEO_H263LEVELTYPE>(level),
                          xFramerate);
            break;
        case OMX_VIDEO_CodingMPEG4:
            setupMPEG4Port(omxNode, portIndex,
                           static_cast<OMX_VIDEO_MPEG4PROFILETYPE>(profile),
                           static_cast<OMX_VIDEO_MPEG4LEVELTYPE>(level),
                           xFramerate);
            break;
        case OMX_VIDEO_CodingVP8:
            setupVPXPort(omxNode, portIndex, xFramerate);
            setupVP8Port(omxNode, portIndex,
                         static_cast<OMX_VIDEO_VP8PROFILETYPE>(profile),
                         static_cast<OMX_VIDEO_VP8LEVELTYPE>(level));
            break;
        case OMX_VIDEO_CodingVP9:
            setupVPXPort(omxNode, portIndex, xFramerate);
            setupVP9Port(omxNode, portIndex,
                         static_cast<OMX_VIDEO_VP9PROFILETYPE>(profile),
                         static_cast<OMX_VIDEO_VP9LEVELTYPE>(level));
            break;
        default:
            break;
    }
}

// LookUpTable of clips and metadata for component testing
void GetURLForComponent(char* URL) {
    strcat(URL, "bbb_352x288_420p_30fps_32frames.yuv");
}

// Encode N Frames
void encodeNFrames(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                   OMX_U32 portIndexOutput,
                   android::Vector<BufferInfo>* iBuffer,
                   android::Vector<BufferInfo>* oBuffer, uint32_t nFrames,
                   uint32_t xFramerate, int bytesCount,
                   std::ifstream& eleStream) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;
    uint32_t ipCount = 0;

    if (ipCount == 0) {
        status = changeFrameRate(omxNode, portIndexOutput, (24U << 16));
        if (status == ::android::hardware::media::omx::V1_0::Status::OK)
            xFramerate = (24U << 16);
    }

    // dispatch output buffers
    for (size_t i = 0; i < oBuffer->size(); i++) {
        dispatchOutputBuffer(omxNode, oBuffer, i);
    }
    // dispatch input buffers
    int32_t timestampIncr = (int)((float)1000000 / (xFramerate >> 16));
    uint64_t timestamp = 0;
    for (size_t i = 0; i < iBuffer->size() && nFrames != 0; i++) {
        char* ipBuffer = static_cast<char*>(
            static_cast<void*>((*iBuffer)[i].mMemory->getPointer()));
        ASSERT_LE(bytesCount,
                  static_cast<int>((*iBuffer)[i].mMemory->getSize()));
        eleStream.read(ipBuffer, bytesCount);
        if (eleStream.gcount() != bytesCount) break;
        dispatchInputBuffer(omxNode, iBuffer, i, bytesCount, 0, timestamp);
        timestamp += timestampIncr;
        nFrames--;
        ipCount++;
    }

    while (1) {
        status =
            observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);

        if (status == android::hardware::media::omx::V1_0::Status::OK) {
            ASSERT_EQ(msg.type, Message::Type::EVENT);
            if (msg.data.eventData.event == OMX_EventPortSettingsChanged) {
                ASSERT_EQ(msg.data.eventData.data1, portIndexOutput);
                ASSERT_EQ(msg.data.eventData.data2,
                          OMX_IndexConfigAndroidIntraRefresh);
            } else {
                ASSERT_TRUE(false);
            }
        }

        if (nFrames == 0) break;

        // Dispatch input buffer
        size_t index = 0;
        if ((index = getEmptyBufferID(iBuffer)) < iBuffer->size()) {
            char* ipBuffer = static_cast<char*>(
                static_cast<void*>((*iBuffer)[index].mMemory->getPointer()));
            ASSERT_LE(bytesCount,
                      static_cast<int>((*iBuffer)[index].mMemory->getSize()));
            eleStream.read(ipBuffer, bytesCount);
            if (eleStream.gcount() != bytesCount) break;
            dispatchInputBuffer(omxNode, iBuffer, index, bytesCount, 0,
                                timestamp);
            timestamp += timestampIncr;
            nFrames--;
            ipCount++;
        }
        if ((index = getEmptyBufferID(oBuffer)) < oBuffer->size()) {
            dispatchOutputBuffer(omxNode, oBuffer, index);
        }
        if (ipCount == 15) {
            changeBitrate(omxNode, portIndexOutput, 768000);
            requestIDR(omxNode, portIndexOutput);
            changeRefreshPeriod(omxNode, portIndexOutput, 15);
        }
    }
}

// set component role
TEST_F(VideoEncHidlTest, SetRole) {
    description("Test Set Component Role");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

// port format enumeration
TEST_F(VideoEncHidlTest, EnumeratePortFormat) {
    description("Test Component on Mandatory Port Parameters (Port Format)");
    if (disableTest) return;
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    OMX_COLOR_FORMATTYPE eColorFormat = OMX_COLOR_FormatYUV420Planar;
    OMX_U32 xFramerate = (30U << 16);
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    status = getParam(omxNode, OMX_IndexParamVideoInit, &params);
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }
    status =
        setVideoPortFormat(omxNode, kPortIndexInput, OMX_VIDEO_CodingUnused,
                           eColorFormat, xFramerate);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    status = setVideoPortFormat(omxNode, kPortIndexOutput, eCompressionFormat,
                                OMX_COLOR_FormatUnused, 0U);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

// test raw stream encode
TEST_F(VideoEncHidlTest, EncodeTest) {
    description("Test Encode");
    if (disableTest) return;
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
    char mURL[512];
    strcpy(mURL, gEnv->getRes().c_str());
    GetURLForComponent(mURL);

    std::ifstream eleStream;
    eleStream.open(mURL, std::ifstream::binary);
    ASSERT_EQ(eleStream.is_open(), true);

    // Configure input port
    uint32_t nFrameWidth = 352;
    uint32_t nFrameHeight = 288;
    uint32_t xFramerate = (30U << 16);
    OMX_COLOR_FORMATTYPE eColorFormat = OMX_COLOR_FormatYUV420Planar;
    setupRAWPort(omxNode, kPortIndexInput, nFrameWidth, nFrameHeight, 0,
                 xFramerate, eColorFormat);
    // Configure output port
    uint32_t nBitRate = 512000;
    setDefaultPortParam(omxNode, kPortIndexOutput, eCompressionFormat, nBitRate,
                        xFramerate);
    setRefreshPeriod(omxNode, kPortIndexOutput, 0);
    unsigned int index;
    omxNode->getExtensionIndex(
        "OMX.google.android.index.prependSPSPPSToIDRFrames",
        [&status, &index](android::hardware::media::omx::V1_0::Status _s,
                          unsigned int _nl) {
            status = _s;
            index = _nl;
        });
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        android::PrependSPSPPSToIDRFramesParams param;
        param.bEnable = OMX_TRUE;
        status = setParam(omxNode, static_cast<OMX_INDEXTYPE>(index), &param);
    }
    if (status != ::android::hardware::media::omx::V1_0::Status::OK)
        std::cerr
            << "[          ] Warning ! unable to prependSPSPPSToIDRFrames\n";

    android::Vector<BufferInfo> iBuffer, oBuffer;

    // set state to idle
    changeStateLoadedtoIdle(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput);
    // set state to executing
    changeStateIdletoExecute(omxNode, observer);

    encodeNFrames(omxNode, observer, kPortIndexOutput, &iBuffer, &oBuffer, 1024,
                  xFramerate, (nFrameWidth * nFrameHeight * 3) >> 1, eleStream);
    // set state to idle
    changeStateExecutetoIdle(omxNode, observer, &iBuffer, &oBuffer);
    // set state to executing
    changeStateIdletoLoaded(omxNode, observer, &iBuffer, &oBuffer,
                            kPortIndexInput, kPortIndexOutput);

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
