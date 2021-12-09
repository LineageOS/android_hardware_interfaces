/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.net.nlinterceptor;

import android.hardware.net.nlinterceptor.InterceptedSocket;

/**
 * Netlink Interceptor
 *
 * This HAL provides a way for Android services to route their Netlink traffic to a location other
 * than the Kernel. One might want to do this for a variety of reasons:
 * -> Route Netlink traffic to a different host.
 * -> Route Netlink traffic to a different VM.
 * -> Convert Netlink commands into proprietary vendor hardware commands.
 *
 * Some important notes regarding Netlink Interceptor.
 * -> All int values are treated as unsigned.
 * -> Users of Netlink Interceptor must close their sockets with closeSocket manually.
 * -> PID != process ID. In this case, it is "port ID", a unique number assigned by the kernel to a
 *    given Netlink socket.
 * -> Netlink PIDs are only unique per family. This means that for all NETLINK_GENERIC sockets,
 *    there can only be one socket with PID "1234". HOWEVER, there can ALSO be a Netlink socket
 *    using NETLINK_ROUTE which has a PID of "1234". Hence, in order to uniquely identify a Netlink
 *    socket, both the PID and Netlink Family are required.
 */
@VintfStability
interface IInterceptor {
    /**
     * Creates a Netlink socket on both the HU and TCU, and a bi-directional gRPC stream to carry
     * data between them. This must be closed by the caller with closeSocket().
     *
     * @param nlFamily - Netlink Family. Support for families other than NETLINK_GENERIC is still
     * experimental.
     * @param clientNlPid - Port ID of the caller's Netlink socket.
     * @param clientName - Human readable name of the caller. Used for debugging.
     *
     * @return InterceptedSocket identifying the socket on the HU allocated for the caller.
     */
    InterceptedSocket createSocket(in int nlFamily, in int clientNlPid, in String clientName);

    /**
     * Closes a socket and gRPC stream given the socket's identifier. This must be invoked manually
     * by the caller of createSocket().
     *
     * @param handle - unique identifier for a socket returned by createSocket.
     */
    void closeSocket(in InterceptedSocket handle);

    /**
     * Subscribes a socket on the TCU to a Netlink multicast group.
     *
     * @param handle - unique identifier for a socket returned by createSocket.
     * @param nlGroup - A single Netlink multicast group that the caller wants to subscribe to.
     */
    void subscribeGroup(in InterceptedSocket handle, in int nlGroup);

    /**
     * Unsubscribes a socket on the TCU from a Netlink multicast group.
     *
     * @param handle - unique identifier for a socket returned by createSocket.
     * @param nlGroup - A single Netlink multicast group that the caller wants to unsubscribe from.
     */
    void unsubscribeGroup(in InterceptedSocket handle, in int nlGroup);
}
