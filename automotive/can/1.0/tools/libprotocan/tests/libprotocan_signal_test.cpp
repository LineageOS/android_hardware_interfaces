/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <libprotocan/Signal.h>

#include <gtest/gtest.h>

namespace android::hardware::automotive::protocan::unittest {

TEST(SignalTest, TestGetSingleBytes) {
  can::V1_0::CanMessage msg = {};
  msg.payload = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  for (unsigned i = 0; i < msg.payload.size(); i++) {
    Signal signal(8 * i, 8);
    ASSERT_EQ(i, signal.get(msg));
  }
}

TEST(SignalTest, TestSetSingleBytes) {
  std::vector<can::V1_0::CanMessage> msgs = {{}, {}, {}};
  msgs[0].payload = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  msgs[1].payload = {0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB};
  msgs[2].payload = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for (unsigned i = 0; i < msgs[0].payload.size(); i++) {
    Signal signal(8 * i, 8);

    for (auto&& msgOriginal : msgs) {
      auto msgModified = msgOriginal;
      signal.set(msgModified, 0xBA);

      auto msgExpected = msgOriginal;
      msgExpected.payload[i] = 0xBA;

      ASSERT_EQ(msgExpected, msgModified) << "i=" << i;
    }
  }
}

TEST(SignalTest, TestGetStart4) {
  /* Data generated with Python3:
   *
   * from cantools.database.can import *
   * hex(Message(1, 'm', 4, [Signal('s', 4, 16, byte_order='little_endian')]).
   *     decode(b'\xde\xad\xbe\xef')['s'])
   */

  can::V1_0::CanMessage msg = {};
  msg.payload = {0xDE, 0xAD, 0xBE, 0xEF};
  can::V1_0::CanMessage msg2 = {};
  msg2.payload = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD};

  Signal s0_4(0, 4);
  Signal s4_4(4, 4);
  Signal s4_8(4, 8);
  Signal s4_16(4, 16);
  Signal s4_28(4, 28);
  Signal s12_8(12, 8);
  Signal s12_12(12, 12);
  Signal s12_16(12, 16);
  Signal s12_20(12, 20);
  Signal s12_32(12, 32);

  ASSERT_EQ(0xEu, s0_4.get(msg));
  ASSERT_EQ(0xDu, s4_4.get(msg));
  ASSERT_EQ(0xDDu, s4_8.get(msg));
  ASSERT_EQ(0xEADDu, s4_16.get(msg));
  ASSERT_EQ(0xEFBEADDu, s4_28.get(msg));
  ASSERT_EQ(0xEAu, s12_8.get(msg));
  ASSERT_EQ(0xBEAu, s12_12.get(msg));
  ASSERT_EQ(0xFBEAu, s12_16.get(msg));
  ASSERT_EQ(0xEFBEAu, s12_20.get(msg));
  ASSERT_EQ(0xDDEEFBEAu, s12_32.get(msg2));
}

TEST(SignalTest, TestGet64) {
  /* Data generated with Python3:
   *
   * from cantools.database.can import *
   * hex(Message(1, 'm', 9, [Signal('s', 4, 64, byte_order='little_endian')]).
   *     decode(b'\xde\xad\xbe\xef\xab\xbc\xcd\xde\xef')['s'])
   */

  can::V1_0::CanMessage msg = {};
  msg.payload = {0xDE, 0xAD, 0xBE, 0xEF, 0xAB, 0xBC, 0xCD, 0xDE, 0xEF};

  Signal s0_64(0, 64);
  Signal s8_64(8, 64);
  Signal s4_64(4, 64);
  Signal s1_64(1, 64);

  ASSERT_EQ(0xDECDBCABEFBEADDEu, s0_64.get(msg));
  ASSERT_EQ(0xEFDECDBCABEFBEADu, s8_64.get(msg));
  ASSERT_EQ(0xFDECDBCABEFBEADDu, s4_64.get(msg));
  ASSERT_EQ(0xEF66DE55F7DF56EFu, s1_64.get(msg));
}

TEST(SignalTest, TestGetAllStarts) {
  /* Data generated with Python3:
   *
   * from cantools.database.can import *
   * hex(Message(1, 'm', 6, [Signal('s', 0, 20, byte_order='little_endian')]).
   *     decode(b'\xde\xad\xbe\xef\xde\xad')['s'])
   */

  std::map<int, Signal::value> shifts = {
      {0, 0xEADDEu}, {1, 0xF56EFu}, {2, 0xFAB77u}, {3, 0x7D5BBu}, {4, 0xBEADDu},  {5, 0xDF56Eu},
      {6, 0xEFAB7u}, {7, 0xF7D5Bu}, {8, 0xFBEADu}, {9, 0x7DF56u}, {10, 0xBEFABu}, {11, 0xDF7D5u},
  };

  can::V1_0::CanMessage msg = {};
  msg.payload = {0xDE, 0xAD, 0xBE, 0xEF, 0xCC, 0xCC};

  for (auto const& [start, expected] : shifts) {
    Signal s(start, 20);
    ASSERT_EQ(expected, s.get(msg)) << "shift of " << start << " failed";
  }
}

