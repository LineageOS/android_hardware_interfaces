/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <ftw.h>
#include <gtest/gtest.h>
#include <hidlmemory/mapping.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <random>

#include "Callbacks.h"
#include "GeneratedTestHarness.h"
#include "TestHarness.h"
#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {
namespace vts {
namespace functional {

using ::android::hardware::neuralnetworks::V1_2::implementation::ExecutionCallback;
using ::android::hardware::neuralnetworks::V1_2::implementation::PreparedModelCallback;
using ::android::nn::allocateSharedMemory;
using ::test_helper::MixedTypedExample;

namespace float32_model {

// In frameworks/ml/nn/runtime/test/generated/, creates a hidl model of float32 mobilenet.
#include "examples/mobilenet_224_gender_basic_fixed.example.cpp"
#include "vts_models/mobilenet_224_gender_basic_fixed.model.cpp"

// Prevent the compiler from complaining about an otherwise unused function.
[[maybe_unused]] auto dummy_createTestModel = createTestModel_dynamic_output_shape;
[[maybe_unused]] auto dummy_get_examples = get_examples_dynamic_output_shape;

// MixedTypedExample is defined in frameworks/ml/nn/tools/test_generator/include/TestHarness.h.
// This function assumes the operation is always ADD.
std::vector<MixedTypedExample> getLargeModelExamples(uint32_t len) {
    float outputValue = 1.0f + static_cast<float>(len);
    return {{.operands = {
                     // Input
                     {.operandDimensions = {{0, {1}}}, .float32Operands = {{0, {1.0f}}}},
                     // Output
                     {.operandDimensions = {{0, {1}}}, .float32Operands = {{0, {outputValue}}}}}}};
}

}  // namespace float32_model

namespace quant8_model {

// In frameworks/ml/nn/runtime/test/generated/, creates a hidl model of quant8 mobilenet.
#include "examples/mobilenet_quantized.example.cpp"
#include "vts_models/mobilenet_quantized.model.cpp"

// Prevent the compiler from complaining about an otherwise unused function.
[[maybe_unused]] auto dummy_createTestModel = createTestModel_dynamic_output_shape;
[[maybe_unused]] auto dummy_get_examples = get_examples_dynamic_output_shape;

// MixedTypedExample is defined in frameworks/ml/nn/tools/test_generator/include/TestHarness.h.
// This function assumes the operation is always ADD.
std::vector<MixedTypedExample> getLargeModelExamples(uint32_t len) {
    uint8_t outputValue = 1 + static_cast<uint8_t>(len);
    return {{.operands = {// Input
                          {.operandDimensions = {{0, {1}}}, .quant8AsymmOperands = {{0, {1}}}},
                          // Output
                          {.operandDimensions = {{0, {1}}},
                           .quant8AsymmOperands = {{0, {outputValue}}}}}}};
}

}  // namespace quant8_model

namespace {

enum class AccessMode { READ_WRITE, READ_ONLY, WRITE_ONLY };

// Creates cache handles based on provided file groups.
// The outer vector corresponds to handles and the inner vector is for fds held by each handle.
void createCacheHandles(const std::vector<std::vector<std::string>>& fileGroups,
                        const std::vector<AccessMode>& mode, hidl_vec<hidl_handle>* handles) {
    handles->resize(fileGroups.size());
    for (uint32_t i = 0; i < fileGroups.size(); i++) {
        std::vector<int> fds;
        for (const auto& file : fileGroups[i]) {
            int fd;
            if (mode[i] == AccessMode::READ_ONLY) {
                fd = open(file.c_str(), O_RDONLY);
            } else if (mode[i] == AccessMode::WRITE_ONLY) {
                fd = open(file.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
            } else if (mode[i] == AccessMode::READ_WRITE) {
                fd = open(file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            } else {
                FAIL();
            }
            ASSERT_GE(fd, 0);
            fds.push_back(fd);
        }
        native_handle_t* cacheNativeHandle = native_handle_create(fds.size(), 0);
        ASSERT_NE(cacheNativeHandle, nullptr);
        std::copy(fds.begin(), fds.end(), &cacheNativeHandle->data[0]);
        (*handles)[i].setTo(cacheNativeHandle, /*shouldOwn=*/true);
    }
}

void createCacheHandles(const std::vector<std::vector<std::string>>& fileGroups, AccessMode mode,
                        hidl_vec<hidl_handle>* handles) {
    createCacheHandles(fileGroups, std::vector<AccessMode>(fileGroups.size(), mode), handles);
}

// Create a chain of broadcast operations. The second operand is always constant tensor [1].
// For simplicity, activation scalar is shared. The second operand is not shared
// in the model to let driver maintain a non-trivial size of constant data and the corresponding
// data locations in cache.
//
//                --------- activation --------
//                ↓      ↓      ↓             ↓
// E.g. input -> ADD -> ADD -> ADD -> ... -> ADD -> output
//                ↑      ↑      ↑             ↑
//               [1]    [1]    [1]           [1]
//
// This function assumes the operation is either ADD or MUL.
template <typename CppType, OperandType operandType>
Model createLargeTestModelImpl(OperationType op, uint32_t len) {
    EXPECT_TRUE(op == OperationType::ADD || op == OperationType::MUL);

    // Model operations and operands.
    std::vector<Operation> operations(len);
    std::vector<Operand> operands(len * 2 + 2);

    // The constant buffer pool. This contains the activation scalar, followed by the
    // per-operation constant operands.
    std::vector<uint8_t> operandValues(sizeof(int32_t) + len * sizeof(CppType));

    // The activation scalar, value = 0.
    operands[0] = {
            .type = OperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = len,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 0, .length = sizeof(int32_t)},
    };
    memset(operandValues.data(), 0, sizeof(int32_t));

    // The buffer value of the constant second operand. The logical value is always 1.0f.
    CppType bufferValue;
    // The scale of the first and second operand.
    float scale1, scale2;
    if (operandType == OperandType::TENSOR_FLOAT32) {
        bufferValue = 1.0f;
        scale1 = 0.0f;
        scale2 = 0.0f;
    } else if (op == OperationType::ADD) {
        bufferValue = 1;
        scale1 = 1.0f;
        scale2 = 1.0f;
    } else {
        // To satisfy the constraint on quant8 MUL: input0.scale * input1.scale < output.scale,
        // set input1 to have scale = 0.5f and bufferValue = 2, i.e. 1.0f in floating point.
        bufferValue = 2;
        scale1 = 1.0f;
        scale2 = 0.5f;
    }

    for (uint32_t i = 0; i < len; i++) {
        const uint32_t firstInputIndex = i * 2 + 1;
        const uint32_t secondInputIndex = firstInputIndex + 1;
        const uint32_t outputIndex = secondInputIndex + 1;

        // The first operation input.
        operands[firstInputIndex] = {
                .type = operandType,
                .dimensions = {1},
                .numberOfConsumers = 1,
                .scale = scale1,
                .zeroPoint = 0,
                .lifetime = (i == 0 ? OperandLifeTime::MODEL_INPUT
                                    : OperandLifeTime::TEMPORARY_VARIABLE),
                .location = {},
        };

        // The second operation input, value = 1.
        operands[secondInputIndex] = {
                .type = operandType,
                .dimensions = {1},
                .numberOfConsumers = 1,
                .scale = scale2,
                .zeroPoint = 0,
                .lifetime = OperandLifeTime::CONSTANT_COPY,
                .location = {.poolIndex = 0,
                             .offset = static_cast<uint32_t>(i * sizeof(CppType) + sizeof(int32_t)),
                             .length = sizeof(CppType)},
        };
        memcpy(operandValues.data() + sizeof(int32_t) + i * sizeof(CppType), &bufferValue,
               sizeof(CppType));

        // The operation. All operations share the same activation scalar.
        // The output operand is created as an input in the next iteration of the loop, in the case
        // of all but the last member of the chain; and after the loop as a model output, in the
        // case of the last member of the chain.
        operations[i] = {
                .type = op,
                .inputs = {firstInputIndex, secondInputIndex, /*activation scalar*/ 0},
                .outputs = {outputIndex},
        };
    }

    // The model output.
    operands.back() = {
            .type = operandType,
            .dimensions = {1},
            .numberOfConsumers = 0,
            .scale = scale1,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::MODEL_OUTPUT,
            .location = {},
    };

    const std::vector<uint32_t> inputIndexes = {1};
    const std::vector<uint32_t> outputIndexes = {len * 2 + 1};
    const std::vector<hidl_memory> pools = {};

    return {
            .operands = operands,
            .operations = operations,
            .inputIndexes = inputIndexes,
            .outputIndexes = outputIndexes,
            .operandValues = operandValues,
            .pools = pools,
    };
}

}  // namespace

// Tag for the compilation caching tests.
class CompilationCachingTestBase : public NeuralnetworksHidlTest {
  protected:
    CompilationCachingTestBase(OperandType type) : kOperandType(type) {}

