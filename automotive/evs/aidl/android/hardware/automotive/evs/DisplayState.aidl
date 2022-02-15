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

package android.hardware.automotive.evs;

/**
 * States for control of the EVS display
 *
 * The DisplayInfo structure describes the basic properties of an EVS display. Any EVS
 * implementation is required to have one. The HAL is responsible for filling out this
 * structure to describe the EVS display. As an implementation detail, this may be a
 * physical display or a virtual display that is overlaid or mixed with another
 * presentation device.
 */
@VintfStability
@Backing(type="int")
enum DisplayState {
    /*
     * Display has not been requested by any application yet
     */
    NOT_OPEN = 0,
    /*
     * Display is inhibited
     */
    NOT_VISIBLE,
    /*
     * Will become visible with next frame
     */
    VISIBLE_ON_NEXT_FRAME,
    /*
     * Display is currently active
     */
    VISIBLE,
    /*
     * Driver is in an undefined state.  Interface should be closed.
     */
    DEAD,
}
