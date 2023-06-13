/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "1.0/Callbacks.h"
#include "1.0/Utils.h"
#include "GeneratedTestHarness.h"
#include "VtsHalNeuralnetworks.h"

#include <optional>
#include <type_traits>
#include <utility>

namespace android::hardware::neuralnetworks::V1_0::vts::functional {

using implementation::PreparedModelCallback;

using PrepareModelMutation = std::function<void(Model*)>;

///////////////////////// UTILITY FUNCTIONS /////////////////////////

static void validateGetSupportedOperations(const sp<IDevice>& device, const std::string& message,
                                           const Model& model) {
    SCOPED_TRACE(message + " [getSupportedOperations]");

    Return<void> ret =
            device->getSupportedOperations(model, [&](ErrorStatus status, const hidl_vec<bool>&) {
                EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
            });
    EXPECT_TRUE(ret.isOk());
}

static void validatePrepareModel(const sp<IDevice>& device, const std::string& message,
                                 const Model& model) {
    SCOPED_TRACE(message + " [prepareModel]");

    sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    Return<ErrorStatus> prepareLaunchStatus = device->prepareModel(model, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(prepareLaunchStatus));

    preparedModelCallback->wait();
    ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, prepareReturnStatus);
    sp<IPreparedModel> preparedModel = preparedModelCallback->getPreparedModel();
    ASSERT_EQ(nullptr, preparedModel.get());
}

// Primary validation function. This function will take a valid model, apply a
// mutation to invalidate the model, then pass these to supportedOperations and
// prepareModel.
static void validate(const sp<IDevice>& device, const std::string& message,
                     const Model& originalModel, const PrepareModelMutation& mutate) {
    Model model = originalModel;
    mutate(&model);

    validateGetSupportedOperations(device, message, model);
    validatePrepareModel(device, message, model);
}

static uint32_t addOperand(Model* model) {
    return hidl_vec_push_back(&model->operands,
                              {
                                      .type = OperandType::INT32,
                                      .dimensions = {},
                                      .numberOfConsumers = 0,
                                      .scale = 0.0f,
                                      .zeroPoint = 0,
                                      .lifetime = OperandLifeTime::MODEL_INPUT,
                                      .location = {.poolIndex = 0, .offset = 0, .length = 0},
                              });
}

static uint32_t addOperand(Model* model, OperandLifeTime lifetime) {
    uint32_t index = addOperand(model);
    model->operands[index].numberOfConsumers = 1;
    model->operands[index].lifetime = lifetime;
    return index;
}

// If we introduce a CONSTANT_COPY for an operand of size operandSize,
// how much will this increase the size of the model?  This assumes
// that we can (re)use all of model.operandValues for the operand
// value.
static size_t constantCopyExtraSize(const Model& model, size_t operandSize) {
    const size_t operandValuesSize = model.operandValues.size();
    return (operandValuesSize < operandSize) ? (operandSize - operandValuesSize) : 0;
}

// Highly specialized utility routine for converting an operand to
// CONSTANT_COPY lifetime.
//
// Expects that:
// - operand has a known size
// - operand->lifetime has already been set to CONSTANT_COPY
// - operand->location has been zeroed out
//
// Does the following:
// - initializes operand->location to point to the beginning of model->operandValues
// - resizes model->operandValues (if necessary) to be large enough for the operand
//   value, padding it with zeroes on the end
//
// Potential problem:
// By changing the operand to CONSTANT_COPY lifetime, this function is effectively initializing the
// operand with unspecified (but deterministic) data. This means that the model may be invalidated
// in two ways: not only is the lifetime of CONSTANT_COPY invalid, but the operand's value in the
// graph may also be invalid (e.g., if the operand is used as an activation code and has an invalid
// value). For now, this should be fine because it just means we're not testing what we think we're
// testing in certain cases; but we can handwave this and assume we're probabilistically likely to
// exercise the validation code over the span of the entire test set and operand space.
//
// Aborts if the specified operand type is an extension type or OEM type.
static void becomeConstantCopy(Model* model, Operand* operand) {
    // sizeOfData will abort if the specified type is an extension type or OEM type.
    const size_t sizeOfOperand = sizeOfData(*operand);
    EXPECT_NE(sizeOfOperand, size_t(0));
    operand->location.poolIndex = 0;
    operand->location.offset = 0;
    operand->location.length = sizeOfOperand;
    if (model->operandValues.size() < sizeOfOperand) {
        model->operandValues.resize(sizeOfOperand);
    }
}

