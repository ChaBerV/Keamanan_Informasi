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
        if(b_recv <= 0) { cout << "\n[!] Bob Terputus.\n"; exit(0); }
        string cipher(buffer, b_recv);
        cout << "\n[Bob (Hex)]: " << cipher << "\n[Bob (Teks)]: " << AES_Decrypt(cipher, ks) << "\n[Alice]> " << flush;
    }
}

int main() {
    WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);
    string ka, kdc_ip, bob_ip;
    
    cout << "=========================================\n";
    cout << "               NODE ALICE\n";
    cout << "=========================================\n";
    cout << "[SETUP] Masukkan Master Key Alice (Ka): "; getline(cin, ka);
    cout << "[SETUP] Masukkan IP KDC (misal 127.0.0.1): "; getline(cin, kdc_ip);
    cout << "[SETUP] Masukkan IP Bob (misal 127.0.0.1): "; getline(cin, bob_ip);

    // KONEKSI KE KDC
    SOCKET kdc_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in kdc_addr;
    kdc_addr.sin_family = AF_INET; kdc_addr.sin_port = htons(8080);
    kdc_addr.sin_addr.s_addr = inet_addr(kdc_ip.c_str());
    
    cout << "\n(Tekan ENTER untuk memulai koneksi dan mengirim Request ke KDC)";
    cin.get();
    
    connect(kdc_sock, (struct sockaddr*)&kdc_addr, sizeof(kdc_addr));
    string req = "Alice,Bob,NonceAlice1";
    cout << "<<< [LANGKAH 1] Request dikirim ke KDC: " << req << endl;
    send(kdc_sock, req.c_str(), req.length(), 0);

    // Langkah 2: Terima Balasan KDC
    char buffer[1024] = {0};
    int b_recv = recv(kdc_sock, buffer, sizeof(buffer), 0);
    if(b_recv <= 0) return 0;
    string msg_hex(buffer, b_recv);
    closesocket(kdc_sock);

    cout << "\n>>> [LANGKAH 2] Balasan dari KDC diterima (Hex): " << msg_hex << endl;
    cout << "(Tekan ENTER untuk mendekripsi balasan KDC menggunakan Ka)";
    cin.get();

    string msg_plain = AES_Decrypt(msg_hex, ka);
    vector<string> parts = split(msg_plain, ",");
    ks = parts[0]; string ticket_hex = parts[4];
    cout << "    [SUCCESS] Terdekripsi!\n    -> Session Key Ks: " << ks << "\n    -> Ticket Bob: " << ticket_hex << endl;

    // KONEKSI KE BOB
    SOCKET bob_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in bob_addr;
    bob_addr.sin_family = AF_INET; bob_addr.sin_port = htons(8081);
    bob_addr.sin_addr.s_addr = inet_addr(bob_ip.c_str());
    connect(bob_sock, (struct sockaddr*)&bob_addr, sizeof(bob_addr));

    cout << "\n(Tekan ENTER untuk meneruskan Ticket ke Bob)";
    cin.get();
    cout << "<<< [LANGKAH 3] Ticket dikirim ke Bob...\n";
    send(bob_sock, ticket_hex.c_str(), ticket_hex.length(), 0);

    // Langkah 4: Terima Tantangan N2
    memset(buffer, 0, sizeof(buffer));
    b_recv = recv(bob_sock, buffer, sizeof(buffer), 0);
    if(b_recv <= 0) return 0;
    string step4_hex(buffer, b_recv);
    
    cout << "\n>>> [LANGKAH 4] Tantangan N2 diterima dari Bob (Hex): " << step4_hex << endl;
    cout << "(Tekan ENTER untuk mendekripsi N2 dan membuat balasan f(N2))";
    cin.get();
    
    string n2 = AES_Decrypt(step4_hex, ks);
    cout << "    -> Isi Tantangan Bob: " << n2 << endl;

    string resp_n2 = n2 + "-OK"; // f(N2) tersederhanakan
    string step5_hex = AES_Encrypt(resp_n2, ks);
    cout << "<<< [LANGKAH 5] Mengirim f(N2) ke Bob (Hex): " << step5_hex << endl;
    send(bob_sock, step5_hex.c_str(), step5_hex.length(), 0);

    cout << "\n=========================================\n";
    cout << "  AUTENTIKASI SELESAI! MEMULAI CHAT AMAN \n";
    cout << "=========================================\n";
    
    thread listener(chat_listener, bob_sock); listener.detach();
    string msg;
    while(true) {
        cout << "[Alice]> "; getline(cin, msg);
        if(msg.empty()) continue;
        string c = AES_Encrypt(msg, ks);
        send(bob_sock, c.c_str(), c.length(), 0);
    }
}