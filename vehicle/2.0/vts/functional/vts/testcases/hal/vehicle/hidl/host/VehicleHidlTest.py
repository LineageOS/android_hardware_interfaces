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


class VehicleHidlTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """A simple testcase for the VEHICLE HIDL HAL."""

    def setUpClass(self):
        """Creates a mirror and init vehicle hal."""
        self.dut = self.registerController(android_device)[0]

        self.dut.shell.InvokeTerminal("one")

        if self.enable_profiling:
            profiling_utils.EnableVTSProfiling(self.dut.shell.one)

        self.dut.hal.InitHidlHal(
            target_type="vehicle",
            target_basepaths=["/system/lib64"],
            target_version=2.0,
            target_package="android.hardware.vehicle",
            target_component_name="IVehicle",
            bits=64)

    def tearDownClass(self):
        """ If profiling is enabled for the test, collect the profiling data
            and disable profiling after the test is done.
        """
        if self.enable_profiling:
            profiling_trace_path = getattr(
                self, self.VTS_PROFILING_TRACING_PATH, "")
            self.ProcessAndUploadTraceData(self.dut, profiling_trace_path)
            profiling_utils.DisableVTSProfiling(self.dut.shell.one)

    def testEcho1(self):
        """A simple testcase which sends a command."""
        self.dut.shell.InvokeTerminal("my_shell1")  # creates a remote shell instance.
        results = self.dut.shell.my_shell1.Execute("echo hello_world")  # runs a shell command.
        logging.info(str(results[const.STDOUT]))  # prints the stdout
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello_world")  # checks the stdout
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)  # checks the exit code

    def testEcho2(self):
        """A simple testcase which sends two commands."""
        self.dut.shell.InvokeTerminal("my_shell2")
        my_shell = getattr(self.dut.shell, "my_shell2")
        results = my_shell.Execute(["echo hello", "echo world"])
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 2)  # check the number of processed commands
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello")
        asserts.assertEqual(results[const.STDOUT][1].strip(), "world")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)
        asserts.assertEqual(results[const.EXIT_CODE][1], 0)

if __name__ == "__main__":
    test_runner.main()
