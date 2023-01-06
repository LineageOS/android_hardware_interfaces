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

#include <aidl/android/hardware/secure_element/BnSecureElement.h>
#include <android-base/hex.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <algorithm>

using aidl::android::hardware::secure_element::BnSecureElement;
using aidl::android::hardware::secure_element::ISecureElementCallback;
using aidl::android::hardware::secure_element::LogicalChannelResponse;
using android::base::HexString;
using ndk::ScopedAStatus;

static const std::vector<uint8_t> kIssuerSecurityDomainSelectResponse = {0x00, 0x00, 0x90, 0x00};

namespace se {
// Application identifier.
using Aid = std::vector<uint8_t>;

// ISO7816 APDU status codes.
enum Status : uint16_t {
    SW_WRONG_DATA = 0x6A80,
    SW_LOGICAL_CHANNEL_NOT_SUPPORTED = 0x6881,
    SW_CONDITIONS_NOT_SATISFIED = 0x6985,
    SW_INCORRECT_P1P2 = 0x6A86,
    SW_BYTES_REMAINING_00 = 0x6100,
    SW_WRONG_LENGTH = 0x6700,
    SW_CORRECT_LENGTH_00 = 0x6C00,
    SW_INS_NOT_SUPPORTED = 0x6D00,
    SW_NO_ERROR = 0x9000,
};

// Type for raw APDUs.
using RawApdu = std::vector<uint8_t>;

// Wrap a command APDU (Application Processing Data Unit) to provide
// accessors for header fields.
struct Apdu {
  public:
    // Construct a command Apdu.
    Apdu(std::vector<uint8_t> packet) : bytes_(std::move(packet)) {
        CHECK(bytes_.size() >= kHeaderSize) << "command APDU created with invalid length";
        size_t payload_len = bytes_.size() - kHeaderSize;

        // TODO(b/123254068) - add support for extended command APDUs.
        // Pre compute Lc and Le.

        // Case 1: CLA | INS | P1 | P2
        if (payload_len == 0) {
            lc_ = 0;
            le_ = 0;
            return;
        }

        // Case 2: CLA | INS | P1 | P2 | Le
        // Le has a value of 1 to 255.
        if (payload_len == 1) {
            le_ = bytes_[kHeaderSize];
            le_ = le_ == 0 ? 256 : le_;
            lc_ = 0;
            return;
        }

        // Case 3: CLA | INS | P1 | P2 | Lc | Data
        // Lc is less than 256 bytes
        // of data, and Le is zero.
        lc_ = bytes_[kHeaderSize];
        if (payload_len <= (1 + lc_)) {
            le_ = 0;
        }

        // Case 4: CLA | INS | P1 | P2 | Lc | Data | Le
        // The legacy Case 4. Lc and Le
        // are less than 256 bytes of data.
        else {
            le_ = bytes_[bytes_.size() - 1];
            le_ = le_ == 0 ? 256 : le_;
        }
    }

    // Construct a response Apdu with data.
    static RawApdu CreateResponse(std::vector<uint8_t> data, Status status) {
        // Append status word.
        data.push_back(status >> 8);
        data.push_back(status);
        return data;
    }

    // Construct a response Apdu with no data.
    static RawApdu CreateResponse(Status status) {
        // Append status word.
        return std::vector<uint8_t>{static_cast<uint8_t>(status >> 8),
                                    static_cast<uint8_t>(status)};
    }

    // Return if command APDU is extended.
    // The ISO/IEC 7816-4:2013 specification defines an extended APDU as any APDU
    // whose payload data, response data or expected data length exceeds the 256
    // byte limit.
    bool IsExtended() const { return (bytes_.size() - kHeaderSize) > 256; }

    // Return if command APDU has payload bytes.
    bool HasPayload() const { return bytes_.size() > kHeaderSize; }

    uint8_t get_cla() const { return bytes_[0]; }
    uint8_t get_ins() const { return bytes_[1]; }
    uint8_t get_p1() const { return bytes_[2]; }
    uint8_t get_p2() const { return bytes_[3]; }

    // Return the channel number encoded in the CLA field.
    uint8_t get_channel_number() const {
        // Type 4 commands — Encode legacy ISO/IEC 7816-4 logical channel
        // information. Type 16 commands — Defined by the ISO/IEC 7816-4:2013
        // specification to
        //   encode information for additional 16 logical channels in the card.
        uint8_t cla = get_cla();
        return (cla & 0x40) == 0 ? cla & 0x3 : 4 + (cla & 0xf);
    }

