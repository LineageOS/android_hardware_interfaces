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
#include <fcntl.h>
#include <ftw.h>
#include <gtest/gtest.h>
#include <hidlmemory/mapping.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <random>
#include <thread>

#include "1.2/Callbacks.h"
#include "GeneratedTestHarness.h"
#include "MemoryUtils.h"
#include "TestHarness.h"
#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

// Forward declaration of the mobilenet generated test models in
// frameworks/ml/nn/runtime/test/generated/.
namespace generated_tests::mobilenet_224_gender_basic_fixed {
const test_helper::TestModel& get_test_model();
}  // namespace generated_tests::mobilenet_224_gender_basic_fixed

namespace generated_tests::mobilenet_quantized {
const test_helper::TestModel& get_test_model();
}  // namespace generated_tests::mobilenet_quantized

namespace android::hardware::neuralnetworks::V1_2::vts::functional {

using namespace test_helper;
using implementation::PreparedModelCallback;
using V1_0::ErrorStatus;
using V1_1::ExecutionPreference;

namespace float32_model {

constexpr auto get_test_model = generated_tests::mobilenet_224_gender_basic_fixed::get_test_model;

}  // namespace float32_model

namespace quant8_model {

constexpr auto get_test_model = generated_tests::mobilenet_quantized::get_test_model;

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
template <typename CppType, TestOperandType operandType>
TestModel createLargeTestModelImpl(TestOperationType op, uint32_t len) {
    EXPECT_TRUE(op == TestOperationType::ADD || op == TestOperationType::MUL);

    // Model operations and operands.
    std::vector<TestOperation> operations(len);
    std::vector<TestOperand> operands(len * 2 + 2);

    // The activation scalar, value = 0.
    operands[0] = {
            .type = TestOperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = len,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
            .data = TestBuffer::createFromVector<int32_t>({0}),
    };

    // The buffer value of the constant second operand. The logical value is always 1.0f.
    CppType bufferValue;
    // The scale of the first and second operand.
    float scale1, scale2;
    if (operandType == TestOperandType::TENSOR_FLOAT32) {
        bufferValue = 1.0f;
        scale1 = 0.0f;
        scale2 = 0.0f;
    } else if (op == TestOperationType::ADD) {
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
                .lifetime = (i == 0 ? TestOperandLifeTime::MODEL_INPUT
                                    : TestOperandLifeTime::TEMPORARY_VARIABLE),
                .data = (i == 0 ? TestBuffer::createFromVector<CppType>({1}) : TestBuffer()),
        };

        // The second operation input, value = 1.
        operands[secondInputIndex] = {
                .type = operandType,
                .dimensions = {1},
                .numberOfConsumers = 1,
                .scale = scale2,
                .zeroPoint = 0,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .data = TestBuffer::createFromVector<CppType>({bufferValue}),
        };

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

    // For TestOperationType::ADD, output = 1 + 1 * len = len + 1
    // For TestOperationType::MUL, output = 1 * 1 ^ len = 1
    CppType outputResult = static_cast<CppType>(op == TestOperationType::ADD ? len + 1u : 1u);

    // The model output.
    operands.back() = {
            .type = operandType,
            .dimensions = {1},
            .numberOfConsumers = 0,
            .scale = scale1,
            .zeroPoint = 0,
            .lifetime = TestOperandLifeTime::MODEL_OUTPUT,
            .data = TestBuffer::createFromVector<CppType>({outputResult}),
    };

    return {
            .main = {.operands = std::move(operands),
                     .operations = std::move(operations),
                     .inputIndexes = {1},
                     .outputIndexes = {len * 2 + 1}},
            .isRelaxed = false,
    };
}

}  // namespace

// Tag for the compilation caching tests.
class CompilationCachingTestBase : public testing::Test {
  protected:
    CompilationCachingTestBase(sp<IDevice> device, OperandType type)
        : kDevice(std::move(device)), kOperandType(type) {}

