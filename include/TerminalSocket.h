//
// Created by liuzikai on 3/11/21.
//

#ifndef META_VISION_SOLAIS_TERMINALSOCKET_H
#define META_VISION_SOLAIS_TERMINALSOCKET_H

#include "Common.h"

namespace meta {

class TerminalSocketBase {
protected:

    enum PackageType {
        SINGLE_STRING,
        SINGLE_INT,
        BYTES,
        LIST_OF_STRINGS
    };

    /**
     * Package structure:
     *  uint8_t package type (enum PackageType)
     *  NUL-terminated string for name
     *  uint32_t content size
     *  bytes
     */

public:

    bool isConnected() const { return connected; }

    void sendSingleString(const string &name, const string &s);

    void sendSingleInt(const string &name, int32_t n);

    void sendBytes(const string &name, uint8_t *buf, size_t size);

    void sendListOfStrings(const string &name, const vector<string> &list);

    using SingleStringCallback = void (*)(void *, const string &name, const string &s);

    using SingleIntCallback = void (*)(void *, const string &name, int32_t n);

    using BytesCallback = void (*)(void *, const string &name, const uint8_t *buf, size_t size);

    using ListOfStringsCallback = void (*)(void *, const string &name, const vector<string> &list);

    void setCallbacks(SingleStringCallback singleString_ = nullptr, SingleIntCallback singleInt_ = nullptr,
                      BytesCallback bytes_ = nullptr, ListOfStringsCallback listOfStrings_ = nullptr) {

        singleString = singleString_;
        singleInt = singleInt_;
        bytes = bytes_;
        listOfStrings = listOfStrings_;
    }

protected:

    bool connected = false;

    SingleStringCallback singleString = nullptr;
    SingleIntCallback singleInt = nullptr;
    BytesCallback bytes = nullptr;
    ListOfStringsCallback listOfStrings = nullptr;

};

class TerminalSocketServer : public TerminalSocketBase {
public:

    void listen(int port);

};

class TerminalSocketClient : public TerminalSocketBase {
public:

    bool connect(const string &ip, int port);

};

}

#endif //META_VISION_SOLAIS_TERMINALSOCKET_H
