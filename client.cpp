#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#define PORT 12345

using namespace std;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[2048] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "❌ Socket creation failed!\n";
        return 1;
    }

    string host;
    cout << "Enter server host (e.g., serveo.net): ";
    getline(cin, host);

    struct hostent* server = gethostbyname(host.c_str());
    if (!server) {
        cerr << "❌ Host not found\n";
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "❌ Connection Failed\n";
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));
    int valread = read(sock, buffer, 2048);
    if (valread > 0) {
        cout << buffer;
        string name;
        getline(cin, name);
        send(sock, name.c_str(), name.length(), 0);
    }

    memset(buffer, 0, sizeof(buffer));
    valread = read(sock, buffer, 2048);
    if (valread > 0) {
        cout << buffer;
        string roll;
        getline(cin, roll);
        send(sock, roll.c_str(), roll.length(), 0);
    }

    memset(buffer, 0, sizeof(buffer));
    valread = read(sock, buffer, 2048);
    if (valread > 0) cout << buffer;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, 2048);
        if (valread <= 0) break;

        cout << buffer;
        string msg(buffer);
        if (msg.find("Your Score:") != string::npos) break;

        string answer;
        getline(cin, answer);
        if (answer.empty()) answer = "a";
        send(sock, answer.c_str(), answer.length(), 0);

        memset(buffer, 0, sizeof(buffer));
        valread = read(sock, buffer, 2048);
        if (valread <= 0) break;
        cout << buffer;
    }

    cout << "📴 Connection closed.\n";
    close(sock);
    return 0;
}
