/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_TESTS_TRIE_V1_0_TRIE_H
#define ANDROID_HARDWARE_TESTS_TRIE_V1_0_TRIE_H

#include <android/hardware/tests/trie/1.0/ITrie.h>
#include <hidl/Status.h>

#include <string>

namespace android {
namespace hardware {
namespace tests {
namespace trie {
namespace V1_0 {
namespace implementation {

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::tests::trie::V1_0::ITrie;
using ::android::hardware::tests::trie::V1_0::TrieNode;

struct Trie : public ITrie {
    // Methods from ::android::hardware::tests::trie::V1_0::ITrie follow.
    virtual Return<void> newTrie(newTrie_cb _hidl_cb) override;
    virtual Return<void> addStrings(const TrieNode& trie, const hidl_vec<hidl_string>& strings,
                                    addStrings_cb _hidl_cb) override;
    virtual Return<void> containsStrings(const TrieNode& trie, const hidl_vec<hidl_string>& strings,
                                         containsStrings_cb _hidl_cb) override;

   private:
    void addString(TrieNode* trieRoot, const std::string& str);
    bool containsString(const TrieNode* trieRoot, const std::string& str);
};

extern "C" ITrie* HIDL_FETCH_ITrie(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace trie
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TESTS_TRIE_V1_0_TRIE_H
