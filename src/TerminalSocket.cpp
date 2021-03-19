//
// Created by liuzikai on 3/11/21.
//

#include "TerminalSocket.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace meta {

// Get socket address, IPv4 or IPv6.
// Put it as a local function here since I don't want to import socket headers in TerminalSocket.h
static void *getClientAddr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

static bool setSocketTimeout(int sock, const struct timeval *tv) {
    if (::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, tv, sizeof(struct timeval)) == -1) {
        std::cerr << "TerminalSocketBase: setsockopt failed\n";
        return false;
    }
    return true;
}

TerminalSocketBase::~TerminalSocketBase() {
    closeSocket();
}

bool TerminalSocketBase::sendSingleString(const string &name, const string &s) {
    if (!connected) return false;

    vector<uint8_t> buf;
    size_t bufSize = prepareHeader(buf, SINGLE_STRING, name, s.length() + 1);

    // The string itself
    buf.insert(buf.end(), s.begin(), s.end());
    buf.emplace_back('\0');

    // Send the data
    auto bytesSent = ::send(sockfd, buf.data(), bufSize, 0);
    if (bytesSent != bufSize) {
        std::cerr << "TerminalSocketBase: only " << bytesSent << "/" << bufSize << " bytes are sent\n";
        return false;
    }

    return true;
}

bool TerminalSocketBase::sendSingleInt(const string &name, int32_t n) {
    if (!connected) return false;

    vector<uint8_t> buf;
    size_t bufSize = prepareHeader(buf, SINGLE_INT, name, 4);

    // The int itself
    emplaceInt32(buf, (uint32_t) n);

    // Send the data
    ::send(sockfd, buf.data(), bufSize, 0);

    return true;
}

bool TerminalSocketBase::sendBytes(const string &name, uint8_t *data, size_t size) {
    if (!connected) return false;

    vector<uint8_t> buf;
    size_t bufSize = prepareHeader(buf, BYTES, name, size);

    // Data
    buf.resize(bufSize);
    ::memcpy(buf.data() + (bufSize - size), data, size);

    // Send the data
    ::send(sockfd, buf.data(), bufSize, 0);

    return true;
}

bool TerminalSocketBase::sendListOfStrings(const string &name, const vector<string> &list) {
    if (!connected) return false;

    // Iterate through strings to count the size
    size_t contentSize = 0;
    for (const auto &s : list) {
        contentSize += s.length() + 1;  // include the NUL
    }

    vector<uint8_t> buf;
    size_t bufSize = prepareHeader(buf, LIST_OF_STRINGS, name, contentSize);

    // Emplace the strings
    for (const auto &s : list) {
        buf.insert(buf.end(), s.begin(), s.end());
        buf.emplace_back('\0');
    }

    // Send the data
    ::send(sockfd, buf.data(), bufSize, 0);

    return true;
}

int TerminalSocketBase::prepareHeader(vector<uint8_t> &buf, TerminalSocketBase::PackageType type, const string &name,
                                      size_t contentSize) {
    size_t bufSize = 1 + (name.length() + 1) + 4 + contentSize;
    buf.reserve(bufSize);

    // Type, 1 byte
    buf.emplace_back((uint8_t) SINGLE_STRING);

    // NUL-terminated name
    buf.insert(buf.end(), name.begin(), name.end());
    buf.emplace_back('\0');

    // Size, 4 bytes, little endian
    emplaceInt32(buf, contentSize);

    return bufSize;
}

void TerminalSocketBase::emplaceInt32(vector<uint8_t> &buf, int32_t n) {
    // Little endian
    buf.emplace_back((uint8_t) (n & 0xFF));
    buf.emplace_back((uint8_t) ((n >> 8) & 0xFF));
    buf.emplace_back((uint8_t) ((n >> 16) & 0xFF));
    buf.emplace_back((uint8_t) ((n >> 24) & 0xFF));
}

void TerminalSocketBase::setupSocket(int fd, TerminalSocketBase::DisconnectCallback disconnected) {
    if (connected || th != nullptr) {
        closeSocket();
    }

    sockfd = fd;
    disconnectCallback = disconnected;
    connected = true;

    threadShouldExit = false;
    th = new std::thread(&TerminalSocketBase::receiverThreadBody, this);
}

void TerminalSocketBase::closeSocket() {
    if (th) {
        threadShouldExit = true;
        th->join();  // will trigger disconnect callback
        delete th;
        th = nullptr;
    }

    connected = false;
    disconnectCallback = nullptr;
    sockfd = 0;
}

