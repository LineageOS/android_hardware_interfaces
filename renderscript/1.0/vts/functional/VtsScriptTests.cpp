/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "VtsHalRenderscriptV1_0TargetTest.h"

/*
 * Create a Blur intrinsic with scriptIntrinsicCreate, and call
 * scriptSetTimeZone to make sure it is not crashing.
 *
 * Calls: elementCreate, scriptIntrinsicCreate, scriptSetTimeZone
 */
TEST_F(RenderscriptHidlTest, IntrinsicTest) {
    // uint8
    Element element = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 1);
    Script script = context->scriptIntrinsicCreate(ScriptIntrinsicID::ID_BLUR, element);
    EXPECT_NE(Script(0), script);

    context->scriptSetTimeZone(script, "UTF-8");
}

/*
 * Create a user script “struct_test”, and verified the setters and getters work
 * for the global variables.
 *
 * Calls: scriptCCreate, scriptGetVarV, scriptSetVarI, scriptSetVarJ,
 * scriptSetVarF, scriptSetVarD, elementCreate, typeCreate,
 * allocationCreateTyped, scriptSetVarObj, scriptSetVarV, scriptSetVarVE
 */
TEST_F(RenderscriptHidlTest, ScriptVarTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode, bitCodeLength);
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // arg tests
    context->scriptSetVarI(script, 0, 100);
    int resultI = 0;
    context->scriptGetVarV(script, 0, sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               resultI = *((int*)_data.data()); });
    EXPECT_EQ(100, resultI);

    context->scriptSetVarJ(script, 1, 101l);
    int resultJ = 0;
    context->scriptGetVarV(script, 1, sizeof(long), [&](const hidl_vec<uint8_t>& _data){
                               resultJ = *((long*)_data.data()); });
    EXPECT_EQ(101, resultJ);

    context->scriptSetVarF(script, 2, 102.0f);
    int resultF = 0.0f;
    context->scriptGetVarV(script, 2, sizeof(float), [&](const hidl_vec<uint8_t>& _data){
                               resultF = *((float*)_data.data()); });
    EXPECT_EQ(102.0f, resultF);

    context->scriptSetVarD(script, 3, 103.0);
    int resultD = 0.0;
    context->scriptGetVarV(script, 3, sizeof(double), [&](const hidl_vec<uint8_t>& _data){
                               resultD = *((double*)_data.data()); });
    EXPECT_EQ(103.0, resultD);

    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 128 x float1
    Type type = context->typeCreate(element, 128, 0, 0, false, false, YuvFormat::YUV_NONE);
    // 128 x float1
    Allocation allocationIn = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                             (int)AllocationUsageType::SCRIPT,
                                                             (Ptr)nullptr);
    Allocation allocationOut = Allocation(0);
    context->scriptSetVarObj(script, 4, (ObjectBase)allocationIn);
    context->scriptGetVarV(script, 4, sizeof(ObjectBase), [&](const hidl_vec<uint8_t>& _data){
                               allocationOut = (Allocation) *((ObjectBase*)_data.data()); });
    EXPECT_EQ(allocationOut, allocationIn);

    std::vector<int> arrayIn = {500, 501, 502, 503};
    std::vector<int> arrayOut(4);
    hidl_vec<uint8_t> arrayData;
    arrayData.setToExternal((uint8_t*)arrayIn.data(), arrayIn.size()*sizeof(int));
    context->scriptSetVarV(script, 5, arrayData);
    context->scriptGetVarV(script, 5, 4*sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               arrayOut = std::vector<int>((int*)_data.data(),
                                                           (int*)_data.data() + 4); });
    EXPECT_EQ(500, arrayOut[0]);
    EXPECT_EQ(501, arrayOut[1]);
    EXPECT_EQ(502, arrayOut[2]);
    EXPECT_EQ(503, arrayOut[3]);

    std::vector<int> dataVE = {1000, 1001};
    std::vector<uint32_t> dimsVE = {1};
    std::vector<int> outVE(2);
    hidl_vec<uint8_t> _dataVE;
    hidl_vec<uint32_t> _dimsVE;
    _dataVE.setToExternal((uint8_t*)dataVE.data(), dataVE.size()*sizeof(int));
    _dimsVE.setToExternal((uint32_t*)dimsVE.data(), dimsVE.size());
    // intx2
    Element elementVE = context->elementCreate(DataType::SIGNED_32, DataKind::USER, false, 2);
    context->scriptSetVarVE(script, 6, _dataVE, elementVE, _dimsVE);
    context->scriptGetVarV(script, 6, 2*sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               outVE = std::vector<int>((int*)_data.data(),
                                                        (int*)_data.data() + 2); });
    EXPECT_EQ(1000, outVE[0]);
    EXPECT_EQ(1001, outVE[1]);
}