// The sizeForBinder() functions estimate the size of the
// representation of a value when sent to binder.  It's probably a bit
// of an under-estimate, because we don't know the size of the
// metadata in the binder format (e.g., representation of the size of
// a vector); but at least it adds up "big" things like vector
// contents.  However, it doesn't treat inter-field or end-of-struct
// padding in a methodical way -- there's no attempt to be consistent
// in whether or not padding in the native (C++) representation
// contributes to the estimated size for the binder representation;
// and there's no attempt to understand what padding (if any) is
// needed in the binder representation.
//
// This assumes that non-metadata uses a fixed length encoding (e.g.,
// a uint32_t is always encoded in sizeof(uint32_t) bytes, rather than
// using an encoding whose length is related to the magnitude of the
// encoded value).

template <typename Type>
static size_t sizeForBinder(const Type& val) {
    static_assert(std::is_trivially_copyable_v<std::remove_reference_t<Type>>,
                  "expected a trivially copyable type");
    return sizeof(val);
}

template <typename Type>
static size_t sizeForBinder(const hidl_vec<Type>& vec) {
    return std::accumulate(vec.begin(), vec.end(), 0,
                           [](size_t acc, const Type& x) { return acc + sizeForBinder(x); });
}

template <>
size_t sizeForBinder(const Operand& operand) {
    size_t size = 0;

    size += sizeForBinder(operand.type);
    size += sizeForBinder(operand.dimensions);
    size += sizeForBinder(operand.numberOfConsumers);
    size += sizeForBinder(operand.scale);
    size += sizeForBinder(operand.zeroPoint);
    size += sizeForBinder(operand.lifetime);
    size += sizeForBinder(operand.location);

    return size;
}

template <>
size_t sizeForBinder(const Operation& operation) {
    size_t size = 0;

    size += sizeForBinder(operation.type);
    size += sizeForBinder(operation.inputs);
    size += sizeForBinder(operation.outputs);

    return size;
}

template <>
size_t sizeForBinder(const hidl_string& name) {
    return name.size();
}

template <>
size_t sizeForBinder(const hidl_memory& memory) {
    // This is just a guess.

    size_t size = 0;

    if (const native_handle_t* handle = memory.handle()) {
        size += sizeof(*handle);
        size += sizeof(handle->data[0] * (handle->numFds + handle->numInts));
    }
    size += sizeForBinder(memory.name());

    return size;
}

template <>
size_t sizeForBinder(const Model& model) {
    size_t size = 0;

    size += sizeForBinder(model.operands);
    size += sizeForBinder(model.operations);
    size += sizeForBinder(model.inputIndexes);
    size += sizeForBinder(model.outputIndexes);
    size += sizeForBinder(model.operandValues);
    size += sizeForBinder(model.pools);

    return size;
}

// https://developer.android.com/reference/android/os/TransactionTooLargeException.html
//
//     "The Binder transaction buffer has a limited fixed size,
//     currently 1Mb, which is shared by all transactions in progress
//     for the process."
//
// Will our representation fit under this limit?  There are three complications:
// - Our representation size is just approximate (see sizeForBinder()).
// - This object may not be the only occupant of the Binder transaction buffer
//   (although our VTS test suite should not be putting multiple objects in the
//   buffer at once).
// - IBinder.MAX_IPC_SIZE recommends limiting a transaction to 64 * 1024 bytes.
// So we'll be very conservative: We want the representation size to be no
// larger than half the recommended limit.
//
// If our representation grows large enough that it still fits within
// the transaction buffer but combined with other transactions may
// exceed the buffer size, then we may see intermittent HAL transport
// errors.
static bool exceedsBinderSizeLimit(size_t representationSize) {
    // There is no C++ API to retrieve the value of the Java variable IBinder.MAX_IPC_SIZE.
    static const size_t kHalfMaxIPCSize = 64 * 1024 / 2;

    return representationSize > kHalfMaxIPCSize;
}

///////////////////////// VALIDATE EXECUTION ORDER ////////////////////////////

