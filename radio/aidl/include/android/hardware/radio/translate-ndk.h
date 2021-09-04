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

#pragma once

#include <limits>
#include "aidl/android/hardware/radio/AccessNetwork.h"
#include "aidl/android/hardware/radio/ActivityStatsInfo.h"
#include "aidl/android/hardware/radio/AddressProperty.h"
#include "aidl/android/hardware/radio/ApnAuthType.h"
#include "aidl/android/hardware/radio/ApnTypes.h"
#include "aidl/android/hardware/radio/AppState.h"
#include "aidl/android/hardware/radio/AppStatus.h"
#include "aidl/android/hardware/radio/AppType.h"
#include "aidl/android/hardware/radio/AudioQuality.h"
#include "aidl/android/hardware/radio/BarringInfo.h"
#include "aidl/android/hardware/radio/BarringInfoBarringType.h"
#include "aidl/android/hardware/radio/BarringInfoBarringTypeSpecificInfo.h"
#include "aidl/android/hardware/radio/BarringInfoBarringTypeSpecificInfoConditional.h"
#include "aidl/android/hardware/radio/BarringInfoServiceType.h"
#include "aidl/android/hardware/radio/Call.h"
#include "aidl/android/hardware/radio/CallForwardInfo.h"
#include "aidl/android/hardware/radio/CallForwardInfoStatus.h"
#include "aidl/android/hardware/radio/CallPresentation.h"
#include "aidl/android/hardware/radio/CallState.h"
#include "aidl/android/hardware/radio/CardPowerState.h"
#include "aidl/android/hardware/radio/CardState.h"
#include "aidl/android/hardware/radio/CardStatus.h"
#include "aidl/android/hardware/radio/Carrier.h"
#include "aidl/android/hardware/radio/CarrierMatchType.h"
#include "aidl/android/hardware/radio/CarrierRestrictions.h"
#include "aidl/android/hardware/radio/CarrierRestrictionsWithPriority.h"
#include "aidl/android/hardware/radio/CdmaBroadcastSmsConfigInfo.h"
#include "aidl/android/hardware/radio/CdmaCallWaiting.h"
#include "aidl/android/hardware/radio/CdmaCallWaitingNumberPlan.h"
#include "aidl/android/hardware/radio/CdmaCallWaitingNumberPresentation.h"
#include "aidl/android/hardware/radio/CdmaCallWaitingNumberType.h"
#include "aidl/android/hardware/radio/CdmaDisplayInfoRecord.h"
#include "aidl/android/hardware/radio/CdmaInfoRecName.h"
#include "aidl/android/hardware/radio/CdmaInformationRecord.h"
#include "aidl/android/hardware/radio/CdmaInformationRecords.h"
#include "aidl/android/hardware/radio/CdmaLineControlInfoRecord.h"
#include "aidl/android/hardware/radio/CdmaNumberInfoRecord.h"
#include "aidl/android/hardware/radio/CdmaOtaProvisionStatus.h"
#include "aidl/android/hardware/radio/CdmaRedirectingNumberInfoRecord.h"
#include "aidl/android/hardware/radio/CdmaRedirectingReason.h"
#include "aidl/android/hardware/radio/CdmaRoamingType.h"
#include "aidl/android/hardware/radio/CdmaSignalInfoRecord.h"
#include "aidl/android/hardware/radio/CdmaSignalStrength.h"
#include "aidl/android/hardware/radio/CdmaSmsAck.h"
#include "aidl/android/hardware/radio/CdmaSmsAddress.h"
#include "aidl/android/hardware/radio/CdmaSmsDigitMode.h"
#include "aidl/android/hardware/radio/CdmaSmsErrorClass.h"
#include "aidl/android/hardware/radio/CdmaSmsMessage.h"
#include "aidl/android/hardware/radio/CdmaSmsNumberMode.h"
#include "aidl/android/hardware/radio/CdmaSmsNumberPlan.h"
#include "aidl/android/hardware/radio/CdmaSmsNumberType.h"
#include "aidl/android/hardware/radio/CdmaSmsSubaddress.h"
#include "aidl/android/hardware/radio/CdmaSmsSubaddressType.h"
#include "aidl/android/hardware/radio/CdmaSmsWriteArgs.h"
#include "aidl/android/hardware/radio/CdmaSmsWriteArgsStatus.h"
#include "aidl/android/hardware/radio/CdmaSubscriptionSource.h"
#include "aidl/android/hardware/radio/CdmaT53AudioControlInfoRecord.h"
#include "aidl/android/hardware/radio/CdmaT53ClirInfoRecord.h"
#include "aidl/android/hardware/radio/CellConfigLte.h"
#include "aidl/android/hardware/radio/CellConnectionStatus.h"
#include "aidl/android/hardware/radio/CellIdentity.h"
#include "aidl/android/hardware/radio/CellIdentityCdma.h"
#include "aidl/android/hardware/radio/CellIdentityGsm.h"
#include "aidl/android/hardware/radio/CellIdentityLte.h"
#include "aidl/android/hardware/radio/CellIdentityNr.h"
#include "aidl/android/hardware/radio/CellIdentityOperatorNames.h"
#include "aidl/android/hardware/radio/CellIdentityTdscdma.h"
#include "aidl/android/hardware/radio/CellIdentityWcdma.h"
#include "aidl/android/hardware/radio/CellInfo.h"
#include "aidl/android/hardware/radio/CellInfoCdma.h"
#include "aidl/android/hardware/radio/CellInfoCellInfoRatSpecificInfo.h"
#include "aidl/android/hardware/radio/CellInfoGsm.h"
#include "aidl/android/hardware/radio/CellInfoInfo.h"
#include "aidl/android/hardware/radio/CellInfoLte.h"
#include "aidl/android/hardware/radio/CellInfoNr.h"
#include "aidl/android/hardware/radio/CellInfoTdscdma.h"
#include "aidl/android/hardware/radio/CellInfoType.h"
#include "aidl/android/hardware/radio/CellInfoWcdma.h"
#include "aidl/android/hardware/radio/CfData.h"
#include "aidl/android/hardware/radio/ClipStatus.h"
#include "aidl/android/hardware/radio/Clir.h"
#include "aidl/android/hardware/radio/ClosedSubscriberGroupInfo.h"
#include "aidl/android/hardware/radio/DataCallFailCause.h"
#include "aidl/android/hardware/radio/DataConnActiveStatus.h"
#include "aidl/android/hardware/radio/DataProfileId.h"
#include "aidl/android/hardware/radio/DataProfileInfo.h"
#include "aidl/android/hardware/radio/DataProfileInfoType.h"
#include "aidl/android/hardware/radio/DataRegStateResult.h"
#include "aidl/android/hardware/radio/DataRegStateResultVopsInfo.h"
#include "aidl/android/hardware/radio/DataRequestReason.h"
#include "aidl/android/hardware/radio/DataThrottlingAction.h"
#include "aidl/android/hardware/radio/DeviceStateType.h"
#include "aidl/android/hardware/radio/Dial.h"
#include "aidl/android/hardware/radio/Domain.h"
#include "aidl/android/hardware/radio/EmcIndicator.h"
#include "aidl/android/hardware/radio/EmergencyCallRouting.h"
#include "aidl/android/hardware/radio/EmergencyNumber.h"
#include "aidl/android/hardware/radio/EmergencyNumberSource.h"
#include "aidl/android/hardware/radio/EmergencyServiceCategory.h"
#include "aidl/android/hardware/radio/EmfIndicator.h"
#include "aidl/android/hardware/radio/EpsQos.h"
#include "aidl/android/hardware/radio/EutranBands.h"
#include "aidl/android/hardware/radio/EvdoSignalStrength.h"
#include "aidl/android/hardware/radio/FrequencyRange.h"
#include "aidl/android/hardware/radio/GeranBands.h"
#include "aidl/android/hardware/radio/GsmBroadcastSmsConfigInfo.h"
#include "aidl/android/hardware/radio/GsmSignalStrength.h"
#include "aidl/android/hardware/radio/GsmSmsMessage.h"
#include "aidl/android/hardware/radio/HandoverFailureMode.h"
#include "aidl/android/hardware/radio/HardwareConfig.h"
#include "aidl/android/hardware/radio/HardwareConfigModem.h"
#include "aidl/android/hardware/radio/HardwareConfigSim.h"
#include "aidl/android/hardware/radio/HardwareConfigState.h"
#include "aidl/android/hardware/radio/HardwareConfigType.h"
#include "aidl/android/hardware/radio/IccIo.h"
#include "aidl/android/hardware/radio/IccIoResult.h"
#include "aidl/android/hardware/radio/ImsSmsMessage.h"
#include "aidl/android/hardware/radio/ImsiEncryptionInfo.h"
#include "aidl/android/hardware/radio/IncrementalResultsPeriodicityRange.h"
#include "aidl/android/hardware/radio/IndicationFilter.h"
#include "aidl/android/hardware/radio/KeepaliveRequest.h"
#include "aidl/android/hardware/radio/KeepaliveStatus.h"
#include "aidl/android/hardware/radio/KeepaliveStatusCode.h"
#include "aidl/android/hardware/radio/KeepaliveType.h"
#include "aidl/android/hardware/radio/LastCallFailCause.h"
#include "aidl/android/hardware/radio/LastCallFailCauseInfo.h"
#include "aidl/android/hardware/radio/LceDataInfo.h"
#include "aidl/android/hardware/radio/LceStatus.h"
#include "aidl/android/hardware/radio/LceStatusInfo.h"
#include "aidl/android/hardware/radio/LinkAddress.h"
#include "aidl/android/hardware/radio/LinkCapacityEstimate.h"
#include "aidl/android/hardware/radio/LteSignalStrength.h"
#include "aidl/android/hardware/radio/LteVopsInfo.h"
#include "aidl/android/hardware/radio/MaxSearchTimeRange.h"
#include "aidl/android/hardware/radio/MaybePort.h"
#include "aidl/android/hardware/radio/MvnoType.h"
#include "aidl/android/hardware/radio/NeighboringCell.h"
#include "aidl/android/hardware/radio/NetworkScanRequest.h"
#include "aidl/android/hardware/radio/NetworkScanResult.h"
#include "aidl/android/hardware/radio/NgranBands.h"
#include "aidl/android/hardware/radio/NrDualConnectivityState.h"
#include "aidl/android/hardware/radio/NrIndicators.h"
#include "aidl/android/hardware/radio/NrQos.h"
#include "aidl/android/hardware/radio/NrSignalStrength.h"
#include "aidl/android/hardware/radio/NrVopsInfo.h"
#include "aidl/android/hardware/radio/NvItem.h"
#include "aidl/android/hardware/radio/NvWriteItem.h"
#include "aidl/android/hardware/radio/OperatorInfo.h"
#include "aidl/android/hardware/radio/OperatorStatus.h"
#include "aidl/android/hardware/radio/OptionalCsgInfo.h"
#include "aidl/android/hardware/radio/OptionalDnn.h"
#include "aidl/android/hardware/radio/OptionalOsAppId.h"
#include "aidl/android/hardware/radio/OptionalPdpProtocolType.h"
#include "aidl/android/hardware/radio/OptionalSliceInfo.h"
#include "aidl/android/hardware/radio/OptionalSscMode.h"
#include "aidl/android/hardware/radio/OptionalTrafficDescriptor.h"
#include "aidl/android/hardware/radio/OsAppId.h"
#include "aidl/android/hardware/radio/P2Constant.h"
#include "aidl/android/hardware/radio/PbReceivedStatus.h"
#include "aidl/android/hardware/radio/PcoDataInfo.h"
#include "aidl/android/hardware/radio/PdpProtocolType.h"
#include "aidl/android/hardware/radio/PersoSubstate.h"
#include "aidl/android/hardware/radio/PhoneRestrictedState.h"
#include "aidl/android/hardware/radio/PhonebookCapacity.h"
#include "aidl/android/hardware/radio/PhonebookRecordInfo.h"
#include "aidl/android/hardware/radio/PhysicalChannelConfig.h"
#include "aidl/android/hardware/radio/PhysicalChannelConfigBand.h"
#include "aidl/android/hardware/radio/PinState.h"
#include "aidl/android/hardware/radio/PortRange.h"
#include "aidl/android/hardware/radio/PreferredNetworkType.h"
#include "aidl/android/hardware/radio/PrlIndicator.h"
#include "aidl/android/hardware/radio/PublicKeyType.h"
#include "aidl/android/hardware/radio/Qos.h"
#include "aidl/android/hardware/radio/QosBandwidth.h"
#include "aidl/android/hardware/radio/QosFilter.h"
#include "aidl/android/hardware/radio/QosFilterDirection.h"
#include "aidl/android/hardware/radio/QosFilterIpsecSpi.h"
#include "aidl/android/hardware/radio/QosFilterIpv6FlowLabel.h"
#include "aidl/android/hardware/radio/QosFilterTypeOfService.h"
#include "aidl/android/hardware/radio/QosFlowIdRange.h"
#include "aidl/android/hardware/radio/QosPortRange.h"
#include "aidl/android/hardware/radio/QosProtocol.h"
#include "aidl/android/hardware/radio/QosSession.h"
#include "aidl/android/hardware/radio/RadioAccessFamily.h"
#include "aidl/android/hardware/radio/RadioAccessNetworks.h"
#include "aidl/android/hardware/radio/RadioAccessSpecifier.h"
#include "aidl/android/hardware/radio/RadioAccessSpecifierBands.h"
#include "aidl/android/hardware/radio/RadioBandMode.h"
#include "aidl/android/hardware/radio/RadioCapability.h"
#include "aidl/android/hardware/radio/RadioCapabilityPhase.h"
#include "aidl/android/hardware/radio/RadioCapabilityStatus.h"
#include "aidl/android/hardware/radio/RadioCdmaSmsConst.h"
#include "aidl/android/hardware/radio/RadioConst.h"
#include "aidl/android/hardware/radio/RadioError.h"
#include "aidl/android/hardware/radio/RadioFrequencyInfo.h"
#include "aidl/android/hardware/radio/RadioIndicationType.h"
#include "aidl/android/hardware/radio/RadioResponseInfo.h"
#include "aidl/android/hardware/radio/RadioResponseInfoModem.h"
#include "aidl/android/hardware/radio/RadioResponseType.h"
#include "aidl/android/hardware/radio/RadioState.h"
#include "aidl/android/hardware/radio/RadioTechnology.h"
#include "aidl/android/hardware/radio/RadioTechnologyFamily.h"
#include "aidl/android/hardware/radio/RegState.h"
#include "aidl/android/hardware/radio/RegStateResult.h"
#include "aidl/android/hardware/radio/RegStateResultAccessTechnologySpecificInfo.h"
#include "aidl/android/hardware/radio/RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo.h"
#include "aidl/android/hardware/radio/RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo.h"
#include "aidl/android/hardware/radio/RegistrationFailCause.h"
#include "aidl/android/hardware/radio/ResetNvType.h"
#include "aidl/android/hardware/radio/RestrictedState.h"
#include "aidl/android/hardware/radio/RouteSelectionDescriptor.h"
#include "aidl/android/hardware/radio/SapApduType.h"
#include "aidl/android/hardware/radio/SapConnectRsp.h"
#include "aidl/android/hardware/radio/SapDisconnectType.h"
#include "aidl/android/hardware/radio/SapResultCode.h"
#include "aidl/android/hardware/radio/SapStatus.h"
#include "aidl/android/hardware/radio/SapTransferProtocol.h"
#include "aidl/android/hardware/radio/ScanIntervalRange.h"
#include "aidl/android/hardware/radio/ScanStatus.h"
#include "aidl/android/hardware/radio/ScanType.h"
#include "aidl/android/hardware/radio/SelectUiccSub.h"
#include "aidl/android/hardware/radio/SendSmsResult.h"
#include "aidl/android/hardware/radio/SetupDataCallResult.h"
#include "aidl/android/hardware/radio/SignalMeasurementType.h"
#include "aidl/android/hardware/radio/SignalStrength.h"
#include "aidl/android/hardware/radio/SignalThresholdInfo.h"
#include "aidl/android/hardware/radio/SimApdu.h"
#include "aidl/android/hardware/radio/SimLockMultiSimPolicy.h"
#include "aidl/android/hardware/radio/SimRefreshResult.h"
#include "aidl/android/hardware/radio/SimRefreshType.h"
#include "aidl/android/hardware/radio/SliceInfo.h"
#include "aidl/android/hardware/radio/SliceServiceType.h"
#include "aidl/android/hardware/radio/SliceStatus.h"
#include "aidl/android/hardware/radio/SlicingConfig.h"
#include "aidl/android/hardware/radio/SmsAcknowledgeFailCause.h"
#include "aidl/android/hardware/radio/SmsWriteArgs.h"
#include "aidl/android/hardware/radio/SmsWriteArgsStatus.h"
#include "aidl/android/hardware/radio/SrvccState.h"
#include "aidl/android/hardware/radio/SsInfoData.h"
#include "aidl/android/hardware/radio/SsRequestType.h"
#include "aidl/android/hardware/radio/SsServiceType.h"
#include "aidl/android/hardware/radio/SsTeleserviceType.h"
#include "aidl/android/hardware/radio/SscMode.h"
#include "aidl/android/hardware/radio/StkCcUnsolSsResult.h"
#include "aidl/android/hardware/radio/SubscriptionType.h"
#include "aidl/android/hardware/radio/SuppServiceClass.h"
#include "aidl/android/hardware/radio/SuppSvcNotification.h"
#include "aidl/android/hardware/radio/TdscdmaSignalStrength.h"
#include "aidl/android/hardware/radio/TimeStampType.h"
#include "aidl/android/hardware/radio/TrafficDescriptor.h"
#include "aidl/android/hardware/radio/TtyMode.h"
#include "aidl/android/hardware/radio/UiccSubActStatus.h"
#include "aidl/android/hardware/radio/UrspRule.h"
#include "aidl/android/hardware/radio/UssdModeType.h"
#include "aidl/android/hardware/radio/UtranBands.h"
#include "aidl/android/hardware/radio/UusDcs.h"
#include "aidl/android/hardware/radio/UusInfo.h"
#include "aidl/android/hardware/radio/UusType.h"
#include "aidl/android/hardware/radio/VoiceRegStateResult.h"
#include "aidl/android/hardware/radio/VopsIndicator.h"
#include "aidl/android/hardware/radio/WcdmaSignalStrength.h"
#include "android/hardware/radio/1.0/types.h"
#include "android/hardware/radio/1.1/types.h"
#include "android/hardware/radio/1.2/types.h"
#include "android/hardware/radio/1.3/types.h"
#include "android/hardware/radio/1.4/types.h"
#include "android/hardware/radio/1.5/types.h"
#include "android/hardware/radio/1.6/types.h"

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::IccIo& in,
        aidl::android::hardware::radio::IccIo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::NeighboringCell& in,
        aidl::android::hardware::radio::NeighboringCell* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::UusInfo& in,
        aidl::android::hardware::radio::UusInfo* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_0::Dial& in,
                                                   aidl::android::hardware::radio::Dial* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LastCallFailCauseInfo& in,
        aidl::android::hardware::radio::LastCallFailCauseInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmSignalStrength& in,
        aidl::android::hardware::radio::GsmSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSignalStrength& in,
        aidl::android::hardware::radio::CdmaSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::EvdoSignalStrength& in,
        aidl::android::hardware::radio::EvdoSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SendSmsResult& in,
        aidl::android::hardware::radio::SendSmsResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::IccIoResult& in,
        aidl::android::hardware::radio::IccIoResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CallForwardInfo& in,
        aidl::android::hardware::radio::CallForwardInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::OperatorInfo& in,
        aidl::android::hardware::radio::OperatorInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SmsWriteArgs& in,
        aidl::android::hardware::radio::SmsWriteArgs* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsAddress& in,
        aidl::android::hardware::radio::CdmaSmsAddress* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsSubaddress& in,
        aidl::android::hardware::radio::CdmaSmsSubaddress* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsMessage& in,
        aidl::android::hardware::radio::CdmaSmsMessage* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsAck& in,
        aidl::android::hardware::radio::CdmaSmsAck* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaBroadcastSmsConfigInfo& in,
        aidl::android::hardware::radio::CdmaBroadcastSmsConfigInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsWriteArgs& in,
        aidl::android::hardware::radio::CdmaSmsWriteArgs* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmBroadcastSmsConfigInfo& in,
        aidl::android::hardware::radio::GsmBroadcastSmsConfigInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmSmsMessage& in,
        aidl::android::hardware::radio::GsmSmsMessage* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::ImsSmsMessage& in,
        aidl::android::hardware::radio::ImsSmsMessage* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SimApdu& in,
        aidl::android::hardware::radio::SimApdu* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::NvWriteItem& in,
        aidl::android::hardware::radio::NvWriteItem* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SelectUiccSub& in,
        aidl::android::hardware::radio::SelectUiccSub* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfigModem& in,
        aidl::android::hardware::radio::HardwareConfigModem* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfigSim& in,
        aidl::android::hardware::radio::HardwareConfigSim* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfig& in,
        aidl::android::hardware::radio::HardwareConfig* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LceStatusInfo& in,
        aidl::android::hardware::radio::LceStatusInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LceDataInfo& in,
        aidl::android::hardware::radio::LceDataInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::ActivityStatsInfo& in,
        aidl::android::hardware::radio::ActivityStatsInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::Carrier& in,
        aidl::android::hardware::radio::Carrier* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CarrierRestrictions& in,
        aidl::android::hardware::radio::CarrierRestrictions* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SuppSvcNotification& in,
        aidl::android::hardware::radio::SuppSvcNotification* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SimRefreshResult& in,
        aidl::android::hardware::radio::SimRefreshResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSignalInfoRecord& in,
        aidl::android::hardware::radio::CdmaSignalInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaCallWaiting& in,
        aidl::android::hardware::radio::CdmaCallWaiting* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaDisplayInfoRecord& in,
        aidl::android::hardware::radio::CdmaDisplayInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaNumberInfoRecord& in,
        aidl::android::hardware::radio::CdmaNumberInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaRedirectingNumberInfoRecord& in,
        aidl::android::hardware::radio::CdmaRedirectingNumberInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaLineControlInfoRecord& in,
        aidl::android::hardware::radio::CdmaLineControlInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaT53ClirInfoRecord& in,
        aidl::android::hardware::radio::CdmaT53ClirInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaT53AudioControlInfoRecord& in,
        aidl::android::hardware::radio::CdmaT53AudioControlInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaInformationRecord& in,
        aidl::android::hardware::radio::CdmaInformationRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaInformationRecords& in,
        aidl::android::hardware::radio::CdmaInformationRecords* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CfData& in,
        aidl::android::hardware::radio::CfData* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SsInfoData& in,
        aidl::android::hardware::radio::SsInfoData* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::StkCcUnsolSsResult& in,
        aidl::android::hardware::radio::StkCcUnsolSsResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::PcoDataInfo& in,
        aidl::android::hardware::radio::PcoDataInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_1::KeepaliveRequest& in,
        aidl::android::hardware::radio::KeepaliveRequest* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_1::KeepaliveStatus& in,
        aidl::android::hardware::radio::KeepaliveStatus* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellIdentityOperatorNames& in,
        aidl::android::hardware::radio::CellIdentityOperatorNames* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellIdentityCdma& in,
        aidl::android::hardware::radio::CellIdentityCdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellInfoCdma& in,
        aidl::android::hardware::radio::CellInfoCdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::WcdmaSignalStrength& in,
        aidl::android::hardware::radio::WcdmaSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::TdscdmaSignalStrength& in,
        aidl::android::hardware::radio::TdscdmaSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::VoiceRegStateResult& in,
        aidl::android::hardware::radio::VoiceRegStateResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_3::RadioResponseInfoModem& in,
        aidl::android::hardware::radio::RadioResponseInfoModem* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::EmergencyNumber& in,
        aidl::android::hardware::radio::EmergencyNumber* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::RadioFrequencyInfo& in,
        aidl::android::hardware::radio::RadioFrequencyInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::LteVopsInfo& in,
        aidl::android::hardware::radio::LteVopsInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::NrIndicators& in,
        aidl::android::hardware::radio::NrIndicators* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::DataRegStateResult& in,
        aidl::android::hardware::radio::DataRegStateResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::DataRegStateResult::VopsInfo& in,
        aidl::android::hardware::radio::DataRegStateResultVopsInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CellConfigLte& in,
        aidl::android::hardware::radio::CellConfigLte* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CellInfo::Info& in,
        aidl::android::hardware::radio::CellInfoInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::RadioCapability& in,
        aidl::android::hardware::radio::RadioCapability* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CarrierRestrictionsWithPriority& in,
        aidl::android::hardware::radio::CarrierRestrictionsWithPriority* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RadioAccessSpecifier& in,
        aidl::android::hardware::radio::RadioAccessSpecifier* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands& in,
        aidl::android::hardware::radio::RadioAccessSpecifierBands* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::SignalThresholdInfo& in,
        aidl::android::hardware::radio::SignalThresholdInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::NetworkScanRequest& in,
        aidl::android::hardware::radio::NetworkScanRequest* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::DataProfileInfo& in,
        aidl::android::hardware::radio::DataProfileInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::LinkAddress& in,
        aidl::android::hardware::radio::LinkAddress* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::ClosedSubscriberGroupInfo& in,
        aidl::android::hardware::radio::ClosedSubscriberGroupInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::OptionalCsgInfo& in,
        aidl::android::hardware::radio::OptionalCsgInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityGsm& in,
        aidl::android::hardware::radio::CellIdentityGsm* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityWcdma& in,
        aidl::android::hardware::radio::CellIdentityWcdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityTdscdma& in,
        aidl::android::hardware::radio::CellIdentityTdscdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityLte& in,
        aidl::android::hardware::radio::CellIdentityLte* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityNr& in,
        aidl::android::hardware::radio::CellIdentityNr* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoGsm& in,
        aidl::android::hardware::radio::CellInfoGsm* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoWcdma& in,
        aidl::android::hardware::radio::CellInfoWcdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoTdscdma& in,
        aidl::android::hardware::radio::CellInfoTdscdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentity& in,
        aidl::android::hardware::radio::CellIdentity* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo& in,
        aidl::android::hardware::radio::BarringInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo::BarringTypeSpecificInfo::Conditional&
                in,
        aidl::android::hardware::radio::BarringInfoBarringTypeSpecificInfoConditional* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo::BarringTypeSpecificInfo& in,
        aidl::android::hardware::radio::BarringInfoBarringTypeSpecificInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RegStateResult::AccessTechnologySpecificInfo::
                Cdma2000RegistrationInfo& in,
        aidl::android::hardware::radio::
                RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RegStateResult::AccessTechnologySpecificInfo::
                EutranRegistrationInfo& in,
        aidl::android::hardware::radio::
                RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::AppStatus& in,
        aidl::android::hardware::radio::AppStatus* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CardStatus& in,
        aidl::android::hardware::radio::CardStatus* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosBandwidth& in,
        aidl::android::hardware::radio::QosBandwidth* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::EpsQos& in,
        aidl::android::hardware::radio::EpsQos* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrQos& in,
        aidl::android::hardware::radio::NrQos* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_6::Qos& in,
                                                   aidl::android::hardware::radio::Qos* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& in,
        aidl::android::hardware::radio::RadioResponseInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PortRange& in,
        aidl::android::hardware::radio::PortRange* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::MaybePort& in,
        aidl::android::hardware::radio::MaybePort* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter& in,
        aidl::android::hardware::radio::QosFilter* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::TypeOfService& in,
        aidl::android::hardware::radio::QosFilterTypeOfService* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::Ipv6FlowLabel& in,
        aidl::android::hardware::radio::QosFilterIpv6FlowLabel* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::IpsecSpi& in,
        aidl::android::hardware::radio::QosFilterIpsecSpi* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosSession& in,
        aidl::android::hardware::radio::QosSession* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SetupDataCallResult& in,
        aidl::android::hardware::radio::SetupDataCallResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::LinkCapacityEstimate& in,
        aidl::android::hardware::radio::LinkCapacityEstimate* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrVopsInfo& in,
        aidl::android::hardware::radio::NrVopsInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::LteSignalStrength& in,
        aidl::android::hardware::radio::LteSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrSignalStrength& in,
        aidl::android::hardware::radio::NrSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SignalStrength& in,
        aidl::android::hardware::radio::SignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfoLte& in,
        aidl::android::hardware::radio::CellInfoLte* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfoNr& in,
        aidl::android::hardware::radio::CellInfoNr* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfo& in,
        aidl::android::hardware::radio::CellInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo& in,
        aidl::android::hardware::radio::CellInfoCellInfoRatSpecificInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NetworkScanResult& in,
        aidl::android::hardware::radio::NetworkScanResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RegStateResult& in,
        aidl::android::hardware::radio::RegStateResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RegStateResult::AccessTechnologySpecificInfo& in,
        aidl::android::hardware::radio::RegStateResultAccessTechnologySpecificInfo* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_6::Call& in,
                                                   aidl::android::hardware::radio::Call* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhysicalChannelConfig& in,
        aidl::android::hardware::radio::PhysicalChannelConfig* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhysicalChannelConfig::Band& in,
        aidl::android::hardware::radio::PhysicalChannelConfigBand* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalSliceInfo& in,
        aidl::android::hardware::radio::OptionalSliceInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SliceInfo& in,
        aidl::android::hardware::radio::SliceInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalDnn& in,
        aidl::android::hardware::radio::OptionalDnn* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalOsAppId& in,
        aidl::android::hardware::radio::OptionalOsAppId* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalTrafficDescriptor& in,
        aidl::android::hardware::radio::OptionalTrafficDescriptor* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::TrafficDescriptor& in,
        aidl::android::hardware::radio::TrafficDescriptor* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OsAppId& in,
        aidl::android::hardware::radio::OsAppId* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SlicingConfig& in,
        aidl::android::hardware::radio::SlicingConfig* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::UrspRule& in,
        aidl::android::hardware::radio::UrspRule* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RouteSelectionDescriptor& in,
        aidl::android::hardware::radio::RouteSelectionDescriptor* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalPdpProtocolType& in,
        aidl::android::hardware::radio::OptionalPdpProtocolType* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalSscMode& in,
        aidl::android::hardware::radio::OptionalSscMode* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::ImsiEncryptionInfo& in,
        aidl::android::hardware::radio::ImsiEncryptionInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhonebookRecordInfo& in,
        aidl::android::hardware::radio::PhonebookRecordInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhonebookCapacity& in,
        aidl::android::hardware::radio::PhonebookCapacity* out);

}  // namespace android::h2a
