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


class SensorsHidlTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Host testcase class for the SENSORS HIDL HAL.

    This class set-up/tear-down the webDB host test framwork and contains host test cases for
    sensors HIDL HAL.
    """

    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer SENSORS service."""
        self.dut = self.registerController(android_device)[0]

        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode

        # Test using the binderized mode
        self.dut.shell.one.Execute(
            "setprop vts.hal.vts.hidl.get_stub true")

        if self.enable_profiling:
            profiling_utils.EnableVTSProfiling(self.dut.shell.one)

        self.dut.hal.InitHidlHal(
            target_type="sensors",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.sensors",
            target_component_name="ISensors",
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

    def testSensorsBasic(self):
        """Test the basic operation of test framework and sensor HIDL HAL

        This test obtains predefined enum values via sensors HIDL HAL host test framework and
        compares them to known values as a sanity check to make sure both sensors HAL
        and the test framework are working properly.
        """
        sensors_types = self.dut.hal.sensors.GetHidlTypeInterface("types")
        logging.info("sensors_types: %s", sensors_types)
        logging.info("OK: %s", sensors_types.OK)
        logging.info("BAD_VALUE: %s", sensors_types.BAD_VALUE)
        logging.info("PERMISSION_DENIED: %s", sensors_types.PERMISSION_DENIED)
        logging.info("INVALID_OPERATION: %s", sensors_types.INVALID_OPERATION)
        asserts.assertEqual(sensors_types.OK, 0);
        asserts.assertEqual(sensors_types.BAD_VALUE, 1);

        logging.info("sensor types:")
        logging.info("SENSOR_TYPE_ACCELEROMETER: %s", sensors_types.SENSOR_TYPE_ACCELEROMETER)
        logging.info("SENSOR_TYPE_GEOMAGNETIC_FIELD: %s", sensors_types.SENSOR_TYPE_GEOMAGNETIC_FIELD)
        logging.info("SENSOR_TYPE_GYROSCOPE: %s", sensors_types.SENSOR_TYPE_GYROSCOPE)
        asserts.assertEqual(sensors_types.SENSOR_TYPE_ACCELEROMETER, 1);
        asserts.assertEqual(sensors_types.SENSOR_TYPE_GEOMAGNETIC_FIELD, 2);
        asserts.assertEqual(sensors_types.SENSOR_TYPE_GYROSCOPE, 4);

if __name__ == "__main__":
    test_runner.main()
