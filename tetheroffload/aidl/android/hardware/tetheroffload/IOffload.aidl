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

package android.hardware.tetheroffload;

import android.hardware.tetheroffload.ForwardedStats;
import android.hardware.tetheroffload.ITetheringOffloadCallback;

/**
 * Interface used to control tethering offload.
 */
@VintfStability
interface IOffload {
    /**
     * Error code for all {@code ServiceSpecificException}s thrown by this interface.
     */
    const int ERROR_CODE_UNUSED = 0;

    /**
     * Indicates intent to start offload for tethering in immediate future.
     *
     * This API must be called exactly once when Tethering is requested by the user.
     *
     * If this API is called multiple times without first calling stopOffload, then the subsequent
     * calls must fail without changing the state of the server.
     *
     * If for some reason, the hardware is currently unable to support offload, this call must fail.
     *
     * @param fd1 A file descriptor bound to the following netlink groups
     *            (NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY).
     * @param fd2 A file descriptor bound to the following netlink groups
     *            (NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY).
     * @param cb Assuming success, this callback must provide unsolicited updates of offload status.
     *           It is assumed to be valid until stopOffload is called.
     *
     * @throws:
     *         - EX_ILLEGAL_ARGUMENT if any file descriptors are invalid.
     *         - EX_ILLEGAL_STATE if this method previously succeeded and stopOffload() was not
     *           later called.
     *         - EX_SERVICE_SPECIFIC with the error message set to a human-readable reason for the
     *           error.
     *
     * Remarks: Initializing offload does not imply that any upstreams or downstreams have yet been,
     * or even will be, chosen.  This API is symmetrical with stopOffload.
     */
    void initOffload(in ParcelFileDescriptor fd1, in ParcelFileDescriptor fd2,
            in ITetheringOffloadCallback cb);

    /**
     * Indicate desire to tear down all tethering offload.
     *
     * Called after tethering is no longer requested by the user. Any remaining offload must
     * be subsequently torn down by the management process.  Upon success, the callback registered
     * in initOffload must be released, and offload must be stopped.
     *
     * @throws:
     *         - EX_ILLEGAL_STATE if initOffload() was not called, or if stopOffload() was already
     *           called.
     *         - EX_SERVICE_SPECIFIC with the error message set to a human-readable reason for the
     *           error.
     *
     * Remarks: Statistics must be reset by this API.
     */
    void stopOffload();

    /**
     * Instruct management process not to forward traffic destined to or from the specified
     * prefixes.
     *
     * This API may only be called after initOffload and before stopOffload.
     *
     * @param prefixes List containing fully specified prefixes. For e.g. 192.168.1.0/24
     * or 2001:4860:684::/64
     *
     * @throws:
     *         - EX_ILLEGAL_ARGUMENT if the IP prefixes are invalid.
     *         - EX_ILLEGAL_STATE if this method is called before initOffload(), or if this method
     *           is called after stopOffload().
     *         - EX_SERVICE_SPECIFIC with the error message set to a human-readable reason for the
     *           error.
     *
     * Remarks: This list overrides any previously specified list
     */
    void setLocalPrefixes(in String[] prefixes);

    /**
     * Query offloaded traffic statistics forwarded to an upstream address.
     *
     * Return statistics that have transpired since the last query.  This would include
     * statistics from all offloaded downstream tether interfaces that have been forwarded to this
     * upstream interface.  After returning the statistics, the counters are reset to zero.
     *
     * Only offloaded statistics must be returned by this API, software stats must not be
     * returned.
     *
     * @param upstream Upstream interface on which traffic exited/entered
     *
     * @return ForwardedStats depicting the received and transmitted bytes
     *
     * @throws:
     *         - EX_SERVICE_SPECIFIC with the error message set to a human-readable reason for the
     *           error.
     */
    ForwardedStats getForwardedStats(in String upstream);

