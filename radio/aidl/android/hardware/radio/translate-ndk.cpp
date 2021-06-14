/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "android/hardware/radio/translate-ndk.h"

namespace android::h2a {

static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::ADDRESS_MAX ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::ADDRESS_MAX));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::SUBADDRESS_MAX ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::SUBADDRESS_MAX));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::BEARER_DATA_MAX ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::BEARER_DATA_MAX));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_MAX_SND_SIZE ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_MAX_SND_SIZE));
static_assert(
        aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_EO_DATA_SEGMENT_MAX ==
        static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_EO_DATA_SEGMENT_MAX));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::MAX_UD_HEADERS ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::MAX_UD_HEADERS));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::USER_DATA_MAX ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::USER_DATA_MAX));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_LARGE_PIC_SIZE ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_LARGE_PIC_SIZE));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_SMALL_PIC_SIZE ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_SMALL_PIC_SIZE));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_VAR_PIC_SIZE ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_VAR_PIC_SIZE));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_ANIM_NUM_BITMAPS ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_ANIM_NUM_BITMAPS));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_LARGE_BITMAP_SIZE ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_LARGE_BITMAP_SIZE));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_SMALL_BITMAP_SIZE ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_SMALL_BITMAP_SIZE));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::UDH_OTHER_SIZE ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::UDH_OTHER_SIZE));
static_assert(aidl::android::hardware::radio::RadioCdmaSmsConst::IP_ADDRESS_SIZE ==
              static_cast<aidl::android::hardware::radio::RadioCdmaSmsConst>(
                      ::android::hardware::radio::V1_0::RadioCdmaSmsConst::IP_ADDRESS_SIZE));

static_assert(aidl::android::hardware::radio::RadioResponseType::SOLICITED ==
              static_cast<aidl::android::hardware::radio::RadioResponseType>(
                      ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED));
static_assert(aidl::android::hardware::radio::RadioResponseType::SOLICITED_ACK ==
              static_cast<aidl::android::hardware::radio::RadioResponseType>(
                      ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED_ACK));
static_assert(aidl::android::hardware::radio::RadioResponseType::SOLICITED_ACK_EXP ==
              static_cast<aidl::android::hardware::radio::RadioResponseType>(
                      ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED_ACK_EXP));

static_assert(aidl::android::hardware::radio::RadioIndicationType::UNSOLICITED ==
              static_cast<aidl::android::hardware::radio::RadioIndicationType>(
                      ::android::hardware::radio::V1_0::RadioIndicationType::UNSOLICITED));
static_assert(aidl::android::hardware::radio::RadioIndicationType::UNSOLICITED_ACK_EXP ==
              static_cast<aidl::android::hardware::radio::RadioIndicationType>(
                      ::android::hardware::radio::V1_0::RadioIndicationType::UNSOLICITED_ACK_EXP));

static_assert(aidl::android::hardware::radio::RestrictedState::NONE ==
              static_cast<aidl::android::hardware::radio::RestrictedState>(
                      ::android::hardware::radio::V1_0::RestrictedState::NONE));
static_assert(aidl::android::hardware::radio::RestrictedState::CS_EMERGENCY ==
              static_cast<aidl::android::hardware::radio::RestrictedState>(
                      ::android::hardware::radio::V1_0::RestrictedState::CS_EMERGENCY));
static_assert(aidl::android::hardware::radio::RestrictedState::CS_NORMAL ==
              static_cast<aidl::android::hardware::radio::RestrictedState>(
                      ::android::hardware::radio::V1_0::RestrictedState::CS_NORMAL));
static_assert(aidl::android::hardware::radio::RestrictedState::CS_ALL ==
              static_cast<aidl::android::hardware::radio::RestrictedState>(
                      ::android::hardware::radio::V1_0::RestrictedState::CS_ALL));
static_assert(aidl::android::hardware::radio::RestrictedState::PS_ALL ==
              static_cast<aidl::android::hardware::radio::RestrictedState>(
                      ::android::hardware::radio::V1_0::RestrictedState::PS_ALL));

static_assert(aidl::android::hardware::radio::CardState::ABSENT ==
              static_cast<aidl::android::hardware::radio::CardState>(
                      ::android::hardware::radio::V1_0::CardState::ABSENT));
static_assert(aidl::android::hardware::radio::CardState::PRESENT ==
              static_cast<aidl::android::hardware::radio::CardState>(
                      ::android::hardware::radio::V1_0::CardState::PRESENT));
static_assert(aidl::android::hardware::radio::CardState::ERROR ==
              static_cast<aidl::android::hardware::radio::CardState>(
                      ::android::hardware::radio::V1_0::CardState::ERROR));
static_assert(aidl::android::hardware::radio::CardState::RESTRICTED ==
              static_cast<aidl::android::hardware::radio::CardState>(
                      ::android::hardware::radio::V1_0::CardState::RESTRICTED));

static_assert(aidl::android::hardware::radio::PinState::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::PinState>(
                      ::android::hardware::radio::V1_0::PinState::UNKNOWN));
static_assert(aidl::android::hardware::radio::PinState::ENABLED_NOT_VERIFIED ==
              static_cast<aidl::android::hardware::radio::PinState>(
                      ::android::hardware::radio::V1_0::PinState::ENABLED_NOT_VERIFIED));
static_assert(aidl::android::hardware::radio::PinState::ENABLED_VERIFIED ==
              static_cast<aidl::android::hardware::radio::PinState>(
                      ::android::hardware::radio::V1_0::PinState::ENABLED_VERIFIED));
static_assert(aidl::android::hardware::radio::PinState::DISABLED ==
              static_cast<aidl::android::hardware::radio::PinState>(
                      ::android::hardware::radio::V1_0::PinState::DISABLED));
static_assert(aidl::android::hardware::radio::PinState::ENABLED_BLOCKED ==
              static_cast<aidl::android::hardware::radio::PinState>(
                      ::android::hardware::radio::V1_0::PinState::ENABLED_BLOCKED));
static_assert(aidl::android::hardware::radio::PinState::ENABLED_PERM_BLOCKED ==
              static_cast<aidl::android::hardware::radio::PinState>(
                      ::android::hardware::radio::V1_0::PinState::ENABLED_PERM_BLOCKED));

static_assert(aidl::android::hardware::radio::AppType::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::AppType>(
                      ::android::hardware::radio::V1_0::AppType::UNKNOWN));
static_assert(aidl::android::hardware::radio::AppType::SIM ==
              static_cast<aidl::android::hardware::radio::AppType>(
                      ::android::hardware::radio::V1_0::AppType::SIM));
static_assert(aidl::android::hardware::radio::AppType::USIM ==
              static_cast<aidl::android::hardware::radio::AppType>(
                      ::android::hardware::radio::V1_0::AppType::USIM));
static_assert(aidl::android::hardware::radio::AppType::RUIM ==
              static_cast<aidl::android::hardware::radio::AppType>(
                      ::android::hardware::radio::V1_0::AppType::RUIM));
static_assert(aidl::android::hardware::radio::AppType::CSIM ==
              static_cast<aidl::android::hardware::radio::AppType>(
                      ::android::hardware::radio::V1_0::AppType::CSIM));
static_assert(aidl::android::hardware::radio::AppType::ISIM ==
              static_cast<aidl::android::hardware::radio::AppType>(
                      ::android::hardware::radio::V1_0::AppType::ISIM));

static_assert(aidl::android::hardware::radio::AppState::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::AppState>(
                      ::android::hardware::radio::V1_0::AppState::UNKNOWN));
static_assert(aidl::android::hardware::radio::AppState::DETECTED ==
              static_cast<aidl::android::hardware::radio::AppState>(
                      ::android::hardware::radio::V1_0::AppState::DETECTED));
static_assert(aidl::android::hardware::radio::AppState::PIN ==
              static_cast<aidl::android::hardware::radio::AppState>(
                      ::android::hardware::radio::V1_0::AppState::PIN));
static_assert(aidl::android::hardware::radio::AppState::PUK ==
              static_cast<aidl::android::hardware::radio::AppState>(
                      ::android::hardware::radio::V1_0::AppState::PUK));
static_assert(aidl::android::hardware::radio::AppState::SUBSCRIPTION_PERSO ==
              static_cast<aidl::android::hardware::radio::AppState>(
                      ::android::hardware::radio::V1_0::AppState::SUBSCRIPTION_PERSO));
static_assert(aidl::android::hardware::radio::AppState::READY ==
              static_cast<aidl::android::hardware::radio::AppState>(
                      ::android::hardware::radio::V1_0::AppState::READY));

static_assert(aidl::android::hardware::radio::RadioState::OFF ==
              static_cast<aidl::android::hardware::radio::RadioState>(
                      ::android::hardware::radio::V1_0::RadioState::OFF));
static_assert(aidl::android::hardware::radio::RadioState::UNAVAILABLE ==
              static_cast<aidl::android::hardware::radio::RadioState>(
                      ::android::hardware::radio::V1_0::RadioState::UNAVAILABLE));
static_assert(aidl::android::hardware::radio::RadioState::ON ==
              static_cast<aidl::android::hardware::radio::RadioState>(
                      ::android::hardware::radio::V1_0::RadioState::ON));

static_assert(aidl::android::hardware::radio::SapConnectRsp::SUCCESS ==
              static_cast<aidl::android::hardware::radio::SapConnectRsp>(
                      ::android::hardware::radio::V1_0::SapConnectRsp::SUCCESS));
static_assert(aidl::android::hardware::radio::SapConnectRsp::CONNECT_FAILURE ==
              static_cast<aidl::android::hardware::radio::SapConnectRsp>(
                      ::android::hardware::radio::V1_0::SapConnectRsp::CONNECT_FAILURE));
static_assert(aidl::android::hardware::radio::SapConnectRsp::MSG_SIZE_TOO_LARGE ==
              static_cast<aidl::android::hardware::radio::SapConnectRsp>(
                      ::android::hardware::radio::V1_0::SapConnectRsp::MSG_SIZE_TOO_LARGE));
static_assert(aidl::android::hardware::radio::SapConnectRsp::MSG_SIZE_TOO_SMALL ==
              static_cast<aidl::android::hardware::radio::SapConnectRsp>(
                      ::android::hardware::radio::V1_0::SapConnectRsp::MSG_SIZE_TOO_SMALL));
static_assert(aidl::android::hardware::radio::SapConnectRsp::CONNECT_OK_CALL_ONGOING ==
              static_cast<aidl::android::hardware::radio::SapConnectRsp>(
                      ::android::hardware::radio::V1_0::SapConnectRsp::CONNECT_OK_CALL_ONGOING));

static_assert(aidl::android::hardware::radio::SapDisconnectType::GRACEFUL ==
              static_cast<aidl::android::hardware::radio::SapDisconnectType>(
                      ::android::hardware::radio::V1_0::SapDisconnectType::GRACEFUL));
static_assert(aidl::android::hardware::radio::SapDisconnectType::IMMEDIATE ==
              static_cast<aidl::android::hardware::radio::SapDisconnectType>(
                      ::android::hardware::radio::V1_0::SapDisconnectType::IMMEDIATE));

static_assert(aidl::android::hardware::radio::SapApduType::APDU ==
              static_cast<aidl::android::hardware::radio::SapApduType>(
                      ::android::hardware::radio::V1_0::SapApduType::APDU));
static_assert(aidl::android::hardware::radio::SapApduType::APDU7816 ==
              static_cast<aidl::android::hardware::radio::SapApduType>(
                      ::android::hardware::radio::V1_0::SapApduType::APDU7816));

static_assert(aidl::android::hardware::radio::SapResultCode::SUCCESS ==
              static_cast<aidl::android::hardware::radio::SapResultCode>(
                      ::android::hardware::radio::V1_0::SapResultCode::SUCCESS));
static_assert(aidl::android::hardware::radio::SapResultCode::GENERIC_FAILURE ==
              static_cast<aidl::android::hardware::radio::SapResultCode>(
                      ::android::hardware::radio::V1_0::SapResultCode::GENERIC_FAILURE));
static_assert(aidl::android::hardware::radio::SapResultCode::CARD_NOT_ACCESSSIBLE ==
              static_cast<aidl::android::hardware::radio::SapResultCode>(
                      ::android::hardware::radio::V1_0::SapResultCode::CARD_NOT_ACCESSSIBLE));
static_assert(aidl::android::hardware::radio::SapResultCode::CARD_ALREADY_POWERED_OFF ==
              static_cast<aidl::android::hardware::radio::SapResultCode>(
                      ::android::hardware::radio::V1_0::SapResultCode::CARD_ALREADY_POWERED_OFF));
static_assert(aidl::android::hardware::radio::SapResultCode::CARD_REMOVED ==
              static_cast<aidl::android::hardware::radio::SapResultCode>(
                      ::android::hardware::radio::V1_0::SapResultCode::CARD_REMOVED));
static_assert(aidl::android::hardware::radio::SapResultCode::CARD_ALREADY_POWERED_ON ==
              static_cast<aidl::android::hardware::radio::SapResultCode>(
                      ::android::hardware::radio::V1_0::SapResultCode::CARD_ALREADY_POWERED_ON));
static_assert(aidl::android::hardware::radio::SapResultCode::DATA_NOT_AVAILABLE ==
              static_cast<aidl::android::hardware::radio::SapResultCode>(
                      ::android::hardware::radio::V1_0::SapResultCode::DATA_NOT_AVAILABLE));
static_assert(aidl::android::hardware::radio::SapResultCode::NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::SapResultCode>(
                      ::android::hardware::radio::V1_0::SapResultCode::NOT_SUPPORTED));

static_assert(aidl::android::hardware::radio::SapStatus::UNKNOWN_ERROR ==
              static_cast<aidl::android::hardware::radio::SapStatus>(
                      ::android::hardware::radio::V1_0::SapStatus::UNKNOWN_ERROR));
static_assert(aidl::android::hardware::radio::SapStatus::CARD_RESET ==
              static_cast<aidl::android::hardware::radio::SapStatus>(
                      ::android::hardware::radio::V1_0::SapStatus::CARD_RESET));
static_assert(aidl::android::hardware::radio::SapStatus::CARD_NOT_ACCESSIBLE ==
              static_cast<aidl::android::hardware::radio::SapStatus>(
                      ::android::hardware::radio::V1_0::SapStatus::CARD_NOT_ACCESSIBLE));
static_assert(aidl::android::hardware::radio::SapStatus::CARD_REMOVED ==
              static_cast<aidl::android::hardware::radio::SapStatus>(
                      ::android::hardware::radio::V1_0::SapStatus::CARD_REMOVED));
static_assert(aidl::android::hardware::radio::SapStatus::CARD_INSERTED ==
              static_cast<aidl::android::hardware::radio::SapStatus>(
                      ::android::hardware::radio::V1_0::SapStatus::CARD_INSERTED));
static_assert(aidl::android::hardware::radio::SapStatus::RECOVERED ==
              static_cast<aidl::android::hardware::radio::SapStatus>(
                      ::android::hardware::radio::V1_0::SapStatus::RECOVERED));

static_assert(aidl::android::hardware::radio::SapTransferProtocol::T0 ==
              static_cast<aidl::android::hardware::radio::SapTransferProtocol>(
                      ::android::hardware::radio::V1_0::SapTransferProtocol::T0));
static_assert(aidl::android::hardware::radio::SapTransferProtocol::T1 ==
              static_cast<aidl::android::hardware::radio::SapTransferProtocol>(
                      ::android::hardware::radio::V1_0::SapTransferProtocol::T1));

static_assert(aidl::android::hardware::radio::CallState::ACTIVE ==
              static_cast<aidl::android::hardware::radio::CallState>(
                      ::android::hardware::radio::V1_0::CallState::ACTIVE));
static_assert(aidl::android::hardware::radio::CallState::HOLDING ==
              static_cast<aidl::android::hardware::radio::CallState>(
                      ::android::hardware::radio::V1_0::CallState::HOLDING));
static_assert(aidl::android::hardware::radio::CallState::DIALING ==
              static_cast<aidl::android::hardware::radio::CallState>(
                      ::android::hardware::radio::V1_0::CallState::DIALING));
static_assert(aidl::android::hardware::radio::CallState::ALERTING ==
              static_cast<aidl::android::hardware::radio::CallState>(
                      ::android::hardware::radio::V1_0::CallState::ALERTING));
static_assert(aidl::android::hardware::radio::CallState::INCOMING ==
              static_cast<aidl::android::hardware::radio::CallState>(
                      ::android::hardware::radio::V1_0::CallState::INCOMING));
static_assert(aidl::android::hardware::radio::CallState::WAITING ==
              static_cast<aidl::android::hardware::radio::CallState>(
                      ::android::hardware::radio::V1_0::CallState::WAITING));

static_assert(aidl::android::hardware::radio::UusType::TYPE1_IMPLICIT ==
              static_cast<aidl::android::hardware::radio::UusType>(
                      ::android::hardware::radio::V1_0::UusType::TYPE1_IMPLICIT));
static_assert(aidl::android::hardware::radio::UusType::TYPE1_REQUIRED ==
              static_cast<aidl::android::hardware::radio::UusType>(
                      ::android::hardware::radio::V1_0::UusType::TYPE1_REQUIRED));
static_assert(aidl::android::hardware::radio::UusType::TYPE1_NOT_REQUIRED ==
              static_cast<aidl::android::hardware::radio::UusType>(
                      ::android::hardware::radio::V1_0::UusType::TYPE1_NOT_REQUIRED));
static_assert(aidl::android::hardware::radio::UusType::TYPE2_REQUIRED ==
              static_cast<aidl::android::hardware::radio::UusType>(
                      ::android::hardware::radio::V1_0::UusType::TYPE2_REQUIRED));
static_assert(aidl::android::hardware::radio::UusType::TYPE2_NOT_REQUIRED ==
              static_cast<aidl::android::hardware::radio::UusType>(
                      ::android::hardware::radio::V1_0::UusType::TYPE2_NOT_REQUIRED));
static_assert(aidl::android::hardware::radio::UusType::TYPE3_REQUIRED ==
              static_cast<aidl::android::hardware::radio::UusType>(
                      ::android::hardware::radio::V1_0::UusType::TYPE3_REQUIRED));
static_assert(aidl::android::hardware::radio::UusType::TYPE3_NOT_REQUIRED ==
              static_cast<aidl::android::hardware::radio::UusType>(
                      ::android::hardware::radio::V1_0::UusType::TYPE3_NOT_REQUIRED));

static_assert(aidl::android::hardware::radio::UusDcs::USP ==
              static_cast<aidl::android::hardware::radio::UusDcs>(
                      ::android::hardware::radio::V1_0::UusDcs::USP));
static_assert(aidl::android::hardware::radio::UusDcs::OSIHLP ==
              static_cast<aidl::android::hardware::radio::UusDcs>(
                      ::android::hardware::radio::V1_0::UusDcs::OSIHLP));
static_assert(aidl::android::hardware::radio::UusDcs::X244 ==
              static_cast<aidl::android::hardware::radio::UusDcs>(
                      ::android::hardware::radio::V1_0::UusDcs::X244));
static_assert(aidl::android::hardware::radio::UusDcs::RMCF ==
              static_cast<aidl::android::hardware::radio::UusDcs>(
                      ::android::hardware::radio::V1_0::UusDcs::RMCF));
static_assert(aidl::android::hardware::radio::UusDcs::IA5C ==
              static_cast<aidl::android::hardware::radio::UusDcs>(
                      ::android::hardware::radio::V1_0::UusDcs::IA5C));

static_assert(aidl::android::hardware::radio::CallPresentation::ALLOWED ==
              static_cast<aidl::android::hardware::radio::CallPresentation>(
                      ::android::hardware::radio::V1_0::CallPresentation::ALLOWED));
static_assert(aidl::android::hardware::radio::CallPresentation::RESTRICTED ==
              static_cast<aidl::android::hardware::radio::CallPresentation>(
                      ::android::hardware::radio::V1_0::CallPresentation::RESTRICTED));
static_assert(aidl::android::hardware::radio::CallPresentation::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::CallPresentation>(
                      ::android::hardware::radio::V1_0::CallPresentation::UNKNOWN));
static_assert(aidl::android::hardware::radio::CallPresentation::PAYPHONE ==
              static_cast<aidl::android::hardware::radio::CallPresentation>(
                      ::android::hardware::radio::V1_0::CallPresentation::PAYPHONE));

static_assert(aidl::android::hardware::radio::Clir::DEFAULT ==
              static_cast<aidl::android::hardware::radio::Clir>(
                      ::android::hardware::radio::V1_0::Clir::DEFAULT));
static_assert(aidl::android::hardware::radio::Clir::INVOCATION ==
              static_cast<aidl::android::hardware::radio::Clir>(
                      ::android::hardware::radio::V1_0::Clir::INVOCATION));
static_assert(aidl::android::hardware::radio::Clir::SUPPRESSION ==
              static_cast<aidl::android::hardware::radio::Clir>(
                      ::android::hardware::radio::V1_0::Clir::SUPPRESSION));

static_assert(aidl::android::hardware::radio::LastCallFailCause::UNOBTAINABLE_NUMBER ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::UNOBTAINABLE_NUMBER));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::NO_ROUTE_TO_DESTINATION ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::NO_ROUTE_TO_DESTINATION));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CHANNEL_UNACCEPTABLE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CHANNEL_UNACCEPTABLE));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::OPERATOR_DETERMINED_BARRING ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::OPERATOR_DETERMINED_BARRING));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NORMAL ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NORMAL));
static_assert(aidl::android::hardware::radio::LastCallFailCause::BUSY ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::BUSY));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NO_USER_RESPONDING ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NO_USER_RESPONDING));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NO_ANSWER_FROM_USER ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NO_ANSWER_FROM_USER));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CALL_REJECTED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CALL_REJECTED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NUMBER_CHANGED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NUMBER_CHANGED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::PREEMPTION ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::PREEMPTION));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::DESTINATION_OUT_OF_ORDER ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::DESTINATION_OUT_OF_ORDER));
static_assert(aidl::android::hardware::radio::LastCallFailCause::INVALID_NUMBER_FORMAT ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::INVALID_NUMBER_FORMAT));
static_assert(aidl::android::hardware::radio::LastCallFailCause::FACILITY_REJECTED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::FACILITY_REJECTED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RESP_TO_STATUS_ENQUIRY ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RESP_TO_STATUS_ENQUIRY));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NORMAL_UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NORMAL_UNSPECIFIED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CONGESTION ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CONGESTION));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NETWORK_OUT_OF_ORDER ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NETWORK_OUT_OF_ORDER));
static_assert(aidl::android::hardware::radio::LastCallFailCause::TEMPORARY_FAILURE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::TEMPORARY_FAILURE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::SWITCHING_EQUIPMENT_CONGESTION ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              SWITCHING_EQUIPMENT_CONGESTION));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::ACCESS_INFORMATION_DISCARDED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::ACCESS_INFORMATION_DISCARDED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::
                      REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::RESOURCES_UNAVAILABLE_OR_UNSPECIFIED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::
                        RESOURCES_UNAVAILABLE_OR_UNSPECIFIED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::QOS_UNAVAILABLE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::QOS_UNAVAILABLE));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::REQUESTED_FACILITY_NOT_SUBSCRIBED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::
                        REQUESTED_FACILITY_NOT_SUBSCRIBED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::INCOMING_CALLS_BARRED_WITHIN_CUG ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              INCOMING_CALLS_BARRED_WITHIN_CUG));
static_assert(aidl::android::hardware::radio::LastCallFailCause::BEARER_CAPABILITY_NOT_AUTHORIZED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              BEARER_CAPABILITY_NOT_AUTHORIZED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::BEARER_CAPABILITY_UNAVAILABLE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              BEARER_CAPABILITY_UNAVAILABLE));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::SERVICE_OPTION_NOT_AVAILABLE ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::SERVICE_OPTION_NOT_AVAILABLE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::BEARER_SERVICE_NOT_IMPLEMENTED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              BEARER_SERVICE_NOT_IMPLEMENTED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::ACM_LIMIT_EXCEEDED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::ACM_LIMIT_EXCEEDED));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::REQUESTED_FACILITY_NOT_IMPLEMENTED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::
                        REQUESTED_FACILITY_NOT_IMPLEMENTED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::
                      ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::SERVICE_OR_OPTION_NOT_IMPLEMENTED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::
                        SERVICE_OR_OPTION_NOT_IMPLEMENTED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::INVALID_TRANSACTION_IDENTIFIER ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              INVALID_TRANSACTION_IDENTIFIER));
static_assert(aidl::android::hardware::radio::LastCallFailCause::USER_NOT_MEMBER_OF_CUG ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::USER_NOT_MEMBER_OF_CUG));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::INCOMPATIBLE_DESTINATION ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::INCOMPATIBLE_DESTINATION));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::INVALID_TRANSIT_NW_SELECTION ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::INVALID_TRANSIT_NW_SELECTION));
static_assert(aidl::android::hardware::radio::LastCallFailCause::SEMANTICALLY_INCORRECT_MESSAGE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              SEMANTICALLY_INCORRECT_MESSAGE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::INVALID_MANDATORY_INFORMATION ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              INVALID_MANDATORY_INFORMATION));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::MESSAGE_TYPE_NON_IMPLEMENTED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::MESSAGE_TYPE_NON_IMPLEMENTED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::
                      MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::INFORMATION_ELEMENT_NON_EXISTENT ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              INFORMATION_ELEMENT_NON_EXISTENT));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CONDITIONAL_IE_ERROR ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CONDITIONAL_IE_ERROR));
static_assert(aidl::android::hardware::radio::LastCallFailCause::
                      MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::RECOVERY_ON_TIMER_EXPIRED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::RECOVERY_ON_TIMER_EXPIRED));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::PROTOCOL_ERROR_UNSPECIFIED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::PROTOCOL_ERROR_UNSPECIFIED));
static_assert(
        aidl::android::hardware::radio::LastCallFailCause::INTERWORKING_UNSPECIFIED ==
        static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                ::android::hardware::radio::V1_0::LastCallFailCause::INTERWORKING_UNSPECIFIED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CALL_BARRED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CALL_BARRED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::FDN_BLOCKED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::FDN_BLOCKED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::IMSI_UNKNOWN_IN_VLR ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::IMSI_UNKNOWN_IN_VLR));
static_assert(aidl::android::hardware::radio::LastCallFailCause::IMEI_NOT_ACCEPTED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::IMEI_NOT_ACCEPTED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::DIAL_MODIFIED_TO_USSD ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::DIAL_MODIFIED_TO_USSD));
static_assert(aidl::android::hardware::radio::LastCallFailCause::DIAL_MODIFIED_TO_SS ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::DIAL_MODIFIED_TO_SS));
static_assert(aidl::android::hardware::radio::LastCallFailCause::DIAL_MODIFIED_TO_DIAL ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::DIAL_MODIFIED_TO_DIAL));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_OFF ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_OFF));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OUT_OF_SERVICE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OUT_OF_SERVICE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NO_VALID_SIM ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NO_VALID_SIM));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_INTERNAL_ERROR ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_INTERNAL_ERROR));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NETWORK_RESP_TIMEOUT ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NETWORK_RESP_TIMEOUT));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NETWORK_REJECT ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NETWORK_REJECT));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_ACCESS_FAILURE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_ACCESS_FAILURE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_LINK_FAILURE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_LINK_FAILURE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_LINK_LOST ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_LINK_LOST));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_UPLINK_FAILURE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_UPLINK_FAILURE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_SETUP_FAILURE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_SETUP_FAILURE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_RELEASE_NORMAL ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_RELEASE_NORMAL));
static_assert(aidl::android::hardware::radio::LastCallFailCause::RADIO_RELEASE_ABNORMAL ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::RADIO_RELEASE_ABNORMAL));
static_assert(aidl::android::hardware::radio::LastCallFailCause::ACCESS_CLASS_BLOCKED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::ACCESS_CLASS_BLOCKED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::NETWORK_DETACH ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::NETWORK_DETACH));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_LOCKED_UNTIL_POWER_CYCLE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::
                              CDMA_LOCKED_UNTIL_POWER_CYCLE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_DROP ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_DROP));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_INTERCEPT ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_INTERCEPT));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_REORDER ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_REORDER));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_SO_REJECT ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_SO_REJECT));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_RETRY_ORDER ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_RETRY_ORDER));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_ACCESS_FAILURE ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_ACCESS_FAILURE));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_PREEMPTED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_PREEMPTED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_NOT_EMERGENCY ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_NOT_EMERGENCY));
static_assert(aidl::android::hardware::radio::LastCallFailCause::CDMA_ACCESS_BLOCKED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::CDMA_ACCESS_BLOCKED));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_1 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_1));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_2 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_2));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_3 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_3));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_4 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_4));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_5 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_5));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_6 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_6));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_7 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_7));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_8 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_8));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_9 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_9));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_10 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_10));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_11 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_11));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_12 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_12));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_13 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_13));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_14 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_14));
static_assert(aidl::android::hardware::radio::LastCallFailCause::OEM_CAUSE_15 ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::OEM_CAUSE_15));
static_assert(aidl::android::hardware::radio::LastCallFailCause::ERROR_UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::LastCallFailCause>(
                      ::android::hardware::radio::V1_0::LastCallFailCause::ERROR_UNSPECIFIED));

static_assert(aidl::android::hardware::radio::HardwareConfigType::MODEM ==
              static_cast<aidl::android::hardware::radio::HardwareConfigType>(
                      ::android::hardware::radio::V1_0::HardwareConfigType::MODEM));
static_assert(aidl::android::hardware::radio::HardwareConfigType::SIM ==
              static_cast<aidl::android::hardware::radio::HardwareConfigType>(
                      ::android::hardware::radio::V1_0::HardwareConfigType::SIM));

static_assert(aidl::android::hardware::radio::RegState::NOT_REG_MT_NOT_SEARCHING_OP ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::NOT_REG_MT_NOT_SEARCHING_OP));
static_assert(aidl::android::hardware::radio::RegState::REG_HOME ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::REG_HOME));
static_assert(aidl::android::hardware::radio::RegState::NOT_REG_MT_SEARCHING_OP ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::NOT_REG_MT_SEARCHING_OP));
static_assert(aidl::android::hardware::radio::RegState::REG_DENIED ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::REG_DENIED));
static_assert(aidl::android::hardware::radio::RegState::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::UNKNOWN));
static_assert(aidl::android::hardware::radio::RegState::REG_ROAMING ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::REG_ROAMING));
static_assert(aidl::android::hardware::radio::RegState::NOT_REG_MT_NOT_SEARCHING_OP_EM ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::NOT_REG_MT_NOT_SEARCHING_OP_EM));
static_assert(aidl::android::hardware::radio::RegState::NOT_REG_MT_SEARCHING_OP_EM ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::NOT_REG_MT_SEARCHING_OP_EM));
static_assert(aidl::android::hardware::radio::RegState::REG_DENIED_EM ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::REG_DENIED_EM));
static_assert(aidl::android::hardware::radio::RegState::UNKNOWN_EM ==
              static_cast<aidl::android::hardware::radio::RegState>(
                      ::android::hardware::radio::V1_0::RegState::UNKNOWN_EM));

static_assert(aidl::android::hardware::radio::DataProfileId::DEFAULT ==
              static_cast<aidl::android::hardware::radio::DataProfileId>(
                      ::android::hardware::radio::V1_0::DataProfileId::DEFAULT));
static_assert(aidl::android::hardware::radio::DataProfileId::TETHERED ==
              static_cast<aidl::android::hardware::radio::DataProfileId>(
                      ::android::hardware::radio::V1_0::DataProfileId::TETHERED));
static_assert(aidl::android::hardware::radio::DataProfileId::IMS ==
              static_cast<aidl::android::hardware::radio::DataProfileId>(
                      ::android::hardware::radio::V1_0::DataProfileId::IMS));
static_assert(aidl::android::hardware::radio::DataProfileId::FOTA ==
              static_cast<aidl::android::hardware::radio::DataProfileId>(
                      ::android::hardware::radio::V1_0::DataProfileId::FOTA));
static_assert(aidl::android::hardware::radio::DataProfileId::CBS ==
              static_cast<aidl::android::hardware::radio::DataProfileId>(
                      ::android::hardware::radio::V1_0::DataProfileId::CBS));
static_assert(aidl::android::hardware::radio::DataProfileId::OEM_BASE ==
              static_cast<aidl::android::hardware::radio::DataProfileId>(
                      ::android::hardware::radio::V1_0::DataProfileId::OEM_BASE));
static_assert(aidl::android::hardware::radio::DataProfileId::INVALID ==
              static_cast<aidl::android::hardware::radio::DataProfileId>(
                      ::android::hardware::radio::V1_0::DataProfileId::INVALID));

