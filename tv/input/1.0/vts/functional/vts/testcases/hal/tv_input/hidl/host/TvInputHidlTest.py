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


class TvInputHidlTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Two hello world test cases which use the shell driver."""

    def setUpClass(self):
        """Creates a mirror and init tv input hal."""
        self.dut = self.registerController(android_device)[0]

        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode

        self.dut.hal.InitHidlHal(target_type="tv_input",
                                 target_basepaths=["/system/lib64"],
                                 target_version=1.0,
                                 target_package="android.hardware.tv.input",
                                 target_component_name="ITvInput",
                                 bits=64)

        self.dut.shell.InvokeTerminal("one")

    def testGetStreamConfigurations(self):
        configs = self.dut.hal.tv_input.getStreamConfigurations(0)
        logging.info('tv input configs: %s', configs)


if __name__ == "__main__":
    test_runner.main()