TEST(SignalTest, TestSetStart4) {
  /* Data generated with Python3:
   *
   * from cantools.database.can import *
   * so=4 ; sl=8
   * md = Message(1, 'm', 4, [Signal('a1', 0, so), Signal('a2', so+sl, 32-so-sl),
   *     Signal('s', so, sl, byte_order='little_endian')])
   * m = md.decode(b'\xcc\xcc\xcc\xcc')
   * m['s'] = 0xDE
   * binascii.hexlify(md.encode(m)).upper()
   */
  typedef struct {
    int start;
    int length;
    Signal::value setValue;
    hidl_vec<uint8_t> payload;
  } case_t;

  std::vector<case_t> cases = {
      {0, 4, 0xDu, {0xCD, 0xCC, 0xCC, 0xCC}},       {4, 4, 0xDu, {0xDC, 0xCC, 0xCC, 0xCC}},
      {4, 8, 0xDEu, {0xEC, 0xCD, 0xCC, 0xCC}},      {4, 16, 0xDEADu, {0xDC, 0xEA, 0xCD, 0xCC}},
      {4, 24, 0xDEADBEu, {0xEC, 0xDB, 0xEA, 0xCD}}, {4, 28, 0xDEADBEEu, {0xEC, 0xBE, 0xAD, 0xDE}},
      {12, 8, 0xDEu, {0xCC, 0xEC, 0xCD, 0xCC}},     {12, 12, 0xDEAu, {0xCC, 0xAC, 0xDE, 0xCC}},
      {12, 16, 0xDEADu, {0xCC, 0xDC, 0xEA, 0xCD}},  {12, 20, 0xDEADBu, {0xCC, 0xBC, 0xAD, 0xDE}},
  };

  can::V1_0::CanMessage msg = {};
  msg.payload = {0xCC, 0xCC, 0xCC, 0xCC};

  for (auto const& tcase : cases) {
    Signal s(tcase.start, tcase.length);

    can::V1_0::CanMessage expectedMsg = {};
    expectedMsg.payload = tcase.payload;

    can::V1_0::CanMessage editedMsg = msg;
    s.set(editedMsg, tcase.setValue);

    ASSERT_EQ(expectedMsg, editedMsg) << " set(" << tcase.start << ", " << tcase.length << ")";
  }
}

TEST(SignalTest, TestSetAllStarts) {
  /* Data generated with Python3:
   * from cantools.database.can import *
   * import binascii
   * import textwrap
   *
   * length = 20
   * for start in range(0, 32 - length):
   *     signals = [Signal('s', start, length, byte_order='little_endian')]
   *     if start > 0: signals.append(Signal('a', 0, start, byte_order='little_endian'))
   *     signals.append(Signal('b', start + length, 32 - start - length,
   *         byte_order='little_endian'))
   *
   *     md = Message(1, 'm', 4, signals)
   *     m = md.decode(b'\xcc\xcc\xcc\xcc')
   *     m['s'] = 0xDEADB
   *     out = binascii.hexlify(md.encode(m)).decode('ascii').upper()
   *     out = ', '.join(['0x{}'.format(v) for v in textwrap.wrap(out, 2)])
   *     print('{{ {:d}, {{ {:s} }}}},'.format(start, out))
   */

  std::map<int, hidl_vec<uint8_t>> shifts = {
      {0, {0xDB, 0xEA, 0xCD, 0xCC}}, {1, {0xB6, 0xD5, 0xDB, 0xCC}},  {2, {0x6C, 0xAB, 0xF7, 0xCC}},
      {3, {0xDC, 0x56, 0xEF, 0xCC}}, {4, {0xBC, 0xAD, 0xDE, 0xCC}},  {5, {0x6C, 0x5B, 0xBD, 0xCD}},
      {6, {0xCC, 0xB6, 0x7A, 0xCF}}, {7, {0xCC, 0x6D, 0xF5, 0xCE}},  {8, {0xCC, 0xDB, 0xEA, 0xCD}},
      {9, {0xCC, 0xB6, 0xD5, 0xDB}}, {10, {0xCC, 0x6C, 0xAB, 0xF7}}, {11, {0xCC, 0xDC, 0x56, 0xEF}},
  };

  can::V1_0::CanMessage msg = {};
  msg.payload = {0xCC, 0xCC, 0xCC, 0xCC};

  for (auto const& [start, expectedPayload] : shifts) {
    Signal s(start, 20);

    can::V1_0::CanMessage expectedMsg = {};
    expectedMsg.payload = expectedPayload;

    can::V1_0::CanMessage editedMsg = msg;
    s.set(editedMsg, 0xDEADB);

    ASSERT_EQ(expectedMsg, editedMsg) << "shift of " << start << " failed";
  }
}

}  // namespace android::hardware::automotive::protocan::unittest
