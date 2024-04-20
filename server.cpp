#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <openssl/aes.h>
#include <openssl/rand.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")


#define MAX_CLIENTS 10

// Define the pre-shared key (for simplicity)
#define AES_KEY_SIZE 128
unsigned char aes_key[AES_KEY_SIZE / 8] = "16bytesecretkey";

// Function to encrypt plaintext using AES
std::string encryptAES(const std::string& plaintext) {
    AES_KEY enc_key;
    AES_set_encrypt_key(aes_key, AES_KEY_SIZE, &enc_key);

    std::string ciphertext;

    // Ensure plaintext length is multiple of AES block size
    int paddedLen = (plaintext.length() + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE * AES_BLOCK_SIZE;
    char* padded_plaintext = new char[paddedLen];
    memset(padded_plaintext, 0, paddedLen);
    memcpy(padded_plaintext, plaintext.c_str(), plaintext.length());

    for (int i = 0; i < paddedLen; i += AES_BLOCK_SIZE) {
        unsigned char encrypted_block[AES_BLOCK_SIZE];
        AES_encrypt(reinterpret_cast<const unsigned char*>(&padded_plaintext[i]), encrypted_block, &enc_key);
        ciphertext.append(reinterpret_cast<char*>(encrypted_block), AES_BLOCK_SIZE);
    }

    delete[] padded_plaintext;

    return ciphertext;
}

// Function to decrypt ciphertext using AES
std::string decryptAES(const std::string& ciphertext) {
    AES_KEY dec_key;
    AES_set_decrypt_key(aes_key, AES_KEY_SIZE, &dec_key);

    std::string decryptedtext;

    for (int i = 0; i < ciphertext.length(); i += AES_BLOCK_SIZE) {
        unsigned char decrypted_block[AES_BLOCK_SIZE];
        AES_decrypt(reinterpret_cast<const unsigned char*>(&ciphertext[i]), decrypted_block, &dec_key);
        decryptedtext.append(reinterpret_cast<char*>(decrypted_block), AES_BLOCK_SIZE);
    }

    // Remove padding
    size_t pad_pos = decryptedtext.find_last_not_of('\0');
    if (pad_pos != std::string::npos) {
        decryptedtext = decryptedtext.substr(0, pad_pos + 1);
    }

    return decryptedtext;
}

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
