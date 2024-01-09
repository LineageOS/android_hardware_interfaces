/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.power;

@VintfStability
@Backing(type="int")
enum SessionTag {
    /**
     * This tag is used to mark uncategorized hint sessions.
     */
    OTHER,

    /**
     * This tag is used to mark the SurfaceFlinger hint session.
     */
    SURFACEFLINGER,

    /**
     * This tag is used to mark hint sessions created by HWUI.
     */
    HWUI,

    /**
     * This tag is used to mark hint sessions created by applications that are
     * categorized as games.
     */
    GAME,

    /**
     * This tag is used to mark the hint session is created by the application.
     * If an applications is categorized as game, then GAME should be used
     * instead.
     */
    APP,
}
