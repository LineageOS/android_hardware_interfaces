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

#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#ifndef HOST
#include "ApPowerControl.h"
#endif  // #ifndef HOST

#include "TestWakeupClientServiceImpl.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

namespace {

using ::android::hardware::automotive::remoteaccess::BOOTUP_REASON_SYSTEM_ENTER_GARAGE_MODE;
using ::android::hardware::automotive::remoteaccess::BOOTUP_REASON_SYSTEM_REMOTE_ACCESS;
using ::android::hardware::automotive::remoteaccess::BOOTUP_REASON_USER_POWER_ON;
using ::android::hardware::automotive::remoteaccess::PowerControllerServiceImpl;
using ::android::hardware::automotive::remoteaccess::ServiceImpl;
using ::android::hardware::automotive::remoteaccess::WakeupClientServiceImpl;
using ::grpc::Server;
using ::grpc::ServerBuilder;
using ::grpc::ServerWriter;

constexpr int SHUTDOWN_REQUEST = 289410889;
constexpr int VEHICLE_IN_USE = 287313738;
constexpr char COMMAND_RUN_EMU_LOCAL_IMAGE[] =
        "source ~/.aae-toolbox/bin/bashrc && aae emulator run";
constexpr char COMMAND_RUN_EMU[] = "./launch_emu.sh -v \"-writable-system -selinux permissive\"";
constexpr char COMMAND_SET_VHAL_PROP[] =
        "adb -s emulator-5554 wait-for-device && adb -s emulator-5554 root "
        "&& sleep 1 && adb -s emulator-5554 wait-for-device && adb -s emulator-5554 shell "
        "dumpsys android.hardware.automotive.vehicle.IVehicle/default --set %d -i %d";

pid_t emuPid = 0;
const char* runEmuCommand = COMMAND_RUN_EMU;

}  // namespace

void RunServer(const std::string& serviceAddr, std::shared_ptr<ServiceImpl> service) {
    ServerBuilder builder;
    builder.AddListeningPort(serviceAddr, grpc::InsecureServerCredentials());
    WakeupClientServiceImpl wakeupClientService(service.get());
    builder.RegisterService(&wakeupClientService);
    PowerControllerServiceImpl powerControllerService(service.get());
    builder.RegisterService(&powerControllerService);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    printf("Test Remote Access GRPC Server listening on %s\n", serviceAddr.c_str());
    server->Wait();
}

pid_t runCommand(const char* bashCommand) {
    pid_t pid = fork();
    if (pid == 0) {
        // In child process. Put it into a separate process group so we can kill it.
        setpgid(0, 0);
        execl("/bin/bash", "bash", "-c", bashCommand, /*terminateArg=*/nullptr);
        exit(0);
    } else {
        return pid;
    }
}

void updateEmuStatus() {
    if (emuPid == 0) {
        return;
    }
    pid_t pid = waitpid(emuPid, nullptr, WNOHANG);
    if (pid == emuPid) {
        // Emu process already exited. If Emu process is still running, pid will be 0.
        emuPid = 0;
    }
}

bool powerOnEmu(ServiceImpl* service, int32_t bootupReason) {
    updateEmuStatus();
    if (emuPid != 0) {
        printf("The emulator is already running\n");
        return false;
    }
    service->setBootupReason(bootupReason);
    emuPid = runCommand(runEmuCommand);
    printf("Emulator started in process: %d\n", emuPid);
    return true;
}

bool powerOn(ServiceImpl* service, int32_t bootupReason) {
#ifdef HOST
    return powerOnEmu(service, bootupReason);
#else
    printf("power on is only supported on host\n");
    return false;
#endif
}

const char* getSetPropCommand(int propId, int value) {
    int size = snprintf(nullptr, 0, COMMAND_SET_VHAL_PROP, propId, value);
    char* command = new char[size + 1];
    snprintf(command, size + 1, COMMAND_SET_VHAL_PROP, propId, value);
    return command;
}

const char* getSetPropCommand(int propId) {
    return getSetPropCommand(propId, /*value=*/1);
}

void powerOffEmu() {
    updateEmuStatus();
    if (emuPid == 0) {
        printf("The emulator is not running\n");
        return;
    }
    const char* command = getSetPropCommand(SHUTDOWN_REQUEST);
    runCommand(command);
    delete[] command;
    waitpid(emuPid, nullptr, /*options=*/0);
    emuPid = 0;
}

void powerOff() {
#ifdef HOST
    powerOffEmu();
#else
    printf("power off is only supported on host\n");
#endif
}

