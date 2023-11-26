#pragma once

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "buffer.h"
#include "message.h"

// ChatClient connection related info
struct ChatConnectionInfo {
    struct addrinfo* info = nullptr;
    struct addrinfo hints;
    SOCKET listenSocket = INVALID_SOCKET;
    fd_set activeSockets;
    fd_set readfds;
    std::map<SOCKET, bool> clients;  // (SOCKET, connected)
};

struct AuthConnectionInfo {
    struct addrinfo* info = nullptr;
    struct addrinfo hints;
    SOCKET authSocket = INVALID_SOCKET;
};

// the ChatRoom server
class ChatServer {
public:
    explicit ChatServer(uint16 port);
    ~ChatServer();

    int RunLoop();

    // Requests (to AuthServer)
    int ReqCreateAccountWeb(SOCKET chatClientSocket, const std::string& email, const std::string& password);
    int ReqAuthenticateAccountWeb(SOCKET chatClientSocket, const std::string& email, const std::string& password);

    // Responses
    int AckCreateAccountSuccess(SOCKET clientSocket, const std::string& email, uint64 userId);
    int AckCreateAccountFailure(SOCKET clientSocket, uint16 reason, const std::string& email);
    int AckAuthenticateAccountSuccess(SOCKET clientSocket, const std::string& email,
                                      const std::vector<std::string>& roomNames);
    int AckAuthenticateAccountFailure(SOCKET clientSocket, uint16 reason, const std::string& email);
    int AckJoinRoom(SOCKET clientSocket, network::MessageStatus status, const std::string& roomName,
                    std::vector<std::string>& userNames);
    int BroadcastJoinRoom(const std::set<std::string>& usersInRoom, const std::string& roomName,
                          const std::string& userName);
    int AckLeaveRoom(SOCKET clientSocket, network::MessageStatus status, const std::string& roomName,
                     const std::string& userName);
    int BroadcastLeaveRoom(const std::set<std::string>& usersInRoom, const std::string& roomName,
                           const std::string& userName);
    int AckChatInRoom(SOCKET clientSocket, network::MessageStatus status, const std::string& roomName,
                      const std::string& userName);
    int BroadcastChatInRoom(const std::set<std::string>& usersInRoom, const std::string& roomName,
                            const std::string& userName, const std::string& chat);

private:
    int InitChatService(uint16 port);
    int InitAuthConn(const std::string& ip, uint16 port);
    int SendMsg(SOCKET socket, uint32 packetSize);  // the name SendMessage is already taken by Windows
    void HandleMessage(network::MessageType msgType, SOCKET clientSocket);
    void Shutdown();

private:
    // low-level network stuff
    ChatConnectionInfo m_ChatConn;
    AuthConnectionInfo m_AuthConn;

    // send/recv buffer
    static constexpr int kRECV_BUF_SIZE = 512;
    char m_RawRecvBuf[kRECV_BUF_SIZE];
    network::Buffer m_RecvBuf{kRECV_BUF_SIZE};

    static constexpr int kSEND_BUF_SIZE = 512;
    network::Buffer m_SendBuf{kSEND_BUF_SIZE};

    // Server cache
    std::map<std::string, SOCKET> m_UserName2ClientSocketMap;  // userName (string) -> SOCKET
    std::map<SOCKET, std::string> m_ClientSocket2UserNameMap;  // SOCKET -> userName (string)
    std::map<std::string, std::set<std::string>> m_RoomMap;    // roomName (string) -> userNames (set of string)
    std::vector<std::string> m_RoomNames;                      // all the keys of m_RoomMap
};