    void SetUp() override {
        NeuralnetworksHidlTest::SetUp();
        ASSERT_NE(device.get(), nullptr);

        // Create cache directory. The cache directory and a temporary cache file is always created
        // to test the behavior of prepareModelFromCache, even when caching is not supported.
        char cacheDirTemp[] = "/data/local/tmp/TestCompilationCachingXXXXXX";
        char* cacheDir = mkdtemp(cacheDirTemp);
        ASSERT_NE(cacheDir, nullptr);
        mCacheDir = cacheDir;
        mCacheDir.push_back('/');

        Return<void> ret = device->getNumberOfCacheFilesNeeded(
                [this](ErrorStatus status, uint32_t numModelCache, uint32_t numDataCache) {
                    EXPECT_EQ(ErrorStatus::NONE, status);
                    mNumModelCache = numModelCache;
                    mNumDataCache = numDataCache;
                });
        EXPECT_TRUE(ret.isOk());
        mIsCachingSupported = mNumModelCache > 0 || mNumDataCache > 0;

        // Create empty cache files.
        mTmpCache = mCacheDir + "tmp";
        for (uint32_t i = 0; i < mNumModelCache; i++) {
            mModelCache.push_back({mCacheDir + "model" + std::to_string(i)});
        }
        for (uint32_t i = 0; i < mNumDataCache; i++) {
            mDataCache.push_back({mCacheDir + "data" + std::to_string(i)});
        }
        // Dummy handles, use AccessMode::WRITE_ONLY for createCacheHandles to create files.
        hidl_vec<hidl_handle> modelHandle, dataHandle, tmpHandle;
        createCacheHandles(mModelCache, AccessMode::WRITE_ONLY, &modelHandle);
        createCacheHandles(mDataCache, AccessMode::WRITE_ONLY, &dataHandle);
        createCacheHandles({{mTmpCache}}, AccessMode::WRITE_ONLY, &tmpHandle);

        if (!mIsCachingSupported) {
            LOG(INFO) << "NN VTS: Early termination of test because vendor service does not "
                         "support compilation caching.";
            std::cout << "[          ]   Early termination of test because vendor service does not "
                         "support compilation caching."
                      << std::endl;
        }
    }