/*
 * Create a user script “struct_test”, and input and output Allocations.
 * Verified the foreach launch correctly for the invoke kernel.
 *
 * Calls: scriptCCreate, scriptInvoke, scriptGetVarV, scriptInvokeV
 */
TEST_F(RenderscriptHidlTest, ScriptInvokeTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode, bitCodeLength);
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // invoke test
    int function_res = 0;
    context->scriptInvoke(script, 0);
    context->scriptGetVarV(script, 0, sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               function_res = *((int*)_data.data()); });
    EXPECT_NE(100, function_res);

    // invokeV test
    int functionV_arg = 5;
    int functionV_res = 0;
    hidl_vec<uint8_t> functionV_data;
    functionV_data.setToExternal((uint8_t*)&functionV_arg, sizeof(int));
    context->scriptInvokeV(script, 1, functionV_data);
    context->scriptGetVarV(script, 0, sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               functionV_res = *((int*)_data.data()); });
    EXPECT_EQ(5, functionV_res);
}

/*
 * Create a user script “struct_test”, and input and output Allocations.
 * Verified the foreach launch correctly for the foreach kernel.
 *
 * Calls: scriptCCreate, elementCreate, typeCreate, allocationCreateTyped,
 * allocation1DWrite, scriptForEach, allocationRead
 */
TEST_F(RenderscriptHidlTest, ScriptForEachTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode, bitCodeLength);
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // uint8_t
    Element element = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 1);
    // 64 x uint8_t
    Type type = context->typeCreate(element, 64, 0, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<uint8_t> dataIn(64), dataOut(64);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static uint8_t val = 0; return val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size());
    // 64 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    Allocation vout = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                     (int)AllocationUsageType::SCRIPT,
                                                     (Ptr)nullptr);
    context->allocation1DWrite(allocation, 0, 0, (Size)dataIn.size(), _data);
    hidl_vec<Allocation> vains;
    vains.setToExternal(&allocation, 1);
    hidl_vec<uint8_t> params;
    context->scriptForEach(script, 1, vains, vout, params, nullptr);
    context->allocationRead(vout, (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(uint8_t));
    bool same = std::all_of(dataOut.begin(), dataOut.end(),
                            [](uint8_t x){ static uint8_t val = 1; return x == val++; });
    EXPECT_EQ(true, same);
}

/*
 * Create a user script “struct_test”, and input and output Allocations.
 * Verified the foreach launch correctly for the reduction kernel.
 *
 * Calls: scriptCCreate, elementCreate, typeCreate, allocationCreateTyped,
 * allocation1DWrite, scriptReduce, contextFinish, allocationRead
 */
TEST_F(RenderscriptHidlTest, ScriptReduceTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode, bitCodeLength);
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // uint8_t
    Element element = context->elementCreate(DataType::SIGNED_32, DataKind::USER, false, 1);
    // 64 x uint8_t
    Type type = context->typeCreate(element, 64, 0, 0, false, false, YuvFormat::YUV_NONE);
    Type type2 = context->typeCreate(element, 1, 0, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<int> dataIn(64), dataOut(1);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(int));
    // 64 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    Allocation vaout = context->allocationCreateTyped(type2, AllocationMipmapControl::NONE,
                                                      (int)AllocationUsageType::SCRIPT,
                                                      (Ptr)nullptr);
    context->allocation1DWrite(allocation, 0, 0, (Size)dataIn.size(), _data);
    hidl_vec<Allocation> vains;
    vains.setToExternal(&allocation, 1);
    context->scriptReduce(script, 0, vains, vaout, nullptr);
    context->contextFinish();
    context->allocationRead(vaout, (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(int));
    // sum of 0, 1, 2, ..., 62, 63
    int sum = 63*64/2;
    EXPECT_EQ(sum, dataOut[0]);
}