static void mutateExecutionOrderTest(const sp<IDevice>& device, const V1_0::Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const Operation& operationObj = model.operations[operation];
        for (uint32_t input : operationObj.inputs) {
            if (model.operands[input].lifetime == OperandLifeTime::TEMPORARY_VARIABLE ||
                model.operands[input].lifetime == OperandLifeTime::MODEL_OUTPUT) {
                // This operation reads an operand written by some
                // other operation.  Move this operation to the
                // beginning of the sequence, ensuring that it reads
                // the operand before that operand is written, thereby
                // violating execution order rules.
                const std::string message = "mutateExecutionOrderTest: operation " +
                                            std::to_string(operation) + " is a reader";
                validate(device, message, model, [operation](Model* model) {
                    auto& operations = model->operations;
                    std::rotate(operations.begin(), operations.begin() + operation,
                                operations.begin() + operation + 1);
                });
                break;  // only need to do this once per operation
            }
        }
        for (uint32_t output : operationObj.outputs) {
            if (model.operands[output].numberOfConsumers > 0) {
                // This operation writes an operand read by some other
                // operation.  Move this operation to the end of the
                // sequence, ensuring that it writes the operand after
                // that operand is read, thereby violating execution
                // order rules.
                const std::string message = "mutateExecutionOrderTest: operation " +
                                            std::to_string(operation) + " is a writer";
                validate(device, message, model, [operation](Model* model) {
                    auto& operations = model->operations;
                    std::rotate(operations.begin() + operation, operations.begin() + operation + 1,
                                operations.end());
                });
                break;  // only need to do this once per operation
            }
        }
    }
}

///////////////////////// VALIDATE MODEL OPERAND TYPE /////////////////////////

static const int32_t invalidOperandTypes[] = {
        static_cast<int32_t>(OperandType::FLOAT32) - 1,              // lower bound fundamental
        static_cast<int32_t>(OperandType::TENSOR_QUANT8_ASYMM) + 1,  // upper bound fundamental
        static_cast<int32_t>(OperandType::OEM) - 1,                  // lower bound OEM
        static_cast<int32_t>(OperandType::TENSOR_OEM_BYTE) + 1,      // upper bound OEM
};

static void mutateOperandTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        for (int32_t invalidOperandType : invalidOperandTypes) {
            const std::string message = "mutateOperandTypeTest: operand " +
                                        std::to_string(operand) + " set to value " +
                                        std::to_string(invalidOperandType);
            validate(device, message, model, [operand, invalidOperandType](Model* model) {
                model->operands[operand].type = static_cast<OperandType>(invalidOperandType);
            });
        }
    }
}

///////////////////////// VALIDATE OPERAND RANK /////////////////////////

static uint32_t getInvalidRank(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
            return 1;
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::TENSOR_QUANT8_ASYMM:
            return 0;
        default:
            return 0;
    }
}

static void mutateOperandRankTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const uint32_t invalidRank = getInvalidRank(model.operands[operand].type);
        const std::string message = "mutateOperandRankTest: operand " + std::to_string(operand) +
                                    " has rank of " + std::to_string(invalidRank);
        validate(device, message, model, [operand, invalidRank](Model* model) {
            model->operands[operand].dimensions = std::vector<uint32_t>(invalidRank, 0);
        });
    }
}

///////////////////////// VALIDATE OPERAND SCALE /////////////////////////

static float getInvalidScale(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
            return 1.0f;
        case OperandType::TENSOR_INT32:
            return -1.0f;
        case OperandType::TENSOR_QUANT8_ASYMM:
            return 0.0f;
        default:
            return 0.0f;
    }
}

static void mutateOperandScaleTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const float invalidScale = getInvalidScale(model.operands[operand].type);
        const std::string message = "mutateOperandScaleTest: operand " + std::to_string(operand) +
                                    " has scale of " + std::to_string(invalidScale);
        validate(device, message, model, [operand, invalidScale](Model* model) {
            model->operands[operand].scale = invalidScale;
        });
    }
}

///////////////////////// VALIDATE OPERAND ZERO POINT /////////////////////////

static std::vector<int32_t> getInvalidZeroPoints(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
            return {1};
        case OperandType::TENSOR_QUANT8_ASYMM:
            return {-1, 256};
        default:
            return {};
    }
}

