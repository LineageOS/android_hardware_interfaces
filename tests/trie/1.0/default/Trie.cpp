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

#define LOG_TAG "hidl_test"

#include "Trie.h"
#include <android-base/logging.h>
#include <inttypes.h>
#include <string>

namespace android {
namespace hardware {
namespace tests {
namespace trie {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::trie::V1_0::ITrie follow.
Return<void> Trie::newTrie(newTrie_cb _hidl_cb) {
    LOG(INFO) << "SERVER(Trie) newTrie()";

    TrieNode ret;
    ret.isTerminal = false;
    _hidl_cb(ret);
    return Void();
}

Return<void> Trie::addStrings(const TrieNode& trie, const hidl_vec<hidl_string>& strings,
                              addStrings_cb _hidl_cb) {
    LOG(INFO) << "SERVER(Trie) addStrings(trie, " << strings.size() << " strings)";

    // Make trie modifiable.
    TrieNode newTrie = trie;

    for (const auto& str : strings) {
        addString(&newTrie, str);
    }
    _hidl_cb(newTrie);
    return Void();
}

Return<void> Trie::containsStrings(const TrieNode& trie, const hidl_vec<hidl_string>& strings,
                                   containsStrings_cb _hidl_cb) {
    LOG(INFO) << "SERVER(Trie) containsStrings(trie, " << strings.size() << " strings)";

    std::vector<bool> ret(strings.size());
    for (size_t i = 0; i != strings.size(); ++i) {
        ret[i] = containsString(&trie, strings[i]);
    }
    _hidl_cb(ret);
    return Void();
}

void Trie::addString(TrieNode* trieRoot, const std::string& str) {
    TrieNode* currNode = trieRoot;

    for (char ch : str) {
        auto& vec = currNode->next;

        auto it = std::find_if(vec.begin(), vec.end(),
                               [&](const TrieEdge& edge) { return ch == edge.character; });

        if (it == vec.end()) {
            vec.resize(vec.size() + 1);
            it = vec.end() - 1;
            it->character = ch;
            it->node.isTerminal = false;
        }

        currNode = &(it->node);
    }

    currNode->isTerminal = true;
}

bool Trie::containsString(const TrieNode* trieRoot, const std::string& str) {
    const TrieNode* currNode = trieRoot;

    for (char ch : str) {
        const auto& vec = currNode->next;

        auto it = std::find_if(vec.begin(), vec.end(),
                               [&](const TrieEdge& edge) { return ch == edge.character; });

        if (it == vec.end()) return false;
        currNode = &(it->node);
    }

    return currNode->isTerminal;
}

ITrie* HIDL_FETCH_ITrie(const char* /* name */) {
    return new Trie();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace trie
}  // namespace tests
}  // namespace hardware
}  // namespace android
