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

package android.hardware.radio;

import java.util.List;

public class Translate {
    static public android.hardware.radio.IccIo h2aTranslate(android.hardware.radio.V1_0.IccIo in) {
        android.hardware.radio.IccIo out = new android.hardware.radio.IccIo();
        out.command = in.command;
        out.fileId = in.fileId;
        out.path = in.path;
        out.p1 = in.p1;
        out.p2 = in.p2;
        out.p3 = in.p3;
        out.data = in.data;
        out.pin2 = in.pin2;
        out.aid = in.aid;
        return out;
    }

    static public android.hardware.radio.NeighboringCell h2aTranslate(
            android.hardware.radio.V1_0.NeighboringCell in) {
        android.hardware.radio.NeighboringCell out = new android.hardware.radio.NeighboringCell();
        out.cid = in.cid;
        out.rssi = in.rssi;
        return out;
    }

    static public android.hardware.radio.UusInfo h2aTranslate(
            android.hardware.radio.V1_0.UusInfo in) {
        android.hardware.radio.UusInfo out = new android.hardware.radio.UusInfo();
        out.uusType = in.uusType;
        out.uusDcs = in.uusDcs;
        out.uusData = in.uusData;
        return out;
    }

    static public android.hardware.radio.Dial h2aTranslate(android.hardware.radio.V1_0.Dial in) {
        android.hardware.radio.Dial out = new android.hardware.radio.Dial();
        out.address = in.address;
        out.clir = in.clir;
        if (in.uusInfo != null) {
            out.uusInfo = new android.hardware.radio.UusInfo[in.uusInfo.size()];
            for (int i = 0; i < in.uusInfo.size(); i++) {
                out.uusInfo[i] = h2aTranslate(in.uusInfo.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.LastCallFailCauseInfo h2aTranslate(
            android.hardware.radio.V1_0.LastCallFailCauseInfo in) {
        android.hardware.radio.LastCallFailCauseInfo out =
                new android.hardware.radio.LastCallFailCauseInfo();
        out.causeCode = in.causeCode;
        out.vendorCause = in.vendorCause;
        return out;
    }

    static public android.hardware.radio.GsmSignalStrength h2aTranslate(
            android.hardware.radio.V1_0.GsmSignalStrength in) {
        android.hardware.radio.GsmSignalStrength out =
                new android.hardware.radio.GsmSignalStrength();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.signalStrength > 2147483647 || in.signalStrength < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.signalStrength");
        }
        out.signalStrength = in.signalStrength;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.bitErrorRate > 2147483647 || in.bitErrorRate < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.bitErrorRate");
        }
        out.bitErrorRate = in.bitErrorRate;
        out.timingAdvance = in.timingAdvance;
        return out;
    }

    static public android.hardware.radio.CdmaSignalStrength h2aTranslate(
            android.hardware.radio.V1_0.CdmaSignalStrength in) {
        android.hardware.radio.CdmaSignalStrength out =
                new android.hardware.radio.CdmaSignalStrength();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.dbm > 2147483647 || in.dbm < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.dbm");
        }
        out.dbm = in.dbm;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.ecio > 2147483647 || in.ecio < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.ecio");
        }
        out.ecio = in.ecio;
        return out;
    }

