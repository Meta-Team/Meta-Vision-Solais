//
// Created by liuzikai on 3/11/21.
//

#ifndef META_VISION_SOLAIS_TERMINALSOCKET_H
#define META_VISION_SOLAIS_TERMINALSOCKET_H

#include "Common.h"
#include <thread>

namespace meta {

class TerminalSocketBase {
protected:

    ~TerminalSocketBase();

    enum PackageType {
        SINGLE_STRING,
        SINGLE_INT,
        BYTES,
        LIST_OF_STRINGS,

        PACKAGE_TYPE_COUNT
    };

    /**
     * Package structure:
     *  1-byte PREAMBLE (defined below)
     *  uint8_t package type (enum PackageType)
     *  NUL-terminated string for name
     *  uint32_t content size
     *  bytes
     */

public:

    bool isConnected() const { return connected; }

    bool sendSingleString(const string &name, const string &s);

    bool sendSingleInt(const string &name, int32_t n);

    bool sendBytes(const string &name, uint8_t *data, size_t size);

    bool sendListOfStrings(const string &name, const vector<string> &list);

    using SingleStringCallback = void (*)(void *, const string &name, const string &s);

    using SingleIntCallback = void (*)(void *, const string &name, int32_t n);

    using BytesCallback = void (*)(void *, const string &name, const uint8_t *buf, size_t size);

    using ListOfStringsCallback = void (*)(void *, const string &name, const vector<string> &list);

    void setCallbacks(void *callBackFirstParameter, SingleStringCallback singleString = nullptr, SingleIntCallback singleInt = nullptr,
                      BytesCallback bytes = nullptr, ListOfStringsCallback listOfStrings = nullptr) {
        callBackParam = callBackFirstParameter;
        singleStringCallBack = singleString;
        singleIntCallBack = singleInt;
        bytesCallBack = bytes;
        listOfStringsCallBack = listOfStrings;
    }

    using DisconnectCallback = void (*)();

protected:

    TerminalSocketBase() = default;  // forbid creating TerminalSocketBase instances at outside world

    static constexpr uint8_t PREAMBLE = 0xCE;

    /**
     * Set a working socket file descriptor and start receiving thread
     * @param fd            A working socket file descriptor from accept() or socket(), will be closed by this instance
     *                      at disconnection.
     * @param disconnected  Callback function when disconnected
     */
    void setupSocket(int fd, DisconnectCallback disconnected);

    /**
     * Close the socket, including closing the file descriptor. Will trigger disconnect callback.
     */
    void closeSocket();

private:

    int sockfd;
    bool connected = false;

    void *callBackParam = nullptr;
    SingleStringCallback singleStringCallBack = nullptr;
    SingleIntCallback singleIntCallBack = nullptr;
    BytesCallback bytesCallBack = nullptr;
    ListOfStringsCallback listOfStringsCallBack = nullptr;

    std::thread *th = nullptr;  // thread to receive data
    std::atomic<bool> threadShouldExit = false;

    enum ReceiverState {
        RECV_PREAMBLE,
        RECV_PACKAGE_TYPE,
        RECV_NAME,
        RECV_SIZE,
        RECV_CONTENT
    };

    static constexpr size_t RECEIVER_BUFFER_SIZE = 0x10000;

    static constexpr struct timeval RECV_TIMEOUT = {1, 0};  // 1s

    void receiverThreadBody();

    // Return buffer size
    int prepareHeader(vector<uint8_t>& buf, PackageType type, const string &name, size_t contentSize);

    DisconnectCallback disconnectCallback = nullptr;

    static void emplaceInt32(vector<uint8_t> &buf, int32_t n);

    static int32_t decodeInt32(const uint8_t *start);
};

class TerminalSocketServer : public TerminalSocketBase {
public:

    bool listen(int port, DisconnectCallback disconnected);

    void disconnect();

protected:

    int listenerFD;

    std::thread *listenerThread = nullptr;
    std::atomic<bool> listenerThreadShouldExit = false;

    void listenerThreadBody(int port, DisconnectCallback disconnected);

};

class TerminalSocketClient : public TerminalSocketBase {
public:

    bool connect(const string &ip, int port, DisconnectCallback disconnected);

    void disconnect();

};

}

#endif //META_VISION_SOLAIS_TERMINALSOCKET_H
