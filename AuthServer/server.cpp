#include "server.h"

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <iostream>
#include <sstream>

#include "auth.pb.h"

using namespace network;

AuthServer::AuthServer(uint16 port) {
    Initialize(port);
    dbHandler.Initialize("127.0.0.1:3306", "root", "root", "chat");
}

AuthServer::~AuthServer() { Shutdown(); }

// Initialization includes:
// 1. Initialize Winsock: WSAStartup
// 2. getaddrinfo
// 3. create socket
// 4. bind
// 5. listen
int AuthServer::Initialize(uint16 port) {
    // Declare and initialize variables
    WSADATA wsaData;
    int result;

    FD_ZERO(&m_Conn.readfds);

    // 1. WSAStartup
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed with error %d\n", result);
        return 1;
    } else {
        printf("WSAStartup OK!\n");
    }

    // 2. getaddrinfo
    ZeroMemory(&m_Conn.hints, sizeof(m_Conn.hints));
    m_Conn.hints.ai_family = AF_INET;        // IPV4
    m_Conn.hints.ai_socktype = SOCK_STREAM;  // Stream
    m_Conn.hints.ai_protocol = IPPROTO_TCP;  // TCP
    m_Conn.hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, std::to_string(port).c_str(), &m_Conn.hints, &m_Conn.info);
    if (result != 0) {
        fprintf(stderr, "getaddrinfo failed with error: %d\n", result);
        WSACleanup();
        return result;
    } else {
        printf("getaddrinfo ok!\n");
    }

    // 3. Create our listen socket [Socket]
    m_Conn.listenSocket = socket(m_Conn.info->ai_family, m_Conn.info->ai_socktype, m_Conn.info->ai_protocol);
    if (m_Conn.listenSocket == INVALID_SOCKET) {
        fprintf(stderr, "socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(m_Conn.info);
        WSACleanup();
        return result;
    } else {
        printf("socket OK!\n");
    }

    // 4. Bind our socket [Bind]
    result = bind(m_Conn.listenSocket, m_Conn.info->ai_addr, (int)m_Conn.info->ai_addrlen);
    if (result == SOCKET_ERROR) {
        fprintf(stderr, "bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(m_Conn.info);
        closesocket(m_Conn.listenSocket);
        WSACleanup();
        return result;
    } else {
        printf("bind OK!\n");
    }

    // 5. [Listen]
    result = listen(m_Conn.listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        fprintf(stderr, "listen failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(m_Conn.info);
        closesocket(m_Conn.listenSocket);
        WSACleanup();
        return result;
    } else {
        printf("listen OK!\n");
    }

    return result;
}

int AuthServer::RunLoop() {
    // Define timeout for select()
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500 * 1000;  // 500 milliseconds timeout

    FD_ZERO(&m_Conn.readfds);
    FD_SET(m_Conn.listenSocket, &m_Conn.readfds);

    // the loop
    while (true) {
        // Copy the fd_set, because select() will modify it
        fd_set copy = m_Conn.readfds;

        // FD_SET connected clients to copy by iterating through m_Conn.clients
        for (const std::pair<SOCKET, bool>& kv : m_Conn.clients) {
            if (kv.second) {
                FD_SET(kv.first, &copy);
            }
        }

        int socketCount = select(0, &copy, NULL, NULL, &tv);
        if (socketCount == 0) {  // Time limit expired
            continue;
        }
        if (socketCount == SOCKET_ERROR) {
            printf("select failed with error: %d\n", WSAGetLastError());
            return socketCount;
        }

        for (int i = 0; i < socketCount; i++) {
            SOCKET sock = copy.fd_array[i];
            if (sock == m_Conn.listenSocket) {  // It's an incoming new connection
                // [Accept]
                SOCKET clientSocket = accept(m_Conn.listenSocket, NULL, NULL);
                if (clientSocket == INVALID_SOCKET) {
                    fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
                } else {
                    printf("accept OK!\n");
                    FD_SET(clientSocket, &m_Conn.readfds);

                    m_Conn.clients.insert({clientSocket, true});
                }
            } else {  // It's an incoming message
                ZeroMemory(&m_RawRecvBuf, kRECV_BUF_SIZE);
                int recvResult = recv(sock, m_RawRecvBuf, kRECV_BUF_SIZE, 0);

                std::map<SOCKET, bool>::iterator it = m_Conn.clients.find(sock);
                if (recvResult <= 0) {
                    if (recvResult < 0) {
                        fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
                    } else {
                        printf("client disconnected!\n");
                    }
                    if (it != m_Conn.clients.end()) {
                        it->second = false;
                    }
                    closesocket(sock);
                    FD_CLR(sock, &m_Conn.readfds);
                } else {
                    printf("recv %d bytes from client.\n", recvResult);

                    m_RecvBuf.Set(m_RawRecvBuf, kRECV_BUF_SIZE);
                    uint32_t packetSize = m_RecvBuf.ReadUInt32LE();
                    MessageType messageType = static_cast<MessageType>(m_RecvBuf.ReadUInt32LE());

                    if (m_RecvBuf.Size() >= packetSize) {
                        uint32_t headerSize = sizeof(uint32_t) * 2;
                        uint32_t payloadSize = packetSize - headerSize;
                        HandleMessage(messageType, sock, payloadSize);
                    }

                    // #TEST
                    // Send message to other clients, and definitely NOT the listening socket
                    /*for (int i = 0; i < m_Conn.readfds.fd_count; i++) {
                        SOCKET outSock = m_Conn.readfds.fd_array[i];
                        if (outSock != m_Conn.listenSocket && outSock != sock) {
                            std::ostringstream ss;
                            ss << "SOCKET #" << sock << ": " << m_RawRecvBuf << "\r\n";
                            std::string strOut = ss.str();

                            send(outSock, strOut.c_str(), strOut.size() + 1, 0);
                        }
                    }*/
                }
            }
        }
    }

    return 0;
}

// Handle received messages
void AuthServer::HandleMessage(network::MessageType msgType, SOCKET clientSocket, uint32_t payloadSize) {
    uint32_t offset = sizeof(uint32_t) * 2;  // packet header = packetSize(uint32_t) + messageType(uint32_t)
    const void* payloadHead = static_cast<const void*>(m_RecvBuf.ConstData() + offset);

    switch (msgType) {
        // received auth::CreateAccountWeb
        case MessageType::kCREATE_ACCOUNT_WEB_REQ: {
            HandleCreateAccountWebReq(payloadHead, payloadSize, clientSocket);
        } break;

        // received auth::AuthenticateWeb
        case MessageType::kAUTHENTICATE_WEB_REQ: {
            HandleAuthenticateWebReq(payloadHead, payloadSize, clientSocket);
        } break;

        default:
            fprintf(stderr, "unknown message.\n");
            break;
    }
}

void AuthServer::HandleCreateAccountWebReq(const void* payloadHead, uint32_t payloadSize, SOCKET clientSocket) {
    auth::CreateAccountWeb createAccountWebReq;
    createAccountWebReq.ParseFromArray(payloadHead, payloadSize);

    int64_t requestId = createAccountWebReq.requestid();
    std::string email = createAccountWebReq.email();
    std::string password = createAccountWebReq.plaintextpassword();

    uint64_t userId{0};
    CreateAccountFailureReason reason = dbHandler.CreateAccount(email, password, userId);
    switch (reason) {
        case CreateAccountFailureReason::kSUCCESS: {
            AckCreateAccountSuccess(clientSocket, requestId, userId);
        } break;
        case CreateAccountFailureReason::kACCOUNT_ALREADY_EXISTS:
        case CreateAccountFailureReason::kINVALID_PASSWORD:
        case CreateAccountFailureReason::kINTERNAL_SERVER_ERROR: {
            AckCreateAccountFailure(clientSocket, requestId, reason);
        } break;
        default:
            std::cerr << "Unknown CreateAccountWeb error" << std::endl;
            break;
    }
}

void AuthServer::HandleAuthenticateWebReq(const void* payloadHead, uint32_t payloadSize, SOCKET clientSocket) {
    auth::AuthenticateWeb authWebReq;
    authWebReq.ParseFromArray(payloadHead, payloadSize);

    int64_t requestId = authWebReq.requestid();
    std::string email = authWebReq.email();
    std::string password = authWebReq.plaintextpassword();

    uint64_t userId{0};
    AuthenticateFailureReason reason = dbHandler.Authenticate("cybercser@gmail.com", "passwd1337", userId);
    switch (reason) {
        case AuthenticateFailureReason::kSUCCESS: {
            std::cout << "Authenticate success" << std::endl;
            AckAuthenticateSuccess(clientSocket, requestId, userId);
        } break;
        case AuthenticateFailureReason::kINVALID_CREDENTIALS:
        case AuthenticateFailureReason::kINTERNAL_SERVER_ERROR: {
            AckAuthenticateFailure(clientSocket, requestId, reason);
        } break;
        default:
            std::cerr << "Unknown AuthenticateWeb error" << std::endl;
            break;
    }
}

int AuthServer::AckCreateAccountSuccess(SOCKET clientSocket, uint64_t requestId, uint64_t userId) {
    auth::CreateAccountWebSuccess createAccountWebSuccess;
    createAccountWebSuccess.set_requestid(requestId);
    createAccountWebSuccess.set_userid(userId);

    // Serialize the message
    m_SendBuf.Reset();
    uint32_t headerSize = sizeof(uint32_t) * 2;
    uint32_t payloadSize = createAccountWebSuccess.ByteSizeLong();
    uint32_t packetSize = headerSize + payloadSize;
    m_SendBuf.WriteUInt32LE(packetSize);
    m_SendBuf.WriteUInt32LE(static_cast<uint32_t>(MessageType::kCREATE_ACCOUNT_WEB_SUCCESS_ACK));

    char* payloadHead = m_SendBuf.Data() + headerSize;
    createAccountWebSuccess.SerializeToArray(payloadHead, payloadSize);

    return SendResponse(clientSocket, packetSize);
}

int AuthServer::AckCreateAccountFailure(SOCKET clientSocket, uint64_t requestId, CreateAccountFailureReason reason) {
    auth::CreateAccountWebFailure createAccountWebFailure;
    createAccountWebFailure.set_requestid(requestId);
    createAccountWebFailure.set_reason(static_cast<auth::CreateAccountWebFailure_FailureReason>(reason));

    // Serialize the message
    m_SendBuf.Reset();
    uint32_t headerSize = sizeof(uint32_t) * 2;
    uint32_t payloadSize = createAccountWebFailure.ByteSizeLong();
    uint32_t packetSize = headerSize + payloadSize;
    m_SendBuf.WriteUInt32LE(packetSize);
    m_SendBuf.WriteUInt32LE(static_cast<uint32_t>(MessageType::kCREATE_ACCOUNT_WEB_FAILURE_ACK));

    char* payloadHead = m_SendBuf.Data() + headerSize;
    createAccountWebFailure.SerializeToArray(payloadHead, payloadSize);

    return SendResponse(clientSocket, packetSize);
}

int AuthServer::AckAuthenticateSuccess(SOCKET clientSocket, uint64_t requestId, uint64_t userId) {
    auth::AuthenticateWebSuccess authWebSuccess;
    authWebSuccess.set_requestid(requestId);
    authWebSuccess.set_userid(userId);

    // Serialize the message
    m_SendBuf.Reset();
    uint32_t headerSize = sizeof(uint32_t) * 2;
    uint32_t payloadSize = authWebSuccess.ByteSizeLong();
    uint32_t packetSize = headerSize + payloadSize;
    m_SendBuf.WriteUInt32LE(packetSize);
    m_SendBuf.WriteUInt32LE(static_cast<uint32_t>(MessageType::kAUTHENTICATE_WEB_SUCCESS_ACK));

    char* payloadHead = m_SendBuf.Data() + headerSize;
    authWebSuccess.SerializeToArray(payloadHead, payloadSize);

    return SendResponse(clientSocket, packetSize);
}

int AuthServer::AckAuthenticateFailure(SOCKET clientSocket, uint64_t requestId, AuthenticateFailureReason reason) {
    auth::AuthenticateWebFailure authWebFailure;
    authWebFailure.set_requestid(requestId);
    authWebFailure.set_reason(static_cast<auth::AuthenticateWebFailure_FailureReason>(reason));

    // Serialize the message
    m_SendBuf.Reset();
    uint32_t headerSize = sizeof(uint32_t) * 2;
    uint32_t payloadSize = authWebFailure.ByteSizeLong();
    uint32_t packetSize = headerSize + payloadSize;
    m_SendBuf.WriteUInt32LE(packetSize);
    m_SendBuf.WriteUInt32LE(static_cast<uint32_t>(MessageType::kAUTHENTICATE_WEB_FAILURE_ACK));

    char* payloadHead = m_SendBuf.Data() + headerSize;
    authWebFailure.SerializeToArray(payloadHead, payloadSize);

    return 0;
}

// Send response to client
int AuthServer::SendResponse(SOCKET clientSocket, uint32 packetSize) {
    // https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
    int sendResult = send(clientSocket, m_SendBuf.ConstData(), packetSize, 0);
    if (sendResult == SOCKET_ERROR) {
        fprintf(stderr, "send failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return sendResult;
    }
    return 0;
}

// Shutdown and cleanup
void AuthServer::Shutdown() {
    printf("shutting down server ...\n");
    freeaddrinfo(m_Conn.info);
    closesocket(m_Conn.listenSocket);
    FD_CLR(m_Conn.listenSocket, &m_Conn.readfds);

    while (m_Conn.readfds.fd_count > 0) {
        // Get the socket number
        SOCKET sock = m_Conn.readfds.fd_array[0];
        // Remove it from the master file list and close the socket
        FD_CLR(sock, &m_Conn.readfds);
        closesocket(sock);
    }

    WSACleanup();
}
