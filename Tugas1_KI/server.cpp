#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h> // Library Socket untuk Windows
#include <ws2tcpip.h>
#include "des.h"      // Import algoritma DES manual

// Pragma ini memberitahu compiler (khusus MSVC) untuk menghubungkan library ws2_32
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

// Perhatikan penggunaan tipe SOCKET (bukan int seperti di Linux)
void receive_messages(SOCKET socket_fd) {
    char buffer[2048];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            cout << "\n[!] Koneksi terputus dari Client.\n";
            closesocket(socket_fd); // Di Windows menggunakan closesocket(), bukan close()
            exit(0);
        }
        
        string ciphertext(buffer, bytes_received);
        cout << "\n\n[RECEIVER] Ciphertext masuk (Hex): " << to_hex(ciphertext) << endl;
        
        string plaintext = des_decrypt(ciphertext, SECRET_KEY);
        cout << "[RECEIVER] Pesan Didekripsi: " << plaintext << "\n> " << flush;
    }
}

int main() {
    // Inisialisasi Winsock (Wajib untuk Windows)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cout << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        cout << "Bind failed.\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        cout << "Listen failed.\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    cout << "=== SERVER DES BEROPERASI (WINDOWS) ===\nMenunggu koneksi dari client...\n";
    
    client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_socket == INVALID_SOCKET) {
        cout << "Accept failed.\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    cout << "[!] Client terhubung. Anda bisa mulai mengetik pesan.\n\n";

    thread recv_thread(receive_messages, client_socket);
    recv_thread.detach();

    string pesan_keluar;
    while (true) {
        cout << "> ";
        getline(cin, pesan_keluar);
        if (pesan_keluar.empty()) continue;

        string ciphertext = des_encrypt(pesan_keluar, SECRET_KEY);
        cout << "[SENDER] Teks tersandi (Hex) yang dikirim: " << to_hex(ciphertext) << endl;
        send(client_socket, ciphertext.c_str(), ciphertext.length(), 0);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}