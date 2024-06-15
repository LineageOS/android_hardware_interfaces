#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <log/log.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <thread>

/**
 * IP Streamer class to send TS data to a specified socket for testing IPTV frontend functions
 * e.g. tuning and playback.
 */

class IpStreamer {
  public:
    // Constructor for IP Streamer object
    IpStreamer();

    // Destructor for IP Streamer object
    ~IpStreamer();

    // Starts a thread to read data from a socket
    void startIpStream();

    // Stops the reading thread started by startIpStream
    void stopIpStream();

    // Thread function that consumes data from a socket
    void ipStreamThreadLoop(FILE* fp);

    std::string getFilePath() { return mFilePath; };

  private:
    int mSockfd = -1;
    FILE* mFp = nullptr;
    bool mIsIpV4 = true;                                         // By default, set to IPV4
    int mPort = 12345;                                           // default port
    int mBufferSize = 188;                                       // bytes
    int mSleepTime = 1;                                          // second
    std::string mIpAddress = "127.0.0.1";                        // default IP address
    std::string mFilePath = "/data/local/tmp/segment000000.ts";  // default path for TS file
    std::thread mIpStreamerThread;
};