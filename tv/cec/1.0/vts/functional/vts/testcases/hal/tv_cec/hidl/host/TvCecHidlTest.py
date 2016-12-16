#!/usr/bin/env python
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

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class TvCecHidlTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Host testcase class for the TV HDMI_CEC HIDL HAL."""

    def setUpClass(self):
        """Creates a mirror and init tv hdmi cec hal service."""
        self.dut = self.registerController(android_device)[0]

        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode

        self.dut.hal.InitHidlHal(target_type="tv_cec",
                                 target_basepaths=["/system/lib64"],
                                 target_version=1.0,
                                 target_package="android.hardware.tv.cec",
                                 target_component_name="IHdmiCec",
                                 bits=64)

    def testGetCecVersion1(self):
        """A simple test case which queries the cec version."""
        logging.info('DIR HAL %s', dir(self.dut.hal))
        version = self.dut.hal.tv_cec.getCecVersion()
        logging.info('Cec version: %s', version)


if __name__ == "__main__":
    test_runner.main()