    static public android.hardware.radio.EvdoSignalStrength h2aTranslate(
            android.hardware.radio.V1_0.EvdoSignalStrength in) {
        android.hardware.radio.EvdoSignalStrength out =
                new android.hardware.radio.EvdoSignalStrength();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.dbm > 2147483647 || in.dbm < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.dbm");
        }
        out.dbm = in.dbm;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.ecio > 2147483647 || in.ecio < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.ecio");
        }
        out.ecio = in.ecio;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.signalNoiseRatio > 2147483647 || in.signalNoiseRatio < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.signalNoiseRatio");
        }
        out.signalNoiseRatio = in.signalNoiseRatio;
        return out;
    }

    static public android.hardware.radio.SendSmsResult h2aTranslate(
            android.hardware.radio.V1_0.SendSmsResult in) {
        android.hardware.radio.SendSmsResult out = new android.hardware.radio.SendSmsResult();
        out.messageRef = in.messageRef;
        out.ackPDU = in.ackPDU;
        out.errorCode = in.errorCode;
        return out;
    }

    static public android.hardware.radio.IccIoResult h2aTranslate(
            android.hardware.radio.V1_0.IccIoResult in) {
        android.hardware.radio.IccIoResult out = new android.hardware.radio.IccIoResult();
        out.sw1 = in.sw1;
        out.sw2 = in.sw2;
        out.simResponse = in.simResponse;
        return out;
    }

    static public android.hardware.radio.CallForwardInfo h2aTranslate(
            android.hardware.radio.V1_0.CallForwardInfo in) {
        android.hardware.radio.CallForwardInfo out = new android.hardware.radio.CallForwardInfo();
        out.status = in.status;
        out.reason = in.reason;
        out.serviceClass = in.serviceClass;
        out.toa = in.toa;
        out.number = in.number;
        out.timeSeconds = in.timeSeconds;
        return out;
    }

    static public android.hardware.radio.OperatorInfo h2aTranslate(
            android.hardware.radio.V1_0.OperatorInfo in) {
        android.hardware.radio.OperatorInfo out = new android.hardware.radio.OperatorInfo();
        out.alphaLong = in.alphaLong;
        out.alphaShort = in.alphaShort;
        out.operatorNumeric = in.operatorNumeric;
        out.status = in.status;
        return out;
    }

    static public android.hardware.radio.SmsWriteArgs h2aTranslate(
            android.hardware.radio.V1_0.SmsWriteArgs in) {
        android.hardware.radio.SmsWriteArgs out = new android.hardware.radio.SmsWriteArgs();
        out.status = in.status;
        out.pdu = in.pdu;
        out.smsc = in.smsc;
        return out;
    }

    static public android.hardware.radio.CdmaSmsAddress h2aTranslate(
            android.hardware.radio.V1_0.CdmaSmsAddress in) {
        android.hardware.radio.CdmaSmsAddress out = new android.hardware.radio.CdmaSmsAddress();
        out.digitMode = in.digitMode;
        out.numberMode = in.numberMode;
        out.numberType = in.numberType;
        out.numberPlan = in.numberPlan;
        if (in.digits != null) {
            out.digits = new byte[in.digits.size()];
            for (int i = 0; i < in.digits.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.digits.get(i) > 127 || in.digits.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.digits.get(i)");
                }
                out.digits[i] = in.digits.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.CdmaSmsSubaddress h2aTranslate(
            android.hardware.radio.V1_0.CdmaSmsSubaddress in) {
        android.hardware.radio.CdmaSmsSubaddress out =
                new android.hardware.radio.CdmaSmsSubaddress();
        out.subaddressType = in.subaddressType;
        out.odd = in.odd;
        if (in.digits != null) {
            out.digits = new byte[in.digits.size()];
            for (int i = 0; i < in.digits.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.digits.get(i) > 127 || in.digits.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.digits.get(i)");
                }
                out.digits[i] = in.digits.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.CdmaSmsMessage h2aTranslate(
            android.hardware.radio.V1_0.CdmaSmsMessage in) {
        android.hardware.radio.CdmaSmsMessage out = new android.hardware.radio.CdmaSmsMessage();
        out.teleserviceId = in.teleserviceId;
        out.isServicePresent = in.isServicePresent;
        out.serviceCategory = in.serviceCategory;
        out.address = h2aTranslate(in.address);
        out.subAddress = h2aTranslate(in.subAddress);
        if (in.bearerData != null) {
            out.bearerData = new byte[in.bearerData.size()];
            for (int i = 0; i < in.bearerData.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.bearerData.get(i) > 127 || in.bearerData.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.bearerData.get(i)");
                }
                out.bearerData[i] = in.bearerData.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.CdmaSmsAck h2aTranslate(
            android.hardware.radio.V1_0.CdmaSmsAck in) {
        android.hardware.radio.CdmaSmsAck out = new android.hardware.radio.CdmaSmsAck();
        out.errorClass = in.errorClass;
        out.smsCauseCode = in.smsCauseCode;
        return out;
    }

    static public android.hardware.radio.CdmaBroadcastSmsConfigInfo h2aTranslate(
            android.hardware.radio.V1_0.CdmaBroadcastSmsConfigInfo in) {
        android.hardware.radio.CdmaBroadcastSmsConfigInfo out =
                new android.hardware.radio.CdmaBroadcastSmsConfigInfo();
        out.serviceCategory = in.serviceCategory;
        out.language = in.language;
        out.selected = in.selected;
        return out;
    }

    static public android.hardware.radio.CdmaSmsWriteArgs h2aTranslate(
            android.hardware.radio.V1_0.CdmaSmsWriteArgs in) {
        android.hardware.radio.CdmaSmsWriteArgs out = new android.hardware.radio.CdmaSmsWriteArgs();
        out.status = in.status;
        out.message = h2aTranslate(in.message);
        return out;
    }

    static public android.hardware.radio.GsmBroadcastSmsConfigInfo h2aTranslate(
            android.hardware.radio.V1_0.GsmBroadcastSmsConfigInfo in) {
        android.hardware.radio.GsmBroadcastSmsConfigInfo out =
                new android.hardware.radio.GsmBroadcastSmsConfigInfo();
        out.fromServiceId = in.fromServiceId;
        out.toServiceId = in.toServiceId;
        out.fromCodeScheme = in.fromCodeScheme;
        out.toCodeScheme = in.toCodeScheme;
        out.selected = in.selected;
        return out;
    }

    static public android.hardware.radio.GsmSmsMessage h2aTranslate(
            android.hardware.radio.V1_0.GsmSmsMessage in) {
        android.hardware.radio.GsmSmsMessage out = new android.hardware.radio.GsmSmsMessage();
        out.smscPdu = in.smscPdu;
        out.pdu = in.pdu;
        return out;
    }

    static public android.hardware.radio.ImsSmsMessage h2aTranslate(
            android.hardware.radio.V1_0.ImsSmsMessage in) {
        android.hardware.radio.ImsSmsMessage out = new android.hardware.radio.ImsSmsMessage();
        out.tech = in.tech;
        out.retry = in.retry;
        out.messageRef = in.messageRef;
        if (in.cdmaMessage != null) {
            out.cdmaMessage = new android.hardware.radio.CdmaSmsMessage[in.cdmaMessage.size()];
            for (int i = 0; i < in.cdmaMessage.size(); i++) {
                out.cdmaMessage[i] = h2aTranslate(in.cdmaMessage.get(i));
            }
        }
        if (in.gsmMessage != null) {
            out.gsmMessage = new android.hardware.radio.GsmSmsMessage[in.gsmMessage.size()];
            for (int i = 0; i < in.gsmMessage.size(); i++) {
                out.gsmMessage[i] = h2aTranslate(in.gsmMessage.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.SimApdu h2aTranslate(
            android.hardware.radio.V1_0.SimApdu in) {
        android.hardware.radio.SimApdu out = new android.hardware.radio.SimApdu();
        out.sessionId = in.sessionId;
        out.cla = in.cla;
        out.instruction = in.instruction;
        out.p1 = in.p1;
        out.p2 = in.p2;
        out.p3 = in.p3;
        out.data = in.data;
        return out;
    }

    static public android.hardware.radio.NvWriteItem h2aTranslate(
            android.hardware.radio.V1_0.NvWriteItem in) {
        android.hardware.radio.NvWriteItem out = new android.hardware.radio.NvWriteItem();
        out.itemId = in.itemId;
        out.value = in.value;
        return out;
    }

    static public android.hardware.radio.SelectUiccSub h2aTranslate(
            android.hardware.radio.V1_0.SelectUiccSub in) {
        android.hardware.radio.SelectUiccSub out = new android.hardware.radio.SelectUiccSub();
        out.slot = in.slot;
        out.appIndex = in.appIndex;
        out.subType = in.subType;
        out.actStatus = in.actStatus;
        return out;
    }

    static public android.hardware.radio.HardwareConfigModem h2aTranslate(
            android.hardware.radio.V1_0.HardwareConfigModem in) {
        android.hardware.radio.HardwareConfigModem out =
                new android.hardware.radio.HardwareConfigModem();
        out.rilModel = in.rilModel;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.rat > 2147483647 || in.rat < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.rat");
        }
        out.rat = in.rat;
        out.maxVoice = in.maxVoice;
        out.maxData = in.maxData;
        out.maxStandby = in.maxStandby;
        return out;
    }

    static public android.hardware.radio.HardwareConfigSim h2aTranslate(
            android.hardware.radio.V1_0.HardwareConfigSim in) {
        android.hardware.radio.HardwareConfigSim out =
                new android.hardware.radio.HardwareConfigSim();
        out.modemUuid = in.modemUuid;
        return out;
    }

    static public android.hardware.radio.HardwareConfig h2aTranslate(
            android.hardware.radio.V1_0.HardwareConfig in) {
        android.hardware.radio.HardwareConfig out = new android.hardware.radio.HardwareConfig();
        out.type = in.type;
        out.uuid = in.uuid;
        out.state = in.state;
        if (in.modem != null) {
            out.modem = new android.hardware.radio.HardwareConfigModem[in.modem.size()];
            for (int i = 0; i < in.modem.size(); i++) {
                out.modem[i] = h2aTranslate(in.modem.get(i));
            }
        }
        if (in.sim != null) {
            out.sim = new android.hardware.radio.HardwareConfigSim[in.sim.size()];
            for (int i = 0; i < in.sim.size(); i++) {
                out.sim[i] = h2aTranslate(in.sim.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.LceStatusInfo h2aTranslate(
            android.hardware.radio.V1_0.LceStatusInfo in) {
        android.hardware.radio.LceStatusInfo out = new android.hardware.radio.LceStatusInfo();
        out.lceStatus = in.lceStatus;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.actualIntervalMs > 127 || in.actualIntervalMs < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.actualIntervalMs");
        }
        out.actualIntervalMs = in.actualIntervalMs;
        return out;
    }

    static public android.hardware.radio.LceDataInfo h2aTranslate(
            android.hardware.radio.V1_0.LceDataInfo in) {
        android.hardware.radio.LceDataInfo out = new android.hardware.radio.LceDataInfo();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.lastHopCapacityKbps > 2147483647 || in.lastHopCapacityKbps < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.lastHopCapacityKbps");
        }
        out.lastHopCapacityKbps = in.lastHopCapacityKbps;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.confidenceLevel > 127 || in.confidenceLevel < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.confidenceLevel");
        }
        out.confidenceLevel = in.confidenceLevel;
        out.lceSuspended = in.lceSuspended;
        return out;
    }

    static public android.hardware.radio.ActivityStatsInfo h2aTranslate(
            android.hardware.radio.V1_0.ActivityStatsInfo in) {
        android.hardware.radio.ActivityStatsInfo out =
                new android.hardware.radio.ActivityStatsInfo();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.sleepModeTimeMs > 2147483647 || in.sleepModeTimeMs < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.sleepModeTimeMs");
        }
        out.sleepModeTimeMs = in.sleepModeTimeMs;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.idleModeTimeMs > 2147483647 || in.idleModeTimeMs < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.idleModeTimeMs");
        }
        out.idleModeTimeMs = in.idleModeTimeMs;
        if (in.txmModetimeMs != null) {
            out.txmModetimeMs = new int[in.txmModetimeMs.length];
            for (int i = 0; i < in.txmModetimeMs.length; i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.txmModetimeMs[i] > 2147483647 || in.txmModetimeMs[i] < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.txmModetimeMs[i]");
                }
                out.txmModetimeMs[i] = in.txmModetimeMs[i];
            }
        }
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.rxModeTimeMs > 2147483647 || in.rxModeTimeMs < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.rxModeTimeMs");
        }
        out.rxModeTimeMs = in.rxModeTimeMs;
        return out;
    }

    static public android.hardware.radio.Carrier h2aTranslate(
            android.hardware.radio.V1_0.Carrier in) {
        android.hardware.radio.Carrier out = new android.hardware.radio.Carrier();
        out.mcc = in.mcc;
        out.mnc = in.mnc;
        out.matchType = in.matchType;
        out.matchData = in.matchData;
        return out;
    }

    static public android.hardware.radio.CarrierRestrictions h2aTranslate(
            android.hardware.radio.V1_0.CarrierRestrictions in) {
        android.hardware.radio.CarrierRestrictions out =
                new android.hardware.radio.CarrierRestrictions();
        if (in.allowedCarriers != null) {
            out.allowedCarriers = new android.hardware.radio.Carrier[in.allowedCarriers.size()];
            for (int i = 0; i < in.allowedCarriers.size(); i++) {
                out.allowedCarriers[i] = h2aTranslate(in.allowedCarriers.get(i));
            }
        }
        if (in.excludedCarriers != null) {
            out.excludedCarriers = new android.hardware.radio.Carrier[in.excludedCarriers.size()];
            for (int i = 0; i < in.excludedCarriers.size(); i++) {
                out.excludedCarriers[i] = h2aTranslate(in.excludedCarriers.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.SuppSvcNotification h2aTranslate(
            android.hardware.radio.V1_0.SuppSvcNotification in) {
        android.hardware.radio.SuppSvcNotification out =
                new android.hardware.radio.SuppSvcNotification();
        out.isMT = in.isMT;
        out.code = in.code;
        out.index = in.index;
        out.type = in.type;
        out.number = in.number;
        return out;
    }

    static public android.hardware.radio.SimRefreshResult h2aTranslate(
            android.hardware.radio.V1_0.SimRefreshResult in) {
        android.hardware.radio.SimRefreshResult out = new android.hardware.radio.SimRefreshResult();
        out.type = in.type;
        out.efId = in.efId;
        out.aid = in.aid;
        return out;
    }

    static public android.hardware.radio.CdmaSignalInfoRecord h2aTranslate(
            android.hardware.radio.V1_0.CdmaSignalInfoRecord in) {
        android.hardware.radio.CdmaSignalInfoRecord out =
                new android.hardware.radio.CdmaSignalInfoRecord();
        out.isPresent = in.isPresent;
        out.signalType = in.signalType;
        out.alertPitch = in.alertPitch;
        out.signal = in.signal;
        return out;
    }

    static public android.hardware.radio.CdmaCallWaiting h2aTranslate(
            android.hardware.radio.V1_0.CdmaCallWaiting in) {
        android.hardware.radio.CdmaCallWaiting out = new android.hardware.radio.CdmaCallWaiting();
        out.number = in.number;
        out.numberPresentation = in.numberPresentation;
        out.name = in.name;
        out.signalInfoRecord = h2aTranslate(in.signalInfoRecord);
        out.numberType = in.numberType;
        out.numberPlan = in.numberPlan;
        return out;
    }

    static public android.hardware.radio.CdmaDisplayInfoRecord h2aTranslate(
            android.hardware.radio.V1_0.CdmaDisplayInfoRecord in) {
        android.hardware.radio.CdmaDisplayInfoRecord out =
                new android.hardware.radio.CdmaDisplayInfoRecord();
        out.alphaBuf = in.alphaBuf;
        return out;
    }

    static public android.hardware.radio.CdmaNumberInfoRecord h2aTranslate(
            android.hardware.radio.V1_0.CdmaNumberInfoRecord in) {
        android.hardware.radio.CdmaNumberInfoRecord out =
                new android.hardware.radio.CdmaNumberInfoRecord();
        out.number = in.number;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.numberType > 127 || in.numberType < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.numberType");
        }
        out.numberType = in.numberType;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.numberPlan > 127 || in.numberPlan < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.numberPlan");
        }
        out.numberPlan = in.numberPlan;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.pi > 127 || in.pi < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.pi");
        }
        out.pi = in.pi;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.si > 127 || in.si < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.si");
        }
        out.si = in.si;
        return out;
    }

    static public android.hardware.radio.CdmaRedirectingNumberInfoRecord h2aTranslate(
            android.hardware.radio.V1_0.CdmaRedirectingNumberInfoRecord in) {
        android.hardware.radio.CdmaRedirectingNumberInfoRecord out =
                new android.hardware.radio.CdmaRedirectingNumberInfoRecord();
        out.redirectingNumber = h2aTranslate(in.redirectingNumber);
        out.redirectingReason = in.redirectingReason;
        return out;
    }

    static public android.hardware.radio.CdmaLineControlInfoRecord h2aTranslate(
            android.hardware.radio.V1_0.CdmaLineControlInfoRecord in) {
        android.hardware.radio.CdmaLineControlInfoRecord out =
                new android.hardware.radio.CdmaLineControlInfoRecord();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.lineCtrlPolarityIncluded > 127 || in.lineCtrlPolarityIncluded < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.lineCtrlPolarityIncluded");
        }
        out.lineCtrlPolarityIncluded = in.lineCtrlPolarityIncluded;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.lineCtrlToggle > 127 || in.lineCtrlToggle < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.lineCtrlToggle");
        }
        out.lineCtrlToggle = in.lineCtrlToggle;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.lineCtrlReverse > 127 || in.lineCtrlReverse < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.lineCtrlReverse");
        }
        out.lineCtrlReverse = in.lineCtrlReverse;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.lineCtrlPowerDenial > 127 || in.lineCtrlPowerDenial < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.lineCtrlPowerDenial");
        }
        out.lineCtrlPowerDenial = in.lineCtrlPowerDenial;
        return out;
    }

    static public android.hardware.radio.CdmaT53ClirInfoRecord h2aTranslate(
            android.hardware.radio.V1_0.CdmaT53ClirInfoRecord in) {
        android.hardware.radio.CdmaT53ClirInfoRecord out =
                new android.hardware.radio.CdmaT53ClirInfoRecord();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.cause > 127 || in.cause < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.cause");
        }
        out.cause = in.cause;
        return out;
    }

    static public android.hardware.radio.CdmaT53AudioControlInfoRecord h2aTranslate(
            android.hardware.radio.V1_0.CdmaT53AudioControlInfoRecord in) {
        android.hardware.radio.CdmaT53AudioControlInfoRecord out =
                new android.hardware.radio.CdmaT53AudioControlInfoRecord();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.upLink > 127 || in.upLink < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.upLink");
        }
        out.upLink = in.upLink;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.downLink > 127 || in.downLink < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.downLink");
        }
        out.downLink = in.downLink;
        return out;
    }

    static public android.hardware.radio.CdmaInformationRecord h2aTranslate(
            android.hardware.radio.V1_0.CdmaInformationRecord in) {
        android.hardware.radio.CdmaInformationRecord out =
                new android.hardware.radio.CdmaInformationRecord();
        out.name = in.name;
        if (in.display != null) {
            out.display = new android.hardware.radio.CdmaDisplayInfoRecord[in.display.size()];
            for (int i = 0; i < in.display.size(); i++) {
                out.display[i] = h2aTranslate(in.display.get(i));
            }
        }
        if (in.number != null) {
            out.number = new android.hardware.radio.CdmaNumberInfoRecord[in.number.size()];
            for (int i = 0; i < in.number.size(); i++) {
                out.number[i] = h2aTranslate(in.number.get(i));
            }
        }
        if (in.signal != null) {
            out.signal = new android.hardware.radio.CdmaSignalInfoRecord[in.signal.size()];
            for (int i = 0; i < in.signal.size(); i++) {
                out.signal[i] = h2aTranslate(in.signal.get(i));
            }
        }
        if (in.redir != null) {
            out.redir = new android.hardware.radio.CdmaRedirectingNumberInfoRecord[in.redir.size()];
            for (int i = 0; i < in.redir.size(); i++) {
                out.redir[i] = h2aTranslate(in.redir.get(i));
            }
        }
        if (in.lineCtrl != null) {
            out.lineCtrl = new android.hardware.radio.CdmaLineControlInfoRecord[in.lineCtrl.size()];
            for (int i = 0; i < in.lineCtrl.size(); i++) {
                out.lineCtrl[i] = h2aTranslate(in.lineCtrl.get(i));
            }
        }
        if (in.clir != null) {
            out.clir = new android.hardware.radio.CdmaT53ClirInfoRecord[in.clir.size()];
            for (int i = 0; i < in.clir.size(); i++) {
                out.clir[i] = h2aTranslate(in.clir.get(i));
            }
        }
        if (in.audioCtrl != null) {
            out.audioCtrl =
                    new android.hardware.radio.CdmaT53AudioControlInfoRecord[in.audioCtrl.size()];
            for (int i = 0; i < in.audioCtrl.size(); i++) {
                out.audioCtrl[i] = h2aTranslate(in.audioCtrl.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.CdmaInformationRecords h2aTranslate(
            android.hardware.radio.V1_0.CdmaInformationRecords in) {
        android.hardware.radio.CdmaInformationRecords out =
                new android.hardware.radio.CdmaInformationRecords();
        if (in.infoRec != null) {
            out.infoRec = new android.hardware.radio.CdmaInformationRecord[in.infoRec.size()];
            for (int i = 0; i < in.infoRec.size(); i++) {
                out.infoRec[i] = h2aTranslate(in.infoRec.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.CfData h2aTranslate(
            android.hardware.radio.V1_0.CfData in) {
        android.hardware.radio.CfData out = new android.hardware.radio.CfData();
        if (in.cfInfo != null) {
            out.cfInfo = new android.hardware.radio.CallForwardInfo[in.cfInfo.size()];
            for (int i = 0; i < in.cfInfo.size(); i++) {
                out.cfInfo[i] = h2aTranslate(in.cfInfo.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.SsInfoData h2aTranslate(
            android.hardware.radio.V1_0.SsInfoData in) {
        android.hardware.radio.SsInfoData out = new android.hardware.radio.SsInfoData();
        if (in.ssInfo != null) {
            out.ssInfo = new int[in.ssInfo.size()];
            for (int i = 0; i < in.ssInfo.size(); i++) {
                out.ssInfo[i] = in.ssInfo.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.StkCcUnsolSsResult h2aTranslate(
            android.hardware.radio.V1_0.StkCcUnsolSsResult in) {
        android.hardware.radio.StkCcUnsolSsResult out =
                new android.hardware.radio.StkCcUnsolSsResult();
        out.serviceType = in.serviceType;
        out.requestType = in.requestType;
        out.teleserviceType = in.teleserviceType;
        out.serviceClass = in.serviceClass;
        out.result = in.result;
        if (in.ssInfo != null) {
            out.ssInfo = new android.hardware.radio.SsInfoData[in.ssInfo.size()];
            for (int i = 0; i < in.ssInfo.size(); i++) {
                out.ssInfo[i] = h2aTranslate(in.ssInfo.get(i));
            }
        }
        if (in.cfData != null) {
            out.cfData = new android.hardware.radio.CfData[in.cfData.size()];
            for (int i = 0; i < in.cfData.size(); i++) {
                out.cfData[i] = h2aTranslate(in.cfData.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.PcoDataInfo h2aTranslate(
            android.hardware.radio.V1_0.PcoDataInfo in) {
        android.hardware.radio.PcoDataInfo out = new android.hardware.radio.PcoDataInfo();
        out.cid = in.cid;
        out.bearerProto = in.bearerProto;
        out.pcoId = in.pcoId;
        if (in.contents != null) {
            out.contents = new byte[in.contents.size()];
            for (int i = 0; i < in.contents.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.contents.get(i) > 127 || in.contents.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.contents.get(i)");
                }
                out.contents[i] = in.contents.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.KeepaliveRequest h2aTranslate(
            android.hardware.radio.V1_1.KeepaliveRequest in) {
        android.hardware.radio.KeepaliveRequest out = new android.hardware.radio.KeepaliveRequest();
        out.type = in.type;
        if (in.sourceAddress != null) {
            out.sourceAddress = new byte[in.sourceAddress.size()];
            for (int i = 0; i < in.sourceAddress.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.sourceAddress.get(i) > 127 || in.sourceAddress.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.sourceAddress.get(i)");
                }
                out.sourceAddress[i] = in.sourceAddress.get(i);
            }
        }
        out.sourcePort = in.sourcePort;
        if (in.destinationAddress != null) {
            out.destinationAddress = new byte[in.destinationAddress.size()];
            for (int i = 0; i < in.destinationAddress.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.destinationAddress.get(i) > 127 || in.destinationAddress.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.destinationAddress.get(i)");
                }
                out.destinationAddress[i] = in.destinationAddress.get(i);
            }
        }
        out.destinationPort = in.destinationPort;
        out.maxKeepaliveIntervalMillis = in.maxKeepaliveIntervalMillis;
        out.cid = in.cid;
        return out;
    }

    static public android.hardware.radio.KeepaliveStatus h2aTranslate(
            android.hardware.radio.V1_1.KeepaliveStatus in) {
        android.hardware.radio.KeepaliveStatus out = new android.hardware.radio.KeepaliveStatus();
        out.sessionHandle = in.sessionHandle;
        out.code = in.code;
        return out;
    }

    static public android.hardware.radio.CellIdentityOperatorNames h2aTranslate(
            android.hardware.radio.V1_2.CellIdentityOperatorNames in) {
        android.hardware.radio.CellIdentityOperatorNames out =
                new android.hardware.radio.CellIdentityOperatorNames();
        out.alphaLong = in.alphaLong;
        out.alphaShort = in.alphaShort;
        return out;
    }

    static public android.hardware.radio.CellIdentityCdma h2aTranslate(
            android.hardware.radio.V1_2.CellIdentityCdma in) {
        android.hardware.radio.CellIdentityCdma out = new android.hardware.radio.CellIdentityCdma();
        out.networkId = in.base.networkId;
        out.systemId = in.base.systemId;
        out.baseStationId = in.base.baseStationId;
        out.longitude = in.base.longitude;
        out.latitude = in.base.latitude;
        out.operatorNames = h2aTranslate(in.operatorNames);
        return out;
    }

    static public android.hardware.radio.CellInfoCdma h2aTranslate(
            android.hardware.radio.V1_2.CellInfoCdma in) {
        android.hardware.radio.CellInfoCdma out = new android.hardware.radio.CellInfoCdma();
        out.cellIdentityCdma = h2aTranslate(in.cellIdentityCdma);
        out.signalStrengthCdma = h2aTranslate(in.signalStrengthCdma);
        out.signalStrengthEvdo = h2aTranslate(in.signalStrengthEvdo);
        return out;
    }

    static public android.hardware.radio.WcdmaSignalStrength h2aTranslate(
            android.hardware.radio.V1_2.WcdmaSignalStrength in) {
        android.hardware.radio.WcdmaSignalStrength out =
                new android.hardware.radio.WcdmaSignalStrength();
        out.signalStrength = in.base.signalStrength;
        out.bitErrorRate = in.base.bitErrorRate;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.rscp > 2147483647 || in.rscp < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.rscp");
        }
        out.rscp = in.rscp;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.ecno > 2147483647 || in.ecno < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.ecno");
        }
        out.ecno = in.ecno;
        return out;
    }

    static public android.hardware.radio.TdscdmaSignalStrength h2aTranslate(
            android.hardware.radio.V1_2.TdscdmaSignalStrength in) {
        android.hardware.radio.TdscdmaSignalStrength out =
                new android.hardware.radio.TdscdmaSignalStrength();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.signalStrength > 2147483647 || in.signalStrength < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.signalStrength");
        }
        out.signalStrength = in.signalStrength;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.bitErrorRate > 2147483647 || in.bitErrorRate < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.bitErrorRate");
        }
        out.bitErrorRate = in.bitErrorRate;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.rscp > 2147483647 || in.rscp < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.rscp");
        }
        out.rscp = in.rscp;
        return out;
    }

    static public android.hardware.radio.VoiceRegStateResult h2aTranslate(
            android.hardware.radio.V1_2.VoiceRegStateResult in) {
        android.hardware.radio.VoiceRegStateResult out =
                new android.hardware.radio.VoiceRegStateResult();
        out.regState = in.regState;
        out.rat = in.rat;
        out.cssSupported = in.cssSupported;
        out.roamingIndicator = in.roamingIndicator;
        out.systemIsInPrl = in.systemIsInPrl;
        out.defaultRoamingIndicator = in.defaultRoamingIndicator;
        out.reasonForDenial = in.reasonForDenial;
        // FIXME Unknown type: android.hardware.radio@1.2::CellIdentity
        // That type's package needs to be converted separately and the corresponding translate
        // function should be added here.
        return out;
    }

    static public android.hardware.radio.RadioResponseInfoModem h2aTranslate(
            android.hardware.radio.V1_3.RadioResponseInfoModem in) {
        android.hardware.radio.RadioResponseInfoModem out =
                new android.hardware.radio.RadioResponseInfoModem();
        out.type = in.type;
        out.serial = in.serial;
        out.error = in.error;
        out.isEnabled = in.isEnabled;
        return out;
    }

    static public android.hardware.radio.EmergencyNumber h2aTranslate(
            android.hardware.radio.V1_4.EmergencyNumber in) {
        android.hardware.radio.EmergencyNumber out = new android.hardware.radio.EmergencyNumber();
        out.number = in.number;
        out.mcc = in.mcc;
        out.mnc = in.mnc;
        out.categories = in.categories;
        if (in.urns != null) {
            out.urns = new String[in.urns.size()];
            for (int i = 0; i < in.urns.size(); i++) {
                out.urns[i] = in.urns.get(i);
            }
        }
        out.sources = in.sources;
        return out;
    }

    static public android.hardware.radio.RadioFrequencyInfo h2aTranslate(
            android.hardware.radio.V1_4.RadioFrequencyInfo in) {
        android.hardware.radio.RadioFrequencyInfo out =
                new android.hardware.radio.RadioFrequencyInfo();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_4.RadioFrequencyInfo.hidl_discriminator.range:
                out.setRange(in.range());
                break;
            case android.hardware.radio.V1_4.RadioFrequencyInfo.hidl_discriminator.channelNumber:
                out.setChannelNumber(in.channelNumber());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.LteVopsInfo h2aTranslate(
            android.hardware.radio.V1_4.LteVopsInfo in) {
        android.hardware.radio.LteVopsInfo out = new android.hardware.radio.LteVopsInfo();
        out.isVopsSupported = in.isVopsSupported;
        out.isEmcBearerSupported = in.isEmcBearerSupported;
        return out;
    }

    static public android.hardware.radio.NrIndicators h2aTranslate(
            android.hardware.radio.V1_4.NrIndicators in) {
        android.hardware.radio.NrIndicators out = new android.hardware.radio.NrIndicators();
        out.isEndcAvailable = in.isEndcAvailable;
        out.isDcNrRestricted = in.isDcNrRestricted;
        out.isNrAvailable = in.isNrAvailable;
        return out;
    }

    static public android.hardware.radio.DataRegStateResult h2aTranslate(
            android.hardware.radio.V1_4.DataRegStateResult in) {
        android.hardware.radio.DataRegStateResult out =
                new android.hardware.radio.DataRegStateResult();
        out.regState = in.base.regState;
        out.rat = in.base.rat;
        out.reasonDataDenied = in.base.reasonDataDenied;
        out.maxDataCalls = in.base.maxDataCalls;
        // FIXME Unknown type: android.hardware.radio@1.2::CellIdentity
        // That type's package needs to be converted separately and the corresponding translate
        // function should be added here.
        out.vopsInfo = h2aTranslate(in.vopsInfo);
        out.nrIndicators = h2aTranslate(in.nrIndicators);
        return out;
    }

    static public android.hardware.radio.DataRegStateResultVopsInfo h2aTranslate(
            android.hardware.radio.V1_4.DataRegStateResult.VopsInfo in) {
        android.hardware.radio.DataRegStateResultVopsInfo out =
                new android.hardware.radio.DataRegStateResultVopsInfo();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_4.DataRegStateResult.VopsInfo.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_4.DataRegStateResult.VopsInfo.hidl_discriminator
                    .lteVopsInfo:
                out.setLteVopsInfo(h2aTranslate(in.lteVopsInfo()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.CellConfigLte h2aTranslate(
            android.hardware.radio.V1_4.CellConfigLte in) {
        android.hardware.radio.CellConfigLte out = new android.hardware.radio.CellConfigLte();
        out.isEndcAvailable = in.isEndcAvailable;
        return out;
    }

    static public android.hardware.radio.CellInfoInfo h2aTranslate(
            android.hardware.radio.V1_4.CellInfo.Info in) {
        android.hardware.radio.CellInfoInfo out = new android.hardware.radio.CellInfoInfo();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_4.CellInfo.Info.hidl_discriminator.gsm:
                // FIXME Unknown type: android.hardware.radio@1.2::CellInfoGsm
                // That type's package needs to be converted separately and the corresponding
                // translate function should be added here.
                break;
            case android.hardware.radio.V1_4.CellInfo.Info.hidl_discriminator.cdma:
                out.setCdma(h2aTranslate(in.cdma()));
                break;
            case android.hardware.radio.V1_4.CellInfo.Info.hidl_discriminator.wcdma:
                // FIXME Unknown type: android.hardware.radio@1.2::CellInfoWcdma
                // That type's package needs to be converted separately and the corresponding
                // translate function should be added here.
                break;
            case android.hardware.radio.V1_4.CellInfo.Info.hidl_discriminator.tdscdma:
                // FIXME Unknown type: android.hardware.radio@1.2::CellInfoTdscdma
                // That type's package needs to be converted separately and the corresponding
                // translate function should be added here.
                break;
            case android.hardware.radio.V1_4.CellInfo.Info.hidl_discriminator.lte:
                // FIXME Unknown type: android.hardware.radio@1.4::CellInfoLte
                // That type's package needs to be converted separately and the corresponding
                // translate function should be added here.
                break;
            case android.hardware.radio.V1_4.CellInfo.Info.hidl_discriminator.nr:
                // FIXME Unknown type: android.hardware.radio@1.4::CellInfoNr
                // That type's package needs to be converted separately and the corresponding
                // translate function should be added here.
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.RadioCapability h2aTranslate(
            android.hardware.radio.V1_4.RadioCapability in) {
        android.hardware.radio.RadioCapability out = new android.hardware.radio.RadioCapability();
        out.session = in.session;
        out.phase = in.phase;
        out.raf = in.raf;
        out.logicalModemUuid = in.logicalModemUuid;
        out.status = in.status;
        return out;
    }

    static public android.hardware.radio.CarrierRestrictionsWithPriority h2aTranslate(
            android.hardware.radio.V1_4.CarrierRestrictionsWithPriority in) {
        android.hardware.radio.CarrierRestrictionsWithPriority out =
                new android.hardware.radio.CarrierRestrictionsWithPriority();
        if (in.allowedCarriers != null) {
            out.allowedCarriers = new android.hardware.radio.Carrier[in.allowedCarriers.size()];
            for (int i = 0; i < in.allowedCarriers.size(); i++) {
                out.allowedCarriers[i] = h2aTranslate(in.allowedCarriers.get(i));
            }
        }
        if (in.excludedCarriers != null) {
            out.excludedCarriers = new android.hardware.radio.Carrier[in.excludedCarriers.size()];
            for (int i = 0; i < in.excludedCarriers.size(); i++) {
                out.excludedCarriers[i] = h2aTranslate(in.excludedCarriers.get(i));
            }
        }
        out.allowedCarriersPrioritized = in.allowedCarriersPrioritized;
        return out;
    }

    static public android.hardware.radio.RadioAccessSpecifier h2aTranslate(
            android.hardware.radio.V1_5.RadioAccessSpecifier in) {
        android.hardware.radio.RadioAccessSpecifier out =
                new android.hardware.radio.RadioAccessSpecifier();
        out.radioAccessNetwork = in.radioAccessNetwork;
        out.bands = h2aTranslate(in.bands);
        if (in.channels != null) {
            out.channels = new int[in.channels.size()];
            for (int i = 0; i < in.channels.size(); i++) {
                out.channels[i] = in.channels.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.RadioAccessSpecifierBands h2aTranslate(
            android.hardware.radio.V1_5.RadioAccessSpecifier.Bands in) {
        android.hardware.radio.RadioAccessSpecifierBands out =
                new android.hardware.radio.RadioAccessSpecifierBands();
        List<Integer> bands;
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_5.RadioAccessSpecifier.Bands.hidl_discriminator
                    .geranBands:
                bands = in.geranBands();
                if (bands != null) {
                    int[] geranBands = new int[bands.size()];
                    for (int i = 0; i < bands.size(); i++) {
                        geranBands[i] = bands.get(i);
                    }
                    out.geranBands(geranBands);
                }
                break;
            case android.hardware.radio.V1_5.RadioAccessSpecifier.Bands.hidl_discriminator
                    .utranBands:
                bands = in.utranBands();
                if (bands != null) {
                    int[] utranBands = new int[bands.size()];
                    for (int i = 0; i < bands.size(); i++) {
                        utranBands[i] = bands.get(i);
                    }
                    out.utranBands(utranBands);
                }
                break;
            case android.hardware.radio.V1_5.RadioAccessSpecifier.Bands.hidl_discriminator
                    .eutranBands:
                bands = in.eutranBands();
                if (bands != null) {
                    int[] eutranBands = new int[bands.size()];
                    for (int i = 0; i < bands.size(); i++) {
                        eutranBands[i] = bands.get(i);
                    }
                    out.eutranBands(eutranBands);
                }
                break;
            case android.hardware.radio.V1_5.RadioAccessSpecifier.Bands.hidl_discriminator
                    .ngranBands:
                bands = in.ngranBands();
                if (bands != null) {
                    int[] ngranBands = new int[bands.size()];
                    for (int i = 0; i < bands.size(); i++) {
                        ngranBands[i] = bands.get(i);
                    }
                    out.ngranBands(ngranBands);
                }
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.SignalThresholdInfo h2aTranslate(
            android.hardware.radio.V1_5.SignalThresholdInfo in) {
        android.hardware.radio.SignalThresholdInfo out =
                new android.hardware.radio.SignalThresholdInfo();
        out.signalMeasurement = in.signalMeasurement;
        out.hysteresisMs = in.hysteresisMs;
        out.hysteresisDb = in.hysteresisDb;
        if (in.thresholds != null) {
            out.thresholds = new int[in.thresholds.size()];
            for (int i = 0; i < in.thresholds.size(); i++) {
                out.thresholds[i] = in.thresholds.get(i);
            }
        }
        out.isEnabled = in.isEnabled;
        return out;
    }

    static public android.hardware.radio.NetworkScanRequest h2aTranslate(
            android.hardware.radio.V1_5.NetworkScanRequest in) {
        android.hardware.radio.NetworkScanRequest out =
                new android.hardware.radio.NetworkScanRequest();
        out.type = in.type;
        out.interval = in.interval;
        if (in.specifiers != null) {
            out.specifiers = new android.hardware.radio.RadioAccessSpecifier[in.specifiers.size()];
            for (int i = 0; i < in.specifiers.size(); i++) {
                out.specifiers[i] = h2aTranslate(in.specifiers.get(i));
            }
        }
        out.maxSearchTime = in.maxSearchTime;
        out.incrementalResults = in.incrementalResults;
        out.incrementalResultsPeriodicity = in.incrementalResultsPeriodicity;
        if (in.mccMncs != null) {
            out.mccMncs = new String[in.mccMncs.size()];
            for (int i = 0; i < in.mccMncs.size(); i++) {
                out.mccMncs[i] = in.mccMncs.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.DataProfileInfo h2aTranslate(
            android.hardware.radio.V1_5.DataProfileInfo in) {
        android.hardware.radio.DataProfileInfo out = new android.hardware.radio.DataProfileInfo();
        out.profileId = in.profileId;
        out.apn = in.apn;
        out.protocol = in.protocol;
        out.roamingProtocol = in.roamingProtocol;
        out.authType = in.authType;
        out.user = in.user;
        out.password = in.password;
        out.type = in.type;
        out.maxConnsTime = in.maxConnsTime;
        out.maxConns = in.maxConns;
        out.waitTime = in.waitTime;
        out.enabled = in.enabled;
        out.supportedApnTypesBitmap = in.supportedApnTypesBitmap;
        out.bearerBitmap = in.bearerBitmap;
        out.mtuV4 = in.mtuV4;
        out.mtuV6 = in.mtuV6;
        out.preferred = in.preferred;
        out.persistent = in.persistent;
        return out;
    }

    static public android.hardware.radio.LinkAddress h2aTranslate(
            android.hardware.radio.V1_5.LinkAddress in) {
        android.hardware.radio.LinkAddress out = new android.hardware.radio.LinkAddress();
        out.address = in.address;
        out.properties = in.properties;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.deprecationTime > 9223372036854775807L || in.deprecationTime < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.deprecationTime");
        }
        out.deprecationTime = in.deprecationTime;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.expirationTime > 9223372036854775807L || in.expirationTime < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.expirationTime");
        }
        out.expirationTime = in.expirationTime;
        return out;
    }

    static public android.hardware.radio.ClosedSubscriberGroupInfo h2aTranslate(
            android.hardware.radio.V1_5.ClosedSubscriberGroupInfo in) {
        android.hardware.radio.ClosedSubscriberGroupInfo out =
                new android.hardware.radio.ClosedSubscriberGroupInfo();
        out.csgIndication = in.csgIndication;
        out.homeNodebName = in.homeNodebName;
        out.csgIdentity = in.csgIdentity;
        return out;
    }

    static public android.hardware.radio.OptionalCsgInfo h2aTranslate(
            android.hardware.radio.V1_5.OptionalCsgInfo in) {
        android.hardware.radio.OptionalCsgInfo out = new android.hardware.radio.OptionalCsgInfo();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_5.OptionalCsgInfo.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_5.OptionalCsgInfo.hidl_discriminator.csgInfo:
                out.setCsgInfo(h2aTranslate(in.csgInfo()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.CellIdentityGsm h2aTranslate(
            android.hardware.radio.V1_5.CellIdentityGsm in) {
        android.hardware.radio.CellIdentityGsm out = new android.hardware.radio.CellIdentityGsm();
        out.mcc = in.base.base.mcc;
        out.mnc = in.base.base.mnc;
        out.lac = in.base.base.lac;
        out.cid = in.base.base.cid;
        out.arfcn = in.base.base.arfcn;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.base.bsic > 127 || in.base.base.bsic < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.base.bsic");
        }
        out.bsic = in.base.base.bsic;
        out.operatorNames = h2aTranslate(in.base.operatorNames);
        if (in.additionalPlmns != null) {
            out.additionalPlmns = new String[in.additionalPlmns.size()];
            for (int i = 0; i < in.additionalPlmns.size(); i++) {
                out.additionalPlmns[i] = in.additionalPlmns.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.CellIdentityWcdma h2aTranslate(
            android.hardware.radio.V1_5.CellIdentityWcdma in) {
        android.hardware.radio.CellIdentityWcdma out =
                new android.hardware.radio.CellIdentityWcdma();
        out.mcc = in.base.base.mcc;
        out.mnc = in.base.base.mnc;
        out.lac = in.base.base.lac;
        out.cid = in.base.base.cid;
        out.psc = in.base.base.psc;
        out.uarfcn = in.base.base.uarfcn;
        out.operatorNames = h2aTranslate(in.base.operatorNames);
        if (in.additionalPlmns != null) {
            out.additionalPlmns = new String[in.additionalPlmns.size()];
            for (int i = 0; i < in.additionalPlmns.size(); i++) {
                out.additionalPlmns[i] = in.additionalPlmns.get(i);
            }
        }
        out.optionalCsgInfo = h2aTranslate(in.optionalCsgInfo);
        return out;
    }

    static public android.hardware.radio.CellIdentityTdscdma h2aTranslate(
            android.hardware.radio.V1_5.CellIdentityTdscdma in) {
        android.hardware.radio.CellIdentityTdscdma out =
                new android.hardware.radio.CellIdentityTdscdma();
        out.mcc = in.base.base.mcc;
        out.mnc = in.base.base.mnc;
        out.lac = in.base.base.lac;
        out.cid = in.base.base.cid;
        out.cpid = in.base.base.cpid;
        out.uarfcn = in.base.uarfcn;
        out.operatorNames = h2aTranslate(in.base.operatorNames);
        if (in.additionalPlmns != null) {
            out.additionalPlmns = new String[in.additionalPlmns.size()];
            for (int i = 0; i < in.additionalPlmns.size(); i++) {
                out.additionalPlmns[i] = in.additionalPlmns.get(i);
            }
        }
        out.optionalCsgInfo = h2aTranslate(in.optionalCsgInfo);
        return out;
    }

    static public android.hardware.radio.CellIdentityLte h2aTranslate(
            android.hardware.radio.V1_5.CellIdentityLte in) {
        android.hardware.radio.CellIdentityLte out = new android.hardware.radio.CellIdentityLte();
        out.mcc = in.base.base.mcc;
        out.mnc = in.base.base.mnc;
        out.ci = in.base.base.ci;
        out.pci = in.base.base.pci;
        out.tac = in.base.base.tac;
        out.earfcn = in.base.base.earfcn;
        out.operatorNames = h2aTranslate(in.base.operatorNames);
        out.bandwidth = in.base.bandwidth;
        if (in.additionalPlmns != null) {
            out.additionalPlmns = new String[in.additionalPlmns.size()];
            for (int i = 0; i < in.additionalPlmns.size(); i++) {
                out.additionalPlmns[i] = in.additionalPlmns.get(i);
            }
        }
        out.optionalCsgInfo = h2aTranslate(in.optionalCsgInfo);
        if (in.bands != null) {
            out.bands = new int[in.bands.size()];
            for (int i = 0; i < in.bands.size(); i++) {
                out.bands[i] = in.bands.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.CellIdentityNr h2aTranslate(
            android.hardware.radio.V1_5.CellIdentityNr in) {
        android.hardware.radio.CellIdentityNr out = new android.hardware.radio.CellIdentityNr();
        out.mcc = in.base.mcc;
        out.mnc = in.base.mnc;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.nci > 9223372036854775807L || in.base.nci < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.nci");
        }
        out.nci = in.base.nci;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.pci > 2147483647 || in.base.pci < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.pci");
        }
        out.pci = in.base.pci;
        out.tac = in.base.tac;
        out.nrarfcn = in.base.nrarfcn;
        out.operatorNames = h2aTranslate(in.base.operatorNames);
        if (in.additionalPlmns != null) {
            out.additionalPlmns = new String[in.additionalPlmns.size()];
            for (int i = 0; i < in.additionalPlmns.size(); i++) {
                out.additionalPlmns[i] = in.additionalPlmns.get(i);
            }
        }
        if (in.bands != null) {
            out.bands = new int[in.bands.size()];
            for (int i = 0; i < in.bands.size(); i++) {
                out.bands[i] = in.bands.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.CellInfoGsm h2aTranslate(
            android.hardware.radio.V1_5.CellInfoGsm in) {
        android.hardware.radio.CellInfoGsm out = new android.hardware.radio.CellInfoGsm();
        out.cellIdentityGsm = h2aTranslate(in.cellIdentityGsm);
        out.signalStrengthGsm = h2aTranslate(in.signalStrengthGsm);
        return out;
    }

    static public android.hardware.radio.CellInfoWcdma h2aTranslate(
            android.hardware.radio.V1_5.CellInfoWcdma in) {
        android.hardware.radio.CellInfoWcdma out = new android.hardware.radio.CellInfoWcdma();
        out.cellIdentityWcdma = h2aTranslate(in.cellIdentityWcdma);
        out.signalStrengthWcdma = h2aTranslate(in.signalStrengthWcdma);
        return out;
    }

    static public android.hardware.radio.CellInfoTdscdma h2aTranslate(
            android.hardware.radio.V1_5.CellInfoTdscdma in) {
        android.hardware.radio.CellInfoTdscdma out = new android.hardware.radio.CellInfoTdscdma();
        out.cellIdentityTdscdma = h2aTranslate(in.cellIdentityTdscdma);
        out.signalStrengthTdscdma = h2aTranslate(in.signalStrengthTdscdma);
        return out;
    }

    static public android.hardware.radio.CellIdentity h2aTranslate(
            android.hardware.radio.V1_5.CellIdentity in) {
        android.hardware.radio.CellIdentity out = new android.hardware.radio.CellIdentity();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_5.CellIdentity.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_5.CellIdentity.hidl_discriminator.gsm:
                out.setGsm(h2aTranslate(in.gsm()));
                break;
            case android.hardware.radio.V1_5.CellIdentity.hidl_discriminator.wcdma:
                out.setWcdma(h2aTranslate(in.wcdma()));
                break;
            case android.hardware.radio.V1_5.CellIdentity.hidl_discriminator.tdscdma:
                out.setTdscdma(h2aTranslate(in.tdscdma()));
                break;
            case android.hardware.radio.V1_5.CellIdentity.hidl_discriminator.cdma:
                out.setCdma(h2aTranslate(in.cdma()));
                break;
            case android.hardware.radio.V1_5.CellIdentity.hidl_discriminator.lte:
                out.setLte(h2aTranslate(in.lte()));
                break;
            case android.hardware.radio.V1_5.CellIdentity.hidl_discriminator.nr:
                out.setNr(h2aTranslate(in.nr()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.BarringInfo h2aTranslate(
            android.hardware.radio.V1_5.BarringInfo in) {
        android.hardware.radio.BarringInfo out = new android.hardware.radio.BarringInfo();
        out.serviceType = in.serviceType;
        out.barringType = in.barringType;
        out.barringTypeSpecificInfo = h2aTranslate(in.barringTypeSpecificInfo);
        return out;
    }

    static public android.hardware.radio.BarringInfoBarringTypeSpecificInfoConditional h2aTranslate(
            android.hardware.radio.V1_5.BarringInfo.BarringTypeSpecificInfo.Conditional in) {
        android.hardware.radio.BarringInfoBarringTypeSpecificInfoConditional out =
                new android.hardware.radio.BarringInfoBarringTypeSpecificInfoConditional();
        out.factor = in.factor;
        out.timeSeconds = in.timeSeconds;
        out.isBarred = in.isBarred;
        return out;
    }

    static public android.hardware.radio.BarringInfoBarringTypeSpecificInfo h2aTranslate(
            android.hardware.radio.V1_5.BarringInfo.BarringTypeSpecificInfo in) {
        android.hardware.radio.BarringInfoBarringTypeSpecificInfo out =
                new android.hardware.radio.BarringInfoBarringTypeSpecificInfo();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_5.BarringInfo.BarringTypeSpecificInfo.hidl_discriminator
                    .noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_5.BarringInfo.BarringTypeSpecificInfo.hidl_discriminator
                    .conditional:
                out.setConditional(h2aTranslate(in.conditional()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio
            .RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo
            h2aTranslate(android.hardware.radio.V1_5.RegStateResult.AccessTechnologySpecificInfo
                                 .Cdma2000RegistrationInfo in) {
        android.hardware.radio
                .RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo out =
                new android.hardware.radio
                        .RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo();
        out.cssSupported = in.cssSupported;
        out.roamingIndicator = in.roamingIndicator;
        out.systemIsInPrl = in.systemIsInPrl;
        out.defaultRoamingIndicator = in.defaultRoamingIndicator;
        return out;
    }

    static public android.hardware.radio
            .RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo
            h2aTranslate(android.hardware.radio.V1_5.RegStateResult.AccessTechnologySpecificInfo
                                 .EutranRegistrationInfo in) {
        android.hardware.radio
                .RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo out =
                new android.hardware.radio
                        .RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo();
        out.lteVopsInfo = h2aTranslate(in.lteVopsInfo);
        out.nrIndicators = h2aTranslate(in.nrIndicators);
        return out;
    }

    static public android.hardware.radio.AppStatus h2aTranslate(
            android.hardware.radio.V1_5.AppStatus in) {
        android.hardware.radio.AppStatus out = new android.hardware.radio.AppStatus();
        out.appType = in.base.appType;
        out.appState = in.base.appState;
        out.persoSubstate = in.persoSubstate;
        out.aidPtr = in.base.aidPtr;
        out.appLabelPtr = in.base.appLabelPtr;
        out.pin1Replaced = in.base.pin1Replaced;
        out.pin1 = in.base.pin1;
        out.pin2 = in.base.pin2;
        return out;
    }

    static public android.hardware.radio.CardStatus h2aTranslate(
            android.hardware.radio.V1_5.CardStatus in) {
        android.hardware.radio.CardStatus out = new android.hardware.radio.CardStatus();
        out.cardState = in.base.base.base.cardState;
        out.universalPinState = in.base.base.base.universalPinState;
        out.gsmUmtsSubscriptionAppIndex = in.base.base.base.gsmUmtsSubscriptionAppIndex;
        out.cdmaSubscriptionAppIndex = in.base.base.base.cdmaSubscriptionAppIndex;
        out.imsSubscriptionAppIndex = in.base.base.base.imsSubscriptionAppIndex;
        if (in.applications != null) {
            out.applications = new android.hardware.radio.AppStatus[in.applications.size()];
            for (int i = 0; i < in.applications.size(); i++) {
                out.applications[i] = h2aTranslate(in.applications.get(i));
            }
        }
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.base.physicalSlotId > 2147483647 || in.base.base.physicalSlotId < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.base.physicalSlotId");
        }
        out.physicalSlotId = in.base.base.physicalSlotId;
        out.atr = in.base.base.atr;
        out.iccid = in.base.base.iccid;
        out.eid = in.base.eid;
        return out;
    }

    static public android.hardware.radio.QosBandwidth h2aTranslate(
            android.hardware.radio.V1_6.QosBandwidth in) {
        android.hardware.radio.QosBandwidth out = new android.hardware.radio.QosBandwidth();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.maxBitrateKbps > 2147483647 || in.maxBitrateKbps < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.maxBitrateKbps");
        }
        out.maxBitrateKbps = in.maxBitrateKbps;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.guaranteedBitrateKbps > 2147483647 || in.guaranteedBitrateKbps < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.guaranteedBitrateKbps");
        }
        out.guaranteedBitrateKbps = in.guaranteedBitrateKbps;
        return out;
    }

    static public android.hardware.radio.EpsQos h2aTranslate(
            android.hardware.radio.V1_6.EpsQos in) {
        android.hardware.radio.EpsQos out = new android.hardware.radio.EpsQos();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.qci < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.qci");
        }
        out.qci = (char) in.qci;
        out.downlink = h2aTranslate(in.downlink);
        out.uplink = h2aTranslate(in.uplink);
        return out;
    }

    static public android.hardware.radio.NrQos h2aTranslate(android.hardware.radio.V1_6.NrQos in) {
        android.hardware.radio.NrQos out = new android.hardware.radio.NrQos();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.fiveQi < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.fiveQi");
        }
        out.fiveQi = (char) in.fiveQi;
        out.downlink = h2aTranslate(in.downlink);
        out.uplink = h2aTranslate(in.uplink);
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.qfi > 127 || in.qfi < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.qfi");
        }
        out.qfi = in.qfi;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.averagingWindowMs < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.averagingWindowMs");
        }
        out.averagingWindowMs = (char) in.averagingWindowMs;
        return out;
    }

    static public android.hardware.radio.Qos h2aTranslate(android.hardware.radio.V1_6.Qos in) {
        android.hardware.radio.Qos out = new android.hardware.radio.Qos();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.Qos.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.Qos.hidl_discriminator.eps:
                out.setEps(h2aTranslate(in.eps()));
                break;
            case android.hardware.radio.V1_6.Qos.hidl_discriminator.nr:
                out.setNr(h2aTranslate(in.nr()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.RadioResponseInfo h2aTranslate(
            android.hardware.radio.V1_6.RadioResponseInfo in) {
        android.hardware.radio.RadioResponseInfo out =
                new android.hardware.radio.RadioResponseInfo();
        out.type = in.type;
        out.serial = in.serial;
        out.error = in.error;
        return out;
    }

    static public android.hardware.radio.PortRange h2aTranslate(
            android.hardware.radio.V1_6.PortRange in) {
        android.hardware.radio.PortRange out = new android.hardware.radio.PortRange();
        out.start = in.start;
        out.end = in.end;
        return out;
    }

    static public android.hardware.radio.MaybePort h2aTranslate(
            android.hardware.radio.V1_6.MaybePort in) {
        android.hardware.radio.MaybePort out = new android.hardware.radio.MaybePort();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.MaybePort.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.MaybePort.hidl_discriminator.range:
                out.setRange(h2aTranslate(in.range()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.QosFilter h2aTranslate(
            android.hardware.radio.V1_6.QosFilter in) {
        android.hardware.radio.QosFilter out = new android.hardware.radio.QosFilter();
        if (in.localAddresses != null) {
            out.localAddresses = new String[in.localAddresses.size()];
            for (int i = 0; i < in.localAddresses.size(); i++) {
                out.localAddresses[i] = in.localAddresses.get(i);
            }
        }
        if (in.remoteAddresses != null) {
            out.remoteAddresses = new String[in.remoteAddresses.size()];
            for (int i = 0; i < in.remoteAddresses.size(); i++) {
                out.remoteAddresses[i] = in.remoteAddresses.get(i);
            }
        }
        out.localPort = h2aTranslate(in.localPort);
        out.remotePort = h2aTranslate(in.remotePort);
        out.protocol = in.protocol;
        out.tos = h2aTranslate(in.tos);
        out.flowLabel = h2aTranslate(in.flowLabel);
        out.spi = h2aTranslate(in.spi);
        out.direction = in.direction;
        out.precedence = in.precedence;
        return out;
    }

    static public android.hardware.radio.QosFilterTypeOfService h2aTranslate(
            android.hardware.radio.V1_6.QosFilter.TypeOfService in) {
        android.hardware.radio.QosFilterTypeOfService out =
                new android.hardware.radio.QosFilterTypeOfService();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.QosFilter.TypeOfService.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.QosFilter.TypeOfService.hidl_discriminator.value:
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.value() > 127 || in.value() < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.value()");
                }
                out.setValue(in.value());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.QosFilterIpv6FlowLabel h2aTranslate(
            android.hardware.radio.V1_6.QosFilter.Ipv6FlowLabel in) {
        android.hardware.radio.QosFilterIpv6FlowLabel out =
                new android.hardware.radio.QosFilterIpv6FlowLabel();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.QosFilter.Ipv6FlowLabel.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.QosFilter.Ipv6FlowLabel.hidl_discriminator.value:
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.value() > 2147483647 || in.value() < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.value()");
                }
                out.setValue(in.value());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.QosFilterIpsecSpi h2aTranslate(
            android.hardware.radio.V1_6.QosFilter.IpsecSpi in) {
        android.hardware.radio.QosFilterIpsecSpi out =
                new android.hardware.radio.QosFilterIpsecSpi();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.QosFilter.IpsecSpi.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.QosFilter.IpsecSpi.hidl_discriminator.value:
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.value() > 2147483647 || in.value() < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.value()");
                }
                out.setValue(in.value());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.QosSession h2aTranslate(
            android.hardware.radio.V1_6.QosSession in) {
        android.hardware.radio.QosSession out = new android.hardware.radio.QosSession();
        out.qosSessionId = in.qosSessionId;
        out.qos = h2aTranslate(in.qos);
        if (in.qosFilters != null) {
            out.qosFilters = new android.hardware.radio.QosFilter[in.qosFilters.size()];
            for (int i = 0; i < in.qosFilters.size(); i++) {
                out.qosFilters[i] = h2aTranslate(in.qosFilters.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.SetupDataCallResult h2aTranslate(
            android.hardware.radio.V1_6.SetupDataCallResult in) {
        android.hardware.radio.SetupDataCallResult out =
                new android.hardware.radio.SetupDataCallResult();
        out.cause = in.cause;
        out.suggestedRetryTime = in.suggestedRetryTime;
        out.cid = in.cid;
        out.active = in.active;
        out.type = in.type;
        out.ifname = in.ifname;
        if (in.addresses != null) {
            out.addresses = new android.hardware.radio.LinkAddress[in.addresses.size()];
            for (int i = 0; i < in.addresses.size(); i++) {
                out.addresses[i] = h2aTranslate(in.addresses.get(i));
            }
        }
        if (in.dnses != null) {
            out.dnses = new String[in.dnses.size()];
            for (int i = 0; i < in.dnses.size(); i++) {
                out.dnses[i] = in.dnses.get(i);
            }
        }
        if (in.gateways != null) {
            out.gateways = new String[in.gateways.size()];
            for (int i = 0; i < in.gateways.size(); i++) {
                out.gateways[i] = in.gateways.get(i);
            }
        }
        if (in.pcscf != null) {
            out.pcscf = new String[in.pcscf.size()];
            for (int i = 0; i < in.pcscf.size(); i++) {
                out.pcscf[i] = in.pcscf.get(i);
            }
        }
        out.mtuV4 = in.mtuV4;
        out.mtuV6 = in.mtuV6;
        out.defaultQos = h2aTranslate(in.defaultQos);
        if (in.qosSessions != null) {
            out.qosSessions = new android.hardware.radio.QosSession[in.qosSessions.size()];
            for (int i = 0; i < in.qosSessions.size(); i++) {
                out.qosSessions[i] = h2aTranslate(in.qosSessions.get(i));
            }
        }
        out.handoverFailureMode = in.handoverFailureMode;
        out.pduSessionId = in.pduSessionId;
        out.sliceInfo = h2aTranslate(in.sliceInfo);
        if (in.trafficDescriptors != null) {
            out.trafficDescriptors =
                    new android.hardware.radio.TrafficDescriptor[in.trafficDescriptors.size()];
            for (int i = 0; i < in.trafficDescriptors.size(); i++) {
                out.trafficDescriptors[i] = h2aTranslate(in.trafficDescriptors.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.LinkCapacityEstimate h2aTranslate(
            android.hardware.radio.V1_6.LinkCapacityEstimate in) {
        android.hardware.radio.LinkCapacityEstimate out =
                new android.hardware.radio.LinkCapacityEstimate();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.downlinkCapacityKbps > 2147483647 || in.downlinkCapacityKbps < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.downlinkCapacityKbps");
        }
        out.downlinkCapacityKbps = in.downlinkCapacityKbps;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.uplinkCapacityKbps > 2147483647 || in.uplinkCapacityKbps < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.uplinkCapacityKbps");
        }
        out.uplinkCapacityKbps = in.uplinkCapacityKbps;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.secondaryDownlinkCapacityKbps > 2147483647 || in.secondaryDownlinkCapacityKbps < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.secondaryDownlinkCapacityKbps");
        }
        out.secondaryDownlinkCapacityKbps = in.secondaryDownlinkCapacityKbps;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.secondaryUplinkCapacityKbps > 2147483647 || in.secondaryUplinkCapacityKbps < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.secondaryUplinkCapacityKbps");
        }
        out.secondaryUplinkCapacityKbps = in.secondaryUplinkCapacityKbps;
        return out;
    }

    static public android.hardware.radio.NrVopsInfo h2aTranslate(
            android.hardware.radio.V1_6.NrVopsInfo in) {
        android.hardware.radio.NrVopsInfo out = new android.hardware.radio.NrVopsInfo();
        out.vopsSupported = in.vopsSupported;
        out.emcSupported = in.emcSupported;
        out.emfSupported = in.emfSupported;
        return out;
    }

    static public android.hardware.radio.LteSignalStrength h2aTranslate(
            android.hardware.radio.V1_6.LteSignalStrength in) {
        android.hardware.radio.LteSignalStrength out =
                new android.hardware.radio.LteSignalStrength();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.signalStrength > 2147483647 || in.base.signalStrength < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.signalStrength");
        }
        out.signalStrength = in.base.signalStrength;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.rsrp > 2147483647 || in.base.rsrp < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.rsrp");
        }
        out.rsrp = in.base.rsrp;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.rsrq > 2147483647 || in.base.rsrq < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.rsrq");
        }
        out.rsrq = in.base.rsrq;
        out.rssnr = in.base.rssnr;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.cqi > 2147483647 || in.base.cqi < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.cqi");
        }
        out.cqi = in.base.cqi;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.timingAdvance > 2147483647 || in.base.timingAdvance < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.timingAdvance");
        }
        out.timingAdvance = in.base.timingAdvance;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.cqiTableIndex > 2147483647 || in.cqiTableIndex < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.cqiTableIndex");
        }
        out.cqiTableIndex = in.cqiTableIndex;
        return out;
    }

    static public android.hardware.radio.NrSignalStrength h2aTranslate(
            android.hardware.radio.V1_6.NrSignalStrength in) {
        android.hardware.radio.NrSignalStrength out = new android.hardware.radio.NrSignalStrength();
        out.ssRsrp = in.base.ssRsrp;
        out.ssRsrq = in.base.ssRsrq;
        out.ssSinr = in.base.ssSinr;
        out.csiRsrp = in.base.csiRsrp;
        out.csiRsrq = in.base.csiRsrq;
        out.csiSinr = in.base.csiSinr;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.csiCqiTableIndex > 2147483647 || in.csiCqiTableIndex < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.csiCqiTableIndex");
        }
        out.csiCqiTableIndex = in.csiCqiTableIndex;
        if (in.csiCqiReport != null) {
            out.csiCqiReport = new byte[in.csiCqiReport.size()];
            for (int i = 0; i < in.csiCqiReport.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.csiCqiReport.get(i) > 127 || in.csiCqiReport.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.csiCqiReport.get(i)");
                }
                out.csiCqiReport[i] = in.csiCqiReport.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.SignalStrength h2aTranslate(
            android.hardware.radio.V1_6.SignalStrength in) {
        android.hardware.radio.SignalStrength out = new android.hardware.radio.SignalStrength();
        out.gsm = h2aTranslate(in.gsm);
        out.cdma = h2aTranslate(in.cdma);
        out.evdo = h2aTranslate(in.evdo);
        out.lte = h2aTranslate(in.lte);
        out.tdscdma = h2aTranslate(in.tdscdma);
        out.wcdma = h2aTranslate(in.wcdma);
        out.nr = h2aTranslate(in.nr);
        return out;
    }

    static public android.hardware.radio.CellInfoLte h2aTranslate(
            android.hardware.radio.V1_6.CellInfoLte in) {
        android.hardware.radio.CellInfoLte out = new android.hardware.radio.CellInfoLte();
        out.cellIdentityLte = h2aTranslate(in.cellIdentityLte);
        out.signalStrengthLte = h2aTranslate(in.signalStrengthLte);
        return out;
    }

    static public android.hardware.radio.CellInfoNr h2aTranslate(
            android.hardware.radio.V1_6.CellInfoNr in) {
        android.hardware.radio.CellInfoNr out = new android.hardware.radio.CellInfoNr();
        out.cellIdentityNr = h2aTranslate(in.cellIdentityNr);
        out.signalStrengthNr = h2aTranslate(in.signalStrengthNr);
        return out;
    }

    static public android.hardware.radio.CellInfo h2aTranslate(
            android.hardware.radio.V1_6.CellInfo in) {
        android.hardware.radio.CellInfo out = new android.hardware.radio.CellInfo();
        out.registered = in.registered;
        out.connectionStatus = in.connectionStatus;
        out.ratSpecificInfo = h2aTranslate(in.ratSpecificInfo);
        return out;
    }

    static public android.hardware.radio.CellInfoCellInfoRatSpecificInfo h2aTranslate(
            android.hardware.radio.V1_6.CellInfo.CellInfoRatSpecificInfo in) {
        android.hardware.radio.CellInfoCellInfoRatSpecificInfo out =
                new android.hardware.radio.CellInfoCellInfoRatSpecificInfo();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.CellInfo.CellInfoRatSpecificInfo.hidl_discriminator
                    .gsm:
                out.setGsm(h2aTranslate(in.gsm()));
                break;
            case android.hardware.radio.V1_6.CellInfo.CellInfoRatSpecificInfo.hidl_discriminator
                    .wcdma:
                out.setWcdma(h2aTranslate(in.wcdma()));
                break;
            case android.hardware.radio.V1_6.CellInfo.CellInfoRatSpecificInfo.hidl_discriminator
                    .tdscdma:
                out.setTdscdma(h2aTranslate(in.tdscdma()));
                break;
            case android.hardware.radio.V1_6.CellInfo.CellInfoRatSpecificInfo.hidl_discriminator
                    .lte:
                out.setLte(h2aTranslate(in.lte()));
                break;
            case android.hardware.radio.V1_6.CellInfo.CellInfoRatSpecificInfo.hidl_discriminator.nr:
                out.setNr(h2aTranslate(in.nr()));
                break;
            case android.hardware.radio.V1_6.CellInfo.CellInfoRatSpecificInfo.hidl_discriminator
                    .cdma:
                out.setCdma(h2aTranslate(in.cdma()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.NetworkScanResult h2aTranslate(
            android.hardware.radio.V1_6.NetworkScanResult in) {
        android.hardware.radio.NetworkScanResult out =
                new android.hardware.radio.NetworkScanResult();
        out.status = in.status;
        out.error = in.error;
        if (in.networkInfos != null) {
            out.networkInfos = new android.hardware.radio.CellInfo[in.networkInfos.size()];
            for (int i = 0; i < in.networkInfos.size(); i++) {
                out.networkInfos[i] = h2aTranslate(in.networkInfos.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.RegStateResult h2aTranslate(
            android.hardware.radio.V1_6.RegStateResult in) {
        android.hardware.radio.RegStateResult out = new android.hardware.radio.RegStateResult();
        out.regState = in.regState;
        out.rat = in.rat;
        out.reasonForDenial = in.reasonForDenial;
        out.cellIdentity = h2aTranslate(in.cellIdentity);
        out.registeredPlmn = in.registeredPlmn;
        out.accessTechnologySpecificInfo = h2aTranslate(in.accessTechnologySpecificInfo);
        return out;
    }

    static public android.hardware.radio.RegStateResultAccessTechnologySpecificInfo h2aTranslate(
            android.hardware.radio.V1_6.RegStateResult.AccessTechnologySpecificInfo in) {
        android.hardware.radio.RegStateResultAccessTechnologySpecificInfo out =
                new android.hardware.radio.RegStateResultAccessTechnologySpecificInfo();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.RegStateResult.AccessTechnologySpecificInfo
                    .hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.RegStateResult.AccessTechnologySpecificInfo
                    .hidl_discriminator.cdmaInfo:
                out.setCdmaInfo(h2aTranslate(in.cdmaInfo()));
                break;
            case android.hardware.radio.V1_6.RegStateResult.AccessTechnologySpecificInfo
                    .hidl_discriminator.eutranInfo:
                out.setEutranInfo(h2aTranslate(in.eutranInfo()));
                break;
            case android.hardware.radio.V1_6.RegStateResult.AccessTechnologySpecificInfo
                    .hidl_discriminator.ngranNrVopsInfo:
                out.setNgranNrVopsInfo(h2aTranslate(in.ngranNrVopsInfo()));
                break;
            case android.hardware.radio.V1_6.RegStateResult.AccessTechnologySpecificInfo
                    .hidl_discriminator.geranDtmSupported:
                out.setGeranDtmSupported(in.geranDtmSupported());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.Call h2aTranslate(android.hardware.radio.V1_6.Call in) {
        android.hardware.radio.Call out = new android.hardware.radio.Call();
        out.state = in.base.base.state;
        out.index = in.base.base.index;
        out.toa = in.base.base.toa;
        out.isMpty = in.base.base.isMpty;
        out.isMT = in.base.base.isMT;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.base.als > 127 || in.base.base.als < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.base.als");
        }
        out.als = in.base.base.als;
        out.isVoice = in.base.base.isVoice;
        out.isVoicePrivacy = in.base.base.isVoicePrivacy;
        out.number = in.base.base.number;
        out.numberPresentation = in.base.base.numberPresentation;
        out.name = in.base.base.name;
        out.namePresentation = in.base.base.namePresentation;
        if (in.base.base.uusInfo != null) {
            out.uusInfo = new android.hardware.radio.UusInfo[in.base.base.uusInfo.size()];
            for (int i = 0; i < in.base.base.uusInfo.size(); i++) {
                out.uusInfo[i] = h2aTranslate(in.base.base.uusInfo.get(i));
            }
        }
        out.audioQuality = in.base.audioQuality;
        out.forwardedNumber = in.forwardedNumber;
        return out;
    }

    static public android.hardware.radio.PhysicalChannelConfig h2aTranslate(
            android.hardware.radio.V1_6.PhysicalChannelConfig in) {
        android.hardware.radio.PhysicalChannelConfig out =
                new android.hardware.radio.PhysicalChannelConfig();
        out.status = in.status;
        out.rat = in.rat;
        out.downlinkChannelNumber = in.downlinkChannelNumber;
        out.uplinkChannelNumber = in.uplinkChannelNumber;
        out.cellBandwidthDownlinkKhz = in.cellBandwidthDownlinkKhz;
        out.cellBandwidthUplinkKhz = in.cellBandwidthUplinkKhz;
        if (in.contextIds != null) {
            out.contextIds = new int[in.contextIds.size()];
            for (int i = 0; i < in.contextIds.size(); i++) {
                out.contextIds[i] = in.contextIds.get(i);
            }
        }
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.physicalCellId > 2147483647 || in.physicalCellId < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.physicalCellId");
        }
        out.physicalCellId = in.physicalCellId;
        out.band = h2aTranslate(in.band);
        return out;
    }

    static public android.hardware.radio.PhysicalChannelConfigBand h2aTranslate(
            android.hardware.radio.V1_6.PhysicalChannelConfig.Band in) {
        android.hardware.radio.PhysicalChannelConfigBand out =
                new android.hardware.radio.PhysicalChannelConfigBand();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.PhysicalChannelConfig.Band.hidl_discriminator
                    .geranBand:
                out.setGeranBand(in.geranBand());
                break;
            case android.hardware.radio.V1_6.PhysicalChannelConfig.Band.hidl_discriminator
                    .utranBand:
                out.setUtranBand(in.utranBand());
                break;
            case android.hardware.radio.V1_6.PhysicalChannelConfig.Band.hidl_discriminator
                    .eutranBand:
                out.setEutranBand(in.eutranBand());
                break;
            case android.hardware.radio.V1_6.PhysicalChannelConfig.Band.hidl_discriminator
                    .ngranBand:
                out.setNgranBand(in.ngranBand());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.OptionalSliceInfo h2aTranslate(
            android.hardware.radio.V1_6.OptionalSliceInfo in) {
        android.hardware.radio.OptionalSliceInfo out =
                new android.hardware.radio.OptionalSliceInfo();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.OptionalSliceInfo.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.OptionalSliceInfo.hidl_discriminator.value:
                out.setValue(h2aTranslate(in.value()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.SliceInfo h2aTranslate(
            android.hardware.radio.V1_6.SliceInfo in) {
        android.hardware.radio.SliceInfo out = new android.hardware.radio.SliceInfo();
        out.sst = in.sst;
        out.sliceDifferentiator = in.sliceDifferentiator;
        out.mappedHplmnSst = in.mappedHplmnSst;
        out.mappedHplmnSD = in.mappedHplmnSD;
        out.status = in.status;
        return out;
    }

    static public android.hardware.radio.OptionalDnn h2aTranslate(
            android.hardware.radio.V1_6.OptionalDnn in) {
        android.hardware.radio.OptionalDnn out = new android.hardware.radio.OptionalDnn();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.OptionalDnn.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.OptionalDnn.hidl_discriminator.value:
                out.setValue(in.value());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.OptionalOsAppId h2aTranslate(
            android.hardware.radio.V1_6.OptionalOsAppId in) {
        android.hardware.radio.OptionalOsAppId out = new android.hardware.radio.OptionalOsAppId();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.OptionalOsAppId.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.OptionalOsAppId.hidl_discriminator.value:
                out.setValue(h2aTranslate(in.value()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.OptionalTrafficDescriptor h2aTranslate(
            android.hardware.radio.V1_6.OptionalTrafficDescriptor in) {
        android.hardware.radio.OptionalTrafficDescriptor out =
                new android.hardware.radio.OptionalTrafficDescriptor();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.OptionalTrafficDescriptor.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.OptionalTrafficDescriptor.hidl_discriminator.value:
                out.setValue(h2aTranslate(in.value()));
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.TrafficDescriptor h2aTranslate(
            android.hardware.radio.V1_6.TrafficDescriptor in) {
        android.hardware.radio.TrafficDescriptor out =
                new android.hardware.radio.TrafficDescriptor();
        out.dnn = h2aTranslate(in.dnn);
        out.osAppId = h2aTranslate(in.osAppId);
        return out;
    }

    static public android.hardware.radio.OsAppId h2aTranslate(
            android.hardware.radio.V1_6.OsAppId in) {
        android.hardware.radio.OsAppId out = new android.hardware.radio.OsAppId();
        if (in.osAppId != null) {
            out.osAppId = new byte[in.osAppId.size()];
            for (int i = 0; i < in.osAppId.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.osAppId.get(i) > 127 || in.osAppId.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.osAppId.get(i)");
                }
                out.osAppId[i] = in.osAppId.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.SlicingConfig h2aTranslate(
            android.hardware.radio.V1_6.SlicingConfig in) {
        android.hardware.radio.SlicingConfig out = new android.hardware.radio.SlicingConfig();
        if (in.urspRules != null) {
            out.urspRules = new android.hardware.radio.UrspRule[in.urspRules.size()];
            for (int i = 0; i < in.urspRules.size(); i++) {
                out.urspRules[i] = h2aTranslate(in.urspRules.get(i));
            }
        }
        if (in.sliceInfo != null) {
            out.sliceInfo = new android.hardware.radio.SliceInfo[in.sliceInfo.size()];
            for (int i = 0; i < in.sliceInfo.size(); i++) {
                out.sliceInfo[i] = h2aTranslate(in.sliceInfo.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.UrspRule h2aTranslate(
            android.hardware.radio.V1_6.UrspRule in) {
        android.hardware.radio.UrspRule out = new android.hardware.radio.UrspRule();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.precedence > 127 || in.precedence < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.precedence");
        }
        out.precedence = in.precedence;
        if (in.trafficDescriptors != null) {
            out.trafficDescriptors =
                    new android.hardware.radio.TrafficDescriptor[in.trafficDescriptors.size()];
            for (int i = 0; i < in.trafficDescriptors.size(); i++) {
                out.trafficDescriptors[i] = h2aTranslate(in.trafficDescriptors.get(i));
            }
        }
        if (in.routeSelectionDescriptor != null) {
            out.routeSelectionDescriptor =
                    new android.hardware.radio
                            .RouteSelectionDescriptor[in.routeSelectionDescriptor.size()];
            for (int i = 0; i < in.routeSelectionDescriptor.size(); i++) {
                out.routeSelectionDescriptor[i] = h2aTranslate(in.routeSelectionDescriptor.get(i));
            }
        }
        return out;
    }

    static public android.hardware.radio.RouteSelectionDescriptor h2aTranslate(
            android.hardware.radio.V1_6.RouteSelectionDescriptor in) {
        android.hardware.radio.RouteSelectionDescriptor out =
                new android.hardware.radio.RouteSelectionDescriptor();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.precedence > 127 || in.precedence < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.precedence");
        }
        out.precedence = in.precedence;
        out.sessionType = h2aTranslate(in.sessionType);
        out.sscMode = h2aTranslate(in.sscMode);
        if (in.sliceInfo != null) {
            out.sliceInfo = new android.hardware.radio.SliceInfo[in.sliceInfo.size()];
            for (int i = 0; i < in.sliceInfo.size(); i++) {
                out.sliceInfo[i] = h2aTranslate(in.sliceInfo.get(i));
            }
        }
        if (in.dnn != null) {
            out.dnn = new String[in.dnn.size()];
            for (int i = 0; i < in.dnn.size(); i++) {
                out.dnn[i] = in.dnn.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.OptionalPdpProtocolType h2aTranslate(
            android.hardware.radio.V1_6.OptionalPdpProtocolType in) {
        android.hardware.radio.OptionalPdpProtocolType out =
                new android.hardware.radio.OptionalPdpProtocolType();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.OptionalPdpProtocolType.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.OptionalPdpProtocolType.hidl_discriminator.value:
                out.setValue(in.value());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.OptionalSscMode h2aTranslate(
            android.hardware.radio.V1_6.OptionalSscMode in) {
        android.hardware.radio.OptionalSscMode out = new android.hardware.radio.OptionalSscMode();
        switch (in.getDiscriminator()) {
            case android.hardware.radio.V1_6.OptionalSscMode.hidl_discriminator.noinit:
                // Nothing to translate for Monostate.
                break;
            case android.hardware.radio.V1_6.OptionalSscMode.hidl_discriminator.value:
                out.setValue(in.value());
                break;
            default:
                throw new RuntimeException(
                        "Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
        }
        return out;
    }

    static public android.hardware.radio.ImsiEncryptionInfo h2aTranslate(
            android.hardware.radio.V1_6.ImsiEncryptionInfo in) {
        android.hardware.radio.ImsiEncryptionInfo out =
                new android.hardware.radio.ImsiEncryptionInfo();
        out.mcc = in.base.mcc;
        out.mnc = in.base.mnc;
        if (in.base.carrierKey != null) {
            out.carrierKey = new byte[in.base.carrierKey.size()];
            for (int i = 0; i < in.base.carrierKey.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.base.carrierKey.get(i) > 127 || in.base.carrierKey.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.base.carrierKey.get(i)");
                }
                out.carrierKey[i] = in.base.carrierKey.get(i);
            }
        }
        out.keyIdentifier = in.base.keyIdentifier;
        out.expirationTime = in.base.expirationTime;
        out.keyType = in.keyType;
        return out;
    }

    static public android.hardware.radio.PhonebookRecordInfo h2aTranslate(
            android.hardware.radio.V1_6.PhonebookRecordInfo in) {
        android.hardware.radio.PhonebookRecordInfo out =
                new android.hardware.radio.PhonebookRecordInfo();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.recordId > 2147483647 || in.recordId < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.recordId");
        }
        out.recordId = in.recordId;
        out.name = in.name;
        out.number = in.number;
        if (in.emails != null) {
            out.emails = new String[in.emails.size()];
            for (int i = 0; i < in.emails.size(); i++) {
                out.emails[i] = in.emails.get(i);
            }
        }
        if (in.additionalNumbers != null) {
            out.additionalNumbers = new String[in.additionalNumbers.size()];
            for (int i = 0; i < in.additionalNumbers.size(); i++) {
                out.additionalNumbers[i] = in.additionalNumbers.get(i);
            }
        }
        return out;
    }

    static public android.hardware.radio.PhonebookCapacity h2aTranslate(
            android.hardware.radio.V1_6.PhonebookCapacity in) {
        android.hardware.radio.PhonebookCapacity out =
                new android.hardware.radio.PhonebookCapacity();
        out.maxAdnRecords = in.maxAdnRecords;
        out.usedAdnRecords = in.usedAdnRecords;
        out.maxEmailRecords = in.maxEmailRecords;
        out.usedEmailRecords = in.usedEmailRecords;
        out.maxAdditionalNumberRecords = in.maxAdditionalNumberRecords;
        out.usedAdditionalNumberRecords = in.usedAdditionalNumberRecords;
        out.maxNameLen = in.maxNameLen;
        out.maxNumberLen = in.maxNumberLen;
        out.maxEmailLen = in.maxEmailLen;
        out.maxAdditionalNumberLen = in.maxAdditionalNumberLen;
        return out;
    }
}