static void mutateOperandZeroPointTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const std::vector<int32_t> invalidZeroPoints =
                getInvalidZeroPoints(model.operands[operand].type);
        for (int32_t invalidZeroPoint : invalidZeroPoints) {
            const std::string message = "mutateOperandZeroPointTest: operand " +
                                        std::to_string(operand) + " has zero point of " +
                                        std::to_string(invalidZeroPoint);
            validate(device, message, model, [operand, invalidZeroPoint](Model* model) {
                model->operands[operand].zeroPoint = invalidZeroPoint;
            });
        }
    }
}

///////////////////////// VALIDATE OPERAND LIFETIME /////////////////////////////////////////////

static std::vector<OperandLifeTime> getInvalidLifeTimes(const Model& model, size_t modelSize,
                                                        const Operand& operand) {
    // TODO: Support OperandLifeTime::CONSTANT_REFERENCE as an invalid lifetime
    // TODO: Support OperandLifeTime::NO_VALUE as an invalid lifetime

    // Ways to get an invalid lifetime:
    // - change whether a lifetime means an operand should have a writer
    std::vector<OperandLifeTime> ret;
    switch (operand.lifetime) {
        case OperandLifeTime::MODEL_OUTPUT:
        case OperandLifeTime::TEMPORARY_VARIABLE:
            ret = {
                    OperandLifeTime::MODEL_INPUT,
                    OperandLifeTime::CONSTANT_COPY,
            };
            break;
        case OperandLifeTime::CONSTANT_COPY:
        case OperandLifeTime::CONSTANT_REFERENCE:
        case OperandLifeTime::MODEL_INPUT:
            ret = {
                    OperandLifeTime::TEMPORARY_VARIABLE,
                    OperandLifeTime::MODEL_OUTPUT,
            };
            break;
        case OperandLifeTime::NO_VALUE:
            // Not enough information to know whether
            // TEMPORARY_VARIABLE or CONSTANT_COPY would be invalid --
            // is this operand written (then CONSTANT_COPY would be
            // invalid) or not (then TEMPORARY_VARIABLE would be
            // invalid)?
            break;
        default:
            ADD_FAILURE();
            break;
    }

    const size_t operandSize = sizeOfData(operand);  // will be zero if shape is unknown
    if (!operandSize ||
        exceedsBinderSizeLimit(modelSize + constantCopyExtraSize(model, operandSize))) {
        // Unknown size or too-large size
        ret.erase(std::remove(ret.begin(), ret.end(), OperandLifeTime::CONSTANT_COPY), ret.end());
    }

    return ret;
}

static void mutateOperandLifeTimeTest(const sp<IDevice>& device, const V1_0::Model& model) {
    const size_t modelSize = sizeForBinder(model);
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const std::vector<OperandLifeTime> invalidLifeTimes =
                getInvalidLifeTimes(model, modelSize, model.operands[operand]);
        for (OperandLifeTime invalidLifeTime : invalidLifeTimes) {
            const std::string message = "mutateOperandLifetimeTest: operand " +
                                        std::to_string(operand) + " has lifetime " +
                                        toString(invalidLifeTime) + " instead of lifetime " +
                                        toString(model.operands[operand].lifetime);
            validate(device, message, model, [operand, invalidLifeTime](Model* model) {
                static const DataLocation kZeroDataLocation = {};
                Operand& operandObj = model->operands[operand];
                switch (operandObj.lifetime) {
                    case OperandLifeTime::MODEL_INPUT: {
                        hidl_vec_remove(&model->inputIndexes, uint32_t(operand));
                        break;
                    }
                    case OperandLifeTime::MODEL_OUTPUT: {
                        hidl_vec_remove(&model->outputIndexes, uint32_t(operand));
                        break;
                    }
                    default:
                        break;
                }
                operandObj.lifetime = invalidLifeTime;
                operandObj.location = kZeroDataLocation;
                switch (invalidLifeTime) {
                    case OperandLifeTime::CONSTANT_COPY: {
                        becomeConstantCopy(model, &operandObj);
                        break;
                    }
                    case OperandLifeTime::MODEL_INPUT:
                        hidl_vec_push_back(&model->inputIndexes, uint32_t(operand));
                        break;
                    case OperandLifeTime::MODEL_OUTPUT:
                        hidl_vec_push_back(&model->outputIndexes, uint32_t(operand));
                        break;
                    default:
                        break;
                }
            });
        }
    }
}

///////////////////////// VALIDATE OPERAND INPUT-or-OUTPUT //////////////////////////////////////

