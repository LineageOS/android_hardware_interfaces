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

package android.hardware.neuralnetworks;

import android.hardware.neuralnetworks.ExtensionNameAndPrefix;
import android.hardware.neuralnetworks.TokenValuePair;

/**
 * A type that is used to represent all configuration related to
 * an Execution.
 */
@VintfStability
parcelable ExecutionConfig {
    /**
     * Specifies whether or not to measure duration of the execution.
     * For {@link IPreparedModel::executeSynchronouslyWithConfig}, the duration runs from the time
     * the driver sees the corresponding call to the execute function to the time the driver returns
     * from the function. For {@link IPreparedModel::executeFencedWithConfig}, please refer to
     * {@link IPreparedModelCallback} for details.
     */
    boolean measureTiming;
    /**
     * The maximum amount of time in nanoseconds that should be spent
     * executing a {@link OperationType::WHILE} operation. If a loop
     * condition model does not output false within this duration,
     * the execution must be aborted. If -1 is provided, the maximum
     * amount of time is {@link DEFAULT_LOOP_TIMEOUT_DURATION_NS}.
     * Other negative values are invalid. When provided, the duration
     * must not exceed {@link MAXIMUM_LOOP_TIMEOUT_DURATION_NS}.
     */
    long loopTimeoutDurationNs;
    /**
     * A vector of token / value pairs represent vendor specific
     * execution hints or metadata. The provided TokenValuePairs must not
     * contain the same token twice. The driver must validate the
     * data and ignore invalid hints. It is up to the driver to
     * decide whether to respect the provided hints or not.
     */
    TokenValuePair[] executionHints;
    /**
     * The mapping between extension names and prefixes of token values.
     * The driver must ignore the corresponding execution hint, if
     * the extension is not supported.
     */
    ExtensionNameAndPrefix[] extensionNameToPrefix;
}
