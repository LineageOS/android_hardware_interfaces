/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.bluetooth.service.default"

#include "net_bluetooth_mgmt.h"

#include <fcntl.h>
#include <log/log.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// Definitions imported from <linux/net/bluetooth/bluetooth.h>
#define BTPROTO_HCI 1

// Definitions imported from <linux/net/bluetooth/hci_sock.h>
#define HCI_CHANNEL_USER 1
#define HCI_CHANNEL_CONTROL 3
#define HCI_DEV_NONE 0xffff

struct sockaddr_hci {
  sa_family_t hci_family;
  unsigned short hci_dev;
  unsigned short hci_channel;
};

// Definitions imported from <linux/net/bluetooth/mgmt.h>
#define MGMT_OP_READ_INDEX_LIST 0x0003
#define MGMT_EV_INDEX_ADDED 0x0004
#define MGMT_EV_CMD_COMPLETE 0x0001
#define MGMT_PKT_SIZE_MAX 1024
#define MGMT_INDEX_NONE 0xFFFF

struct mgmt_pkt {
  uint16_t opcode;
  uint16_t index;
  uint16_t len;
  uint8_t data[MGMT_PKT_SIZE_MAX];
} __attribute__((packed));

struct mgmt_ev_read_index_list {
  uint16_t opcode;
  uint8_t status;
  uint16_t num_controllers;
  uint16_t index[];
} __attribute__((packed));

// Definitions imported from <linux/rfkill.h>
#define RFKILL_STATE_SOFT_BLOCKED 0
#define RFKILL_STATE_UNBLOCKED 1
#define RFKILL_STATE_HARD_BLOCKED 2

#define RFKILL_TYPE_BLUETOOTH 2

#define RFKILL_OP_ADD 0
#define RFKILL_OP_CHANGE 2

struct rfkill_event {
  uint32_t idx;
  uint8_t type;
  uint8_t op;
  uint8_t soft;
  uint8_t hard;
} __attribute__((packed));

namespace aidl::android::hardware::bluetooth::impl {

// Wait indefinitely for the selected HCI interface to be enabled in the
// bluetooth driver.
int NetBluetoothMgmt::waitHciDev(int hci_interface) {
  ALOGI("waiting for hci interface %d", hci_interface);

  int ret = -1;
  struct mgmt_pkt cmd;
  struct pollfd pollfd;
  struct sockaddr_hci hci_addr = {
      .hci_family = AF_BLUETOOTH,
      .hci_dev = HCI_DEV_NONE,
      .hci_channel = HCI_CHANNEL_CONTROL,
  };

  // Open and bind a socket to the bluetooth control interface in the
  // kernel driver, used to send control commands and receive control
  // events.
  int fd = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
  if (fd < 0) {
    ALOGE("unable to open raw bluetooth socket: %s", strerror(errno));
    return -1;
  }

  if (bind(fd, (struct sockaddr*)&hci_addr, sizeof(hci_addr)) < 0) {
    ALOGE("unable to bind bluetooth control channel: %s", strerror(errno));
    goto end;
  }

  // Send the control command [Read Index List].
  cmd = {
      .opcode = MGMT_OP_READ_INDEX_LIST,
      .index = MGMT_INDEX_NONE,
      .len = 0,
  };

  if (write(fd, &cmd, 6) != 6) {
    ALOGE("error writing mgmt command: %s", strerror(errno));
    goto end;
  }

  // Poll the control socket waiting for the command response,
  // and subsequent [Index Added] events. The loops continue without
  // timeout until the selected hci interface is detected.
  pollfd = {.fd = fd, .events = POLLIN};

  for (;;) {
    ret = poll(&pollfd, 1, -1);

    // Poll interrupted, try again.
    if (ret == -1 && (errno == EINTR || errno == EAGAIN)) {
      continue;
    }

    // Poll failure, abandon.
    if (ret == -1) {
      ALOGE("poll error: %s", strerror(errno));
      break;
    }

    // Spurious wakeup, try again.
    if (ret == 0 || (pollfd.revents & POLLIN) == 0) {
      continue;
    }

    // Read the next control event.
    struct mgmt_pkt ev {};
    ret = read(fd, &ev, sizeof(ev));
    if (ret < 0) {
      ALOGE("error reading mgmt event: %s", strerror(errno));
      goto end;
    }

    // Received [Read Index List] command response.
    if (ev.opcode == MGMT_EV_CMD_COMPLETE) {
      struct mgmt_ev_read_index_list* data =
          (struct mgmt_ev_read_index_list*)ev.data;

      for (int i = 0; i < data->num_controllers; i++) {
        if (data->index[i] >= hci_interface) {
          ALOGI("hci interface %d found", data->index[i]);
          ret = data->index[i];
          goto end;
        }
      }
    }

    // Received [Index Added] event.
    if (ev.opcode == MGMT_EV_INDEX_ADDED && ev.index == hci_interface) {
      ALOGI("hci interface %d added", hci_interface);
      ret = 0;
      goto end;
    }
  }

end:
  ::close(fd);
  return ret;
}

int NetBluetoothMgmt::openRfkill() {
  int fd = open("/dev/rfkill", O_RDWR);
  if (fd < 0) {
    ALOGE("unable to open /dev/rfkill: %s", strerror(errno));
    return -1;
  }

  if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
    ALOGE("unable to set rfkill control device to non-blocking: %s",
          strerror(errno));
    ::close(fd);
    return -1;
  }

