#pragma once

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "buffer.h"
#include "message.h"
#include "db_handler.h"

struct ConnectionInfo {
    struct addrinfo* info = nullptr;
    struct addrinfo hints;
    SOCKET listenSocket = INVALID_SOCKET;
    fd_set readfds;
    std::map<SOCKET, bool> clients;  // (SOCKET, connected)
};

// the Authentication server
class AuthServer {
public:
    explicit AuthServer(uint16 port);
    ~AuthServer();

    int RunLoop();

    // Responses
    int AckCreateAccountWebSuccess(SOCKET sock, uint64_t requestId, uint64_t userId);
    int AckCreateAccountWebFailure(SOCKET sock, uint64_t requestId, network::CreateAccountFailureReason reason);
    int AckAuthenticateWebSuccess(SOCKET sock, uint64_t requestId, uint64_t userId);
    int AckAuthenticateWebFailure(SOCKET sock, uint64_t requestId, network::AuthenticateAccountFailureReason reason);

private:
    int Initialize(uint16 port);
    int SendResponse(SOCKET sock, uint32 packetSize);
    void HandleMessage(network::MessageType msgType, SOCKET sock, uint32_t msgBytesSize);
    void HandleAuthenticateWebReq(const void* payloadHead, uint32_t payloadSize, SOCKET sock);
    void HandleCreateAccountWebReq(const void* payloadHead, uint32_t payloadSize, SOCKET sock);
    void Shutdown();

private:
    // low-level network stuff
    ConnectionInfo m_Conn;

    // send/recv buffer
    static constexpr int kRECV_BUF_SIZE = 512;
    char m_RawRecvBuf[kRECV_BUF_SIZE];
    network::Buffer m_RecvBuf{kRECV_BUF_SIZE};

    static constexpr int kSEND_BUF_SIZE = 512;
    network::Buffer m_SendBuf{kSEND_BUF_SIZE};

    // database
    DBHandler dbHandler;
};
