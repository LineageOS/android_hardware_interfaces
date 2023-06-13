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

#include <android-base/unique_fd.h>

#include "mainloop.hpp"
#include "thread_chip.hpp"

namespace aidl {
namespace android {
namespace hardware {
namespace threadnetwork {

class Service : public ot::Posix::Mainloop::Source {
  public:
    Service(char* urls[], int numUrls);

    void Update(otSysMainloopContext& context) override;
    void Process(const otSysMainloopContext& context) override;
    void startLoop(void);

  private:
    ::android::base::unique_fd mBinderFd;
    std::vector<std::shared_ptr<ThreadChip>> mThreadChips;
};
}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
