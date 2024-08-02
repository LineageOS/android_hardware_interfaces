/*
 * Copyright (C) 2024 The Android Open Source Project
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

#define LOG_TAG "nfc_behavior_changes_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/nfc/BnNfc.h>
#include <aidl/android/hardware/nfc/INfc.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>

#include "NfcAdaptation.h"
#include "SyncEvent.h"
#include "nci_defs.h"
#include "nfa_api.h"
#include "nfa_ee_api.h"

using aidl::android::hardware::nfc::INfc;
using android::getAidlHalInstanceNames;
using android::PrintInstanceNameToString;
using android::base::StringPrintf;

static SyncEvent sNfaEnableEvent;  // event for NFA_Enable()
static SyncEvent sNfaVsCommand;    // event for VS commands
static SyncEvent sNfaEnableDisablePollingEvent;
static SyncEvent sNfaPowerChangeEvent;
static bool sIsNfaEnabled;
static tNFA_STATUS sVSCmdStatus;

static void nfaDeviceManagementCallback(uint8_t dmEvent, tNFA_DM_CBACK_DATA* eventData) {
    LOG(DEBUG) << StringPrintf("%s: enter; event=0x%X", __func__, dmEvent);

    switch (dmEvent) {
        case NFA_DM_ENABLE_EVT: /* Result of NFA_Enable */
        {
            SyncEventGuard guard(sNfaEnableEvent);
            LOG(DEBUG) << StringPrintf("%s: NFA_DM_ENABLE_EVT; status=0x%X", __func__,
                                       eventData->status);
            sIsNfaEnabled = eventData->status == NFA_STATUS_OK;
            sNfaEnableEvent.notifyOne();
        } break;

        case NFA_DM_DISABLE_EVT: /* Result of NFA_Disable */
        {
            SyncEventGuard guard(sNfaEnableEvent);
            LOG(DEBUG) << StringPrintf("%s: NFA_DM_DISABLE_EVT; status=0x%X", __func__,
                                       eventData->status);
            sIsNfaEnabled = eventData->status == NFA_STATUS_OK;
            sNfaEnableEvent.notifyOne();
        } break;

        case NFA_DM_PWR_MODE_CHANGE_EVT: {
            SyncEventGuard guard(sNfaPowerChangeEvent);
            LOG(DEBUG) << StringPrintf(
                    "%s: NFA_DM_PWR_MODE_CHANGE_EVT: status=0x%X, power_mode=0x%X", __func__,
                    eventData->status, eventData->power_mode.power_mode);

            sNfaPowerChangeEvent.notifyOne();

        } break;
    }
}

static void nfaConnectionCallback(uint8_t connEvent, tNFA_CONN_EVT_DATA* eventData) {
    LOG(DEBUG) << StringPrintf("%s: event= %u", __func__, connEvent);

    switch (connEvent) {
        case NFA_LISTEN_DISABLED_EVT: {
            SyncEventGuard guard(sNfaEnableDisablePollingEvent);
            sNfaEnableDisablePollingEvent.notifyOne();
        } break;

        case NFA_LISTEN_ENABLED_EVT: {
            SyncEventGuard guard(sNfaEnableDisablePollingEvent);
            sNfaEnableDisablePollingEvent.notifyOne();
        } break;

        case NFA_RF_DISCOVERY_STARTED_EVT:  // RF Discovery started
        {
            LOG(DEBUG) << StringPrintf("%s: NFA_RF_DISCOVERY_STARTED_EVT: status = %u", __func__,
                                       eventData->status);

            SyncEventGuard guard(sNfaEnableDisablePollingEvent);
            sNfaEnableDisablePollingEvent.notifyOne();
        } break;

        case NFA_RF_DISCOVERY_STOPPED_EVT:  // RF Discovery stopped event
        {
            LOG(DEBUG) << StringPrintf("%s: NFA_RF_DISCOVERY_STOPPED_EVT: status = %u", __func__,
                                       eventData->status);

            SyncEventGuard guard(sNfaEnableDisablePollingEvent);
            sNfaEnableDisablePollingEvent.notifyOne();
        } break;
    }
}