    /**
     * Instruct hardware to send callbacks, and possibly stop offload, after certain number of bytes
     * have been transferred in either direction on this upstream interface.
     *
     * The specified quota bytes must be applied to all traffic on the given upstream interface.
     * This includes hardware forwarded traffic, software forwarded traffic, and AP-originated
     * traffic. IPv4 and IPv6 traffic both count towards the same quota. IP headers are included
     * in the byte count quota, but, link-layer headers are not.
     *
     * This API may only be called while offload is occurring on this upstream. The hardware
     * management process MUST NOT store the values when offload is not started and apply them
     * once offload is started. This is because the quota values would likely become stale over
     * time and would not reflect any new traffic that has occurred.
     *
     * The specified quota bytes MUST replace any previous quotas set by
     * {@code setDataWarningAndLimit} specified on the same interface. It may be interpreted as
     * "tell me when either <warningBytes> or <limitBytes> bytes have been transferred
     * (in either direction), and stop offload when <limitBytes> bytes have been transferred,
     * starting now and counting from zero on <upstream>."
     *
     * Once the {@code warningBytes} is reached, the callback registered in initOffload must be
     * called with {@code OFFLOAD_WARNING_REACHED} to indicate this event. Once the event fires
     * for this upstream, no further {@code OFFLOAD_WARNING_REACHED} event will be fired for this
     * upstream unless this method is called again with the same interface. Note that there is
     * no need to call initOffload again to resume offload if stopOffload was not called by the
     * client.
     *
     * Similarly, Once the {@code limitBytes} is reached, the callback registered in initOffload
     * must be called with {@code OFFLOAD_STOPPED_LIMIT_REACHED} to indicate this event. Once
     * the event fires for this upstream, no further {@code OFFLOAD_STOPPED_LIMIT_REACHED}
     * event will be fired for this upstream unless this method is called again with the same
     * interface. However, unlike {@code warningBytes}, when {@code limitBytes} is reached,
     * all offload must be stopped. If offload is desired again, the hardware management
     * process must be completely reprogrammed by calling setUpstreamParameters and
     * addDownstream again.
     *
     * Note that {@code warningBytes} must always be less than or equal to {@code limitBytes},
     * when {@code warningBytes} is reached, {@code limitBytes} may still valid unless this method
     * is called again with the same interface.
     *
     * @param upstream Upstream interface name that quota must apply to.
     * @param warningBytes The quota of warning, defined as the number of bytes, starting from
     *                     zero and counting from now.
     * @param limitBytes The quota of limit, defined as the number of bytes, starting from zero
     *                   and counting from now.
     *
     * @throws:
     *         - EX_ILLEGAL_ARGUMENT if any parameters are invalid (such as invalid upstream
     *           or negative number of bytes).
     *         - EX_ILLEGAL_STATE if this method is called before initOffload(), or if this method
     *           is called after stopOffload().
     *         - EX_SERVICE_SPECIFIC with the error message set to a human-readable reason for the
     *           error.
     */
    void setDataWarningAndLimit(in String upstream, in long warningBytes, in long limitBytes);

    /**
     * Instruct hardware to start forwarding traffic to the specified upstream.
     *
     * When iface, v4Addr, and v4Gw are all non-null, the management process may begin forwarding
     * any currently configured or future configured IPv4 downstreams to this upstream interface.
     *
     * If any of the previously three mentioned parameters are null, then any current IPv4 offload
     * must be stopped.
     *
     * When iface and v6Gws are both non-null, and in the case of v6Gws, are not empty, the
     * management process may begin forwarding any currently configured or future configured IPv6
     * downstreams to this upstream interface.
     *
     * If either of the two above parameters are null, or no V6 Gateways are provided, then IPv6
     * offload must be stopped.
     *
     * This API may only be called after initOffload and before stopOffload.
     *
     * @param iface  Upstream interface name.  Note that only one is needed because IPv4 and IPv6
     *               interfaces cannot be different (only known that this can occur during software
     *               xlat, which cannot be offloaded through hardware anyways).  If the iface is
     *               null, offload must be stopped.
     * @param v4Addr The local IPv4 address assigned to the provided upstream interface, i.e. the
     *               IPv4 address the packets are NATed to. For e.g. 192.168.0.12.
     * @param v4Gw   The IPv4 address of the IPv4 gateway on the upstream interface.
     *               For e.g. 192.168.1.1
     * @param v6Gws  A list of IPv6 addresses (for e.g. fe80::97be:9de7:b24b:9194) for possible IPv6
     *               gateways on the upstream interface.
     *
     * @throws:
     *         - EX_ILLEGAL_ARGUMENT if any parameters are invalid (such as invalid upstream
     *           or IP addresses).
     *         - EX_ILLEGAL_STATE if this method is called before initOffload(), or if this method
     *           is called after stopOffload().
     *         - EX_SERVICE_SPECIFIC with the error message set to a human-readable reason for the
     *           error.
     *
     * Remarks: This overrides any previously configured parameters.
     */
    void setUpstreamParameters(
            in String iface, in String v4Addr, in String v4Gw, in String[] v6Gws);

