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

#define LOG_TAG "media_omx_hidl_audio_dec_test"
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
#include <media_audio_hidl_test_common.h>
#include <media_hidl_test_common.h>
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

class AudioDecHidlTest : public ::testing::VtsHalHidlTargetTestBase {
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
            {"mp3", mp3}, {"amrnb", amrnb},   {"amrwb", amrwb},
            {"aac", aac}, {"vorbis", vorbis}, {"opus", opus},
            {"pcm", pcm}, {"flac", flac},
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
        struct CompToCoding {
            standardComp CompName;
            OMX_AUDIO_CODINGTYPE eEncoding;
        };
        static const CompToCoding kCompToCoding[] = {
            {mp3, OMX_AUDIO_CodingMP3},
            {amrnb, OMX_AUDIO_CodingAMR},
            {amrwb, OMX_AUDIO_CodingAMR},
            {aac, OMX_AUDIO_CodingAAC},
            {vorbis, OMX_AUDIO_CodingVORBIS},
            {pcm, OMX_AUDIO_CodingPCM},
            {opus, (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingAndroidOPUS},
            {flac, OMX_AUDIO_CodingFLAC},
        };
        static const size_t kNumCompToCoding =
            sizeof(kCompToCoding) / sizeof(kCompToCoding[0]);
        size_t i;
        for (i = 0; i < kNumCompToCoding; ++i) {
            if (kCompToCoding[i].CompName == compName) {
                eEncoding = kCompToCoding[i].eEncoding;
                break;
            }
        }
        ASSERT_NE(i, kNumCompToCoding);
    }

    virtual void TearDown() override {
        if (omxNode != nullptr) {
            EXPECT_TRUE((omxNode->freeNode()).isOk());
            omxNode = nullptr;
        }
    }

    enum standardComp {
        mp3,
        amrnb,
        amrwb,
        aac,
        vorbis,
        opus,
        pcm,
        flac,
        unknown_comp,
    };

    sp<IOmx> omx;
    sp<CodecObserver> observer;
    sp<IOmxNode> omxNode;
    standardComp compName;
    OMX_AUDIO_CODINGTYPE eEncoding;
};

void setDefaultPortParam(
    sp<IOmxNode> omxNode, OMX_U32 portIndex, OMX_AUDIO_CODINGTYPE eEncoding,
    int32_t nChannels = 2, int32_t nSampleRate = 44100,
    OMX_NUMERICALDATATYPE eNumData = OMX_NumericalDataSigned,
    int32_t nBitPerSample = 16) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_PARAM_PORTDEFINITIONTYPE portDef;
    status = getPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    portDef.format.audio.bFlagErrorConcealment = OMX_TRUE;
    portDef.format.audio.eEncoding = eEncoding;
    status = setPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    switch ((int)eEncoding) {
        case OMX_AUDIO_CodingPCM:
            setupPCMPort(omxNode, portIndex, nChannels, eNumData, nBitPerSample,
                         nSampleRate);
            break;
        default:
            ASSERT_TRUE(false);
            break;
    }
}

