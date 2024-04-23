#include <iostream> // Standard input/output stream
#include <cstring> // String manipulation functions
#include <winsock2.h> // Windows Sockets API
#include <ws2tcpip.h> // TCP/IP protocol
#include <thread> // Multi-threading support

#pragma comment(lib, "ws2_32.lib") // Linking with Winsock library

#define MAX_CLIENTS 10 // Maximum number of clients the server can handle

SOCKET acceptedSockets[MAX_CLIENTS]; // Array to store accepted sockets
int acceptedSocketsCount = 0; // Counter to keep track of accepted sockets

// Function to receive data from a sender socket and broadcast it to all other accepted sockets
void receiveAndBroadcast(SOCKET senderSocket, const char* buffer) {
    for (int i = 0; i < acceptedSocketsCount; ++i) {
        if (acceptedSockets[i] != senderSocket) {
            send(acceptedSockets[i], buffer, strlen(buffer), 0);
        }
    }
}

// Function to receive and print incoming data on a socket
void receiveAndPrintIncomingData(SOCKET socketFD) {
    char buffer[1024];

    while (true) {
        int amountReceived = recv(socketFD, buffer, sizeof(buffer), 0);

        if (amountReceived > 0) {
            buffer[amountReceived] = '\0';
            std::cout << "Received message: " << buffer << std::endl;
            receiveAndBroadcast(socketFD, buffer); // Broadcast the received message
        }

        if (amountReceived == 0)
            break;
    }

    closesocket(socketFD); // Close the socket after receiving data
}

// Function to start listening for incoming data on a new thread
void startListeningAndPrintMessagesOnNewThread(SOCKET fd) {
    std::thread th(receiveAndPrintIncomingData, fd); // Create a new thread for listening
    th.detach(); // Detach the thread to allow it to run independently
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
            acceptedSockets[acceptedSocketsCount++] = clientSocket; // Add the accepted socket to the array
            startListeningAndPrintMessagesOnNewThread(clientSocket); // Start listening on a new thread
        }
        else {
            std::cerr << "Maximum clients reached. Connection rejected." << std::endl;
            closesocket(clientSocket); // Close the socket if maximum clients reached
        }
    }

    closesocket(serverSocketFD); // Close the server socket
    WSACleanup(); // Clean up Winsock resources

    return 0;
}
