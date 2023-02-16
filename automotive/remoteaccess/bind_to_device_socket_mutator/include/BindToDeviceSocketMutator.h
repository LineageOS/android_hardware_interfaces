/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <grpc++/grpc++.h>
#include <src/core/lib/iomgr/socket_mutator.h>
#include <string>

namespace android::hardware::automotive::remoteaccess {

class BindToDeviceSocketMutator final : public grpc_socket_mutator {
  public:
    BindToDeviceSocketMutator(const std::string_view& interface_name);

    bool mutateFd(int fd);

  private:
    std::string mIfname;
};

}  // namespace android::hardware::automotive::remoteaccess