void getInputChannelInfo(sp<IOmxNode> omxNode, OMX_U32 kPortIndexInput,
                         OMX_AUDIO_CODINGTYPE eEncoding, int32_t* nChannels,
                         int32_t* nSampleRate) {
    *nChannels = 0;
    *nSampleRate = 0;
    android::hardware::media::omx::V1_0::Status status;

    switch ((int)eEncoding) {
        case OMX_AUDIO_CodingPCM: {
            OMX_AUDIO_PARAM_PCMMODETYPE param;
            status = getPortParam(omxNode, OMX_IndexParamAudioPcm,
                                  kPortIndexInput, &param);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
            *nChannels = param.nChannels;
            *nSampleRate = param.nSamplingRate;
            break;
        }
        case OMX_AUDIO_CodingMP3: {
            OMX_AUDIO_PARAM_MP3TYPE param;
            status = getPortParam(omxNode, OMX_IndexParamAudioMp3,
                                  kPortIndexInput, &param);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
            *nChannels = param.nChannels;
            *nSampleRate = param.nSampleRate;
            break;
        }
        case OMX_AUDIO_CodingFLAC: {
            OMX_AUDIO_PARAM_FLACTYPE param;
            status = getPortParam(omxNode, OMX_IndexParamAudioFlac,
                                  kPortIndexInput, &param);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
            *nChannels = param.nChannels;
            *nSampleRate = param.nSampleRate;
            break;
        }
        case OMX_AUDIO_CodingAndroidOPUS: {
            OMX_AUDIO_PARAM_ANDROID_OPUSTYPE param;
            status = getPortParam(omxNode,
                                  (OMX_INDEXTYPE)OMX_IndexParamAudioAndroidOpus,
                                  kPortIndexInput, &param);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
            *nChannels = param.nChannels;
            *nSampleRate = param.nSampleRate;
            break;
        }
        case OMX_AUDIO_CodingVORBIS: {
            OMX_AUDIO_PARAM_VORBISTYPE param;
            status = getPortParam(omxNode, OMX_IndexParamAudioVorbis,
                                  kPortIndexInput, &param);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
            *nChannels = param.nChannels;
            *nSampleRate = param.nSampleRate;
            break;
        }
        case OMX_AUDIO_CodingAMR: {
            OMX_AUDIO_PARAM_AMRTYPE param;
            status = getPortParam(omxNode, OMX_IndexParamAudioAmr,
                                  kPortIndexInput, &param);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
            *nChannels = param.nChannels;
            *nSampleRate = 8000;
            break;
        }
        case OMX_AUDIO_CodingAAC: {
            OMX_AUDIO_PARAM_AACPROFILETYPE param;
            status = getPortParam(omxNode, OMX_IndexParamAudioAac,
                                  kPortIndexInput, &param);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
            *nChannels = param.nChannels;
            *nSampleRate = param.nSampleRate;
            break;
        }
        default:
            ASSERT_TRUE(false);
            break;
    }
}

void GetURLForComponent(AudioDecHidlTest::standardComp comp, const char** mURL,
                        const char** info) {
    struct CompToURL {
        AudioDecHidlTest::standardComp comp;
        const char* mURL;
        const char* info;
    };
    static const CompToURL kCompToURL[] = {
        {AudioDecHidlTest::standardComp::mp3,
         "/sdcard/raw/MP3_48KHz_128kbps_s_1_17_CBR.audio.mp3",
         "/sdcard/raw/MP3_48KHz_128kbps_s_1_17_CBR.audio.info"},
        {AudioDecHidlTest::standardComp::aac,
         "/sdcard/raw/H264_500_AAC_128.audio.aac",
         "/sdcard/raw/H264_500_AAC_128.audio.info"},
        {AudioDecHidlTest::standardComp::amrnb,
         "/sdcard/raw/H264_320_AMRNB_6.audio.amr",
         "/sdcard/raw/H264_320_AMRNB_6.audio.info"},
        {AudioDecHidlTest::standardComp::amrwb, "", ""},
        {AudioDecHidlTest::standardComp::vorbis, "", ""},
        {AudioDecHidlTest::standardComp::opus, "", ""},
        {AudioDecHidlTest::standardComp::flac, "", ""},
    };

    for (size_t i = 0; i < sizeof(kCompToURL) / sizeof(kCompToURL[0]); ++i) {
        if (kCompToURL[i].comp == comp) {
            *mURL = kCompToURL[i].mURL;
            *info = kCompToURL[i].info;
            return;
        }
    }
}

void flushAllPorts(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                   android::Vector<BufferInfo>* iBuffer,
                   android::Vector<BufferInfo>* oBuffer,
                   OMX_U32 kPortIndexInput, OMX_U32 kPortIndexOutput) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;
    // Flush
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandFlush),
                                  kPortIndexInput);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandFlush);
    ASSERT_EQ(msg.data.eventData.data2, kPortIndexInput);
    // test if client got all its buffers back
    for (size_t i = 0; i < iBuffer->size(); ++i) {
        EXPECT_EQ((*iBuffer)[i].owner, client);
    }

    status = omxNode->sendCommand(toRawCommandType(OMX_CommandFlush),
                                  kPortIndexOutput);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandFlush);
    ASSERT_EQ(msg.data.eventData.data2, kPortIndexOutput);
    // test if client got all its buffers back
    for (size_t i = 0; i < oBuffer->size(); ++i) {
        EXPECT_EQ((*oBuffer)[i].owner, client);
    }
}

