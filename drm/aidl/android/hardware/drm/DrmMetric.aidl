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

import android.hardware.drm.DrmMetricNamedValue;

/**
 * The metric being captured.
 *
 * A metric must have a name and at least one value. A metric may have 0 or
 * more attributes. The fields of a Metric are opaque to the framework.
 */
@VintfStability
parcelable DrmMetric {
    String name;

    /**
     * Detail(s) about the metric being captured.
     *
     * The fields of an Attribute are opaque to the framework.
     */
    List<DrmMetricNamedValue> attributes;

    /**
     * Value(s) of the metric.
     *
     * A metric may have multiple values. The component name may be left empty
     * if there is only supposed to be one value for the given metric. The
     * fields of the Value are opaque to the framework.
     */
    List<DrmMetricNamedValue> values;
}
