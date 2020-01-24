#include <android-base/logging.h>
#include <getopt.h>
#include <unistd.h>

#include "vhal_v2_0/virtualization/GrpcVehicleServer.h"
#include "vhal_v2_0/virtualization/Utils.h"

int main(int argc, char* argv[]) {
    namespace vhal_impl = android::hardware::automotive::vehicle::V2_0::impl;

    vhal_impl::VsockServerInfo serverInfo;

    // unique values to identify the options
    constexpr int OPT_VHAL_SERVER_CID = 1001;
    constexpr int OPT_VHAL_SERVER_PORT_NUMBER = 1002;

    struct option longOptions[] = {
            {"server_cid", 1, 0, OPT_VHAL_SERVER_CID},
            {"server_port", 1, 0, OPT_VHAL_SERVER_PORT_NUMBER},
            {nullptr, 0, nullptr, 0},
    };

    int optValue;
    while ((optValue = getopt_long_only(argc, argv, ":", longOptions, 0)) != -1) {
        switch (optValue) {
            case OPT_VHAL_SERVER_CID:
                serverInfo.serverCid = std::atoi(optarg);
                LOG(DEBUG) << "Vehicle HAL server CID: " << serverInfo.serverCid;
                break;
            case OPT_VHAL_SERVER_PORT_NUMBER:
                serverInfo.serverPort = std::atoi(optarg);
                LOG(DEBUG) << "Vehicle HAL server port: " << serverInfo.serverPort;
                break;
            default:
                // ignore other options
                break;
        }
    }

    if (serverInfo.serverCid == 0 || serverInfo.serverPort == 0) {
        LOG(FATAL) << "Invalid server information, CID: " << serverInfo.serverCid
                   << "; port: " << serverInfo.serverPort;
        // Will abort after logging
    }

    auto server = vhal_impl::makeGrpcVehicleServer(vhal_impl::getVsockUri(serverInfo));
    server->Start();
    return 0;
}
