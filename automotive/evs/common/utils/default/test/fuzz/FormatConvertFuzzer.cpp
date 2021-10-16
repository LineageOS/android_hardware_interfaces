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

#include <fuzzer/FuzzedDataProvider.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "FormatConvert.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    // 1 random value (4bytes) + min imagesize = 16*2 times bytes per pixel (worse case 2)
    if (size < (4 + 16 * 2 * 2)) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    std::size_t image_pixel_size = size - 4;
    image_pixel_size = (image_pixel_size & INT_MAX) / 2;

    // API have a requirement that width must be divied by 16 except yuyvtorgb
    int min_height = 2;
    int max_height = (image_pixel_size / 16) & ~(1);  // must be even number
    int height = fdp.ConsumeIntegralInRange<uint32_t>(min_height, max_height);
    int width = (image_pixel_size / height) & ~(16);  // must be divisible by 16

    uint8_t* src = (uint8_t*)(data + 4);
    uint32_t* tgt = (uint32_t*)malloc(sizeof(uint32_t) * image_pixel_size);

#ifdef COPY_NV21_TO_RGB32
    android::hardware::automotive::evs::common::Utils::copyNV21toRGB32(width, height, src, tgt,
                                                                       width);
#elif COPY_NV21_TO_BGR32
    android::hardware::automotive::evs::common::Utils::copyNV21toBGR32(width, height, src, tgt,
                                                                       width);
#elif COPY_YV12_TO_RGB32
    android::hardware::automotive::evs::common::Utils::copyYV12toRGB32(width, height, src, tgt,
                                                                       width);
#elif COPY_YV12_TO_BGR32
    android::hardware::automotive::evs::common::Utils::copyYV12toBGR32(width, height, src, tgt,
                                                                       width);
#elif COPY_YUYV_TO_RGB32
    android::hardware::automotive::evs::common::Utils::copyYUYVtoRGB32(width, height, src, width,
                                                                       tgt, width);
#elif COPY_YUYV_TO_BGR32
    android::hardware::automotive::evs::common::Utils::copyYUYVtoBGR32(width, height, src, width,
                                                                       tgt, width);
#endif

    free(tgt);

    return 0;
}