    // Return the length of the command data field.
    uint16_t get_lc() const { return lc_; }

    // Return the expected length of the response data field.
    // Le should be have the same format as Lc.
    uint16_t get_le() const { return le_; }

    // Get the pointer to the APDU raw data.
    std::vector<uint8_t> const& get_data() const { return bytes_; }

  private:
    // Size of command header, including CLA, INS, P1, P2 fields.
    const size_t kHeaderSize = 4;

    // Command or response buffer.
    std::vector<uint8_t> bytes_{};

    // Lengths of command data field and expected response data field.
    uint16_t lc_{0};
    uint16_t le_{0};
};

// Type of SE applets.
class Applet {
  public:
    virtual ~Applet() {}

    // Called to inform this applet that it has been selected.
    virtual RawApdu Select(Aid const& aid, uint8_t p2) = 0;

    // Called by the Java Card runtime environment to process an
    // incoming APDU command. SELECT commands are processed by \ref select
    // instead.
    virtual RawApdu Process(Apdu const& apdu) = 0;
};
};  // namespace se

// Implement the Google-eSE-test.cap test applet for passing OMAPI CTS tests
// on Cuttlefish. The reference can be found here:
// cts/tests/tests/secure_element/sample_applet/src/com/android/cts/omapi/test/CtsAndroidOmapiTestApplet.java
class CtsAndroidOmapiTestApplet : public se::Applet {
  public:
    CtsAndroidOmapiTestApplet() {}
    virtual ~CtsAndroidOmapiTestApplet() {}

    se::RawApdu Select(se::Aid const& aid, uint8_t /*p2*/) override {
        if (aid[aid.size() - 1] == 0x31) {
            // AID: A000000476416E64726F696443545331
            return se::Apdu::CreateResponse(se::Status::SW_NO_ERROR);
        } else {
            // AID: A000000476416E64726F696443545332
            return se::Apdu::CreateResponse(GenerateBerTLVBytes(SELECT_RESPONSE_DATA_LENGTH),
                                            se::Status::SW_NO_ERROR);
        }
    }

    se::RawApdu ReadNextResponseChunk(uint16_t max_output_len) {
        uint16_t output_len = static_cast<uint16_t>(response_.size() - response_offset_);
        output_len = std::min<uint16_t>(max_output_len, output_len);
        std::vector<uint8_t> output{
                &response_[response_offset_],
                &response_[response_offset_ + output_len],
        };
        response_offset_ += output_len;
        uint16_t remaining_len = response_.size() - response_offset_;
        se::Status status = se::Status::SW_NO_ERROR;
        if (remaining_len > 0) {
            if (remaining_len > 256) {
                remaining_len = 0x00;
            }
            status = se::Status(se::Status::SW_BYTES_REMAINING_00 | remaining_len);
        } else {
            response_.clear();
            response_offset_ = 0;
        }
        return se::Apdu::CreateResponse(output, status);
    }

