//
// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "bluetooth_address.h"

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

void BluetoothAddress::bytes_to_string(const uint8_t* addr, char* addr_str) {
  sprintf(addr_str, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2],
          addr[3], addr[4], addr[5]);
}

bool BluetoothAddress::string_to_bytes(const char* addr_str, uint8_t* addr) {
  if (addr_str == NULL) return false;
  if (strnlen(addr_str, kStringLength) != kStringLength) return false;
  unsigned char trailing_char = '\0';

  return (sscanf(addr_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%1c",
                 &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5],
                 &trailing_char) == kBytes);
}

bool BluetoothAddress::get_local_address(uint8_t* local_addr) {
  char property[PROPERTY_VALUE_MAX] = {0};
  bool valid_bda = false;

  // Get local bdaddr storage path from a system property.
  if (property_get(PROPERTY_BT_BDADDR_PATH, property, NULL)) {
    int addr_fd;

    ALOGD("%s: Trying %s", __func__, property);

    addr_fd = open(property, O_RDONLY);
    if (addr_fd != -1) {
      int bytes_read = read(addr_fd, property, kStringLength);
      CHECK(bytes_read == kStringLength);
      close(addr_fd);

      // Null terminate the string.
      property[kStringLength] = '\0';

      // If the address is not all zeros, then use it.
      const uint8_t zero_bdaddr[kBytes] = {0, 0, 0, 0, 0, 0};
      if ((string_to_bytes(property, local_addr)) &&
          (memcmp(local_addr, zero_bdaddr, kBytes) != 0)) {
        valid_bda = true;
        ALOGD("%s: Got Factory BDA %s", __func__, property);
      }
    }
  }

  // No BDADDR found in the file. Look for BDA in a factory property.
  if (!valid_bda && property_get(FACTORY_BDADDR_PROPERTY, property, NULL) &&
      string_to_bytes(property, local_addr)) {
    valid_bda = true;
  }

  // No factory BDADDR found. Look for a previously stored BDA.
  if (!valid_bda && property_get(PERSIST_BDADDR_PROPERTY, property, NULL) &&
      string_to_bytes(property, local_addr)) {
    valid_bda = true;
  }

  return valid_bda;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
