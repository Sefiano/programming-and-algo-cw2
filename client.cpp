#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

void listenAndPrint(SOCKET socketFD) {
    char buffer[1024];

    while (true) {
        int amountReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);

        if (amountReceived > 0) {
            buffer[amountReceived] = '\0';
            std::cout << buffer << std::endl; // Print received message
        }

        if (amountReceived == 0)
            break;
    }

    closesocket(socketFD);
}

void startListeningAndPrintMessagesOnNewThread(SOCKET fd) {
    std::thread th(listenAndPrint, fd);
    th.detach();
}

int main() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    SOCKET socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFD == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);
    serverAddress.sin_port = htons(2000);

    if (connect(socketFD, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
        closesocket(socketFD);
        WSACleanup();
        return 1;
    }

    startListeningAndPrintMessagesOnNewThread(socketFD);

    char name[1024];
    printf("please enter your name?\n");
    fgets(name, sizeof(name), stdin);
    name[strlen(name) - 1] = '\0';

    char buffer[1024];
    printf("type and we will send (type exit)...\n");

    while (true) {
        char line[1024];
        fgets(line, sizeof(line), stdin);
        line[strlen(line) - 1] = '\0';

        snprintf(buffer, sizeof(buffer), "%s: %s", name, line);

        if (strcmp(line, "exit") == 0)
            break;

        int len = static_cast<int>(strlen(buffer));
        if (len > 0 && len < sizeof(buffer)) {
            send(socketFD, buffer, len, 0);
        }
        else {
            std::cerr << "Invalid input." << std::endl;
        }
    }

    closesocket(socketFD);
    WSACleanup();

    return 0;
}
