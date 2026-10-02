#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "des.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const string SECRET_KEY = "MyDesKey"; 

string to_hex(const string& input) {
    static const char* const lut = "0123456789ABCDEF";
    string output;
    for (size_t i = 0; i < input.length(); ++i) {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
        output.push_back(' ');
    }
    return output;
}

void receive_messages(SOCKET socket_fd) {
    char buffer[2048];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            cout << "\n[!] Koneksi terputus dari Server.\n";
            closesocket(socket_fd);
            exit(0);
        }
        
        string ciphertext(buffer, bytes_received);
        cout << "\n\n[RECEIVER] Ciphertext masuk (Hex): " << to_hex(ciphertext) << endl;
        
        string plaintext = des_decrypt(ciphertext, SECRET_KEY);
        cout << "[RECEIVER] Pesan Didekripsi: " << plaintext << "\n> " << flush;
    }
}

int main() {
    // Inisialisasi Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cout << "\n Socket creation error \n";
        WSACleanup();
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Hubungkan ke localhost
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "\nConnection Failed \n";
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    
    cout << "=== CLIENT DES BEROPERASI (WINDOWS) ===\nTerhubung ke Server!\n\n";
    
    thread recv_thread(receive_messages, sock);
    recv_thread.detach();

    string pesan_keluar;
    while (true) {
        cout << "> ";
        getline(cin, pesan_keluar);
        if (pesan_keluar.empty()) continue;

        string ciphertext = des_encrypt(pesan_keluar, SECRET_KEY);
        cout << "[SENDER] Teks tersandi (Hex) yang dikirim: " << to_hex(ciphertext) << endl;
        send(sock, ciphertext.c_str(), ciphertext.length(), 0);
    }
    
    closesocket(sock);
    WSACleanup();
    return 0;
}