    void TearDown() override {
        // If the test passes, remove the tmp directory.  Otherwise, keep it for debugging purposes.
        if (!::testing::Test::HasFailure()) {
            // Recursively remove the cache directory specified by mCacheDir.
            auto callback = [](const char* entry, const struct stat*, int, struct FTW*) {
                return remove(entry);
            };
            nftw(mCacheDir.c_str(), callback, 128, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
        }
        NeuralnetworksHidlTest::TearDown();
    }

    // Model and examples creators. According to kOperandType, the following methods will return
    // either float32 model/examples or the quant8 variant.
    Model createTestModel() {
        if (kOperandType == OperandType::TENSOR_FLOAT32) {
            return float32_model::createTestModel();
        } else {
            return quant8_model::createTestModel();
        }
    }

    std::vector<MixedTypedExample> get_examples() {
        if (kOperandType == OperandType::TENSOR_FLOAT32) {
            return float32_model::get_examples();
        } else {
            return quant8_model::get_examples();
        }
    }

    Model createLargeTestModel(OperationType op, uint32_t len) {
        if (kOperandType == OperandType::TENSOR_FLOAT32) {
            return createLargeTestModelImpl<float, OperandType::TENSOR_FLOAT32>(op, len);
        } else {
            return createLargeTestModelImpl<uint8_t, OperandType::TENSOR_QUANT8_ASYMM>(op, len);
        }
    }

    std::vector<MixedTypedExample> getLargeModelExamples(uint32_t len) {
        if (kOperandType == OperandType::TENSOR_FLOAT32) {
            return float32_model::getLargeModelExamples(len);
        } else {
            return quant8_model::getLargeModelExamples(len);
        }
    }

    // See if the service can handle the model.
    bool isModelFullySupported(const V1_2::Model& model) {
        bool fullySupportsModel = false;
        Return<void> supportedCall = device->getSupportedOperations_1_2(
                model,
                [&fullySupportsModel, &model](ErrorStatus status, const hidl_vec<bool>& supported) {
                    ASSERT_EQ(ErrorStatus::NONE, status);
                    ASSERT_EQ(supported.size(), model.operations.size());
                    fullySupportsModel = std::all_of(supported.begin(), supported.end(),
                                                     [](bool valid) { return valid; });
                });
        EXPECT_TRUE(supportedCall.isOk());
        return fullySupportsModel;
    }

    void saveModelToCache(const V1_2::Model& model, const hidl_vec<hidl_handle>& modelCache,
                          const hidl_vec<hidl_handle>& dataCache,
                          sp<IPreparedModel>* preparedModel = nullptr) {
        if (preparedModel != nullptr) *preparedModel = nullptr;

        // Launch prepare model.
        sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
        ASSERT_NE(nullptr, preparedModelCallback.get());
        hidl_array<uint8_t, sizeof(mToken)> cacheToken(mToken);
        Return<ErrorStatus> prepareLaunchStatus =
                device->prepareModel_1_2(model, ExecutionPreference::FAST_SINGLE_ANSWER, modelCache,
                                         dataCache, cacheToken, preparedModelCallback);
        ASSERT_TRUE(prepareLaunchStatus.isOk());
        ASSERT_EQ(static_cast<ErrorStatus>(prepareLaunchStatus), ErrorStatus::NONE);

        // Retrieve prepared model.
        preparedModelCallback->wait();
        ASSERT_EQ(preparedModelCallback->getStatus(), ErrorStatus::NONE);
        if (preparedModel != nullptr) {
            *preparedModel =
                    V1_2::IPreparedModel::castFrom(preparedModelCallback->getPreparedModel())
                            .withDefault(nullptr);
        }
    }

    bool checkEarlyTermination(ErrorStatus status) {
        if (status == ErrorStatus::GENERAL_FAILURE) {
            LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                         "save the prepared model that it does not support.";
            std::cout << "[          ]   Early termination of test because vendor service cannot "
                         "save the prepared model that it does not support."
                      << std::endl;
            return true;
        }
        return false;
    }

    bool checkEarlyTermination(const V1_2::Model& model) {
        if (!isModelFullySupported(model)) {
            LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                         "prepare model that it does not support.";
            std::cout << "[          ]   Early termination of test because vendor service cannot "
                         "prepare model that it does not support."
                      << std::endl;
            return true;
        }
        return false;
    }

    void prepareModelFromCache(const hidl_vec<hidl_handle>& modelCache,
                               const hidl_vec<hidl_handle>& dataCache,
                               sp<IPreparedModel>* preparedModel, ErrorStatus* status) {
        // Launch prepare model from cache.
        sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
        ASSERT_NE(nullptr, preparedModelCallback.get());
        hidl_array<uint8_t, sizeof(mToken)> cacheToken(mToken);
        Return<ErrorStatus> prepareLaunchStatus = device->prepareModelFromCache(
                modelCache, dataCache, cacheToken, preparedModelCallback);
        ASSERT_TRUE(prepareLaunchStatus.isOk());
        if (static_cast<ErrorStatus>(prepareLaunchStatus) != ErrorStatus::NONE) {
            *preparedModel = nullptr;
            *status = static_cast<ErrorStatus>(prepareLaunchStatus);
            return;
        }

        // Retrieve prepared model.
        preparedModelCallback->wait();
        *status = preparedModelCallback->getStatus();
        *preparedModel = V1_2::IPreparedModel::castFrom(preparedModelCallback->getPreparedModel())
                                 .withDefault(nullptr);
    }

    // Absolute path to the temporary cache directory.
    std::string mCacheDir;

    // Groups of file paths for model and data cache in the tmp cache directory, initialized with
    // outer_size = mNum{Model|Data}Cache, inner_size = 1. The outer vector corresponds to handles
    // and the inner vector is for fds held by each handle.
    std::vector<std::vector<std::string>> mModelCache;
    std::vector<std::vector<std::string>> mDataCache;

    // A separate temporary file path in the tmp cache directory.
    std::string mTmpCache;

    uint8_t mToken[static_cast<uint32_t>(Constant::BYTE_SIZE_OF_CACHE_TOKEN)] = {};
    uint32_t mNumModelCache;
    uint32_t mNumDataCache;
    uint32_t mIsCachingSupported;

    // The primary data type of the testModel.
    const OperandType kOperandType;
};

// A parameterized fixture of CompilationCachingTestBase. Every test will run twice, with the first
// pass running with float32 models and the second pass running with quant8 models.
class CompilationCachingTest : public CompilationCachingTestBase,
                               public ::testing::WithParamInterface<OperandType> {
  protected:
    CompilationCachingTest() : CompilationCachingTestBase(GetParam()) {}
};

TEST_P(CompilationCachingTest, CacheSavingAndRetrieval) {
    // Create test HIDL model and compile.
    const Model testModel = createTestModel();
    if (checkEarlyTermination(testModel)) return;
    sp<IPreparedModel> preparedModel = nullptr;

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache);
    }

