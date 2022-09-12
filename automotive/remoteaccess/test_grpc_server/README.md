# Test GRPC Server.

A test GRPC server that implements wakeup_client.proto. This test server acts
as a reference implementation for a remote wakeup client running on TCU. This
reference server also implements wakeup_client_debug.proto which is the
debugging interface. It is recommended that the actual implementation also
implements this test interface for easier end-to-end testing.
