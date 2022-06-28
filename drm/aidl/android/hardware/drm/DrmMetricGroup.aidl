/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.drm;

import android.hardware.drm.DrmMetric;

/**
 * This message contains plugin-specific metrics made available to the client.
 * The message is used for making vendor-specific metrics available to an
 * application. The framework is not consuming any of the information.
 *
 * Metrics are grouped in instances of DrmMetricGroup. Each group contains
 * multiple instances of Metric.
 *
 * Example:
 *
 * Capture the timing information of a buffer copy event, "buf_copy", broken
 * out by the "size" of the buffer.
 *
 * DrmMetricGroup {
 *   metrics[0] {
 *     name: "buf_copy"
 *     attributes[0] {
 *       name: "size"
 *       type: INT64_TYPE
 *       int64Value: 1024
 *     }
 *     values[0] {
 *       componentName: "operation_count"
 *       type: INT64_TYPE
 *       int64Value: 75
 *     }
 *     values[1] {
 *       component_name: "average_time_seconds"
 *       type: DOUBLE_TYPE
 *       doubleValue: 0.00000042
 *     }
 *   }
 * }
 */
@VintfStability
parcelable DrmMetricGroup {
    /**
     * The list of metrics to be captured.
     */
    List<DrmMetric> metrics;
}