/*
 * This test creates an allocation and binds it to a data segment in the
 * RenderScript script, represented in the bitcode.
 *
 * Calls: scriptCCreate, elementCreate, typeCreate, allocationCreateTyped,
 * allocationGetPointer, scriptBindAllocation
 *
 * This test currently has a bug, and should be fixed by 3/17.
 * TODO(butlermichael)
 */
/*
TEST_F(RenderscriptHidlTest, ScriptBindTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode, bitCodeLength);
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // uint8_t
    Element element = context->elementCreate(DataType::SIGNED_32, DataKind::USER, false, 1);
    // 64 x uint8_t
    Type type = context->typeCreate(element, 64, 0, 0, false, false, YuvFormat::YUV_NONE);
    // 64 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    Ptr dataPtr1, dataPtr2;
    Size stride;
    context->allocationGetPointer(allocation, 0, AllocationCubemapFace::POSITIVE_X, 0,
                                  [&](Ptr _dataPtr, Size _stride){ dataPtr1 = _dataPtr;
                                      stride = _stride; });
    context->scriptBindAllocation(script, allocation, 7);
    context->allocationGetPointer(allocation, 0, AllocationCubemapFace::POSITIVE_X, 0,
                                  [&](Ptr _dataPtr, Size _stride){ dataPtr2 = _dataPtr;
                                      stride = _stride; });
    EXPECT_NE(dataPtr1, dataPtr2);
}
*/

/*
 * This test groups together two RenderScript intrinsic kernels to run one after
 * the other asynchronously with respect to the client. The test configures YuvToRGB(A) and Blur,
 * and links them together such that Blur will execute after YuvToRGB(A) and use its result. The
 * test checks the data returned to make sure it was changed after passing through the entire
 * ScriptGroup.
 *
 * Calls: elementCreate, typeCreate, allocationCreateTyped, allocation2DWrite,
 * scriptIntrinsicCreate, scriptKernelIDCreate, scriptGroupCreate,
 * scriptGroupSetInput, scriptGroupSetOutput, scriptGroupExecute,
 * allocation2DRead
 *
 * This test currently has a bug, and should be fixed by 3/17.
 * TODO(butlermichael)
 */
/*
TEST_F(RenderscriptHidlTest, ScriptGroupTest) {
    //std::vector<uint8_t> dataIn(256*256*1, 128), dataOut(256*256*3, 0);
    std::vector<uint8_t> dataIn(256*256*1, 128), dataOut(256*256*4, 0);
    hidl_vec<uint8_t> _dataIn, _dataOut;
    _dataIn.setToExternal(dataIn.data(), dataIn.size());
    _dataOut.setToExternal(dataOut.data(), dataIn.size());

    // 256 x 256 YUV pixels
    Element element1 = context->elementCreate(DataType::UNSIGNED_8, DataKind::PIXEL_YUV, true, 1);
    //Type type1 = context->typeCreate(element1, 256, 256, 0, false, false, YuvFormat::YUV_420_888);
    Type type1 = context->typeCreate(element1, 256, 256, 0, false, false, YuvFormat::YUV_NV21);
    Allocation allocation1 = context->allocationCreateTyped(type1, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    context->allocation2DWrite(allocation1, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 256, 256,
                               _dataIn, 0);
    Script yuv2rgb = context->scriptIntrinsicCreate(ScriptIntrinsicID::ID_YUV_TO_RGB, element1);
    EXPECT_NE(Script(0), yuv2rgb);

    ScriptKernelID yuv2rgbKID = context->scriptKernelIDCreate(yuv2rgb, 0, 2);
    EXPECT_NE(ScriptKernelID(0), yuv2rgbKID);

    // 256 x 256 RGB pixels
    //Element element2 = context->elementCreate(DataType::UNSIGNED_8, DataKind::PIXEL_RGB, true, 3);
    Element element2 = context->elementCreate(DataType::UNSIGNED_8, DataKind::PIXEL_RGBA, true, 4);
    Type type2 = context->typeCreate(element2, 256, 256, 0, false, false, YuvFormat::YUV_NONE);
    Allocation allocation2 = context->allocationCreateTyped(type2, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    context->allocation2DWrite(allocation2, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 256, 256,
                               _dataOut, 0);
    Script blur = context->scriptIntrinsicCreate(ScriptIntrinsicID::ID_BLUR, element2);
    EXPECT_NE(Script(0), blur);

    ScriptKernelID blurKID = context->scriptKernelIDCreate(blur, 0, 2);
    EXPECT_NE(ScriptKernelID(0), blurKID);

    // ScriptGroup
    hidl_vec<ScriptKernelID> kernels = {yuv2rgbKID, blurKID};
    hidl_vec<ScriptKernelID> srcK = {yuv2rgbKID};
    hidl_vec<ScriptKernelID> dstK = {blurKID};
    hidl_vec<ScriptFieldID> dstF = {};
    hidl_vec<Type> types = {type2};
    ScriptGroup scriptGroup = context->scriptGroupCreate(kernels, srcK, dstK, dstF, types);
    EXPECT_NE(ScriptGroup(0), scriptGroup);

    context->scriptGroupSetInput(scriptGroup, yuv2rgbKID, allocation1);
    context->scriptGroupSetOutput(scriptGroup, blurKID, allocation2);
    context->scriptGroupExecute(scriptGroup);

    // verify contents were changed
    context->allocation2DRead(allocation2, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 256, 256,
                              (Ptr)dataOut.data(), (Size)dataOut.size(), 0);
    bool same = std::all_of(dataOut.begin(), dataOut.end(), [](uint8_t x){ return x != 0; });
    EXPECT_EQ(true, same);
}
*/