void TerminalSocketBase::receiverThreadBody() {

    ReceiverState state = RECV_PREAMBLE;

    vector<uint8_t> buf;  // use vector to ensure the support of large packages
    buf.resize(RECEIVER_BUFFER_SIZE * 4);  // preallocate some space

    size_t offset = 0;  // current buffer offset;

    ssize_t recvSize;
    size_t sizeRemainingByteCount;
    size_t contentRemainingByteCount;

    size_t contentSize;
    size_t contentStartIndex;

    if (!setSocketTimeout(sockfd, &RECV_TIMEOUT)) {
        // return;
    }

    while (!threadShouldExit) {

        if (state == RECV_PREAMBLE) {

            recvSize = ::recv(sockfd, buf.data(), 1, 0);  // receive only one byte
            if (recvSize == 0) {
                // Timeout, continue and check RECV_PREAMBLE
                continue;
            } else if (recvSize == -1) {
                // Error occurred
                std::cerr << "TerminalSocketBase: recv error: " << ::strerror(recvSize) << " \n";
                break;
            }
            if (buf[0] == PREAMBLE) {
                state = RECV_PACKAGE_TYPE;  // transfer to receive package type
                offset = 0;  // preamble not included in the buf, will be overwritten
            }

        } else {

            if (offset + RECEIVER_BUFFER_SIZE > buf.size()) {
                buf.resize(buf.size() * 2);  // enlarge the buffer to 2x
            }

            recvSize = ::recv(sockfd, buf.data() + offset, RECEIVER_BUFFER_SIZE, 0);
            if (recvSize == 0) {
                // Timeout, continue and check RECV_PREAMBLE
                continue;
            } else if (recvSize == -1) {
                // Error occurred
                std::cerr << "TerminalSocketBase: recv error: " << ::strerror(recvSize) << " \n";
                break;
            }

            size_t i = offset;
            offset += recvSize;  // move forward, may get overwritten below

            for (; i < offset; i++) {
                switch (state) {
                    case RECV_PACKAGE_TYPE:
                        assert(i == 0 && "Unexpected package handling at receiver. Package type should be at offset 0");
                        if (buf[i] < PACKAGE_TYPE_COUNT) {  // valid
                            state = RECV_NAME;  // transfer to receiving name
                        } else {
                            std::cerr << "Received invalid package type " << buf[i] << "\n";
                            state = RECV_PREAMBLE;
                            offset = 0;  // this will break the for loop (stop processing remaining bytes)
                        }
                        break;
                    case RECV_NAME:
                        if (buf[i] == '\0') {
                            state = RECV_SIZE;  // transfer to receive 4-byte size
                            sizeRemainingByteCount = 4;
                        }
                        break;
                    case RECV_SIZE:
                        sizeRemainingByteCount--;
                        if (sizeRemainingByteCount == 0) {
                            contentSize = contentRemainingByteCount = decodeInt32(buf.data() + (i - 3));
                            state = RECV_CONTENT;  // transfer to receive content
                            contentStartIndex = i + 1;
                        }
                        break;
                    case RECV_CONTENT:
                        contentRemainingByteCount--;
                        if (contentRemainingByteCount == 0) {

                            string name((char *) (buf.data() + 1));  // NUL will be found
                            switch ((PackageType) buf[0]) {
                                case SINGLE_STRING:
                                    if (singleStringCallBack) {
                                        singleStringCallBack(callBackParam, name,
                                                             (char *) (buf.data() + contentStartIndex));
                                    }
                                    break;
                                case SINGLE_INT:
                                    if (singleIntCallBack) {
                                        singleIntCallBack(callBackParam, name,
                                                          decodeInt32(buf.data() + contentStartIndex));
                                    }
                                    break;
                                case BYTES:
                                    if (bytesCallBack) {
                                        bytesCallBack(callBackParam, name, buf.data() + contentStartIndex, contentSize);
                                    }
                                    break;
                                case LIST_OF_STRINGS:
                                    if (listOfStringsCallBack) {
                                        vector<string> list;
                                        size_t strStart = contentStartIndex;
                                        while (strStart < contentStartIndex + contentSize) {
                                            list.emplace_back((char *) (buf.data() + strStart));
                                            strStart += list.back().length() + 1;
                                        }
                                        listOfStringsCallBack(callBackParam, name, list);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            state = RECV_PREAMBLE;  // transfer to receive preamble
                            offset = 0;  // this will break the for loop
                        }
                        break;
                    default:
                        assert(!"Invalid receiver state");
                }
            }
        }

    }

    if (disconnectCallback) disconnectCallback();
}

int32_t TerminalSocketBase::decodeInt32(const uint8_t *start) {
    // Little endian
    return (start[3] << 24) | (start[2] << 16) | (start[1] << 8) | start[0];
}

bool TerminalSocketServer::listen(int port, DisconnectCallback disconnected) {

    struct addrinfo hints, *servinfo, *p;
    int rv;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // use my IP

    if ((rv = ::getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "TerminalSocketServer: getaddrinfo failed: " << ::gai_strerror(rv) << "\n";
        return false;
    }

    // Loop through all the results and bind to the first we can
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((listenerFD = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            std::cerr << "TerminalSocketServer: socket failed\n";
            continue;
        }

        if (::setsockopt(listenerFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            std::cerr << "TerminalSocketServer: setsockopt failed\n";
            return false;
        }

        if (::bind(listenerFD, p->ai_addr, p->ai_addrlen) == -1) {
            ::close(listenerFD);
            std::cerr << "TerminalSocketServer: bind failed\n";
            continue;
        }

        // listenerFD ready
        break;
    }

    if (p == nullptr) {
        std::cerr << "TerminalSocketServer: failed to bind\n";
        return false;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (::listen(listenerFD, 10) == -1) {
        std::cerr << "TerminalSocketServer: listen failed\n";
        return false;
    }

    if (isConnected() || listenerThread != nullptr) {
        closeSocket();
    }

    // Start a new thread to listen to connections
    // listenerFD will be closed by the listenerThread
    listenerThreadShouldExit = false;
    listenerThread = new std::thread(&TerminalSocketServer::listenerThreadBody, this, port, disconnected);

    return true;
}

void TerminalSocketServer::disconnect() {
    if (isConnected()) {
        closeSocket();
    }
    if (listenerThread) {
        listenerThreadShouldExit = true;
        listenerThread->join();
        delete listenerThread;
        listenerThread = nullptr;
    }
}

void TerminalSocketServer::listenerThreadBody(int port, DisconnectCallback disconnected) {

    std::cout << "TerminalSocketServer: listening on port " << port << "...\n";

    struct sockaddr_storage clientAddr;  // client address information
    socklen_t addrStructSize = sizeof(clientAddr);
    int newFD;
    char s[INET6_ADDRSTRLEN];

    while (!listenerThreadShouldExit) {  // main accept() loop

        newFD = ::accept(listenerFD, (struct sockaddr *) &clientAddr, &addrStructSize);
        if (newFD == -1) {
            std::cerr << "TerminalSocketServer: accept failed\n";
            continue;
        }

        // Get client address
        ::inet_ntop(clientAddr.ss_family,
                    getClientAddr((struct sockaddr *) &clientAddr),
                    s, sizeof(s));
        std::cout << "TerminalSocketServer: get connection from " << s << "\n";

        // Setup socket
        setupSocket(newFD, disconnected);  // new will be managed by TerminalSocketBase

        // The listener thread will just accept one connection
        break;
    }

    ::close(listenerFD);
    listenerFD = 0;

    // Listener thread exit
    std::cout << "TerminalSocketServer: listener thread exit\n";
}

bool TerminalSocketClient::connect(const string &ip, int port, DisconnectCallback disconnected) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = ::getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "TerminalSocketClient: getaddrinfo failed: " << ::gai_strerror(rv) << "\n";
        return false;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            std::cerr << "TerminalSocketClient: socket failed\n";
            continue;
        }

        if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            std::cerr << "TerminalSocketClient: connect failed\n";
            continue;
        }

        // sockfd ready
        break;
    }

    if (p == nullptr) {
        std::cerr << "TerminalSocketClient: failed to connect\n";
        return false;
    }

    ::inet_ntop(p->ai_family, getClientAddr((struct sockaddr *) p->ai_addr), s, sizeof(s));
    std::cout << "TerminalSocketClient: connect to " << s << "\n";

    ::freeaddrinfo(servinfo);  // all done with this structure

    setupSocket(sockfd, disconnected);  // sockfd will be managed by TerminalSocketBase

    return true;
}

void TerminalSocketClient::disconnect() {
    if (isConnected()) {
        closeSocket();
    }
}

}