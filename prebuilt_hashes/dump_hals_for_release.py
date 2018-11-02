#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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
Dump new HIDL types that are introduced in each dessert release.
"""

from __future__ import print_function

import argparse
import collections
import json
import os
import re

class Globals:
    pass

class Constants:
    CURRENT = 'current'
    HAL_PATH_PATTERN = r'/((?:[a-zA-Z_]+/)*)(\d+\.\d+)/([a-zA-Z_]+).hal'
    CURRENT_TXT_PATTERN = r'(?:.*/)?([0-9]+|current).txt'

def trim_trailing_comments(line):
    idx = line.find('#')
    if idx < 0: return line
    return line[0:idx]

def strip_begin(s, prefix):
    if s.startswith(prefix):
        return strip_begin(s[len(prefix):], prefix)
    return s

def strip_end(s, suffix):
    if s.endswith(suffix):
        return strip_end(s[0:-len(suffix)], suffix)
    return s

def get_interfaces(file_name):
    with open(file_name) as file:
        for line in file:
            line_tokens = trim_trailing_comments(line).strip().split()
            if not line_tokens:
                continue
            assert len(line_tokens) == 2, \
                "Unrecognized line in file {}:\n{}".format(file_name, line)
            yield line_tokens[1]

def api_level_to_int(api_level):
    try:
        if api_level == Constants.CURRENT: return float('inf')
        return int(api_level)
    except ValueError:
        return None

def get_interfaces_from_package_root(package, root):
    root = strip_end(root, '/')
    for dirpath, _, filenames in os.walk(root, topdown=False):
        dirpath = strip_begin(dirpath, root)
        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            mo = re.match(Constants.HAL_PATH_PATTERN, filepath)
            if not mo:
                continue
            yield '{}.{}@{}::{}'.format(
                package, mo.group(1).strip('/').replace('/', '.'), mo.group(2), mo.group(3))

def filter_out(iterable):
    return iterable if not Globals.filter_out else filter(
        lambda s: all(re.search(pattern, s) is None for pattern in Globals.filter_out),
        iterable)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pretty', help='Print pretty JSON', action='store_true')
    parser.add_argument('--package-root', metavar='PACKAGE:PATH', nargs='*',
        help='package root of current directory, e.g. android.hardware:hardware/interfaces')
    parser.add_argument('--filter-out', metavar='REGEX', nargs='*',
        help='A regular expression that filters out interfaces.')
    parser.add_argument('hashes', metavar='FILE', nargs='*',
        help='Locations of current.txt for each release.')
    parser.parse_args(namespace=Globals)

    interfaces_for_level = dict()

    for filename in Globals.hashes or tuple():
        mo = re.match(Constants.CURRENT_TXT_PATTERN, filename)
        assert mo is not None, \
            'Input hash file names must have the format {} but is {}'.format(Constants.CURRENT_TXT_PATTERN, filename)

        api_level = mo.group(1)
        assert api_level_to_int(api_level) is not None

        if api_level not in interfaces_for_level:
            interfaces_for_level[api_level] = set()
        interfaces_for_level[api_level].update(filter_out(get_interfaces(filename)))

    for package_root in Globals.package_root or tuple():
        tup = package_root.split(':')
        assert len(tup) == 2, \
            '--package-root must have the format PACKAGE:PATH, but is {}'.format(package_root)
        if Constants.CURRENT not in interfaces_for_level:
            interfaces_for_level[Constants.CURRENT] = set()
        interfaces_for_level[Constants.CURRENT].update(filter_out(get_interfaces_from_package_root(*tup)))

    seen_interfaces = set()
    new_interfaces_for_level = collections.OrderedDict()
    for level, interfaces in sorted(interfaces_for_level.items(), key=lambda tup: api_level_to_int(tup[0])):
        if level != Constants.CURRENT:
            removed_interfaces = seen_interfaces - interfaces
            assert not removed_interfaces, \
                "The following interfaces are removed from API level {}:\n{}".format(
                    level, removed_interfaces)
        new_interfaces_for_level[level] = sorted(interfaces - seen_interfaces)
        seen_interfaces.update(interfaces)

    print(json.dumps(new_interfaces_for_level,
        separators=None if Globals.pretty else (',',':'),
        indent=4 if Globals.pretty else None))

if __name__ == '__main__':
    main()
