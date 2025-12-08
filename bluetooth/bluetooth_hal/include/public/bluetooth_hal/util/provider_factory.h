/*
 * Copyright 2025 The Android Open Source Project
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

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace bluetooth_hal {
namespace util {

template <typename Key, typename Interface>
class MultiKeyProviderFactory {
 public:
  using FactoryFn = std::function<std::unique_ptr<Interface>()>;

  static std::unique_ptr<Interface> Create(const Key& key) {
    std::lock_guard<std::recursive_mutex> lock(factories_mutex_);
    auto it = provider_factories_.find(key);
    if (it != provider_factories_.end() && it->second) {
      return it->second();
    }
    return nullptr;
  }

  static void RegisterProviderFactory(const Key& key, FactoryFn factory) {
    std::lock_guard<std::recursive_mutex> lock(factories_mutex_);
    provider_factories_[key] = std::move(factory);
  }

  static void UnregisterProviderFactory(const Key& key) {
    std::lock_guard<std::recursive_mutex> lock(factories_mutex_);
    provider_factories_.erase(key);
  }

  static bool IsRegistered(const Key& key) {
    std::lock_guard<std::recursive_mutex> lock(factories_mutex_);
    return provider_factories_.count(key) > 0;
  }

  static void UnregisterAllProviderFactories() {
    std::lock_guard<std::recursive_mutex> lock(factories_mutex_);
    provider_factories_.clear();
  }

 private:
  static inline std::unordered_map<Key, FactoryFn> provider_factories_;
  static inline std::recursive_mutex factories_mutex_;
};

// A dummy key for the single-provider factory implementation.
enum class SingleProviderKey { kInstance };

template <typename Interface, typename Fallback>
class ProviderFactory {
 public:
  using FactoryFn = std::function<std::unique_ptr<Interface>()>;

  static std::unique_ptr<Interface> Create() {
    auto instance = internal_factory_.Create(SingleProviderKey::kInstance);
    if (instance) {
      return instance;
    }
    return std::make_unique<Fallback>();
  }

  static void RegisterProviderFactory(FactoryFn factory) {
    internal_factory_.RegisterProviderFactory(SingleProviderKey::kInstance,
                                              std::move(factory));
  }

  static void UnregisterProviderFactory() {
    internal_factory_.UnregisterProviderFactory(SingleProviderKey::kInstance);
  }

 private:
  static inline MultiKeyProviderFactory<SingleProviderKey, Interface>
      internal_factory_;
};

}  // namespace util
}  // namespace bluetooth_hal

namespace std {
template <>
struct hash<::bluetooth_hal::util::SingleProviderKey> {
  std::size_t operator()(
      const ::bluetooth_hal::util::SingleProviderKey& key) const {
    return hash<int>()(static_cast<int>(key));
  }
};
}  // namespace std