static_assert(aidl::android::hardware::radio::SmsAcknowledgeFailCause::MEMORY_CAPACITY_EXCEEDED ==
              static_cast<aidl::android::hardware::radio::SmsAcknowledgeFailCause>(
                      ::android::hardware::radio::V1_0::SmsAcknowledgeFailCause::
                              MEMORY_CAPACITY_EXCEEDED));
static_assert(
        aidl::android::hardware::radio::SmsAcknowledgeFailCause::UNSPECIFIED_ERROR ==
        static_cast<aidl::android::hardware::radio::SmsAcknowledgeFailCause>(
                ::android::hardware::radio::V1_0::SmsAcknowledgeFailCause::UNSPECIFIED_ERROR));

static_assert(aidl::android::hardware::radio::CallForwardInfoStatus::DISABLE ==
              static_cast<aidl::android::hardware::radio::CallForwardInfoStatus>(
                      ::android::hardware::radio::V1_0::CallForwardInfoStatus::DISABLE));
static_assert(aidl::android::hardware::radio::CallForwardInfoStatus::ENABLE ==
              static_cast<aidl::android::hardware::radio::CallForwardInfoStatus>(
                      ::android::hardware::radio::V1_0::CallForwardInfoStatus::ENABLE));
static_assert(aidl::android::hardware::radio::CallForwardInfoStatus::INTERROGATE ==
              static_cast<aidl::android::hardware::radio::CallForwardInfoStatus>(
                      ::android::hardware::radio::V1_0::CallForwardInfoStatus::INTERROGATE));
static_assert(aidl::android::hardware::radio::CallForwardInfoStatus::REGISTRATION ==
              static_cast<aidl::android::hardware::radio::CallForwardInfoStatus>(
                      ::android::hardware::radio::V1_0::CallForwardInfoStatus::REGISTRATION));
static_assert(aidl::android::hardware::radio::CallForwardInfoStatus::ERASURE ==
              static_cast<aidl::android::hardware::radio::CallForwardInfoStatus>(
                      ::android::hardware::radio::V1_0::CallForwardInfoStatus::ERASURE));

static_assert(aidl::android::hardware::radio::ClipStatus::CLIP_PROVISIONED ==
              static_cast<aidl::android::hardware::radio::ClipStatus>(
                      ::android::hardware::radio::V1_0::ClipStatus::CLIP_PROVISIONED));
static_assert(aidl::android::hardware::radio::ClipStatus::CLIP_UNPROVISIONED ==
              static_cast<aidl::android::hardware::radio::ClipStatus>(
                      ::android::hardware::radio::V1_0::ClipStatus::CLIP_UNPROVISIONED));
static_assert(aidl::android::hardware::radio::ClipStatus::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::ClipStatus>(
                      ::android::hardware::radio::V1_0::ClipStatus::UNKNOWN));

static_assert(aidl::android::hardware::radio::SmsWriteArgsStatus::REC_UNREAD ==
              static_cast<aidl::android::hardware::radio::SmsWriteArgsStatus>(
                      ::android::hardware::radio::V1_0::SmsWriteArgsStatus::REC_UNREAD));
static_assert(aidl::android::hardware::radio::SmsWriteArgsStatus::REC_READ ==
              static_cast<aidl::android::hardware::radio::SmsWriteArgsStatus>(
                      ::android::hardware::radio::V1_0::SmsWriteArgsStatus::REC_READ));
static_assert(aidl::android::hardware::radio::SmsWriteArgsStatus::STO_UNSENT ==
              static_cast<aidl::android::hardware::radio::SmsWriteArgsStatus>(
                      ::android::hardware::radio::V1_0::SmsWriteArgsStatus::STO_UNSENT));
static_assert(aidl::android::hardware::radio::SmsWriteArgsStatus::STO_SENT ==
              static_cast<aidl::android::hardware::radio::SmsWriteArgsStatus>(
                      ::android::hardware::radio::V1_0::SmsWriteArgsStatus::STO_SENT));

static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_UNSPECIFIED));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_EURO ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_EURO));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_USA ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_USA));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_JPN ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_JPN));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_AUS ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_AUS));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_AUS_2 ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_AUS_2));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_CELL_800 ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_CELL_800));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_PCS ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_PCS));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_JTACS ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_JTACS));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_KOREA_PCS ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_KOREA_PCS));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_5_450M ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_5_450M));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_IMT2000 ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_IMT2000));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_7_700M_2 ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_7_700M_2));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_8_1800M ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_8_1800M));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_9_900M ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_9_900M));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_10_800M_2 ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_10_800M_2));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_EURO_PAMR_400M ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_EURO_PAMR_400M));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_AWS ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_AWS));
static_assert(aidl::android::hardware::radio::RadioBandMode::BAND_MODE_USA_2500M ==
              static_cast<aidl::android::hardware::radio::RadioBandMode>(
                      ::android::hardware::radio::V1_0::RadioBandMode::BAND_MODE_USA_2500M));

static_assert(aidl::android::hardware::radio::OperatorStatus::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::OperatorStatus>(
                      ::android::hardware::radio::V1_0::OperatorStatus::UNKNOWN));
static_assert(aidl::android::hardware::radio::OperatorStatus::AVAILABLE ==
              static_cast<aidl::android::hardware::radio::OperatorStatus>(
                      ::android::hardware::radio::V1_0::OperatorStatus::AVAILABLE));
static_assert(aidl::android::hardware::radio::OperatorStatus::CURRENT ==
              static_cast<aidl::android::hardware::radio::OperatorStatus>(
                      ::android::hardware::radio::V1_0::OperatorStatus::CURRENT));
static_assert(aidl::android::hardware::radio::OperatorStatus::FORBIDDEN ==
              static_cast<aidl::android::hardware::radio::OperatorStatus>(
                      ::android::hardware::radio::V1_0::OperatorStatus::FORBIDDEN));

static_assert(aidl::android::hardware::radio::PreferredNetworkType::GSM_WCDMA ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::GSM_WCDMA));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::GSM_ONLY ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::GSM_ONLY));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::WCDMA ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::WCDMA));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::GSM_WCDMA_AUTO ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::GSM_WCDMA_AUTO));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::CDMA_EVDO_AUTO ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::CDMA_EVDO_AUTO));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::CDMA_ONLY ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::CDMA_ONLY));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::EVDO_ONLY ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::EVDO_ONLY));
static_assert(
        aidl::android::hardware::radio::PreferredNetworkType::GSM_WCDMA_CDMA_EVDO_AUTO ==
        static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                ::android::hardware::radio::V1_0::PreferredNetworkType::GSM_WCDMA_CDMA_EVDO_AUTO));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::LTE_CDMA_EVDO ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::LTE_CDMA_EVDO));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::LTE_GSM_WCDMA ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::LTE_GSM_WCDMA));
static_assert(
        aidl::android::hardware::radio::PreferredNetworkType::LTE_CMDA_EVDO_GSM_WCDMA ==
        static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                ::android::hardware::radio::V1_0::PreferredNetworkType::LTE_CMDA_EVDO_GSM_WCDMA));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::LTE_ONLY ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::LTE_ONLY));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::LTE_WCDMA ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::LTE_WCDMA));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_ONLY ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::TD_SCDMA_ONLY));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_WCDMA ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::TD_SCDMA_WCDMA));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_LTE ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::TD_SCDMA_LTE));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_GSM ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::TD_SCDMA_GSM));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_GSM_LTE ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::TD_SCDMA_GSM_LTE));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_GSM_WCDMA ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::TD_SCDMA_GSM_WCDMA));
static_assert(aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_WCDMA_LTE ==
              static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                      ::android::hardware::radio::V1_0::PreferredNetworkType::TD_SCDMA_WCDMA_LTE));
static_assert(
        aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_GSM_WCDMA_LTE ==
        static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                ::android::hardware::radio::V1_0::PreferredNetworkType::TD_SCDMA_GSM_WCDMA_LTE));
static_assert(
        aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO ==
        static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                ::android::hardware::radio::V1_0::PreferredNetworkType::
                        TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO));
static_assert(
        aidl::android::hardware::radio::PreferredNetworkType::TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA ==
        static_cast<aidl::android::hardware::radio::PreferredNetworkType>(
                ::android::hardware::radio::V1_0::PreferredNetworkType::
                        TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA));

static_assert(aidl::android::hardware::radio::CdmaSubscriptionSource::RUIM_SIM ==
              static_cast<aidl::android::hardware::radio::CdmaSubscriptionSource>(
                      ::android::hardware::radio::V1_0::CdmaSubscriptionSource::RUIM_SIM));
static_assert(aidl::android::hardware::radio::CdmaSubscriptionSource::NV ==
              static_cast<aidl::android::hardware::radio::CdmaSubscriptionSource>(
                      ::android::hardware::radio::V1_0::CdmaSubscriptionSource::NV));

static_assert(aidl::android::hardware::radio::CdmaRoamingType::HOME_NETWORK ==
              static_cast<aidl::android::hardware::radio::CdmaRoamingType>(
                      ::android::hardware::radio::V1_0::CdmaRoamingType::HOME_NETWORK));
static_assert(aidl::android::hardware::radio::CdmaRoamingType::AFFILIATED_ROAM ==
              static_cast<aidl::android::hardware::radio::CdmaRoamingType>(
                      ::android::hardware::radio::V1_0::CdmaRoamingType::AFFILIATED_ROAM));
static_assert(aidl::android::hardware::radio::CdmaRoamingType::ANY_ROAM ==
              static_cast<aidl::android::hardware::radio::CdmaRoamingType>(
                      ::android::hardware::radio::V1_0::CdmaRoamingType::ANY_ROAM));

static_assert(aidl::android::hardware::radio::TtyMode::OFF ==
              static_cast<aidl::android::hardware::radio::TtyMode>(
                      ::android::hardware::radio::V1_0::TtyMode::OFF));
static_assert(aidl::android::hardware::radio::TtyMode::FULL ==
              static_cast<aidl::android::hardware::radio::TtyMode>(
                      ::android::hardware::radio::V1_0::TtyMode::FULL));
static_assert(aidl::android::hardware::radio::TtyMode::HCO ==
              static_cast<aidl::android::hardware::radio::TtyMode>(
                      ::android::hardware::radio::V1_0::TtyMode::HCO));
static_assert(aidl::android::hardware::radio::TtyMode::VCO ==
              static_cast<aidl::android::hardware::radio::TtyMode>(
                      ::android::hardware::radio::V1_0::TtyMode::VCO));

static_assert(aidl::android::hardware::radio::NvItem::CDMA_MEID ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_MEID));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_MIN ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_MIN));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_MDN ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_MDN));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_ACCOLC ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_ACCOLC));
static_assert(aidl::android::hardware::radio::NvItem::DEVICE_MSL ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::DEVICE_MSL));
static_assert(aidl::android::hardware::radio::NvItem::RTN_RECONDITIONED_STATUS ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::RTN_RECONDITIONED_STATUS));
static_assert(aidl::android::hardware::radio::NvItem::RTN_ACTIVATION_DATE ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::RTN_ACTIVATION_DATE));
static_assert(aidl::android::hardware::radio::NvItem::RTN_LIFE_TIMER ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::RTN_LIFE_TIMER));
static_assert(aidl::android::hardware::radio::NvItem::RTN_LIFE_CALLS ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::RTN_LIFE_CALLS));
static_assert(aidl::android::hardware::radio::NvItem::RTN_LIFE_DATA_TX ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::RTN_LIFE_DATA_TX));
static_assert(aidl::android::hardware::radio::NvItem::RTN_LIFE_DATA_RX ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::RTN_LIFE_DATA_RX));
static_assert(aidl::android::hardware::radio::NvItem::OMADM_HFA_LEVEL ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::OMADM_HFA_LEVEL));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_NAI ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_NAI));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_HOME_ADDRESS ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_HOME_ADDRESS));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_AAA_AUTH ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_AAA_AUTH));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_HA_AUTH ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_HA_AUTH));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_PRI_HA_ADDR ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_PRI_HA_ADDR));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_SEC_HA_ADDR ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_SEC_HA_ADDR));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_REV_TUN_PREF ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_REV_TUN_PREF));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_HA_SPI ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_HA_SPI));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_AAA_SPI ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_AAA_SPI));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_MN_HA_SS ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_MN_HA_SS));
static_assert(aidl::android::hardware::radio::NvItem::MIP_PROFILE_MN_AAA_SS ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::MIP_PROFILE_MN_AAA_SS));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_PRL_VERSION ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_PRL_VERSION));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_BC10 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_BC10));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_BC14 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_BC14));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_SO68 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_SO68));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_SO73_COP0 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_SO73_COP0));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_SO73_COP1TO7 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_SO73_COP1TO7));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_1X_ADVANCED_ENABLED ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_1X_ADVANCED_ENABLED));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_EHRPD_ENABLED ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_EHRPD_ENABLED));
static_assert(aidl::android::hardware::radio::NvItem::CDMA_EHRPD_FORCED ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::CDMA_EHRPD_FORCED));
static_assert(aidl::android::hardware::radio::NvItem::LTE_BAND_ENABLE_25 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_BAND_ENABLE_25));
static_assert(aidl::android::hardware::radio::NvItem::LTE_BAND_ENABLE_26 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_BAND_ENABLE_26));
static_assert(aidl::android::hardware::radio::NvItem::LTE_BAND_ENABLE_41 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_BAND_ENABLE_41));
static_assert(aidl::android::hardware::radio::NvItem::LTE_SCAN_PRIORITY_25 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_SCAN_PRIORITY_25));
static_assert(aidl::android::hardware::radio::NvItem::LTE_SCAN_PRIORITY_26 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_SCAN_PRIORITY_26));
static_assert(aidl::android::hardware::radio::NvItem::LTE_SCAN_PRIORITY_41 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_SCAN_PRIORITY_41));
static_assert(aidl::android::hardware::radio::NvItem::LTE_HIDDEN_BAND_PRIORITY_25 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_HIDDEN_BAND_PRIORITY_25));
static_assert(aidl::android::hardware::radio::NvItem::LTE_HIDDEN_BAND_PRIORITY_26 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_HIDDEN_BAND_PRIORITY_26));
static_assert(aidl::android::hardware::radio::NvItem::LTE_HIDDEN_BAND_PRIORITY_41 ==
              static_cast<aidl::android::hardware::radio::NvItem>(
                      ::android::hardware::radio::V1_0::NvItem::LTE_HIDDEN_BAND_PRIORITY_41));

static_assert(aidl::android::hardware::radio::ResetNvType::RELOAD ==
              static_cast<aidl::android::hardware::radio::ResetNvType>(
                      ::android::hardware::radio::V1_0::ResetNvType::RELOAD));
static_assert(aidl::android::hardware::radio::ResetNvType::ERASE ==
              static_cast<aidl::android::hardware::radio::ResetNvType>(
                      ::android::hardware::radio::V1_0::ResetNvType::ERASE));
static_assert(aidl::android::hardware::radio::ResetNvType::FACTORY_RESET ==
              static_cast<aidl::android::hardware::radio::ResetNvType>(
                      ::android::hardware::radio::V1_0::ResetNvType::FACTORY_RESET));

static_assert(aidl::android::hardware::radio::HardwareConfigState::ENABLED ==
              static_cast<aidl::android::hardware::radio::HardwareConfigState>(
                      ::android::hardware::radio::V1_0::HardwareConfigState::ENABLED));
static_assert(aidl::android::hardware::radio::HardwareConfigState::STANDBY ==
              static_cast<aidl::android::hardware::radio::HardwareConfigState>(
                      ::android::hardware::radio::V1_0::HardwareConfigState::STANDBY));
static_assert(aidl::android::hardware::radio::HardwareConfigState::DISABLED ==
              static_cast<aidl::android::hardware::radio::HardwareConfigState>(
                      ::android::hardware::radio::V1_0::HardwareConfigState::DISABLED));

static_assert(aidl::android::hardware::radio::LceStatus::NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::LceStatus>(
                      ::android::hardware::radio::V1_0::LceStatus::NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::LceStatus::STOPPED ==
              static_cast<aidl::android::hardware::radio::LceStatus>(
                      ::android::hardware::radio::V1_0::LceStatus::STOPPED));
static_assert(aidl::android::hardware::radio::LceStatus::ACTIVE ==
              static_cast<aidl::android::hardware::radio::LceStatus>(
                      ::android::hardware::radio::V1_0::LceStatus::ACTIVE));

static_assert(aidl::android::hardware::radio::CarrierMatchType::ALL ==
              static_cast<aidl::android::hardware::radio::CarrierMatchType>(
                      ::android::hardware::radio::V1_0::CarrierMatchType::ALL));
static_assert(aidl::android::hardware::radio::CarrierMatchType::SPN ==
              static_cast<aidl::android::hardware::radio::CarrierMatchType>(
                      ::android::hardware::radio::V1_0::CarrierMatchType::SPN));
static_assert(aidl::android::hardware::radio::CarrierMatchType::IMSI_PREFIX ==
              static_cast<aidl::android::hardware::radio::CarrierMatchType>(
                      ::android::hardware::radio::V1_0::CarrierMatchType::IMSI_PREFIX));
static_assert(aidl::android::hardware::radio::CarrierMatchType::GID1 ==
              static_cast<aidl::android::hardware::radio::CarrierMatchType>(
                      ::android::hardware::radio::V1_0::CarrierMatchType::GID1));
static_assert(aidl::android::hardware::radio::CarrierMatchType::GID2 ==
              static_cast<aidl::android::hardware::radio::CarrierMatchType>(
                      ::android::hardware::radio::V1_0::CarrierMatchType::GID2));

static_assert(aidl::android::hardware::radio::CdmaSmsDigitMode::FOUR_BIT ==
              static_cast<aidl::android::hardware::radio::CdmaSmsDigitMode>(
                      ::android::hardware::radio::V1_0::CdmaSmsDigitMode::FOUR_BIT));
static_assert(aidl::android::hardware::radio::CdmaSmsDigitMode::EIGHT_BIT ==
              static_cast<aidl::android::hardware::radio::CdmaSmsDigitMode>(
                      ::android::hardware::radio::V1_0::CdmaSmsDigitMode::EIGHT_BIT));

static_assert(aidl::android::hardware::radio::CdmaSmsNumberMode::NOT_DATA_NETWORK ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberMode>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberMode::NOT_DATA_NETWORK));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberMode::DATA_NETWORK ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberMode>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberMode::DATA_NETWORK));

static_assert(aidl::android::hardware::radio::CdmaSmsNumberType::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberType::UNKNOWN));
static_assert(
        aidl::android::hardware::radio::CdmaSmsNumberType::INTERNATIONAL_OR_DATA_IP ==
        static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(
                ::android::hardware::radio::V1_0::CdmaSmsNumberType::INTERNATIONAL_OR_DATA_IP));
static_assert(
        aidl::android::hardware::radio::CdmaSmsNumberType::NATIONAL_OR_INTERNET_MAIL ==
        static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(
                ::android::hardware::radio::V1_0::CdmaSmsNumberType::NATIONAL_OR_INTERNET_MAIL));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberType::NETWORK ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberType::NETWORK));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberType::SUBSCRIBER ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberType::SUBSCRIBER));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberType::ALPHANUMERIC ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberType::ALPHANUMERIC));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberType::ABBREVIATED ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberType::ABBREVIATED));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberType::RESERVED_7 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberType::RESERVED_7));

static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::UNKNOWN));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::TELEPHONY ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::TELEPHONY));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_2 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_2));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::DATA ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::DATA));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::TELEX ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::TELEX));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_5 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_5));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_6 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_6));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_7 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_7));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_8 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_8));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::PRIVATE ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::PRIVATE));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_10 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_10));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_11 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_11));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_12 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_12));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_13 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_13));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_14 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_14));
static_assert(aidl::android::hardware::radio::CdmaSmsNumberPlan::RESERVED_15 ==
              static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaSmsNumberPlan::RESERVED_15));

static_assert(aidl::android::hardware::radio::CdmaSmsSubaddressType::NSAP ==
              static_cast<aidl::android::hardware::radio::CdmaSmsSubaddressType>(
                      ::android::hardware::radio::V1_0::CdmaSmsSubaddressType::NSAP));
static_assert(aidl::android::hardware::radio::CdmaSmsSubaddressType::USER_SPECIFIED ==
              static_cast<aidl::android::hardware::radio::CdmaSmsSubaddressType>(
                      ::android::hardware::radio::V1_0::CdmaSmsSubaddressType::USER_SPECIFIED));

static_assert(aidl::android::hardware::radio::CdmaSmsErrorClass::NO_ERROR ==
              static_cast<aidl::android::hardware::radio::CdmaSmsErrorClass>(
                      ::android::hardware::radio::V1_0::CdmaSmsErrorClass::NO_ERROR));
static_assert(aidl::android::hardware::radio::CdmaSmsErrorClass::ERROR ==
              static_cast<aidl::android::hardware::radio::CdmaSmsErrorClass>(
                      ::android::hardware::radio::V1_0::CdmaSmsErrorClass::ERROR));

static_assert(aidl::android::hardware::radio::CdmaSmsWriteArgsStatus::REC_UNREAD ==
              static_cast<aidl::android::hardware::radio::CdmaSmsWriteArgsStatus>(
                      ::android::hardware::radio::V1_0::CdmaSmsWriteArgsStatus::REC_UNREAD));
static_assert(aidl::android::hardware::radio::CdmaSmsWriteArgsStatus::REC_READ ==
              static_cast<aidl::android::hardware::radio::CdmaSmsWriteArgsStatus>(
                      ::android::hardware::radio::V1_0::CdmaSmsWriteArgsStatus::REC_READ));
static_assert(aidl::android::hardware::radio::CdmaSmsWriteArgsStatus::STO_UNSENT ==
              static_cast<aidl::android::hardware::radio::CdmaSmsWriteArgsStatus>(
                      ::android::hardware::radio::V1_0::CdmaSmsWriteArgsStatus::STO_UNSENT));
static_assert(aidl::android::hardware::radio::CdmaSmsWriteArgsStatus::STO_SENT ==
              static_cast<aidl::android::hardware::radio::CdmaSmsWriteArgsStatus>(
                      ::android::hardware::radio::V1_0::CdmaSmsWriteArgsStatus::STO_SENT));

static_assert(aidl::android::hardware::radio::CellInfoType::NONE ==
              static_cast<aidl::android::hardware::radio::CellInfoType>(
                      ::android::hardware::radio::V1_0::CellInfoType::NONE));
static_assert(aidl::android::hardware::radio::CellInfoType::GSM ==
              static_cast<aidl::android::hardware::radio::CellInfoType>(
                      ::android::hardware::radio::V1_0::CellInfoType::GSM));
static_assert(aidl::android::hardware::radio::CellInfoType::CDMA ==
              static_cast<aidl::android::hardware::radio::CellInfoType>(
                      ::android::hardware::radio::V1_0::CellInfoType::CDMA));
static_assert(aidl::android::hardware::radio::CellInfoType::LTE ==
              static_cast<aidl::android::hardware::radio::CellInfoType>(
                      ::android::hardware::radio::V1_0::CellInfoType::LTE));
static_assert(aidl::android::hardware::radio::CellInfoType::WCDMA ==
              static_cast<aidl::android::hardware::radio::CellInfoType>(
                      ::android::hardware::radio::V1_0::CellInfoType::WCDMA));
static_assert(aidl::android::hardware::radio::CellInfoType::TD_SCDMA ==
              static_cast<aidl::android::hardware::radio::CellInfoType>(
                      ::android::hardware::radio::V1_0::CellInfoType::TD_SCDMA));

static_assert(aidl::android::hardware::radio::TimeStampType::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::TimeStampType>(
                      ::android::hardware::radio::V1_0::TimeStampType::UNKNOWN));
static_assert(aidl::android::hardware::radio::TimeStampType::ANTENNA ==
              static_cast<aidl::android::hardware::radio::TimeStampType>(
                      ::android::hardware::radio::V1_0::TimeStampType::ANTENNA));
static_assert(aidl::android::hardware::radio::TimeStampType::MODEM ==
              static_cast<aidl::android::hardware::radio::TimeStampType>(
                      ::android::hardware::radio::V1_0::TimeStampType::MODEM));
static_assert(aidl::android::hardware::radio::TimeStampType::OEM_RIL ==
              static_cast<aidl::android::hardware::radio::TimeStampType>(
                      ::android::hardware::radio::V1_0::TimeStampType::OEM_RIL));
static_assert(aidl::android::hardware::radio::TimeStampType::JAVA_RIL ==
              static_cast<aidl::android::hardware::radio::TimeStampType>(
                      ::android::hardware::radio::V1_0::TimeStampType::JAVA_RIL));

static_assert(aidl::android::hardware::radio::ApnAuthType::NO_PAP_NO_CHAP ==
              static_cast<aidl::android::hardware::radio::ApnAuthType>(
                      ::android::hardware::radio::V1_0::ApnAuthType::NO_PAP_NO_CHAP));
static_assert(aidl::android::hardware::radio::ApnAuthType::PAP_NO_CHAP ==
              static_cast<aidl::android::hardware::radio::ApnAuthType>(
                      ::android::hardware::radio::V1_0::ApnAuthType::PAP_NO_CHAP));
static_assert(aidl::android::hardware::radio::ApnAuthType::NO_PAP_CHAP ==
              static_cast<aidl::android::hardware::radio::ApnAuthType>(
                      ::android::hardware::radio::V1_0::ApnAuthType::NO_PAP_CHAP));
static_assert(aidl::android::hardware::radio::ApnAuthType::PAP_CHAP ==
              static_cast<aidl::android::hardware::radio::ApnAuthType>(
                      ::android::hardware::radio::V1_0::ApnAuthType::PAP_CHAP));

static_assert(aidl::android::hardware::radio::RadioTechnologyFamily::THREE_GPP ==
              static_cast<aidl::android::hardware::radio::RadioTechnologyFamily>(
                      ::android::hardware::radio::V1_0::RadioTechnologyFamily::THREE_GPP));
static_assert(aidl::android::hardware::radio::RadioTechnologyFamily::THREE_GPP2 ==
              static_cast<aidl::android::hardware::radio::RadioTechnologyFamily>(
                      ::android::hardware::radio::V1_0::RadioTechnologyFamily::THREE_GPP2));

static_assert(aidl::android::hardware::radio::RadioCapabilityPhase::CONFIGURED ==
              static_cast<aidl::android::hardware::radio::RadioCapabilityPhase>(
                      ::android::hardware::radio::V1_0::RadioCapabilityPhase::CONFIGURED));
static_assert(aidl::android::hardware::radio::RadioCapabilityPhase::START ==
              static_cast<aidl::android::hardware::radio::RadioCapabilityPhase>(
                      ::android::hardware::radio::V1_0::RadioCapabilityPhase::START));
static_assert(aidl::android::hardware::radio::RadioCapabilityPhase::APPLY ==
              static_cast<aidl::android::hardware::radio::RadioCapabilityPhase>(
                      ::android::hardware::radio::V1_0::RadioCapabilityPhase::APPLY));
static_assert(aidl::android::hardware::radio::RadioCapabilityPhase::UNSOL_RSP ==
              static_cast<aidl::android::hardware::radio::RadioCapabilityPhase>(
                      ::android::hardware::radio::V1_0::RadioCapabilityPhase::UNSOL_RSP));
static_assert(aidl::android::hardware::radio::RadioCapabilityPhase::FINISH ==
              static_cast<aidl::android::hardware::radio::RadioCapabilityPhase>(
                      ::android::hardware::radio::V1_0::RadioCapabilityPhase::FINISH));

static_assert(aidl::android::hardware::radio::RadioCapabilityStatus::NONE ==
              static_cast<aidl::android::hardware::radio::RadioCapabilityStatus>(
                      ::android::hardware::radio::V1_0::RadioCapabilityStatus::NONE));
static_assert(aidl::android::hardware::radio::RadioCapabilityStatus::SUCCESS ==
              static_cast<aidl::android::hardware::radio::RadioCapabilityStatus>(
                      ::android::hardware::radio::V1_0::RadioCapabilityStatus::SUCCESS));
static_assert(aidl::android::hardware::radio::RadioCapabilityStatus::FAIL ==
              static_cast<aidl::android::hardware::radio::RadioCapabilityStatus>(
                      ::android::hardware::radio::V1_0::RadioCapabilityStatus::FAIL));

static_assert(aidl::android::hardware::radio::UssdModeType::NOTIFY ==
              static_cast<aidl::android::hardware::radio::UssdModeType>(
                      ::android::hardware::radio::V1_0::UssdModeType::NOTIFY));
static_assert(aidl::android::hardware::radio::UssdModeType::REQUEST ==
              static_cast<aidl::android::hardware::radio::UssdModeType>(
                      ::android::hardware::radio::V1_0::UssdModeType::REQUEST));
static_assert(aidl::android::hardware::radio::UssdModeType::NW_RELEASE ==
              static_cast<aidl::android::hardware::radio::UssdModeType>(
                      ::android::hardware::radio::V1_0::UssdModeType::NW_RELEASE));
static_assert(aidl::android::hardware::radio::UssdModeType::LOCAL_CLIENT ==
              static_cast<aidl::android::hardware::radio::UssdModeType>(
                      ::android::hardware::radio::V1_0::UssdModeType::LOCAL_CLIENT));
static_assert(aidl::android::hardware::radio::UssdModeType::NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::UssdModeType>(
                      ::android::hardware::radio::V1_0::UssdModeType::NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::UssdModeType::NW_TIMEOUT ==
              static_cast<aidl::android::hardware::radio::UssdModeType>(
                      ::android::hardware::radio::V1_0::UssdModeType::NW_TIMEOUT));

static_assert(aidl::android::hardware::radio::SimRefreshType::SIM_FILE_UPDATE ==
              static_cast<aidl::android::hardware::radio::SimRefreshType>(
                      ::android::hardware::radio::V1_0::SimRefreshType::SIM_FILE_UPDATE));
static_assert(aidl::android::hardware::radio::SimRefreshType::SIM_INIT ==
              static_cast<aidl::android::hardware::radio::SimRefreshType>(
                      ::android::hardware::radio::V1_0::SimRefreshType::SIM_INIT));
static_assert(aidl::android::hardware::radio::SimRefreshType::SIM_RESET ==
              static_cast<aidl::android::hardware::radio::SimRefreshType>(
                      ::android::hardware::radio::V1_0::SimRefreshType::SIM_RESET));

static_assert(aidl::android::hardware::radio::SrvccState::HANDOVER_STARTED ==
              static_cast<aidl::android::hardware::radio::SrvccState>(
                      ::android::hardware::radio::V1_0::SrvccState::HANDOVER_STARTED));
static_assert(aidl::android::hardware::radio::SrvccState::HANDOVER_COMPLETED ==
              static_cast<aidl::android::hardware::radio::SrvccState>(
                      ::android::hardware::radio::V1_0::SrvccState::HANDOVER_COMPLETED));
static_assert(aidl::android::hardware::radio::SrvccState::HANDOVER_FAILED ==
              static_cast<aidl::android::hardware::radio::SrvccState>(
                      ::android::hardware::radio::V1_0::SrvccState::HANDOVER_FAILED));
static_assert(aidl::android::hardware::radio::SrvccState::HANDOVER_CANCELED ==
              static_cast<aidl::android::hardware::radio::SrvccState>(
                      ::android::hardware::radio::V1_0::SrvccState::HANDOVER_CANCELED));

static_assert(aidl::android::hardware::radio::UiccSubActStatus::DEACTIVATE ==
              static_cast<aidl::android::hardware::radio::UiccSubActStatus>(
                      ::android::hardware::radio::V1_0::UiccSubActStatus::DEACTIVATE));
static_assert(aidl::android::hardware::radio::UiccSubActStatus::ACTIVATE ==
              static_cast<aidl::android::hardware::radio::UiccSubActStatus>(
                      ::android::hardware::radio::V1_0::UiccSubActStatus::ACTIVATE));

static_assert(aidl::android::hardware::radio::SubscriptionType::SUBSCRIPTION_1 ==
              static_cast<aidl::android::hardware::radio::SubscriptionType>(
                      ::android::hardware::radio::V1_0::SubscriptionType::SUBSCRIPTION_1));
static_assert(aidl::android::hardware::radio::SubscriptionType::SUBSCRIPTION_2 ==
              static_cast<aidl::android::hardware::radio::SubscriptionType>(
                      ::android::hardware::radio::V1_0::SubscriptionType::SUBSCRIPTION_2));
static_assert(aidl::android::hardware::radio::SubscriptionType::SUBSCRIPTION_3 ==
              static_cast<aidl::android::hardware::radio::SubscriptionType>(
                      ::android::hardware::radio::V1_0::SubscriptionType::SUBSCRIPTION_3));

static_assert(aidl::android::hardware::radio::DataProfileInfoType::COMMON ==
              static_cast<aidl::android::hardware::radio::DataProfileInfoType>(
                      ::android::hardware::radio::V1_0::DataProfileInfoType::COMMON));
static_assert(aidl::android::hardware::radio::DataProfileInfoType::THREE_GPP ==
              static_cast<aidl::android::hardware::radio::DataProfileInfoType>(
                      ::android::hardware::radio::V1_0::DataProfileInfoType::THREE_GPP));
static_assert(aidl::android::hardware::radio::DataProfileInfoType::THREE_GPP2 ==
              static_cast<aidl::android::hardware::radio::DataProfileInfoType>(
                      ::android::hardware::radio::V1_0::DataProfileInfoType::THREE_GPP2));

static_assert(aidl::android::hardware::radio::PhoneRestrictedState::NONE ==
              static_cast<aidl::android::hardware::radio::PhoneRestrictedState>(
                      ::android::hardware::radio::V1_0::PhoneRestrictedState::NONE));
static_assert(aidl::android::hardware::radio::PhoneRestrictedState::CS_EMERGENCY ==
              static_cast<aidl::android::hardware::radio::PhoneRestrictedState>(
                      ::android::hardware::radio::V1_0::PhoneRestrictedState::CS_EMERGENCY));
static_assert(aidl::android::hardware::radio::PhoneRestrictedState::CS_NORMAL ==
              static_cast<aidl::android::hardware::radio::PhoneRestrictedState>(
                      ::android::hardware::radio::V1_0::PhoneRestrictedState::CS_NORMAL));
static_assert(aidl::android::hardware::radio::PhoneRestrictedState::CS_ALL ==
              static_cast<aidl::android::hardware::radio::PhoneRestrictedState>(
                      ::android::hardware::radio::V1_0::PhoneRestrictedState::CS_ALL));
static_assert(aidl::android::hardware::radio::PhoneRestrictedState::PS_ALL ==
              static_cast<aidl::android::hardware::radio::PhoneRestrictedState>(
                      ::android::hardware::radio::V1_0::PhoneRestrictedState::PS_ALL));

static_assert(
        aidl::android::hardware::radio::CdmaCallWaitingNumberPresentation::ALLOWED ==
        static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPresentation>(
                ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPresentation::ALLOWED));
static_assert(
        aidl::android::hardware::radio::CdmaCallWaitingNumberPresentation::RESTRICTED ==
        static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPresentation>(
                ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPresentation::RESTRICTED));
static_assert(
        aidl::android::hardware::radio::CdmaCallWaitingNumberPresentation::UNKNOWN ==
        static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPresentation>(
                ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPresentation::UNKNOWN));

static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberType::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberType>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberType::UNKNOWN));
static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberType::INTERNATIONAL ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberType>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberType::INTERNATIONAL));
static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberType::NATIONAL ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberType>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberType::NATIONAL));
static_assert(
        aidl::android::hardware::radio::CdmaCallWaitingNumberType::NETWORK_SPECIFIC ==
        static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberType>(
                ::android::hardware::radio::V1_0::CdmaCallWaitingNumberType::NETWORK_SPECIFIC));
