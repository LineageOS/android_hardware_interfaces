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

package android.hardware.biometrics.fingerprint;

/**
 * Additional context associated with a pointer event.
 */
@VintfStability
parcelable PointerContext {
    /* See android.view.MotionEvent#getPointerId. */
    int pointerId = 0;

    /* The distance in pixels from the left edge of the display. */
    int x = 0;

    /* The distance in pixels from the top edge of the display. */
    int y = 0;

    /* See android.view.MotionEvent#getTouchMinor. */
    float minor = 0f;

    /* See android.view.MotionEvent#getTouchMajor. */
    float major = 0f;

    /* Flag indicating that the display is in AoD mode. */
    boolean isAoD = false;
}
