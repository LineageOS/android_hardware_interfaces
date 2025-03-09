package android.hardware.gnss.gnss_assistance;

/**
 * Gnss correction associated with a component (e.g. the Ionospheric error).
 *
 * @hide
 */
@VintfStability
parcelable GnssCorrectionComponent {
    /**
     * Uniquely identifies the source of correction (e.g. "Klobuchar" for
     * ionospheric corrections).
     * Clients should not depend on the value of the source key but, rather,
     * can compare before/after to detect changes.
     */
    String sourceKey;

    /**
     * Time interval referenced against the GPS epoch. The start must be less than
     * or equal to the end. When the start equals the end, the interval is empty.
     */
    @VintfStability
    parcelable GnssInterval {
        /**
         * Inclusive start of the interval in milliseconds since the GPS epoch.
         * A timestamp matching this interval will have to be the same or after the
         * start. Required as a reference time for the initial correction value and
         * its rate of change over time.
         */
        long startMillisSinceGpsEpoch;

        /**
         * Exclusive end of the interval in milliseconds since the GPS epoch. If
         * specified, a timestamp matching this interval will have to be before the
         * end.
         */
        long endMillisSinceGpsEpoch;
    }

    /** The correction is only applicable during this time interval. */
    GnssInterval validityInterval;

    /** Pseudorange correction. */
    @VintfStability
    parcelable PseudorangeCorrection {
        /* Correction to be added to the measured pseudorange, in meters. */
        double correctionMeters;

        /* Uncertainty of the correction, in meters. */
        double correctionUncertaintyMeters;

        /**
         * Linear approximation of the change in correction over time. Intended
         * usage is to adjust the correction using the formula:
         *   correctionMeters + correctionRateMetersPerSecond * delta_seconds
         * Where `delta_seconds` is the number of elapsed seconds since the beginning
         * of the correction validity interval.
         */
        double correctionRateMetersPerSecond;
    }

    /* Pseudorange correction. */
    PseudorangeCorrection pseudorangeCorrection;
}