static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberType::SUBSCRIBER ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberType>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberType::SUBSCRIBER));

static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberPlan::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPlan::UNKNOWN));
static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberPlan::ISDN ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPlan::ISDN));
static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberPlan::DATA ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPlan::DATA));
static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberPlan::TELEX ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPlan::TELEX));
static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberPlan::NATIONAL ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPlan::NATIONAL));
static_assert(aidl::android::hardware::radio::CdmaCallWaitingNumberPlan::PRIVATE ==
              static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPlan>(
                      ::android::hardware::radio::V1_0::CdmaCallWaitingNumberPlan::PRIVATE));

static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::SPL_UNLOCKED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::SPL_UNLOCKED));
static_assert(
        aidl::android::hardware::radio::CdmaOtaProvisionStatus::SPC_RETRIES_EXCEEDED ==
        static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::SPC_RETRIES_EXCEEDED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::A_KEY_EXCHANGED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::A_KEY_EXCHANGED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::SSD_UPDATED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::SSD_UPDATED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::NAM_DOWNLOADED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::NAM_DOWNLOADED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::MDN_DOWNLOADED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::MDN_DOWNLOADED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::IMSI_DOWNLOADED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::IMSI_DOWNLOADED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::PRL_DOWNLOADED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::PRL_DOWNLOADED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::COMMITTED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::COMMITTED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::OTAPA_STARTED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::OTAPA_STARTED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::OTAPA_STOPPED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::OTAPA_STOPPED));
static_assert(aidl::android::hardware::radio::CdmaOtaProvisionStatus::OTAPA_ABORTED ==
              static_cast<aidl::android::hardware::radio::CdmaOtaProvisionStatus>(
                      ::android::hardware::radio::V1_0::CdmaOtaProvisionStatus::OTAPA_ABORTED));

static_assert(aidl::android::hardware::radio::CdmaInfoRecName::DISPLAY ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::DISPLAY));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::CALLED_PARTY_NUMBER ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::CALLED_PARTY_NUMBER));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::CALLING_PARTY_NUMBER ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::CALLING_PARTY_NUMBER));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::CONNECTED_NUMBER ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::CONNECTED_NUMBER));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::SIGNAL ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::SIGNAL));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::REDIRECTING_NUMBER ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::REDIRECTING_NUMBER));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::LINE_CONTROL ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::LINE_CONTROL));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::EXTENDED_DISPLAY ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::EXTENDED_DISPLAY));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::T53_CLIR ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::T53_CLIR));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::T53_RELEASE ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::T53_RELEASE));
static_assert(aidl::android::hardware::radio::CdmaInfoRecName::T53_AUDIO_CONTROL ==
              static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(
                      ::android::hardware::radio::V1_0::CdmaInfoRecName::T53_AUDIO_CONTROL));

static_assert(aidl::android::hardware::radio::CdmaRedirectingReason::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::CdmaRedirectingReason>(
                      ::android::hardware::radio::V1_0::CdmaRedirectingReason::UNKNOWN));
static_assert(
        aidl::android::hardware::radio::CdmaRedirectingReason::CALL_FORWARDING_BUSY ==
        static_cast<aidl::android::hardware::radio::CdmaRedirectingReason>(
                ::android::hardware::radio::V1_0::CdmaRedirectingReason::CALL_FORWARDING_BUSY));
static_assert(
        aidl::android::hardware::radio::CdmaRedirectingReason::CALL_FORWARDING_NO_REPLY ==
        static_cast<aidl::android::hardware::radio::CdmaRedirectingReason>(
                ::android::hardware::radio::V1_0::CdmaRedirectingReason::CALL_FORWARDING_NO_REPLY));
static_assert(
        aidl::android::hardware::radio::CdmaRedirectingReason::CALLED_DTE_OUT_OF_ORDER ==
        static_cast<aidl::android::hardware::radio::CdmaRedirectingReason>(
                ::android::hardware::radio::V1_0::CdmaRedirectingReason::CALLED_DTE_OUT_OF_ORDER));
static_assert(
        aidl::android::hardware::radio::CdmaRedirectingReason::CALL_FORWARDING_BY_THE_CALLED_DTE ==
        static_cast<aidl::android::hardware::radio::CdmaRedirectingReason>(
                ::android::hardware::radio::V1_0::CdmaRedirectingReason::
                        CALL_FORWARDING_BY_THE_CALLED_DTE));
static_assert(
        aidl::android::hardware::radio::CdmaRedirectingReason::CALL_FORWARDING_UNCONDITIONAL ==
        static_cast<aidl::android::hardware::radio::CdmaRedirectingReason>(
                ::android::hardware::radio::V1_0::CdmaRedirectingReason::
                        CALL_FORWARDING_UNCONDITIONAL));
static_assert(aidl::android::hardware::radio::CdmaRedirectingReason::RESERVED ==
              static_cast<aidl::android::hardware::radio::CdmaRedirectingReason>(
                      ::android::hardware::radio::V1_0::CdmaRedirectingReason::RESERVED));

static_assert(aidl::android::hardware::radio::SsServiceType::CFU ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::CFU));
static_assert(aidl::android::hardware::radio::SsServiceType::CF_BUSY ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::CF_BUSY));
static_assert(aidl::android::hardware::radio::SsServiceType::CF_NO_REPLY ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::CF_NO_REPLY));
static_assert(aidl::android::hardware::radio::SsServiceType::CF_NOT_REACHABLE ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::CF_NOT_REACHABLE));
static_assert(aidl::android::hardware::radio::SsServiceType::CF_ALL ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::CF_ALL));
static_assert(aidl::android::hardware::radio::SsServiceType::CF_ALL_CONDITIONAL ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::CF_ALL_CONDITIONAL));
static_assert(aidl::android::hardware::radio::SsServiceType::CLIP ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::CLIP));
static_assert(aidl::android::hardware::radio::SsServiceType::CLIR ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::CLIR));
static_assert(aidl::android::hardware::radio::SsServiceType::COLP ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::COLP));
static_assert(aidl::android::hardware::radio::SsServiceType::COLR ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::COLR));
static_assert(aidl::android::hardware::radio::SsServiceType::WAIT ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::WAIT));
static_assert(aidl::android::hardware::radio::SsServiceType::BAOC ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::BAOC));
static_assert(aidl::android::hardware::radio::SsServiceType::BAOIC ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::BAOIC));
static_assert(aidl::android::hardware::radio::SsServiceType::BAOIC_EXC_HOME ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::BAOIC_EXC_HOME));
static_assert(aidl::android::hardware::radio::SsServiceType::BAIC ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::BAIC));
static_assert(aidl::android::hardware::radio::SsServiceType::BAIC_ROAMING ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::BAIC_ROAMING));
static_assert(aidl::android::hardware::radio::SsServiceType::ALL_BARRING ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::ALL_BARRING));
static_assert(aidl::android::hardware::radio::SsServiceType::OUTGOING_BARRING ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::OUTGOING_BARRING));
static_assert(aidl::android::hardware::radio::SsServiceType::INCOMING_BARRING ==
              static_cast<aidl::android::hardware::radio::SsServiceType>(
                      ::android::hardware::radio::V1_0::SsServiceType::INCOMING_BARRING));

static_assert(aidl::android::hardware::radio::SsRequestType::ACTIVATION ==
              static_cast<aidl::android::hardware::radio::SsRequestType>(
                      ::android::hardware::radio::V1_0::SsRequestType::ACTIVATION));
static_assert(aidl::android::hardware::radio::SsRequestType::DEACTIVATION ==
              static_cast<aidl::android::hardware::radio::SsRequestType>(
                      ::android::hardware::radio::V1_0::SsRequestType::DEACTIVATION));
static_assert(aidl::android::hardware::radio::SsRequestType::INTERROGATION ==
              static_cast<aidl::android::hardware::radio::SsRequestType>(
                      ::android::hardware::radio::V1_0::SsRequestType::INTERROGATION));
static_assert(aidl::android::hardware::radio::SsRequestType::REGISTRATION ==
              static_cast<aidl::android::hardware::radio::SsRequestType>(
                      ::android::hardware::radio::V1_0::SsRequestType::REGISTRATION));
static_assert(aidl::android::hardware::radio::SsRequestType::ERASURE ==
              static_cast<aidl::android::hardware::radio::SsRequestType>(
                      ::android::hardware::radio::V1_0::SsRequestType::ERASURE));

static_assert(
        aidl::android::hardware::radio::SsTeleserviceType::ALL_TELE_AND_BEARER_SERVICES ==
        static_cast<aidl::android::hardware::radio::SsTeleserviceType>(
                ::android::hardware::radio::V1_0::SsTeleserviceType::ALL_TELE_AND_BEARER_SERVICES));
static_assert(aidl::android::hardware::radio::SsTeleserviceType::ALL_TELESEVICES ==
              static_cast<aidl::android::hardware::radio::SsTeleserviceType>(
                      ::android::hardware::radio::V1_0::SsTeleserviceType::ALL_TELESEVICES));
static_assert(aidl::android::hardware::radio::SsTeleserviceType::TELEPHONY ==
              static_cast<aidl::android::hardware::radio::SsTeleserviceType>(
                      ::android::hardware::radio::V1_0::SsTeleserviceType::TELEPHONY));
static_assert(aidl::android::hardware::radio::SsTeleserviceType::ALL_DATA_TELESERVICES ==
              static_cast<aidl::android::hardware::radio::SsTeleserviceType>(
                      ::android::hardware::radio::V1_0::SsTeleserviceType::ALL_DATA_TELESERVICES));
static_assert(aidl::android::hardware::radio::SsTeleserviceType::SMS_SERVICES ==
              static_cast<aidl::android::hardware::radio::SsTeleserviceType>(
                      ::android::hardware::radio::V1_0::SsTeleserviceType::SMS_SERVICES));
static_assert(
        aidl::android::hardware::radio::SsTeleserviceType::ALL_TELESERVICES_EXCEPT_SMS ==
        static_cast<aidl::android::hardware::radio::SsTeleserviceType>(
                ::android::hardware::radio::V1_0::SsTeleserviceType::ALL_TELESERVICES_EXCEPT_SMS));

static_assert(aidl::android::hardware::radio::SuppServiceClass::NONE ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::NONE));
static_assert(aidl::android::hardware::radio::SuppServiceClass::VOICE ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::VOICE));
static_assert(aidl::android::hardware::radio::SuppServiceClass::DATA ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::DATA));
static_assert(aidl::android::hardware::radio::SuppServiceClass::FAX ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::FAX));
static_assert(aidl::android::hardware::radio::SuppServiceClass::SMS ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::SMS));
static_assert(aidl::android::hardware::radio::SuppServiceClass::DATA_SYNC ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::DATA_SYNC));
static_assert(aidl::android::hardware::radio::SuppServiceClass::DATA_ASYNC ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::DATA_ASYNC));
static_assert(aidl::android::hardware::radio::SuppServiceClass::PACKET ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::PACKET));
static_assert(aidl::android::hardware::radio::SuppServiceClass::PAD ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::PAD));
static_assert(aidl::android::hardware::radio::SuppServiceClass::MAX ==
              static_cast<aidl::android::hardware::radio::SuppServiceClass>(
                      ::android::hardware::radio::V1_0::SuppServiceClass::MAX));

static_assert(aidl::android::hardware::radio::MvnoType::NONE ==
              static_cast<aidl::android::hardware::radio::MvnoType>(
                      ::android::hardware::radio::V1_0::MvnoType::NONE));
static_assert(aidl::android::hardware::radio::MvnoType::IMSI ==
              static_cast<aidl::android::hardware::radio::MvnoType>(
                      ::android::hardware::radio::V1_0::MvnoType::IMSI));
static_assert(aidl::android::hardware::radio::MvnoType::GID ==
              static_cast<aidl::android::hardware::radio::MvnoType>(
                      ::android::hardware::radio::V1_0::MvnoType::GID));
static_assert(aidl::android::hardware::radio::MvnoType::SPN ==
              static_cast<aidl::android::hardware::radio::MvnoType>(
                      ::android::hardware::radio::V1_0::MvnoType::SPN));

static_assert(aidl::android::hardware::radio::DeviceStateType::POWER_SAVE_MODE ==
              static_cast<aidl::android::hardware::radio::DeviceStateType>(
                      ::android::hardware::radio::V1_0::DeviceStateType::POWER_SAVE_MODE));
static_assert(aidl::android::hardware::radio::DeviceStateType::CHARGING_STATE ==
              static_cast<aidl::android::hardware::radio::DeviceStateType>(
                      ::android::hardware::radio::V1_0::DeviceStateType::CHARGING_STATE));
static_assert(aidl::android::hardware::radio::DeviceStateType::LOW_DATA_EXPECTED ==
              static_cast<aidl::android::hardware::radio::DeviceStateType>(
                      ::android::hardware::radio::V1_0::DeviceStateType::LOW_DATA_EXPECTED));

static_assert(aidl::android::hardware::radio::P2Constant::NO_P2 ==
              static_cast<aidl::android::hardware::radio::P2Constant>(
                      ::android::hardware::radio::V1_0::P2Constant::NO_P2));

static_assert(aidl::android::hardware::radio::CardPowerState::POWER_DOWN ==
              static_cast<aidl::android::hardware::radio::CardPowerState>(
                      ::android::hardware::radio::V1_1::CardPowerState::POWER_DOWN));
static_assert(aidl::android::hardware::radio::CardPowerState::POWER_UP ==
              static_cast<aidl::android::hardware::radio::CardPowerState>(
                      ::android::hardware::radio::V1_1::CardPowerState::POWER_UP));
static_assert(aidl::android::hardware::radio::CardPowerState::POWER_UP_PASS_THROUGH ==
              static_cast<aidl::android::hardware::radio::CardPowerState>(
                      ::android::hardware::radio::V1_1::CardPowerState::POWER_UP_PASS_THROUGH));

static_assert(aidl::android::hardware::radio::GeranBands::BAND_T380 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_T380));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_T410 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_T410));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_450 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_450));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_480 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_480));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_710 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_710));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_750 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_750));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_T810 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_T810));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_850 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_850));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_P900 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_P900));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_E900 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_E900));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_R900 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_R900));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_DCS1800 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_DCS1800));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_PCS1900 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_PCS1900));
static_assert(aidl::android::hardware::radio::GeranBands::BAND_ER900 ==
              static_cast<aidl::android::hardware::radio::GeranBands>(
                      ::android::hardware::radio::V1_1::GeranBands::BAND_ER900));

static_assert(aidl::android::hardware::radio::ScanType::ONE_SHOT ==
              static_cast<aidl::android::hardware::radio::ScanType>(
                      ::android::hardware::radio::V1_1::ScanType::ONE_SHOT));
static_assert(aidl::android::hardware::radio::ScanType::PERIODIC ==
              static_cast<aidl::android::hardware::radio::ScanType>(
                      ::android::hardware::radio::V1_1::ScanType::PERIODIC));

static_assert(aidl::android::hardware::radio::ScanStatus::PARTIAL ==
              static_cast<aidl::android::hardware::radio::ScanStatus>(
                      ::android::hardware::radio::V1_1::ScanStatus::PARTIAL));
static_assert(aidl::android::hardware::radio::ScanStatus::COMPLETE ==
              static_cast<aidl::android::hardware::radio::ScanStatus>(
                      ::android::hardware::radio::V1_1::ScanStatus::COMPLETE));

static_assert(aidl::android::hardware::radio::KeepaliveType::NATT_IPV4 ==
              static_cast<aidl::android::hardware::radio::KeepaliveType>(
                      ::android::hardware::radio::V1_1::KeepaliveType::NATT_IPV4));
static_assert(aidl::android::hardware::radio::KeepaliveType::NATT_IPV6 ==
              static_cast<aidl::android::hardware::radio::KeepaliveType>(
                      ::android::hardware::radio::V1_1::KeepaliveType::NATT_IPV6));

static_assert(aidl::android::hardware::radio::KeepaliveStatusCode::ACTIVE ==
              static_cast<aidl::android::hardware::radio::KeepaliveStatusCode>(
                      ::android::hardware::radio::V1_1::KeepaliveStatusCode::ACTIVE));
static_assert(aidl::android::hardware::radio::KeepaliveStatusCode::INACTIVE ==
              static_cast<aidl::android::hardware::radio::KeepaliveStatusCode>(
                      ::android::hardware::radio::V1_1::KeepaliveStatusCode::INACTIVE));
static_assert(aidl::android::hardware::radio::KeepaliveStatusCode::PENDING ==
              static_cast<aidl::android::hardware::radio::KeepaliveStatusCode>(
                      ::android::hardware::radio::V1_1::KeepaliveStatusCode::PENDING));

static_assert(aidl::android::hardware::radio::RadioConst::CDMA_ALPHA_INFO_BUFFER_LENGTH ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::CDMA_ALPHA_INFO_BUFFER_LENGTH));
static_assert(
        aidl::android::hardware::radio::RadioConst::CDMA_NUMBER_INFO_BUFFER_LENGTH ==
        static_cast<aidl::android::hardware::radio::RadioConst>(
                ::android::hardware::radio::V1_2::RadioConst::CDMA_NUMBER_INFO_BUFFER_LENGTH));
static_assert(aidl::android::hardware::radio::RadioConst::MAX_RILDS ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::MAX_RILDS));
static_assert(aidl::android::hardware::radio::RadioConst::MAX_SOCKET_NAME_LENGTH ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::MAX_SOCKET_NAME_LENGTH));
static_assert(aidl::android::hardware::radio::RadioConst::MAX_CLIENT_ID_LENGTH ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::MAX_CLIENT_ID_LENGTH));
static_assert(aidl::android::hardware::radio::RadioConst::MAX_DEBUG_SOCKET_NAME_LENGTH ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::MAX_DEBUG_SOCKET_NAME_LENGTH));
static_assert(aidl::android::hardware::radio::RadioConst::MAX_QEMU_PIPE_NAME_LENGTH ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::MAX_QEMU_PIPE_NAME_LENGTH));
static_assert(aidl::android::hardware::radio::RadioConst::MAX_UUID_LENGTH ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::MAX_UUID_LENGTH));
static_assert(aidl::android::hardware::radio::RadioConst::CARD_MAX_APPS ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::CARD_MAX_APPS));
static_assert(aidl::android::hardware::radio::RadioConst::CDMA_MAX_NUMBER_OF_INFO_RECS ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::CDMA_MAX_NUMBER_OF_INFO_RECS));
static_assert(aidl::android::hardware::radio::RadioConst::SS_INFO_MAX ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::SS_INFO_MAX));
static_assert(aidl::android::hardware::radio::RadioConst::NUM_SERVICE_CLASSES ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::NUM_SERVICE_CLASSES));
static_assert(aidl::android::hardware::radio::RadioConst::NUM_TX_POWER_LEVELS ==
              static_cast<aidl::android::hardware::radio::RadioConst>(
                      ::android::hardware::radio::V1_2::RadioConst::NUM_TX_POWER_LEVELS));
static_assert(
        aidl::android::hardware::radio::RadioConst::RADIO_ACCESS_SPECIFIER_MAX_SIZE ==
        static_cast<aidl::android::hardware::radio::RadioConst>(
                ::android::hardware::radio::V1_2::RadioConst::RADIO_ACCESS_SPECIFIER_MAX_SIZE));

static_assert(aidl::android::hardware::radio::ScanIntervalRange::MIN ==
              static_cast<aidl::android::hardware::radio::ScanIntervalRange>(
                      ::android::hardware::radio::V1_2::ScanIntervalRange::MIN));
static_assert(aidl::android::hardware::radio::ScanIntervalRange::MAX ==
              static_cast<aidl::android::hardware::radio::ScanIntervalRange>(
                      ::android::hardware::radio::V1_2::ScanIntervalRange::MAX));

static_assert(aidl::android::hardware::radio::MaxSearchTimeRange::MIN ==
              static_cast<aidl::android::hardware::radio::MaxSearchTimeRange>(
                      ::android::hardware::radio::V1_2::MaxSearchTimeRange::MIN));
static_assert(aidl::android::hardware::radio::MaxSearchTimeRange::MAX ==
              static_cast<aidl::android::hardware::radio::MaxSearchTimeRange>(
                      ::android::hardware::radio::V1_2::MaxSearchTimeRange::MAX));

static_assert(aidl::android::hardware::radio::IncrementalResultsPeriodicityRange::MIN ==
              static_cast<aidl::android::hardware::radio::IncrementalResultsPeriodicityRange>(
                      ::android::hardware::radio::V1_2::IncrementalResultsPeriodicityRange::MIN));
static_assert(aidl::android::hardware::radio::IncrementalResultsPeriodicityRange::MAX ==
              static_cast<aidl::android::hardware::radio::IncrementalResultsPeriodicityRange>(
                      ::android::hardware::radio::V1_2::IncrementalResultsPeriodicityRange::MAX));

static_assert(aidl::android::hardware::radio::CellConnectionStatus::NONE ==
              static_cast<aidl::android::hardware::radio::CellConnectionStatus>(
                      ::android::hardware::radio::V1_2::CellConnectionStatus::NONE));
static_assert(aidl::android::hardware::radio::CellConnectionStatus::PRIMARY_SERVING ==
              static_cast<aidl::android::hardware::radio::CellConnectionStatus>(
                      ::android::hardware::radio::V1_2::CellConnectionStatus::PRIMARY_SERVING));
static_assert(aidl::android::hardware::radio::CellConnectionStatus::SECONDARY_SERVING ==
              static_cast<aidl::android::hardware::radio::CellConnectionStatus>(
                      ::android::hardware::radio::V1_2::CellConnectionStatus::SECONDARY_SERVING));

static_assert(aidl::android::hardware::radio::AudioQuality::UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::UNSPECIFIED));
static_assert(aidl::android::hardware::radio::AudioQuality::AMR ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::AMR));
static_assert(aidl::android::hardware::radio::AudioQuality::AMR_WB ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::AMR_WB));
static_assert(aidl::android::hardware::radio::AudioQuality::GSM_EFR ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::GSM_EFR));
static_assert(aidl::android::hardware::radio::AudioQuality::GSM_FR ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::GSM_FR));
static_assert(aidl::android::hardware::radio::AudioQuality::GSM_HR ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::GSM_HR));
static_assert(aidl::android::hardware::radio::AudioQuality::EVRC ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::EVRC));
static_assert(aidl::android::hardware::radio::AudioQuality::EVRC_B ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::EVRC_B));
static_assert(aidl::android::hardware::radio::AudioQuality::EVRC_WB ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::EVRC_WB));
static_assert(aidl::android::hardware::radio::AudioQuality::EVRC_NW ==
              static_cast<aidl::android::hardware::radio::AudioQuality>(
                      ::android::hardware::radio::V1_2::AudioQuality::EVRC_NW));

static_assert(aidl::android::hardware::radio::DataRequestReason::NORMAL ==
              static_cast<aidl::android::hardware::radio::DataRequestReason>(
                      ::android::hardware::radio::V1_2::DataRequestReason::NORMAL));
static_assert(aidl::android::hardware::radio::DataRequestReason::SHUTDOWN ==
              static_cast<aidl::android::hardware::radio::DataRequestReason>(
                      ::android::hardware::radio::V1_2::DataRequestReason::SHUTDOWN));
static_assert(aidl::android::hardware::radio::DataRequestReason::HANDOVER ==
              static_cast<aidl::android::hardware::radio::DataRequestReason>(
                      ::android::hardware::radio::V1_2::DataRequestReason::HANDOVER));

static_assert(aidl::android::hardware::radio::EmergencyServiceCategory::UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(
                      ::android::hardware::radio::V1_4::EmergencyServiceCategory::UNSPECIFIED));
static_assert(aidl::android::hardware::radio::EmergencyServiceCategory::POLICE ==
              static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(
                      ::android::hardware::radio::V1_4::EmergencyServiceCategory::POLICE));
static_assert(aidl::android::hardware::radio::EmergencyServiceCategory::AMBULANCE ==
              static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(
                      ::android::hardware::radio::V1_4::EmergencyServiceCategory::AMBULANCE));
static_assert(aidl::android::hardware::radio::EmergencyServiceCategory::FIRE_BRIGADE ==
              static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(
                      ::android::hardware::radio::V1_4::EmergencyServiceCategory::FIRE_BRIGADE));
static_assert(aidl::android::hardware::radio::EmergencyServiceCategory::MARINE_GUARD ==
              static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(
                      ::android::hardware::radio::V1_4::EmergencyServiceCategory::MARINE_GUARD));
static_assert(aidl::android::hardware::radio::EmergencyServiceCategory::MOUNTAIN_RESCUE ==
              static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(
                      ::android::hardware::radio::V1_4::EmergencyServiceCategory::MOUNTAIN_RESCUE));
static_assert(aidl::android::hardware::radio::EmergencyServiceCategory::MIEC ==
              static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(
                      ::android::hardware::radio::V1_4::EmergencyServiceCategory::MIEC));
static_assert(aidl::android::hardware::radio::EmergencyServiceCategory::AIEC ==
              static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(
                      ::android::hardware::radio::V1_4::EmergencyServiceCategory::AIEC));

static_assert(aidl::android::hardware::radio::EmergencyNumberSource::NETWORK_SIGNALING ==
              static_cast<aidl::android::hardware::radio::EmergencyNumberSource>(
                      ::android::hardware::radio::V1_4::EmergencyNumberSource::NETWORK_SIGNALING));
static_assert(aidl::android::hardware::radio::EmergencyNumberSource::SIM ==
              static_cast<aidl::android::hardware::radio::EmergencyNumberSource>(
                      ::android::hardware::radio::V1_4::EmergencyNumberSource::SIM));
static_assert(aidl::android::hardware::radio::EmergencyNumberSource::MODEM_CONFIG ==
              static_cast<aidl::android::hardware::radio::EmergencyNumberSource>(
                      ::android::hardware::radio::V1_4::EmergencyNumberSource::MODEM_CONFIG));
static_assert(aidl::android::hardware::radio::EmergencyNumberSource::DEFAULT ==
              static_cast<aidl::android::hardware::radio::EmergencyNumberSource>(
                      ::android::hardware::radio::V1_4::EmergencyNumberSource::DEFAULT));

static_assert(aidl::android::hardware::radio::EmergencyCallRouting::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::EmergencyCallRouting>(
                      ::android::hardware::radio::V1_4::EmergencyCallRouting::UNKNOWN));
static_assert(aidl::android::hardware::radio::EmergencyCallRouting::EMERGENCY ==
              static_cast<aidl::android::hardware::radio::EmergencyCallRouting>(
                      ::android::hardware::radio::V1_4::EmergencyCallRouting::EMERGENCY));
static_assert(aidl::android::hardware::radio::EmergencyCallRouting::NORMAL ==
              static_cast<aidl::android::hardware::radio::EmergencyCallRouting>(
                      ::android::hardware::radio::V1_4::EmergencyCallRouting::NORMAL));

static_assert(aidl::android::hardware::radio::RadioTechnology::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::UNKNOWN));
static_assert(aidl::android::hardware::radio::RadioTechnology::GPRS ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::GPRS));
static_assert(aidl::android::hardware::radio::RadioTechnology::EDGE ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::EDGE));
static_assert(aidl::android::hardware::radio::RadioTechnology::UMTS ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::UMTS));
static_assert(aidl::android::hardware::radio::RadioTechnology::IS95A ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::IS95A));
static_assert(aidl::android::hardware::radio::RadioTechnology::IS95B ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::IS95B));
static_assert(aidl::android::hardware::radio::RadioTechnology::ONE_X_RTT ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::ONE_X_RTT));
static_assert(aidl::android::hardware::radio::RadioTechnology::EVDO_0 ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::EVDO_0));
static_assert(aidl::android::hardware::radio::RadioTechnology::EVDO_A ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::EVDO_A));
static_assert(aidl::android::hardware::radio::RadioTechnology::HSDPA ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::HSDPA));
static_assert(aidl::android::hardware::radio::RadioTechnology::HSUPA ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::HSUPA));
static_assert(aidl::android::hardware::radio::RadioTechnology::HSPA ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::HSPA));
static_assert(aidl::android::hardware::radio::RadioTechnology::EVDO_B ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::EVDO_B));
static_assert(aidl::android::hardware::radio::RadioTechnology::EHRPD ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::EHRPD));
static_assert(aidl::android::hardware::radio::RadioTechnology::LTE ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::LTE));
static_assert(aidl::android::hardware::radio::RadioTechnology::HSPAP ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::HSPAP));
static_assert(aidl::android::hardware::radio::RadioTechnology::GSM ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::GSM));
static_assert(aidl::android::hardware::radio::RadioTechnology::TD_SCDMA ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::TD_SCDMA));
static_assert(aidl::android::hardware::radio::RadioTechnology::IWLAN ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::IWLAN));
static_assert(aidl::android::hardware::radio::RadioTechnology::LTE_CA ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::LTE_CA));
static_assert(aidl::android::hardware::radio::RadioTechnology::NR ==
              static_cast<aidl::android::hardware::radio::RadioTechnology>(
                      ::android::hardware::radio::V1_4::RadioTechnology::NR));