  for (;;) {
    struct rfkill_event event {};
    ssize_t res = read(fd, &event, sizeof(event));
    if (res < 0) {
      ALOGE("error reading rfkill events: %s", strerror(errno));
      break;
    }

    ALOGI("index:%d type:%d op:%d", event.idx, event.type, event.op);

    if (event.op == RFKILL_OP_ADD && event.type == RFKILL_TYPE_BLUETOOTH) {
      rfkill_bt_index_ = event.idx;
      rfkill_fd_ = fd;
      return 0;
    }
  }

  ::close(fd);
  return -1;
}

// Block or unblock Bluetooth.
int NetBluetoothMgmt::rfkill(int block) {
  if (rfkill_fd_ == -1) {
    openRfkill();
  }

  if (rfkill_fd_ == -1) {
    ALOGE("rfkill unavailable");
    return -1;
  }

  struct rfkill_event event = {
      .idx = static_cast<uint32_t>(rfkill_bt_index_),
      .type = RFKILL_TYPE_BLUETOOTH,
      .op = RFKILL_OP_CHANGE,
      .soft = static_cast<uint8_t>(block),
      .hard = 0,
  };

  int res = write(rfkill_fd_, &event, sizeof(event));
  if (res < 0) {
    ALOGE("error writing rfkill command: %s", strerror(errno));
    return -1;
  }

  return 0;
}

int NetBluetoothMgmt::openHci(int hci_interface) {
  ALOGI("opening hci interface %d", hci_interface);

  // Block Bluetooth.
  rfkill(1);

  // Wait for the HCI interface to complete initialization or to come online.
  hci_interface = waitHciDev(hci_interface);
  if (hci_interface < 0) {
    ALOGE("hci interface not found");
    return -1;
  }

  // Open the raw HCI socket.
  int fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
  if (fd < 0) {
    ALOGE("unable to open raw bluetooth socket: %s", strerror(errno));
    return -1;
  }

  struct sockaddr_hci hci_addr = {
      .hci_family = AF_BLUETOOTH,
      .hci_dev = static_cast<uint16_t>(hci_interface),
      .hci_channel = HCI_CHANNEL_USER,
  };

  // Bind the socket to the selected interface.
  if (bind(fd, (struct sockaddr*)&hci_addr, sizeof(hci_addr)) < 0) {
    ALOGE("unable to bind bluetooth user channel: %s", strerror(errno));
    ::close(fd);
    return -1;
  }

  ALOGI("hci interface %d ready", hci_interface);
  bt_fd_ = fd;
  return fd;
}

void NetBluetoothMgmt::closeHci() {
  if (bt_fd_ != -1) {
    ::close(bt_fd_);
    bt_fd_ = -1;
  }

  // Unblock Bluetooth.
  rfkill(0);
}

}  // namespace aidl::android::hardware::bluetooth::impl
