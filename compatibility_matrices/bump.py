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

        self.current_level = cmdline_args.current_level
        self.current_letter = cmdline_args.current_letter
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
        self.edit_android_mk()

    def bump_kernel_configs(self):
        check_call([
            self.top / "kernel/configs/tools/bump.py",
            self.current_letter,
            self.next_letter,
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
            self.next_letter,
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
        lines = []
        with open(android_mk) as f:
            if self.next_module_name in f.read():
                return
            f.seek(0)
            for line in f:
              if f"    {self.device_module_name} \\\n" in line:
                  lines.append(f"    {self.current_module_name} \\\n")

              if self.current_module_name in line:
                  lines.append(f"    {self.next_module_name} \\\n")
              else:
                  lines.append(line)

        with open(android_mk, "w") as f:
            f.write("".join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("current_level",
                        type=str,
                        help="VINTF level of the current version (e.g. 9)")
    parser.add_argument("next_level",
                        type=str,
                        help="VINTF level of the next version (e.g. 10)")
    parser.add_argument("current_letter",
                        type=str,
                        help="Letter of the API level of the current version (e.g. v)")
    parser.add_argument("next_letter",
                        type=str,
                        help="Letter of the API level of the next version (e.g. w)")
    cmdline_args = parser.parse_args()

    Bump(cmdline_args).run()


if __name__ == "__main__":
    main()