    // Retrieve preparedModel from cache.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (!mIsCachingSupported) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
            ASSERT_EQ(preparedModel, nullptr);
            return;
        } else if (checkEarlyTermination(status)) {
            ASSERT_EQ(preparedModel, nullptr);
            return;
        } else {
            ASSERT_EQ(status, ErrorStatus::NONE);
            ASSERT_NE(preparedModel, nullptr);
        }
    }

    // Execute and verify results.
    generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; }, get_examples(),
                                           testModel.relaxComputationFloat32toFloat16,
                                           /*testDynamicOutputShape=*/false);
}

TEST_P(CompilationCachingTest, CacheSavingAndRetrievalNonZeroOffset) {
    // Create test HIDL model and compile.
    const Model testModel = createTestModel();
    if (checkEarlyTermination(testModel)) return;
    sp<IPreparedModel> preparedModel = nullptr;

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        uint8_t dummyBytes[] = {0, 0};
        // Write a dummy integer to the cache.
        // The driver should be able to handle non-empty cache and non-zero fd offset.
        for (uint32_t i = 0; i < modelCache.size(); i++) {
            ASSERT_EQ(write(modelCache[i].getNativeHandle()->data[0], &dummyBytes,
                            sizeof(dummyBytes)),
                      sizeof(dummyBytes));
        }
        for (uint32_t i = 0; i < dataCache.size(); i++) {
            ASSERT_EQ(
                    write(dataCache[i].getNativeHandle()->data[0], &dummyBytes, sizeof(dummyBytes)),
                    sizeof(dummyBytes));
        }
        saveModelToCache(testModel, modelCache, dataCache);
    }

    // Retrieve preparedModel from cache.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        uint8_t dummyByte = 0;
        // Advance the offset of each handle by one byte.
        // The driver should be able to handle non-zero fd offset.
        for (uint32_t i = 0; i < modelCache.size(); i++) {
            ASSERT_GE(read(modelCache[i].getNativeHandle()->data[0], &dummyByte, 1), 0);
        }
        for (uint32_t i = 0; i < dataCache.size(); i++) {
            ASSERT_GE(read(dataCache[i].getNativeHandle()->data[0], &dummyByte, 1), 0);
        }
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (!mIsCachingSupported) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
            ASSERT_EQ(preparedModel, nullptr);
            return;
        } else if (checkEarlyTermination(status)) {
            ASSERT_EQ(preparedModel, nullptr);
            return;
        } else {
            ASSERT_EQ(status, ErrorStatus::NONE);
            ASSERT_NE(preparedModel, nullptr);
        }
    }

    // Execute and verify results.
    generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; }, get_examples(),
                                           testModel.relaxComputationFloat32toFloat16,
                                           /*testDynamicOutputShape=*/false);
}

