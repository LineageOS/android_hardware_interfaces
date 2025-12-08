/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.gnss.gnss_assistance;

/**
 * Represents an ionospheric model as a single-layer 2D grid with a constant height.
 * This is also referred to as a thin-shell model.
 *
 * The format is a simplified adaptation of the IONEX (IONosphere Map EXchange) specification.
 *
 * IONEX: The IONosphere Map EXchange
 * Format Version 1
 * https://igs.org/formats-and-standards/
 *
 * See the "Application of IONEX TEC Maps" in the IONEX specification for
 * procedures on interpolating between consecutive epochs and grid points.
 *
 * @hide
 */
@VintfStability
parcelable IonexAssistance {
    /**
     * Metadata describing the grid dimensions, height, and mapping function that applies to the
     * tecMapSnapshot in this model.
     */
    Header header;

    /** A TEC map at a moment in time. */
    TecMapSnapshot tecMapSnapshot;

    /**
     * Defines the metadata for the ionospheric grid model.
     *
     * @hide
     */
    @VintfStability
    parcelable Header {
        /**
         * Enumeration of mapping functions for TEC determination.
         */
        @VintfStability
        @Backing(type="int")
        enum MappingFunction {
            /** No mapping function. */
            NONE = 0,
            /** 1/cos(z) mapping function. */
            COSZ = 1,
            /** Q-factor mapping function. */
            QFAC = 2,
        }
        /**
         * The mapping function adopted for TEC determination.
         * e.g. "NONE" (no mapping function), "COSZ" (1/cos(z)).
         */
        MappingFunction mappingFunction;

        /** The mean earth radius in kilometers. */
        float baseRadiusKm;

        /**
         * The height of the ionospheric layer, measured from the surface of the earth in
         * kilometers.
         */
        float heightKm;

        /** The definition of the latitude and longitude axes for the grid. */
        Axes axesInfo;
    }

    /**
     * Defines the geographic latitude-longitude axes for the TEC maps.
     *
     * @hide
     */
    @VintfStability
    parcelable Axes {
        /** The definition for the latitude axis. */
        Axis latitudeAxis;
        /** The definition for the longitude axis. */
        Axis longitudeAxis;
    }

    /**
     * Defines the limits and resolution of a single grid axis (latitude or longitude).
     *
     * For example, an axis with the sequence of points [87.5, 85.0, ..., -87.5] is defined by:
     * - startDeg: 87.5
     * - deltaDeg: -2.5
     * - numPoints: 71
     *
     * @hide
     */
    @VintfStability
    parcelable Axis {
        /** The starting value of the axis, in degrees. */
        double startDeg;

        /** The step size between grid points, in degrees. May be negative. */
        double deltaDeg;

        /** The total number of grid points along this axis. */
        int numPoints;
    }

    /**
     * Represents a Total Electron Content (TEC) map at a specific moment in time.
     *
     * @hide
     */
    @VintfStability
    parcelable TecMapSnapshot {
        /**
         * The epoch of the TEC map, in seconds since the Unix epoch (UTC).
         */
        long epochTimeSeconds;

        /**
         * A flattened 2D array of values in Total Electron Content units (TECU), where
         * 1 TECU = 10^16 electrons/m^2.
         *
         * The Ionospheric delay, in meters, of a signal propagating from the zenith
         * is given by the formula:
         *
         * (40.3 / f^2) * (VTEC * 10^16)
         *
         * Where:
         * - 40.3 is a constant in m^3/s^2
         * - f is the signal frequency in Hz
         * - VTEC is in TECU (1 TECU = 10^16 electrons/m^2)
         *
         * See: "NTCM-G Ionospheric Model Description." (2022)
         *
         * The data is organized in row-major order: latitudes are rows, longitudes are columns.
         * (lat1, lon1), (lat1, lon2), ..., (lat1, lonN), (lat2, lon1), ...
         *
         * The total number of values must be:
         * latitudeAxis.numPoints * longitudeAxis.numPoints
         *
         * Non-available TEC values are represented as NaN.
         *
         * To compute the latitude and longitude for a given zero-based index i:
         * n = latitudeAxis.numPoints
         * latitude_deg = latitudeAxis.startDeg + (i / n) * latitudeAxis.deltaDeg
         * longitude_deg = longitudeAxis.startDeg + (i % n) * longitudeAxis.deltaDeg
         */
        float[] tecMap;

        /**
         * An optional flattened 2D array of TEC Root-Mean-Square (RMS) error values in TECU.
         * Values are formatted exactly in the same way as TEC values.
         * If not available, this array will be empty.
         */
        float[] rmsMap;
    }
}