    se::RawApdu Process(se::Apdu const& apdu) override {
        uint16_t lc;
        uint16_t le = apdu.get_le();
        uint8_t p1 = apdu.get_p1();
        uint8_t p2 = apdu.get_p2();

        switch (apdu.get_ins()) {
            case NO_DATA_INS_1:
            case NO_DATA_INS_2:
                LOG(INFO) << __func__ << ": NO_DATA_INS_1|2";
                return se::Apdu::CreateResponse(se::Status::SW_NO_ERROR);

            case DATA_INS_1:
            case DATA_INS_2:
                // Return 256 bytes of data.
                LOG(INFO) << __func__ << ": DATA_INS_1|2";
                return se::Apdu::CreateResponse(GeneratesBytes(256), se::Status::SW_NO_ERROR);

            case GET_RESPONSE_INS:
                // ISO GET_RESPONSE command.
                LOG(INFO) << __func__ << ": GET_RESPONSE_INS";
                if (response_.empty()) {
                    return se::Apdu::CreateResponse(se::Status::SW_CONDITIONS_NOT_SATISFIED);
                }
                return ReadNextResponseChunk(apdu.get_le());

            case SW_62xx_APDU_INS:
                LOG(INFO) << __func__ << ": SW_62xx_APDU_INS";
                if (p1 < 1 || p1 > 16) {
                    return se::Apdu::CreateResponse(se::Status::SW_INCORRECT_P1P2);
                }
                if (p2 == SW_62xx_DATA_APDU_P2) {
                    return se::Apdu::CreateResponse(GeneratesBytes(3),
                                                    se::Status(SW_62xx_resp[p1 - 1]));
                }
                if (p2 == SW_62xx_VALIDATE_DATA_P2) {
                    std::vector<uint8_t> output{SW_62xx_VALIDATE_DATA_RESP.begin(),
                                                SW_62xx_VALIDATE_DATA_RESP.end()};
                    output[2] = p1;
                    return se::Apdu::CreateResponse(std::move(output),
                                                    se::Status(SW_62xx_resp[p1 - 1]));
                }
                return se::Apdu::CreateResponse(se::Status(SW_62xx_resp[p1 - 1]));

            case SEGMENTED_RESP_INS_1:
            case SEGMENTED_RESP_INS_2:
                LOG(INFO) << __func__ << ": SEGMENTED_RESP_INS_1|2";
                response_ = GeneratesBytes((static_cast<uint16_t>(p1) << 8) | p2);
                response_offset_ = 0;
                return ReadNextResponseChunk(std::min<uint16_t>(apdu.get_le(), 256));

            case SEGMENTED_RESP_INS_3:
            case SEGMENTED_RESP_INS_4:
                LOG(INFO) << __func__ << ": SEGMENTED_RESP_INS_3|4";
                response_ = GeneratesBytes((static_cast<uint16_t>(p1) << 8) | p2);
                response_offset_ = 0;
                return ReadNextResponseChunk(apdu.get_le());

            case SEGMENTED_RESP_INS_5:
                LOG(INFO) << __func__ << ": SEGMENTED_RESP_INS_5";
                if (le == 0xff) {
                    return se::Apdu::CreateResponse(
                            se::Status(se::Status::SW_CORRECT_LENGTH_00 | 0xff));
                }
                response_ = GeneratesBytes((static_cast<uint16_t>(p1) << 8) | p2);
                response_offset_ = 0;
                return ReadNextResponseChunk(apdu.get_le());

            case CHECK_SELECT_P2_APDU:
                LOG(INFO) << __func__ << ": CHECK_SELECT_P2_APDU";
                return se::Apdu::CreateResponse(std::vector<uint8_t>{apdu.get_p2()},
                                                se::Status::SW_NO_ERROR);

            default:
                // Case is not known.
                LOG(INFO) << __func__ << ": UNKNOWN_INS";
                return se::Apdu::CreateResponse(se::Status::SW_INS_NOT_SUPPORTED);
        }
    }

  private:
    std::vector<uint8_t> response_{};
    uint16_t response_offset_{0};

    static const uint8_t NO_DATA_INS_1 = 0x06;
    static const uint8_t NO_DATA_INS_2 = 0x0A;
    static const uint8_t DATA_INS_1 = 0x08;
    static const uint8_t DATA_INS_2 = 0x0C;
    static const uint8_t SW_62xx_APDU_INS = 0xF3;
    static const uint8_t SW_62xx_DATA_APDU_P2 = 0x08;
    static const uint8_t SW_62xx_VALIDATE_DATA_P2 = 0x0C;

    static constexpr std::array<uint8_t, 7> SW_62xx_VALIDATE_DATA_RESP = {0x01, 0xF3, 0x00, 0x0C,
                                                                          0x01, 0xAA, 0x00};
    static constexpr uint16_t SW_62xx_resp[] = {
            0x6200, 0x6281, 0x6282, 0x6283, 0x6285, 0x62F1, 0x62F2, 0x63F1,
            0x63F2, 0x63C2, 0x6202, 0x6280, 0x6284, 0x6286, 0x6300, 0x6381,
    };

    static const uint8_t SEGMENTED_RESP_INS_1 = 0xC2;
    static const uint8_t SEGMENTED_RESP_INS_2 = 0xC4;
    static const uint8_t SEGMENTED_RESP_INS_3 = 0xC6;
    static const uint8_t SEGMENTED_RESP_INS_4 = 0xC8;
    static const uint8_t SEGMENTED_RESP_INS_5 = 0xCF;
    static const uint8_t CHECK_SELECT_P2_APDU = 0xF4;
    static const uint8_t GET_RESPONSE_INS = 0xC0;
    static const uint8_t BER_TLV_TYPE = 0x1F;
    static const uint16_t SELECT_RESPONSE_DATA_LENGTH = 252;

