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

#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace bluetooth_hal {
namespace transport {

class VendorPacketValidatorInterface {
 public:
  using FactoryFn =
      std::function<std::unique_ptr<VendorPacketValidatorInterface>()>;

  /**
   * @brief Registers a vendor-specific factory for creating
   * VendorPacketValidatorInterface instances.
   *
   * If a vendor factory is registered, VendorPacketValidatorInterface::Create()
   * will use it. Otherwise, a default implementation will be created.
   *
   * @param factory The factory function to register.
   */
  static void RegisterVendorPacketValidator(FactoryFn factory);

  virtual ~VendorPacketValidatorInterface() = default;

  virtual bool IsValidVendorSpecificEvent(
      std::span<const uint8_t> data) const = 0;

  /**
   * @brief Creates an instance of VendorPacketValidatorInterface.
   *
   * This factory method will use a registered vendor factory if available,
   * otherwise it will create a default implementation.
   *
   * @return A unique_ptr to a VendorPacketValidatorInterface instance.
   */
  static std::unique_ptr<VendorPacketValidatorInterface> Create();

 private:
  static FactoryFn vendor_factory_;
};

}  // namespace transport
}  // namespace bluetooth_hal
