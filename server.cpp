#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 10

SOCKET acceptedSockets[MAX_CLIENTS];
int acceptedSocketsCount = 0;

void receiveAndBroadcast(SOCKET senderSocket, const char* buffer) {
    for (int i = 0; i < acceptedSocketsCount; ++i) {
        if (acceptedSockets[i] != senderSocket) {
            send(acceptedSockets[i], buffer, strlen(buffer), 0);
        }
    }
}

void receiveAndPrintIncomingData(SOCKET socketFD) {
    char buffer[1024];

    while (true) {
        int amountReceived = recv(socketFD, buffer, sizeof(buffer), 0);

        if (amountReceived > 0) {
            buffer[amountReceived] = '\0';
            std::cout << "Received message: " << buffer << std::endl;
            receiveAndBroadcast(socketFD, buffer);
        }

        if (amountReceived == 0)
            break;
    }

    closesocket(socketFD);
}

void startListeningAndPrintMessagesOnNewThread(SOCKET fd) {
    std::thread th(receiveAndPrintIncomingData, fd);
    th.detach();
}

int main() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    SOCKET serverSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocketFD == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(2000);

    if (bind(serverSocketFD, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocketFD);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocketFD, 10) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocketFD);
        WSACleanup();
        return 1;
    }

    while (true) {
        SOCKET clientSocket = accept(serverSocketFD, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(serverSocketFD);
            WSACleanup();
            return 1;
        }

        if (acceptedSocketsCount < MAX_CLIENTS) {
            acceptedSockets[acceptedSocketsCount++] = clientSocket;
            startListeningAndPrintMessagesOnNewThread(clientSocket);
        }
        else {
            std::cerr << "Maximum clients reached. Connection rejected." << std::endl;
            closesocket(clientSocket);
        }
    }

    closesocket(serverSocketFD);
    WSACleanup();

    return 0;
}