    static const uint16_t LENGTH_256 = 0x0100;
    static constexpr std::array<uint8_t, 256> resp_bytes256{
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
            0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
            0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
            0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
            0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
            0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53,
            0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61,
            0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
            0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D,
            0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B,
            0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
            0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
            0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5,
            0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3,
            0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1,
            0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
            0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED,
            0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB,
            0xFC, 0xFD, 0xFE, 0xFF};

    // Generate a response buffer of the selected length containing valid
    // BER TLV bytes.
    static std::vector<uint8_t> GenerateBerTLVBytes(uint16_t le) {
        // Support length from 0x00 - 0x7FFF.
        uint16_t le_len = 1;
        if (le < (uint16_t)0x80) {
            le_len = 1;
        } else if (le < (uint16_t)0x100) {
            le_len = 2;
        } else {
            le_len = 3;
        }

        uint16_t total_len = (uint16_t)(le + 2 + le_len);
        std::vector<uint8_t> output(total_len);
        uint16_t i = 0;

        output[i++] = BER_TLV_TYPE;
        output[i++] = 0x00;  // second byte of Type
        if (le < 0x80) {
            output[i++] = le;
        } else if (le < 0x100) {
            output[i++] = 0x81;
            output[i++] = le;
        } else {
            output[i++] = 0x82;
            output[i++] = (le >> 8);
            output[i++] = (le & 0xFF);
        }
        while (i < total_len) {
            output[i++] = ((i - 2 - le_len) & 0xFF);
        }

        // Set the last byte to 0xFF for CTS validation.
        output[total_len - 1] = 0xFF;
        return output;
    }

    // Generate a response buffer of the selected length using the
    // array resp_bytes256 as input.
    static std::vector<uint8_t> GeneratesBytes(uint16_t total_len) {
        std::vector<uint8_t> output(total_len);
        uint16_t i = 0;

        while (i < total_len) {
            if ((total_len - i) >= resp_bytes256.size()) {
                std::memcpy(&output[i], resp_bytes256.data(), resp_bytes256.size());
                i += resp_bytes256.size();
            } else {
                output[i] = i & 0xFF;
                i += 1;
            }
        }

        // Set the last byte to 0xFF for CTS validation.
        output[total_len - 1] = 0xFF;
        return output;
    }
};

class EmulatedSecureElement : public BnSecureElement {
  public:
    EmulatedSecureElement() {
        std::shared_ptr<CtsAndroidOmapiTestApplet> test_applet =
                std::make_shared<CtsAndroidOmapiTestApplet>();

        applets_.push_back(std::pair{se::Aid{0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64, 0x72,
                                             0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x31},
                                     test_applet});

        applets_.push_back(std::pair{se::Aid{0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64, 0x72,
                                             0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x32},
                                     test_applet});
    }

    ScopedAStatus init(const std::shared_ptr<ISecureElementCallback>& clientCallback) override {
        LOG(INFO) << __func__ << " callback: " << clientCallback.get();
        if (!clientCallback) {
            return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
        }
        client_callback_ = clientCallback;
        client_callback_->onStateChange(true, "init");
        return ScopedAStatus::ok();
    }

    ScopedAStatus getAtr(std::vector<uint8_t>* aidl_return) override {
        LOG(INFO) << __func__;
        *aidl_return = atr_;
        return ScopedAStatus::ok();
    }