static_assert(aidl::android::hardware::radio::RadioAccessFamily::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::UNKNOWN));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::GPRS ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::GPRS));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::EDGE ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::EDGE));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::UMTS ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::UMTS));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::IS95A ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::IS95A));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::IS95B ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::IS95B));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::ONE_X_RTT ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::ONE_X_RTT));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::EVDO_0 ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::EVDO_0));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::EVDO_A ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::EVDO_A));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::HSDPA ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::HSDPA));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::HSUPA ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::HSUPA));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::HSPA ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::HSPA));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::EVDO_B ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::EVDO_B));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::EHRPD ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::EHRPD));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::LTE ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::LTE));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::HSPAP ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::HSPAP));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::GSM ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::GSM));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::TD_SCDMA ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::TD_SCDMA));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::LTE_CA ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::LTE_CA));
static_assert(aidl::android::hardware::radio::RadioAccessFamily::NR ==
              static_cast<aidl::android::hardware::radio::RadioAccessFamily>(
                      ::android::hardware::radio::V1_4::RadioAccessFamily::NR));

static_assert(aidl::android::hardware::radio::FrequencyRange::LOW ==
              static_cast<aidl::android::hardware::radio::FrequencyRange>(
                      ::android::hardware::radio::V1_4::FrequencyRange::LOW));
static_assert(aidl::android::hardware::radio::FrequencyRange::MID ==
              static_cast<aidl::android::hardware::radio::FrequencyRange>(
                      ::android::hardware::radio::V1_4::FrequencyRange::MID));
static_assert(aidl::android::hardware::radio::FrequencyRange::HIGH ==
              static_cast<aidl::android::hardware::radio::FrequencyRange>(
                      ::android::hardware::radio::V1_4::FrequencyRange::HIGH));
static_assert(aidl::android::hardware::radio::FrequencyRange::MMWAVE ==
              static_cast<aidl::android::hardware::radio::FrequencyRange>(
                      ::android::hardware::radio::V1_4::FrequencyRange::MMWAVE));

static_assert(aidl::android::hardware::radio::DataConnActiveStatus::INACTIVE ==
              static_cast<aidl::android::hardware::radio::DataConnActiveStatus>(
                      ::android::hardware::radio::V1_4::DataConnActiveStatus::INACTIVE));
static_assert(aidl::android::hardware::radio::DataConnActiveStatus::DORMANT ==
              static_cast<aidl::android::hardware::radio::DataConnActiveStatus>(
                      ::android::hardware::radio::V1_4::DataConnActiveStatus::DORMANT));
static_assert(aidl::android::hardware::radio::DataConnActiveStatus::ACTIVE ==
              static_cast<aidl::android::hardware::radio::DataConnActiveStatus>(
                      ::android::hardware::radio::V1_4::DataConnActiveStatus::ACTIVE));

static_assert(aidl::android::hardware::radio::PdpProtocolType::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::PdpProtocolType>(
                      ::android::hardware::radio::V1_4::PdpProtocolType::UNKNOWN));
static_assert(aidl::android::hardware::radio::PdpProtocolType::IP ==
              static_cast<aidl::android::hardware::radio::PdpProtocolType>(
                      ::android::hardware::radio::V1_4::PdpProtocolType::IP));
static_assert(aidl::android::hardware::radio::PdpProtocolType::IPV6 ==
              static_cast<aidl::android::hardware::radio::PdpProtocolType>(
                      ::android::hardware::radio::V1_4::PdpProtocolType::IPV6));
static_assert(aidl::android::hardware::radio::PdpProtocolType::IPV4V6 ==
              static_cast<aidl::android::hardware::radio::PdpProtocolType>(
                      ::android::hardware::radio::V1_4::PdpProtocolType::IPV4V6));
static_assert(aidl::android::hardware::radio::PdpProtocolType::PPP ==
              static_cast<aidl::android::hardware::radio::PdpProtocolType>(
                      ::android::hardware::radio::V1_4::PdpProtocolType::PPP));
static_assert(aidl::android::hardware::radio::PdpProtocolType::NON_IP ==
              static_cast<aidl::android::hardware::radio::PdpProtocolType>(
                      ::android::hardware::radio::V1_4::PdpProtocolType::NON_IP));
static_assert(aidl::android::hardware::radio::PdpProtocolType::UNSTRUCTURED ==
              static_cast<aidl::android::hardware::radio::PdpProtocolType>(
                      ::android::hardware::radio::V1_4::PdpProtocolType::UNSTRUCTURED));

static_assert(aidl::android::hardware::radio::AccessNetwork::GERAN ==
              static_cast<aidl::android::hardware::radio::AccessNetwork>(
                      ::android::hardware::radio::V1_5::AccessNetwork::GERAN));
static_assert(aidl::android::hardware::radio::AccessNetwork::UTRAN ==
              static_cast<aidl::android::hardware::radio::AccessNetwork>(
                      ::android::hardware::radio::V1_5::AccessNetwork::UTRAN));
static_assert(aidl::android::hardware::radio::AccessNetwork::EUTRAN ==
              static_cast<aidl::android::hardware::radio::AccessNetwork>(
                      ::android::hardware::radio::V1_5::AccessNetwork::EUTRAN));
static_assert(aidl::android::hardware::radio::AccessNetwork::CDMA2000 ==
              static_cast<aidl::android::hardware::radio::AccessNetwork>(
                      ::android::hardware::radio::V1_5::AccessNetwork::CDMA2000));
static_assert(aidl::android::hardware::radio::AccessNetwork::IWLAN ==
              static_cast<aidl::android::hardware::radio::AccessNetwork>(
                      ::android::hardware::radio::V1_5::AccessNetwork::IWLAN));
static_assert(aidl::android::hardware::radio::AccessNetwork::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::AccessNetwork>(
                      ::android::hardware::radio::V1_5::AccessNetwork::UNKNOWN));
static_assert(aidl::android::hardware::radio::AccessNetwork::NGRAN ==
              static_cast<aidl::android::hardware::radio::AccessNetwork>(
                      ::android::hardware::radio::V1_5::AccessNetwork::NGRAN));

static_assert(aidl::android::hardware::radio::SignalMeasurementType::RSSI ==
              static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
                      ::android::hardware::radio::V1_5::SignalMeasurementType::RSSI));
static_assert(aidl::android::hardware::radio::SignalMeasurementType::RSCP ==
              static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
                      ::android::hardware::radio::V1_5::SignalMeasurementType::RSCP));
static_assert(aidl::android::hardware::radio::SignalMeasurementType::RSRP ==
              static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
                      ::android::hardware::radio::V1_5::SignalMeasurementType::RSRP));
static_assert(aidl::android::hardware::radio::SignalMeasurementType::RSRQ ==
              static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
                      ::android::hardware::radio::V1_5::SignalMeasurementType::RSRQ));
static_assert(aidl::android::hardware::radio::SignalMeasurementType::RSSNR ==
              static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
                      ::android::hardware::radio::V1_5::SignalMeasurementType::RSSNR));
static_assert(aidl::android::hardware::radio::SignalMeasurementType::SSRSRP ==
              static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
                      ::android::hardware::radio::V1_5::SignalMeasurementType::SSRSRP));
static_assert(aidl::android::hardware::radio::SignalMeasurementType::SSRSRQ ==
              static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
                      ::android::hardware::radio::V1_5::SignalMeasurementType::SSRSRQ));
static_assert(aidl::android::hardware::radio::SignalMeasurementType::SSSINR ==
              static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
                      ::android::hardware::radio::V1_5::SignalMeasurementType::SSSINR));

static_assert(aidl::android::hardware::radio::SimLockMultiSimPolicy::NO_MULTISIM_POLICY ==
              static_cast<aidl::android::hardware::radio::SimLockMultiSimPolicy>(
                      ::android::hardware::radio::V1_4::SimLockMultiSimPolicy::NO_MULTISIM_POLICY));
static_assert(
        aidl::android::hardware::radio::SimLockMultiSimPolicy::ONE_VALID_SIM_MUST_BE_PRESENT ==
        static_cast<aidl::android::hardware::radio::SimLockMultiSimPolicy>(
                ::android::hardware::radio::V1_4::SimLockMultiSimPolicy::
                        ONE_VALID_SIM_MUST_BE_PRESENT));

static_assert(aidl::android::hardware::radio::RadioAccessNetworks::GERAN ==
              static_cast<aidl::android::hardware::radio::RadioAccessNetworks>(
                      ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN));
static_assert(aidl::android::hardware::radio::RadioAccessNetworks::UTRAN ==
              static_cast<aidl::android::hardware::radio::RadioAccessNetworks>(
                      ::android::hardware::radio::V1_5::RadioAccessNetworks::UTRAN));
static_assert(aidl::android::hardware::radio::RadioAccessNetworks::EUTRAN ==
              static_cast<aidl::android::hardware::radio::RadioAccessNetworks>(
                      ::android::hardware::radio::V1_5::RadioAccessNetworks::EUTRAN));
static_assert(aidl::android::hardware::radio::RadioAccessNetworks::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::RadioAccessNetworks>(
                      ::android::hardware::radio::V1_5::RadioAccessNetworks::UNKNOWN));
static_assert(aidl::android::hardware::radio::RadioAccessNetworks::NGRAN ==
              static_cast<aidl::android::hardware::radio::RadioAccessNetworks>(
                      ::android::hardware::radio::V1_5::RadioAccessNetworks::NGRAN));
static_assert(aidl::android::hardware::radio::RadioAccessNetworks::CDMA2000 ==
              static_cast<aidl::android::hardware::radio::RadioAccessNetworks>(
                      ::android::hardware::radio::V1_5::RadioAccessNetworks::CDMA2000));

static_assert(aidl::android::hardware::radio::UtranBands::BAND_1 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_1));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_2 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_2));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_3 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_3));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_4 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_4));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_5 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_5));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_6 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_6));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_7 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_7));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_8 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_8));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_9 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_9));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_10 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_10));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_11 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_11));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_12 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_12));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_13 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_13));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_14 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_14));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_19 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_19));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_20 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_20));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_21 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_21));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_22 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_22));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_25 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_25));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_26 ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_26));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_A ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_A));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_B ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_B));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_C ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_C));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_D ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_D));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_E ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_E));
static_assert(aidl::android::hardware::radio::UtranBands::BAND_F ==
              static_cast<aidl::android::hardware::radio::UtranBands>(
                      ::android::hardware::radio::V1_5::UtranBands::BAND_F));

static_assert(aidl::android::hardware::radio::EutranBands::BAND_1 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_1));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_2 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_2));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_3 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_3));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_4 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_4));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_5 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_5));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_6 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_6));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_7 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_7));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_8 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_8));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_9 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_9));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_10 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_10));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_11 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_11));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_12 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_12));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_13 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_13));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_14 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_14));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_17 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_17));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_18 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_18));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_19 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_19));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_20 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_20));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_21 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_21));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_22 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_22));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_23 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_23));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_24 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_24));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_25 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_25));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_26 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_26));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_27 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_27));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_28 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_28));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_30 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_30));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_31 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_31));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_33 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_33));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_34 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_34));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_35 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_35));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_36 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_36));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_37 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_37));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_38 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_38));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_39 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_39));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_40 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_40));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_41 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_41));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_42 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_42));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_43 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_43));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_44 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_44));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_45 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_45));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_46 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_46));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_47 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_47));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_48 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_48));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_65 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_65));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_66 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_66));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_68 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_68));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_70 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_70));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_49 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_49));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_50 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_50));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_51 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_51));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_52 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_52));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_53 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_53));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_71 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_71));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_72 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_72));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_73 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_73));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_74 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_74));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_85 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_85));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_87 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_87));
static_assert(aidl::android::hardware::radio::EutranBands::BAND_88 ==
              static_cast<aidl::android::hardware::radio::EutranBands>(
                      ::android::hardware::radio::V1_5::EutranBands::BAND_88));

static_assert(aidl::android::hardware::radio::ApnTypes::NONE ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::NONE));
static_assert(aidl::android::hardware::radio::ApnTypes::DEFAULT ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::DEFAULT));
static_assert(aidl::android::hardware::radio::ApnTypes::MMS ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::MMS));
static_assert(aidl::android::hardware::radio::ApnTypes::SUPL ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::SUPL));
static_assert(aidl::android::hardware::radio::ApnTypes::DUN ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::DUN));
static_assert(aidl::android::hardware::radio::ApnTypes::HIPRI ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::HIPRI));
static_assert(aidl::android::hardware::radio::ApnTypes::FOTA ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::FOTA));
static_assert(aidl::android::hardware::radio::ApnTypes::IMS ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::IMS));
static_assert(aidl::android::hardware::radio::ApnTypes::CBS ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::CBS));
static_assert(aidl::android::hardware::radio::ApnTypes::IA ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::IA));
static_assert(aidl::android::hardware::radio::ApnTypes::EMERGENCY ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::EMERGENCY));
static_assert(aidl::android::hardware::radio::ApnTypes::ALL ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::ALL));
static_assert(aidl::android::hardware::radio::ApnTypes::MCX ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::MCX));
static_assert(aidl::android::hardware::radio::ApnTypes::XCAP ==
              static_cast<aidl::android::hardware::radio::ApnTypes>(
                      ::android::hardware::radio::V1_5::ApnTypes::XCAP));

static_assert(aidl::android::hardware::radio::AddressProperty::NONE ==
              static_cast<aidl::android::hardware::radio::AddressProperty>(
                      ::android::hardware::radio::V1_5::AddressProperty::NONE));
static_assert(aidl::android::hardware::radio::AddressProperty::DEPRECATED ==
              static_cast<aidl::android::hardware::radio::AddressProperty>(
                      ::android::hardware::radio::V1_5::AddressProperty::DEPRECATED));

static_assert(aidl::android::hardware::radio::Domain::CS ==
              static_cast<aidl::android::hardware::radio::Domain>(
                      ::android::hardware::radio::V1_5::Domain::CS));
static_assert(aidl::android::hardware::radio::Domain::PS ==
              static_cast<aidl::android::hardware::radio::Domain>(
                      ::android::hardware::radio::V1_5::Domain::PS));

static_assert(aidl::android::hardware::radio::BarringInfoServiceType::CS_SERVICE ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::CS_SERVICE));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::PS_SERVICE ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::PS_SERVICE));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::CS_VOICE ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::CS_VOICE));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::MO_SIGNALLING ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::MO_SIGNALLING));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::MO_DATA ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::MO_DATA));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::CS_FALLBACK ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::CS_FALLBACK));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::MMTEL_VOICE ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::MMTEL_VOICE));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::MMTEL_VIDEO ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::MMTEL_VIDEO));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::EMERGENCY ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::EMERGENCY));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::SMS ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::SMS));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_1 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_1));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_2 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_2));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_3 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_3));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_4 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_4));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_5 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_5));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_6 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_6));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_7 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_7));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_8 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_8));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_9 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_9));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_10 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_10));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_11 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_11));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_12 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_12));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_13 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_13));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_14 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_14));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_15 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_15));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_16 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_16));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_17 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_17));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_18 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_18));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_19 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_19));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_20 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_20));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_21 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_21));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_22 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_22));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_23 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_23));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_24 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_24));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_25 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_25));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_26 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_26));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_27 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_27));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_28 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_28));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_29 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_29));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_30 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_30));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_31 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_31));
static_assert(aidl::android::hardware::radio::BarringInfoServiceType::OPERATOR_32 ==
              static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(
                      ::android::hardware::radio::V1_5::BarringInfo::ServiceType::OPERATOR_32));

static_assert(aidl::android::hardware::radio::BarringInfoBarringType::NONE ==
              static_cast<aidl::android::hardware::radio::BarringInfoBarringType>(
                      ::android::hardware::radio::V1_5::BarringInfo::BarringType::NONE));
static_assert(aidl::android::hardware::radio::BarringInfoBarringType::CONDITIONAL ==
              static_cast<aidl::android::hardware::radio::BarringInfoBarringType>(
                      ::android::hardware::radio::V1_5::BarringInfo::BarringType::CONDITIONAL));
static_assert(aidl::android::hardware::radio::BarringInfoBarringType::UNCONDITIONAL ==
              static_cast<aidl::android::hardware::radio::BarringInfoBarringType>(
                      ::android::hardware::radio::V1_5::BarringInfo::BarringType::UNCONDITIONAL));

static_assert(aidl::android::hardware::radio::IndicationFilter::NONE ==
              static_cast<aidl::android::hardware::radio::IndicationFilter>(
                      ::android::hardware::radio::V1_5::IndicationFilter::NONE));
static_assert(aidl::android::hardware::radio::IndicationFilter::ALL ==
              static_cast<aidl::android::hardware::radio::IndicationFilter>(
                      ::android::hardware::radio::V1_5::IndicationFilter::ALL));
static_assert(aidl::android::hardware::radio::IndicationFilter::SIGNAL_STRENGTH ==
              static_cast<aidl::android::hardware::radio::IndicationFilter>(
                      ::android::hardware::radio::V1_5::IndicationFilter::SIGNAL_STRENGTH));
static_assert(aidl::android::hardware::radio::IndicationFilter::FULL_NETWORK_STATE ==
              static_cast<aidl::android::hardware::radio::IndicationFilter>(
                      ::android::hardware::radio::V1_5::IndicationFilter::FULL_NETWORK_STATE));
static_assert(
        aidl::android::hardware::radio::IndicationFilter::DATA_CALL_DORMANCY_CHANGED ==
        static_cast<aidl::android::hardware::radio::IndicationFilter>(
                ::android::hardware::radio::V1_5::IndicationFilter::DATA_CALL_DORMANCY_CHANGED));
static_assert(aidl::android::hardware::radio::IndicationFilter::LINK_CAPACITY_ESTIMATE ==
              static_cast<aidl::android::hardware::radio::IndicationFilter>(
                      ::android::hardware::radio::V1_5::IndicationFilter::LINK_CAPACITY_ESTIMATE));
static_assert(aidl::android::hardware::radio::IndicationFilter::PHYSICAL_CHANNEL_CONFIG ==
              static_cast<aidl::android::hardware::radio::IndicationFilter>(
                      ::android::hardware::radio::V1_5::IndicationFilter::PHYSICAL_CHANNEL_CONFIG));
static_assert(aidl::android::hardware::radio::IndicationFilter::REGISTRATION_FAILURE ==
              static_cast<aidl::android::hardware::radio::IndicationFilter>(
                      ::android::hardware::radio::V1_5::IndicationFilter::REGISTRATION_FAILURE));
static_assert(aidl::android::hardware::radio::IndicationFilter::BARRING_INFO ==
              static_cast<aidl::android::hardware::radio::IndicationFilter>(
                      ::android::hardware::radio::V1_5::IndicationFilter::BARRING_INFO));

static_assert(aidl::android::hardware::radio::RegistrationFailCause::NONE ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::NONE));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::IMSI_UNKNOWN_IN_HLR ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::IMSI_UNKNOWN_IN_HLR));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::ILLEGAL_MS ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::ILLEGAL_MS));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::IMSI_UNKNOWN_IN_VLR ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::IMSI_UNKNOWN_IN_VLR));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::IMEI_NOT_ACCEPTED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::IMEI_NOT_ACCEPTED));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::ILLEGAL_ME ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::ILLEGAL_ME));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::GPRS_SERVICES_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              GPRS_SERVICES_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::
                      GPRS_AND_NON_GPRS_SERVICES_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              GPRS_AND_NON_GPRS_SERVICES_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::
                      MS_IDENTITY_CANNOT_BE_DERIVED_BY_NETWORK ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              MS_IDENTITY_CANNOT_BE_DERIVED_BY_NETWORK));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::IMPLICITLY_DETACHED ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::IMPLICITLY_DETACHED));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::PLMN_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::PLMN_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::LOCATION_AREA_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              LOCATION_AREA_NOT_ALLOWED));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::ROAMING_NOT_ALLOWED ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::ROAMING_NOT_ALLOWED));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::GPRS_SERVICES_NOT_ALLOWED_IN_PLMN ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        GPRS_SERVICES_NOT_ALLOWED_IN_PLMN));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::NO_SUITABLE_CELLS ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::NO_SUITABLE_CELLS));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::MSC_TEMPORARILY_NOT_REACHABLE ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        MSC_TEMPORARILY_NOT_REACHABLE));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::NETWORK_FAILURE ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::NETWORK_FAILURE));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::MAC_FAILURE ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::MAC_FAILURE));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::SYNC_FAILURE ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::SYNC_FAILURE));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::CONGESTION ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::CONGESTION));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::GSM_AUTHENTICATION_UNACCEPTABLE ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        GSM_AUTHENTICATION_UNACCEPTABLE));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::NOT_AUTHORIZED_FOR_THIS_CSG ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              NOT_AUTHORIZED_FOR_THIS_CSG));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::
                      SMS_PROVIDED_BY_GPRS_IN_ROUTING_AREA ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              SMS_PROVIDED_BY_GPRS_IN_ROUTING_AREA));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::SERVICE_OPTION_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              SERVICE_OPTION_NOT_SUPPORTED));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::SERVICE_OPTION_NOT_SUBSCRIBED ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        SERVICE_OPTION_NOT_SUBSCRIBED));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::
                      SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::CALL_CANNOT_BE_IDENTIFIED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              CALL_CANNOT_BE_IDENTIFIED));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::NO_PDP_CONTEXT_ACTIVATED ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::NO_PDP_CONTEXT_ACTIVATED));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_1 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_1));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_2 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_2));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_3 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_3));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_4 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_4));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_5 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_5));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_6 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_6));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_7 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_7));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_8 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_8));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_9 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_9));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_10 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_10));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_11 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_11));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_12 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_12));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_13 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_13));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_14 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_14));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_15 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_15));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::RETRY_UPON_ENTRY_INTO_NEW_CELL_16 ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        RETRY_UPON_ENTRY_INTO_NEW_CELL_16));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::SEMANTICALLY_INCORRECT_MESSAGE ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        SEMANTICALLY_INCORRECT_MESSAGE));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::INVALID_MANDATORY_INFORMATION ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::
                        INVALID_MANDATORY_INFORMATION));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::
                      MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::
                      MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::
                      INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED));
static_assert(
        aidl::android::hardware::radio::RegistrationFailCause::CONDITIONAL_IE_ERROR ==
        static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                ::android::hardware::radio::V1_5::RegistrationFailCause::CONDITIONAL_IE_ERROR));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::
                      MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE));
static_assert(aidl::android::hardware::radio::RegistrationFailCause::PROTOCOL_ERROR_UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::RegistrationFailCause>(
                      ::android::hardware::radio::V1_5::RegistrationFailCause::
                              PROTOCOL_ERROR_UNSPECIFIED));

static_assert(aidl::android::hardware::radio::PrlIndicator::NOT_REGISTERED ==
              static_cast<aidl::android::hardware::radio::PrlIndicator>(
                      ::android::hardware::radio::V1_5::PrlIndicator::NOT_REGISTERED));
static_assert(aidl::android::hardware::radio::PrlIndicator::NOT_IN_PRL ==
              static_cast<aidl::android::hardware::radio::PrlIndicator>(
                      ::android::hardware::radio::V1_5::PrlIndicator::NOT_IN_PRL));
static_assert(aidl::android::hardware::radio::PrlIndicator::IN_PRL ==
              static_cast<aidl::android::hardware::radio::PrlIndicator>(
                      ::android::hardware::radio::V1_5::PrlIndicator::IN_PRL));

static_assert(aidl::android::hardware::radio::PersoSubstate::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::UNKNOWN));
static_assert(aidl::android::hardware::radio::PersoSubstate::IN_PROGRESS ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::IN_PROGRESS));
static_assert(aidl::android::hardware::radio::PersoSubstate::READY ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::READY));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_NETWORK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_NETWORK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_NETWORK_SUBSET ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_NETWORK_SUBSET));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_CORPORATE ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_CORPORATE));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_SERVICE_PROVIDER ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_SERVICE_PROVIDER));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_SIM ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_SIM));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_NETWORK_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_NETWORK_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_NETWORK_SUBSET_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_NETWORK_SUBSET_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_CORPORATE_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_CORPORATE_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_SERVICE_PROVIDER_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_SERVICE_PROVIDER_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_SIM_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_SIM_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_NETWORK1 ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_NETWORK1));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_NETWORK2 ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_NETWORK2));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_HRPD ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_HRPD));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_CORPORATE ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_CORPORATE));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_SERVICE_PROVIDER ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_SERVICE_PROVIDER));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_RUIM ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_RUIM));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_NETWORK1_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_NETWORK1_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_NETWORK2_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_NETWORK2_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_HRPD_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_HRPD_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_CORPORATE_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_CORPORATE_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_SERVICE_PROVIDER_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_SERVICE_PROVIDER_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::RUIM_RUIM_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::RUIM_RUIM_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_SPN ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_SPN));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_SPN_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_SPN_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_SP_EHPLMN ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_SP_EHPLMN));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_SP_EHPLMN_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_SP_EHPLMN_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_ICCID ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_ICCID));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_ICCID_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_ICCID_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_IMPI ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_IMPI));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_IMPI_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_IMPI_PUK));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_NS_SP ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_NS_SP));
static_assert(aidl::android::hardware::radio::PersoSubstate::SIM_NS_SP_PUK ==
              static_cast<aidl::android::hardware::radio::PersoSubstate>(
                      ::android::hardware::radio::V1_5::PersoSubstate::SIM_NS_SP_PUK));

static_assert(aidl::android::hardware::radio::QosFlowIdRange::MIN ==
              static_cast<aidl::android::hardware::radio::QosFlowIdRange>(
                      ::android::hardware::radio::V1_6::QosFlowIdRange::MIN));
static_assert(aidl::android::hardware::radio::QosFlowIdRange::MAX ==
              static_cast<aidl::android::hardware::radio::QosFlowIdRange>(
                      ::android::hardware::radio::V1_6::QosFlowIdRange::MAX));

static_assert(aidl::android::hardware::radio::QosProtocol::UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::QosProtocol>(
                      ::android::hardware::radio::V1_6::QosProtocol::UNSPECIFIED));
static_assert(aidl::android::hardware::radio::QosProtocol::TCP ==
              static_cast<aidl::android::hardware::radio::QosProtocol>(
                      ::android::hardware::radio::V1_6::QosProtocol::TCP));
static_assert(aidl::android::hardware::radio::QosProtocol::UDP ==
              static_cast<aidl::android::hardware::radio::QosProtocol>(
                      ::android::hardware::radio::V1_6::QosProtocol::UDP));
static_assert(aidl::android::hardware::radio::QosProtocol::ESP ==
              static_cast<aidl::android::hardware::radio::QosProtocol>(
                      ::android::hardware::radio::V1_6::QosProtocol::ESP));
static_assert(aidl::android::hardware::radio::QosProtocol::AH ==
              static_cast<aidl::android::hardware::radio::QosProtocol>(
                      ::android::hardware::radio::V1_6::QosProtocol::AH));

static_assert(aidl::android::hardware::radio::QosFilterDirection::DOWNLINK ==
              static_cast<aidl::android::hardware::radio::QosFilterDirection>(
                      ::android::hardware::radio::V1_6::QosFilterDirection::DOWNLINK));
static_assert(aidl::android::hardware::radio::QosFilterDirection::UPLINK ==
              static_cast<aidl::android::hardware::radio::QosFilterDirection>(
                      ::android::hardware::radio::V1_6::QosFilterDirection::UPLINK));
static_assert(aidl::android::hardware::radio::QosFilterDirection::BIDIRECTIONAL ==
              static_cast<aidl::android::hardware::radio::QosFilterDirection>(
                      ::android::hardware::radio::V1_6::QosFilterDirection::BIDIRECTIONAL));

static_assert(aidl::android::hardware::radio::QosPortRange::MIN ==
              static_cast<aidl::android::hardware::radio::QosPortRange>(
                      ::android::hardware::radio::V1_6::QosPortRange::MIN));
static_assert(aidl::android::hardware::radio::QosPortRange::MAX ==
              static_cast<aidl::android::hardware::radio::QosPortRange>(
                      ::android::hardware::radio::V1_6::QosPortRange::MAX));

static_assert(aidl::android::hardware::radio::RadioError::NONE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NONE));
static_assert(aidl::android::hardware::radio::RadioError::RADIO_NOT_AVAILABLE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE));
static_assert(aidl::android::hardware::radio::RadioError::GENERIC_FAILURE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::GENERIC_FAILURE));
static_assert(aidl::android::hardware::radio::RadioError::PASSWORD_INCORRECT ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::PASSWORD_INCORRECT));
static_assert(aidl::android::hardware::radio::RadioError::SIM_PIN2 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SIM_PIN2));
static_assert(aidl::android::hardware::radio::RadioError::SIM_PUK2 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SIM_PUK2));
static_assert(aidl::android::hardware::radio::RadioError::REQUEST_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::RadioError::CANCELLED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::CANCELLED));
static_assert(
        aidl::android::hardware::radio::RadioError::OP_NOT_ALLOWED_DURING_VOICE_CALL ==
        static_cast<aidl::android::hardware::radio::RadioError>(
                ::android::hardware::radio::V1_6::RadioError::OP_NOT_ALLOWED_DURING_VOICE_CALL));
static_assert(
        aidl::android::hardware::radio::RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW ==
        static_cast<aidl::android::hardware::radio::RadioError>(
                ::android::hardware::radio::V1_6::RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW));
static_assert(aidl::android::hardware::radio::RadioError::SMS_SEND_FAIL_RETRY ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SMS_SEND_FAIL_RETRY));
static_assert(aidl::android::hardware::radio::RadioError::SIM_ABSENT ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SIM_ABSENT));
static_assert(aidl::android::hardware::radio::RadioError::SUBSCRIPTION_NOT_AVAILABLE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SUBSCRIPTION_NOT_AVAILABLE));
static_assert(aidl::android::hardware::radio::RadioError::MODE_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::MODE_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::RadioError::FDN_CHECK_FAILURE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::FDN_CHECK_FAILURE));
static_assert(aidl::android::hardware::radio::RadioError::ILLEGAL_SIM_OR_ME ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::ILLEGAL_SIM_OR_ME));
static_assert(aidl::android::hardware::radio::RadioError::MISSING_RESOURCE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::MISSING_RESOURCE));
static_assert(aidl::android::hardware::radio::RadioError::NO_SUCH_ELEMENT ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NO_SUCH_ELEMENT));
static_assert(aidl::android::hardware::radio::RadioError::DIAL_MODIFIED_TO_USSD ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::DIAL_MODIFIED_TO_USSD));
static_assert(aidl::android::hardware::radio::RadioError::DIAL_MODIFIED_TO_SS ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::DIAL_MODIFIED_TO_SS));
static_assert(aidl::android::hardware::radio::RadioError::DIAL_MODIFIED_TO_DIAL ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::DIAL_MODIFIED_TO_DIAL));
static_assert(aidl::android::hardware::radio::RadioError::USSD_MODIFIED_TO_DIAL ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::USSD_MODIFIED_TO_DIAL));
static_assert(aidl::android::hardware::radio::RadioError::USSD_MODIFIED_TO_SS ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::USSD_MODIFIED_TO_SS));
static_assert(aidl::android::hardware::radio::RadioError::USSD_MODIFIED_TO_USSD ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::USSD_MODIFIED_TO_USSD));
static_assert(aidl::android::hardware::radio::RadioError::SS_MODIFIED_TO_DIAL ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SS_MODIFIED_TO_DIAL));
static_assert(aidl::android::hardware::radio::RadioError::SS_MODIFIED_TO_USSD ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SS_MODIFIED_TO_USSD));
static_assert(aidl::android::hardware::radio::RadioError::SUBSCRIPTION_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SUBSCRIPTION_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::RadioError::SS_MODIFIED_TO_SS ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SS_MODIFIED_TO_SS));
static_assert(aidl::android::hardware::radio::RadioError::LCE_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::LCE_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::RadioError::NO_MEMORY ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NO_MEMORY));
static_assert(aidl::android::hardware::radio::RadioError::INTERNAL_ERR ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INTERNAL_ERR));
static_assert(aidl::android::hardware::radio::RadioError::SYSTEM_ERR ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SYSTEM_ERR));
static_assert(aidl::android::hardware::radio::RadioError::MODEM_ERR ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::MODEM_ERR));
static_assert(aidl::android::hardware::radio::RadioError::INVALID_STATE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INVALID_STATE));
static_assert(aidl::android::hardware::radio::RadioError::NO_RESOURCES ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NO_RESOURCES));
static_assert(aidl::android::hardware::radio::RadioError::SIM_ERR ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SIM_ERR));
static_assert(aidl::android::hardware::radio::RadioError::INVALID_ARGUMENTS ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS));
static_assert(aidl::android::hardware::radio::RadioError::INVALID_SIM_STATE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INVALID_SIM_STATE));
static_assert(aidl::android::hardware::radio::RadioError::INVALID_MODEM_STATE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INVALID_MODEM_STATE));
static_assert(aidl::android::hardware::radio::RadioError::INVALID_CALL_ID ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INVALID_CALL_ID));
static_assert(aidl::android::hardware::radio::RadioError::NO_SMS_TO_ACK ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NO_SMS_TO_ACK));
static_assert(aidl::android::hardware::radio::RadioError::NETWORK_ERR ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NETWORK_ERR));
static_assert(aidl::android::hardware::radio::RadioError::REQUEST_RATE_LIMITED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::REQUEST_RATE_LIMITED));
static_assert(aidl::android::hardware::radio::RadioError::SIM_BUSY ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SIM_BUSY));
static_assert(aidl::android::hardware::radio::RadioError::SIM_FULL ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::SIM_FULL));
static_assert(aidl::android::hardware::radio::RadioError::NETWORK_REJECT ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NETWORK_REJECT));
static_assert(aidl::android::hardware::radio::RadioError::OPERATION_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OPERATION_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::RadioError::EMPTY_RECORD ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::EMPTY_RECORD));
static_assert(aidl::android::hardware::radio::RadioError::INVALID_SMS_FORMAT ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INVALID_SMS_FORMAT));
static_assert(aidl::android::hardware::radio::RadioError::ENCODING_ERR ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::ENCODING_ERR));
static_assert(aidl::android::hardware::radio::RadioError::INVALID_SMSC_ADDRESS ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INVALID_SMSC_ADDRESS));
static_assert(aidl::android::hardware::radio::RadioError::NO_SUCH_ENTRY ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NO_SUCH_ENTRY));
static_assert(aidl::android::hardware::radio::RadioError::NETWORK_NOT_READY ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NETWORK_NOT_READY));
static_assert(aidl::android::hardware::radio::RadioError::NOT_PROVISIONED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NOT_PROVISIONED));
static_assert(aidl::android::hardware::radio::RadioError::NO_SUBSCRIPTION ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NO_SUBSCRIPTION));
static_assert(aidl::android::hardware::radio::RadioError::NO_NETWORK_FOUND ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NO_NETWORK_FOUND));
static_assert(aidl::android::hardware::radio::RadioError::DEVICE_IN_USE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::DEVICE_IN_USE));
static_assert(aidl::android::hardware::radio::RadioError::ABORTED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::ABORTED));
static_assert(aidl::android::hardware::radio::RadioError::INVALID_RESPONSE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::INVALID_RESPONSE));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_1 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_1));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_2 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_2));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_3 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_3));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_4 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_4));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_5 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_5));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_6 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_6));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_7 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_7));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_8 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_8));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_9 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_9));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_10 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_10));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_11 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_11));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_12 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_12));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_13 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_13));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_14 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_14));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_15 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_15));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_16 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_16));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_17 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_17));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_18 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_18));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_19 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_19));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_20 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_20));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_21 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_21));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_22 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_22));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_23 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_23));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_24 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_24));
static_assert(aidl::android::hardware::radio::RadioError::OEM_ERROR_25 ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_25));
static_assert(aidl::android::hardware::radio::RadioError::SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::
                              SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::RadioError::ACCESS_BARRED ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::ACCESS_BARRED));
