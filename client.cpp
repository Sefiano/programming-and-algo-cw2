#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <limits>

#pragma comment(lib, "ws2_32.lib")

// Function to encrypt plaintext using Caesar Cipher
std::string encrypt(std::string text, int shift) {
    std::string result = "";
    for (char& c : text) {
        if (isalpha(c)) {
            char shifted = (isupper(c)) ? 'A' : 'a';
            result += char((int(c - shifted + shift) % 26) + shifted);
        }
        else {
            result += c;
        }
    }
    return result;
}

// Function to decrypt ciphertext using Caesar Cipher
std::string decrypt(std::string text, int shift) {
    return encrypt(text, 26 - shift);
}

// Function to generate a random password
std::string generatePassword(int length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+";
    std::string password = "";
    for (int i = 0; i < length; ++i) {
        password += charset[rand() % charset.length()];
    }
    return password;
}

// Function to validate password requirements
bool validatePassword(const std::string& password) {
    bool hasUpperCase = false;
    bool hasSpecialChar = false;
    if (password.length() < 8)
        return false;
    for (char c : password) {
        if (isupper(c))
            hasUpperCase = true;
        if (!isalnum(c) && !isspace(c))
            hasSpecialChar = true;
    }
    return hasUpperCase && hasSpecialChar;
}

// Forward declaration of the signup function
void signup() {
    std::string username, password;
    std::cout << "Enter your username: ";
    std::cin >> username;

    std::cout << "Do you want to generate a password? (1 for yes, 0 for no): ";
    int generateChoice;
    std::cin >> generateChoice;

    if (std::cin.fail() || (generateChoice != 0 && generateChoice != 1)) { // Check for invalid input
        std::cout << "Invalid input. Please enter either 0 or 1." << std::endl;
        std::cin.clear(); // Clear the error flag
        std::cin.ignore(32767, '\n'); // Ignore rest of the line
        return;
    }

    if (generateChoice == 1) {
        password = generatePassword(12); // Generating a random password of length 12
        std::cout << "Generated password: " << password << std::endl;
    }
    else {
        std::cout << "Enter your password: ";
        std::cin >> password;
    }

    if (!validatePassword(password)) {
        std::cout << "Password does not meet criteria. Please ensure it is at least 8 characters long with at least one uppercase letter and one special character." << std::endl;
        return;
    }

    std::ofstream outfile("credentials.txt", std::ios::app);
    if (!outfile) {
        std::cout << "Error opening file for writing." << std::endl;
        return;
    }

    // Encrypt username and password before storing
    std::string encryptedUsername = encrypt(username, 3); // Caesar Cipher shift 3
    std::string encryptedPassword = encrypt(password, 3); // Caesar Cipher shift 3

    outfile << encryptedUsername << " " << encryptedPassword << std::endl;
    outfile.close();
    std::cout << "Signup successful." << std::endl;
}

// Function to handle user login
std::string login() {
    std::string username, password;
    std::cout << "Enter your username: ";
    std::cin >> username;
    std::cout << "Enter your password: ";
    std::cin >> password;

    std::ifstream infile("credentials.txt");
    if (!infile) {
        std::cout << "No users signed up yet." << std::endl;
        return "";
    }

    std::string storedUsername, storedPassword;
    bool found = false;
    while (infile >> storedUsername >> storedPassword) {
        if (decrypt(storedUsername, 3) == username && decrypt(storedPassword, 3) == password) {
            found = true;
            break;
        }
    }

    infile.close();

    if (!found) {
        std::cout << "Invalid username or password." << std::endl;
        return "";
    }

    std::cout << "Login successful!" << std::endl;
    return username;
}

void listenAndPrint(SOCKET socketFD, const std::string& username) {
    char buffer[1024];

    while (true) {
        int amountReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);

        if (amountReceived > 0) {
            buffer[amountReceived] = '\0';
            std::cout << buffer << std::endl; // Print received message with sender's name
        }

        if (amountReceived == 0)
            break;
    }

    closesocket(socketFD);
}

void startListeningAndPrintMessagesOnNewThread(SOCKET fd, const std::string& username) {
    std::thread th(listenAndPrint, fd, username);
    th.detach();
}

int main() {
    std::string username; // Declare username variable here

    srand(static_cast<unsigned int>(time(nullptr))); // Seed for random number generation

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    while (true) {
        std::cout << "Welcome to the authentication system.\n"
            << "1. Signup\n"
            << "2. Login\n"
            << "3. Exit\n"
            << "Choose an option: ";
        int option;
        std::cin >> option;

        if (std::cin.fail()) { // Check if input extraction failed
            std::cout << "Invalid input. Please enter a number." << std::endl;
            std::cin.clear(); // Clear the error flag
            std::cin.ignore(32767, '\n'); // Ignore rest of the line
            continue; // Restart the loop to prompt again
        }

        switch (option) {
        case 1:
            signup();
            break;
        case 2:
            // Check login status and retrieve username
            username = login();
            if (!username.empty()) { // Proceed if login successful
                // Proceed with chat functionality after successful login
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

                startListeningAndPrintMessagesOnNewThread(socketFD, username);

                char buffer[1024];
                printf("type and we will send (type exit)...\n");

                while (true) {
                    char line[1024];
                    fgets(line, sizeof(line), stdin);
                    line[strlen(line) - 1] = '\0';

                    snprintf(buffer, sizeof(buffer), "%s: %s", username.c_str(), line); // Correctly append username to the message

                    if (strcmp(line, "exit") == 0) {
                        closesocket(socketFD);
                        WSACleanup();
                        return 0;
                    }

                    int len = static_cast<int>(strlen(buffer));
                    if (len > 0 && len < sizeof(buffer)) {
                        send(socketFD, buffer, len, 0);
                    }
                    else {
                        std::cerr << "Invalid input." << std::endl;
                    }
                }
            }
            break;
        case 3:
            exit(0);
        default:
            std::cout << "Invalid option. Please try again." << std::endl;
        }
    }

    return 0;
}