static std::optional<OperandLifeTime> getInputOutputLifeTime(const Model& model, size_t modelSize,
                                                             const Operand& operand) {
    // Ways to get an invalid lifetime (with respect to model inputIndexes and outputIndexes):
    // - change whether a lifetime means an operand is a model input, a model output, or neither
    // - preserve whether or not a lifetime means an operand should have a writer
    switch (operand.lifetime) {
        case OperandLifeTime::CONSTANT_COPY:
        case OperandLifeTime::CONSTANT_REFERENCE:
            return OperandLifeTime::MODEL_INPUT;
        case OperandLifeTime::MODEL_INPUT: {
            const size_t operandSize = sizeOfData(operand);  // will be zero if shape is unknown
            if (!operandSize ||
                exceedsBinderSizeLimit(modelSize + constantCopyExtraSize(model, operandSize))) {
                // Unknown size or too-large size
                break;
            }
            return OperandLifeTime::CONSTANT_COPY;
        }
        case OperandLifeTime::MODEL_OUTPUT:
            return OperandLifeTime::TEMPORARY_VARIABLE;
        case OperandLifeTime::TEMPORARY_VARIABLE:
            return OperandLifeTime::MODEL_OUTPUT;
        case OperandLifeTime::NO_VALUE:
            // Not enough information to know whether
            // TEMPORARY_VARIABLE or CONSTANT_COPY would be an
            // appropriate choice -- is this operand written (then
            // TEMPORARY_VARIABLE would be appropriate) or not (then
            // CONSTANT_COPY would be appropriate)?
            break;
        default:
            ADD_FAILURE();
            break;
    }

    return std::nullopt;
}

static void mutateOperandInputOutputTest(const sp<IDevice>& device, const V1_0::Model& model) {
    const size_t modelSize = sizeForBinder(model);
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const std::optional<OperandLifeTime> changedLifeTime =
                getInputOutputLifeTime(model, modelSize, model.operands[operand]);
        if (changedLifeTime) {
            const std::string message = "mutateOperandInputOutputTest: operand " +
                                        std::to_string(operand) + " has lifetime " +
                                        toString(*changedLifeTime) + " instead of lifetime " +
                                        toString(model.operands[operand].lifetime);
            validate(device, message, model, [operand, changedLifeTime](Model* model) {
                static const DataLocation kZeroDataLocation = {};
                Operand& operandObj = model->operands[operand];
                operandObj.lifetime = *changedLifeTime;
                operandObj.location = kZeroDataLocation;
                if (*changedLifeTime == OperandLifeTime::CONSTANT_COPY) {
                    becomeConstantCopy(model, &operandObj);
                }
            });
        }
    }
}

///////////////////////// VALIDATE OPERAND NUMBER OF CONSUMERS //////////////////////////////////

static std::vector<uint32_t> getInvalidNumberOfConsumers(uint32_t numberOfConsumers) {
    if (numberOfConsumers == 0) {
        return {1};
    } else {
        return {numberOfConsumers - 1, numberOfConsumers + 1};
    }
}

static void mutateOperandNumberOfConsumersTest(const sp<IDevice>& device,
                                               const V1_0::Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const std::vector<uint32_t> invalidNumberOfConsumersVec =
                getInvalidNumberOfConsumers(model.operands[operand].numberOfConsumers);
        for (uint32_t invalidNumberOfConsumers : invalidNumberOfConsumersVec) {
            const std::string message =
                    "mutateOperandNumberOfConsumersTest: operand " + std::to_string(operand) +
                    " numberOfConsumers = " + std::to_string(invalidNumberOfConsumers);
            validate(device, message, model, [operand, invalidNumberOfConsumers](Model* model) {
                model->operands[operand].numberOfConsumers = invalidNumberOfConsumers;
            });
        }
    }
}

///////////////////////// VALIDATE OPERAND NUMBER OF WRITERS ////////////////////////////////////