static_assert(aidl::android::hardware::radio::RadioError::BLOCKED_DUE_TO_CALL ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::BLOCKED_DUE_TO_CALL));
static_assert(aidl::android::hardware::radio::RadioError::RF_HARDWARE_ISSUE ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::RF_HARDWARE_ISSUE));
static_assert(aidl::android::hardware::radio::RadioError::NO_RF_CALIBRATION_INFO ==
              static_cast<aidl::android::hardware::radio::RadioError>(
                      ::android::hardware::radio::V1_6::RadioError::NO_RF_CALIBRATION_INFO));

static_assert(aidl::android::hardware::radio::HandoverFailureMode::LEGACY ==
              static_cast<aidl::android::hardware::radio::HandoverFailureMode>(
                      ::android::hardware::radio::V1_6::HandoverFailureMode::LEGACY));
static_assert(aidl::android::hardware::radio::HandoverFailureMode::DO_FALLBACK ==
              static_cast<aidl::android::hardware::radio::HandoverFailureMode>(
                      ::android::hardware::radio::V1_6::HandoverFailureMode::DO_FALLBACK));
static_assert(
        aidl::android::hardware::radio::HandoverFailureMode::NO_FALLBACK_RETRY_HANDOVER ==
        static_cast<aidl::android::hardware::radio::HandoverFailureMode>(
                ::android::hardware::radio::V1_6::HandoverFailureMode::NO_FALLBACK_RETRY_HANDOVER));
static_assert(aidl::android::hardware::radio::HandoverFailureMode::NO_FALLBACK_RETRY_SETUP_NORMAL ==
              static_cast<aidl::android::hardware::radio::HandoverFailureMode>(
                      ::android::hardware::radio::V1_6::HandoverFailureMode::
                              NO_FALLBACK_RETRY_SETUP_NORMAL));

static_assert(aidl::android::hardware::radio::NrDualConnectivityState::ENABLE ==
              static_cast<aidl::android::hardware::radio::NrDualConnectivityState>(
                      ::android::hardware::radio::V1_6::NrDualConnectivityState::ENABLE));
static_assert(aidl::android::hardware::radio::NrDualConnectivityState::DISABLE ==
              static_cast<aidl::android::hardware::radio::NrDualConnectivityState>(
                      ::android::hardware::radio::V1_6::NrDualConnectivityState::DISABLE));
static_assert(
        aidl::android::hardware::radio::NrDualConnectivityState::DISABLE_IMMEDIATE ==
        static_cast<aidl::android::hardware::radio::NrDualConnectivityState>(
                ::android::hardware::radio::V1_6::NrDualConnectivityState::DISABLE_IMMEDIATE));

static_assert(aidl::android::hardware::radio::DataThrottlingAction::NO_DATA_THROTTLING ==
              static_cast<aidl::android::hardware::radio::DataThrottlingAction>(
                      ::android::hardware::radio::V1_6::DataThrottlingAction::NO_DATA_THROTTLING));
static_assert(aidl::android::hardware::radio::DataThrottlingAction::THROTTLE_SECONDARY_CARRIER ==
              static_cast<aidl::android::hardware::radio::DataThrottlingAction>(
                      ::android::hardware::radio::V1_6::DataThrottlingAction::
                              THROTTLE_SECONDARY_CARRIER));
static_assert(
        aidl::android::hardware::radio::DataThrottlingAction::THROTTLE_ANCHOR_CARRIER ==
        static_cast<aidl::android::hardware::radio::DataThrottlingAction>(
                ::android::hardware::radio::V1_6::DataThrottlingAction::THROTTLE_ANCHOR_CARRIER));
static_assert(aidl::android::hardware::radio::DataThrottlingAction::HOLD ==
              static_cast<aidl::android::hardware::radio::DataThrottlingAction>(
                      ::android::hardware::radio::V1_6::DataThrottlingAction::HOLD));

static_assert(aidl::android::hardware::radio::VopsIndicator::VOPS_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::VopsIndicator>(
                      ::android::hardware::radio::V1_6::VopsIndicator::VOPS_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::VopsIndicator::VOPS_OVER_3GPP ==
              static_cast<aidl::android::hardware::radio::VopsIndicator>(
                      ::android::hardware::radio::V1_6::VopsIndicator::VOPS_OVER_3GPP));
static_assert(aidl::android::hardware::radio::VopsIndicator::VOPS_OVER_NON_3GPP ==
              static_cast<aidl::android::hardware::radio::VopsIndicator>(
                      ::android::hardware::radio::V1_6::VopsIndicator::VOPS_OVER_NON_3GPP));

static_assert(aidl::android::hardware::radio::EmcIndicator::EMC_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::EmcIndicator>(
                      ::android::hardware::radio::V1_6::EmcIndicator::EMC_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::EmcIndicator::EMC_NR_CONNECTED_TO_5GCN ==
              static_cast<aidl::android::hardware::radio::EmcIndicator>(
                      ::android::hardware::radio::V1_6::EmcIndicator::EMC_NR_CONNECTED_TO_5GCN));
static_assert(aidl::android::hardware::radio::EmcIndicator::EMC_EUTRA_CONNECTED_TO_5GCN ==
              static_cast<aidl::android::hardware::radio::EmcIndicator>(
                      ::android::hardware::radio::V1_6::EmcIndicator::EMC_EUTRA_CONNECTED_TO_5GCN));
static_assert(aidl::android::hardware::radio::EmcIndicator::EMC_BOTH_NR_EUTRA_CONNECTED_TO_5GCN ==
              static_cast<aidl::android::hardware::radio::EmcIndicator>(
                      ::android::hardware::radio::V1_6::EmcIndicator::
                              EMC_BOTH_NR_EUTRA_CONNECTED_TO_5GCN));

static_assert(aidl::android::hardware::radio::EmfIndicator::EMF_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::EmfIndicator>(
                      ::android::hardware::radio::V1_6::EmfIndicator::EMF_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::EmfIndicator::EMF_NR_CONNECTED_TO_5GCN ==
              static_cast<aidl::android::hardware::radio::EmfIndicator>(
                      ::android::hardware::radio::V1_6::EmfIndicator::EMF_NR_CONNECTED_TO_5GCN));
static_assert(aidl::android::hardware::radio::EmfIndicator::EMF_EUTRA_CONNECTED_TO_5GCN ==
              static_cast<aidl::android::hardware::radio::EmfIndicator>(
                      ::android::hardware::radio::V1_6::EmfIndicator::EMF_EUTRA_CONNECTED_TO_5GCN));
static_assert(aidl::android::hardware::radio::EmfIndicator::EMF_BOTH_NR_EUTRA_CONNECTED_TO_5GCN ==
              static_cast<aidl::android::hardware::radio::EmfIndicator>(
                      ::android::hardware::radio::V1_6::EmfIndicator::
                              EMF_BOTH_NR_EUTRA_CONNECTED_TO_5GCN));

static_assert(aidl::android::hardware::radio::NgranBands::BAND_1 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_1));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_2 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_2));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_3 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_3));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_5 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_5));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_7 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_7));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_8 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_8));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_12 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_12));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_14 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_14));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_18 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_18));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_20 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_20));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_25 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_25));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_28 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_28));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_29 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_29));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_30 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_30));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_34 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_34));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_38 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_38));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_39 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_39));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_40 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_40));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_41 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_41));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_48 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_48));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_50 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_50));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_51 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_51));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_65 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_65));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_66 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_66));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_70 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_70));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_71 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_71));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_74 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_74));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_75 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_75));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_76 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_76));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_77 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_77));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_78 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_78));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_79 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_79));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_80 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_80));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_81 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_81));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_82 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_82));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_83 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_83));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_84 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_84));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_86 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_86));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_89 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_89));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_90 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_90));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_91 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_91));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_92 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_92));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_93 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_93));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_94 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_94));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_95 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_95));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_257 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_257));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_258 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_258));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_260 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_260));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_261 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_261));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_26 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_26));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_46 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_46));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_53 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_53));
static_assert(aidl::android::hardware::radio::NgranBands::BAND_96 ==
              static_cast<aidl::android::hardware::radio::NgranBands>(
                      ::android::hardware::radio::V1_6::NgranBands::BAND_96));

static_assert(aidl::android::hardware::radio::SliceServiceType::NONE ==
              static_cast<aidl::android::hardware::radio::SliceServiceType>(
                      ::android::hardware::radio::V1_6::SliceServiceType::NONE));
static_assert(aidl::android::hardware::radio::SliceServiceType::EMBB ==
              static_cast<aidl::android::hardware::radio::SliceServiceType>(
                      ::android::hardware::radio::V1_6::SliceServiceType::EMBB));
static_assert(aidl::android::hardware::radio::SliceServiceType::URLLC ==
              static_cast<aidl::android::hardware::radio::SliceServiceType>(
                      ::android::hardware::radio::V1_6::SliceServiceType::URLLC));
static_assert(aidl::android::hardware::radio::SliceServiceType::MIOT ==
              static_cast<aidl::android::hardware::radio::SliceServiceType>(
                      ::android::hardware::radio::V1_6::SliceServiceType::MIOT));

static_assert(aidl::android::hardware::radio::DataCallFailCause::NONE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NONE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OPERATOR_BARRED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OPERATOR_BARRED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NAS_SIGNALLING ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NAS_SIGNALLING));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INSUFFICIENT_RESOURCES ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INSUFFICIENT_RESOURCES));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MISSING_UNKNOWN_APN ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MISSING_UKNOWN_APN));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::UNKNOWN_PDP_ADDRESS_TYPE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::UNKNOWN_PDP_ADDRESS_TYPE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::USER_AUTHENTICATION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::USER_AUTHENTICATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ACTIVATION_REJECT_GGSN ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ACTIVATION_REJECT_GGSN));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ACTIVATION_REJECT_UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              ACTIVATION_REJECT_UNSPECIFIED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::SERVICE_OPTION_NOT_SUPPORTED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::SERVICE_OPTION_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::SERVICE_OPTION_NOT_SUBSCRIBED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              SERVICE_OPTION_NOT_SUBSCRIBED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::SERVICE_OPTION_OUT_OF_ORDER ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::SERVICE_OPTION_OUT_OF_ORDER));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NSAPI_IN_USE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NSAPI_IN_USE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::REGULAR_DEACTIVATION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::REGULAR_DEACTIVATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::QOS_NOT_ACCEPTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::QOS_NOT_ACCEPTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NETWORK_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NETWORK_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UMTS_REACTIVATION_REQ ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::UMTS_REACTIVATION_REQ));
static_assert(aidl::android::hardware::radio::DataCallFailCause::FEATURE_NOT_SUPP ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::FEATURE_NOT_SUPP));
static_assert(aidl::android::hardware::radio::DataCallFailCause::TFT_SEMANTIC_ERROR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::TFT_SEMANTIC_ERROR));
static_assert(aidl::android::hardware::radio::DataCallFailCause::TFT_SYTAX_ERROR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::TFT_SYTAX_ERROR));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UNKNOWN_PDP_CONTEXT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::UNKNOWN_PDP_CONTEXT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::FILTER_SEMANTIC_ERROR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::FILTER_SEMANTIC_ERROR));
static_assert(aidl::android::hardware::radio::DataCallFailCause::FILTER_SYTAX_ERROR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::FILTER_SYTAX_ERROR));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PDP_WITHOUT_ACTIVE_TFT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PDP_WITHOUT_ACTIVE_TFT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ONLY_IPV4_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ONLY_IPV4_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ONLY_IPV6_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ONLY_IPV6_ALLOWED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ONLY_SINGLE_BEARER_ALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::ONLY_SINGLE_BEARER_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ESM_INFO_NOT_RECEIVED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ESM_INFO_NOT_RECEIVED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDN_CONN_DOES_NOT_EXIST ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDN_CONN_DOES_NOT_EXIST));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MULTI_CONN_TO_SAME_PDN_NOT_ALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MULTI_CONN_TO_SAME_PDN_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MAX_ACTIVE_PDP_CONTEXT_REACHED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MAX_ACTIVE_PDP_CONTEXT_REACHED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UNSUPPORTED_APN_IN_CURRENT_PLMN ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              UNSUPPORTED_APN_IN_CURRENT_PLMN));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_TRANSACTION_ID ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_TRANSACTION_ID));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MESSAGE_INCORRECT_SEMANTIC ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MESSAGE_INCORRECT_SEMANTIC));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_MANDATORY_INFO ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_MANDATORY_INFO));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MESSAGE_TYPE_UNSUPPORTED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MESSAGE_TYPE_UNSUPPORTED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MSG_TYPE_NONCOMPATIBLE_STATE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MSG_TYPE_NONCOMPATIBLE_STATE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UNKNOWN_INFO_ELEMENT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::UNKNOWN_INFO_ELEMENT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CONDITIONAL_IE_ERROR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CONDITIONAL_IE_ERROR));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PROTOCOL_ERRORS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PROTOCOL_ERRORS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::APN_TYPE_CONFLICT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::APN_TYPE_CONFLICT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_PCSCF_ADDR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_PCSCF_ADDR));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::INTERNAL_CALL_PREEMPT_BY_HIGH_PRIO_APN ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        INTERNAL_CALL_PREEMPT_BY_HIGH_PRIO_APN));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMM_ACCESS_BARRED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMM_ACCESS_BARRED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMERGENCY_IFACE_ONLY ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMERGENCY_IFACE_ONLY));
static_assert(aidl::android::hardware::radio::DataCallFailCause::IFACE_MISMATCH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::IFACE_MISMATCH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::COMPANION_IFACE_IN_USE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::COMPANION_IFACE_IN_USE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::IP_ADDRESS_MISMATCH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::IP_ADDRESS_MISMATCH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::IFACE_AND_POL_FAMILY_MISMATCH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              IFACE_AND_POL_FAMILY_MISMATCH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMM_ACCESS_BARRED_INFINITE_RETRY ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              EMM_ACCESS_BARRED_INFINITE_RETRY));
static_assert(aidl::android::hardware::radio::DataCallFailCause::AUTH_FAILURE_ON_EMERGENCY_CALL ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              AUTH_FAILURE_ON_EMERGENCY_CALL));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_1 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_1));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_2 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_2));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_3 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_3));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_4 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_4));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_5 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_5));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_6 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_6));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_7 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_7));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_8 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_8));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_9 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_9));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_10 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_10));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_11 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_11));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_12 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_12));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_13 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_13));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_14 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_14));
static_assert(aidl::android::hardware::radio::DataCallFailCause::OEM_DCFAILCAUSE_15 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::OEM_DCFAILCAUSE_15));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::VOICE_REGISTRATION_FAIL ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::VOICE_REGISTRATION_FAIL));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DATA_REGISTRATION_FAIL ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DATA_REGISTRATION_FAIL));
static_assert(aidl::android::hardware::radio::DataCallFailCause::SIGNAL_LOST ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::SIGNAL_LOST));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PREF_RADIO_TECH_CHANGED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PREF_RADIO_TECH_CHANGED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RADIO_POWER_OFF ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::RADIO_POWER_OFF));
static_assert(aidl::android::hardware::radio::DataCallFailCause::TETHERED_CALL_ACTIVE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::TETHERED_CALL_ACTIVE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ERROR_UNSPECIFIED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ERROR_UNSPECIFIED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::LLC_SNDCP ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::LLC_SNDCP));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ACTIVATION_REJECTED_BCM_VIOLATION ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        ACTIVATION_REJECTED_BCM_VIOLATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      COLLISION_WITH_NETWORK_INITIATED_REQUEST ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              COLLISION_WITH_NETWORK_INITIATED_REQUEST));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ONLY_IPV4V6_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ONLY_IPV4V6_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ONLY_NON_IP_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ONLY_NON_IP_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UNSUPPORTED_QCI_VALUE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::UNSUPPORTED_QCI_VALUE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::BEARER_HANDLING_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              BEARER_HANDLING_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_DNS_ADDR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_DNS_ADDR));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::INVALID_PCSCF_OR_DNS_ADDRESS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_PCSCF_OR_DNS_ADDRESS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CALL_PREEMPT_BY_EMERGENCY_APN ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              CALL_PREEMPT_BY_EMERGENCY_APN));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::UE_INITIATED_DETACH_OR_DISCONNECT ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        UE_INITIATED_DETACH_OR_DISCONNECT));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_REASON_UNSPECIFIED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_REASON_UNSPECIFIED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_ADMIN_PROHIBITED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_ADMIN_PROHIBITED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_FA_INSUFFICIENT_RESOURCES ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_FA_INSUFFICIENT_RESOURCES));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      MIP_FA_MOBILE_NODE_AUTHENTICATION_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_FA_MOBILE_NODE_AUTHENTICATION_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      MIP_FA_HOME_AGENT_AUTHENTICATION_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_FA_HOME_AGENT_AUTHENTICATION_FAILURE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_REQUESTED_LIFETIME_TOO_LONG ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MIP_FA_REQUESTED_LIFETIME_TOO_LONG));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_MALFORMED_REQUEST ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_MALFORMED_REQUEST));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_FA_MALFORMED_REPLY ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_MALFORMED_REPLY));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_FA_ENCAPSULATION_UNAVAILABLE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_FA_ENCAPSULATION_UNAVAILABLE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      MIP_FA_VJ_HEADER_COMPRESSION_UNAVAILABLE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_FA_VJ_HEADER_COMPRESSION_UNAVAILABLE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_REVERSE_TUNNEL_UNAVAILABLE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MIP_FA_REVERSE_TUNNEL_UNAVAILABLE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_REVERSE_TUNNEL_IS_MANDATORY ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MIP_FA_REVERSE_TUNNEL_IS_MANDATORY));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_DELIVERY_STYLE_NOT_SUPPORTED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MIP_FA_DELIVERY_STYLE_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_FA_MISSING_NAI ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_MISSING_NAI));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_MISSING_HOME_AGENT ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_MISSING_HOME_AGENT));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_MISSING_HOME_ADDRESS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_MISSING_HOME_ADDRESS));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_UNKNOWN_CHALLENGE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_UNKNOWN_CHALLENGE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_FA_MISSING_CHALLENGE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_MISSING_CHALLENGE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_FA_STALE_CHALLENGE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MIP_FA_STALE_CHALLENGE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_HA_REASON_UNSPECIFIED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_HA_REASON_UNSPECIFIED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_HA_ADMIN_PROHIBITED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_HA_ADMIN_PROHIBITED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_HA_INSUFFICIENT_RESOURCES ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_HA_INSUFFICIENT_RESOURCES));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      MIP_HA_MOBILE_NODE_AUTHENTICATION_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_HA_MOBILE_NODE_AUTHENTICATION_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      MIP_HA_FOREIGN_AGENT_AUTHENTICATION_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_HA_FOREIGN_AGENT_AUTHENTICATION_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_HA_REGISTRATION_ID_MISMATCH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_HA_REGISTRATION_ID_MISMATCH));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_HA_MALFORMED_REQUEST ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MIP_HA_MALFORMED_REQUEST));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_HA_UNKNOWN_HOME_AGENT_ADDRESS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MIP_HA_UNKNOWN_HOME_AGENT_ADDRESS));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_HA_REVERSE_TUNNEL_UNAVAILABLE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MIP_HA_REVERSE_TUNNEL_UNAVAILABLE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MIP_HA_REVERSE_TUNNEL_IS_MANDATORY ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MIP_HA_REVERSE_TUNNEL_IS_MANDATORY));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_HA_ENCAPSULATION_UNAVAILABLE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MIP_HA_ENCAPSULATION_UNAVAILABLE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CLOSE_IN_PROGRESS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CLOSE_IN_PROGRESS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NETWORK_INITIATED_TERMINATION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              NETWORK_INITIATED_TERMINATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MODEM_APP_PREEMPTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MODEM_APP_PREEMPTED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDN_IPV4_CALL_DISALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDN_IPV4_CALL_DISALLOWED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDN_IPV4_CALL_THROTTLED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDN_IPV4_CALL_THROTTLED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDN_IPV6_CALL_DISALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDN_IPV6_CALL_DISALLOWED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDN_IPV6_CALL_THROTTLED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDN_IPV6_CALL_THROTTLED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MODEM_RESTART ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MODEM_RESTART));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PDP_PPP_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PDP_PPP_NOT_SUPPORTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UNPREFERRED_RAT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::UNPREFERRED_RAT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PHYSICAL_LINK_CLOSE_IN_PROGRESS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              PHYSICAL_LINK_CLOSE_IN_PROGRESS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::APN_PENDING_HANDOVER ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::APN_PENDING_HANDOVER));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PROFILE_BEARER_INCOMPATIBLE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PROFILE_BEARER_INCOMPATIBLE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::SIM_CARD_CHANGED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::SIM_CARD_CHANGED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::LOW_POWER_MODE_OR_POWERING_DOWN ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              LOW_POWER_MODE_OR_POWERING_DOWN));
static_assert(aidl::android::hardware::radio::DataCallFailCause::APN_DISABLED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::APN_DISABLED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MAX_PPP_INACTIVITY_TIMER_EXPIRED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MAX_PPP_INACTIVITY_TIMER_EXPIRED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::IPV6_ADDRESS_TRANSFER_FAILED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::IPV6_ADDRESS_TRANSFER_FAILED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::TRAT_SWAP_FAILED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::TRAT_SWAP_FAILED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EHRPD_TO_HRPD_FALLBACK ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EHRPD_TO_HRPD_FALLBACK));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MIP_CONFIG_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MIP_CONFIG_FAILURE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDN_INACTIVITY_TIMER_EXPIRED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDN_INACTIVITY_TIMER_EXPIRED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MAX_IPV4_CONNECTIONS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MAX_IPV4_CONNECTIONS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MAX_IPV6_CONNECTIONS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MAX_IPV6_CONNECTIONS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::APN_MISMATCH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::APN_MISMATCH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::IP_VERSION_MISMATCH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::IP_VERSION_MISMATCH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DUN_CALL_DISALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DUN_CALL_DISALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INTERNAL_EPC_NONEPC_TRANSITION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              INTERNAL_EPC_NONEPC_TRANSITION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INTERFACE_IN_USE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INTERFACE_IN_USE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::APN_DISALLOWED_ON_ROAMING ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::APN_DISALLOWED_ON_ROAMING));
static_assert(aidl::android::hardware::radio::DataCallFailCause::APN_PARAMETERS_CHANGED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::APN_PARAMETERS_CHANGED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NULL_APN_DISALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NULL_APN_DISALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::THERMAL_MITIGATION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::THERMAL_MITIGATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DATA_SETTINGS_DISABLED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DATA_SETTINGS_DISABLED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DATA_ROAMING_SETTINGS_DISABLED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              DATA_ROAMING_SETTINGS_DISABLED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DDS_SWITCHED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DDS_SWITCHED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::FORBIDDEN_APN_NAME ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::FORBIDDEN_APN_NAME));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DDS_SWITCH_IN_PROGRESS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DDS_SWITCH_IN_PROGRESS));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::CALL_DISALLOWED_IN_ROAMING ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::CALL_DISALLOWED_IN_ROAMING));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NON_IP_NOT_SUPPORTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NON_IP_NOT_SUPPORTED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDN_NON_IP_CALL_THROTTLED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDN_NON_IP_CALL_THROTTLED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDN_NON_IP_CALL_DISALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDN_NON_IP_CALL_DISALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CDMA_LOCK ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CDMA_LOCK));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CDMA_INTERCEPT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CDMA_INTERCEPT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CDMA_REORDER ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CDMA_REORDER));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CDMA_RELEASE_DUE_TO_SO_REJECTION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              CDMA_RELEASE_DUE_TO_SO_REJECTION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CDMA_INCOMING_CALL ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CDMA_INCOMING_CALL));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CDMA_ALERT_STOP ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CDMA_ALERT_STOP));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::CHANNEL_ACQUISITION_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::CHANNEL_ACQUISITION_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MAX_ACCESS_PROBE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MAX_ACCESS_PROBE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      CONCURRENT_SERVICE_NOT_SUPPORTED_BY_BASE_STATION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              CONCURRENT_SERVICE_NOT_SUPPORTED_BY_BASE_STATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NO_RESPONSE_FROM_BASE_STATION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              NO_RESPONSE_FROM_BASE_STATION));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::REJECTED_BY_BASE_STATION ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::REJECTED_BY_BASE_STATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CONCURRENT_SERVICES_INCOMPATIBLE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              CONCURRENT_SERVICES_INCOMPATIBLE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NO_CDMA_SERVICE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NO_CDMA_SERVICE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RUIM_NOT_PRESENT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::RUIM_NOT_PRESENT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CDMA_RETRY_ORDER ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CDMA_RETRY_ORDER));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ACCESS_BLOCK ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ACCESS_BLOCK));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ACCESS_BLOCK_ALL ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ACCESS_BLOCK_ALL));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::IS707B_MAX_ACCESS_PROBES ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::IS707B_MAX_ACCESS_PROBES));
static_assert(aidl::android::hardware::radio::DataCallFailCause::THERMAL_EMERGENCY ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::THERMAL_EMERGENCY));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CONCURRENT_SERVICES_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              CONCURRENT_SERVICES_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INCOMING_CALL_REJECTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INCOMING_CALL_REJECTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NO_SERVICE_ON_GATEWAY ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NO_SERVICE_ON_GATEWAY));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NO_GPRS_CONTEXT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NO_GPRS_CONTEXT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ILLEGAL_MS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ILLEGAL_MS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ILLEGAL_ME ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ILLEGAL_ME));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::GPRS_SERVICES_NOT_ALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::GPRS_SERVICES_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK));
static_assert(aidl::android::hardware::radio::DataCallFailCause::IMPLICITLY_DETACHED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::IMPLICITLY_DETACHED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PLMN_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PLMN_NOT_ALLOWED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::LOCATION_AREA_NOT_ALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::LOCATION_AREA_NOT_ALLOWED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PDP_DUPLICATE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PDP_DUPLICATE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UE_RAT_CHANGE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::UE_RAT_CHANGE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CONGESTION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CONGESTION));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::NO_PDP_CONTEXT_ACTIVATED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::NO_PDP_CONTEXT_ACTIVATED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ACCESS_CLASS_DSAC_REJECTION ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::ACCESS_CLASS_DSAC_REJECTION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PDP_ACTIVATE_MAX_RETRY_FAILED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              PDP_ACTIVATE_MAX_RETRY_FAILED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RADIO_ACCESS_BEARER_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::RADIO_ACCESS_BEARER_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ESM_UNKNOWN_EPS_BEARER_CONTEXT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              ESM_UNKNOWN_EPS_BEARER_CONTEXT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DRB_RELEASED_BY_RRC ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DRB_RELEASED_BY_RRC));
static_assert(aidl::android::hardware::radio::DataCallFailCause::CONNECTION_RELEASED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::CONNECTION_RELEASED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMM_DETACHED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMM_DETACHED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMM_ATTACH_FAILED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMM_ATTACH_FAILED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMM_ATTACH_STARTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMM_ATTACH_STARTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::LTE_NAS_SERVICE_REQUEST_FAILED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              LTE_NAS_SERVICE_REQUEST_FAILED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DUPLICATE_BEARER_ID ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DUPLICATE_BEARER_ID));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ESM_COLLISION_SCENARIOS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::ESM_COLLISION_SCENARIOS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      ESM_BEARER_DEACTIVATED_TO_SYNC_WITH_NETWORK ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              ESM_BEARER_DEACTIVATED_TO_SYNC_WITH_NETWORK));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      ESM_NW_ACTIVATED_DED_BEARER_WITH_ID_OF_DEF_BEARER ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              ESM_NW_ACTIVATED_DED_BEARER_WITH_ID_OF_DEF_BEARER));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ESM_BAD_OTA_MESSAGE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ESM_BAD_OTA_MESSAGE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ESM_DOWNLOAD_SERVER_REJECTED_THE_CALL ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        ESM_DOWNLOAD_SERVER_REJECTED_THE_CALL));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ESM_CONTEXT_TRANSFERRED_DUE_TO_IRAT ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        ESM_CONTEXT_TRANSFERRED_DUE_TO_IRAT));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::DS_EXPLICIT_DEACTIVATION ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::DS_EXPLICIT_DEACTIVATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ESM_LOCAL_CAUSE_NONE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ESM_LOCAL_CAUSE_NONE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::LTE_THROTTLING_NOT_REQUIRED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::LTE_THROTTLING_NOT_REQUIRED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ACCESS_CONTROL_LIST_CHECK_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        ACCESS_CONTROL_LIST_CHECK_FAILURE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::SERVICE_NOT_ALLOWED_ON_PLMN ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::SERVICE_NOT_ALLOWED_ON_PLMN));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMM_T3417_EXPIRED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMM_T3417_EXPIRED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMM_T3417_EXT_EXPIRED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMM_T3417_EXT_EXPIRED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_UPLINK_DATA_TRANSMISSION_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        RRC_UPLINK_DATA_TRANSMISSION_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_UPLINK_DELIVERY_FAILED_DUE_TO_HANDOVER ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_UPLINK_DELIVERY_FAILED_DUE_TO_HANDOVER));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_UPLINK_CONNECTION_RELEASE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_UPLINK_CONNECTION_RELEASE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_UPLINK_RADIO_LINK_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_UPLINK_RADIO_LINK_FAILURE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_UPLINK_ERROR_REQUEST_FROM_NAS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        RRC_UPLINK_ERROR_REQUEST_FROM_NAS));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_ACCESS_STRATUM_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        RRC_CONNECTION_ACCESS_STRATUM_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_CONNECTION_ANOTHER_PROCEDURE_IN_PROGRESS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_ANOTHER_PROCEDURE_IN_PROGRESS));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_ACCESS_BARRED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::RRC_CONNECTION_ACCESS_BARRED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_CELL_RESELECTION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_CELL_RESELECTION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_CONFIG_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_CONFIG_FAILURE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_TIMER_EXPIRED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::RRC_CONNECTION_TIMER_EXPIRED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_LINK_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::RRC_CONNECTION_LINK_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_CELL_NOT_CAMPED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_CELL_NOT_CAMPED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_SYSTEM_INTERVAL_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        RRC_CONNECTION_SYSTEM_INTERVAL_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_REJECT_BY_NETWORK ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_REJECT_BY_NETWORK));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_NORMAL_RELEASE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_NORMAL_RELEASE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_RADIO_LINK_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        RRC_CONNECTION_RADIO_LINK_FAILURE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_REESTABLISHMENT_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        RRC_CONNECTION_REESTABLISHMENT_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_CONNECTION_OUT_OF_SERVICE_DURING_CELL_REGISTER ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_OUT_OF_SERVICE_DURING_CELL_REGISTER));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_ABORT_REQUEST ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::RRC_CONNECTION_ABORT_REQUEST));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_CONNECTION_SYSTEM_INFORMATION_BLOCK_READ_ERROR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_SYSTEM_INFORMATION_BLOCK_READ_ERROR));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      NETWORK_INITIATED_DETACH_WITH_AUTO_REATTACH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              NETWORK_INITIATED_DETACH_WITH_AUTO_REATTACH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      NETWORK_INITIATED_DETACH_NO_AUTO_REATTACH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              NETWORK_INITIATED_DETACH_NO_AUTO_REATTACH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ESM_PROCEDURE_TIME_OUT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ESM_PROCEDURE_TIME_OUT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_CONNECTION_ID ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_CONNECTION_ID));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MAXIMIUM_NSAPIS_EXCEEDED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MAXIMIUM_NSAPIS_EXCEEDED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_PRIMARY_NSAPI ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_PRIMARY_NSAPI));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::CANNOT_ENCODE_OTA_MESSAGE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::CANNOT_ENCODE_OTA_MESSAGE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RADIO_ACCESS_BEARER_SETUP_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        RADIO_ACCESS_BEARER_SETUP_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PDP_ESTABLISH_TIMEOUT_EXPIRED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              PDP_ESTABLISH_TIMEOUT_EXPIRED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDP_MODIFY_TIMEOUT_EXPIRED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDP_MODIFY_TIMEOUT_EXPIRED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::PDP_INACTIVE_TIMEOUT_EXPIRED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::PDP_INACTIVE_TIMEOUT_EXPIRED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PDP_LOWERLAYER_ERROR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PDP_LOWERLAYER_ERROR));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PDP_MODIFY_COLLISION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PDP_MODIFY_COLLISION));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MAXINUM_SIZE_OF_L2_MESSAGE_EXCEEDED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        MAXINUM_SIZE_OF_L2_MESSAGE_EXCEEDED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NAS_REQUEST_REJECTED_BY_NETWORK ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              NAS_REQUEST_REJECTED_BY_NETWORK));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_INVALID_REQUEST ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_INVALID_REQUEST));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_CONNECTION_TRACKING_AREA_ID_CHANGED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_TRACKING_AREA_ID_CHANGED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_RF_UNAVAILABLE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_RF_UNAVAILABLE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_CONNECTION_ABORTED_DUE_TO_IRAT_CHANGE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_ABORTED_DUE_TO_IRAT_CHANGE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_CONNECTION_RELEASED_SECURITY_NOT_ACTIVE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_RELEASED_SECURITY_NOT_ACTIVE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::RRC_CONNECTION_ABORTED_AFTER_HANDOVER ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        RRC_CONNECTION_ABORTED_AFTER_HANDOVER));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_CONNECTION_ABORTED_AFTER_IRAT_CELL_CHANGE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_ABORTED_AFTER_IRAT_CELL_CHANGE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      RRC_CONNECTION_ABORTED_DURING_IRAT_CELL_CHANGE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              RRC_CONNECTION_ABORTED_DURING_IRAT_CELL_CHANGE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::IMSI_UNKNOWN_IN_HOME_SUBSCRIBER_SERVER ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        IMSI_UNKNOWN_IN_HOME_SUBSCRIBER_SERVER));
static_assert(aidl::android::hardware::radio::DataCallFailCause::IMEI_NOT_ACCEPTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::IMEI_NOT_ACCEPTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EPS_SERVICES_NOT_ALLOWED_IN_PLMN ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              EPS_SERVICES_NOT_ALLOWED_IN_PLMN));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MSC_TEMPORARILY_NOT_REACHABLE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MSC_TEMPORARILY_NOT_REACHABLE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::CS_DOMAIN_NOT_AVAILABLE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::CS_DOMAIN_NOT_AVAILABLE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::ESM_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::ESM_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MAC_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::MAC_FAILURE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::SYNCHRONIZATION_FAILURE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::SYNCHRONIZATION_FAILURE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::UE_SECURITY_CAPABILITIES_MISMATCH ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        UE_SECURITY_CAPABILITIES_MISMATCH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::SECURITY_MODE_REJECTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::SECURITY_MODE_REJECTED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::UNACCEPTABLE_NON_EPS_AUTHENTICATION ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        UNACCEPTABLE_NON_EPS_AUTHENTICATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      CS_FALLBACK_CALL_ESTABLISHMENT_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              CS_FALLBACK_CALL_ESTABLISHMENT_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NO_EPS_BEARER_CONTEXT_ACTIVATED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              NO_EPS_BEARER_CONTEXT_ACTIVATED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_EMM_STATE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_EMM_STATE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NAS_LAYER_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NAS_LAYER_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::MULTIPLE_PDP_CALL_NOT_ALLOWED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              MULTIPLE_PDP_CALL_NOT_ALLOWED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMBMS_NOT_ENABLED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMBMS_NOT_ENABLED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::IRAT_HANDOVER_FAILED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::IRAT_HANDOVER_FAILED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::EMBMS_REGULAR_DEACTIVATION ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::EMBMS_REGULAR_DEACTIVATION));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::TEST_LOOPBACK_REGULAR_DEACTIVATION ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        TEST_LOOPBACK_REGULAR_DEACTIVATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::LOWER_LAYER_REGISTRATION_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              LOWER_LAYER_REGISTRATION_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DATA_PLAN_EXPIRED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DATA_PLAN_EXPIRED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UMTS_HANDOVER_TO_IWLAN ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::UMTS_HANDOVER_TO_IWLAN));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      EVDO_CONNECTION_DENY_BY_GENERAL_OR_NETWORK_BUSY ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              EVDO_CONNECTION_DENY_BY_GENERAL_OR_NETWORK_BUSY));
static_assert(aidl::android::hardware::radio::DataCallFailCause::
                      EVDO_CONNECTION_DENY_BY_BILLING_OR_AUTHENTICATION_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              EVDO_CONNECTION_DENY_BY_BILLING_OR_AUTHENTICATION_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EVDO_HDR_CHANGED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EVDO_HDR_CHANGED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EVDO_HDR_EXITED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EVDO_HDR_EXITED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EVDO_HDR_NO_SESSION ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EVDO_HDR_NO_SESSION));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::EVDO_USING_GPS_FIX_INSTEAD_OF_HDR_CALL ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        EVDO_USING_GPS_FIX_INSTEAD_OF_HDR_CALL));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::EVDO_HDR_CONNECTION_SETUP_TIMEOUT ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        EVDO_HDR_CONNECTION_SETUP_TIMEOUT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::FAILED_TO_ACQUIRE_COLOCATED_HDR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              FAILED_TO_ACQUIRE_COLOCATED_HDR));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::OTASP_COMMIT_IN_PROGRESS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::OTASP_COMMIT_IN_PROGRESS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NO_HYBRID_HDR_SERVICE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NO_HYBRID_HDR_SERVICE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::HDR_NO_LOCK_GRANTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::HDR_NO_LOCK_GRANTED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DBM_OR_SMS_IN_PROGRESS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DBM_OR_SMS_IN_PROGRESS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::HDR_FADE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::HDR_FADE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::HDR_ACCESS_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::HDR_ACCESS_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UNSUPPORTED_1X_PREV ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::UNSUPPORTED_1X_PREV));
