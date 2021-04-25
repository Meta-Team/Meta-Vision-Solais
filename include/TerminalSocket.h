//
// Created by liuzikai on 3/11/21.
//

#ifndef META_VISION_SOLAIS_TERMINALSOCKET_H
#define META_VISION_SOLAIS_TERMINALSOCKET_H

#include "Common.h"
#include <thread>
#include <boost/asio.hpp>
#include <utility>
#include <google/protobuf/message.h>

using boost::asio::ip::tcp;

namespace meta {

/**
 * TerminalSocket provides a bidirectional TCP socket to send and receive several types of data, each labelled with a
 * NUL-terminated string.
 *
 * This class is a base class (not instantiable from outside) for TerminalSocketServer and TerminalSocketClient (below).
 * Only one concurrent TCP connection is allowed (see TerminalSocketServer::startAccept() below).
 *
 * Boost asio is used for the low-level TCP socket. The main motivation is the requirement for asynchronous socket
 * operation. Sockets are slow. If instant processing results are to be sent, we don't want the socket operations take
 * too much time. In this module, four sending functions are async. They return immediately after accepting the data
 * to be sent. A separate thread (ioThread) is launched as this class instantiates, which processes the actual I/O
 * operations (io_context.run()). Received data is processed and sent back through four callback functions.
 *
 * The class accepted a running io_context from outside.
 *
 * Async operations need careful memory management. All the resource used should be alive when the operations are
 * actually performed and there should not be any memory leak after the operations are performed or cancelled.
 * shared_ptr's are frequently used in this class to help manage the memory.
 *
 * Cleaning up sockets also requires careful handling. Boost does well on cleaning up the socket. No manual shutdown or
 * close is performed on the socket objects in this class. Instead, we just carefully manage the life cycles of socket
 * instances and let their destructor do the clean-up. When a socket instance is destroyed, queued async work will
 * be aborted, which is handled in all async callback handlers.
 *
 * The thing we need to pay careful attention is the disconnection callback. Once the thread pool is created (by
 * creating a word_guard in ioThread), it seems that there is no easy way to call io_context.run() without making the
 * current thread become a worker thread (i.e. io_context.run() doesn't return). Therefore, we can't ask the
 * disconnection to be handled immediately. Considering starting a new connection and replacing the currently alive one,
 * the original callback must not be replaced by the new one before it's called, which is not easy to realize
 * if the callback is stored as a member variable. The current solution is: let async_recv carries the
 * callback function pointer. It is triggered when the socket disconnected (in handleRecv), or is passed to the next
 * async_recv (in handleRecv) when the next async receiving starts. If a socket is alive, there should be one
 * async_recv work in the queue that carries the function pointer. When the socket is disconnected, the callback is
 * triggered and there won't be any more async_recv. Therefore the callback will be triggered exactly once.
 * handleSend() do nothing with the callback but simply ignore the disconnection.
 */
class TerminalSocketBase {
protected:

    /**
     * Types of packages that can be sent and received.
     */
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

    bool connected() const { return (socket != nullptr && socket->is_open() && !socketDisconnected); }

    /**
     * Async send a single string.
     * @param name  Name of the package.
     * @param s     The string to send. Doesn't need to be alive after this call (copied for async operation).
     * @return      Whether the operation succeeded.
     */
    bool sendSingleString(const string &name, const string &s);

    /**
     * Async send a single integer
     * @param name  Name of the package.
     * @param n     The integer to send.
     * @return      Whether the operation succeeded.
     */
    bool sendSingleInt(const string &name, int32_t n);

    /**
     * Async send bytes. Can be use to send only name (nullptr data and size 0).
     * @param name  Name of the package.
     * @param s     The start of the data. Doesn't need to be alive after this call (copied for async operation).
     * @param size  The number of bytes to send.
     * @return      Whether the operation succeeded.
     */
    bool sendBytes(const string &name, uint8_t *data = nullptr, size_t size = 0);

    /**
     * Async send bytes. Can be use to send only name (nullptr data and size 0).
     * @param name     Name of the package.
     * @param message  A protobuf message.
     * @return      Whether the operation succeeded.
     */
    bool sendBytes(const string &name, google::protobuf::Message &message);

    /**
     * Async send a list of strings
     * @param name  Name of the package.
     * @param list  The list of strings to send. Doesn't need to be alive after this call (copied for async operation).
     * @return      Whether the operation succeeded.
     */
    bool sendListOfStrings(const string &name, const vector<string> &list);

    /**
     * Callback function type for arrival of a single string. The name and the string are read-only and not persistent
     * (need to be copied if they are to be used later).
     */
    using SingleStringCallback = std::function<void(std::string_view name, std::string_view s)>;

    /**
     * Callback function type for arrival of a single integer. The name is read-only and not persistent (need to be
     * copied if it is to be used later).
     */
    using SingleIntCallback = std::function<void(std::string_view name, int32_t n)>;

    /**
     * Callback function type for arrival of bytes. The name and the data are read-only and not persistent (need to be
     * copied if they are to be used later).
     */
    using BytesCallback = std::function<void(std::string_view name, const uint8_t *buf, size_t size)>;

    /**
     * Callback function type for arrival of a list of strings. The name and the strings are read-only and not
     * persistent (need to be copied if they are to be used later).
     */
    using ListOfStringsCallback = std::function<void(std::string_view name, const vector<const char *> &list)>;