static void mutateOperandAddWriterTest(const sp<IDevice>& device, const V1_0::Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (size_t badOutputNum = 0; badOutputNum < model.operations[operation].outputs.size();
             ++badOutputNum) {
            const uint32_t outputOperandIndex = model.operations[operation].outputs[badOutputNum];
            const std::string message = "mutateOperandAddWriterTest: operation " +
                                        std::to_string(operation) + " writes to " +
                                        std::to_string(outputOperandIndex);
            // We'll insert a copy of the operation, all of whose
            // OTHER output operands are newly-created -- i.e.,
            // there'll only be a duplicate write of ONE of that
            // operation's output operands.
            validate(device, message, model, [operation, badOutputNum](Model* model) {
                Operation newOperation = model->operations[operation];
                for (uint32_t input : newOperation.inputs) {
                    ++model->operands[input].numberOfConsumers;
                }
                for (size_t outputNum = 0; outputNum < newOperation.outputs.size(); ++outputNum) {
                    if (outputNum == badOutputNum) continue;

                    Operand operandValue = model->operands[newOperation.outputs[outputNum]];
                    operandValue.numberOfConsumers = 0;
                    if (operandValue.lifetime == OperandLifeTime::MODEL_OUTPUT) {
                        operandValue.lifetime = OperandLifeTime::TEMPORARY_VARIABLE;
                    } else {
                        ASSERT_EQ(operandValue.lifetime, OperandLifeTime::TEMPORARY_VARIABLE);
                    }
                    newOperation.outputs[outputNum] =
                            hidl_vec_push_back(&model->operands, operandValue);
                }
                // Where do we insert the extra writer (a new
                // operation)?  It has to be later than all the
                // writers of its inputs.  The easiest thing to do
                // is to insert it at the end of the operation
                // sequence.
                hidl_vec_push_back(&model->operations, newOperation);
            });
        }
    }
}

///////////////////////// VALIDATE EXTRA ??? /////////////////////////

// TODO: Operand::location

///////////////////////// VALIDATE OPERATION OPERAND TYPE /////////////////////////

static void mutateOperand(Operand* operand, OperandType type) {
    Operand newOperand = *operand;
    newOperand.type = type;
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
            newOperand.dimensions = hidl_vec<uint32_t>();
            newOperand.scale = 0.0f;
            newOperand.zeroPoint = 0;
            break;
        case OperandType::TENSOR_FLOAT32:
            newOperand.dimensions =
                    operand->dimensions.size() > 0 ? operand->dimensions : hidl_vec<uint32_t>({1});
            newOperand.scale = 0.0f;
            newOperand.zeroPoint = 0;
            break;
        case OperandType::TENSOR_INT32:
            newOperand.dimensions =
                    operand->dimensions.size() > 0 ? operand->dimensions : hidl_vec<uint32_t>({1});
            newOperand.zeroPoint = 0;
            break;
        case OperandType::TENSOR_QUANT8_ASYMM:
            newOperand.dimensions =
                    operand->dimensions.size() > 0 ? operand->dimensions : hidl_vec<uint32_t>({1});
            newOperand.scale = operand->scale != 0.0f ? operand->scale : 1.0f;
            break;
        case OperandType::OEM:
        case OperandType::TENSOR_OEM_BYTE:
        default:
            break;
    }
    *operand = newOperand;
}

static bool mutateOperationOperandTypeSkip(size_t operand, const Model& model) {
    // LSH_PROJECTION's second argument is allowed to have any type. This is the
    // only operation that currently has a type that can be anything independent
    // from any other type. Changing the operand type to any other type will
    // result in a valid model for LSH_PROJECTION. If this is the case, skip the
    // test.
    for (const Operation& operation : model.operations) {
        if (operation.type == OperationType::LSH_PROJECTION && operand == operation.inputs[1]) {
            return true;
        }
    }
    return false;
}

static void mutateOperationOperandTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        if (mutateOperationOperandTypeSkip(operand, model)) {
            continue;
        }
        for (OperandType invalidOperandType : hidl_enum_range<OperandType>{}) {
            // Do not test OEM types
            if (invalidOperandType == model.operands[operand].type ||
                invalidOperandType == OperandType::OEM ||
                invalidOperandType == OperandType::TENSOR_OEM_BYTE) {
                continue;
            }
            const std::string message = "mutateOperationOperandTypeTest: operand " +
                                        std::to_string(operand) + " set to type " +
                                        toString(invalidOperandType);
            validate(device, message, model, [operand, invalidOperandType](Model* model) {
                mutateOperand(&model->operands[operand], invalidOperandType);
            });
        }
    }
}

///////////////////////// VALIDATE MODEL OPERATION TYPE /////////////////////////

