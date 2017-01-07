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

from vts.proto import ComponentSpecificationMessage_pb2 as CompSpecMsg
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

        self.dut.shell.one.Execute(
            "setprop vts.hal.vts.hidl.get_stub true")

        self.dut.hal.InitHidlHal(
            target_type="tv_cec",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.tv.cec",
            target_component_name="IHdmiCec",
            hw_binder_service_name="tv.cec",
            bits=64 if self.dut.is64Bit else 32)

    def testGetCecVersion1(self):
        """A simple test case which queries the cec version."""
        logging.info('DIR HAL %s', dir(self.dut.hal))
        version = self.dut.hal.tv_cec.getCecVersion()
        logging.info('Cec version: %s', version)

    def testSendRandomMessage(self):
        """A test case which sends a random message."""
        self.vtypes = self.dut.hal.tv_cec.GetHidlTypeInterface("types")
        logging.info("tv_cec types: %s", self.vtypes)

        message = CompSpecMsg.VariableSpecificationMessage()
        message.name = "CecMessage"
        message.type = CompSpecMsg.TYPE_STRUCT
        initiator = message.struct_value.add()
        initiator.name = "initiator"
        initiator.type = CompSpecMsg.TYPE_ENUM
        initiator.scalar_value.int32_t = self.vtypes.TV
        destination = message.struct_value.add()
        destination.name = "destination"
        destination.type = CompSpecMsg.TYPE_ENUM
        destination.scalar_value.int32_t = self.vtypes.PLAYBACK_1
        body = message.struct_value.add()
        body.name = "body"
        body.type = CompSpecMsg.TYPE_VECTOR
        vector1 = body.vector_value.add()
        vector1.type = CompSpecMsg.TYPE_SCALAR
        vector1.scalar_type = "uint8_t"
        vector1.scalar_value.uint8_t = 1
        vector2 = body.vector_value.add()
        vector2.type = CompSpecMsg.TYPE_SCALAR
        vector2.scalar_type = "uint8_t"
        vector2.scalar_value.uint8_t = 2
        vector3 = body.vector_value.add()
        vector3.type = CompSpecMsg.TYPE_SCALAR
        vector3.scalar_type = "uint8_t"
        vector3.scalar_value.uint8_t = 3
        logging.info("message: %s", message)
        result = self.dut.hal.tv_cec.sendMessage(message)
        logging.info('sendMessage result: %s', result)

if __name__ == "__main__":
    test_runner.main()