TEST_P(CompilationCachingTest, SaveToCacheInvalidNumCache) {
    // Create test HIDL model and compile.
    const Model testModel = createTestModel();
    if (checkEarlyTermination(testModel)) return;

    // Test with number of model cache files greater than mNumModelCache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an additional cache file for model cache.
        mModelCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of model cache files smaller than mNumModelCache.
    if (mModelCache.size() > 0) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pop out the last cache file.
        auto tmp = mModelCache.back();
        mModelCache.pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.push_back(tmp);
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of data cache files greater than mNumDataCache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an additional cache file for data cache.
        mDataCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache.pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of data cache files smaller than mNumDataCache.
    if (mDataCache.size() > 0) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pop out the last cache file.
        auto tmp = mDataCache.back();
        mDataCache.pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache.push_back(tmp);
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_P(CompilationCachingTest, PrepareModelFromCacheInvalidNumCache) {
    // Create test HIDL model and compile.
    const Model testModel = createTestModel();
    if (checkEarlyTermination(testModel)) return;

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache);
    }

    // Test with number of model cache files greater than mNumModelCache.
    {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        mModelCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.pop_back();
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of model cache files smaller than mNumModelCache.
    if (mModelCache.size() > 0) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        auto tmp = mModelCache.back();
        mModelCache.pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.push_back(tmp);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of data cache files greater than mNumDataCache.
    {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        mDataCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache.pop_back();
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of data cache files smaller than mNumDataCache.
    if (mDataCache.size() > 0) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        auto tmp = mDataCache.back();
        mDataCache.pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache.push_back(tmp);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_P(CompilationCachingTest, SaveToCacheInvalidNumFd) {
    // Create test HIDL model and compile.
    const Model testModel = createTestModel();
    if (checkEarlyTermination(testModel)) return;

    // Go through each handle in model cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        mModelCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in model cache, test with NumFd equal to 0.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        auto tmp = mModelCache[i].back();
        mModelCache[i].pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].push_back(tmp);
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        mDataCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache[i].pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with NumFd equal to 0.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        auto tmp = mDataCache[i].back();
        mDataCache[i].pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache[i].push_back(tmp);
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_P(CompilationCachingTest, PrepareModelFromCacheInvalidNumFd) {
    // Create test HIDL model and compile.
    const Model testModel = createTestModel();
    if (checkEarlyTermination(testModel)) return;

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache);
    }

    // Go through each handle in model cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        mModelCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].pop_back();
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in model cache, test with NumFd equal to 0.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        auto tmp = mModelCache[i].back();
        mModelCache[i].pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].push_back(tmp);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        mDataCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache[i].pop_back();
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with NumFd equal to 0.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        auto tmp = mDataCache[i].back();
        mDataCache[i].pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache[i].push_back(tmp);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_P(CompilationCachingTest, SaveToCacheInvalidAccessMode) {
    // Create test HIDL model and compile.
    const Model testModel = createTestModel();
    if (checkEarlyTermination(testModel)) return;
    std::vector<AccessMode> modelCacheMode(mNumModelCache, AccessMode::READ_WRITE);
    std::vector<AccessMode> dataCacheMode(mNumDataCache, AccessMode::READ_WRITE);

    // Go through each handle in model cache, test with invalid access mode.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        modelCacheMode[i] = AccessMode::READ_ONLY;
        createCacheHandles(mModelCache, modelCacheMode, &modelCache);
        createCacheHandles(mDataCache, dataCacheMode, &dataCache);
        modelCacheMode[i] = AccessMode::READ_WRITE;
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with invalid access mode.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        dataCacheMode[i] = AccessMode::READ_ONLY;
        createCacheHandles(mModelCache, modelCacheMode, &modelCache);
        createCacheHandles(mDataCache, dataCacheMode, &dataCache);
        dataCacheMode[i] = AccessMode::READ_WRITE;
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_P(CompilationCachingTest, PrepareModelFromCacheInvalidAccessMode) {
    // Create test HIDL model and compile.
    const Model testModel = createTestModel();
    if (checkEarlyTermination(testModel)) return;
    std::vector<AccessMode> modelCacheMode(mNumModelCache, AccessMode::READ_WRITE);
    std::vector<AccessMode> dataCacheMode(mNumDataCache, AccessMode::READ_WRITE);

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache);
    }

    // Go through each handle in model cache, test with invalid access mode.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        modelCacheMode[i] = AccessMode::WRITE_ONLY;
        createCacheHandles(mModelCache, modelCacheMode, &modelCache);
        createCacheHandles(mDataCache, dataCacheMode, &dataCache);
        modelCacheMode[i] = AccessMode::READ_WRITE;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with invalid access mode.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        dataCacheMode[i] = AccessMode::WRITE_ONLY;
        createCacheHandles(mModelCache, modelCacheMode, &modelCache);
        createCacheHandles(mDataCache, dataCacheMode, &dataCache);
        dataCacheMode[i] = AccessMode::READ_WRITE;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }
}

