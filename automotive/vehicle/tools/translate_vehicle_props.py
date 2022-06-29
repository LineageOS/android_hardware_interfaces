#!/usr/bin/python

# Copyright (C) 2021 The Android Open Source Project
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
"""Tools to translate VehicleProperty from types.hal into AIDL format.
   To store it to a file:
   $ python translate_vehicle_props.py types.hal > VehicleProperty.aidl
"""
import re
import sys

ENUM_TYPE_TO_PARSE = [ "VehiclePropertyType", "VehiclePropertyGroup", "VehicleArea" ]
VEHICLE_PROP_ENUM = "VehicleProperty"

RE_COMMENT_BEGIN = re.compile("\s*\/\*\*")
RE_COMMENT_END = re.compile("\s*\*\/")
RE_COMMENT_SINGLE_LINE = re.compile("\s*\/\*\*.*\*\/")

RE_ENUM_START = re.compile("\s*enum\s*(\w+)\s?.*\{")
RE_ENUM_END = re.compile("\s*\}\;")

RE_ENUM_ELEMENT = re.compile("\s*(\w+)\s*\=\s*(\w+),")

RE_VEHICLE_PROP_ELEMENT = re.compile("\s*(\w+)\s*\=\s*\(?\s*(\w+)\s*\|?\s*(\w+\:\w+)?\s*\|?\s*(\w+:\w+)?\s*\|?\s*(\w+:\w+)?\s*\)?,")

DBG_COMMENT = False
DBG_ENUM = False

class HIDLParser:
    def __init__(self):
        self.inEnum = False
        self.currentEnumName = None
        self.inVehicleProperty = False
        self.inComment = False
        self.recentComments = []
        self.currentType = None
        self.enumMap = {}
        self.outputMsg = []
        self.multilineFormat = []
        self.outputMsg.append("package android.hardware.automotive.vehicle\n\n")

    def addRecentCommentToMsg(self):
        self.outputMsg.extend(self.recentComments)
        self.recentComments = []

    def addToMsg(self, msg):
        self.outputMsg.append(msg)

    def printOutputMsg(self):
        msg = "".join(self.outputMsg)
        print(msg)

    def parseLine(self, line):
        if self.inComment:
            self.recentComments.append(line)
            if RE_COMMENT_END.match(line):
                self.inComment = False
                if DBG_COMMENT:
                    print("Comment end:{}".format(self.recentComments))
            return
        elif RE_COMMENT_BEGIN.match(line):
            self.recentComments = []
            self.recentComments.append(line)
            if RE_COMMENT_SINGLE_LINE.match(line):
                if DBG_COMMENT:
                    print("Single line Comment:{}".format(self.recentComments))
                return
            self.inComment = True
            if DBG_COMMENT:
                print("Comment start")
            return

        if self.inEnum:
            if RE_ENUM_END.match(line):
                self.inEnum = False
                if DBG_ENUM:
                    print("End enum {}".format(self.currentEnumName))
            else:
                matchElement = RE_ENUM_ELEMENT.match(line);
                if matchElement:
                    elementName = matchElement.group(1)
                    elementValue = matchElement.group(2)
                    self.enumMap[self.currentEnumName + ':' + elementName] = elementValue
        elif self.inVehicleProperty:
            if RE_ENUM_END.match(line):
                self.inVehicleProperty = False
                self.addToMsg("}\n")
            else:
                text = line.strip()
                if len(text) == 0:
                    self.multilineFormat = []
                else:
                    self.multilineFormat.append(text)
                    textToMatch = "".join(self.multilineFormat)
                    match = RE_VEHICLE_PROP_ELEMENT.match(textToMatch)
                    if match:
                        self.multilineFormat = []
                        name = match.group(1)
                        val = match.group(2)
                        type1 = match.group(3)
                        self.addRecentCommentToMsg()
                        if type1 == None: # one line case
                            self.addToMsg("    {} = {},\n".format(name, val))
                        else:
                            type2 = match.group(4)
                            type3 = match.group(5)
                            self.addToMsg("    {} = {} + {} + {} + {}, // {},{},{}\n".\
                            format(name, val, self.enumMap[type1], self.enumMap[type3],\
                                self.enumMap[type2], type1, type3, type2))
        else:
            matchEnum = RE_ENUM_START.match(line)
            if matchEnum:
                enumName = matchEnum.group(1)
                if enumName in ENUM_TYPE_TO_PARSE:
                    self.currentEnumName = enumName
                    self.inEnum = True
                    if DBG_ENUM:
                        print("enum {}".format(enumName))
                elif enumName == VEHICLE_PROP_ENUM:
                    self.inVehicleProperty = True
                    self.addRecentCommentToMsg()
                    self.addToMsg("@Backing(type=\"int\")\nenum VehicleProperty {\n")
                    if DBG_ENUM:
                        print("VehicleProperty starts, all enum values {}".format(self.enumMap))


def main():
    if len(sys.argv) != 2:
        print("Usage: {} types_hal_file".format(sys.argv[0]))
        sys.exit(1)
    file_name = sys.argv[1]
    with open(file_name, 'r') as f:
        parser = HIDLParser()
        for line in f.readlines():
            parser.parseLine(line)
        parser.printOutputMsg()

if __name__ == "__main__":
    main()
