//
// Created by liuzikai on 3/11/21.
//

#ifndef META_VISION_SOLAIS_TERMINALSOCKET_H
#define META_VISION_SOLAIS_TERMINALSOCKET_H

#include "Common.h"
#include <thread>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

using boost::asio::ip::tcp;

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

    using SingleStringCallback = void (*)(void *, const char *name, const char *s);

    using SingleIntCallback = void (*)(void *, const char *name, int32_t n);

    using BytesCallback = void (*)(void *, const char *name, const uint8_t *buf, size_t size);

    using ListOfStringsCallback = void (*)(void *, const char *name, const vector<const char *> &list);

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

    // Forbid creating TerminalSocketBase instances at outside world
    TerminalSocketBase();

    static constexpr uint8_t PREAMBLE = 0xCE;

    /**
     * Set a working socket file descriptor and start receiving thread.
     * @param newSocket     A socket that is already setup on ioContext.
     * @param disconnected  Callback function when disconnected.
     */
    void setupSocket(std::shared_ptr<tcp::socket> newSocket, DisconnectCallback disconnected);

    /**
     * Close the socket. disconnectCallback maybe triggered by handleRecv if the recv cycle is still running.
     */
    void closeSocket();

    boost::asio::io_context ioContext;  // open to derive classes to create sockets

private:

    // ================================ Socket and IO Context ================================

    std::shared_ptr<tcp::socket> socket = nullptr;

    bool connected = false;  // socket object itself is complicated so this bool is used for one-time switch

    std::thread ioThread;  // thread for ioService
    void ioThreadBody();

    // ================================ Sending ================================

    void handleSend(vector<uint8_t>* buf, const boost::system::error_code &error, size_t numBytes);

    static vector<uint8_t> *allocateBuffer(PackageType type, const string &name, size_t contentSize);

    static void emplaceInt32(vector<uint8_t> &buf, int32_t n);

    // ================================ Receiving ================================

    void *callBackParam = nullptr;
    SingleStringCallback singleStringCallBack = nullptr;
    SingleIntCallback singleIntCallBack = nullptr;
    BytesCallback bytesCallBack = nullptr;
    ListOfStringsCallback listOfStringsCallBack = nullptr;

    enum ReceiverState {
        RECV_PREAMBLE,
        RECV_PACKAGE_TYPE,
        RECV_NAME,
        RECV_SIZE,
        RECV_CONTENT
    };

    static constexpr size_t RECEIVER_BUFFER_SIZE = 0x10000;

    ReceiverState recvState = RECV_PREAMBLE;

    vector<uint8_t> recvBuf;  // use vector to ensure the support of large packages
    ssize_t recvOffset = 0;

    ssize_t recvRemainingBytes = 0;
    PackageType recvCurrentPackageType = PACKAGE_TYPE_COUNT;
    ssize_t recvNameStart = -1;
    ssize_t contentSize = -1;
    ssize_t recvContentStart = -1;

    void handleRecv(boost::system::error_code const& error, size_t numBytes);

    DisconnectCallback disconnectCallback = nullptr;

    static int32_t decodeInt32(const uint8_t *start);

    void handlePackage() const;
};

class TerminalSocketServer : public TerminalSocketBase {
public:

    TerminalSocketServer(int port, DisconnectCallback disconnectCallback = nullptr);

    void startAccept();

    void disconnect();

protected:

    int port;
    tcp::acceptor acceptor;
    DisconnectCallback disconnectCallback;

    void handleAccept(std::shared_ptr<tcp::socket> socket, const boost::system::error_code& error);

};

class TerminalSocketClient : public TerminalSocketBase {
public:

    TerminalSocketClient() : resolver(ioContext) {}

    bool connect(const string &server, const string &port, DisconnectCallback disconnectCallback);

    void disconnect();

protected:

    tcp::resolver resolver;
    DisconnectCallback disconnectCallback;

};

}

#endif //META_VISION_SOLAIS_TERMINALSOCKET_H