static_assert(aidl::android::hardware::radio::DataCallFailCause::LOCAL_END ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::LOCAL_END));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NO_SERVICE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NO_SERVICE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::FADE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::FADE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NORMAL_RELEASE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NORMAL_RELEASE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ACCESS_ATTEMPT_ALREADY_IN_PROGRESS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        ACCESS_ATTEMPT_ALREADY_IN_PROGRESS));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::REDIRECTION_OR_HANDOFF_IN_PROGRESS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        REDIRECTION_OR_HANDOFF_IN_PROGRESS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::EMERGENCY_MODE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::EMERGENCY_MODE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PHONE_IN_USE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PHONE_IN_USE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_MODE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_MODE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::INVALID_SIM_STATE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::INVALID_SIM_STATE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::NO_COLLOCATED_HDR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::NO_COLLOCATED_HDR));
static_assert(aidl::android::hardware::radio::DataCallFailCause::UE_IS_ENTERING_POWERSAVE_MODE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              UE_IS_ENTERING_POWERSAVE_MODE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::DUAL_SWITCH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::DUAL_SWITCH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PPP_TIMEOUT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PPP_TIMEOUT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PPP_AUTH_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PPP_AUTH_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PPP_OPTION_MISMATCH ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PPP_OPTION_MISMATCH));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PPP_PAP_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PPP_PAP_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PPP_CHAP_FAILURE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PPP_CHAP_FAILURE));
static_assert(aidl::android::hardware::radio::DataCallFailCause::PPP_CLOSE_IN_PROGRESS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::PPP_CLOSE_IN_PROGRESS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::LIMITED_TO_IPV4 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::LIMITED_TO_IPV4));
static_assert(aidl::android::hardware::radio::DataCallFailCause::LIMITED_TO_IPV6 ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::LIMITED_TO_IPV6));
static_assert(aidl::android::hardware::radio::DataCallFailCause::VSNCP_TIMEOUT ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_TIMEOUT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::VSNCP_GEN_ERROR ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_GEN_ERROR));
static_assert(aidl::android::hardware::radio::DataCallFailCause::VSNCP_APN_UNAUTHORIZED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_APN_UNATHORIZED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::VSNCP_PDN_LIMIT_EXCEEDED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_PDN_LIMIT_EXCEEDED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::VSNCP_NO_PDN_GATEWAY_ADDRESS ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_NO_PDN_GATEWAY_ADDRESS));
static_assert(aidl::android::hardware::radio::DataCallFailCause::VSNCP_PDN_GATEWAY_UNREACHABLE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              VSNCP_PDN_GATEWAY_UNREACHABLE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::VSNCP_PDN_GATEWAY_REJECT ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_PDN_GATEWAY_REJECT));
static_assert(aidl::android::hardware::radio::DataCallFailCause::VSNCP_INSUFFICIENT_PARAMETERS ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              VSNCP_INSUFFICIENT_PARAMETERS));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::VSNCP_RESOURCE_UNAVAILABLE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_RESOURCE_UNAVAILABLE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::VSNCP_ADMINISTRATIVELY_PROHIBITED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::
                        VSNCP_ADMINISTRATIVELY_PROHIBITED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::VSNCP_PDN_ID_IN_USE ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_PDN_ID_IN_USE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::VSNCP_SUBSCRIBER_LIMITATION ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_SUBSCRIBER_LIMITATION));
static_assert(aidl::android::hardware::radio::DataCallFailCause::VSNCP_PDN_EXISTS_FOR_THIS_APN ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::
                              VSNCP_PDN_EXISTS_FOR_THIS_APN));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::VSNCP_RECONNECT_NOT_ALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::VSNCP_RECONNECT_NOT_ALLOWED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::IPV6_PREFIX_UNAVAILABLE ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::IPV6_PREFIX_UNAVAILABLE));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::HANDOFF_PREFERENCE_CHANGED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::HANDOFF_PREFERENCE_CHANGED));
static_assert(aidl::android::hardware::radio::DataCallFailCause::SLICE_REJECTED ==
              static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                      ::android::hardware::radio::V1_6::DataCallFailCause::SLICE_REJECTED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::MATCH_ALL_RULE_NOT_ALLOWED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::MATCH_ALL_RULE_NOT_ALLOWED));
static_assert(
        aidl::android::hardware::radio::DataCallFailCause::ALL_MATCHING_RULES_FAILED ==
        static_cast<aidl::android::hardware::radio::DataCallFailCause>(
                ::android::hardware::radio::V1_6::DataCallFailCause::ALL_MATCHING_RULES_FAILED));

static_assert(aidl::android::hardware::radio::SliceStatus::UNKNOWN ==
              static_cast<aidl::android::hardware::radio::SliceStatus>(
                      ::android::hardware::radio::V1_6::SliceStatus::UNKNOWN));
static_assert(aidl::android::hardware::radio::SliceStatus::CONFIGURED ==
              static_cast<aidl::android::hardware::radio::SliceStatus>(
                      ::android::hardware::radio::V1_6::SliceStatus::CONFIGURED));
static_assert(aidl::android::hardware::radio::SliceStatus::ALLOWED ==
              static_cast<aidl::android::hardware::radio::SliceStatus>(
                      ::android::hardware::radio::V1_6::SliceStatus::ALLOWED));
static_assert(
        aidl::android::hardware::radio::SliceStatus::REJECTED_NOT_AVAILABLE_IN_PLMN ==
        static_cast<aidl::android::hardware::radio::SliceStatus>(
                ::android::hardware::radio::V1_6::SliceStatus::REJECTED_NOT_AVAILABLE_IN_PLMN));
static_assert(
        aidl::android::hardware::radio::SliceStatus::REJECTED_NOT_AVAILABLE_IN_REG_AREA ==
        static_cast<aidl::android::hardware::radio::SliceStatus>(
                ::android::hardware::radio::V1_6::SliceStatus::REJECTED_NOT_AVAILABLE_IN_REG_AREA));
static_assert(aidl::android::hardware::radio::SliceStatus::DEFAULT_CONFIGURED ==
              static_cast<aidl::android::hardware::radio::SliceStatus>(
                      ::android::hardware::radio::V1_6::SliceStatus::DEFAULT_CONFIGURED));

static_assert(aidl::android::hardware::radio::SscMode::MODE_1 ==
              static_cast<aidl::android::hardware::radio::SscMode>(
                      ::android::hardware::radio::V1_6::SscMode::MODE_1));
static_assert(aidl::android::hardware::radio::SscMode::MODE_2 ==
              static_cast<aidl::android::hardware::radio::SscMode>(
                      ::android::hardware::radio::V1_6::SscMode::MODE_2));
static_assert(aidl::android::hardware::radio::SscMode::MODE_3 ==
              static_cast<aidl::android::hardware::radio::SscMode>(
                      ::android::hardware::radio::V1_6::SscMode::MODE_3));

static_assert(aidl::android::hardware::radio::PublicKeyType::EPDG ==
              static_cast<aidl::android::hardware::radio::PublicKeyType>(
                      ::android::hardware::radio::V1_6::PublicKeyType::EPDG));
static_assert(aidl::android::hardware::radio::PublicKeyType::WLAN ==
              static_cast<aidl::android::hardware::radio::PublicKeyType>(
                      ::android::hardware::radio::V1_6::PublicKeyType::WLAN));

static_assert(aidl::android::hardware::radio::PbReceivedStatus::PB_RECEIVED_OK ==
              static_cast<aidl::android::hardware::radio::PbReceivedStatus>(
                      ::android::hardware::radio::V1_6::PbReceivedStatus::PB_RECEIVED_OK));
static_assert(aidl::android::hardware::radio::PbReceivedStatus::PB_RECEIVED_ERROR ==
              static_cast<aidl::android::hardware::radio::PbReceivedStatus>(
                      ::android::hardware::radio::V1_6::PbReceivedStatus::PB_RECEIVED_ERROR));
static_assert(aidl::android::hardware::radio::PbReceivedStatus::PB_RECEIVED_ABORT ==
              static_cast<aidl::android::hardware::radio::PbReceivedStatus>(
                      ::android::hardware::radio::V1_6::PbReceivedStatus::PB_RECEIVED_ABORT));
