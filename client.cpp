// ✅ Updated Client Code with Chat Support Before Quiz

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <thread>
#include <chrono>
#include <sys/select.h>
#include <fcntl.h>

#define PORT 12345
using namespace std;

bool quizStarted = false;

// Countdown + input
bool getAnswerWithCountdown(string &input, int qno, int sock) {
    fd_set set;
    struct timeval timeout;

    for (int i = 10; i > 0; --i) {
        cout << "\r⏳ Time left: " << i << "s  " << flush;

        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);

        if (rv == -1) {
            perror("select");
            return false;
        } else if (rv > 0) {
            getline(cin, input);
            return true;
        }
    }

    cout << "\n⏰ Time's up! No answer given.\n";
    string naMsg = "NA:" + to_string(qno);
    send(sock, naMsg.c_str(), naMsg.length(), 0);
    return false;
}

void receiveFromServer(int sock) {
    char buffer[2048];
    while (!quizStarted) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread > 0) {
            string msg(buffer);
            if (msg.find("Your answer") != string::npos || msg.find("Your Score:") != string::npos) {
                quizStarted = true;
            }
            cout << "\n" << msg;
            if (quizStarted) break;
        }
    }
}

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
    cout << "Enter server IP (e.g., 127.0.0.1): ";
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

    // Step 1: Send name and roll
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

    // Step 2: Chat + wait phase
    thread receiveThread(receiveFromServer, sock);

    while (!quizStarted) {
        string message;
        getline(cin, message);
        if (!message.empty()) {
            send(sock, message.c_str(), message.length(), 0);
        }
    }

    receiveThread.join();

    // Step 3: Quiz answer loop
    int qno = 1;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        valread = read(sock, buffer, 2048);
        if (valread <= 0) break;

        string msg(buffer);
        if (msg.find("Your Score:") != string::npos) {
            cout << "\n🎉 " << msg << "\n";
            break;
        }

        cout << "\n\n❓ Question " << qno << ":\n" << msg;

        string answer;
        bool gotInput = getAnswerWithCountdown(answer, qno, sock);
        if (gotInput) {
            send(sock, answer.c_str(), answer.length(), 0);
        }

        memset(buffer, 0, sizeof(buffer));
        valread = read(sock, buffer, 2048);
        if (valread <= 0) break;

        cout << "\n" << buffer;
        qno++;
    }

    cout << "\n📴 Connection closed.\n";
    close(sock);
    return 0;
}