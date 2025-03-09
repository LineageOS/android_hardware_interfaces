package android.hardware.gnss.gnss_assistance;

/**
 * Contains Glonass almanac data.
 * This is defined in Glonass ICD v5.1, Section 4.5.
 *
 * @hide
 */
@VintfStability
parcelable GlonassAlmanac {
    /**
     * Contains Glonass satellite almanac data.
     * This is defined in Glonass ICD v5.1, Section 4.5.
     */
    @VintfStability
    parcelable GlonassSatelliteAlmanac {
        /** Slot number. */
        int slotNumber;

        /** Satellite health (0=healthy, 1=unhealthy). */
        int svHealth;

        /** Frequency channel number. */
        int frequencyChannel;

        /** Coarse value of satellite time correction to GLONASS time in seconds. */
        double tau;

        /** Time of first ascending node passage of satellite in seconds. */
        double tLambda;

        /** Longitude of the first ascending node in semi-circles. */
        double lambda;

        /** Correction to the mean value of inclination in semi-circles. */
        double deltaI;

        /** Correction to the mean value of the draconian period in seconds per orbital period. */
        double deltaT;

        /** Rate of change of draconian period in seconds per orbital period squared. */
        double deltaTDot;

        /** Eccentricity. */
        double eccentricity;

        /** Argument of perigee in radians. */
        double omega;
    }

    /** Almanac issue date in milliseconds (UTC). */
    long issueDateMs;

    /** Array of GlonassSatelliteAlmanac. */
    GlonassSatelliteAlmanac[] satelliteAlmanac;
}
