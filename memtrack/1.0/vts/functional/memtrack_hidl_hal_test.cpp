/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "memtrack_hidl_hal_test"
#include <android-base/logging.h>
#include <android/hardware/memtrack/1.0/IMemtrack.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

using ::android::hardware::memtrack::V1_0::IMemtrack;
using ::android::hardware::memtrack::V1_0::MemtrackRecord;
using ::android::hardware::memtrack::V1_0::MemtrackFlag;
using ::android::hardware::memtrack::V1_0::MemtrackType;
using ::android::hardware::memtrack::V1_0::MemtrackStatus;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::sp;
using std::vector;
using std::count_if;

class MemtrackHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    memtrack = IMemtrack::getService("memtrack");
    ASSERT_NE(memtrack, nullptr);
  }

  virtual void TearDown() override {}

  sp<IMemtrack> memtrack;
};

/* Returns true if flags contains at least min, and no more than max,
 * of the flags in flagSet. Returns false otherwise.
 */
bool rightFlagCount(uint32_t flags, vector<MemtrackFlag> flagSet, uint32_t min,
                    uint32_t max) {
  uint32_t count =
      count_if(flagSet.begin(), flagSet.end(),
               [&](MemtrackFlag f) { return flags & (uint32_t)f; });
  return (min <= count && count <= max);
}

/* Returns true when passed a valid, defined status, false otherwise.
 */
bool validStatus(MemtrackStatus s) {
  vector<MemtrackStatus> statusVec = {
      MemtrackStatus::SUCCESS, MemtrackStatus::MEMORY_TRACKING_NOT_SUPPORTED,
      MemtrackStatus::TYPE_NOT_SUPPORTED};
  return std::find(statusVec.begin(), statusVec.end(), s) != statusVec.end();
}

/* Returns a pid found in /proc for which the string read from
 * /proc/[pid]/cmdline matches cmd, or -1 if no such pid exists.
 */
pid_t getPidFromCmd(const char cmd[], uint32_t len) {
  const char procs[] = "/proc/";
  DIR *dir = opendir(procs);
  if (!dir) {
    return -1;
  }

  struct dirent *proc;
  while ((proc = readdir(dir)) != NULL) {
    if (!isdigit(proc->d_name[0])) {
      continue;
    }
    char line[len];
    char fname[PATH_MAX];
    snprintf(fname, PATH_MAX, "/proc/%s/cmdline", proc->d_name);

    FILE *file = fopen(fname, "r");
    if (!file) {
      continue;
    }
    char *str = fgets(line, len, file);
    fclose(file);
    if (!str || strcmp(str, cmd)) {
      continue;
    } else {
      closedir(dir);
      return atoi(proc->d_name);
    }
  }
  closedir(dir);
  return -1;
}

auto generate_cb(MemtrackStatus *s, hidl_vec<MemtrackRecord> *v) {
  return [=](MemtrackStatus status, hidl_vec<MemtrackRecord> vec) {
    *s = status;
    *v = vec;
  };
}

/* Sanity check results when getMemory() is passed a negative PID
 */
TEST_F(MemtrackHidlTest, BadPidTest) {
  MemtrackStatus s;
  hidl_vec<MemtrackRecord> v;
  auto cb = generate_cb(&s, &v);
  for (uint32_t i = 0; i < static_cast<uint32_t>(MemtrackType::NUM_TYPES);
       i++) {
    Return<void> ret =
        memtrack->getMemory(-1, static_cast<MemtrackType>(i), cb);
    ASSERT_TRUE(ret.isOk());
    ASSERT_TRUE(validStatus(s));
  }
}

/* Sanity check results when getMemory() is passed a bad memory usage type
 */
TEST_F(MemtrackHidlTest, BadTypeTest) {
  MemtrackStatus s;
  hidl_vec<MemtrackRecord> v;
  auto cb = generate_cb(&s, &v);
  Return<void> ret = memtrack->getMemory(getpid(), MemtrackType::NUM_TYPES, cb);
  ASSERT_TRUE(ret.isOk());
  ASSERT_TRUE(validStatus(s));
}

/* Call memtrack on the surfaceflinger process and check that the results are
 * reasonable for all memory types, including valid flag combinations for
 * every MemtrackRecord returned.
 */
TEST_F(MemtrackHidlTest, SurfaceflingerTest) {
  const char cmd[] = "/system/bin/surfaceflinger";
  const uint32_t len = sizeof(cmd);
  pid_t pid = getPidFromCmd(cmd, len);
  ASSERT_LE(0, pid) << "Surfaceflinger process not found";

  MemtrackStatus s;
  hidl_vec<MemtrackRecord> v;
  auto cb = generate_cb(&s, &v);
  uint32_t unsupportedCount = 0;
  for (uint32_t i = 0; i < static_cast<uint32_t>(MemtrackType::NUM_TYPES);
       i++) {
    Return<void> ret =
        memtrack->getMemory(pid, static_cast<MemtrackType>(i), cb);
    ASSERT_TRUE(ret.isOk());

    switch (s) {
      case MemtrackStatus::MEMORY_TRACKING_NOT_SUPPORTED:
        unsupportedCount++;
        break;
      case MemtrackStatus::TYPE_NOT_SUPPORTED:
        break;
      case MemtrackStatus::SUCCESS: {
        for (uint32_t j = 0; j < v.size(); j++) {
          // Enforce flag constraints
          vector<MemtrackFlag> smapFlags = {MemtrackFlag::SMAPS_ACCOUNTED,
                                            MemtrackFlag::SMAPS_UNACCOUNTED};
          EXPECT_TRUE(rightFlagCount(v[j].flags, smapFlags, 1, 1));
          vector<MemtrackFlag> shareFlags = {MemtrackFlag::SHARED,
                                             MemtrackFlag::SHARED_PSS,
                                             MemtrackFlag::PRIVATE};
          EXPECT_TRUE(rightFlagCount(v[j].flags, shareFlags, 0, 1));
          vector<MemtrackFlag> systemFlags = {MemtrackFlag::SYSTEM,
                                              MemtrackFlag::DEDICATED};
          EXPECT_TRUE(rightFlagCount(v[j].flags, systemFlags, 0, 1));
          vector<MemtrackFlag> secureFlags = {MemtrackFlag::SECURE,
                                              MemtrackFlag::NONSECURE};
          EXPECT_TRUE(rightFlagCount(v[j].flags, secureFlags, 0, 1));
        }
        break;
      }
      default:
        FAIL();
    }
  }
  // If tracking is not supported this should be returned for all types.
  ASSERT_TRUE(unsupportedCount == 0 ||
              unsupportedCount ==
                  static_cast<uint32_t>(MemtrackType::NUM_TYPES));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