void help() {
    std::cout << "Remote Access Host Test Utility" << std::endl
              << "help:\t"
              << "Print out this help info" << std::endl
              << "genFakeTask start [clientID]:\t"
              << "Start generating a fake task every 5s" << std::endl
              << "genFakeTask stop:\t"
              << "Stop the fake task generation" << std::endl
              << "status:\t"
              << "Print current status" << std::endl
              << "power on:\t"
              << "Power on the emulator, simulate user enters vehicle while AP is off"
              << " (only supported on host)" << std::endl
              << "power off:\t"
              << "Power off the emulator, simulate user leaves vehicle"
              << " (only supported on host)" << std::endl
              << "inject task [clientID] [taskData]:\t"
              << "Inject a remote task" << std::endl
              << "set vehicleInUse:\t"
              << "Set vehicle in use, simulate user enter vehicle while boot up for remote task "
              << "(only supported on host)" << std::endl;
}

void parseCommand(const std::string& userInput, std::shared_ptr<ServiceImpl> service) {
    if (userInput == "") {
        // ignore empty line.
    } else if (userInput == "help") {
        help();
    } else if (userInput.rfind("genFakeTask start", 0) == 0) {
        std::string clientId;
        std::stringstream ss;
        ss << userInput;
        int i = 0;
        while (std::getline(ss, clientId, ' ')) {
            i++;
            if (i == 3) {
                break;
            }
        }
        if (i != 3) {
            printf("Missing clientId, see 'help'\n");
            return;
        }
        service->startGeneratingFakeTask(clientId);
    } else if (userInput == "genFakeTask stop") {
        service->stopGeneratingFakeTask();
    } else if (userInput == "status") {
        printf("isWakeupRequired: %B, isRemoteTaskConnectionAlive: %B\n",
               service->isWakeupRequired(), service->isRemoteTaskConnectionAlive());
    } else if (userInput == "power on") {
        service->setVehicleInUse(true);
        powerOn(service.get(), BOOTUP_REASON_USER_POWER_ON);
    } else if (userInput == "power off") {
        service->setVehicleInUse(false);
        powerOff();
    } else if (userInput.rfind("inject task", 0) == 0) {
        std::stringstream ss;
        ss << userInput;
        std::string data;
        std::string taskData;
        std::string clientId;
        int i = 0;
        while (std::getline(ss, data, ' ')) {
            i++;
            if (i == 3) {
                clientId = data;
            }
            if (i == 4) {
                taskData = data;
            }
        }
        if (taskData == "" || clientId == "") {
            printf("Missing taskData or clientId, see 'help'\n");
            return;
        }
        service->injectTask(taskData, clientId);
        printf("Remote task with client ID: %s, data: %s injected\n", clientId.c_str(),
               taskData.c_str());
    } else if (userInput == "set vehicleInUse") {
        service->setVehicleInUse(true);
    } else {
        printf("Unknown command, see 'help'\n");
    }
}

void saHandler(int signum) {
    if (emuPid != 0) {
        kill(-emuPid, signum);
        waitpid(emuPid, nullptr, /*options=*/0);
        // Sleep for 1 seconds to allow emulator to print out logs.
        sleep(1);
    }
    exit(-1);
}

class MyServiceImpl final : public ServiceImpl {
  public:
    void wakeupApplicationProcessor(int32_t bootupReason) override {
#ifdef HOST
        powerOnEmu(this, bootupReason);
#else
        wakeupAp();
#endif
    };
};

// Usage: TestWakeupClientServerHost [--local-image] [service_address_to_start]
int main(int argc, char** argv) {
    std::string serviceAddr = GRPC_SERVICE_ADDRESS;
    for (int i = 1; i < argc; i++) {
        char* arg = argv[1];
        if (strcmp(arg, "--local-image") == 0) {
            runEmuCommand = COMMAND_RUN_EMU_LOCAL_IMAGE;
            continue;
        }
        serviceAddr = arg;
    }
    // Let the server thread run, we will force kill the server when we exit the program.
    std::shared_ptr<ServiceImpl> service = std::make_shared<MyServiceImpl>();
    std::thread serverThread([serviceAddr, service] { RunServer(serviceAddr, service); });

    // Register the signal handler for SIGTERM and SIGINT so that we can stop the emulator before
    // exit.
    struct sigaction sa = {};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = saHandler;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // Start processing the user inputs.
    std::string userInput;
    while (true) {
        std::cout << ">>> ";
        std::getline(std::cin, userInput);
        parseCommand(userInput, service);
    }
    return 0;
}
