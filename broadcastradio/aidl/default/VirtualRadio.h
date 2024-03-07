/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "VirtualProgram.h"

#include <vector>

namespace aidl::android::hardware::broadcastradio {

/**
 * A radio frequency space mock.
 *
 * This represents all broadcast waves in the air for a given radio technology,
 * not a captured station list in the radio tuner memory.
 *
 * It's meant to abstract out radio content from default tuner implementation.
 */
class VirtualRadio final {
  public:
    VirtualRadio(const std::string& name, const std::vector<VirtualProgram>& initialList);
    std::string getName() const;
    const std::vector<VirtualProgram>& getProgramList() const;
    bool getProgram(const ProgramSelector& selector, VirtualProgram* program) const;
    std::vector<IdentifierType> getSupportedIdentifierTypes() const;

    static const VirtualRadio& getAmFmRadio();
    static const VirtualRadio& getDabRadio();

  private:
    const std::string mName;
    std::vector<VirtualProgram> mPrograms;
};

}  // namespace aidl::android::hardware::broadcastradio
