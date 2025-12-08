/*
 * Copyright (C) 2024 The Android Open Source Project
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
#pragma once

#include <android-base/macros.h>
#include <android-base/thread_annotations.h>

#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace android::hardware::radio::minimal::sim {

class Filesystem {
  public:
    /** 3GPP TS 27.007 8.18 */
    struct Path {
        int32_t fileId;
        std::string pathId;
        auto operator<=>(const Path&) const = default;
        std::string toString() const;
    };

    typedef std::span<uint8_t const> FileView;

  private:
    mutable std::mutex mFilesGuard;
    std::map<Path, std::vector<uint8_t>> mFiles GUARDED_BY(mFilesGuard);
    std::set<int32_t> mUpdates GUARDED_BY(mFilesGuard);

    DISALLOW_COPY_AND_ASSIGN(Filesystem);

  public:
    Filesystem();

    void write(const Path& path, FileView contents);
    void write(const Path& path, std::string_view contents);
    void write(const Path& path, std::vector<uint8_t>&& contents);
    std::optional<FileView> read(const Path& path) const;

    void writeBch(const Path& path, std::string_view contents);
    std::optional<std::string> readBch(const Path& path) const;

    std::optional<Path> find(uint16_t fileId);

    std::set<int32_t> fetchAndClearUpdates();
};

namespace paths {

extern const Filesystem::Path mf;
extern const Filesystem::Path fplmn;
extern const Filesystem::Path iccid;
extern const Filesystem::Path msisdn;
extern const Filesystem::Path pl;
extern const Filesystem::Path arr;
extern const Filesystem::Path ad;

}  // namespace paths

}  // namespace android::hardware::radio::minimal::sim
