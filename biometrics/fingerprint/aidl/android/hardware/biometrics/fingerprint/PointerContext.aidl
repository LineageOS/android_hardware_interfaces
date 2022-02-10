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
    /**
     * Pointer ID obtained from MotionEvent#getPointerId or -1 if the ID cannot be obtained, for
     * example if this event originated from a low-level wake-up gesture.
     *
     * See android.view.MotionEvent#getPointerId.
     */
    int pointerId = -1;

    /**
     * The distance in pixels from the left edge of the display.
     *
     * This is obtained from MotionEvent#getRawX and translated relative to Surface#ROTATION_0.
     * Meaning, this value is always reported as if the device is in its natural (e.g. portrait)
     * orientation.
     *
     * See android.view.MotionEvent#getRawX.
     */
    float x = 0f;

    /**
     * The distance in pixels from the top edge of the display.
     *
     * This is obtained from MotionEvent#getRawY and translated relative to Surface#ROTATION_0.
     * Meaning, this value is always reported as if the device is in its natural (e.g. portrait)
     * orientation.
     *
     * See android.view.MotionEvent#getRawY.
     */
    float y = 0f;

    /* See android.view.MotionEvent#getTouchMinor. */
    float minor = 0f;

    /* See android.view.MotionEvent#getTouchMajor. */
    float major = 0f;

    /* See android.view.MotionEvent#getOrientation. */
    float orientation = 0f;

    /* Flag indicating that the display is in AOD mode. */
    boolean isAod = false;

    /**
     * The time of the user interaction that produced this event, in milliseconds.
     *
     * This is obtained from MotionEvent#getEventTime, which uses SystemClock.uptimeMillis() as
     * the clock.
     *
     * See android.view.MotionEvent#getEventTime
     */
    long time = 0;

    /**
     * The time of the first user interaction in this gesture, in milliseconds.
     *
     * If this event is MotionEvent#ACTION_DOWN, it means it's the first event in this gesture,
     * and `gestureStart` will be equal to `time`.
     *
     * This is obtained from MotionEvent#getDownTime, which uses SystemClock.uptimeMillis() as
     * the clock.
     *
     * See android.view.MotionEvent#getDownTime
     */
    long gestureStart = 0;
}
