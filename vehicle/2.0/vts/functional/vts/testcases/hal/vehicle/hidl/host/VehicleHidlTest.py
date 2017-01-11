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

        self.vehicle = self.dut.hal.vehicle  # shortcut
        self.vtypes = self.dut.hal.vehicle.GetHidlTypeInterface("types")
        logging.info("vehicle types: %s", self.vtypes)

    def tearDownClass(self):
        """Disables the profiling.

        If profiling is enabled for the test, collect the profiling data
        and disable profiling after the test is done.
        """
        if self.enable_profiling:
            profiling_trace_path = getattr(
                self, self.VTS_PROFILING_TRACING_PATH, "")
            self.ProcessAndUploadTraceData(self.dut, profiling_trace_path)
            profiling_utils.DisableVTSProfiling(self.dut.shell.one)

    def testListProperties(self):
        """Checks whether some PropConfigs are returned.

        Verifies that call to getAllPropConfigs is not failing and
        it returns at least 1 vehicle property config.
        """
        allConfigs = self.vehicle.getAllPropConfigs()
        logging.info("all supported properties: %s", allConfigs)
        asserts.assertLess(0, len(allConfigs))

    def testMandatoryProperties(self):
        """Verifies that all mandatory properties are supported."""
        mandatoryProps = set([self.vtypes.DRIVING_STATUS])  # 1 property so far
        logging.info(self.vtypes.DRIVING_STATUS)
        allConfigs = self.dut.hal.vehicle.getAllPropConfigs()

        for config in allConfigs:
            mandatoryProps.discard(config['prop'])

        asserts.assertEqual(0, len(mandatoryProps))

    def getSupportInfo(self):
        """Check whether OBD2_{LIVE|FREEZE}_FRAME is supported."""
        isLiveSupported, isFreezeSupported = False, False
        allConfigs = self.vehicle.getAllPropConfigs()
        for config in allConfigs:
            if config['prop'] == self.vtypes.OBD2_LIVE_FRAME:
                isLiveSupported = True
            elif config['prop'] == self.vtypes.OBD2_FREEZE_FRAME:
                isFreezeSupported = True
            if isLiveSupported and isFreezeSupported:
                break
        return isLiveSupported, isFreezeSupported

    def testObd2SensorProperties(self):
        """Test reading the live and freeze OBD2 frame properties.

        OBD2 (On-Board Diagnostics 2) is the industry standard protocol
        for retrieving diagnostic sensor information from vehicles.
        """
        def checkLiveFrameRead():
            """Validates reading the OBD2_LIVE_FRAME (if available)."""
            logging.info("checkLiveFrameRead no-op pass")

        def checkFreezeFrameRead():
            """Validates reading the OBD2_FREEZE_FRAME (if available)."""
            logging.info("checkLiveFrameRead no-op pass")

        isLiveSupported, isFreezeSupported = self.getSupportInfo()
        logging.info("isLiveSupported = %s, isFreezeSupported = %s",
                     isLiveSupported, isFreezeSupported)
        if isLiveSupported:
            checkLiveFrameRead()
        if isFreezeSupported:
            checkFreezeFrameRead()

if __name__ == "__main__":
    test_runner.main()
