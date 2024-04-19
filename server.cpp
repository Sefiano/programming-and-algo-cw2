#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

struct AcceptedSocket
{
    SOCKET acceptedSocketFD;
    sockaddr_in address;
    int error;
    bool acceptedSuccessfully;
};

void broadcastMessage(const char* message, struct AcceptedSocket* acceptedSockets, int acceptedSocketsCount, SOCKET senderSocketFD, std::mutex& acceptedSocketsMutex) {
    acceptedSocketsMutex.lock();
    for (int i = 0; i < acceptedSocketsCount; i++) {
        if (acceptedSockets[i].acceptedSocketFD != senderSocketFD) {
            send(acceptedSockets[i].acceptedSocketFD, message, strlen(message), 0);
        }
    }
    acceptedSocketsMutex.unlock();
}

struct AcceptedSocket* acceptIncomingConnection(SOCKET serverSocketFD) {
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    SOCKET clientSocketFD = accept(serverSocketFD, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);

    struct AcceptedSocket* acceptedSocket = new struct AcceptedSocket;
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD != INVALID_SOCKET;

    if (!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = WSAGetLastError();

    return acceptedSocket;
}

void receiveAndPrintIncomingData(SOCKET socketFD, struct AcceptedSocket* acceptedSockets, int acceptedSocketsCount, std::mutex& acceptedSocketsMutex) {
    char buffer[1024];

    while (true) {
        int amountReceived = recv(socketFD, buffer, 1024, 0);

        if (amountReceived > 0) {
            buffer[amountReceived] = '\0';
            std::string message(buffer);
            std::string senderName = message.substr(0, message.find(':')); // Extract sender's name
            std::cout << senderName << ": " << message.substr(message.find(':') + 2) << std::endl; // Print sender's name and message
            broadcastMessage(buffer, acceptedSockets, acceptedSocketsCount, socketFD, acceptedSocketsMutex);
        }

        if (amountReceived == 0)
            break;
    }

    closesocket(socketFD);
}

void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket* pSocket, struct AcceptedSocket* acceptedSockets, int acceptedSocketsCount, std::mutex& acceptedSocketsMutex) {
    std::thread th(receiveAndPrintIncomingData, pSocket->acceptedSocketFD, acceptedSockets, acceptedSocketsCount, std::ref(acceptedSocketsMutex));
    th.detach();
}

void startAcceptingIncomingConnections(SOCKET serverSocketFD, struct AcceptedSocket* acceptedSockets, int& acceptedSocketsCount, std::mutex& acceptedSocketsMutex) {
    while (true) {
        struct AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);
        acceptedSocketsMutex.lock();
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;
        acceptedSocketsMutex.unlock();

        receiveAndPrintIncomingDataOnSeparateThread(clientSocket, acceptedSockets, acceptedSocketsCount, acceptedSocketsMutex);
    }
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

    struct AcceptedSocket acceptedSockets[10];
    int acceptedSocketsCount = 0;
    std::mutex acceptedSocketsMutex;

    startAcceptingIncomingConnections(serverSocketFD, acceptedSockets, acceptedSocketsCount, acceptedSocketsMutex);

    closesocket(serverSocketFD);
    WSACleanup();

    return 0;
}