static_assert(aidl::android::hardware::radio::PbReceivedStatus::PB_RECEIVED_FINAL ==
              static_cast<aidl::android::hardware::radio::PbReceivedStatus>(
                      ::android::hardware::radio::V1_6::PbReceivedStatus::PB_RECEIVED_FINAL));

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::IccIo& in,
        aidl::android::hardware::radio::IccIo* out) {
    out->command = static_cast<int32_t>(in.command);
    out->fileId = static_cast<int32_t>(in.fileId);
    out->path = in.path;
    out->p1 = static_cast<int32_t>(in.p1);
    out->p2 = static_cast<int32_t>(in.p2);
    out->p3 = static_cast<int32_t>(in.p3);
    out->data = in.data;
    out->pin2 = in.pin2;
    out->aid = in.aid;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::NeighboringCell& in,
        aidl::android::hardware::radio::NeighboringCell* out) {
    out->cid = in.cid;
    out->rssi = static_cast<int32_t>(in.rssi);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::UusInfo& in,
        aidl::android::hardware::radio::UusInfo* out) {
    out->uusType = static_cast<aidl::android::hardware::radio::UusType>(in.uusType);
    out->uusDcs = static_cast<aidl::android::hardware::radio::UusDcs>(in.uusDcs);
    out->uusData = in.uusData;
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_0::Dial& in,
                                                   aidl::android::hardware::radio::Dial* out) {
    out->address = in.address;
    out->clir = static_cast<aidl::android::hardware::radio::Clir>(in.clir);
    {
        size_t size = in.uusInfo.size();
        aidl::android::hardware::radio::UusInfo uusInfo;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.uusInfo[i], &uusInfo)) return false;
            out->uusInfo.push_back(uusInfo);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LastCallFailCauseInfo& in,
        aidl::android::hardware::radio::LastCallFailCauseInfo* out) {
    out->causeCode = static_cast<aidl::android::hardware::radio::LastCallFailCause>(in.causeCode);
    out->vendorCause = in.vendorCause;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmSignalStrength& in,
        aidl::android::hardware::radio::GsmSignalStrength* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.signalStrength > std::numeric_limits<int32_t>::max() || in.signalStrength < 0) {
        return false;
    }
    out->signalStrength = static_cast<int32_t>(in.signalStrength);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.bitErrorRate > std::numeric_limits<int32_t>::max() || in.bitErrorRate < 0) {
        return false;
    }
    out->bitErrorRate = static_cast<int32_t>(in.bitErrorRate);
    out->timingAdvance = static_cast<int32_t>(in.timingAdvance);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSignalStrength& in,
        aidl::android::hardware::radio::CdmaSignalStrength* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.dbm > std::numeric_limits<int32_t>::max() || in.dbm < 0) {
        return false;
    }
    out->dbm = static_cast<int32_t>(in.dbm);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.ecio > std::numeric_limits<int32_t>::max() || in.ecio < 0) {
        return false;
    }
    out->ecio = static_cast<int32_t>(in.ecio);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::EvdoSignalStrength& in,
        aidl::android::hardware::radio::EvdoSignalStrength* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.dbm > std::numeric_limits<int32_t>::max() || in.dbm < 0) {
        return false;
    }
    out->dbm = static_cast<int32_t>(in.dbm);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.ecio > std::numeric_limits<int32_t>::max() || in.ecio < 0) {
        return false;
    }
    out->ecio = static_cast<int32_t>(in.ecio);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.signalNoiseRatio > std::numeric_limits<int32_t>::max() || in.signalNoiseRatio < 0) {
        return false;
    }
    out->signalNoiseRatio = static_cast<int32_t>(in.signalNoiseRatio);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SendSmsResult& in,
        aidl::android::hardware::radio::SendSmsResult* out) {
    out->messageRef = static_cast<int32_t>(in.messageRef);
    out->ackPDU = in.ackPDU;
    out->errorCode = static_cast<int32_t>(in.errorCode);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::IccIoResult& in,
        aidl::android::hardware::radio::IccIoResult* out) {
    out->sw1 = static_cast<int32_t>(in.sw1);
    out->sw2 = static_cast<int32_t>(in.sw2);
    out->simResponse = in.simResponse;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CallForwardInfo& in,
        aidl::android::hardware::radio::CallForwardInfo* out) {
    out->status = static_cast<aidl::android::hardware::radio::CallForwardInfoStatus>(in.status);
    out->reason = static_cast<int32_t>(in.reason);
    out->serviceClass = static_cast<int32_t>(in.serviceClass);
    out->toa = static_cast<int32_t>(in.toa);
    out->number = in.number;
    out->timeSeconds = static_cast<int32_t>(in.timeSeconds);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::OperatorInfo& in,
        aidl::android::hardware::radio::OperatorInfo* out) {
    out->alphaLong = in.alphaLong;
    out->alphaShort = in.alphaShort;
    out->operatorNumeric = in.operatorNumeric;
    out->status = static_cast<aidl::android::hardware::radio::OperatorStatus>(in.status);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SmsWriteArgs& in,
        aidl::android::hardware::radio::SmsWriteArgs* out) {
    out->status = static_cast<aidl::android::hardware::radio::SmsWriteArgsStatus>(in.status);
    out->pdu = in.pdu;
    out->smsc = in.smsc;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsAddress& in,
        aidl::android::hardware::radio::CdmaSmsAddress* out) {
    out->digitMode = static_cast<aidl::android::hardware::radio::CdmaSmsDigitMode>(in.digitMode);
    out->numberMode = static_cast<aidl::android::hardware::radio::CdmaSmsNumberMode>(in.numberMode);
    out->numberType = static_cast<aidl::android::hardware::radio::CdmaSmsNumberType>(in.numberType);
    out->numberPlan = static_cast<aidl::android::hardware::radio::CdmaSmsNumberPlan>(in.numberPlan);
    {
        size_t size = in.digits.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.digits[i] > std::numeric_limits<int8_t>::max() || in.digits[i] < 0) {
                return false;
            }
            out->digits.push_back(static_cast<int8_t>(in.digits[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsSubaddress& in,
        aidl::android::hardware::radio::CdmaSmsSubaddress* out) {
    out->subaddressType =
            static_cast<aidl::android::hardware::radio::CdmaSmsSubaddressType>(in.subaddressType);
    out->odd = static_cast<bool>(in.odd);
    {
        size_t size = in.digits.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.digits[i] > std::numeric_limits<int8_t>::max() || in.digits[i] < 0) {
                return false;
            }
            out->digits.push_back(static_cast<int8_t>(in.digits[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsMessage& in,
        aidl::android::hardware::radio::CdmaSmsMessage* out) {
    out->teleserviceId = static_cast<int32_t>(in.teleserviceId);
    out->isServicePresent = static_cast<bool>(in.isServicePresent);
    out->serviceCategory = static_cast<int32_t>(in.serviceCategory);
    if (!translate(in.address, &out->address)) return false;
    if (!translate(in.subAddress, &out->subAddress)) return false;
    {
        size_t size = in.bearerData.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.bearerData[i] > std::numeric_limits<int8_t>::max() || in.bearerData[i] < 0) {
                return false;
            }
            out->bearerData.push_back(static_cast<int8_t>(in.bearerData[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsAck& in,
        aidl::android::hardware::radio::CdmaSmsAck* out) {
    out->errorClass = static_cast<aidl::android::hardware::radio::CdmaSmsErrorClass>(in.errorClass);
    out->smsCauseCode = static_cast<int32_t>(in.smsCauseCode);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaBroadcastSmsConfigInfo& in,
        aidl::android::hardware::radio::CdmaBroadcastSmsConfigInfo* out) {
    out->serviceCategory = static_cast<int32_t>(in.serviceCategory);
    out->language = static_cast<int32_t>(in.language);
    out->selected = static_cast<bool>(in.selected);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsWriteArgs& in,
        aidl::android::hardware::radio::CdmaSmsWriteArgs* out) {
    out->status = static_cast<aidl::android::hardware::radio::CdmaSmsWriteArgsStatus>(in.status);
    if (!translate(in.message, &out->message)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmBroadcastSmsConfigInfo& in,
        aidl::android::hardware::radio::GsmBroadcastSmsConfigInfo* out) {
    out->fromServiceId = static_cast<int32_t>(in.fromServiceId);
    out->toServiceId = static_cast<int32_t>(in.toServiceId);
    out->fromCodeScheme = static_cast<int32_t>(in.fromCodeScheme);
    out->toCodeScheme = static_cast<int32_t>(in.toCodeScheme);
    out->selected = static_cast<bool>(in.selected);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmSmsMessage& in,
        aidl::android::hardware::radio::GsmSmsMessage* out) {
    out->smscPdu = in.smscPdu;
    out->pdu = in.pdu;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::ImsSmsMessage& in,
        aidl::android::hardware::radio::ImsSmsMessage* out) {
    out->tech = static_cast<aidl::android::hardware::radio::RadioTechnologyFamily>(in.tech);
    out->retry = static_cast<bool>(in.retry);
    out->messageRef = static_cast<int32_t>(in.messageRef);
    {
        size_t size = in.cdmaMessage.size();
        aidl::android::hardware::radio::CdmaSmsMessage cdmaMessage;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.cdmaMessage[i], &cdmaMessage)) return false;
            out->cdmaMessage.push_back(cdmaMessage);
        }
    }
    {
        size_t size = in.gsmMessage.size();
        aidl::android::hardware::radio::GsmSmsMessage gsmMessage;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.gsmMessage[i], &gsmMessage)) return false;
            out->gsmMessage.push_back(gsmMessage);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SimApdu& in,
        aidl::android::hardware::radio::SimApdu* out) {
    out->sessionId = static_cast<int32_t>(in.sessionId);
    out->cla = static_cast<int32_t>(in.cla);
    out->instruction = static_cast<int32_t>(in.instruction);
    out->p1 = static_cast<int32_t>(in.p1);
    out->p2 = static_cast<int32_t>(in.p2);
    out->p3 = static_cast<int32_t>(in.p3);
    out->data = in.data;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::NvWriteItem& in,
        aidl::android::hardware::radio::NvWriteItem* out) {
    out->itemId = static_cast<aidl::android::hardware::radio::NvItem>(in.itemId);
    out->value = in.value;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SelectUiccSub& in,
        aidl::android::hardware::radio::SelectUiccSub* out) {
    out->slot = static_cast<int32_t>(in.slot);
    out->appIndex = static_cast<int32_t>(in.appIndex);
    out->subType = static_cast<aidl::android::hardware::radio::SubscriptionType>(in.subType);
    out->actStatus = static_cast<aidl::android::hardware::radio::UiccSubActStatus>(in.actStatus);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfigModem& in,
        aidl::android::hardware::radio::HardwareConfigModem* out) {
    out->rilModel = static_cast<int32_t>(in.rilModel);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.rat > std::numeric_limits<int32_t>::max() || in.rat < 0) {
        return false;
    }
    out->rat = static_cast<int32_t>(in.rat);
    out->maxVoice = static_cast<int32_t>(in.maxVoice);
    out->maxData = static_cast<int32_t>(in.maxData);
    out->maxStandby = static_cast<int32_t>(in.maxStandby);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfigSim& in,
        aidl::android::hardware::radio::HardwareConfigSim* out) {
    out->modemUuid = in.modemUuid;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfig& in,
        aidl::android::hardware::radio::HardwareConfig* out) {
    out->type = static_cast<aidl::android::hardware::radio::HardwareConfigType>(in.type);
    out->uuid = in.uuid;
    out->state = static_cast<aidl::android::hardware::radio::HardwareConfigState>(in.state);
    {
        size_t size = in.modem.size();
        aidl::android::hardware::radio::HardwareConfigModem modem;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.modem[i], &modem)) return false;
            out->modem.push_back(modem);
        }
    }
    {
        size_t size = in.sim.size();
        aidl::android::hardware::radio::HardwareConfigSim sim;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.sim[i], &sim)) return false;
            out->sim.push_back(sim);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LceStatusInfo& in,
        aidl::android::hardware::radio::LceStatusInfo* out) {
    out->lceStatus = static_cast<aidl::android::hardware::radio::LceStatus>(in.lceStatus);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.actualIntervalMs > std::numeric_limits<int8_t>::max() || in.actualIntervalMs < 0) {
        return false;
    }
    out->actualIntervalMs = static_cast<int8_t>(in.actualIntervalMs);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LceDataInfo& in,
        aidl::android::hardware::radio::LceDataInfo* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.lastHopCapacityKbps > std::numeric_limits<int32_t>::max() ||
        in.lastHopCapacityKbps < 0) {
        return false;
    }
    out->lastHopCapacityKbps = static_cast<int32_t>(in.lastHopCapacityKbps);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.confidenceLevel > std::numeric_limits<int8_t>::max() || in.confidenceLevel < 0) {
        return false;
    }
    out->confidenceLevel = static_cast<int8_t>(in.confidenceLevel);
    out->lceSuspended = static_cast<bool>(in.lceSuspended);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::ActivityStatsInfo& in,
        aidl::android::hardware::radio::ActivityStatsInfo* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.sleepModeTimeMs > std::numeric_limits<int32_t>::max() || in.sleepModeTimeMs < 0) {
        return false;
    }
    out->sleepModeTimeMs = static_cast<int32_t>(in.sleepModeTimeMs);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.idleModeTimeMs > std::numeric_limits<int32_t>::max() || in.idleModeTimeMs < 0) {
        return false;
    }
    out->idleModeTimeMs = static_cast<int32_t>(in.idleModeTimeMs);
    {
        size_t size = sizeof(in.txmModetimeMs) / sizeof(in.txmModetimeMs[0]);
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.txmModetimeMs[i] > std::numeric_limits<int32_t>::max() ||
                in.txmModetimeMs[i] < 0) {
                return false;
            }
            out->txmModetimeMs.push_back(static_cast<int32_t>(in.txmModetimeMs[i]));
        }
    }
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.rxModeTimeMs > std::numeric_limits<int32_t>::max() || in.rxModeTimeMs < 0) {
        return false;
    }
    out->rxModeTimeMs = static_cast<int32_t>(in.rxModeTimeMs);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::Carrier& in,
        aidl::android::hardware::radio::Carrier* out) {
    out->mcc = in.mcc;
    out->mnc = in.mnc;
    out->matchType = static_cast<aidl::android::hardware::radio::CarrierMatchType>(in.matchType);
    out->matchData = in.matchData;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CarrierRestrictions& in,
        aidl::android::hardware::radio::CarrierRestrictions* out) {
    {
        size_t size = in.allowedCarriers.size();
        aidl::android::hardware::radio::Carrier allowedCarriers;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.allowedCarriers[i], &allowedCarriers)) return false;
            out->allowedCarriers.push_back(allowedCarriers);
        }
    }
    {
        size_t size = in.excludedCarriers.size();
        aidl::android::hardware::radio::Carrier excludedCarriers;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.excludedCarriers[i], &excludedCarriers)) return false;
            out->excludedCarriers.push_back(excludedCarriers);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SuppSvcNotification& in,
        aidl::android::hardware::radio::SuppSvcNotification* out) {
    out->isMT = static_cast<bool>(in.isMT);
    out->code = static_cast<int32_t>(in.code);
    out->index = static_cast<int32_t>(in.index);
    out->type = static_cast<int32_t>(in.type);
    out->number = in.number;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SimRefreshResult& in,
        aidl::android::hardware::radio::SimRefreshResult* out) {
    out->type = static_cast<aidl::android::hardware::radio::SimRefreshType>(in.type);
    out->efId = static_cast<int32_t>(in.efId);
    out->aid = in.aid;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSignalInfoRecord& in,
        aidl::android::hardware::radio::CdmaSignalInfoRecord* out) {
    out->isPresent = static_cast<bool>(in.isPresent);
    out->signalType = static_cast<int8_t>(in.signalType);
    out->alertPitch = static_cast<int8_t>(in.alertPitch);
    out->signal = static_cast<int8_t>(in.signal);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaCallWaiting& in,
        aidl::android::hardware::radio::CdmaCallWaiting* out) {
    out->number = in.number;
    out->numberPresentation =
            static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPresentation>(
                    in.numberPresentation);
    out->name = in.name;
    if (!translate(in.signalInfoRecord, &out->signalInfoRecord)) return false;
    out->numberType =
            static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberType>(in.numberType);
    out->numberPlan =
            static_cast<aidl::android::hardware::radio::CdmaCallWaitingNumberPlan>(in.numberPlan);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaDisplayInfoRecord& in,
        aidl::android::hardware::radio::CdmaDisplayInfoRecord* out) {
    out->alphaBuf = in.alphaBuf;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaNumberInfoRecord& in,
        aidl::android::hardware::radio::CdmaNumberInfoRecord* out) {
    out->number = in.number;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.numberType > std::numeric_limits<int8_t>::max() || in.numberType < 0) {
        return false;
    }
    out->numberType = static_cast<int8_t>(in.numberType);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.numberPlan > std::numeric_limits<int8_t>::max() || in.numberPlan < 0) {
        return false;
    }
    out->numberPlan = static_cast<int8_t>(in.numberPlan);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.pi > std::numeric_limits<int8_t>::max() || in.pi < 0) {
        return false;
    }
    out->pi = static_cast<int8_t>(in.pi);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.si > std::numeric_limits<int8_t>::max() || in.si < 0) {
        return false;
    }
    out->si = static_cast<int8_t>(in.si);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaRedirectingNumberInfoRecord& in,
        aidl::android::hardware::radio::CdmaRedirectingNumberInfoRecord* out) {
    if (!translate(in.redirectingNumber, &out->redirectingNumber)) return false;
    out->redirectingReason = static_cast<aidl::android::hardware::radio::CdmaRedirectingReason>(
            in.redirectingReason);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaLineControlInfoRecord& in,
        aidl::android::hardware::radio::CdmaLineControlInfoRecord* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.lineCtrlPolarityIncluded > std::numeric_limits<int8_t>::max() ||
        in.lineCtrlPolarityIncluded < 0) {
        return false;
    }
    out->lineCtrlPolarityIncluded = static_cast<int8_t>(in.lineCtrlPolarityIncluded);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.lineCtrlToggle > std::numeric_limits<int8_t>::max() || in.lineCtrlToggle < 0) {
        return false;
    }
    out->lineCtrlToggle = static_cast<int8_t>(in.lineCtrlToggle);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.lineCtrlReverse > std::numeric_limits<int8_t>::max() || in.lineCtrlReverse < 0) {
        return false;
    }
    out->lineCtrlReverse = static_cast<int8_t>(in.lineCtrlReverse);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.lineCtrlPowerDenial > std::numeric_limits<int8_t>::max() || in.lineCtrlPowerDenial < 0) {
        return false;
    }
    out->lineCtrlPowerDenial = static_cast<int8_t>(in.lineCtrlPowerDenial);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaT53ClirInfoRecord& in,
        aidl::android::hardware::radio::CdmaT53ClirInfoRecord* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.cause > std::numeric_limits<int8_t>::max() || in.cause < 0) {
        return false;
    }
    out->cause = static_cast<int8_t>(in.cause);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaT53AudioControlInfoRecord& in,
        aidl::android::hardware::radio::CdmaT53AudioControlInfoRecord* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.upLink > std::numeric_limits<int8_t>::max() || in.upLink < 0) {
        return false;
    }
    out->upLink = static_cast<int8_t>(in.upLink);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.downLink > std::numeric_limits<int8_t>::max() || in.downLink < 0) {
        return false;
    }
    out->downLink = static_cast<int8_t>(in.downLink);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaInformationRecord& in,
        aidl::android::hardware::radio::CdmaInformationRecord* out) {
    out->name = static_cast<aidl::android::hardware::radio::CdmaInfoRecName>(in.name);
    {
        size_t size = in.display.size();
        aidl::android::hardware::radio::CdmaDisplayInfoRecord display;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.display[i], &display)) return false;
            out->display.push_back(display);
        }
    }
    {
        size_t size = in.number.size();
        aidl::android::hardware::radio::CdmaNumberInfoRecord number;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.number[i], &number)) return false;
            out->number.push_back(number);
        }
    }
    {
        size_t size = in.signal.size();
        aidl::android::hardware::radio::CdmaSignalInfoRecord signal;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.signal[i], &signal)) return false;
            out->signal.push_back(signal);
        }
    }
    {
        size_t size = in.redir.size();
        aidl::android::hardware::radio::CdmaRedirectingNumberInfoRecord redir;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.redir[i], &redir)) return false;
            out->redir.push_back(redir);
        }
    }
    {
        size_t size = in.lineCtrl.size();
        aidl::android::hardware::radio::CdmaLineControlInfoRecord lineCtrl;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.lineCtrl[i], &lineCtrl)) return false;
            out->lineCtrl.push_back(lineCtrl);
        }
    }
    {
        size_t size = in.clir.size();
        aidl::android::hardware::radio::CdmaT53ClirInfoRecord clir;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.clir[i], &clir)) return false;
            out->clir.push_back(clir);
        }
    }
    {
        size_t size = in.audioCtrl.size();
        aidl::android::hardware::radio::CdmaT53AudioControlInfoRecord audioCtrl;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.audioCtrl[i], &audioCtrl)) return false;
            out->audioCtrl.push_back(audioCtrl);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaInformationRecords& in,
        aidl::android::hardware::radio::CdmaInformationRecords* out) {
    {
        size_t size = in.infoRec.size();
        aidl::android::hardware::radio::CdmaInformationRecord infoRec;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.infoRec[i], &infoRec)) return false;
            out->infoRec.push_back(infoRec);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CfData& in,
        aidl::android::hardware::radio::CfData* out) {
    {
        size_t size = in.cfInfo.size();
        aidl::android::hardware::radio::CallForwardInfo cfInfo;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.cfInfo[i], &cfInfo)) return false;
            out->cfInfo.push_back(cfInfo);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SsInfoData& in,
        aidl::android::hardware::radio::SsInfoData* out) {
    {
        size_t size = in.ssInfo.size();
        for (size_t i = 0; i < size; i++) {
            out->ssInfo.push_back(static_cast<int32_t>(in.ssInfo[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::StkCcUnsolSsResult& in,
        aidl::android::hardware::radio::StkCcUnsolSsResult* out) {
    out->serviceType = static_cast<aidl::android::hardware::radio::SsServiceType>(in.serviceType);
    out->requestType = static_cast<aidl::android::hardware::radio::SsRequestType>(in.requestType);
    out->teleserviceType =
            static_cast<aidl::android::hardware::radio::SsTeleserviceType>(in.teleserviceType);
    out->serviceClass =
            static_cast<aidl::android::hardware::radio::SuppServiceClass>(in.serviceClass);
    out->result = static_cast<aidl::android::hardware::radio::RadioError>(in.result);
    {
        size_t size = in.ssInfo.size();
        aidl::android::hardware::radio::SsInfoData ssInfo;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.ssInfo[i], &ssInfo)) return false;
            out->ssInfo.push_back(ssInfo);
        }
    }
    {
        size_t size = in.cfData.size();
        aidl::android::hardware::radio::CfData cfData;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.cfData[i], &cfData)) return false;
            out->cfData.push_back(cfData);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::PcoDataInfo& in,
        aidl::android::hardware::radio::PcoDataInfo* out) {
    out->cid = static_cast<int32_t>(in.cid);
    out->bearerProto = in.bearerProto;
    out->pcoId = static_cast<int32_t>(in.pcoId);
    {
        size_t size = in.contents.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.contents[i] > std::numeric_limits<int8_t>::max() || in.contents[i] < 0) {
                return false;
            }
            out->contents.push_back(static_cast<int8_t>(in.contents[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_1::KeepaliveRequest& in,
        aidl::android::hardware::radio::KeepaliveRequest* out) {
    out->type = static_cast<aidl::android::hardware::radio::KeepaliveType>(in.type);
    {
        size_t size = in.sourceAddress.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.sourceAddress[i] > std::numeric_limits<int8_t>::max() ||
                in.sourceAddress[i] < 0) {
                return false;
            }
            out->sourceAddress.push_back(static_cast<int8_t>(in.sourceAddress[i]));
        }
    }
    out->sourcePort = static_cast<int32_t>(in.sourcePort);
    {
        size_t size = in.destinationAddress.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.destinationAddress[i] > std::numeric_limits<int8_t>::max() ||
                in.destinationAddress[i] < 0) {
                return false;
            }
            out->destinationAddress.push_back(static_cast<int8_t>(in.destinationAddress[i]));
        }
    }
    out->destinationPort = static_cast<int32_t>(in.destinationPort);
    out->maxKeepaliveIntervalMillis = static_cast<int32_t>(in.maxKeepaliveIntervalMillis);
    out->cid = static_cast<int32_t>(in.cid);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_1::KeepaliveStatus& in,
        aidl::android::hardware::radio::KeepaliveStatus* out) {
    out->sessionHandle = static_cast<int32_t>(in.sessionHandle);
    out->code = static_cast<aidl::android::hardware::radio::KeepaliveStatusCode>(in.code);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellIdentityOperatorNames& in,
        aidl::android::hardware::radio::CellIdentityOperatorNames* out) {
    out->alphaLong = in.alphaLong;
    out->alphaShort = in.alphaShort;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellIdentityCdma& in,
        aidl::android::hardware::radio::CellIdentityCdma* out) {
    out->networkId = static_cast<int32_t>(in.base.networkId);
    out->systemId = static_cast<int32_t>(in.base.systemId);
    out->baseStationId = static_cast<int32_t>(in.base.baseStationId);
    out->longitude = static_cast<int32_t>(in.base.longitude);
    out->latitude = static_cast<int32_t>(in.base.latitude);
    if (!translate(in.operatorNames, &out->operatorNames)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellInfoCdma& in,
        aidl::android::hardware::radio::CellInfoCdma* out) {
    if (!translate(in.cellIdentityCdma, &out->cellIdentityCdma)) return false;
    if (!translate(in.signalStrengthCdma, &out->signalStrengthCdma)) return false;
    if (!translate(in.signalStrengthEvdo, &out->signalStrengthEvdo)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::WcdmaSignalStrength& in,
        aidl::android::hardware::radio::WcdmaSignalStrength* out) {
    out->signalStrength = static_cast<int32_t>(in.base.signalStrength);
    out->bitErrorRate = static_cast<int32_t>(in.base.bitErrorRate);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.rscp > std::numeric_limits<int32_t>::max() || in.rscp < 0) {
        return false;
    }
    out->rscp = static_cast<int32_t>(in.rscp);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.ecno > std::numeric_limits<int32_t>::max() || in.ecno < 0) {
        return false;
    }
    out->ecno = static_cast<int32_t>(in.ecno);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::TdscdmaSignalStrength& in,
        aidl::android::hardware::radio::TdscdmaSignalStrength* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.signalStrength > std::numeric_limits<int32_t>::max() || in.signalStrength < 0) {
        return false;
    }
    out->signalStrength = static_cast<int32_t>(in.signalStrength);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.bitErrorRate > std::numeric_limits<int32_t>::max() || in.bitErrorRate < 0) {
        return false;
    }
    out->bitErrorRate = static_cast<int32_t>(in.bitErrorRate);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.rscp > std::numeric_limits<int32_t>::max() || in.rscp < 0) {
        return false;
    }
    out->rscp = static_cast<int32_t>(in.rscp);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::VoiceRegStateResult& in,
        aidl::android::hardware::radio::VoiceRegStateResult* out) {
    out->regState = static_cast<aidl::android::hardware::radio::RegState>(in.regState);
    out->rat = static_cast<int32_t>(in.rat);
    out->cssSupported = static_cast<bool>(in.cssSupported);
    out->roamingIndicator = static_cast<int32_t>(in.roamingIndicator);
    out->systemIsInPrl = static_cast<int32_t>(in.systemIsInPrl);
    out->defaultRoamingIndicator = static_cast<int32_t>(in.defaultRoamingIndicator);
    out->reasonForDenial = static_cast<int32_t>(in.reasonForDenial);
    // FIXME Unknown type: android.hardware.radio@1.2::CellIdentity
    // That type's package needs to be converted separately and the corresponding translate function
    // should be added here.
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_3::RadioResponseInfoModem& in,
        aidl::android::hardware::radio::RadioResponseInfoModem* out) {
    out->type = static_cast<aidl::android::hardware::radio::RadioResponseType>(in.type);
    out->serial = static_cast<int32_t>(in.serial);
    out->error = static_cast<aidl::android::hardware::radio::RadioError>(in.error);
    out->isEnabled = static_cast<bool>(in.isEnabled);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::EmergencyNumber& in,
        aidl::android::hardware::radio::EmergencyNumber* out) {
    out->number = in.number;
    out->mcc = in.mcc;
    out->mnc = in.mnc;
    out->categories =
            static_cast<aidl::android::hardware::radio::EmergencyServiceCategory>(in.categories);
    {
        size_t size = in.urns.size();
        for (size_t i = 0; i < size; i++) {
            out->urns.push_back(in.urns[i]);
        }
    }
    out->sources = static_cast<aidl::android::hardware::radio::EmergencyNumberSource>(in.sources);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::RadioFrequencyInfo& in,
        aidl::android::hardware::radio::RadioFrequencyInfo* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_4::RadioFrequencyInfo::hidl_discriminator::range:
            *out = static_cast<aidl::android::hardware::radio::FrequencyRange>(in.range());
            break;
        case ::android::hardware::radio::V1_4::RadioFrequencyInfo::hidl_discriminator::
                channelNumber:
            *out = static_cast<int32_t>(in.channelNumber());
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::LteVopsInfo& in,
        aidl::android::hardware::radio::LteVopsInfo* out) {
    out->isVopsSupported = static_cast<bool>(in.isVopsSupported);
    out->isEmcBearerSupported = static_cast<bool>(in.isEmcBearerSupported);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::NrIndicators& in,
        aidl::android::hardware::radio::NrIndicators* out) {
    out->isEndcAvailable = static_cast<bool>(in.isEndcAvailable);
    out->isDcNrRestricted = static_cast<bool>(in.isDcNrRestricted);
    out->isNrAvailable = static_cast<bool>(in.isNrAvailable);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::DataRegStateResult& in,
        aidl::android::hardware::radio::DataRegStateResult* out) {
    out->regState = static_cast<aidl::android::hardware::radio::RegState>(in.base.regState);
    out->rat = static_cast<int32_t>(in.base.rat);
    out->reasonDataDenied = static_cast<int32_t>(in.base.reasonDataDenied);
    out->maxDataCalls = static_cast<int32_t>(in.base.maxDataCalls);
    // FIXME Unknown type: android.hardware.radio@1.2::CellIdentity
    // That type's package needs to be converted separately and the corresponding translate function
    // should be added here.
    if (!translate(in.vopsInfo, &out->vopsInfo)) return false;
    if (!translate(in.nrIndicators, &out->nrIndicators)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::DataRegStateResult::VopsInfo& in,
        aidl::android::hardware::radio::DataRegStateResultVopsInfo* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_4::DataRegStateResult::VopsInfo::hidl_discriminator::
                noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_4::DataRegStateResult::VopsInfo::hidl_discriminator::
                lteVopsInfo: {
            aidl::android::hardware::radio::LteVopsInfo lteVopsInfo;
            if (!translate(in.lteVopsInfo(), &lteVopsInfo)) return false;
            out->set<aidl::android::hardware::radio::DataRegStateResultVopsInfo::lteVopsInfo>(
                    lteVopsInfo);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CellConfigLte& in,
        aidl::android::hardware::radio::CellConfigLte* out) {
    out->isEndcAvailable = static_cast<bool>(in.isEndcAvailable);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CellInfo::Info& in,
        aidl::android::hardware::radio::CellInfoInfo* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_4::CellInfo::Info::hidl_discriminator::gsm:
            // FIXME Unknown type: android.hardware.radio@1.2::CellInfoGsm
            // That type's package needs to be converted separately and the corresponding translate
            // function should be added here.
            break;
        case ::android::hardware::radio::V1_4::CellInfo::Info::hidl_discriminator::cdma: {
            aidl::android::hardware::radio::CellInfoCdma cdma;
            if (!translate(in.cdma(), &cdma)) return false;
            out->set<aidl::android::hardware::radio::CellInfoInfo::cdma>(cdma);
        } break;
        case ::android::hardware::radio::V1_4::CellInfo::Info::hidl_discriminator::wcdma:
            // FIXME Unknown type: android.hardware.radio@1.2::CellInfoWcdma
            // That type's package needs to be converted separately and the corresponding translate
            // function should be added here.
            break;
        case ::android::hardware::radio::V1_4::CellInfo::Info::hidl_discriminator::tdscdma:
            // FIXME Unknown type: android.hardware.radio@1.2::CellInfoTdscdma
            // That type's package needs to be converted separately and the corresponding translate
            // function should be added here.
            break;
        case ::android::hardware::radio::V1_4::CellInfo::Info::hidl_discriminator::lte:
            // FIXME Unknown type: android.hardware.radio@1.4::CellInfoLte
            // That type's package needs to be converted separately and the corresponding translate
            // function should be added here.
            break;
        case ::android::hardware::radio::V1_4::CellInfo::Info::hidl_discriminator::nr:
            // FIXME Unknown type: android.hardware.radio@1.4::CellInfoNr
            // That type's package needs to be converted separately and the corresponding translate
            // function should be added here.
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::RadioCapability& in,
        aidl::android::hardware::radio::RadioCapability* out) {
    out->session = static_cast<int32_t>(in.session);
    out->phase = static_cast<aidl::android::hardware::radio::RadioCapabilityPhase>(in.phase);
    out->raf = static_cast<aidl::android::hardware::radio::RadioAccessFamily>(in.raf);
    out->logicalModemUuid = in.logicalModemUuid;
    out->status = static_cast<aidl::android::hardware::radio::RadioCapabilityStatus>(in.status);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CarrierRestrictionsWithPriority& in,
        aidl::android::hardware::radio::CarrierRestrictionsWithPriority* out) {
    {
        size_t size = in.allowedCarriers.size();
        aidl::android::hardware::radio::Carrier allowedCarriers;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.allowedCarriers[i], &allowedCarriers)) return false;
            out->allowedCarriers.push_back(allowedCarriers);
        }
    }
    {
        size_t size = in.excludedCarriers.size();
        aidl::android::hardware::radio::Carrier excludedCarriers;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.excludedCarriers[i], &excludedCarriers)) return false;
            out->excludedCarriers.push_back(excludedCarriers);
        }
    }
    out->allowedCarriersPrioritized = static_cast<bool>(in.allowedCarriersPrioritized);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RadioAccessSpecifier& in,
        aidl::android::hardware::radio::RadioAccessSpecifier* out) {
    out->radioAccessNetwork =
            static_cast<aidl::android::hardware::radio::RadioAccessNetworks>(in.radioAccessNetwork);
    if (!translate(in.bands, &out->bands)) return false;
    {
        size_t size = in.channels.size();
        for (size_t i = 0; i < size; i++) {
            out->channels.push_back(static_cast<int32_t>(in.channels[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands& in,
        aidl::android::hardware::radio::RadioAccessSpecifierBands* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands::hidl_discriminator::
                geranBands: {
            ::android::hardware::hidl_vec<::android::hardware::radio::V1_1::GeranBands> geranBands =
                    in.geranBands();
            size_t size = geranBands.size();
            for (size_t i = 0; i < size; i++) {
                out->get<aidl::android::hardware::radio::RadioAccessSpecifierBands::Tag::
                                 geranBands>()
                        .push_back(static_cast<aidl::android::hardware::radio::GeranBands>(
                                geranBands[i]));
            }
        } break;
        case ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands::hidl_discriminator::
                utranBands: {
            ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::UtranBands> utranBands =
                    in.utranBands();
            size_t size = utranBands.size();
            for (size_t i = 0; i < size; i++) {
                out->get<aidl::android::hardware::radio::RadioAccessSpecifierBands::Tag::
                                 utranBands>()
                        .push_back(static_cast<aidl::android::hardware::radio::UtranBands>(
                                utranBands[i]));
            }
        } break;
        case ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands::hidl_discriminator::
                eutranBands: {
            ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::EutranBands>
                    eutranBands = in.eutranBands();
            size_t size = eutranBands.size();
            for (size_t i = 0; i < size; i++) {
                out->get<aidl::android::hardware::radio::RadioAccessSpecifierBands::Tag::
                                 eutranBands>()
                        .push_back(static_cast<aidl::android::hardware::radio::EutranBands>(
                                eutranBands[i]));
            }
        } break;
        case ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands::hidl_discriminator::
                ngranBands: {
            ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::NgranBands> ngranBands =
                    in.ngranBands();
            size_t size = ngranBands.size();
            for (size_t i = 0; i < size; i++) {
                out->get<aidl::android::hardware::radio::RadioAccessSpecifierBands::Tag::
                                 ngranBands>()
                        .push_back(static_cast<aidl::android::hardware::radio::NgranBands>(
                                ngranBands[i]));
            }
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::SignalThresholdInfo& in,
        aidl::android::hardware::radio::SignalThresholdInfo* out) {
    out->signalMeasurement = static_cast<aidl::android::hardware::radio::SignalMeasurementType>(
            in.signalMeasurement);
    out->hysteresisMs = static_cast<int32_t>(in.hysteresisMs);
    out->hysteresisDb = static_cast<int32_t>(in.hysteresisDb);
    {
        size_t size = in.thresholds.size();
        for (size_t i = 0; i < size; i++) {
            out->thresholds.push_back(static_cast<int32_t>(in.thresholds[i]));
        }
    }
    out->isEnabled = static_cast<bool>(in.isEnabled);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::NetworkScanRequest& in,
        aidl::android::hardware::radio::NetworkScanRequest* out) {
    out->type = static_cast<aidl::android::hardware::radio::ScanType>(in.type);
    out->interval = static_cast<int32_t>(in.interval);
    {
        size_t size = in.specifiers.size();
        aidl::android::hardware::radio::RadioAccessSpecifier specifiers;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.specifiers[i], &specifiers)) return false;
            out->specifiers.push_back(specifiers);
        }
    }
    out->maxSearchTime = static_cast<int32_t>(in.maxSearchTime);
    out->incrementalResults = static_cast<bool>(in.incrementalResults);
    out->incrementalResultsPeriodicity = static_cast<int32_t>(in.incrementalResultsPeriodicity);
    {
        size_t size = in.mccMncs.size();
        for (size_t i = 0; i < size; i++) {
            out->mccMncs.push_back(in.mccMncs[i]);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::DataProfileInfo& in,
        aidl::android::hardware::radio::DataProfileInfo* out) {
    out->profileId = static_cast<aidl::android::hardware::radio::DataProfileId>(in.profileId);
    out->apn = in.apn;
    out->protocol = static_cast<aidl::android::hardware::radio::PdpProtocolType>(in.protocol);
    out->roamingProtocol =
            static_cast<aidl::android::hardware::radio::PdpProtocolType>(in.roamingProtocol);
    out->authType = static_cast<aidl::android::hardware::radio::ApnAuthType>(in.authType);
    out->user = in.user;
    out->password = in.password;
    out->type = static_cast<aidl::android::hardware::radio::DataProfileInfoType>(in.type);
    out->maxConnsTime = static_cast<int32_t>(in.maxConnsTime);
    out->maxConns = static_cast<int32_t>(in.maxConns);
    out->waitTime = static_cast<int32_t>(in.waitTime);
    out->enabled = static_cast<bool>(in.enabled);
    out->supportedApnTypesBitmap =
            static_cast<aidl::android::hardware::radio::ApnTypes>(in.supportedApnTypesBitmap);
    out->bearerBitmap =
            static_cast<aidl::android::hardware::radio::RadioAccessFamily>(in.bearerBitmap);
    out->mtuV4 = static_cast<int32_t>(in.mtuV4);
    out->mtuV6 = static_cast<int32_t>(in.mtuV6);
    out->preferred = static_cast<bool>(in.preferred);
    out->persistent = static_cast<bool>(in.persistent);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::LinkAddress& in,
        aidl::android::hardware::radio::LinkAddress* out) {
    out->address = in.address;
    out->properties = static_cast<aidl::android::hardware::radio::AddressProperty>(in.properties);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.deprecationTime > std::numeric_limits<int64_t>::max() || in.deprecationTime < 0) {
        return false;
    }
    out->deprecationTime = static_cast<int64_t>(in.deprecationTime);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.expirationTime > std::numeric_limits<int64_t>::max() || in.expirationTime < 0) {
        return false;
    }
    out->expirationTime = static_cast<int64_t>(in.expirationTime);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::ClosedSubscriberGroupInfo& in,
        aidl::android::hardware::radio::ClosedSubscriberGroupInfo* out) {
    out->csgIndication = static_cast<bool>(in.csgIndication);
    out->homeNodebName = in.homeNodebName;
    out->csgIdentity = static_cast<int32_t>(in.csgIdentity);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::OptionalCsgInfo& in,
        aidl::android::hardware::radio::OptionalCsgInfo* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_5::OptionalCsgInfo::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_5::OptionalCsgInfo::hidl_discriminator::csgInfo: {
            aidl::android::hardware::radio::ClosedSubscriberGroupInfo csgInfo;
            if (!translate(in.csgInfo(), &csgInfo)) return false;
            out->set<aidl::android::hardware::radio::OptionalCsgInfo::csgInfo>(csgInfo);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityGsm& in,
        aidl::android::hardware::radio::CellIdentityGsm* out) {
    out->mcc = in.base.base.mcc;
    out->mnc = in.base.base.mnc;
    out->lac = static_cast<int32_t>(in.base.base.lac);
    out->cid = static_cast<int32_t>(in.base.base.cid);
    out->arfcn = static_cast<int32_t>(in.base.base.arfcn);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.base.bsic > std::numeric_limits<int8_t>::max() || in.base.base.bsic < 0) {
        return false;
    }
    out->bsic = static_cast<int8_t>(in.base.base.bsic);
    if (!translate(in.base.operatorNames, &out->operatorNames)) return false;
    {
        size_t size = in.additionalPlmns.size();
        for (size_t i = 0; i < size; i++) {
            out->additionalPlmns.push_back(in.additionalPlmns[i]);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityWcdma& in,
        aidl::android::hardware::radio::CellIdentityWcdma* out) {
    out->mcc = in.base.base.mcc;
    out->mnc = in.base.base.mnc;
    out->lac = static_cast<int32_t>(in.base.base.lac);
    out->cid = static_cast<int32_t>(in.base.base.cid);
    out->psc = static_cast<int32_t>(in.base.base.psc);
    out->uarfcn = static_cast<int32_t>(in.base.base.uarfcn);
    if (!translate(in.base.operatorNames, &out->operatorNames)) return false;
    {
        size_t size = in.additionalPlmns.size();
        for (size_t i = 0; i < size; i++) {
            out->additionalPlmns.push_back(in.additionalPlmns[i]);
        }
    }
    if (!translate(in.optionalCsgInfo, &out->optionalCsgInfo)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityTdscdma& in,
        aidl::android::hardware::radio::CellIdentityTdscdma* out) {
    out->mcc = in.base.base.mcc;
    out->mnc = in.base.base.mnc;
    out->lac = static_cast<int32_t>(in.base.base.lac);
    out->cid = static_cast<int32_t>(in.base.base.cid);
    out->cpid = static_cast<int32_t>(in.base.base.cpid);
    out->uarfcn = static_cast<int32_t>(in.base.uarfcn);
    if (!translate(in.base.operatorNames, &out->operatorNames)) return false;
    {
        size_t size = in.additionalPlmns.size();
        for (size_t i = 0; i < size; i++) {
            out->additionalPlmns.push_back(in.additionalPlmns[i]);
        }
    }
    if (!translate(in.optionalCsgInfo, &out->optionalCsgInfo)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityLte& in,
        aidl::android::hardware::radio::CellIdentityLte* out) {
    out->mcc = in.base.base.mcc;
    out->mnc = in.base.base.mnc;
    out->ci = static_cast<int32_t>(in.base.base.ci);
    out->pci = static_cast<int32_t>(in.base.base.pci);
    out->tac = static_cast<int32_t>(in.base.base.tac);
    out->earfcn = static_cast<int32_t>(in.base.base.earfcn);
    if (!translate(in.base.operatorNames, &out->operatorNames)) return false;
    out->bandwidth = static_cast<int32_t>(in.base.bandwidth);
    {
        size_t size = in.additionalPlmns.size();
        for (size_t i = 0; i < size; i++) {
            out->additionalPlmns.push_back(in.additionalPlmns[i]);
        }
    }
    if (!translate(in.optionalCsgInfo, &out->optionalCsgInfo)) return false;
    {
        size_t size = in.bands.size();
        for (size_t i = 0; i < size; i++) {
            out->bands.push_back(
                    static_cast<aidl::android::hardware::radio::EutranBands>(in.bands[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityNr& in,
        aidl::android::hardware::radio::CellIdentityNr* out) {
    out->mcc = in.base.mcc;
    out->mnc = in.base.mnc;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.nci > std::numeric_limits<int64_t>::max() || in.base.nci < 0) {
        return false;
    }
    out->nci = static_cast<int64_t>(in.base.nci);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.pci > std::numeric_limits<int32_t>::max() || in.base.pci < 0) {
        return false;
    }
    out->pci = static_cast<int32_t>(in.base.pci);
    out->tac = static_cast<int32_t>(in.base.tac);
    out->nrarfcn = static_cast<int32_t>(in.base.nrarfcn);
    if (!translate(in.base.operatorNames, &out->operatorNames)) return false;
    {
        size_t size = in.additionalPlmns.size();
        for (size_t i = 0; i < size; i++) {
            out->additionalPlmns.push_back(in.additionalPlmns[i]);
        }
    }
    {
        size_t size = in.bands.size();
        for (size_t i = 0; i < size; i++) {
            out->bands.push_back(
                    static_cast<aidl::android::hardware::radio::NgranBands>(in.bands[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoGsm& in,
        aidl::android::hardware::radio::CellInfoGsm* out) {
    if (!translate(in.cellIdentityGsm, &out->cellIdentityGsm)) return false;
    if (!translate(in.signalStrengthGsm, &out->signalStrengthGsm)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoWcdma& in,
        aidl::android::hardware::radio::CellInfoWcdma* out) {
    if (!translate(in.cellIdentityWcdma, &out->cellIdentityWcdma)) return false;
    if (!translate(in.signalStrengthWcdma, &out->signalStrengthWcdma)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoTdscdma& in,
        aidl::android::hardware::radio::CellInfoTdscdma* out) {
    if (!translate(in.cellIdentityTdscdma, &out->cellIdentityTdscdma)) return false;
    if (!translate(in.signalStrengthTdscdma, &out->signalStrengthTdscdma)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentity& in,
        aidl::android::hardware::radio::CellIdentity* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::gsm: {
            aidl::android::hardware::radio::CellIdentityGsm gsm;
            if (!translate(in.gsm(), &gsm)) return false;
            out->set<aidl::android::hardware::radio::CellIdentity::gsm>(gsm);
        } break;
        case ::android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::wcdma: {
            aidl::android::hardware::radio::CellIdentityWcdma wcdma;
            if (!translate(in.wcdma(), &wcdma)) return false;
            out->set<aidl::android::hardware::radio::CellIdentity::wcdma>(wcdma);
        } break;
        case ::android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::tdscdma: {
            aidl::android::hardware::radio::CellIdentityTdscdma tdscdma;
            if (!translate(in.tdscdma(), &tdscdma)) return false;
            out->set<aidl::android::hardware::radio::CellIdentity::tdscdma>(tdscdma);
        } break;
        case ::android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::cdma: {
            aidl::android::hardware::radio::CellIdentityCdma cdma;
            if (!translate(in.cdma(), &cdma)) return false;
            out->set<aidl::android::hardware::radio::CellIdentity::cdma>(cdma);
        } break;
        case ::android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::lte: {
            aidl::android::hardware::radio::CellIdentityLte lte;
            if (!translate(in.lte(), &lte)) return false;
            out->set<aidl::android::hardware::radio::CellIdentity::lte>(lte);
        } break;
        case ::android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::nr: {
            aidl::android::hardware::radio::CellIdentityNr nr;
            if (!translate(in.nr(), &nr)) return false;
            out->set<aidl::android::hardware::radio::CellIdentity::nr>(nr);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo& in,
        aidl::android::hardware::radio::BarringInfo* out) {
    out->serviceType =
            static_cast<aidl::android::hardware::radio::BarringInfoServiceType>(in.serviceType);
    out->barringType =
            static_cast<aidl::android::hardware::radio::BarringInfoBarringType>(in.barringType);
    if (!translate(in.barringTypeSpecificInfo, &out->barringTypeSpecificInfo)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo::BarringTypeSpecificInfo::Conditional&
                in,
        aidl::android::hardware::radio::BarringInfoBarringTypeSpecificInfoConditional* out) {
    out->factor = static_cast<int32_t>(in.factor);
    out->timeSeconds = static_cast<int32_t>(in.timeSeconds);
    out->isBarred = static_cast<bool>(in.isBarred);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo::BarringTypeSpecificInfo& in,
        aidl::android::hardware::radio::BarringInfoBarringTypeSpecificInfo* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_5::BarringInfo::BarringTypeSpecificInfo::
                hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_5::BarringInfo::BarringTypeSpecificInfo::
                hidl_discriminator::conditional: {
            aidl::android::hardware::radio::BarringInfoBarringTypeSpecificInfoConditional
                    conditional;
            if (!translate(in.conditional(), &conditional)) return false;
            out->set<aidl::android::hardware::radio::BarringInfoBarringTypeSpecificInfo::
                             conditional>(conditional);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RegStateResult::AccessTechnologySpecificInfo::
                Cdma2000RegistrationInfo& in,
        aidl::android::hardware::radio::
                RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo* out) {
    out->cssSupported = static_cast<bool>(in.cssSupported);
    out->roamingIndicator = static_cast<int32_t>(in.roamingIndicator);
    out->systemIsInPrl =
            static_cast<aidl::android::hardware::radio::PrlIndicator>(in.systemIsInPrl);
    out->defaultRoamingIndicator = static_cast<int32_t>(in.defaultRoamingIndicator);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RegStateResult::AccessTechnologySpecificInfo::
                EutranRegistrationInfo& in,
        aidl::android::hardware::radio::
                RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo* out) {
    if (!translate(in.lteVopsInfo, &out->lteVopsInfo)) return false;
    if (!translate(in.nrIndicators, &out->nrIndicators)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::AppStatus& in,
        aidl::android::hardware::radio::AppStatus* out) {
    out->appType = static_cast<aidl::android::hardware::radio::AppType>(in.base.appType);
    out->appState = static_cast<aidl::android::hardware::radio::AppState>(in.base.appState);
    out->persoSubstate =
            static_cast<aidl::android::hardware::radio::PersoSubstate>(in.persoSubstate);
    out->aidPtr = in.base.aidPtr;
    out->appLabelPtr = in.base.appLabelPtr;
    out->pin1Replaced = static_cast<int32_t>(in.base.pin1Replaced);
    out->pin1 = static_cast<aidl::android::hardware::radio::PinState>(in.base.pin1);
    out->pin2 = static_cast<aidl::android::hardware::radio::PinState>(in.base.pin2);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CardStatus& in,
        aidl::android::hardware::radio::CardStatus* out) {
    out->cardState =
            static_cast<aidl::android::hardware::radio::CardState>(in.base.base.base.cardState);
    out->universalPinState = static_cast<aidl::android::hardware::radio::PinState>(
            in.base.base.base.universalPinState);
    out->gsmUmtsSubscriptionAppIndex =
            static_cast<int32_t>(in.base.base.base.gsmUmtsSubscriptionAppIndex);
    out->cdmaSubscriptionAppIndex =
            static_cast<int32_t>(in.base.base.base.cdmaSubscriptionAppIndex);
    out->imsSubscriptionAppIndex = static_cast<int32_t>(in.base.base.base.imsSubscriptionAppIndex);
    {
        size_t size = in.applications.size();
        aidl::android::hardware::radio::AppStatus applications;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.applications[i], &applications)) return false;
            out->applications.push_back(applications);
        }
    }
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.base.physicalSlotId > std::numeric_limits<int32_t>::max() ||
        in.base.base.physicalSlotId < 0) {
        return false;
    }
    out->physicalSlotId = static_cast<int32_t>(in.base.base.physicalSlotId);
    out->atr = in.base.base.atr;
    out->iccid = in.base.base.iccid;
    out->eid = in.base.eid;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosBandwidth& in,
        aidl::android::hardware::radio::QosBandwidth* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.maxBitrateKbps > std::numeric_limits<int32_t>::max() || in.maxBitrateKbps < 0) {
        return false;
    }
    out->maxBitrateKbps = static_cast<int32_t>(in.maxBitrateKbps);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.guaranteedBitrateKbps > std::numeric_limits<int32_t>::max() ||
        in.guaranteedBitrateKbps < 0) {
        return false;
    }
    out->guaranteedBitrateKbps = static_cast<int32_t>(in.guaranteedBitrateKbps);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::EpsQos& in,
        aidl::android::hardware::radio::EpsQos* out) {
    out->qci = static_cast<char16_t>(in.qci);
    if (!translate(in.downlink, &out->downlink)) return false;
    if (!translate(in.uplink, &out->uplink)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrQos& in,
        aidl::android::hardware::radio::NrQos* out) {
    out->fiveQi = static_cast<char16_t>(in.fiveQi);
    if (!translate(in.downlink, &out->downlink)) return false;
    if (!translate(in.uplink, &out->uplink)) return false;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.qfi > std::numeric_limits<int8_t>::max() || in.qfi < 0) {
        return false;
    }
    out->qfi = static_cast<int8_t>(in.qfi);
    out->averagingWindowMs = static_cast<char16_t>(in.averagingWindowMs);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_6::Qos& in,
                                                   aidl::android::hardware::radio::Qos* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::Qos::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::Qos::hidl_discriminator::eps: {
            aidl::android::hardware::radio::EpsQos eps;
            if (!translate(in.eps(), &eps)) return false;
            out->set<aidl::android::hardware::radio::Qos::eps>(eps);
        } break;
        case ::android::hardware::radio::V1_6::Qos::hidl_discriminator::nr: {
            aidl::android::hardware::radio::NrQos nr;
            if (!translate(in.nr(), &nr)) return false;
            out->set<aidl::android::hardware::radio::Qos::nr>(nr);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& in,
        aidl::android::hardware::radio::RadioResponseInfo* out) {
    out->type = static_cast<aidl::android::hardware::radio::RadioResponseType>(in.type);
    out->serial = static_cast<int32_t>(in.serial);
    out->error = static_cast<aidl::android::hardware::radio::RadioError>(in.error);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PortRange& in,
        aidl::android::hardware::radio::PortRange* out) {
    out->start = static_cast<int32_t>(in.start);
    out->end = static_cast<int32_t>(in.end);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::MaybePort& in,
        aidl::android::hardware::radio::MaybePort* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::MaybePort::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::MaybePort::hidl_discriminator::range: {
            aidl::android::hardware::radio::PortRange range;
            if (!translate(in.range(), &range)) return false;
            out->set<aidl::android::hardware::radio::MaybePort::range>(range);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter& in,
        aidl::android::hardware::radio::QosFilter* out) {
    {
        size_t size = in.localAddresses.size();
        for (size_t i = 0; i < size; i++) {
            out->localAddresses.push_back(in.localAddresses[i]);
        }
    }
    {
        size_t size = in.remoteAddresses.size();
        for (size_t i = 0; i < size; i++) {
            out->remoteAddresses.push_back(in.remoteAddresses[i]);
        }
    }
    if (!translate(in.localPort, &out->localPort)) return false;
    if (!translate(in.remotePort, &out->remotePort)) return false;
    out->protocol = static_cast<aidl::android::hardware::radio::QosProtocol>(in.protocol);
    if (!translate(in.tos, &out->tos)) return false;
    if (!translate(in.flowLabel, &out->flowLabel)) return false;
    if (!translate(in.spi, &out->spi)) return false;
    out->direction = static_cast<aidl::android::hardware::radio::QosFilterDirection>(in.direction);
    out->precedence = static_cast<int32_t>(in.precedence);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::TypeOfService& in,
        aidl::android::hardware::radio::QosFilterTypeOfService* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::QosFilter::TypeOfService::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::QosFilter::TypeOfService::hidl_discriminator::value:
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.value() > std::numeric_limits<int8_t>::max() || in.value() < 0) {
                return false;
            }
            *out = static_cast<int8_t>(in.value());
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::Ipv6FlowLabel& in,
        aidl::android::hardware::radio::QosFilterIpv6FlowLabel* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::QosFilter::Ipv6FlowLabel::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::QosFilter::Ipv6FlowLabel::hidl_discriminator::value:
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.value() > std::numeric_limits<int32_t>::max() || in.value() < 0) {
                return false;
            }
            *out = static_cast<int32_t>(in.value());
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::IpsecSpi& in,
        aidl::android::hardware::radio::QosFilterIpsecSpi* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::QosFilter::IpsecSpi::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::QosFilter::IpsecSpi::hidl_discriminator::value:
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.value() > std::numeric_limits<int32_t>::max() || in.value() < 0) {
                return false;
            }
            *out = static_cast<int32_t>(in.value());
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosSession& in,
        aidl::android::hardware::radio::QosSession* out) {
    out->qosSessionId = static_cast<int32_t>(in.qosSessionId);
    if (!translate(in.qos, &out->qos)) return false;
    {
        size_t size = in.qosFilters.size();
        aidl::android::hardware::radio::QosFilter qosFilters;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.qosFilters[i], &qosFilters)) return false;
            out->qosFilters.push_back(qosFilters);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SetupDataCallResult& in,
        aidl::android::hardware::radio::SetupDataCallResult* out) {
    out->cause = static_cast<aidl::android::hardware::radio::DataCallFailCause>(in.cause);
    out->suggestedRetryTime = static_cast<int64_t>(in.suggestedRetryTime);
    out->cid = static_cast<int32_t>(in.cid);
    out->active = static_cast<aidl::android::hardware::radio::DataConnActiveStatus>(in.active);
    out->type = static_cast<aidl::android::hardware::radio::PdpProtocolType>(in.type);
    out->ifname = in.ifname;
    {
        size_t size = in.addresses.size();
        aidl::android::hardware::radio::LinkAddress addresses;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.addresses[i], &addresses)) return false;
            out->addresses.push_back(addresses);
        }
    }
    {
        size_t size = in.dnses.size();
        for (size_t i = 0; i < size; i++) {
            out->dnses.push_back(in.dnses[i]);
        }
    }
    {
        size_t size = in.gateways.size();
        for (size_t i = 0; i < size; i++) {
            out->gateways.push_back(in.gateways[i]);
        }
    }
    {
        size_t size = in.pcscf.size();
        for (size_t i = 0; i < size; i++) {
            out->pcscf.push_back(in.pcscf[i]);
        }
    }
    out->mtuV4 = static_cast<int32_t>(in.mtuV4);
    out->mtuV6 = static_cast<int32_t>(in.mtuV6);
    if (!translate(in.defaultQos, &out->defaultQos)) return false;
    {
        size_t size = in.qosSessions.size();
        aidl::android::hardware::radio::QosSession qosSessions;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.qosSessions[i], &qosSessions)) return false;
            out->qosSessions.push_back(qosSessions);
        }
    }
    out->handoverFailureMode = static_cast<aidl::android::hardware::radio::HandoverFailureMode>(
            in.handoverFailureMode);
    out->pduSessionId = static_cast<int32_t>(in.pduSessionId);
    if (!translate(in.sliceInfo, &out->sliceInfo)) return false;
    {
        size_t size = in.trafficDescriptors.size();
        aidl::android::hardware::radio::TrafficDescriptor trafficDescriptors;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.trafficDescriptors[i], &trafficDescriptors)) return false;
            out->trafficDescriptors.push_back(trafficDescriptors);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::LinkCapacityEstimate& in,
        aidl::android::hardware::radio::LinkCapacityEstimate* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.downlinkCapacityKbps > std::numeric_limits<int32_t>::max() ||
        in.downlinkCapacityKbps < 0) {
        return false;
    }
    out->downlinkCapacityKbps = static_cast<int32_t>(in.downlinkCapacityKbps);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.uplinkCapacityKbps > std::numeric_limits<int32_t>::max() || in.uplinkCapacityKbps < 0) {
        return false;
    }
    out->uplinkCapacityKbps = static_cast<int32_t>(in.uplinkCapacityKbps);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.secondaryDownlinkCapacityKbps > std::numeric_limits<int32_t>::max() ||
        in.secondaryDownlinkCapacityKbps < 0) {
        return false;
    }
    out->secondaryDownlinkCapacityKbps = static_cast<int32_t>(in.secondaryDownlinkCapacityKbps);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.secondaryUplinkCapacityKbps > std::numeric_limits<int32_t>::max() ||
        in.secondaryUplinkCapacityKbps < 0) {
        return false;
    }
    out->secondaryUplinkCapacityKbps = static_cast<int32_t>(in.secondaryUplinkCapacityKbps);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrVopsInfo& in,
        aidl::android::hardware::radio::NrVopsInfo* out) {
    out->vopsSupported =
            static_cast<aidl::android::hardware::radio::VopsIndicator>(in.vopsSupported);
    out->emcSupported = static_cast<aidl::android::hardware::radio::EmcIndicator>(in.emcSupported);
    out->emfSupported = static_cast<aidl::android::hardware::radio::EmfIndicator>(in.emfSupported);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::LteSignalStrength& in,
        aidl::android::hardware::radio::LteSignalStrength* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.signalStrength > std::numeric_limits<int32_t>::max() ||
        in.base.signalStrength < 0) {
        return false;
    }
    out->signalStrength = static_cast<int32_t>(in.base.signalStrength);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.rsrp > std::numeric_limits<int32_t>::max() || in.base.rsrp < 0) {
        return false;
    }
    out->rsrp = static_cast<int32_t>(in.base.rsrp);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.rsrq > std::numeric_limits<int32_t>::max() || in.base.rsrq < 0) {
        return false;
    }
    out->rsrq = static_cast<int32_t>(in.base.rsrq);
    out->rssnr = static_cast<int32_t>(in.base.rssnr);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.cqi > std::numeric_limits<int32_t>::max() || in.base.cqi < 0) {
        return false;
    }
    out->cqi = static_cast<int32_t>(in.base.cqi);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.timingAdvance > std::numeric_limits<int32_t>::max() || in.base.timingAdvance < 0) {
        return false;
    }
    out->timingAdvance = static_cast<int32_t>(in.base.timingAdvance);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.cqiTableIndex > std::numeric_limits<int32_t>::max() || in.cqiTableIndex < 0) {
        return false;
    }
    out->cqiTableIndex = static_cast<int32_t>(in.cqiTableIndex);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrSignalStrength& in,
        aidl::android::hardware::radio::NrSignalStrength* out) {
    out->ssRsrp = static_cast<int32_t>(in.base.ssRsrp);
    out->ssRsrq = static_cast<int32_t>(in.base.ssRsrq);
    out->ssSinr = static_cast<int32_t>(in.base.ssSinr);
    out->csiRsrp = static_cast<int32_t>(in.base.csiRsrp);
    out->csiRsrq = static_cast<int32_t>(in.base.csiRsrq);
    out->csiSinr = static_cast<int32_t>(in.base.csiSinr);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.csiCqiTableIndex > std::numeric_limits<int32_t>::max() || in.csiCqiTableIndex < 0) {
        return false;
    }
    out->csiCqiTableIndex = static_cast<int32_t>(in.csiCqiTableIndex);
    {
        size_t size = in.csiCqiReport.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.csiCqiReport[i] > std::numeric_limits<int8_t>::max() || in.csiCqiReport[i] < 0) {
                return false;
            }
            out->csiCqiReport.push_back(static_cast<int8_t>(in.csiCqiReport[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SignalStrength& in,
        aidl::android::hardware::radio::SignalStrength* out) {
    if (!translate(in.gsm, &out->gsm)) return false;
    if (!translate(in.cdma, &out->cdma)) return false;
    if (!translate(in.evdo, &out->evdo)) return false;
    if (!translate(in.lte, &out->lte)) return false;
    if (!translate(in.tdscdma, &out->tdscdma)) return false;
    if (!translate(in.wcdma, &out->wcdma)) return false;
    if (!translate(in.nr, &out->nr)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfoLte& in,
        aidl::android::hardware::radio::CellInfoLte* out) {
    if (!translate(in.cellIdentityLte, &out->cellIdentityLte)) return false;
    if (!translate(in.signalStrengthLte, &out->signalStrengthLte)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfoNr& in,
        aidl::android::hardware::radio::CellInfoNr* out) {
    if (!translate(in.cellIdentityNr, &out->cellIdentityNr)) return false;
    if (!translate(in.signalStrengthNr, &out->signalStrengthNr)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfo& in,
        aidl::android::hardware::radio::CellInfo* out) {
    out->registered = static_cast<bool>(in.registered);
    out->connectionStatus =
            static_cast<aidl::android::hardware::radio::CellConnectionStatus>(in.connectionStatus);
    if (!translate(in.ratSpecificInfo, &out->ratSpecificInfo)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo& in,
        aidl::android::hardware::radio::CellInfoCellInfoRatSpecificInfo* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo::
                hidl_discriminator::gsm: {
            aidl::android::hardware::radio::CellInfoGsm gsm;
            if (!translate(in.gsm(), &gsm)) return false;
            out->set<aidl::android::hardware::radio::CellInfoCellInfoRatSpecificInfo::gsm>(gsm);
        } break;
        case ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo::
                hidl_discriminator::wcdma: {
            aidl::android::hardware::radio::CellInfoWcdma wcdma;
            if (!translate(in.wcdma(), &wcdma)) return false;
            out->set<aidl::android::hardware::radio::CellInfoCellInfoRatSpecificInfo::wcdma>(wcdma);
        } break;
        case ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo::
                hidl_discriminator::tdscdma: {
            aidl::android::hardware::radio::CellInfoTdscdma tdscdma;
            if (!translate(in.tdscdma(), &tdscdma)) return false;
            out->set<aidl::android::hardware::radio::CellInfoCellInfoRatSpecificInfo::tdscdma>(
                    tdscdma);
        } break;
        case ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo::
                hidl_discriminator::lte: {
            aidl::android::hardware::radio::CellInfoLte lte;
            if (!translate(in.lte(), &lte)) return false;
            out->set<aidl::android::hardware::radio::CellInfoCellInfoRatSpecificInfo::lte>(lte);
        } break;
        case ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo::
                hidl_discriminator::nr: {
            aidl::android::hardware::radio::CellInfoNr nr;
            if (!translate(in.nr(), &nr)) return false;
            out->set<aidl::android::hardware::radio::CellInfoCellInfoRatSpecificInfo::nr>(nr);
        } break;
        case ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo::
                hidl_discriminator::cdma: {
            aidl::android::hardware::radio::CellInfoCdma cdma;
            if (!translate(in.cdma(), &cdma)) return false;
            out->set<aidl::android::hardware::radio::CellInfoCellInfoRatSpecificInfo::cdma>(cdma);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NetworkScanResult& in,
        aidl::android::hardware::radio::NetworkScanResult* out) {
    out->status = static_cast<aidl::android::hardware::radio::ScanStatus>(in.status);
    out->error = static_cast<aidl::android::hardware::radio::RadioError>(in.error);
    {
        size_t size = in.networkInfos.size();
        aidl::android::hardware::radio::CellInfo networkInfos;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.networkInfos[i], &networkInfos)) return false;
            out->networkInfos.push_back(networkInfos);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RegStateResult& in,
        aidl::android::hardware::radio::RegStateResult* out) {
    out->regState = static_cast<aidl::android::hardware::radio::RegState>(in.regState);
    out->rat = static_cast<aidl::android::hardware::radio::RadioTechnology>(in.rat);
    out->reasonForDenial =
            static_cast<aidl::android::hardware::radio::RegistrationFailCause>(in.reasonForDenial);
    if (!translate(in.cellIdentity, &out->cellIdentity)) return false;
    out->registeredPlmn = in.registeredPlmn;
    if (!translate(in.accessTechnologySpecificInfo, &out->accessTechnologySpecificInfo))
        return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RegStateResult::AccessTechnologySpecificInfo& in,
        aidl::android::hardware::radio::RegStateResultAccessTechnologySpecificInfo* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::RegStateResult::AccessTechnologySpecificInfo::
                hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::RegStateResult::AccessTechnologySpecificInfo::
                hidl_discriminator::cdmaInfo: {
            aidl::android::hardware::radio::
                    RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo cdmaInfo;
            if (!translate(in.cdmaInfo(), &cdmaInfo)) return false;
            out->set<aidl::android::hardware::radio::RegStateResultAccessTechnologySpecificInfo::
                             cdmaInfo>(cdmaInfo);
        } break;
        case ::android::hardware::radio::V1_6::RegStateResult::AccessTechnologySpecificInfo::
                hidl_discriminator::eutranInfo: {
            aidl::android::hardware::radio::
                    RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo eutranInfo;
            if (!translate(in.eutranInfo(), &eutranInfo)) return false;
            out->set<aidl::android::hardware::radio::RegStateResultAccessTechnologySpecificInfo::
                             eutranInfo>(eutranInfo);
        } break;
        case ::android::hardware::radio::V1_6::RegStateResult::AccessTechnologySpecificInfo::
                hidl_discriminator::ngranNrVopsInfo: {
            aidl::android::hardware::radio::NrVopsInfo ngranNrVopsInfo;
            if (!translate(in.ngranNrVopsInfo(), &ngranNrVopsInfo)) return false;
            out->set<aidl::android::hardware::radio::RegStateResultAccessTechnologySpecificInfo::
                             ngranNrVopsInfo>(ngranNrVopsInfo);
        } break;
        case ::android::hardware::radio::V1_6::RegStateResult::AccessTechnologySpecificInfo::
                hidl_discriminator::geranDtmSupported:
            out->set<aidl::android::hardware::radio::RegStateResultAccessTechnologySpecificInfo::
                             geranDtmSupported>(static_cast<bool>(in.geranDtmSupported()));
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_6::Call& in,
                                                   aidl::android::hardware::radio::Call* out) {
    out->state = static_cast<aidl::android::hardware::radio::CallState>(in.base.base.state);
    out->index = static_cast<int32_t>(in.base.base.index);
    out->toa = static_cast<int32_t>(in.base.base.toa);
    out->isMpty = static_cast<bool>(in.base.base.isMpty);
    out->isMT = static_cast<bool>(in.base.base.isMT);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.base.als > std::numeric_limits<int8_t>::max() || in.base.base.als < 0) {
        return false;
    }
    out->als = static_cast<int8_t>(in.base.base.als);
    out->isVoice = static_cast<bool>(in.base.base.isVoice);
    out->isVoicePrivacy = static_cast<bool>(in.base.base.isVoicePrivacy);
    out->number = in.base.base.number;
    out->numberPresentation = static_cast<aidl::android::hardware::radio::CallPresentation>(
            in.base.base.numberPresentation);
    out->name = in.base.base.name;
    out->namePresentation = static_cast<aidl::android::hardware::radio::CallPresentation>(
            in.base.base.namePresentation);
    {
        size_t size = in.base.base.uusInfo.size();
        aidl::android::hardware::radio::UusInfo uusInfo;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.base.base.uusInfo[i], &uusInfo)) return false;
            out->uusInfo.push_back(uusInfo);
        }
    }
    out->audioQuality =
            static_cast<aidl::android::hardware::radio::AudioQuality>(in.base.audioQuality);
    out->forwardedNumber = in.forwardedNumber;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhysicalChannelConfig& in,
        aidl::android::hardware::radio::PhysicalChannelConfig* out) {
    out->status = static_cast<aidl::android::hardware::radio::CellConnectionStatus>(in.status);
    out->rat = static_cast<aidl::android::hardware::radio::RadioTechnology>(in.rat);
    out->downlinkChannelNumber = static_cast<int32_t>(in.downlinkChannelNumber);
    out->uplinkChannelNumber = static_cast<int32_t>(in.uplinkChannelNumber);
    out->cellBandwidthDownlinkKhz = static_cast<int32_t>(in.cellBandwidthDownlinkKhz);
    out->cellBandwidthUplinkKhz = static_cast<int32_t>(in.cellBandwidthUplinkKhz);
    {
        size_t size = in.contextIds.size();
        for (size_t i = 0; i < size; i++) {
            out->contextIds.push_back(static_cast<int32_t>(in.contextIds[i]));
        }
    }
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.physicalCellId > std::numeric_limits<int32_t>::max() || in.physicalCellId < 0) {
        return false;
    }
    out->physicalCellId = static_cast<int32_t>(in.physicalCellId);
    if (!translate(in.band, &out->band)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhysicalChannelConfig::Band& in,
        aidl::android::hardware::radio::PhysicalChannelConfigBand* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::PhysicalChannelConfig::Band::hidl_discriminator::
                geranBand:
            *out = static_cast<aidl::android::hardware::radio::GeranBands>(in.geranBand());
            break;
        case ::android::hardware::radio::V1_6::PhysicalChannelConfig::Band::hidl_discriminator::
                utranBand:
            *out = static_cast<aidl::android::hardware::radio::UtranBands>(in.utranBand());
            break;
        case ::android::hardware::radio::V1_6::PhysicalChannelConfig::Band::hidl_discriminator::
                eutranBand:
            *out = static_cast<aidl::android::hardware::radio::EutranBands>(in.eutranBand());
            break;
        case ::android::hardware::radio::V1_6::PhysicalChannelConfig::Band::hidl_discriminator::
                ngranBand:
            *out = static_cast<aidl::android::hardware::radio::NgranBands>(in.ngranBand());
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalSliceInfo& in,
        aidl::android::hardware::radio::OptionalSliceInfo* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::OptionalSliceInfo::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::OptionalSliceInfo::hidl_discriminator::value: {
            aidl::android::hardware::radio::SliceInfo value;
            if (!translate(in.value(), &value)) return false;
            out->set<aidl::android::hardware::radio::OptionalSliceInfo::value>(value);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SliceInfo& in,
        aidl::android::hardware::radio::SliceInfo* out) {
    out->sst = static_cast<aidl::android::hardware::radio::SliceServiceType>(in.sst);
    out->sliceDifferentiator = static_cast<int32_t>(in.sliceDifferentiator);
    out->mappedHplmnSst =
            static_cast<aidl::android::hardware::radio::SliceServiceType>(in.mappedHplmnSst);
    out->mappedHplmnSD = static_cast<int32_t>(in.mappedHplmnSD);
    out->status = static_cast<aidl::android::hardware::radio::SliceStatus>(in.status);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalDnn& in,
        aidl::android::hardware::radio::OptionalDnn* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::OptionalDnn::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::OptionalDnn::hidl_discriminator::value:
            *out = in.value();
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalOsAppId& in,
        aidl::android::hardware::radio::OptionalOsAppId* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::OptionalOsAppId::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::OptionalOsAppId::hidl_discriminator::value: {
            aidl::android::hardware::radio::OsAppId value;
            if (!translate(in.value(), &value)) return false;
            out->set<aidl::android::hardware::radio::OptionalOsAppId::value>(value);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalTrafficDescriptor& in,
        aidl::android::hardware::radio::OptionalTrafficDescriptor* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::OptionalTrafficDescriptor::hidl_discriminator::
                noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::OptionalTrafficDescriptor::hidl_discriminator::
                value: {
            aidl::android::hardware::radio::TrafficDescriptor value;
            if (!translate(in.value(), &value)) return false;
            out->set<aidl::android::hardware::radio::OptionalTrafficDescriptor::value>(value);
        } break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::TrafficDescriptor& in,
        aidl::android::hardware::radio::TrafficDescriptor* out) {
    if (!translate(in.dnn, &out->dnn)) return false;
    if (!translate(in.osAppId, &out->osAppId)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OsAppId& in,
        aidl::android::hardware::radio::OsAppId* out) {
    {
        size_t size = in.osAppId.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.osAppId[i] > std::numeric_limits<int8_t>::max() || in.osAppId[i] < 0) {
                return false;
            }
            out->osAppId.push_back(static_cast<int8_t>(in.osAppId[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SlicingConfig& in,
        aidl::android::hardware::radio::SlicingConfig* out) {
    {
        size_t size = in.urspRules.size();
        aidl::android::hardware::radio::UrspRule urspRules;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.urspRules[i], &urspRules)) return false;
            out->urspRules.push_back(urspRules);
        }
    }
    {
        size_t size = in.sliceInfo.size();
        aidl::android::hardware::radio::SliceInfo sliceInfo;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.sliceInfo[i], &sliceInfo)) return false;
            out->sliceInfo.push_back(sliceInfo);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::UrspRule& in,
        aidl::android::hardware::radio::UrspRule* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.precedence > std::numeric_limits<int8_t>::max() || in.precedence < 0) {
        return false;
    }
    out->precedence = static_cast<int8_t>(in.precedence);
    {
        size_t size = in.trafficDescriptors.size();
        aidl::android::hardware::radio::TrafficDescriptor trafficDescriptors;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.trafficDescriptors[i], &trafficDescriptors)) return false;
            out->trafficDescriptors.push_back(trafficDescriptors);
        }
    }
    {
        size_t size = in.routeSelectionDescriptor.size();
        aidl::android::hardware::radio::RouteSelectionDescriptor routeSelectionDescriptor;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.routeSelectionDescriptor[i], &routeSelectionDescriptor)) return false;
            out->routeSelectionDescriptor.push_back(routeSelectionDescriptor);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RouteSelectionDescriptor& in,
        aidl::android::hardware::radio::RouteSelectionDescriptor* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.precedence > std::numeric_limits<int8_t>::max() || in.precedence < 0) {
        return false;
    }
    out->precedence = static_cast<int8_t>(in.precedence);
    if (!translate(in.sessionType, &out->sessionType)) return false;
    if (!translate(in.sscMode, &out->sscMode)) return false;
    {
        size_t size = in.sliceInfo.size();
        aidl::android::hardware::radio::SliceInfo sliceInfo;
        for (size_t i = 0; i < size; i++) {
            if (!translate(in.sliceInfo[i], &sliceInfo)) return false;
            out->sliceInfo.push_back(sliceInfo);
        }
    }
    {
        size_t size = in.dnn.size();
        for (size_t i = 0; i < size; i++) {
            out->dnn.push_back(in.dnn[i]);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalPdpProtocolType& in,
        aidl::android::hardware::radio::OptionalPdpProtocolType* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::OptionalPdpProtocolType::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::OptionalPdpProtocolType::hidl_discriminator::value:
            *out = static_cast<aidl::android::hardware::radio::PdpProtocolType>(in.value());
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalSscMode& in,
        aidl::android::hardware::radio::OptionalSscMode* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::radio::V1_6::OptionalSscMode::hidl_discriminator::noinit:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::radio::V1_6::OptionalSscMode::hidl_discriminator::value:
            *out = static_cast<aidl::android::hardware::radio::SscMode>(in.value());
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::ImsiEncryptionInfo& in,
        aidl::android::hardware::radio::ImsiEncryptionInfo* out) {
    out->mcc = in.base.mcc;
    out->mnc = in.base.mnc;
    {
        size_t size = in.base.carrierKey.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.base.carrierKey[i] > std::numeric_limits<int8_t>::max() ||
                in.base.carrierKey[i] < 0) {
                return false;
            }
            out->carrierKey.push_back(static_cast<int8_t>(in.base.carrierKey[i]));
        }
    }
    out->keyIdentifier = in.base.keyIdentifier;
    out->expirationTime = static_cast<int64_t>(in.base.expirationTime);
    out->keyType = static_cast<aidl::android::hardware::radio::PublicKeyType>(in.keyType);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhonebookRecordInfo& in,
        aidl::android::hardware::radio::PhonebookRecordInfo* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.recordId > std::numeric_limits<int32_t>::max() || in.recordId < 0) {
        return false;
    }
    out->recordId = static_cast<int32_t>(in.recordId);
    out->name = in.name;
    out->number = in.number;
    {
        size_t size = in.emails.size();
        for (size_t i = 0; i < size; i++) {
            out->emails.push_back(in.emails[i]);
        }
    }
    {
        size_t size = in.additionalNumbers.size();
        for (size_t i = 0; i < size; i++) {
            out->additionalNumbers.push_back(in.additionalNumbers[i]);
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhonebookCapacity& in,
        aidl::android::hardware::radio::PhonebookCapacity* out) {
    out->maxAdnRecords = static_cast<int32_t>(in.maxAdnRecords);
    out->usedAdnRecords = static_cast<int32_t>(in.usedAdnRecords);
    out->maxEmailRecords = static_cast<int32_t>(in.maxEmailRecords);
    out->usedEmailRecords = static_cast<int32_t>(in.usedEmailRecords);
    out->maxAdditionalNumberRecords = static_cast<int32_t>(in.maxAdditionalNumberRecords);
    out->usedAdditionalNumberRecords = static_cast<int32_t>(in.usedAdditionalNumberRecords);
    out->maxNameLen = static_cast<int32_t>(in.maxNameLen);
    out->maxNumberLen = static_cast<int32_t>(in.maxNumberLen);
    out->maxEmailLen = static_cast<int32_t>(in.maxEmailLen);
    out->maxAdditionalNumberLen = static_cast<int32_t>(in.maxAdditionalNumberLen);
    return true;
}

}  // namespace android::h2a
