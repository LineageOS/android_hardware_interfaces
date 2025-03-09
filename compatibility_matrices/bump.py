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
"""

import argparse
import os
import pathlib
import re
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

        self.current_level = cmdline_args.current_level
        self.current_letter = cmdline_args.current_letter
        self.current_version = cmdline_args.platform_version
        self.current_module_name = f"framework_compatibility_matrix.{self.current_level}.xml"
        self.current_xml = self.interfaces_dir / f"compatibility_matrices/compatibility_matrix.{self.current_level}.xml"
        self.device_module_name = "framework_compatibility_matrix.device.xml"

        self.next_level = cmdline_args.next_level
        self.next_letter = cmdline_args.next_letter
        self.next_module_name = f"framework_compatibility_matrix.{self.next_level}.xml"
        self.next_xml = self.interfaces_dir / f"compatibility_matrices/compatibility_matrix.{self.next_level}.xml"

    def run(self):
        self.bump_kernel_configs()
        self.copy_matrix()
        self.edit_android_bp()
        self.bump_libvintf()

    def bump_kernel_configs(self):
        check_call([
            self.top / "kernel/configs/tools/bump.py",
            self.current_letter.lower(),
            self.next_letter.lower(),
        ])

    def copy_matrix(self):
        with open(self.current_xml) as f_current, open(self.next_xml, "w") as f_next:
            f_next.write(f_current.read().replace(f"level=\"{self.current_level}\"", f"level=\"{self.next_level}\""))

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
            self.next_letter.lower(),
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

        # update the SYSTEM_MATRIX_DEPS variable and the phony module's
        # product_variables entry.
        lines = []
        with open(android_bp) as f:
            for line in f:
              if f"    \"{self.device_module_name}\",\n" in line:
                  lines.append(f"    \"{self.current_module_name}\",\n")

              if f"                \"{self.current_module_name}\",\n" in line:
                  lines.append(f"                \"{self.next_module_name}\",\n")
              else:
                  lines.append(line)

        with open(android_bp, "w") as f:
            f.write("".join(lines))

    def bump_libvintf(self):
        if not self.current_version:
            print("Skip libvintf update...")
            return
        try:
            check_call(["grep", "-h",
                        f"{self.current_letter.upper()} = {self.current_level}",
                        "system/libvintf/include/vintf/Level.h"])
        except subprocess.CalledProcessError:
            print("Adding new API level to libvintf")
            add_lines_above("system/libvintf/analyze_matrix/analyze_matrix.cpp",
                            "        case Level::UNSPECIFIED:",
                            textwrap.indent(textwrap.dedent(f"""\
                                    case Level::{self.current_letter.upper()}:
                                        return "Android {self.current_version} ({self.current_letter.upper()})";"""),
                            "    "*2))
            add_lines_above("system/libvintf/include/vintf/Level.h",
                            "    // To add new values:",
                            f"    {self.current_letter.upper()} = {self.current_level},")
            add_lines_above("system/libvintf/include/vintf/Level.h",
                            "        Level::UNSPECIFIED,",
                            f"        Level::{self.current_letter.upper()},")
            add_lines_above("system/libvintf/RuntimeInfo.cpp",
                            "            // Add more levels above this line.",
                            textwrap.indent(textwrap.dedent(f"""\
                                        case {self.current_version}: {{
                                            ret = Level::{self.current_letter.upper()};
                                        }} break;"""),
                            "    "*3))


def add_lines_above(file, pattern, lines):
    with open(file, 'r+') as f:
        text = f.read()
        split_text = re.split(rf"\n{pattern}\n", text)
        if len(split_text) != 2:
            # Only one pattern must be found, otherwise the source must be
            # changed unexpectedly.
            raise Exception(
                f'Pattern "{pattern}" not found or multiple patterns found in {file}')
        f.seek(0)
        f.write(f"\n{lines}\n{pattern}\n".join(split_text))
        f.truncate()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("current_level",
                        type=str,
                        help="VINTF level of the current version (e.g. 202404)")
    parser.add_argument("next_level",
                        type=str,
                        help="VINTF level of the next version (e.g. 202504)")
    parser.add_argument("current_letter",
                        type=str,
                        help="Letter of the API level of the current version (e.g. v)")
    parser.add_argument("next_letter",
                        type=str,
                        help="Letter of the API level of the next version (e.g. w)")
    parser.add_argument("platform_version",
                        type=str,
                        nargs="?",
                        help="Android release version number number (e.g. 15)")
    cmdline_args = parser.parse_args()

    Bump(cmdline_args).run()


if __name__ == "__main__":
    main()