void static nfaVSCallback(uint8_t event, uint16_t /* param_len */, uint8_t* p_param) {
    switch (event & NCI_OID_MASK) {
        case NCI_MSG_PROP_ANDROID: {
            uint8_t android_sub_opcode = p_param[3];
            switch (android_sub_opcode) {
                case NCI_ANDROID_PASSIVE_OBSERVE: {
                    sVSCmdStatus = p_param[4];
                    LOG(INFO) << StringPrintf("Observe mode RSP: status: %x", sVSCmdStatus);
                    SyncEventGuard guard(sNfaVsCommand);
                    sNfaVsCommand.notifyOne();
                } break;
                case NCI_ANDROID_POLLING_FRAME_NTF: {
                    // TODO
                } break;
                default:
                    LOG(WARNING) << StringPrintf("Unknown Android sub opcode %x",
                                                 android_sub_opcode);
            }
        } break;
        default:
            break;
    }
}

/*
 * Enable passive observe mode.
 */
tNFA_STATUS static nfaObserveModeEnable(bool enable) {
    tNFA_STATUS status = NFA_STATUS_FAILED;

    status = NFA_StopRfDiscovery();
    if (status == NFA_STATUS_OK) {
        if (!sNfaEnableDisablePollingEvent.wait(1000)) {
            LOG(WARNING) << "Timeout waiting to disable NFC RF discovery";
            return NFA_STATUS_TIMEOUT;
        }
    }

    uint8_t cmd[] = {(NCI_MT_CMD << NCI_MT_SHIFT) | NCI_GID_PROP, NCI_MSG_PROP_ANDROID,
                     NCI_ANDROID_PASSIVE_OBSERVE_PARAM_SIZE, NCI_ANDROID_PASSIVE_OBSERVE,
                     static_cast<uint8_t>(enable ? NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE
                                                 : NCI_ANDROID_PASSIVE_OBSERVE_PARAM_DISABLE)};

    status = NFA_SendRawVsCommand(sizeof(cmd), cmd, nfaVSCallback);

    if (status == NFA_STATUS_OK) {
        if (!sNfaVsCommand.wait(1000)) {
            LOG(WARNING) << "Timeout waiting for NFA VS command response";
            return NFA_STATUS_TIMEOUT;
        }
    }

    return status;
}

class NfcBehaviorChanges : public testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        tNFA_STATUS status = NFA_STATUS_OK;

        sIsNfaEnabled = false;
        sVSCmdStatus = NFA_STATUS_OK;

        NfcAdaptation& theInstance = NfcAdaptation::GetInstance();
        theInstance.Initialize();  // start GKI, NCI task, NFC task

        {
            SyncEventGuard guard(sNfaEnableEvent);
            tHAL_NFC_ENTRY* halFuncEntries = theInstance.GetHalEntryFuncs();

            NFA_Init(halFuncEntries);

            status = NFA_Enable(nfaDeviceManagementCallback, nfaConnectionCallback);
            ASSERT_EQ(status, NFA_STATUS_OK);

            // wait for NFA command to finish
            ASSERT_TRUE(sNfaEnableEvent.wait(1000))
                    << "Timeout waiting for NFA command on NFA_Enable";
        }

        ASSERT_TRUE(sIsNfaEnabled) << "Could not initialize NFC controller";

        status = NFA_StartRfDiscovery();
        ASSERT_EQ(status, NFA_STATUS_OK);
        ASSERT_TRUE(sNfaEnableDisablePollingEvent.wait(1000)) << "Timeout starting RF discovery";
    }
};

/*
 * ObserveModeEnable:
 * Attempts to enable observe mode. Does not test Observe Mode functionality,
 * but simply verifies that the enable command responds successfully.
 *
 * @VsrTest = GMS-VSR-3.2.8-001
 */
TEST_P(NfcBehaviorChanges, ObserveModeEnableDisable) {
    tNFA_STATUS status = nfaObserveModeEnable(true);
    ASSERT_EQ(status, NFA_STATUS_OK);

    status = nfaObserveModeEnable(false);
    ASSERT_EQ(status, NFA_STATUS_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NfcBehaviorChanges);
INSTANTIATE_TEST_SUITE_P(Nfc, NfcBehaviorChanges,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(INfc::descriptor)),
                         ::android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    std::system("/system/bin/svc nfc disable"); /* Turn off NFC service */
    sleep(5);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    std::system("/system/bin/svc nfc enable"); /* Turn on NFC service */
    sleep(5);
    return status;
}
