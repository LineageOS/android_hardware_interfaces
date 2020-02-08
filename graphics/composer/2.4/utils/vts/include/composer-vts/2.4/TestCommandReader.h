/*
 * Copyright 2020 The Android Open Source Project
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

#include <composer-command-buffer/2.4/ComposerCommandBuffer.h>
#include <composer-vts/2.1/TestCommandReader.h>

namespace android::hardware::graphics::composer::V2_4::vts {

// A command parser that checks that no error nor unexpected commands are
// returned.
class TestCommandReader
    : public android::hardware::graphics::composer::V2_1::vts::TestCommandReader {
  protected:
    void parseSingleCommand(int32_t commandRaw, uint16_t length) override;
};

}  // namespace android::hardware::graphics::composer::V2_4::vts