static const int32_t invalidOperationTypes[] = {
        static_cast<int32_t>(OperationType::ADD) - 1,            // lower bound fundamental
        static_cast<int32_t>(OperationType::TANH) + 1,           // upper bound fundamental
        static_cast<int32_t>(OperationType::OEM_OPERATION) - 1,  // lower bound OEM
        static_cast<int32_t>(OperationType::OEM_OPERATION) + 1,  // upper bound OEM
};

static void mutateOperationTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (int32_t invalidOperationType : invalidOperationTypes) {
            const std::string message = "mutateOperationTypeTest: operation " +
                                        std::to_string(operation) + " set to value " +
                                        std::to_string(invalidOperationType);
            validate(device, message, model, [operation, invalidOperationType](Model* model) {
                model->operations[operation].type =
                        static_cast<OperationType>(invalidOperationType);
            });
        }
    }
}

///////////////////////// VALIDATE MODEL OPERATION INPUT OPERAND INDEX /////////////////////////

static void mutateOperationInputOperandIndexTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const uint32_t invalidOperand = model.operands.size();
        for (size_t input = 0; input < model.operations[operation].inputs.size(); ++input) {
            const std::string message = "mutateOperationInputOperandIndexTest: operation " +
                                        std::to_string(operation) + " input " +
                                        std::to_string(input);
            validate(device, message, model, [operation, input, invalidOperand](Model* model) {
                model->operations[operation].inputs[input] = invalidOperand;
            });
        }
    }
}

///////////////////////// VALIDATE MODEL OPERATION OUTPUT OPERAND INDEX /////////////////////////

static void mutateOperationOutputOperandIndexTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const uint32_t invalidOperand = model.operands.size();
        for (size_t output = 0; output < model.operations[operation].outputs.size(); ++output) {
            const std::string message = "mutateOperationOutputOperandIndexTest: operation " +
                                        std::to_string(operation) + " output " +
                                        std::to_string(output);
            validate(device, message, model, [operation, output, invalidOperand](Model* model) {
                model->operations[operation].outputs[output] = invalidOperand;
            });
        }
    }
}

///////////////////////// VALIDATE MODEL OPERANDS WRITTEN ///////////////////////////////////////

static void mutateOperationRemoveWriteTest(const sp<IDevice>& device, const V1_0::Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (size_t outputNum = 0; outputNum < model.operations[operation].outputs.size();
             ++outputNum) {
            const uint32_t outputOperandIndex = model.operations[operation].outputs[outputNum];
            if (model.operands[outputOperandIndex].numberOfConsumers > 0) {
                const std::string message = "mutateOperationRemoveWriteTest: operation " +
                                            std::to_string(operation) + " writes to " +
                                            std::to_string(outputOperandIndex);
                validate(device, message, model, [operation, outputNum](Model* model) {
                    uint32_t& outputOperandIndex = model->operations[operation].outputs[outputNum];
                    Operand operandValue = model->operands[outputOperandIndex];
                    operandValue.numberOfConsumers = 0;
                    if (operandValue.lifetime == OperandLifeTime::MODEL_OUTPUT) {
                        operandValue.lifetime = OperandLifeTime::TEMPORARY_VARIABLE;
                    } else {
                        ASSERT_EQ(operandValue.lifetime, OperandLifeTime::TEMPORARY_VARIABLE);
                    }
                    outputOperandIndex = hidl_vec_push_back(&model->operands, operandValue);
                });
            }
        }
    }
}

///////////////////////// REMOVE OPERAND FROM EVERYTHING /////////////////////////

static void removeValueAndDecrementGreaterValues(hidl_vec<uint32_t>* vec, uint32_t value) {
    if (vec) {
        // remove elements matching "value"
        auto last = std::remove(vec->begin(), vec->end(), value);
        vec->resize(std::distance(vec->begin(), last));

        // decrement elements exceeding "value"
        std::transform(vec->begin(), vec->end(), vec->begin(),
                       [value](uint32_t v) { return v > value ? v-- : v; });
    }
}

static void removeOperand(Model* model, uint32_t index) {
    hidl_vec_removeAt(&model->operands, index);
    for (Operation& operation : model->operations) {
        removeValueAndDecrementGreaterValues(&operation.inputs, index);
        removeValueAndDecrementGreaterValues(&operation.outputs, index);
    }
    removeValueAndDecrementGreaterValues(&model->inputIndexes, index);
    removeValueAndDecrementGreaterValues(&model->outputIndexes, index);
}

