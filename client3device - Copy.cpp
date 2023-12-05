#include <iostream>
#include <cstring>
#include "SDL2/SDL_net.h"
#include <vector>
#include <sstream>
#include <chrono>

using namespace std;

#define BUFFER_SIZE 1024

struct Question {
    int level;
    string content;
    vector<string> answerList;
};

Question decodeQuestion(const string& message) {
    Question question;
    istringstream iss(message);
    string line;

    // Read the question level
    getline(iss, line);
    question.level = stoi(line.substr(line.find(":") + 1));

    // Read the question content
    getline(iss, question.content);

    // Read the answer options
    while (getline(iss, line)) {
        question.answerList.push_back(line);
    }

    return question;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << endl;
        return -1;
    }

    if (SDLNet_Init() < 0) {
        cerr << "SDLNet_Init failed: " << SDLNet_GetError() << endl;
        return -1;
    }

    IPaddress ip;
    if (SDLNet_ResolveHost(&ip, argv[1], stoi(argv[2])) < 0) {
        cerr << "SDLNet_ResolveHost failed: " << SDLNet_GetError() << endl;
        SDLNet_Quit();
        return -1;
    }

    TCPsocket clientSocket = SDLNet_TCP_Open(&ip);
    if (!clientSocket) {
        cerr << "SDLNet_TCP_Open failed: " << SDLNet_GetError() << endl;
        SDLNet_Quit();
        return -1;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Receive the welcome message from the server
    if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
        cerr << "Error receiving welcome message: " << SDLNet_GetError() << endl;
        SDLNet_TCP_Close(clientSocket);
        SDLNet_Quit();
        return -1;
    }

    cout << buffer << endl;

    // Send client's name to the server
    string name;
    cout << "Enter your name: ";
    getline(cin, name);
    if (SDLNet_TCP_Send(clientSocket, name.c_str(), name.length()) <= 0) {
        cerr << "Error sending name: " << SDLNet_GetError() << endl;
        SDLNet_TCP_Close(clientSocket);
        SDLNet_Quit();
        return -1;
    }

    while (true) {
        int choice;
        cout << "Who wants to be a millionaire" << endl;
        cout << "1. Start Game" << endl;
        cout << "2. Challenge another player" << endl;
        cout << "Other to quit" << endl;
        cin >> choice;
        cin.ignore(); // Add this line to discard the newline character

        switch (choice) {
            case 1: {
                // Send "START_GAME" to initiate the quiz
                if (SDLNet_TCP_Send(clientSocket, "START_GAME", strlen("START_GAME")) <= 0) {
                    cerr << "Error sending START_GAME message: " << SDLNet_GetError() << endl;
                    SDLNet_TCP_Close(clientSocket);
                    SDLNet_Quit();
                    return -1;
                }

                while (true) {
                    auto start_time = chrono::high_resolution_clock::now();

                    memset(buffer, 0, BUFFER_SIZE);
                    if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
                        cerr << "Error receiving server message: " << SDLNet_GetError() << endl;
                        SDLNet_TCP_Close(clientSocket);
                        SDLNet_Quit();
                        return -1;
                    } else if (strcmp(buffer, "GAME_OVER") == 0) {
                        auto end_time = chrono::high_resolution_clock::now();
                        auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

                        cout << "You answered incorrectly. " << buffer << endl;
                        memset(buffer, 0, BUFFER_SIZE);
                        if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
                            cerr << "Error receiving score message: " << SDLNet_GetError() << endl;
                            SDLNet_TCP_Close(clientSocket);
                            SDLNet_Quit();
                            return -1;
                        }
                        cout << "RTT: " << duration.count() << " milliseconds" << endl;
                        cout << buffer << endl;
                        break;
                    } else {
                        // Decode the server's question message
                        Question question = decodeQuestion(buffer);

                        // Print the question
                        cout << "Level: " << question.level << endl;
                        cout << "Question: " << question.content << endl;
                        cout << "Options:" << endl;
                        for (const string& option : question.answerList) {
                            cout << option << endl;
                        }

                        string clientAnswer;
                        getline(cin, clientAnswer);
                        if (SDLNet_TCP_Send(clientSocket, clientAnswer.c_str(), clientAnswer.length()) <= 0) {
                            cerr << "Error sending answer: " << SDLNet_GetError() << endl;
                            SDLNet_TCP_Close(clientSocket);
                            SDLNet_Quit();
                            return -1;
                        }
                    }
                }

                break;
            }
            case 2:
                break;

            default:
                SDLNet_TCP_Close(clientSocket);
                SDLNet_Quit();
                return 0;
        }
    }

    SDLNet_TCP_Close(clientSocket);
    SDLNet_Quit();

    return 0;
}
