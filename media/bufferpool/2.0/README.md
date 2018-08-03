1. Overview

A buffer pool enables processes to transfer buffers asynchronously.
Without a buffer pool, a process calls a synchronous method of the other
process and waits until the call finishes transferring a buffer. This adds
unwanted latency due to context switching. With help from a buffer pool, a
process can pass buffers asynchronously and reduce context switching latency.

Passing an interface and a handle adds extra latency also. To mitigate the
latency, passing IDs with local cache is used. For security concerns about
rogue clients, FMQ is used to communicate between a buffer pool and a client
process. FMQ is used to send buffer ownership change status from a client
process to a buffer pool. Except FMQ, a buffer pool does not use any shared
memory.

2. FMQ

FMQ is used to send buffer ownership status changes to a buffer pool from a
buffer pool client. A buffer pool synchronizes FMQ messages when there is a
hidl request from the clients. Every client has its own connection and FMQ
to communicate with the buffer pool. So sending an FMQ message on behalf of
other clients is not possible.

FMQ messages are sent when a buffer is acquired or released. Also, FMQ messages
are sent when a buffer is transferred from a client to another client. FMQ has
its own ID from a buffer pool. A client is specified with the ID.

To transfer a buffer, a sender must send an FMQ message. The message must
include a receiver's ID and a transaction ID. A receiver must send the
transaction ID to fetch a buffer from a buffer pool. Since the sender already
registered the receiver via an FMQ message, The buffer pool must verify the
receiver with the transaction ID. In order to prevent faking a receiver, a
connection to a buffer pool from client is made and kept privately. Also part of
transaction ID is a sender ID in order to prevent fake transactions from other
clients. This must be verified with an FMQ message from a buffer pool.

FMQ messages are defined in BufferStatus and BufferStatusMessage of 'types.hal'.

3. Interfaces

IConnection
A connection to a buffer pool from a buffer pool client. The connection
provides the functionalities to share buffers between buffer pool clients.
The connection must be unique for each client.

IAccessor
An accessor to a buffer pool which makes a connection to the buffer pool.
IAccesssor#connect creates an IConnection.

IClientManager
A manager of buffer pool clients and clients' connections to buffer pools. It
sets up a process to be a receiver of buffers from a buffer pool. The manager
is unique in a process.

