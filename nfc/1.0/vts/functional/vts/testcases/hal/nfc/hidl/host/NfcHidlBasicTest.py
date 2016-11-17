#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import time

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

PASSTHROUGH_MODE_KEY = "passthrough_mode"


class NfcHidlBasicTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """A simple testcase for the NFC HIDL HAL."""

    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer NFC service."""
        self.dut = self.registerController(android_device)[0]

        self.getUserParams(opt_param_names=[PASSTHROUGH_MODE_KEY])

        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode
        self.dut.shell.one.Execute("service call nfc 6")  # Turn off
        time.sleep(2)

        if getattr(self, PASSTHROUGH_MODE_KEY, True):
            self.dut.shell.one.Execute(
                "setprop vts.hal.vts.hidl.get_stub true")
        else:
            self.dut.shell.one.Execute(
                "setprop vts.hal.vts.hidl.get_stub false")

        self.dut.hal.InitHidlHal(target_type="nfc",
                                 target_basepaths=["/system/lib64"],
                                 target_version=1.0,
                                 target_package="android.hardware.nfc",
                                 target_component_name="INfc",
                                 bits=64)

    def tearDownClass(self):
        """Turns off the framework-layer NFC service."""
        self.dut.shell.one.Execute("service call nfc 6")  # make sure it's off

    def testBase(self):
        """A simple test case which just calls each registered function."""
        # TODO: extend to make realistic testcases
        # For example, call after CORE_INIT_RSP is received.
        # result = self.dut.hal.nfc.coreInitialized([1])
        # logging.info("coreInitialized result: %s", result)

        def send_event(NfcEvent, NfcStatus):
            logging.info("callback send_event")
            logging.info("arg0 %s", NfcEvent)
            logging.info("arg1 %s", NfcStatus)

        def send_data(NfcData):
            logging.info("callback send_data")
            logging.info("arg0 %s", NfcData)

        client_callback = self.dut.hal.nfc.GetHidlCallbackInterface(
            "INfcClientCallback",
            sendEvent=send_event,
            sendData=send_data)

        result = self.dut.hal.nfc.open(client_callback)
        logging.info("open result: %s", result)

        result = self.dut.hal.nfc.prediscover()
        logging.info("prediscover result: %s", result)

        result = self.dut.hal.nfc.controlGranted()
        logging.info("controlGranted result: %s", result)

        result = self.dut.hal.nfc.powerCycle()
        logging.info("powerCycle result: %s", result)

        nfc_types = self.dut.hal.nfc.GetHidlTypeInterface("types")
        logging.info("nfc_types: %s", nfc_types)

        result = self.dut.hal.nfc.write([0, 1, 2, 3, 4, 5])
        logging.info("write result: %s", result)

        result = self.dut.hal.nfc.close()
        logging.info("close result: %s", result)

        self.SetCoverageData(self.dut.hal.nfc.GetRawCodeCoverage())

if __name__ == "__main__":
    test_runner.main()