// Copy file contents between file groups.
// The outer vector corresponds to handles and the inner vector is for fds held by each handle.
// The outer vector sizes must match and the inner vectors must have size = 1.
static void copyCacheFiles(const std::vector<std::vector<std::string>>& from,
                           const std::vector<std::vector<std::string>>& to) {
    constexpr size_t kBufferSize = 1000000;
    uint8_t buffer[kBufferSize];

    ASSERT_EQ(from.size(), to.size());
    for (uint32_t i = 0; i < from.size(); i++) {
        ASSERT_EQ(from[i].size(), 1u);
        ASSERT_EQ(to[i].size(), 1u);
        int fromFd = open(from[i][0].c_str(), O_RDONLY);
        int toFd = open(to[i][0].c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        ASSERT_GE(fromFd, 0);
        ASSERT_GE(toFd, 0);

        ssize_t readBytes;
        while ((readBytes = read(fromFd, &buffer, kBufferSize)) > 0) {
            ASSERT_EQ(write(toFd, &buffer, readBytes), readBytes);
        }
        ASSERT_GE(readBytes, 0);

        close(fromFd);
        close(toFd);
    }
}

// Number of operations in the large test model.
constexpr uint32_t kLargeModelSize = 100;
constexpr uint32_t kNumIterationsTOCTOU = 100;

TEST_P(CompilationCachingTest, SaveToCache_TOCTOU) {
    if (!mIsCachingSupported) return;

    // Create test models and check if fully supported by the service.
    const Model testModelMul = createLargeTestModel(OperationType::MUL, kLargeModelSize);
    if (checkEarlyTermination(testModelMul)) return;
    const Model testModelAdd = createLargeTestModel(OperationType::ADD, kLargeModelSize);
    if (checkEarlyTermination(testModelAdd)) return;

    // Save the testModelMul compilation to cache.
    auto modelCacheMul = mModelCache;
    for (auto& cache : modelCacheMul) {
        cache[0].append("_mul");
    }
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(modelCacheMul, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModelMul, modelCache, dataCache);
    }

    // Use a different token for testModelAdd.
    mToken[0]++;

    // This test is probabilistic, so we run it multiple times.
    for (uint32_t i = 0; i < kNumIterationsTOCTOU; i++) {
        // Save the testModelAdd compilation to cache.
        {
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);

            // Spawn a thread to copy the cache content concurrently while saving to cache.
            std::thread thread(copyCacheFiles, std::cref(modelCacheMul), std::cref(mModelCache));
            saveModelToCache(testModelAdd, modelCache, dataCache);
            thread.join();
        }

        // Retrieve preparedModel from cache.
        {
            sp<IPreparedModel> preparedModel = nullptr;
            ErrorStatus status;
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);

            // The preparation may fail or succeed, but must not crash. If the preparation succeeds,
            // the prepared model must be executed with the correct result and not crash.
            if (status != ErrorStatus::NONE) {
                ASSERT_EQ(preparedModel, nullptr);
            } else {
                ASSERT_NE(preparedModel, nullptr);
                generated_tests::EvaluatePreparedModel(
                        preparedModel, [](int) { return false; },
                        getLargeModelExamples(kLargeModelSize),
                        testModelAdd.relaxComputationFloat32toFloat16,
                        /*testDynamicOutputShape=*/false);
            }
        }
    }
}

