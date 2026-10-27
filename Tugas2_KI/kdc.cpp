#include <iostream>
#include <string>
#include <winsock2.h>
#include <conio.h> // Untuk getch() agar bisa pause
#include "aes_manual.h"
#pragma comment(lib, "ws2_32.lib")

using namespace std;

vector<string> split(string str, string token){
    vector<string>result;
    while(str.size()){
        int index = str.find(token);
        if(index!=string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str); str = "";
        }
    }
    return result;
}

int main() {
    WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);
    string ka, kb, ks;
    
    cout << "=========================================\n";
    cout << "   KDC (KEY DISTRIBUTION CENTER) NODE\n";
    cout << "=========================================\n";
    cout << "[SETUP] Masukkan Master Key Alice (Ka): "; getline(cin, ka);
    cout << "[SETUP] Masukkan Master Key Bob (Kb): "; getline(cin, kb);
    cout << "[SETUP] Generate Session Key Baru (Ks): "; getline(cin, ks);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);
    
    cout << "\n[!] KDC SIAP beroperasi di Port 8080.\n";
    cout << "[!] Menunggu Request dari Alice...\n";
    SOCKET alice_sock = accept(server_fd, NULL, NULL);

    // Langkah 1: KDC Menerima Request
    char buffer[1024] = {0};
    int b_recv = recv(alice_sock, buffer, sizeof(buffer), 0);
    if(b_recv <= 0) return 0;
    string req(buffer, b_recv);
    
    cout << "\n>>> [LANGKAH 1] Request Masuk dari Alice: " << req << endl;
    vector<string> req_parts = split(req, ",");
    string IDA = req_parts[0]; string IDB = req_parts[1]; string N1 = req_parts[2];

    cout << "\n(Tekan ENTER untuk memproses dan mengirim balasan ke Alice)";
    cin.get();

    // Langkah 2: KDC Mengirim Balasan
    string ticket_plain = ks + "," + IDA;
    string ticket_hex = AES_Encrypt(ticket_plain, kb);
    string msg_plain = ks + "," + IDA + "," + IDB + "," + N1 + "," + ticket_hex;
    string msg_hex = AES_Encrypt(msg_plain, ka);

    cout << "<<< [LANGKAH 2] Mengirim Enkripsi E_Ka() ke Alice...\n";
    cout << "    Isi (Hex): " << msg_hex << endl;
    send(alice_sock, msg_hex.c_str(), msg_hex.length(), 0);

    closesocket(alice_sock); closesocket(server_fd); WSACleanup();
    cout << "\n[!] Tugas KDC Selesai. KDC akan ditutup.\n";
    return 0;
}