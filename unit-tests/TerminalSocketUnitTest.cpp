//
// Created by liuzikai on 3/16/21.
//

#include <iostream>
#include <unistd.h>
#include "TerminalSocket.h"
#define INTERACTIVE_MODE  1
using namespace meta;

static char serverName[] = "Server";
static char clientName[] = "Client";

void processSingleString(void *param, const char *name, const char *s) {
    std::cout << static_cast<const char *>(param) << " received a string <" << name << "> \"" << s << "\"\n";
}

void processSingleInt(void *param, const char *name, int n) {
    std::cout << static_cast<const char *>(param) << " received a int <" << name << "> " << n << "\n";
}

void processBytes(void *param, const char *name, const uint8_t *buf, size_t size) {
    std::cout << static_cast<const char *>(param) << " received bytes <" << name << "> ";
    for (size_t i = 0; i < size; i++) {
        std::cout << std::hex << (int) buf[i] << std::dec << "  ";
    }
    std::cout << "\n";
}

void processListOfString(void *param, const char *name, const vector<const char *> &list) {
    std::cout << static_cast<const char *>(param) << " received list of strings <" << name << ">\n";
    for (const auto &s : list) {
        std::cout << "  \"" << s << "\"\n";
    }
}

void handleServerDisconnection(TerminalSocketServer* s);
void handleClientDisconnection(TerminalSocketClient* c);

TerminalSocketServer server(8800, handleServerDisconnection);
TerminalSocketClient client(handleClientDisconnection);

uint8_t testBytes1[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
uint8_t testBytes2[] = {0xFF};
uint8_t testBytes3[] = {};

void handleServerDisconnection(TerminalSocketServer* s) {
    static int count = 0;
    std::cerr << "Server disconnected " << ++count << std::endl;
    s->startAccept();
}

void handleClientDisconnection(TerminalSocketClient* c) {
    static int count = 0;
    std::cerr << "Client disconnected " << ++count << std::endl;
}

int main() {

#if !INTERACTIVE_MODE
    usleep(5000000);
#endif

    std::cerr << "1. Setup server...\n";

    server.startAccept();
    server.setCallbacks(serverName,
                        processSingleString,
                        processSingleInt,
                        processBytes,
                        processListOfString);

    std::cout.flush();
    std::cerr.flush();
    std::cerr << "2. Press any key to continue setting up client...\n";
#if INTERACTIVE_MODE
    std::cin.get();
#endif

    client.connect("127.0.0.1", "8800");
    client.setCallbacks(clientName,
                        processSingleString,
                        processSingleInt,
                        processBytes,
                        processListOfString);

    std::cout.flush();
    std::cerr.flush();
    std::cerr << "3. Press any key to start tests server -> client...\n";
#if INTERACTIVE_MODE
    std::cin.get();
#endif

    server.sendSingleString("FirstString", "Hello world");
    server.sendSingleString("SecondString", "Meta-Vision-Solais");

    server.sendSingleInt("FirstInt", 2333);
    server.sendSingleInt("SecondInt", 6666);

    server.sendListOfStrings("FirstStringList", {"A", "B", "AA", "BBB", "CCC", "DDDD"});
    server.sendListOfStrings("SecondStringList", {"AAAAAAAAAAAAAAA"});
    server.sendListOfStrings("ThirdStringList", {""});

    server.sendBytes("FirstBytes", testBytes1, sizeof(testBytes1));
    server.sendBytes("SecondBytes", testBytes2, sizeof(testBytes2));
    server.sendBytes("ThirdBytes", testBytes3, sizeof(testBytes3));

    std::cout.flush();
    std::cerr.flush();
    std::cerr << "4. Press any key to start tests client -> server...\n";
#if INTERACTIVE_MODE
    std::cin.get();
#endif

    client.sendSingleString("FirstString", "Hello world");
    client.sendSingleString("SecondString", "Meta-Vision-Solais");

    client.sendSingleInt("FirstInt", 2333);
    client.sendSingleInt("SecondInt", 6666);

    client.sendListOfStrings("FirstStringList", {"A", "B", "AA", "BBB", "CCC", "DDDD"});
    client.sendListOfStrings("SecondStringList", {"AAAAAAAAAAAAAAA"});
    client.sendListOfStrings("ThirdStringList", {""});

    client.sendBytes("FirstBytes", testBytes1, sizeof(testBytes1));
    client.sendBytes("SecondBytes", testBytes2, sizeof(testBytes2));
    client.sendBytes("ThirdBytes", testBytes3, sizeof(testBytes3));

    std::cout.flush();
    std::cerr.flush();
    std::cerr << "5. Press any key to disconnect client...\n";
#if INTERACTIVE_MODE
    std::cin.get();
#endif

    client.disconnect();

    std::cout.flush();
    std::cerr.flush();
    std::cerr << "6. Press any key to reconnect client...\n";
#if INTERACTIVE_MODE
    std::cin.get();
#endif

    client.connect("127.0.0.1", "8800");

    std::cout.flush();
    std::cerr.flush();
    std::cerr << "7. Press any key to start tests server -> client (2nd)...\n";
#if INTERACTIVE_MODE
    std::cin.get();
#endif

    server.sendSingleString("FirstString", "Hello world");
    server.sendSingleString("SecondString", "Meta-Vision-Solais");

    server.sendSingleInt("FirstInt", 2333);
    server.sendSingleInt("SecondInt", 6666);

    server.sendListOfStrings("FirstStringList", {"A", "B", "AA", "BBB", "CCC", "DDDD"});
    server.sendListOfStrings("SecondStringList", {"AAAAAAAAAAAAAAA"});
    server.sendListOfStrings("ThirdStringList", {""});

    server.sendBytes("FirstBytes", testBytes1, sizeof(testBytes1));
    server.sendBytes("SecondBytes", testBytes2, sizeof(testBytes2));
    server.sendBytes("ThirdBytes", testBytes3, sizeof(testBytes3));

    std::cout.flush();
    std::cerr.flush();
    std::cerr << "8. Press any key to disconnect server...\n";
#if INTERACTIVE_MODE
    std::cin.get();
#endif

    server.disconnect();


    std::cout.flush();
    std::cerr.flush();
    return 0;
}