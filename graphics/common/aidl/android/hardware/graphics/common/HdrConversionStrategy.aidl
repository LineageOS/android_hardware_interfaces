/**
 * Copyright (c) 2022, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.common;

import android.hardware.graphics.common.Hdr;
/**
 * Input parameter for IComposerClient.setHdrConversionStrategy
 */
@VintfStability
union HdrConversionStrategy {
    /**
     * When this parameter is set to true, HDR conversion is disabled by the
     * implementation. The output HDR type will change dynamically to match the content. This value
     * is never set to false, as other union values will be present in the false case.
     */
    boolean passthrough = true;

    /**
     * When this parameter is set, the output HDR type is selected by the
     * implementation. The implementation is only allowed to set the output HDR type to the HDR
     * types present in this list. If conversion to any of the autoHdrTypes types is not possible,
     * the implementation should do no conversion.
     */
    Hdr[] autoAllowedHdrTypes;

    /**
     * When this parameter is set, the implementation should convert all
     * content to this HDR type, when possible. If not possible, the functionality should be similar
     * to passthrough=true.
     */
    Hdr forceHdrConversion;
}
