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
        RETURN_VALUE_IF(!mContext, void(), "nullContext");
        std::shared_ptr<EffectContext::StatusMQ> statusMQ = mContext->getStatusFmq();
        std::shared_ptr<EffectContext::DataMQ> inputMQ = mContext->getInputDataFmq();
        std::shared_ptr<EffectContext::DataMQ> outputMQ = mContext->getOutputDataFmq();

        // Only this worker will read from input data MQ and write to output data MQ.
        auto readSamples = inputMQ->availableToRead(), writeSamples = outputMQ->availableToWrite();
        if (readSamples && writeSamples) {
            auto processSamples = std::min(readSamples, writeSamples);
            LOG(VERBOSE) << __func__ << " available to read " << readSamples
                         << " available to write " << writeSamples << " process " << processSamples;

            auto buffer = mContext->getWorkBuffer();
            inputMQ->read(buffer, processSamples);

            IEffect::Status status = effectProcessImpl(buffer, buffer, processSamples);
            outputMQ->write(buffer, status.fmqProduced);
            statusMQ->writeBlocking(&status, 1);
            LOG(VERBOSE) << __func__ << " done processing, effect consumed " << status.fmqConsumed
                         << " produced " << status.fmqProduced;
        } else {
            // TODO: maybe add some sleep here to avoid busy waiting
        }
    }

    // must implement by each effect implementation
    // TODO: consider if this interface need adjustment to handle in-place processing
    virtual IEffect::Status effectProcessImpl(float* in, float* out, int samples) = 0;

  private:
    // make sure the context only set once.
    std::once_flag mOnceFlag;
    std::shared_ptr<EffectContext> mContext;
};

}  // namespace aidl::android::hardware::audio::effect