    void SetUp() override {
        testing::Test::SetUp();
        ASSERT_NE(kDevice.get(), nullptr);

        // Create cache directory. The cache directory and a temporary cache file is always created
        // to test the behavior of prepareModelFromCache, even when caching is not supported.
        char cacheDirTemp[] = "/data/local/tmp/TestCompilationCachingXXXXXX";
        char* cacheDir = mkdtemp(cacheDirTemp);
        ASSERT_NE(cacheDir, nullptr);
        mCacheDir = cacheDir;
        mCacheDir.push_back('/');

        Return<void> ret = kDevice->getNumberOfCacheFilesNeeded(
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
        if (!testing::Test::HasFailure()) {
            // Recursively remove the cache directory specified by mCacheDir.
            auto callback = [](const char* entry, const struct stat*, int, struct FTW*) {
                return remove(entry);
            };
            nftw(mCacheDir.c_str(), callback, 128, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
        }
        testing::Test::TearDown();
    }

    // Model and examples creators. According to kOperandType, the following methods will return
    // either float32 model/examples or the quant8 variant.
    TestModel createTestModel() {
        if (kOperandType == OperandType::TENSOR_FLOAT32) {
            return float32_model::get_test_model();
        } else {
            return quant8_model::get_test_model();
        }
    }

    TestModel createLargeTestModel(OperationType op, uint32_t len) {
        if (kOperandType == OperandType::TENSOR_FLOAT32) {
            return createLargeTestModelImpl<float, TestOperandType::TENSOR_FLOAT32>(
                    static_cast<TestOperationType>(op), len);
        } else {
            return createLargeTestModelImpl<uint8_t, TestOperandType::TENSOR_QUANT8_ASYMM>(
                    static_cast<TestOperationType>(op), len);
        }
    }

    // See if the service can handle the model.
    bool isModelFullySupported(const Model& model) {
        bool fullySupportsModel = false;
        Return<void> supportedCall = kDevice->getSupportedOperations_1_2(
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

    void saveModelToCache(const Model& model, const hidl_vec<hidl_handle>& modelCache,
                          const hidl_vec<hidl_handle>& dataCache,
                          sp<IPreparedModel>* preparedModel = nullptr) {
        if (preparedModel != nullptr) *preparedModel = nullptr;

        // Launch prepare model.
        sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
        hidl_array<uint8_t, sizeof(mToken)> cacheToken(mToken);
        Return<ErrorStatus> prepareLaunchStatus =
                kDevice->prepareModel_1_2(model, ExecutionPreference::FAST_SINGLE_ANSWER,
                                          modelCache, dataCache, cacheToken, preparedModelCallback);
        ASSERT_TRUE(prepareLaunchStatus.isOk());
        ASSERT_EQ(static_cast<ErrorStatus>(prepareLaunchStatus), ErrorStatus::NONE);

        // Retrieve prepared model.
        preparedModelCallback->wait();
        ASSERT_EQ(preparedModelCallback->getStatus(), ErrorStatus::NONE);
        if (preparedModel != nullptr) {
            *preparedModel = IPreparedModel::castFrom(preparedModelCallback->getPreparedModel())
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

    bool checkEarlyTermination(const Model& model) {
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
        hidl_array<uint8_t, sizeof(mToken)> cacheToken(mToken);
        Return<ErrorStatus> prepareLaunchStatus = kDevice->prepareModelFromCache(
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
        *preparedModel = IPreparedModel::castFrom(preparedModelCallback->getPreparedModel())
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

    const sp<IDevice> kDevice;
    // The primary data type of the testModel.
    const OperandType kOperandType;
};

using CompilationCachingTestParam = std::tuple<NamedDevice, OperandType>;

// A parameterized fixture of CompilationCachingTestBase. Every test will run twice, with the first
// pass running with float32 models and the second pass running with quant8 models.
class CompilationCachingTest : public CompilationCachingTestBase,
                               public testing::WithParamInterface<CompilationCachingTestParam> {
  protected:
    CompilationCachingTest()
        : CompilationCachingTestBase(getData(std::get<NamedDevice>(GetParam())),
                                     std::get<OperandType>(GetParam())) {}
};

TEST_P(CompilationCachingTest, CacheSavingAndRetrieval) {
    // Create test HIDL model and compile.
    const TestModel& testModel = createTestModel();
    const Model model = createModel(testModel);
    if (checkEarlyTermination(model)) return;
    sp<IPreparedModel> preparedModel = nullptr;

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(model, modelCache, dataCache);
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
    EvaluatePreparedModel(preparedModel, testModel,
                          /*testDynamicOutputShape=*/false);
}

TEST_P(CompilationCachingTest, CacheSavingAndRetrievalNonZeroOffset) {
    // Create test HIDL model and compile.
    const TestModel& testModel = createTestModel();
    const Model model = createModel(testModel);
    if (checkEarlyTermination(model)) return;
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
        saveModelToCache(model, modelCache, dataCache);
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
    EvaluatePreparedModel(preparedModel, testModel,
                          /*testDynamicOutputShape=*/false);
}

TEST_P(CompilationCachingTest, SaveToCacheInvalidNumCache) {
    // Create test HIDL model and compile.
    const TestModel& testModel = createTestModel();
    const Model model = createModel(testModel);
    if (checkEarlyTermination(model)) return;

    // Test with number of model cache files greater than mNumModelCache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an additional cache file for model cache.
        mModelCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
    const TestModel& testModel = createTestModel();
    const Model model = createModel(testModel);
    if (checkEarlyTermination(model)) return;

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(model, modelCache, dataCache);
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
    const TestModel& testModel = createTestModel();
    const Model model = createModel(testModel);
    if (checkEarlyTermination(model)) return;

    // Go through each handle in model cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        mModelCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
    const TestModel& testModel = createTestModel();
    const Model model = createModel(testModel);
    if (checkEarlyTermination(model)) return;

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(model, modelCache, dataCache);
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
    const TestModel& testModel = createTestModel();
    const Model model = createModel(testModel);
    if (checkEarlyTermination(model)) return;
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
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
        saveModelToCache(model, modelCache, dataCache, &preparedModel);
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        EvaluatePreparedModel(preparedModel, testModel,
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
    const TestModel& testModel = createTestModel();
    const Model model = createModel(testModel);
    if (checkEarlyTermination(model)) return;
    std::vector<AccessMode> modelCacheMode(mNumModelCache, AccessMode::READ_WRITE);
    std::vector<AccessMode> dataCacheMode(mNumDataCache, AccessMode::READ_WRITE);

    // Save the compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(model, modelCache, dataCache);
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
    const TestModel testModelMul = createLargeTestModel(OperationType::MUL, kLargeModelSize);
    const Model modelMul = createModel(testModelMul);
    if (checkEarlyTermination(modelMul)) return;
    const TestModel testModelAdd = createLargeTestModel(OperationType::ADD, kLargeModelSize);
    const Model modelAdd = createModel(testModelAdd);
    if (checkEarlyTermination(modelAdd)) return;

    // Save the modelMul compilation to cache.
    auto modelCacheMul = mModelCache;
    for (auto& cache : modelCacheMul) {
        cache[0].append("_mul");
    }
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(modelCacheMul, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(modelMul, modelCache, dataCache);
    }

    // Use a different token for modelAdd.
    mToken[0]++;

    // This test is probabilistic, so we run it multiple times.
    for (uint32_t i = 0; i < kNumIterationsTOCTOU; i++) {
        // Save the modelAdd compilation to cache.
        {
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);

            // Spawn a thread to copy the cache content concurrently while saving to cache.
            std::thread thread(copyCacheFiles, std::cref(modelCacheMul), std::cref(mModelCache));
            saveModelToCache(modelAdd, modelCache, dataCache);
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
                EvaluatePreparedModel(preparedModel, testModelAdd,
                                      /*testDynamicOutputShape=*/false);
            }
        }
    }
}

TEST_P(CompilationCachingTest, PrepareFromCache_TOCTOU) {
    if (!mIsCachingSupported) return;

    // Create test models and check if fully supported by the service.
    const TestModel testModelMul = createLargeTestModel(OperationType::MUL, kLargeModelSize);
    const Model modelMul = createModel(testModelMul);
    if (checkEarlyTermination(modelMul)) return;
    const TestModel testModelAdd = createLargeTestModel(OperationType::ADD, kLargeModelSize);
    const Model modelAdd = createModel(testModelAdd);
    if (checkEarlyTermination(modelAdd)) return;

    // Save the modelMul compilation to cache.
    auto modelCacheMul = mModelCache;
    for (auto& cache : modelCacheMul) {
        cache[0].append("_mul");
    }
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(modelCacheMul, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(modelMul, modelCache, dataCache);
    }

    // Use a different token for modelAdd.
    mToken[0]++;

    // This test is probabilistic, so we run it multiple times.
    for (uint32_t i = 0; i < kNumIterationsTOCTOU; i++) {
        // Save the modelAdd compilation to cache.
        {
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            saveModelToCache(modelAdd, modelCache, dataCache);
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
                EvaluatePreparedModel(preparedModel, testModelAdd,
                                      /*testDynamicOutputShape=*/false);
            }
        }
    }
}

TEST_P(CompilationCachingTest, ReplaceSecuritySensitiveCache) {
    if (!mIsCachingSupported) return;

    // Create test models and check if fully supported by the service.
    const TestModel testModelMul = createLargeTestModel(OperationType::MUL, kLargeModelSize);
    const Model modelMul = createModel(testModelMul);
    if (checkEarlyTermination(modelMul)) return;
    const TestModel testModelAdd = createLargeTestModel(OperationType::ADD, kLargeModelSize);
    const Model modelAdd = createModel(testModelAdd);
    if (checkEarlyTermination(modelAdd)) return;

    // Save the modelMul compilation to cache.
    auto modelCacheMul = mModelCache;
    for (auto& cache : modelCacheMul) {
        cache[0].append("_mul");
    }
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(modelCacheMul, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(modelMul, modelCache, dataCache);
    }

    // Use a different token for modelAdd.
    mToken[0]++;

    // Save the modelAdd compilation to cache.
    {
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(modelAdd, modelCache, dataCache);
    }

    // Replace the model cache of modelAdd with modelMul.
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

static const auto kNamedDeviceChoices = testing::ValuesIn(getNamedDevices());
static const auto kOperandTypeChoices =
        testing::Values(OperandType::TENSOR_FLOAT32, OperandType::TENSOR_QUANT8_ASYMM);

std::string printCompilationCachingTest(
        const testing::TestParamInfo<CompilationCachingTestParam>& info) {
    const auto& [namedDevice, operandType] = info.param;
    const std::string type = (operandType == OperandType::TENSOR_FLOAT32 ? "float32" : "quant8");
    return gtestCompliantName(getName(namedDevice) + "_" + type);
}

INSTANTIATE_TEST_CASE_P(TestCompilationCaching, CompilationCachingTest,
                        testing::Combine(kNamedDeviceChoices, kOperandTypeChoices),
                        printCompilationCachingTest);

using CompilationCachingSecurityTestParam = std::tuple<NamedDevice, OperandType, uint32_t>;

class CompilationCachingSecurityTest
    : public CompilationCachingTestBase,
      public testing::WithParamInterface<CompilationCachingSecurityTestParam> {
  protected:
    CompilationCachingSecurityTest()
        : CompilationCachingTestBase(getData(std::get<NamedDevice>(GetParam())),
                                     std::get<OperandType>(GetParam())) {}

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
        const TestModel& testModel = createTestModel();
        const Model model = createModel(testModel);
        if (checkEarlyTermination(model)) return;

        // Save the compilation to cache.
        {
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            saveModelToCache(model, modelCache, dataCache);
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

    const uint32_t kSeed = std::get<uint32_t>(GetParam());
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

std::string printCompilationCachingSecurityTest(
        const testing::TestParamInfo<CompilationCachingSecurityTestParam>& info) {
    const auto& [namedDevice, operandType, seed] = info.param;
    const std::string type = (operandType == OperandType::TENSOR_FLOAT32 ? "float32" : "quant8");
    return gtestCompliantName(getName(namedDevice) + "_" + type + "_" + std::to_string(seed));
}

INSTANTIATE_TEST_CASE_P(TestCompilationCaching, CompilationCachingSecurityTest,
                        testing::Combine(kNamedDeviceChoices, kOperandTypeChoices,
                                         testing::Range(0U, 10U)),
                        printCompilationCachingSecurityTest);

}  // namespace android::hardware::neuralnetworks::V1_2::vts::functional
