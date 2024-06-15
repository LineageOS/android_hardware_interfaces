#include "IpStreamer.h"

IpStreamer::IpStreamer() {}

IpStreamer::~IpStreamer() {}

void IpStreamer::startIpStream() {
    ALOGI("Starting IP Stream thread");
    mFp = fopen(mFilePath.c_str(), "rb");
    if (mFp == nullptr) {
        ALOGE("Failed to open file at path: %s", mFilePath.c_str());
        return;
    }
    mIpStreamerThread = std::thread(&IpStreamer::ipStreamThreadLoop, this, mFp);
}

void IpStreamer::stopIpStream() {
    ALOGI("Stopping IP Stream thread");
    close(mSockfd);
    if (mFp != nullptr) fclose(mFp);
    if (mIpStreamerThread.joinable()) {
        mIpStreamerThread.join();
    }
}

void IpStreamer::ipStreamThreadLoop(FILE* fp) {
    mSockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (mSockfd < 0) {
        ALOGE("IpStreamer::ipStreamThreadLoop: Socket creation failed (%s)", strerror(errno));
        exit(1);
    }

    if (mFp == NULL) {
        ALOGE("IpStreamer::ipStreamThreadLoop: Cannot open file %s: (%s)", mFilePath.c_str(),
              strerror(errno));
        exit(1);
    }

    struct sockaddr_in destaddr;
    memset(&destaddr, 0, sizeof(destaddr));
    destaddr.sin_family = mIsIpV4 ? AF_INET : AF_INET6;
    destaddr.sin_port = htons(mPort);
    destaddr.sin_addr.s_addr = inet_addr(mIpAddress.c_str());

    char buf[mBufferSize];
    int n;
    while (1) {
        if (fp == nullptr) break;
        n = fread(buf, 1, mBufferSize, fp);
        ALOGI("IpStreamer::ipStreamThreadLoop: Bytes read from fread(): %d\n", n);
        if (n <= 0) {
            break;
        }
        sendto(mSockfd, buf, n, 0, (struct sockaddr*)&destaddr, sizeof(destaddr));
        sleep(mSleepTime);
    }
}
