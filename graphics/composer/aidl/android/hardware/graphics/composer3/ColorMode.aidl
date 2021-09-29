/**
 * Copyright (c) 2021, The Android Open Source Project
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

package android.hardware.graphics.composer3;

@VintfStability
@Backing(type="int")
enum ColorMode {
    /**
     * DEFAULT is the "native" gamut of the display.
     * White Point: Vendor/OEM defined
     * Panel Gamma: Vendor/OEM defined (typically 2.2)
     * Rendering Intent: Vendor/OEM defined (typically 'enhanced')
     */
    NATIVE = 0,
    /**
     * STANDARD_BT601_625 corresponds with display
     * settings that implement the ITU-R Recommendation BT.601
     * or Rec 601. Using 625 line version
     * Rendering Intent: Colorimetric
     * Primaries:
     *                  x       y
     *  green           0.290   0.600
     *  blue            0.150   0.060
     *  red             0.640   0.330
     *  white (D65)     0.3127  0.3290
     *
     *  KR = 0.299, KB = 0.114. This adjusts the luminance interpretation
     *  for RGB conversion from the one purely determined by the primaries
     *  to minimize the color shift into RGB space that uses BT.709
     *  primaries.
     *
     * Gamma Correction (GC):
     *
     *  if Vlinear < 0.018
     *    Vnonlinear = 4.500 * Vlinear
     *  else
     *    Vnonlinear = 1.099 * (Vlinear)^(0.45) – 0.099
     */
    STANDARD_BT601_625 = 1,
    /**
     * Primaries:
     *                  x       y
     *  green           0.290   0.600
     *  blue            0.150   0.060
     *  red             0.640   0.330
     *  white (D65)     0.3127  0.3290
     *
     *  Use the unadjusted KR = 0.222, KB = 0.071 luminance interpretation
     *  for RGB conversion.
     *
     * Gamma Correction (GC):
     *
     *  if Vlinear < 0.018
     *    Vnonlinear = 4.500 * Vlinear
     *  else
     *    Vnonlinear = 1.099 * (Vlinear)^(0.45) – 0.099
     */
    STANDARD_BT601_625_UNADJUSTED = 2,
    /**
     * Primaries:
     *                  x       y
     *  green           0.310   0.595
     *  blue            0.155   0.070
     *  red             0.630   0.340
     *  white (D65)     0.3127  0.3290
     *
     *  KR = 0.299, KB = 0.114. This adjusts the luminance interpretation
     *  for RGB conversion from the one purely determined by the primaries
     *  to minimize the color shift into RGB space that uses BT.709
     *  primaries.
     *
     * Gamma Correction (GC):
     *
     *  if Vlinear < 0.018
     *    Vnonlinear = 4.500 * Vlinear
     *  else
     *    Vnonlinear = 1.099 * (Vlinear)^(0.45) – 0.099
     */
    STANDARD_BT601_525 = 3,
    /**
     * Primaries:
     *                  x       y
     *  green           0.310   0.595
     *  blue            0.155   0.070
     *  red             0.630   0.340
     *  white (D65)     0.3127  0.3290
     *
     *  Use the unadjusted KR = 0.212, KB = 0.087 luminance interpretation
     *  for RGB conversion (as in SMPTE 240M).
     *
     * Gamma Correction (GC):
     *
     *  if Vlinear < 0.018
     *    Vnonlinear = 4.500 * Vlinear
     *  else
     *    Vnonlinear = 1.099 * (Vlinear)^(0.45) – 0.099
     */
    STANDARD_BT601_525_UNADJUSTED = 4,
    /**
     * REC709 corresponds with display settings that implement
     * the ITU-R Recommendation BT.709 / Rec. 709 for high-definition television.
     * Rendering Intent: Colorimetric
     * Primaries:
     *                  x       y
     *  green           0.300   0.600
     *  blue            0.150   0.060
     *  red             0.640   0.330
     *  white (D65)     0.3127  0.3290
     *
     * HDTV REC709 Inverse Gamma Correction (IGC): V represents normalized
     * (with [0 to 1] range) value of R, G, or B.
     *
     *  if Vnonlinear < 0.081
     *    Vlinear = Vnonlinear / 4.5
     *  else
     *    Vlinear = ((Vnonlinear + 0.099) / 1.099) ^ (1/0.45)
     *
     * HDTV REC709 Gamma Correction (GC):
     *
     *  if Vlinear < 0.018
     *    Vnonlinear = 4.5 * Vlinear
     *  else
     *    Vnonlinear = 1.099 * (Vlinear) ^ 0.45 – 0.099
     */
    STANDARD_BT709 = 5,
    /**
     * DCI_P3 corresponds with display settings that implement
     * SMPTE EG 432-1 and SMPTE RP 431-2
     * Rendering Intent: Colorimetric
     * Primaries:
     *                  x       y
     *  green           0.265   0.690
     *  blue            0.150   0.060
     *  red             0.680   0.320
     *  white (D65)     0.3127  0.3290
     *
     * Gamma: 2.6
     */
    DCI_P3 = 6,
    /**
     * SRGB corresponds with display settings that implement
     * the sRGB color space. Uses the same primaries as ITU-R Recommendation
     * BT.709
     * Rendering Intent: Colorimetric
     * Primaries:
     *                  x       y
     *  green           0.300   0.600
     *  blue            0.150   0.060
     *  red             0.640   0.330
     *  white (D65)     0.3127  0.3290
     *
     * PC/Internet (sRGB) Inverse Gamma Correction (IGC):
     *
     *  if Vnonlinear ≤ 0.03928
     *    Vlinear = Vnonlinear / 12.92
     *  else
     *    Vlinear = ((Vnonlinear + 0.055)/1.055) ^ 2.4
     *
     * PC/Internet (sRGB) Gamma Correction (GC):
     *
     *  if Vlinear ≤ 0.0031308
     *    Vnonlinear = 12.92 * Vlinear
     *  else
     *    Vnonlinear = 1.055 * (Vlinear)^(1/2.4) – 0.055
     */
    SRGB = 7,
    /**
     * ADOBE_RGB corresponds with the RGB color space developed
     * by Adobe Systems, Inc. in 1998.
     * Rendering Intent: Colorimetric
     * Primaries:
     *                  x       y
     *  green           0.210   0.710
     *  blue            0.150   0.060
     *  red             0.640   0.330
     *  white (D65)     0.3127  0.3290
     *
     * Gamma: 2.2
     */
    ADOBE_RGB = 8,
    /**
     * DISPLAY_P3 is a color space that uses the DCI_P3 primaries,
     * the D65 white point and the SRGB transfer functions.
     * Rendering Intent: Colorimetric
     * Primaries:
     *                  x       y
     *  green           0.265   0.690
     *  blue            0.150   0.060
     *  red             0.680   0.320
     *  white (D65)     0.3127  0.3290
     *
     * PC/Internet (sRGB) Gamma Correction (GC):
     *
     *  if Vlinear ≤ 0.0030186
     *    Vnonlinear = 12.92 * Vlinear
     *  else
     *    Vnonlinear = 1.055 * (Vlinear)^(1/2.4) – 0.055
     *
     * Note: In most cases sRGB transfer function will be fine.
     */
    DISPLAY_P3 = 9,
    /**
     * BT2020 corresponds with display settings that implement the ITU-R
     * Recommendation BT.2020 / Rec. 2020 for UHDTV.
     *
     * Primaries:
     *                  x       y
     *  green           0.170   0.797
     *  blue            0.131   0.046
     *  red             0.708   0.292
     *  white (D65)     0.3127  0.3290
     *
     * Inverse Gamma Correction (IGC): V represents normalized (with [0 to 1]
     * range) value of R, G, or B.
     *
     *  if Vnonlinear < b * 4.5
     *    Vlinear = Vnonlinear / 4.5
     *  else
     *    Vlinear = ((Vnonlinear + (a - 1)) / a) ^ (1/0.45)
     *
     * Gamma Correction (GC):
     *
     *  if Vlinear < b
     *    Vnonlinear = 4.5 * Vlinear
     *  else
     *    Vnonlinear = a * Vlinear ^ 0.45 - (a - 1)
     *
     * where
     *
     *   a = 1.09929682680944, b = 0.018053968510807
     *
     * For practical purposes, these a/b values can be used instead
     *
     *   a = 1.099, b = 0.018 for 10-bit display systems
     *   a = 1.0993, b = 0.0181 for 12-bit display systems
     */
    BT2020 = 10,
    /**
     * BT2100_PQ and BT2100_HLG correspond with display settings that
     * implement the ITU-R Recommendation BT.2100 / Rec. 2100 for HDR TV.
     *
     * Primaries:
     *                  x       y
     *  green           0.170   0.797
     *  blue            0.131   0.046
     *  red             0.708   0.292
     *  white (D65)     0.3127  0.3290
     *
     * For BT2100_PQ, the transfer function is Perceptual Quantizer (PQ). For
     * BT2100_HLG, the transfer function is Hybrid Log-Gamma (HLG).
     */
    BT2100_PQ = 11,
    BT2100_HLG = 12,
    /**
     * DISPLAY_BT2020 corresponds with display settings that implement the ITU-R
     * Recommendation BT.2020 / Rec. 2020 for UHDTV, but specifies an SRGB
     * transfer function.
     *
     * Primaries:
     *                  x       y
     *  green           0.170   0.797
     *  blue            0.131   0.046
     *  red             0.708   0.292
     *  white (D65)     0.3127  0.3290
     *
     * Transfer Function is sRGB
     */
    DISPLAY_BT2020 = 13,
}