/*
 * Similar to the ScriptGroup test, this test verifies the execution flow of
 * RenderScript kernels and invokables.
 *
 * Calls: scriptFieldIDCreate, closureCreate, scriptInvokeIDCreate,
 * invokeClosureCreate, closureSetArg, closureSetGlobal, scriptGroup2Create,
 * scriptGroupExecute
 *
 * This test currently still a work in progress, and should be finished by 3/17.
 * TODO(butlermichael)
 */
/*
TEST_F(RenderscriptHidlTest, ScriptGroup2Test) {

    ScriptFieldID fieldID = context->scriptFieldIDCreate(script, slot);
    EXPECT_NE(ScriptFieldID(0), fieldID);

    ScriptKernelID kernelID = context->scriptKernelIDCreate(script, slot, sig);
    EXPECT_NE(ScriptKernelID(0), kernelID);

    Allocation returnValue = 0;
    hidl_vec<ScriptFieldID> fieldIDS = {};
    hidl_vec<int64_t> values = {};
    hidl_vec<int32_t> sizes = {};
    hidl_veC<Closure> depClosures = {};
    hidl_vec<ScriptFieldID> depFieldIDS = {};
    Closure closure1 = context->closureCreate(kernelID, returnValue, fieldIDS, values, sizes,
                                             depClosures, depFieldIDS);
    EXPECT_NE(Closure(0), closure1);

    ScriptInvokeID invokeID = context->scriptInvokeIDCreate(script, slot);
    EXPECT_NE(ScriptInvokeID(0), invokeID);

    hidl_vec<uint8_t> params = {};
    hidl_vec<ScriptFieldID> fieldsIDS2 = {};
    hidl_vec<int64_t> values2 = {};
    hidl_vec<int32_t> sizes2 = {};
    Closure closure2 = context->invokeClosureCreate(invokeID, params, fieldIDS2, values2, sizes2);
    EXPECT_NE(Closure(0), closure2);

    context->closureSetArg(closure, index, value, size);
    context->closureSetGlobal(closure, fieldID, value, size);

    hidl_string name = "script_group_2_test";
    hidl_string cacheDir = "data/local/tmp/";
    hidl_vec<Closures> closures;
    ScriptGroup2 scriptGroup2 = context->scriptGroup2Create(name, cacheDir, closures);
    EXPECT_NE(ScriptGroup2(0), scriptGroup2);

    context->scriptGroupExecute(scriptGroup2);
    // verify script group launched...
}
*/
