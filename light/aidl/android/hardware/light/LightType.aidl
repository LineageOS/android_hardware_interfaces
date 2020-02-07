/*
 * Copyright (C) 2020 The Android Open Source Project
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
package android.hardware.light;

/**
 * These light IDs correspond to logical lights, not physical.
 * So for example, if your INDICATOR light is in line with your
 * BUTTONS, it might make sense to also light the INDICATOR
 * light to a reasonable color when the BUTTONS are lit.
 */
@VintfStability
enum LightType {
    BACKLIGHT = 0,
    KEYBOARD = 1,
    BUTTONS = 2,
    BATTERY = 3,
    NOTIFICATIONS = 4,
    ATTENTION = 5,
    BLUETOOTH = 6,
    WIFI = 7,
    MICROPHONE = 8,
}
