// ✅ Updated Quiz Server with Persistent Results, Set-A/B Support, and Client Chat Feature

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <ctime>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

#define PORT 12345
#define QUESTIONS_PER_QUIZ 5

mutex mutex_lock;
vector<tuple<int, string, string>> clientSockets;
map<string, int> scores;
string currentQuizType;
bool quizStarted = false;

struct Question {
    string question;
    string options[4];
    char correctOption;
};

string trim(const string& str) {
    string result = str;
    result.erase(0, result.find_first_not_of(" \n\r\t"));
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

vector<Question> loadQuestionsFromJson(const string &filename) {
    vector<Question> questions;
    ifstream file(filename);
    json j;
    file >> j;
    for (auto &item : j) {
        Question q;
        q.question = item["question"];
        for (int i = 0; i < 4; ++i) q.options[i] = item["options"][i];
        q.correctOption = item["answer"].get<string>()[0];
        questions.push_back(q);
    }
    return questions;
}

vector<Question> getRandomQuiz(vector<Question>& allQuestions) {
    random_device rd;
    mt19937 g(rd());
    shuffle(allQuestions.begin(), allQuestions.end(), g);
    return vector<Question>(allQuestions.begin(), allQuestions.begin() + QUESTIONS_PER_QUIZ);
}

void broadcastMessage(const string& message, int excludeSocket = -1) {
    mutex_lock.lock();
    for (auto& entry : clientSockets) {
        int sock = get<0>(entry);
        if (sock != excludeSocket) {
            send(sock, message.c_str(), message.size(), 0);
        }
    }
    mutex_lock.unlock();
}

void chatListener(int clientSocket, string clientName) {
    char buffer[2048];
    while (!quizStarted) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(clientSocket, buffer, sizeof(buffer));
        if (valread > 0) {
            string msg = trim(string(buffer, valread));
            if (!msg.empty()) {
                string fullMsg = "💬 " + clientName + ": " + msg + "\n";
                broadcastMessage(fullMsg, clientSocket);
            }
        }
    }
}

void handleClient(int clientSocket, string clientName, string clientRoll, const vector<Question>& quiz) {
    char buffer[2048];
    int score = 0;
    vector<Question> personalizedQuiz = quiz;

    for (int i = 0; i < personalizedQuiz.size(); ++i) {
        Question& q = personalizedQuiz[i];
        vector<pair<string, char>> optionPairs = {
            {q.options[0], 'a'}, {q.options[1], 'b'},
            {q.options[2], 'c'}, {q.options[3], 'd'}
        };

        string correctText = q.options[q.correctOption - 'a'];
        shuffle(optionPairs.begin(), optionPairs.end(), default_random_engine(random_device{}()));

        for (int j = 0; j < 4; ++j) {
            q.options[j] = optionPairs[j].first;
            if (optionPairs[j].first == correctText) {
                q.correctOption = 'a' + j;
            }
        }

        string questionText = "Q" + to_string(i + 1) + ": " + q.question + "\n";
        for (int j = 0; j < 4; ++j)
            questionText += string(1, 'a' + j) + ") " + q.options[j] + "\n";
        questionText += "Your answer (a/b/c/d): ";
        send(clientSocket, questionText.c_str(), questionText.length(), 0);

        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        timeout.tv_sec = 11; timeout.tv_usec = 0;

        memset(buffer, 0, sizeof(buffer));
        int activity = select(clientSocket + 1, &readfds, NULL, NULL, &timeout);

        if (activity > 0 && FD_ISSET(clientSocket, &readfds)) {
            int valread = read(clientSocket, buffer, sizeof(buffer));
            string answer = trim(string(buffer, valread));
            char selected = tolower(answer[0]);

            cout << "📩 " << clientName << " answered: " << selected << " for question " << i + 1 << "\n";

            if (selected == q.correctOption) {
                score++;
                string msg = "✅ Correct!\n\n";
                send(clientSocket, msg.c_str(), msg.length(), 0);
            } else {
                string msg = "❌ Wrong! Correct answer: ";
                msg += q.correctOption;
                msg += "\n\n";
                send(clientSocket, msg.c_str(), msg.length(), 0);
            }
        } else {
            string msg = "⏳ Time's up! You didn't answer this question.\n\n";
            send(clientSocket, msg.c_str(), msg.length(), 0);
            cout << "⚠️  " << clientName << " did not answer question " << i + 1 << "\n";
        }
    }

    string result = "🎉 Quiz Over! Your Score: " + to_string(score) + "/" + to_string(QUESTIONS_PER_QUIZ) + "\n";
    send(clientSocket, result.c_str(), result.length(), 0);

    mutex_lock.lock();
    scores[clientName] = score;

    json entry = {
        {"name", clientName},
        {"roll", clientRoll},
        {"quiz_type", currentQuizType},
        {"score", score},
        {"total", QUESTIONS_PER_QUIZ},
        {"timestamp", time(0)}
    };
    ofstream file("result.json", ios::app);
    file << entry << endl;
    mutex_lock.unlock();

    close(clientSocket);
}

