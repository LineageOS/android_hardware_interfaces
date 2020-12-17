/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <libnl++/Message.h>

namespace android::nl {

/**
 * In-place message mutator.
 *
 * Useful for making small changes (such as adjusting const-sized attributes or struct fields)
 * efficiently and in-place. However, if you need to rebuild the message (e.g. to modify variable
 * sized attributes or add/remove them), you need to use MessageFactory instead.
 */
class MessageMutator {
  public:
    /**
     * Construct message mutator object from editable buffer.
     */
    MessageMutator(nlmsghdr* buffer, size_t totalLen);

    nlmsghdr* operator->() const;
    operator Buffer<nlmsghdr>() const;

    /**
     * Read current attribute value.
     *
     * \param Read-only attribute buffer.
     * \returns Attribute value.
     */
    uint64_t read(Buffer<nlattr> attr) const;

    /**
     * Write new attribute value.
     *
     * \param Read-only attribute buffer.
     * \param val New value to set.
     */
    void write(Buffer<nlattr> attr, uint64_t val) const;

  private:
    const Buffer<nlmsghdr> mConstBuffer;
    nlmsghdr* mMutableBuffer;
};

}  // namespace android::nl
