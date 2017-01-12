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
from vts.utils.python.profiling import profiling_utils


class VibratorHidlTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """A simple testcase for the VIBRATOR HIDL HAL."""

    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer VIBRATOR service."""
        self.dut = self.registerController(android_device)[0]

        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode

        # Test using the binderized mode
        self.dut.shell.one.Execute(
            "setprop vts.hal.vts.hidl.get_stub true")

        if getattr(self, self.ENABLE_PROFILING, False):
            profiling_utils.EnableVTSProfiling(self.dut.shell.one)

        self.dut.hal.InitHidlHal(
            target_type="vibrator",
            target_basepaths=["/system/lib64"],
            target_version=1.0,
            target_package="android.hardware.vibrator",
            target_component_name="IVibrator",
            bits=64)

    def tearDownClass(self):
        """ If profiling is enabled for the test, collect the profiling data
            and disable profiling after the test is done.
        """
        if getattr(self, self.ENABLE_PROFILING, False):
            profiling_trace_path = getattr(
                self, self.VTS_PROFILING_TRACING_PATH, "")
            self.ProcessAndUploadTraceData(self.dut, profiling_trace_path)
            profiling_utils.DisableVTSProfiling(self.dut.shell.one)

    def testVibratorBasic(self):
        """A simple test case which just calls each registered function."""
        vibrator_types = self.dut.hal.vibrator.GetHidlTypeInterface("types")
        logging.info("vibrator_types: %s", vibrator_types)
        logging.info("OK: %s", vibrator_types.OK)
        logging.info("ERR: %s", vibrator_types.ERR)

        result = self.dut.hal.vibrator.on(10000)
        logging.info("on result: %s", result)
        asserts.assertEqual(vibrator_types.OK, result)

        time.sleep(1)

        result = self.dut.hal.vibrator.off()
        logging.info("off result: %s", result)
        asserts.assertEqual(vibrator_types.OK, result)

if __name__ == "__main__":
    test_runner.main()