void askDetails(int socket, string& name, string& roll) {
    char buffer[2048] = {0};
    string askName = "Enter your name: ";
    send(socket, askName.c_str(), askName.length(), 0);
    int n1 = read(socket, buffer, sizeof(buffer));
    name = trim(string(buffer, n1));

    memset(buffer, 0, sizeof(buffer));
    string askRoll = "Enter your roll number: ";
    send(socket, askRoll.c_str(), askRoll.length(), 0);
    int n2 = read(socket, buffer, sizeof(buffer));
    roll = trim(string(buffer, n2));
}

void acceptClients(int server_fd) {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket >= 0) {
            thread([new_socket]() {
                string name, roll;
                askDetails(new_socket, name, roll);

                mutex_lock.lock();
                clientSockets.push_back({new_socket, name, roll});
                cout << "✅ " << name << " (" << roll << ") connected.\n";
                mutex_lock.unlock();

                string waitMsg = "🎮 Waiting for quiz to start... (You can chat while waiting)\n";
                send(new_socket, waitMsg.c_str(), waitMsg.length(), 0);

                chatListener(new_socket, name);
            }).detach();
        }
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    cout << "Choose quiz type (Midsem/Endsem): ";
    getline(cin, currentQuizType);

    string filename = (currentQuizType == "Midsem") ? "setA.json" : "setB.json";
    vector<Question> allQuestions = loadQuestionsFromJson(filename);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    cout << "🚀 Server is running. Waiting for clients...\n";
    thread acceptThread(acceptClients, server_fd);

    string command;
    while (true) {
        cout << "\n⏳ Currently connected: " << clientSockets.size() << " players.\n";
        cout << "Type 'start' to begin the quiz or press Enter to refresh: ";
        getline(cin, command);
        if (command == "start" && clientSockets.size() > 0) break;
        else if (command == "start" && clientSockets.size() == 0)
            cout << "⚠ No clients connected yet!\n";
    }

    quizStarted = true;

    cout << "🔥 Starting the quiz for " << clientSockets.size() << " players...\n";
    vector<Question> quiz = getRandomQuiz(allQuestions);
    vector<thread> threads;
    for (auto& entry : clientSockets) {
        int socket = get<0>(entry);
        string name = get<1>(entry);
        string roll = get<2>(entry);
        threads.emplace_back(handleClient, socket, name, roll, quiz);
    }
    for (auto& t : threads) if (t.joinable()) t.join();
    acceptThread.detach();

    vector<pair<string, int>> ranking(scores.begin(), scores.end());
    sort(ranking.begin(), ranking.end(), [](auto &a, auto &b) {
        return a.second > b.second;
    });

    cout << "🏁 Quiz finished. 🏆 Final Rankings:\n";
    int rank = 1;
    for (auto &p : ranking) {
        cout << "#" << rank++ << ": " << p.first << " => " << p.second << "/" << QUESTIONS_PER_QUIZ << "\n";
    }

    close(server_fd);
    return 0;
}