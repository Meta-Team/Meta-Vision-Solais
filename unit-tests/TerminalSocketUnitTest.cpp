//
// Created by liuzikai on 3/16/21.
//

#include <iostream>
#include "TerminalSocket.h"
#define PORT 8800
using namespace meta;

static char serverName[] = "Server";
static char clientName[] = "Client";

void processSingleString(void *param, const string &name, const string &s) {
    std::cout << static_cast<const char *>(param) << " received a string <" << name << "> \"" << s << "\"\n";
}

void processSingleInt(void *param, const string &name, int n) {
    std::cout << static_cast<const char *>(param) << " received a int <" << name << "> " << n << "\n";
}

TerminalSocketServer server;
TerminalSocketClient client;

int main() {

    std::cerr << "Setup server...\n";

    server.listen(8800, [](){ std::cerr << "Server disconnected\n"; });
    server.setCallbacks(serverName,
                        processSingleString,
                        processSingleInt,
                        nullptr,
                        nullptr);

    std::cerr << "Press any key to continue setting up client...\n";
    std::cin.get();

    client.connect("127.0.0.1", 8800, [](){ std::cerr << "Client disconnected\n"; });
    client.setCallbacks(clientName,
                        processSingleString,
                        processSingleInt,
                        nullptr,
                        nullptr);

    std::cerr << "Press any key to start tests server -> client...\n";
    std::cin.get();

    server.sendSingleString("FirstString", "Hello world");
    server.sendSingleString("SecondString", "Meta-Vision-Solais");


    std::cerr << "Going to disconnect...\n";

    server.disconnect();
    client.disconnect();

    return 0;
}