    /**
     * Configure a downstream interface and prefix in the hardware management process that may be
     * forwarded.
     *
     * The prefix may be an IPv4 or an IPv6 prefix to signify which family can be offloaded from
     * the specified tether interface.  The list of IPv4 and IPv6 downstreams that are configured
     * may differ.
     *
     * If the given protocol, as determined by the prefix, has an upstream set,
     * the hardware may begin forwarding traffic between the upstream and any devices on the
     * downstream interface that have IP addresses within the specified prefix. Other traffic from
     * the same downstream interfaces is unaffected and must be forwarded if and only if it was
     * already being forwarded.
     *
     * If no upstream is currently configured, then these downstream interface and prefixes must be
     * preserved so that offload may begin in the future when an upstream is set.
     *
     * This API does not replace any previously configured downstreams and any such downstreams must
     * be explicitly removed by calling removeDownstream.
     *
     * This API may only be called after initOffload and before stopOffload.
     *
     * @param iface  Downstream interface
     * @param prefix Downstream prefix depicting addresses that may be offloaded.
     *               For e.g. 192.168.1.0/24 or 2001:4860:684::/64)
     *
     * @throws:
     *         - EX_ILLEGAL_ARGUMENT if any parameters are invalid (such as invalid downstream
     *           or IP prefix).
     *         - EX_ILLEGAL_STATE if this method is called before initOffload(), or if this method
     *           is called after stopOffload().
     *         - EX_SERVICE_SPECIFIC with the error message set to a human-readable reason for the
     *           error.
     *
     * Remarks: The hardware management process may fail this call in a normal situation.  This can
     *          happen because the hardware cannot support the current number of prefixes, the
     *          hardware cannot support concurrent offload on multiple interfaces, the hardware
     *          cannot currently support offload on the tether interface for some reason, or any
     *          other dynamic configuration issues which may occur.  In this case,
     *          traffic must remain unaffected and must be forwarded if and only if it was already
     *          being forwarded.
     */
    void addDownstream(in String iface, in String prefix);

    /**
     * Remove a downstream prefix that may be forwarded from the hardware management process.
     *
     * The prefix may be an IPv4 or an IPv6 prefix. If it was not previously configured using
     * addDownstream, then this must be a no-op.
     *
     * This API may only be called after initOffload and before stopOffload.
     *
     * @param iface  Downstream interface
     * @param prefix Downstream prefix depicting prefix that must no longer be offloaded
     *               For e.g. 192.168.1.0/24 or 2001:4860:684::/64)
     *
     * @throws:
     *         - EX_ILLEGAL_ARGUMENT if any parameters are invalid (such as invalid downstream
     *           or IP prefix).
     *         - EX_ILLEGAL_STATE if this method is called before initOffload(), or if this method
     *           is called after stopOffload().
     *         - EX_SERVICE_SPECIFIC with the error message set to a human-readable reason for the
     *           error.
     */
    void removeDownstream(in String iface, in String prefix);
}
