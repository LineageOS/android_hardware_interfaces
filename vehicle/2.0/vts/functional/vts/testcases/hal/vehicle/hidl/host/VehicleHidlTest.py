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
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode

        if self.enable_profiling:
            profiling_utils.EnableVTSProfiling(self.dut.shell.one)

        self.dut.hal.InitHidlHal(
            target_type="vehicle",
            target_basepaths=self.dut.libPaths,
            target_version=2.0,
            target_package="android.hardware.vehicle",
            target_component_name="IVehicle",
            hw_binder_service_name="Vehicle",
            bits=64 if self.dut.is64Bit else 32)

    def tearDownClass(self):
        """ If profiling is enabled for the test, collect the profiling data
            and disable profiling after the test is done.
        """
        if self.enable_profiling:
            profiling_trace_path = getattr(
                self, self.VTS_PROFILING_TRACING_PATH, "")
            self.ProcessAndUploadTraceData(self.dut, profiling_trace_path)
            profiling_utils.DisableVTSProfiling(self.dut.shell.one)

    def testListProperties(self):
        logging.info("vehicle_types")
        vehicle_types = self.dut.hal.vehicle.GetHidlTypeInterface("types")
        logging.info("vehicle_types: %s", vehicle_types)

        allConfigs = self.dut.hal.vehicle.getAllPropConfigs()
        logging.info("all supported properties: %s", allConfigs)

if __name__ == "__main__":
    test_runner.main()