TEST_P(CompilationCachingTest, PrepareFromCache_TOCTOU) {
    if (!mIsCachingSupported) return;

    // Create test models and check if fully supported by the service.
    const Model testModelMul = createLargeTestModel(OperationType::MUL, kLargeModelSize);
    if (checkEarlyTermination(testModelMul)) return;
    const Model testModelAdd = createLargeTestModel(OperationType::ADD, kLargeModelSize);
    if (checkEarlyTermination(testModelAdd)) return;

    // Save the testModelMul compilation to cache.
    auto modelCacheMul = mModelCache;
    for (auto& cache : modelCacheMul) {
        cache[0].append("_mul");
    }
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(modelCacheMul, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModelMul, modelCache, dataCache);
    }

    // Use a different token for testModelAdd.
    mToken[0]++;

    // This test is probabilistic, so we run it multiple times.
    for (uint32_t i = 0; i < kNumIterationsTOCTOU; i++) {
        // Save the testModelAdd compilation to cache.
        {
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            saveModelToCache(testModelAdd, modelCache, dataCache);
        }

        // Retrieve preparedModel from cache.
        {
            sp<IPreparedModel> preparedModel = nullptr;
            ErrorStatus status;
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);

            // Spawn a thread to copy the cache content concurrently while preparing from cache.
            std::thread thread(copyCacheFiles, std::cref(modelCacheMul), std::cref(mModelCache));
            prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
            thread.join();

            // The preparation may fail or succeed, but must not crash. If the preparation succeeds,
            // the prepared model must be executed with the correct result and not crash.
            if (status != ErrorStatus::NONE) {
                ASSERT_EQ(preparedModel, nullptr);
            } else {
                ASSERT_NE(preparedModel, nullptr);
                generated_tests::EvaluatePreparedModel(
                        preparedModel, [](int) { return false; },
                        getLargeModelExamples(kLargeModelSize),
                        testModelAdd.relaxComputationFloat32toFloat16,
                        /*testDynamicOutputShape=*/false);
            }
        }
    }
}

TEST_P(CompilationCachingTest, ReplaceSecuritySensitiveCache) {
    if (!mIsCachingSupported) return;

    // Create test models and check if fully supported by the service.
    const Model testModelMul = createLargeTestModel(OperationType::MUL, kLargeModelSize);
    if (checkEarlyTermination(testModelMul)) return;
    const Model testModelAdd = createLargeTestModel(OperationType::ADD, kLargeModelSize);
    if (checkEarlyTermination(testModelAdd)) return;

    // Save the testModelMul compilation to cache.
    auto modelCacheMul = mModelCache;
    for (auto& cache : modelCacheMul) {
        cache[0].append("_mul");
    }
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(modelCacheMul, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModelMul, modelCache, dataCache);
    }

    // Use a different token for testModelAdd.
    mToken[0]++;

    // Save the testModelAdd compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModelAdd, modelCache, dataCache);
    }

    // Replace the model cache of testModelAdd with testModelMul.
    copyCacheFiles(modelCacheMul, mModelCache);

    // Retrieve the preparedModel from cache, expect failure.
    {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }
}

static const auto kOperandTypeChoices =
        ::testing::Values(OperandType::TENSOR_FLOAT32, OperandType::TENSOR_QUANT8_ASYMM);

INSTANTIATE_TEST_CASE_P(TestCompilationCaching, CompilationCachingTest, kOperandTypeChoices);

