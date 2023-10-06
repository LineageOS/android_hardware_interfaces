package android.hardware.wifi.supplicant;

/**
 * Enum values for QoS policy response status.
 */
@VintfStability
@Backing(type="int")
enum QosPolicyScsResponseStatusCode {
    SUCCESS,
    /**
     * Network policy does not permit the stream to be assigned the requested
     * user priority (UP), but the AP might accept another request from the STA
     * with the same TCLAS classifier(s) but a different user priority (UP).
     */
    TCLAS_REQUEST_DECLINED,
    /**
     * Requested TCLAS processing is not supported by the AP.
     */
    TCLAS_NOT_SUPPORTED_BY_AP,
    /**
     * The AP has insufficient TCLAS processing resources to satisfy the request
     * (i.e. to classify and process the traffic).
     */
    TCLAS_INSUFFICIENT_RESOURCES,
    /**
     * Sufficient TCLAS processing resources were available when the SCS
     * stream was created, but are no longer available.
     */
    TCLAS_RESOURCES_EXHAUSTED,
    /**
     * Insufficient capacity to sustain the current QoS treatment.
     */
    TCLAS_PROCESSING_TERMINATED_INSUFFICIENT_QOS,
    /**
     * Conflict with a (new or dynamic) network policy.
     */
    TCLAS_PROCESSING_TERMINATED_POLICY_CONFLICT,
    /**
     * Other reason for decline.
     */
    TCLAS_PROCESSING_TERMINATED,
    /**
     * AP did not send a response within the timeout period (1 sec).
     */
    TIMEOUT,
}
