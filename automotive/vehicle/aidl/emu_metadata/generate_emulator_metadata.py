#!/usr/bin/python3

#
# Script for generation of VHAL properties metadata .json from AIDL interface
#
# This metadata is used to display human property names, names of enum
# data types for their values, change and access modes and other information,
# available from AIDL block comments, but not at runtime.
#
# Usage example:
#  ./emu_metadata/generate_emulator_metadata.py android/hardware/automotive/vehicle $OUT/android.hardware.automotive.vehicle-types-meta.json
#  (Note, that the resulting file has to match a '*types-meta.json' pattern to be parsed by the emulator).
#

import json
import os
import re
import sys

from pathlib import Path

RE_PACKAGE = re.compile(r"\npackage\s([\.a-z0-9]*);")
RE_IMPORT = re.compile(r"\nimport\s([\.a-zA-Z0-9]*);")
RE_ENUM = re.compile(r"\s*enum\s+(\w*) {\n(.*)}", re.MULTILINE | re.DOTALL)
RE_COMMENT = re.compile(r"(?:(?:\/\*\*)((?:.|\n)*?)(?:\*\/))?(?:\n|^)\s*(\w*)(?:\s+=\s*)?((?:[\.\-a-zA-Z0-9]|\s|\+|)*),",
                        re.DOTALL)
RE_BLOCK_COMMENT_TITLE = re.compile("^(?:\s|\*)*((?:\w|\s|\.)*)\n(?:\s|\*)*(?:\n|$)")
RE_BLOCK_COMMENT_ANNOTATION = re.compile("^(?:\s|\*)*@(\w*)\s+((?:[\w:\.])*)", re.MULTILINE)
RE_HEX_NUMBER = re.compile("([\.\-0-9A-Za-z]+)")


class JEnum:
    def __init__(self, package, name):
        self.package = package
        self.name = name
        self.values = []

class Enum:
    def __init__(self, package, name, text, imports):
        self.text = text
        self.parsed = False
        self.imports = imports
        self.jenum = JEnum(package, name)

    def parse(self, enums):
        if self.parsed:
            return
        for dep in self.imports:
            enums[dep].parse(enums)
        print("Parsing " + self.jenum.name)
        matches = RE_COMMENT.findall(self.text)
        defaultValue = 0
        for match in matches:
            value = dict()
            value['name'] = match[1]
            value['value'] = self.calculateValue(match[2], defaultValue, enums)
            defaultValue = value['value'] + 1
            if self.jenum.name == "VehicleProperty":
                block_comment = match[0]
                self.parseBlockComment(value, block_comment)
            self.jenum.values.append(value)
        self.parsed = True
        self.text = None

    def get_value(self, value_name):
        for value in self.jenum.values:
            if value['name'] == value_name:
                return value['value']
        raise Exception("Cannot decode value: " + self.jenum.package + " : " + value_name)

    def calculateValue(self, expression, default_value, enums):
        numbers = RE_HEX_NUMBER.findall(expression)
        if len(numbers) == 0:
            return default_value
        result = 0
        base = 10
        if numbers[0].lower().startswith("0x"):
            base = 16
        for number in numbers:
            if '.' in number:
                package, val_name = number.split('.')
                for dep in self.imports:
                    if package in dep:
                        result += enums[dep].get_value(val_name)
            else:
                result += int(number, base)
        return result

    def parseBlockComment(self, value, blockComment):
        titles = RE_BLOCK_COMMENT_TITLE.findall(blockComment)
        for title in titles:
            value['name'] = title
            break
        annots_res = RE_BLOCK_COMMENT_ANNOTATION.findall(blockComment)
        for annot in annots_res:
            value[annot[0]] = annot[1].replace(".", ":")

class Converter:
    # Only addition is supported for now, but that covers all existing properties except
    # OBD diagnostics, which use bitwise shifts
    def convert(self, input):
        text = Path(input).read_text()
        matches = RE_ENUM.findall(text)
        package = RE_PACKAGE.findall(text)[0]
        imports = RE_IMPORT.findall(text)
        enums = []
        for match in matches:
            enum = Enum(package, match[0], match[1], imports)
            enums.append(enum)
        return enums


def main():
    if (len(sys.argv) != 3):
        print("Usage: ", sys.argv[0], " INPUT_PATH OUTPUT")
        sys.exit(1)
    aidl_path = sys.argv[1]
    out_path = sys.argv[2]
    enums_dict = dict()
    for file in os.listdir(aidl_path):
        enums = Converter().convert(os.path.join(aidl_path, file))
        for enum in enums:
            enums_dict[enum.jenum.package + "." + enum.jenum.name] = enum

    result = []
    for enum_name, enum in enums_dict.items():
        enum.parse(enums_dict)
        result.append(enum.jenum.__dict__)

    json_result = json.dumps(result, default=None, indent=2)
    with open(out_path, 'w') as f:
        f.write(json_result)


if __name__ == "__main__":
    main()
