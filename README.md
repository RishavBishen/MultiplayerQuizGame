
# 🎮 Multiplayer Quiz Game (C++ with TCP Sockets)

Welcome to the **Multiplayer Quiz Game** — a terminal-based quiz application where multiple users can connect to a central server, participate in a quiz simultaneously, and get scored and ranked in real time.

---

## 📦 Project Structure

```
.
├── client.cpp          # C++ TCP client source code
├── server.cpp          # C++ TCP server source code
├── questions.json      # Quiz question bank in JSON format
```

---

## ⚙️ Requirements

- OS: **Linux** or **WSL (for Windows)**
- Compiler: **g++**
- Standard C++ libraries
- POSIX Sockets support (`<sys/socket.h>`, `<netinet/in.h>`, etc.)

---

## 📥 Setup Instructions

### 1. **Clone the Repository**

```bash
git clone https://github.com/your-username/MultiplayerQuizGame.git
cd MultiplayerQuizGame
```

> Make sure all files (`server.cpp`, `client.cpp`, `questions.json`) are in the same directory.

### 2. **Compile the Server and Client**

```bash
g++ server.cpp -o server -pthread
g++ client.cpp -o client
```

---

## 🚀 How to Run

### 🖥️ Step 1: Start the Server

```bash
./server
```

- The server listens for client connections.
- Once all players have connected, type `start` in the server terminal to begin the quiz.

---

### 👥 Step 2: Connect Clients

Each participant should run:

```bash
./client <server_ip_address>
```

- Example (on same machine):
  ```bash
  ./client 127.0.0.1
  ```

- The client will prompt:
  - Name
  - Roll Number
  - Then wait for the quiz to start.

---

## 🧠 `questions.json` Format

Each quiz question is stored as a JSON object like this:

```json
[
  {
    "question": "What is the capital of France?",
    "options": ["Berlin", "London", "Paris", "Madrid"],
    "answer": "c"
  }
]
```

- `"answer"` should be `"a"`, `"b"`, `"c"`, or `"d"` corresponding to the option.

---

## 🌟 Features

- ✅ Multi-client support with real-time quiz
- ✅ Questions loaded from external JSON file
- ✅ Randomized options per client
- ✅ Immediate feedback on answers
- ✅ Final score + leaderboard display

---

## 🌐 Network Configuration Tips

- For LAN play, ensure all devices are on the same network.
- For remote play, use tools like [`ngrok`](https://ngrok.com/) or port forwarding.
- Server listens on **port `12345`**. Open this port if using a firewall.

---

## 📸 Sample Gameplay Flow

```text
Client:
Enter your name: Alok
Enter your roll number: 2303302
Waiting for the quiz to start...

Server:
New client connected: Alok (2303302)
Type 'start' to begin the quiz:
> start
```

---

## 🙌 Acknowledgements

- Built using standard C++ and socket programming.
- JSON parsing handled via `nlohmann/json` (included inline in the code).

---

## 📜 License

This project is open-source and free to use for educational purposes.

---

