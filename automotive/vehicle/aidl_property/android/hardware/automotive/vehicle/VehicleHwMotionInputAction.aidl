/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.automotive.vehicle;

/**
 * See {@code android.view.MotionEvent#ACTION_*} fields for more details.
 */
@VintfStability
@Backing(type="int")
enum VehicleHwMotionInputAction {
    /**
     * Motion down
     */
    ACTION_DOWN = 0,
    /**
     * Motion up
     */
    ACTION_UP = 1,
    /**
     * Motion move
     */
    ACTION_MOVE = 2,
    /**
     * Motion cancel
     */
    ACTION_CANCEL = 3,
    /**
     * Motion outside
     */
    ACTION_OUTSIDE = 4,
    /**
     * Motion pointer down
     */
    ACTION_POINTER_DOWN = 5,
    /**
     * Motion pointer up
     */
    ACTION_POINTER_UP = 6,
    /**
     * Motion hover move
     */
    ACTION_HOVER_MOVE = 7,
    /**
     * Motion scroll
     */
    ACTION_SCROLL = 8,
    /**
     * Motion hover enter
     */
    ACTION_HOVER_ENTER = 9,
    /**
     * Motion hover exit
     */
    ACTION_HOVER_EXIT = 10,
    /**
     * Motion button press
     */
    ACTION_BUTTON_PRESS = 11,
    /**
     * Motion button release
     */
    ACTION_BUTTON_RELEASE = 12,
}
