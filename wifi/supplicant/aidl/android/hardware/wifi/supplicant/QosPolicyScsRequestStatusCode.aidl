package android.hardware.wifi.supplicant;

/**
 * Enum values for QoS policy request status.
 */
@VintfStability
@Backing(type="int")
enum QosPolicyScsRequestStatusCode {
    /**
     * SCS request was sent to the AP.
     */
    SENT,
    /**
     * Add request conflicts with an existing policy ID.
     */
    ALREADY_ACTIVE,
    /**
     * Remove request is for a policy ID that does not exist.
     */
    NOT_EXIST,
    /**
     * QoS policy params are invalid.
     */
    INVALID,
}