    /**
     * Set callback functions for arrivals of data.
     * @param singleString            Callback for arrival of a single string. Can be nullptr.
     * @param singleInt               Callback for arrival of a single integer. Can be nullptr.
     * @param bytes                   Callback for arrival of bytes. Can be nullptr.
     * @param listOfStrings           Callback for a list of strings. Can be nullptr.
     */
    void setCallbacks(SingleStringCallback singleString = nullptr,
                      SingleIntCallback singleInt = nullptr,
                      BytesCallback bytes = nullptr,
                      ListOfStringsCallback listOfStrings = nullptr) {
        singleStringCallBack = std::move(singleString);
        singleIntCallBack = std::move(singleInt);
        bytesCallBack = std::move(bytes);
        listOfStringsCallBack = std::move(listOfStrings);
    }

    /**
     * Get the number of bytes sent/received since last clear.
     * @return {sent bytes, received bytes}
     */
    std::pair<unsigned, unsigned> getAndClearStats();

protected:

    /**
     * Initialize a TerminalSocketBase. This constructor is set as protected to avoid instantiation from outside.
     * @param ioContext  A running io_context object.
     */
    explicit TerminalSocketBase(boost::asio::io_context &ioContext);

    static constexpr uint8_t PREAMBLE = 0xCE;

    /**
     * Set a working socket file descriptor and start receiving thread.
     * @param newSocket           A socket that is already setup on ioContext.
     * @param disconnectCallback  Callback function when disconnected.
     */
    template<class T>
    void setupSocket(std::shared_ptr<tcp::socket> newSocket, std::function<void(T *)> disconnectCallback);

    /**
     * Close the socket. disconnectCallback maybe triggered by handleRecv if the recv cycle is still running.
     */
    void closeSocket();

    boost::asio::io_context &ioContext;

    std::atomic<unsigned> uploadBytes = 0;
    std::atomic<unsigned> downloadBytes = 0;

private:

    // ================================ Socket and IO Context ================================

    std::shared_ptr<tcp::socket> socket = nullptr;
    std::atomic<bool> socketDisconnected = false;

    // ================================ Sending ================================

    void handleSend(std::shared_ptr<vector<uint8_t>> buf, const boost::system::error_code &error, size_t numBytes);

    static std::shared_ptr<vector<uint8_t>> allocateBuffer(PackageType type, const string &name, size_t contentSize);

    static void emplaceInt32(vector<uint8_t> &buf, int32_t n);

    // ================================ Receiving ================================

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

    static constexpr size_t RECV_BUFFER_SIZE = 0x10000;

    ReceiverState recvState = RECV_PREAMBLE;

    vector<uint8_t> recvBuf;  // use vector to ensure the support of large packages
    ssize_t recvOffset = 0;

    ssize_t recvRemainingBytes = 0;
    PackageType recvCurrentPackageType = PACKAGE_TYPE_COUNT;
    ssize_t recvNameStart = -1;
    ssize_t recvContentSize = -1;
    ssize_t recvContentStart = -1;

    template<class T>
    void
    handleRecv(const boost::system::error_code &error, size_t numBytes, std::function<void(T *)> disconnectCallback);

    static int32_t decodeInt32(const uint8_t *start);

    void handlePackage() const;

};

/**
 * Derived class to act as the TCP server (listener).
 */
class TerminalSocketServer : public TerminalSocketBase {
public:

    using ServerDisconnectionCallback = std::function<void(TerminalSocketServer *)>;

    /**
     * Create a server listening on the given port. The server will not accept incoming connection until startAccept()
     * is called. A server can connect to at most one client concurrently.
     * @param port                   The port to listen on.
     * @param disconnectionCallback  Callback function when the socket is disconnected.
     */
    explicit TerminalSocketServer(boost::asio::io_context &ioContext,
                                  int port, ServerDisconnectionCallback disconnectionCallback = nullptr);

    /**
     * Start accepting incoming connection. A server can connect to at most one client concurrently. If this function
     * is called when there is already a connection, and another request comes, the previous one will get disconnected.
     *
     * To restart acceptance after disconnection, call this function in the disconnect callback.
     */
    void startAccept();

    /**
     * Disconnect the socket.
     */
    void disconnect();

protected:

    int port;
    tcp::acceptor acceptor;
    ServerDisconnectionCallback disconnectionCallback;

    void handleAccept(std::shared_ptr<tcp::socket> socket, const boost::system::error_code &error);

};

/**
 * Derived class to act as the TCP client (connector).
 */
class TerminalSocketClient : public TerminalSocketBase {
public:

    using ClientDisconnectionCallback = std::function<void(TerminalSocketClient *)>;

    /**
     * Create a client.
     */
    explicit TerminalSocketClient(boost::asio::io_context &ioContext,
                                  ClientDisconnectionCallback disconnectionCallback = nullptr)
            : TerminalSocketBase(ioContext),
              resolver(ioContext),
              disconnectionCallback(std::move(disconnectionCallback)) {}

    /**
     * Try to connect to a server. This function is sync.
     * @param server              The server IP or name (will be resolved).
     * @param port                The server port.
     * @return                    Whether the connection success. This function is sync.
     */
    bool connect(const string &server, const string &port);

    /**
     * Disconnect the socket.
     */
    void disconnect();

protected:

    tcp::resolver resolver;

    ClientDisconnectionCallback disconnectionCallback;

};

}

#endif //META_VISION_SOLAIS_TERMINALSOCKET_H
