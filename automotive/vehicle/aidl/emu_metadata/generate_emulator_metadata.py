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

RE_ENUM = re.compile(r"\s*enum\s+(\w*) {\n(.*)}", re.MULTILINE | re.DOTALL)
RE_COMMENT = re.compile(r"(?:(?:\/\*\*)((?:.|\n)*?)(?:\*\/))?(?:\n|^)\s*(\w*)(?:\s+=\s*)?((?:[a-zA-Z0-9]|\s|\+|)*),", re.DOTALL)
RE_BLOCK_COMMENT_TITLE = re.compile("^(?:\s|\*)*((?:\w|\s|\.)*)\n(?:\s|\*)*(?:\n|$)")
RE_BLOCK_COMMENT_ANNOTATION = re.compile("^(?:\s|\*)*@(\w*)\s+((?:\w|:)*)", re.MULTILINE)
RE_HEX_NUMBER = re.compile("([0-9A-Fa-fxX]+)")


class JEnum:
    def __init__(self, name):
        self.name = name
        self.values = []


class Converter:
    # Only addition is supported for now, but that covers all existing properties except
    # OBD diagnostics, which use bitwise shifts
    def calculateValue(self, expression, default_value):
        numbers = RE_HEX_NUMBER.findall(expression)
        if len(numbers) == 0:
            return default_value
        result = 0
        base = 10
        if numbers[0].lower().startswith("0x"):
            base = 16
        for number in numbers:
            result += int(number, base)
        return result

    def parseBlockComment(self, value, blockComment):
        titles = RE_BLOCK_COMMENT_TITLE.findall(blockComment)
        for title in titles:
            value['name'] = title
            break
        annots_res = RE_BLOCK_COMMENT_ANNOTATION.findall(blockComment)
        for annot in annots_res:
            value[annot[0]] = annot[1]

    def parseEnumContents(self, enum: JEnum, enumValue):
        matches = RE_COMMENT.findall(enumValue)
        defaultValue = 0
        for match in matches:
            value = dict()
            value['name'] = match[1]
            value['value'] = self.calculateValue(match[2], defaultValue)
            defaultValue = value['value'] + 1
            if enum.name == "VehicleProperty":
                block_comment = match[0]
                self.parseBlockComment(value, block_comment)
            enum.values.append(value)

    def convert(self, input):
        text = Path(input).read_text()
        matches = RE_ENUM.findall(text)
        jenums = []
        for match in matches:
            enum = JEnum(match[0])
            self.parseEnumContents(enum, match[1])
            jenums.append(enum)
        return jenums

def main():
    if (len(sys.argv) != 3):
        print("Usage: ", sys.argv[0], " INPUT_PATH OUTPUT")
        sys.exit(1)
    aidl_path = sys.argv[1]
    out_path = sys.argv[2]
    result = []
    for file in os.listdir(aidl_path):
        result.extend(Converter().convert(os.path.join(aidl_path, file)))
    json_result = json.dumps(result, default=vars, indent=2)
    with open(out_path, 'w') as f:
        f.write(json_result)


if __name__ == "__main__":
    main()
