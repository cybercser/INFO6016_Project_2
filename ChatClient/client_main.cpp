#include <conio.h>

#include <iostream>
#include <random>
#include <thread>

#include "client.h"

// Need to link Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT 5555

// this snippet is generated by Codeium
std::string MumboJumbo() {
    // Define the subjects and adjectives
    std::vector<std::string> subjects = {"The cat", "The dog", "The bird"};
    std::vector<std::string> adjectives = {"happy", "playful", "cute"};

    // Randomly select a subject and an adjective
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> subjectDist(0, subjects.size() - 1);
    std::uniform_int_distribution<> adjectiveDist(0, adjectives.size() - 1);

    std::string subject = subjects[subjectDist(gen)];
    std::string adjective = adjectives[adjectiveDist(gen)];

    // Form the sentence
    std::string sentence = subject + " is " + adjective;

    return sentence;
}

void RecvLoop(ChatClient* client) {
    while (true) {
        client->RecvResponse();
    }
}

int main(int argc, char** argv) {
    std::string userName{"Fan"};
    std::string password{"Fanshawe"};

    if (argc > 2) {
        userName = argv[1];
        password = argv[2];
    }

    ChatClient client{"127.0.0.1", DEFAULT_PORT};

    std::thread t{RecvLoop, &client};

    std::cout << "Pressed Enter key to execute each step" << std::endl;

    // Send messages to the server when a key is pressed
    int step = 0;
    int bQuit = false;
    while (!bQuit) {
        if (_kbhit()) {
            char key = _getch();
            if (key == '\r') {
                // Pressed Enter key to execute each step
                switch (step) {
                    case 0:
                        std::cout << "ReqLogin" << std::endl;
                        client.ReqLogin(userName, password);
                        break;
                    case 1:
                        std::cout << "ReqJoinRoom #graphics" << std::endl;
                        client.ReqJoinRoom("graphics");
                        break;
                    case 2:
                        std::cout << "ReqJoinRoom #network" << std::endl;
                        client.ReqJoinRoom("network");
                        break;
                    case 4:
                        std::cout << "ReqLeaveRoom #graphics" << std::endl;
                        client.ReqLeaveRoom("graphics");
                        break;
                    case 3:
                        std::cout << "ReqChatInRoom" << std::endl;
                        client.ReqChatInRoom("network", MumboJumbo());
                        break;
                    default:
                        bQuit = true;
                        break;
                }
                step++;
            }
        }
    }

    t.join();

    return 0;
}