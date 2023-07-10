#!/usr/bin/env python3
#
# Copyright (C) 2023 The Android Open Source Project
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
"""
Creates the next compatibility matrix.

Requires libvintf Level.h to be updated before executing this script.
"""

import argparse
import os
import pathlib
import shutil
import subprocess
import textwrap


def check_call(*args, **kwargs):
    print(args)
    subprocess.check_call(*args, **kwargs)


def check_output(*args, **kwargs):
    print(args)
    return subprocess.check_output(*args, **kwargs)


class Bump(object):

    def __init__(self, cmdline_args):
        self.top = pathlib.Path(os.environ["ANDROID_BUILD_TOP"])
        self.interfaces_dir = self.top / "hardware/interfaces"

        self.current_level = cmdline_args.current
        self.current_module_name = f"framework_compatibility_matrix.{self.current_level}.xml"
        self.current_xml = self.interfaces_dir / f"compatibility_matrices/compatibility_matrix.{self.current_level}.xml"

        self.next_level = cmdline_args.next
        self.next_module_name = f"framework_compatibility_matrix.{self.next_level}.xml"
        self.next_xml = self.interfaces_dir / f"compatibility_matrices/compatibility_matrix.{self.next_level}.xml"

        self.level_to_letter = self.get_level_to_letter_mapping()
        print("Found level mapping in libvintf Level.h:", self.level_to_letter)

    def run(self):
        self.bump_kernel_configs()
        self.copy_matrix()
        self.edit_android_bp()
        self.edit_android_mk()

    def get_level_to_letter_mapping(self):
        levels_file = self.top / "system/libvintf/include/vintf/Level.h"
        with open(levels_file) as f:
            lines = f.readlines()
            pairs = [
                line.split("=", maxsplit=2) for line in lines if "=" in line
            ]
            return {
                level.strip().removesuffix(","): letter.strip()
                for letter, level in pairs
            }

    def bump_kernel_configs(self):
        check_call([
            self.top / "kernel/configs/tools/bump.py",
            self.level_to_letter[self.current_level].lower(),
            self.level_to_letter[self.next_level].lower(),
        ])

    def copy_matrix(self):
        shutil.copyfile(self.current_xml, self.next_xml)

    def edit_android_bp(self):
        android_bp = self.interfaces_dir / "compatibility_matrices/Android.bp"

        with open(android_bp, "r+") as f:
            if self.next_module_name not in f.read():
                f.seek(0, 2)  # end of file
                f.write("\n")
                f.write(
                    textwrap.dedent(f"""\
                        vintf_compatibility_matrix {{
                            name: "{self.next_module_name}",
                        }}
                    """))

        next_kernel_configs = check_output(
            """grep -rh name: | sed -E 's/^.*"(.*)".*/\\1/g'""",
            cwd=self.top / "kernel/configs" /
            self.level_to_letter[self.next_level].lower(),
            text=True,
            shell=True,
        ).splitlines()
        print(next_kernel_configs)

        check_call([
            "bpmodify", "-w", "-m", self.next_module_name, "-property", "stem",
            "-str", self.next_xml.name, android_bp
        ])

        check_call([
            "bpmodify", "-w", "-m", self.next_module_name, "-property", "srcs",
            "-a",
            self.next_xml.relative_to(android_bp.parent), android_bp
        ])

        check_call([
            "bpmodify", "-w", "-m", self.next_module_name, "-property",
            "kernel_configs", "-a", " ".join(next_kernel_configs), android_bp
        ])

    def edit_android_mk(self):
        android_mk = self.interfaces_dir / "compatibility_matrices/Android.mk"
        with open(android_mk) as f:
            if self.next_module_name in f.read():
                return
            f.seek(0)
            lines = f.readlines()
        current_module_line_number = None
        for line_number, line in enumerate(lines):
            if self.current_module_name in line:
                current_module_line_number = line_number
                break
        assert current_module_line_number is not None
        lines.insert(current_module_line_number + 1,
                     f"    {self.next_module_name} \\\n")
        with open(android_mk, "w") as f:
            f.write("".join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("current",
                        type=str,
                        help="VINTF level of the current version (e.g. 9)")
    parser.add_argument("next",
                        type=str,
                        help="VINTF level of the next version (e.g. 10)")
    cmdline_args = parser.parse_args()

    Bump(cmdline_args).run()


if __name__ == "__main__":
    main()