    ScopedAStatus reset() override {
        LOG(INFO) << __func__;
        CHECK(client_callback_ != nullptr) << " init not invoked";
        client_callback_->onStateChange(false, "reset");
        client_callback_->onStateChange(true, "reset");
        // All channels are closed after reset.
        for (auto channel : channels_) {
            channel = Channel();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus isCardPresent(bool* aidl_return) override {
        LOG(INFO) << __func__;
        *aidl_return = true;
        return ScopedAStatus::ok();
    }

    ScopedAStatus openBasicChannel(const std::vector<uint8_t>& aid, int8_t p2,
                                   std::vector<uint8_t>* aidl_return) override {
        LOG(INFO) << __func__ << " aid: " << HexString(aid.data(), aid.size()) << " (" << aid.size()
                  << ") p2 " << p2;

        std::vector<uint8_t> select_response;
        std::shared_ptr<se::Applet> applet = nullptr;

        // The basic channel can only be opened once, and stays opened
        // and locked until the channel is closed.
        if (channels_[0].opened) {
            LOG(INFO) << __func__ << " basic channel already opened";
            return ScopedAStatus::fromServiceSpecificError(CHANNEL_NOT_AVAILABLE);
        }

        // If the AID is defined (the AID is not Null and the length of the
        // AID is not 0) and the channel is not locked then the corresponding
        // applet shall be selected.
        if (aid.size() > 0) {
            applet = SelectApplet(aid);
            if (applet == nullptr) {
                // No applet registered with matching AID.
                LOG(INFO) << __func__ << " basic channel AID not found";
                return ScopedAStatus::fromServiceSpecificError(NO_SUCH_ELEMENT_ERROR);
            }
            select_response = applet->Select(aid, p2);
        }

        // If the AID is a 0 length AID and the channel is not locked, the
        // method will select the Issuer Security Domain of the SE by sending a
        // SELECT command with a 0 length AID as defined in
        // [GP Card specification].
        if (aid.size() == 0) {
            select_response = kIssuerSecurityDomainSelectResponse;
        }

        LOG(INFO) << __func__ << " sending response: "
                  << HexString(select_response.data(), select_response.size());

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol
        // or APDU. The functionality here is enough to exercise the framework,
        // but actual calls to the secure element will fail. This implementation
        // does not model channel isolation or any other aspects important to
        // implementing secure element.
        channels_[0] = Channel(aid, p2, applet);
        *aidl_return = select_response;
        return ScopedAStatus::ok();
    }

    ScopedAStatus openLogicalChannel(
            const std::vector<uint8_t>& aid, int8_t p2,
            ::aidl::android::hardware::secure_element::LogicalChannelResponse* aidl_return)
            override {
        LOG(INFO) << __func__ << " aid: " << HexString(aid.data(), aid.size()) << " (" << aid.size()
                  << ") p2 " << p2;

        size_t channel_number = 1;
        std::vector<uint8_t> select_response;
        std::shared_ptr<se::Applet> applet = nullptr;

        // Look for an available channel number.
        for (; channel_number < channels_.size(); channel_number++) {
            if (channels_[channel_number].opened == false) {
                break;
            }
        }

        // All channels are currently allocated.
        if (channel_number >= channels_.size()) {
            LOG(INFO) << __func__ << " all logical channels already opened";
            return ScopedAStatus::fromServiceSpecificError(CHANNEL_NOT_AVAILABLE);
        }

        // If the AID is defined (the AID is not Null and the length of the
        // AID is not 0) then the corresponding applet shall be selected.
        if (aid.size() > 0) {
            applet = SelectApplet(aid);
            if (applet == nullptr) {
                // No applet registered with matching AID.
                LOG(INFO) << __func__ << " logical channel AID not found";
                return ScopedAStatus::fromServiceSpecificError(NO_SUCH_ELEMENT_ERROR);
            }
            select_response = applet->Select(aid, p2);
        }

        // If the length of the AID is 0, the method will select the
        // Issuer Security Domain of the SE by sending a SELECT command
        // with 0 length AID as defined in [GPCS].
        if (aid.size() == 0) {
            select_response = kIssuerSecurityDomainSelectResponse;
        }

        LOG(INFO) << __func__ << " sending response: "
                  << HexString(select_response.data(), select_response.size());

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol
        // or APDU. The functionality here is enough to exercise the framework,
        // but actual calls to the secure element will fail. This implementation
        // does not model channel isolation or any other aspects important to
        // implementing secure element.
        channels_[channel_number] = Channel(aid, p2, applet);
        *aidl_return = LogicalChannelResponse{
                .channelNumber = static_cast<int8_t>(channel_number),
                .selectResponse = select_response,
        };
        return ScopedAStatus::ok();
    }

    ScopedAStatus closeChannel(int8_t channel_number) override {
        LOG(INFO) << __func__ << " channel number: " << static_cast<int>(channel_number);
        // The selected basic or logical channel is not opened.
        if (channel_number >= channels_.size() || !channels_[channel_number].opened) {
            return ScopedAStatus::ok();
        }

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol
        // or APDU. The functionality here is enough to exercise the framework,
        // but actual calls to the secure element will fail. This implementation
        // does not model channel isolation or any other aspects important to
        // implementing secure element.
        channels_[channel_number].opened = false;
        return ScopedAStatus::ok();
    }

    ScopedAStatus transmit(const std::vector<uint8_t>& data,
                           std::vector<uint8_t>* aidl_return) override {
        LOG(INFO) << __func__ << " data: " << HexString(data.data(), data.size()) << " ("
                  << data.size() << ")";

        se::Apdu apdu(data);
        uint8_t channel_number = apdu.get_channel_number();
        std::vector<uint8_t> response_apdu;

        switch (apdu.get_ins()) {
            // TODO(b/123254068) - Implement support channel management APDUs.
            case MANAGE_CHANNEL_INS:
                // P1 = '00' to open
                // P1 = '80' to close
                LOG(INFO) << __func__ << " MANAGE_CHANNEL apdu";
                response_apdu =
                        se::Apdu::CreateResponse(se::Status::SW_LOGICAL_CHANNEL_NOT_SUPPORTED);
                break;

            // TODO(b/123254068) - Implement support channel management APDUs.
            case SELECT_INS:
                LOG(INFO) << __func__ << " SELECT apdu";
                response_apdu =
                        se::Apdu::CreateResponse(se::Status::SW_LOGICAL_CHANNEL_NOT_SUPPORTED);
                break;

            default:
                CHECK(channel_number < channels_.size()) << " invalid channel number";
                if (!channels_[channel_number].opened) {
                    LOG(INFO) << __func__ << " the channel " << static_cast<int>(channel_number)
                              << " is not opened";
                    response_apdu =
                            se::Apdu::CreateResponse(se::Status::SW_LOGICAL_CHANNEL_NOT_SUPPORTED);
                    break;
                }
                // Send the APDU to the applet for processing.
                // Applet implementation is optional, default to sending
                // SW_INS_NOT_SUPPORTED.
                if (channels_[channel_number].applet == nullptr) {
                    response_apdu = se::Apdu::CreateResponse(se::Status::SW_INS_NOT_SUPPORTED);
                } else {
                    response_apdu = channels_[channel_number].applet->Process(apdu);
                }
                break;
        }

        aidl_return->assign(response_apdu.begin(), response_apdu.end());
        LOG(INFO) << __func__
                  << " response: " << HexString(aidl_return->data(), aidl_return->size()) << " ("
                  << aidl_return->size() << ")";
        return ScopedAStatus::ok();
    }

  private:
    struct Channel {
      public:
        Channel() = default;
        Channel(Channel const&) = default;
        Channel(se::Aid const& aid, uint8_t p2, std::shared_ptr<se::Applet> applet)
            : opened(true), aid(aid), p2(p2), applet(std::move(applet)) {}
        Channel& operator=(Channel const&) = default;

        bool opened{false};
        se::Aid aid{};
        uint8_t p2{0};
        std::shared_ptr<se::Applet> applet{nullptr};
    };

    // OMAPI abstraction.

    // Channel 0 is the basic channel, channels 1-19 are the logical channels.
    std::array<Channel, 20> channels_{};
    std::shared_ptr<ISecureElementCallback> client_callback_;

    // Secure element abstraction.

    static const uint8_t MANAGE_CHANNEL_INS = 0x70;
    static const uint8_t SELECT_INS = 0xa4;

    // Secure element ATR (Answer-To-Reset).
    // The format is specified by ISO/IEC 1816-4 2020 and lists
    // the capabilities of the card.
    //
    // TODO(b/123254068): encode the default SE properties in the ATR:
    // support for extended Lc / Le fields, maximum number of logical channels.
    // The CTS tests are *not* checking this value.
    std::vector<uint8_t> const atr_{};

    // Applet registration.
    std::vector<std::pair<se::Aid, std::shared_ptr<se::Applet>>> applets_{};

    // Return the first applet that matches the selected aid.
    std::shared_ptr<se::Applet> SelectApplet(se::Aid const& aid) {
        for (auto& [applet_aid, applet] : applets_) {
            if (applet_aid == aid) {
                return applet;
            }
        }
        return nullptr;
    }
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    auto se = ndk::SharedRefBase::make<EmulatedSecureElement>();
    const std::string name = std::string() + BnSecureElement::descriptor + "/eSE1";
    binder_status_t status = AServiceManager_addService(se->asBinder().get(), name.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
