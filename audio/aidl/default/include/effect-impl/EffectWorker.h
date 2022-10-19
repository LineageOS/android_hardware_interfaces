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
#include <algorithm>
#include <memory>
#include <mutex>
#include <string>

#include "EffectContext.h"
#include "EffectThread.h"

namespace aidl::android::hardware::audio::effect {

std::string toString(RetCode& code);

class EffectWorker : public EffectThread {
  public:
    // set effect context for worker, suppose to only happen once here
    void setContext(std::shared_ptr<EffectContext> context) {
        std::call_once(mOnceFlag, [&]() { mContext = context; });
    };

    // handle FMQ and call effect implemented virtual function
    void process() override {
        if (!mContext) {
            LOG(ERROR) << __func__ << " invalid context!";
            return;
        }
        std::shared_ptr<EffectContext::StatusMQ> statusMQ = mContext->getStatusFmq();
        std::shared_ptr<EffectContext::DataMQ> inputMQ = mContext->getInputDataFmq();
        std::shared_ptr<EffectContext::DataMQ> outputMQ = mContext->getOutputDataFmq();

        // Only this worker will read from input data MQ and write to output data MQ.
        auto readSize = inputMQ->availableToRead(), writeSize = outputMQ->availableToWrite();
        if (readSize && writeSize) {
            LOG(DEBUG) << __func__ << " available to read " << readSize << " available to write "
                       << writeSize;
            auto buffer = mContext->getWorkBuffer();
            inputMQ->read(buffer, readSize);
            IEffect::Status status = effectProcessImpl();
            writeSize = std::min((int32_t)writeSize, status.fmqByteProduced);
            outputMQ->write(buffer, writeSize);
            statusMQ->writeBlocking(&status, 1);
            LOG(DEBUG) << __func__ << " done processing, effect consumed " << status.fmqByteConsumed
                       << " produced " << status.fmqByteProduced;
        } else {
            // TODO: maybe add some sleep here to avoid busy waiting
        }
    }

    // must implement by each effect implementation
    virtual IEffect::Status effectProcessImpl() = 0;

  private:
    // make sure the context only set once.
    std::once_flag mOnceFlag;
    std::shared_ptr<EffectContext> mContext;
};

}  // namespace aidl::android::hardware::audio::effect
