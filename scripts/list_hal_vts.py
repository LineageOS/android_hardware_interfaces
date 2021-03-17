#!/usr/bin/python3

"""
List VTS tests for each HAL by parsing module-info.json.

Example usage:

  # First, build modules-info.json
  m -j "${ANDROID_PRODUCT_OUT#$ANDROID_BUILD_TOP/}/module-info.json"

  # List with pretty-printed JSON. *IDL packages without a VTS module will show up
  # as keys with empty lists.
  ./list_hals_vts.py | python3 -m json.tool

  # List with CSV. *IDL packages without a VTS module will show up as a line with
  # empty value in the VTS module column.
  ./list_hals_vts.py --csv
"""

import argparse
import collections
import csv
import io
import json
import os
import logging
import pathlib
import re
import sys

PATH_PACKAGE_PATTERN = re.compile(
  r'^hardware/interfaces/(?P<path>(?:\w+/)*?)(?:aidl|(?P<version>\d+\.\d+))/.*')


class CriticalHandler(logging.StreamHandler):
  def emit(self, record):
    super(CriticalHandler, self).emit(record)
    if record.levelno >= logging.CRITICAL:
      sys.exit(1)


logger = logging.getLogger(__name__)
logger.addHandler(CriticalHandler())


def default_json():
  out = os.environ.get('ANDROID_PRODUCT_OUT')
  if not out: return None
  return os.path.join(out, 'module-info.json')


def infer_package(path):
  """
  Infer package from a relative path from build top where a VTS module lives.

  :param path: a path like 'hardware/interfaces/vibrator/aidl/vts'
  :return: The inferred *IDL package, e.g. 'android.hardware.vibrator'

  >>> infer_package('hardware/interfaces/automotive/sv/1.0/vts/functional')
  'android.hardware.automotive.sv@1.0'
  >>> infer_package('hardware/interfaces/vibrator/aidl/vts')
  'android.hardware.vibrator'
  """
  mo = re.match(PATH_PACKAGE_PATTERN, path)
  if not mo: return None
  package = 'android.hardware.' + ('.'.join(pathlib.Path(mo.group('path')).parts))
  if mo.group('version'):
    package += '@' + mo.group('version')
  return package


def load_modules_info(json_file):
  """
  :param json_file: The path to modules-info.json
  :return: a dictionary, where the keys are inferred *IDL package names, and
           values are a list of VTS modules with that inferred package name.
  """
  with open(json_file) as fp:
    root = json.load(fp)
    ret = collections.defaultdict(list)
    for module_name, module_info in root.items():
      if 'vts' not in module_info.get('compatibility_suites', []):
        continue
      for path in module_info.get('path', []):
        inferred_package = infer_package(path)
        if not inferred_package:
          continue
        ret[inferred_package].append(module_name)
    return ret


def add_missing_idl(vts_modules):
  top = os.environ.get("ANDROID_BUILD_TOP")
  interfaces = None
  if top:
    interfaces = os.path.join(top, "hardware", "interfaces")
  else:
    logger.warning("Missing ANDROID_BUILD_TOP")
    interfaces = "hardware/interfaces"
  if not os.path.isdir(interfaces):
    logger.error("Not adding missing *IDL modules because missing hardware/interfaces dir")
    return
  assert not interfaces.endswith(os.path.sep)
  for root, dirs, files in os.walk(interfaces):
    for dir in dirs:
      full_dir = os.path.join(root, dir)
      assert full_dir.startswith(interfaces)
      top_rel_dir = os.path.join('hardware', 'interfaces', full_dir[len(interfaces) + 1:])
      inferred_package = infer_package(top_rel_dir)
      if inferred_package is None:
        continue
      if inferred_package not in vts_modules:
        vts_modules[inferred_package] = []


def main():
  parser = argparse.ArgumentParser(__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument('json', metavar='module-info.json', default=default_json(), nargs='?')
  parser.add_argument('--csv', action='store_true', help='Print CSV. If not specified print JSON.')
  args = parser.parse_args()
  if not args.json:
    logger.critical('No module-info.json is specified or found.')
  vts_modules = load_modules_info(args.json)
  add_missing_idl(vts_modules)

  if args.csv:
    out = io.StringIO()
    writer = csv.writer(out, )
    writer.writerow(["package", "vts_module"])
    for package, modules in vts_modules.items():
      if not modules:
        writer.writerow([package, ""])
      for module in modules:
        writer.writerow([package, module])
    result = out.getvalue()
  else:
    result = json.dumps(vts_modules)

  print(result)


if __name__ == '__main__':
  main()
