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

#include "nfc_api.h"
#define LOG_TAG "nfc_behavior_changes_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/nfc/BnNfc.h>
#include <aidl/android/hardware/nfc/INfc.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
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
static std::vector<uint8_t> sCaps(0);
static uint8_t sObserveModeState;
static bool sIsNfaEnabled;
static tNFA_STATUS sVSCmdStatus;

static const int SET_PASSIVE_OBSERVER_TECH_TIMEOUT_MS = 15;

static int get_vsr_api_level() {
    int api_level =
            ::android::base::GetIntProperty("ro.vendor.api_level", -1);
    if (api_level != -1) {
        return api_level;
    }

    api_level =
            ::android::base::GetIntProperty("ro.board.api_level", -1);
    if (api_level != -1) {
        return api_level;
    }

    api_level =
            ::android::base::GetIntProperty("ro.board.first_api_level", -1);
    EXPECT_NE(api_level, -1) << "Could not find VSR API level.";

    return api_level;

}

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

void static nfaVSCallback(uint8_t event, uint16_t param_len, uint8_t* p_param) {
    switch (event & NCI_OID_MASK) {
        case NCI_MSG_PROP_ANDROID: {
            uint8_t android_sub_opcode = p_param[3];
            switch (android_sub_opcode) {
                case NCI_QUERY_ANDROID_PASSIVE_OBSERVE: {
                    sObserveModeState = p_param[5];
                    LOG(INFO) << StringPrintf("Query observe mode state response is %x",
                                              sObserveModeState);
                    SyncEventGuard guard(sNfaVsCommand);
                    sNfaVsCommand.notifyOne();
                } break;
                case NCI_ANDROID_SET_PASSIVE_OBSERVER_TECH: {
                    if (param_len == 5) {
                        if ((p_param[0] & NCI_MT_MASK) == (NCI_MT_RSP << NCI_MT_SHIFT)) {
                            sVSCmdStatus = p_param[4];
                            LOG(INFO) << StringPrintf("Observe mode RSP: status: %x", sVSCmdStatus);
                            SyncEventGuard guard(sNfaVsCommand);
                            sNfaVsCommand.notifyOne();
                        } else {
                            LOG(WARNING) << StringPrintf(
                                    "Observe Mode RSP has incorrect message type: %x", p_param[0]);
                        }
                    } else {
                        LOG(WARNING) << StringPrintf("Observe Mode RSP has incorrect length: %d",
                                                     param_len);
                    }
                } break;
                case NCI_ANDROID_POLLING_FRAME_NTF: {
                    // TODO
                } break;
                case NCI_ANDROID_GET_CAPS: {
                    sVSCmdStatus = p_param[4];
                    SyncEventGuard guard(sNfaVsCommand);
                    sCaps.assign(p_param + 8, p_param + param_len);
                    sNfaVsCommand.notifyOne();
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
 * Get observe mode state.
 */
tNFA_STATUS static nfaQueryObserveModeState() {
    tNFA_STATUS status = NFA_STATUS_FAILED;

    uint8_t cmd[] = {NCI_QUERY_ANDROID_PASSIVE_OBSERVE};

    status = NFA_SendVsCommand(NCI_MSG_PROP_ANDROID, sizeof(cmd), cmd, nfaVSCallback);

    if (status == NFA_STATUS_OK) {
        if (!sNfaVsCommand.wait(1000)) {
            LOG(WARNING) << "Timeout waiting for query observe mode response";
            return NFA_STATUS_TIMEOUT;
        }
    }

    return status;
}

/*
 * Enable per-technology observe mode.
 */
tNFA_STATUS static nfaSetPassiveObserverTech(uint8_t tech_mask) {
    tNFA_STATUS status = NFA_STATUS_FAILED;

    uint8_t cmd[] = {NCI_ANDROID_SET_PASSIVE_OBSERVER_TECH, tech_mask};

    status = NFA_SendVsCommand(NCI_MSG_PROP_ANDROID, sizeof(cmd), cmd, nfaVSCallback);

    if (status == NFA_STATUS_OK) {
        if (!sNfaVsCommand.wait(SET_PASSIVE_OBSERVER_TECH_TIMEOUT_MS)) {
            LOG(WARNING) << "Timeout waiting for set observer tech response";
            return NFA_STATUS_TIMEOUT;
        }
    }

    return status;
}

/*
 * Get chipset capabilities.
 */
tNFA_STATUS static nfaGetCaps() {
    tNFA_STATUS status = NFA_STATUS_FAILED;

    uint8_t cmd[] = {NCI_ANDROID_GET_CAPS};
    status = NFA_SendVsCommand(NCI_MSG_PROP_ANDROID, sizeof(cmd), cmd, nfaVSCallback);

    if (status == NFA_STATUS_OK) {
        if (!sNfaVsCommand.wait(1000)) {
            LOG(WARNING) << "Timeout waiting for GET_CAPS response";
            return NFA_STATUS_TIMEOUT;
        }
    }

    return status;
}

/*
 * Get observe mode capabilities.
 */
uint8_t static getCapsPassiveObserverModeValue() {
    return sCaps[2];
}

class NfcBehaviorChanges : public testing::TestWithParam<std::string> {
protected:
    void SetUp() override {
        tNFA_STATUS status = NFA_STATUS_OK;
        status = NFA_StartRfDiscovery();
        ASSERT_EQ(status, NFA_STATUS_OK);
        ASSERT_TRUE(sNfaEnableDisablePollingEvent.wait(1000)) << "Timeout starting RF discovery";
    }

    static void SetUpTestSuite() {
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

        // Disable polling.
        uint8_t rf_discovery_cmd[] = {0x21, 0x03, 0x07, 0x03, 0x80, 0x01, 0x81, 0x01, 0x82, 0x01};
        NFA_SendRawVsCommand(sizeof(rf_discovery_cmd), rf_discovery_cmd, nfaVSCallback);
        usleep(10000);

        if (get_vsr_api_level() >= 202504) {
            uint8_t cmd[] = {NCI_ANDROID_SET_PASSIVE_OBSERVER_TECH, 0x03};
            status = NFA_SendVsCommand(NCI_MSG_PROP_ANDROID, sizeof(cmd), cmd, nfaVSCallback);
            if (status == NFA_STATUS_OK) {
                if (!sNfaVsCommand.wait(1000)) {
                    LOG(WARNING) << "Timeout waiting for observemode response";
                }
            }
        }
    }

    static void TearDownTestSuite() {
        uint8_t rf_deactivate_cmd[] = {0x21, 0x06, 0x01, 0x00};
        NFA_SendRawVsCommand(sizeof(rf_deactivate_cmd), rf_deactivate_cmd, nfaVSCallback);
        usleep(10000);
    }
};

/*
 * SetPassiveObserverTech_getCaps:
 * Verifies GET_CAPS returns get correct value for observe mode capabilities.
 */
TEST_P(NfcBehaviorChanges, SetPassiveObserverTech_getCaps) {
    if (get_vsr_api_level() < 202504) {
        GTEST_SKIP() << "Skipping test for board API level < 202504";
    }

    tNFC_STATUS status = nfaGetCaps();

    ASSERT_EQ(status, NFC_STATUS_OK);
    ASSERT_EQ(getCapsPassiveObserverModeValue(), 0x2);
}

/*
 * SetPassiveObserverTech_allExceptF:
 * Verifies observe mode can be enabled for NFC-A, NFC-B, NFC-V, and disable for NFC-F.
 *
 * @VsrTest = GMS-VSR-3.2.8-002
 */
TEST_P(NfcBehaviorChanges, SetPassiveObserverTech_allExceptF) {
    if (get_vsr_api_level() < 202504) {
        GTEST_SKIP() << "Skipping test for board API level < 202504";
    }

    tNFC_STATUS status = nfaSetPassiveObserverTech(NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_A |
                                       NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_B);
    ASSERT_EQ(status, NFA_STATUS_OK);
    status = nfaQueryObserveModeState();
    ASSERT_EQ(status, NFA_STATUS_OK);
    ASSERT_EQ(sObserveModeState, NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_A |
                        NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_B);
}

/*
 * SetPassiveObserverTech_allOnAndOff:
 * Verifies observe mode can be enabled and disabled for all technologies.
 *
 * @VsrTest = GMS-VSR-3.2.8-002
 */
TEST_P(NfcBehaviorChanges, SetPassiveObserverTech_allOnAndOff) {
    if (get_vsr_api_level() < 202504) {
        GTEST_SKIP() << "Skipping test for board API level < 202504";
    }

    tNFC_STATUS status = nfaSetPassiveObserverTech(NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_A |
                                                   NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_B);
    ASSERT_EQ(status, NFA_STATUS_OK);
    status = nfaQueryObserveModeState();
    ASSERT_EQ(status, NFA_STATUS_OK);
    ASSERT_EQ(sObserveModeState, NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_A |
                                         NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_B);

    status = nfaSetPassiveObserverTech(0x00);
    ASSERT_EQ(status, NFA_STATUS_OK);
    status = nfaQueryObserveModeState();
    ASSERT_EQ(status, NFA_STATUS_OK);
    ASSERT_EQ(sObserveModeState, 0x00);
}

/*
 * SetPassiveObserverTech_testThroughput:
 * Verifies observe mode can be enabled and disabled repeatedly without timing out or erroring.
 *
 * @VsrTest = GMS-VSR-3.2.8-002
 */
TEST_P(NfcBehaviorChanges, SetPassiveObserverTech_testThroughput) {
    if (get_vsr_api_level() < 202504) {
        GTEST_SKIP() << "Skipping test for board API level < 202504";
    }

    for (int i = 0; i < 100; ++i) {
        tNFC_STATUS status = nfaSetPassiveObserverTech(NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_A |
                                                       NCI_ANDROID_PASSIVE_OBSERVE_PARAM_ENABLE_B);
        ASSERT_EQ(status, NFA_STATUS_OK);

        status = nfaSetPassiveObserverTech(0x00);
        ASSERT_EQ(status, NFA_STATUS_OK);
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NfcBehaviorChanges);
INSTANTIATE_TEST_SUITE_P(Nfc, NfcBehaviorChanges,
        testing::ValuesIn(::android::getAidlHalInstanceNames(INfc::descriptor)),
        ::android::PrintInstanceNameToString
);

int main(int argc, char **argv) {
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