static void removeOperandTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const std::string message = "removeOperandTest: operand " + std::to_string(operand);
        validate(device, message, model,
                 [operand](Model* model) { removeOperand(model, operand); });
    }
}

///////////////////////// REMOVE OPERATION /////////////////////////

static void removeOperation(Model* model, uint32_t index) {
    for (uint32_t operand : model->operations[index].inputs) {
        model->operands[operand].numberOfConsumers--;
    }
    hidl_vec_removeAt(&model->operations, index);
}

static void removeOperationTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const std::string message = "removeOperationTest: operation " + std::to_string(operation);
        validate(device, message, model,
                 [operation](Model* model) { removeOperation(model, operation); });
    }
}

///////////////////////// REMOVE OPERATION INPUT /////////////////////////

static void removeOperationInputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (size_t input = 0; input < model.operations[operation].inputs.size(); ++input) {
            const Operation& op = model.operations[operation];
            // CONCATENATION has at least 2 inputs, with the last element being
            // INT32. Skip this test if removing one of CONCATENATION's
            // inputs still produces a valid model.
            if (op.type == OperationType::CONCATENATION && op.inputs.size() > 2 &&
                input != op.inputs.size() - 1) {
                continue;
            }
            const std::string message = "removeOperationInputTest: operation " +
                                        std::to_string(operation) + ", input " +
                                        std::to_string(input);
            validate(device, message, model, [operation, input](Model* model) {
                uint32_t operand = model->operations[operation].inputs[input];
                model->operands[operand].numberOfConsumers--;
                hidl_vec_removeAt(&model->operations[operation].inputs, input);
            });
        }
    }
}

///////////////////////// REMOVE OPERATION OUTPUT /////////////////////////

static void removeOperationOutputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (size_t output = 0; output < model.operations[operation].outputs.size(); ++output) {
            const std::string message = "removeOperationOutputTest: operation " +
                                        std::to_string(operation) + ", output " +
                                        std::to_string(output);
            validate(device, message, model, [operation, output](Model* model) {
                hidl_vec_removeAt(&model->operations[operation].outputs, output);
            });
        }
    }
}

///////////////////////// MODEL VALIDATION /////////////////////////

// TODO: remove model input
// TODO: remove model output
// TODO: add unused operation

///////////////////////// ADD OPERATION INPUT /////////////////////////

static void addOperationInputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const std::string message = "addOperationInputTest: operation " + std::to_string(operation);
        validate(device, message, model, [operation](Model* model) {
            uint32_t index = addOperand(model, OperandLifeTime::MODEL_INPUT);
            hidl_vec_push_back(&model->operations[operation].inputs, index);
            hidl_vec_push_back(&model->inputIndexes, index);
        });
    }
}

///////////////////////// ADD OPERATION OUTPUT /////////////////////////

static void addOperationOutputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const std::string message =
                "addOperationOutputTest: operation " + std::to_string(operation);
        validate(device, message, model, [operation](Model* model) {
            uint32_t index = addOperand(model, OperandLifeTime::MODEL_OUTPUT);
            hidl_vec_push_back(&model->operations[operation].outputs, index);
            hidl_vec_push_back(&model->outputIndexes, index);
        });
    }
}

////////////////////////// ENTRY POINT //////////////////////////////

void validateModel(const sp<IDevice>& device, const Model& model) {
    mutateExecutionOrderTest(device, model);
    mutateOperandTypeTest(device, model);
    mutateOperandRankTest(device, model);
    mutateOperandScaleTest(device, model);
    mutateOperandZeroPointTest(device, model);
    mutateOperandLifeTimeTest(device, model);
    mutateOperandInputOutputTest(device, model);
    mutateOperandNumberOfConsumersTest(device, model);
    mutateOperandAddWriterTest(device, model);
    mutateOperationOperandTypeTest(device, model);
    mutateOperationTypeTest(device, model);
    mutateOperationInputOperandIndexTest(device, model);
    mutateOperationOutputOperandIndexTest(device, model);
    mutateOperationRemoveWriteTest(device, model);
    removeOperandTest(device, model);
    removeOperationTest(device, model);
    removeOperationInputTest(device, model);
    removeOperationOutputTest(device, model);
    addOperationInputTest(device, model);
    addOperationOutputTest(device, model);
}

}  // namespace android::hardware::neuralnetworks::V1_0::vts::functional