class CompilationCachingSecurityTest
    : public CompilationCachingTestBase,
      public ::testing::WithParamInterface<std::tuple<OperandType, uint32_t>> {
  protected:
    CompilationCachingSecurityTest() : CompilationCachingTestBase(std::get<0>(GetParam())) {}

    void SetUp() {
        CompilationCachingTestBase::SetUp();
        generator.seed(kSeed);
    }

    // Get a random integer within a closed range [lower, upper].
    template <typename T>
    T getRandomInt(T lower, T upper) {
        std::uniform_int_distribution<T> dis(lower, upper);
        return dis(generator);
    }

    // Randomly flip one single bit of the cache entry.
    void flipOneBitOfCache(const std::string& filename, bool* skip) {
        FILE* pFile = fopen(filename.c_str(), "r+");
        ASSERT_EQ(fseek(pFile, 0, SEEK_END), 0);
        long int fileSize = ftell(pFile);
        if (fileSize == 0) {
            fclose(pFile);
            *skip = true;
            return;
        }
        ASSERT_EQ(fseek(pFile, getRandomInt(0l, fileSize - 1), SEEK_SET), 0);
        int readByte = fgetc(pFile);
        ASSERT_NE(readByte, EOF);
        ASSERT_EQ(fseek(pFile, -1, SEEK_CUR), 0);
        ASSERT_NE(fputc(static_cast<uint8_t>(readByte) ^ (1U << getRandomInt(0, 7)), pFile), EOF);
        fclose(pFile);
        *skip = false;
    }

    // Randomly append bytes to the cache entry.
    void appendBytesToCache(const std::string& filename, bool* skip) {
        FILE* pFile = fopen(filename.c_str(), "a");
        uint32_t appendLength = getRandomInt(1, 256);
        for (uint32_t i = 0; i < appendLength; i++) {
            ASSERT_NE(fputc(getRandomInt<uint8_t>(0, 255), pFile), EOF);
        }
        fclose(pFile);
        *skip = false;
    }

    enum class ExpectedResult { GENERAL_FAILURE, NOT_CRASH };

    // Test if the driver behaves as expected when given corrupted cache or token.
    // The modifier will be invoked after save to cache but before prepare from cache.
    // The modifier accepts one pointer argument "skip" as the returning value, indicating
    // whether the test should be skipped or not.
    void testCorruptedCache(ExpectedResult expected, std::function<void(bool*)> modifier) {
        const Model testModel = createTestModel();
        if (checkEarlyTermination(testModel)) return;

        // Save the compilation to cache.
        {
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            saveModelToCache(testModel, modelCache, dataCache);
        }

        bool skip = false;
        modifier(&skip);
        if (skip) return;

        // Retrieve preparedModel from cache.
        {
            sp<IPreparedModel> preparedModel = nullptr;
            ErrorStatus status;
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);

            switch (expected) {
                case ExpectedResult::GENERAL_FAILURE:
                    ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
                    ASSERT_EQ(preparedModel, nullptr);
                    break;
                case ExpectedResult::NOT_CRASH:
                    ASSERT_EQ(preparedModel == nullptr, status != ErrorStatus::NONE);
                    break;
                default:
                    FAIL();
            }
        }
    }

    const uint32_t kSeed = std::get<1>(GetParam());
    std::mt19937 generator;
};

TEST_P(CompilationCachingSecurityTest, CorruptedModelCache) {
    if (!mIsCachingSupported) return;
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        testCorruptedCache(ExpectedResult::GENERAL_FAILURE,
                           [this, i](bool* skip) { flipOneBitOfCache(mModelCache[i][0], skip); });
    }
}

TEST_P(CompilationCachingSecurityTest, WrongLengthModelCache) {
    if (!mIsCachingSupported) return;
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        testCorruptedCache(ExpectedResult::GENERAL_FAILURE,
                           [this, i](bool* skip) { appendBytesToCache(mModelCache[i][0], skip); });
    }
}

TEST_P(CompilationCachingSecurityTest, CorruptedDataCache) {
    if (!mIsCachingSupported) return;
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        testCorruptedCache(ExpectedResult::NOT_CRASH,
                           [this, i](bool* skip) { flipOneBitOfCache(mDataCache[i][0], skip); });
    }
}

TEST_P(CompilationCachingSecurityTest, WrongLengthDataCache) {
    if (!mIsCachingSupported) return;
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        testCorruptedCache(ExpectedResult::NOT_CRASH,
                           [this, i](bool* skip) { appendBytesToCache(mDataCache[i][0], skip); });
    }
}

TEST_P(CompilationCachingSecurityTest, WrongToken) {
    if (!mIsCachingSupported) return;
    testCorruptedCache(ExpectedResult::GENERAL_FAILURE, [this](bool* skip) {
        // Randomly flip one single bit in mToken.
        uint32_t ind =
                getRandomInt(0u, static_cast<uint32_t>(Constant::BYTE_SIZE_OF_CACHE_TOKEN) - 1);
        mToken[ind] ^= (1U << getRandomInt(0, 7));
        *skip = false;
    });
}

INSTANTIATE_TEST_CASE_P(TestCompilationCaching, CompilationCachingSecurityTest,
                        ::testing::Combine(kOperandTypeChoices, ::testing::Range(0U, 10U)));

}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
