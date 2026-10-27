#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include "aes_manual.h"
#pragma comment(lib, "ws2_32.lib")
using namespace std;

string ks = "";

vector<string> split(string str, string token){
    vector<string>result;
    while(str.size()){
        int index = str.find(token);
        if(index!=string::npos){ result.push_back(str.substr(0,index)); str = str.substr(index+token.size());}
        else{ result.push_back(str); str = ""; }
    }
    return result;
}

void chat_listener(SOCKET sock) {
    char buffer[2048];
    while(true) {
        memset(buffer, 0, sizeof(buffer));
        int b_recv = recv(sock, buffer, sizeof(buffer), 0);
        if(b_recv <= 0) { cout << "\n[!] Alice Terputus.\n"; exit(0); }
        string cipher(buffer, b_recv);
        cout << "\n[Alice (Hex)]: " << cipher << "\n[Alice (Teks)]: " << AES_Decrypt(cipher, ks) << "\n[Bob]> " << flush;
    }
}

int main() {
    WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);
    string kb;
    cout << "=========================================\n";
    cout << "               NODE BOB\n";
    cout << "=========================================\n";
    cout << "[SETUP] Masukkan Master Key Bob (Kb): "; getline(cin, kb);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8081);
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);
    
    cout << "\n[!] Bob SIAP di Port 8081. Menunggu koneksi Alice...\n";
    SOCKET alice_sock = accept(server_fd, NULL, NULL);
    cout << "[!] Alice terhubung.\n";

    // Langkah 3: Terima Ticket
    char buffer[1024] = {0};
    int b_recv = recv(alice_sock, buffer, sizeof(buffer), 0);
    if(b_recv <= 0) return 0;
    string ticket_hex(buffer, b_recv);
    
    cout << "\n>>> [LANGKAH 3] Ticket dari Alice diterima (Hex): " << ticket_hex << endl;
    cout << "(Tekan ENTER untuk mendekripsi Ticket ini menggunakan Kb)";
    cin.get();
    
    string ticket_plain = AES_Decrypt(ticket_hex, kb);
    vector<string> t_parts = split(ticket_plain, ",");
    ks = t_parts[0]; string IDA = t_parts[1];
    cout << "    [SUCCESS] Ticket Dekripsi:\n    -> Session Key (Ks): " << ks << "\n    -> Identitas Pemohon: " << IDA << endl;

    // Langkah 4: Kirim Tantangan N2 ke Alice
    cout << "\n(Tekan ENTER untuk membuat Challenge (Nonce 2) dan mengirim ke Alice)";
    cin.get();
    string n2;
    cout << "Ketik teks untuk Nonce 2 (misal: TantanganBob1): "; getline(cin, n2);
    
    string step4_hex = AES_Encrypt(n2, ks);
    cout << "<<< [LANGKAH 4] Mengirim Enkripsi N2 ke Alice (Hex): " << step4_hex << endl;
    send(alice_sock, step4_hex.c_str(), step4_hex.length(), 0);

    // Langkah 5: Terima balasan f(N2) dari Alice
    memset(buffer, 0, sizeof(buffer));
    b_recv = recv(alice_sock, buffer, sizeof(buffer), 0);
    if(b_recv <= 0) return 0;
    string step5_hex(buffer, b_recv);
    
    cout << "\n>>> [LANGKAH 5] Balasan f(N2) Alice diterima (Hex): " << step5_hex << endl;
    cout << "(Tekan ENTER untuk memverifikasi balasan Alice)";
    cin.get();
    
    string n2_resp = AES_Decrypt(step5_hex, ks);
    cout << "    [CEK] Nilai terdekripsi: " << n2_resp << endl;
    
    if(n2_resp == n2 + "-OK") {
        cout << "\n=========================================\n";
        cout << "  AUTENTIKASI BERHASIL! SESSION KEY AKTIF\n";
        cout << "=========================================\n";
    } else { 
        cout << "\n[!] Autentikasi Gagal! Kunci salah.\n"; return 0; 
    }

    // Masuk ke Mode Chat
    thread listener(chat_listener, alice_sock); listener.detach();
    string msg;
    while(true) {
        cout << "[Bob]> "; getline(cin, msg);
        if(msg.empty()) continue;
        string c = AES_Encrypt(msg, ks);
        send(alice_sock, c.c_str(), c.length(), 0);
    }
}