void decodeNFrames(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                   android::Vector<BufferInfo>* iBuffer,
                   android::Vector<BufferInfo>* oBuffer,
                   OMX_AUDIO_CODINGTYPE eEncoding, OMX_U32 kPortIndexInput,
                   OMX_U32 kPortIndexOutput, uint32_t nFrames,
                   std::ifstream& eleStream, std::ifstream& eleInfo) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;

    // dispatch output buffers
    for (size_t i = 0; i < oBuffer->size(); i++) {
        dispatchOutputBuffer(omxNode, oBuffer, i);
    }
    // dispatch input buffers
    int bytesCount = 0;
    for (size_t i = 0; i < iBuffer->size(); i++) {
        char* ipBuffer = static_cast<char*>(
            static_cast<void*>((*iBuffer)[i].mMemory->getPointer()));
        if (!(eleInfo >> bytesCount)) break;
        eleStream.read(ipBuffer, bytesCount);
        ASSERT_EQ(eleStream.gcount(), bytesCount);
        dispatchInputBuffer(omxNode, iBuffer, i, bytesCount, 0, 0);
    }

    while (nFrames != 0) {
        status =
            observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
        if (status == android::hardware::media::omx::V1_0::Status::OK &&
            msg.type == Message::Type::EVENT &&
            msg.data.eventData.event == OMX_EventPortSettingsChanged) {
            ASSERT_EQ(msg.data.eventData.data1, kPortIndexOutput);

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

                // Port Reconfigurations
                int32_t nChannels;
                int32_t nSampleRate;
                getInputChannelInfo(omxNode, kPortIndexInput, eEncoding,
                                    &nChannels, &nSampleRate);
                setDefaultPortParam(omxNode, kPortIndexOutput,
                                    OMX_AUDIO_CodingPCM, nChannels,
                                    nSampleRate);

                // If you can disable a port, then you should be able to enable
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
            continue;
        }
        size_t index = 0;
        if ((index = getEmptyBufferID(iBuffer)) < iBuffer->size()) {
            char* ipBuffer = static_cast<char*>(
                static_cast<void*>((*iBuffer)[index].mMemory->getPointer()));
            if (!(eleInfo >> bytesCount)) break;
            eleStream.read(ipBuffer, bytesCount);
            ASSERT_EQ(eleStream.gcount(), bytesCount);
            dispatchInputBuffer(omxNode, iBuffer, index, bytesCount, 0, 0);
        }
        if ((index = getEmptyBufferID(oBuffer)) < oBuffer->size()) {
            dispatchOutputBuffer(omxNode, oBuffer, index);
        }
        nFrames--;
    }
}

// Set Component Role
TEST_F(AudioDecHidlTest, SetRole) {
    android::hardware::media::omx::V1_0::Status status;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

// Enumerate Port Format
TEST_F(AudioDecHidlTest, EnumeratePortFormat) {
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
    if (status == ::android::hardware::media::omx::V1_0::Status::OK) {
        ASSERT_EQ(params.nPorts, 2U);
        kPortIndexInput = params.nStartPortNumber;
        kPortIndexOutput = kPortIndexInput + 1;
    }
    status = setAudioPortFormat(omxNode, kPortIndexInput, eEncoding);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    status = setAudioPortFormat(omxNode, kPortIndexOutput, OMX_AUDIO_CodingPCM);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

// Decode Test
TEST_F(AudioDecHidlTest, DecodeTest) {
    android::hardware::media::omx::V1_0::Status status;
    uint32_t kPortIndexInput = 0, kPortIndexOutput = 1;
    status = setRole(omxNode, gEnv->getRole().c_str());
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    OMX_PORT_PARAM_TYPE params;
    status = getParam(omxNode, OMX_IndexParamAudioInit, &params);
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

    if (eEncoding == OMX_AUDIO_CodingPCM)
        setDefaultPortParam(omxNode, kPortIndexInput, eEncoding);
    int32_t nChannels;
    int32_t nSampleRate;
    getInputChannelInfo(omxNode, kPortIndexInput, eEncoding, &nChannels,
                        &nSampleRate);
    setDefaultPortParam(omxNode, kPortIndexOutput, OMX_AUDIO_CodingPCM,
                        nChannels, nSampleRate);

    Message msg;
    android::Vector<BufferInfo> iBuffer, oBuffer;

    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    allocatePortBuffers(omxNode, &iBuffer, kPortIndexInput);
    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);

    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);
    allocatePortBuffers(omxNode, &oBuffer, kPortIndexOutput);

    status =
        observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, &iBuffer, &oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

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

    // Port Reconfiguration
    decodeNFrames(omxNode, observer, &iBuffer, &oBuffer, eEncoding,
                  kPortIndexInput, kPortIndexOutput, (1 << 12), eleStream,
                  eleInfo);

    // flush
    flushAllPorts(omxNode, observer, &iBuffer, &oBuffer, kPortIndexInput,
                  kPortIndexOutput);

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
