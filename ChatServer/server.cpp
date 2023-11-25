#include "server.h"

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <stdio.h>

#include "auth.pb.h"

using namespace network;

ChatServer::ChatServer(uint16 port) {
    // init chatroom logic stuff
    m_RoomNames.push_back("graphics");
    m_RoomNames.push_back("network");
    m_RoomNames.push_back("media");
    m_RoomNames.push_back("configuration");

    std::set<std::string> emptyUserSet{};
    m_RoomMap.insert(std::make_pair("graphics", emptyUserSet));
    m_RoomMap.insert(std::make_pair("network", emptyUserSet));
    m_RoomMap.insert(std::make_pair("media", emptyUserSet));
    m_RoomMap.insert(std::make_pair("configuration", emptyUserSet));

    // init networking stuff
    InitChatService(port);
    InitAuthConn("127.0.0.1", 5556);
}

ChatServer::~ChatServer() { Shutdown(); }

// Initialization includes:
// 1. InitChatService Winsock: WSAStartup
// 2. getaddrinfo
// 3. create socket
// 4. bind
// 5. listen
int ChatServer::InitChatService(uint16 port) {
    // Declare and initialize variables
    WSADATA wsaData;
    int result;

    FD_ZERO(&m_ChatConn.readfds);

    // 1. WSAStartup
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed with error %d\n", result);
        return 1;
    } else {
        printf("WSAStartup OK!\n");
    }

    // 2. getaddrinfo
    ZeroMemory(&m_ChatConn.hints, sizeof(m_ChatConn.hints));
    m_ChatConn.hints.ai_family = AF_INET;        // IPV4
    m_ChatConn.hints.ai_socktype = SOCK_STREAM;  // Stream
    m_ChatConn.hints.ai_protocol = IPPROTO_TCP;  // TCP
    m_ChatConn.hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, std::to_string(port).c_str(), &m_ChatConn.hints, &m_ChatConn.info);
    if (result != 0) {
        printf("getaddrinfo failed with error: %d\n", result);
        WSACleanup();
        return result;
    } else {
        printf("getaddrinfo ok!\n");
    }

    // 3. Create our listen socket [Socket]
    m_ChatConn.listenSocket =
        socket(m_ChatConn.info->ai_family, m_ChatConn.info->ai_socktype, m_ChatConn.info->ai_protocol);
    if (m_ChatConn.listenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(m_ChatConn.info);
        WSACleanup();
        return result;
    } else {
        printf("socket OK!\n");
    }

    // 4. Bind our socket [Bind]
    result = bind(m_ChatConn.listenSocket, m_ChatConn.info->ai_addr, (int)m_ChatConn.info->ai_addrlen);
    if (result == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(m_ChatConn.info);
        closesocket(m_ChatConn.listenSocket);
        WSACleanup();
        return result;
    } else {
        printf("bind OK!\n");
    }

    // 5. [Listen]
    result = listen(m_ChatConn.listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(m_ChatConn.info);
        closesocket(m_ChatConn.listenSocket);
        WSACleanup();
        return result;
    } else {
        printf("listen OK!\n");
    }

    return result;
}

// Initialize connection to AuthServer
int ChatServer::InitAuthConn(const std::string& ip, uint16 port) {
    // Declare and initialize variables
    WSADATA wsaData;

    // 1. WSAStartup
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed with error %d\n", result);
        return 1;
    } else {
        printf("WSAStartup OK!\n");
    }

    // 2. getaddrinfo
    ZeroMemory(&m_AuthConn.hints, sizeof(m_AuthConn.hints));
    m_AuthConn.hints.ai_family = AF_INET;        // IPV4
    m_AuthConn.hints.ai_socktype = SOCK_STREAM;  // Stream
    m_AuthConn.hints.ai_protocol = IPPROTO_TCP;  // TCP

    result = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &m_AuthConn.hints, &m_AuthConn.info);
    if (result != 0) {
        printf("getaddrinfo failed with error: %d\n", result);
        WSACleanup();
        return result;
    } else {
        printf("getaddrinfo ok!\n");
    }

    // 3. Create our socket [Socket]
    m_AuthConn.authSocket =
        socket(m_AuthConn.info->ai_family, m_AuthConn.info->ai_socktype, m_AuthConn.info->ai_protocol);
    if (m_AuthConn.authSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(m_AuthConn.info);
        WSACleanup();
        return result;
    } else {
        printf("socket OK!\n");
    }

    // 4. Connect to server [Connect]
    result = connect(m_AuthConn.authSocket, m_AuthConn.info->ai_addr, (int)m_AuthConn.info->ai_addrlen);
    if (result == SOCKET_ERROR) {
        printf("connect AuthServer failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(m_AuthConn.info);
        closesocket(m_AuthConn.authSocket);
        WSACleanup();
        return result;
    } else {
        printf("connect AuthServer OK!\n");
    }

    return result;
}

int ChatServer::RunLoop() {
    // Define timeout for select()
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500 * 1000;  // 500 milliseconds, half a second

    FD_ZERO(&m_ChatConn.readfds);
    FD_SET(m_ChatConn.listenSocket, &m_ChatConn.readfds);

    // select work here
    while (true) {
        // Copy the fd_set, because select() will modify it
        fd_set copy = m_ChatConn.readfds;

        // FD_SET connected clients to copy by iterating through m_ChatConn.clients
        for (const std::pair<SOCKET, bool>& kv : m_ChatConn.clients) {
            if (kv.second) {
                FD_SET(kv.first, &copy);
            }
        }

        // Select will check all sockets in the SocketsReadyForReading set
        // to see if there is any data to be read on the socket.
        // https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select
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
            if (sock == m_ChatConn.listenSocket) {  // It's an incoming new connection
                // [Accept]
                SOCKET clientSocket = accept(m_ChatConn.listenSocket, NULL, NULL);
                if (clientSocket == INVALID_SOCKET) {
                    fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
                } else {
                    printf("accept OK!\n");
                    FD_SET(clientSocket, &m_ChatConn.readfds);

                    m_ChatConn.clients.insert({clientSocket, true});
                }
            } else {  // It's an incoming message
                ZeroMemory(&m_RawRecvBuf, kRECV_BUF_SIZE);
                int recvResult = recv(sock, m_RawRecvBuf, kRECV_BUF_SIZE, 0);

                std::map<SOCKET, bool>::iterator it = m_ChatConn.clients.find(sock);
                if (recvResult <= 0) {
                    if (recvResult < 0) {
                        fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
                    } else {
                        printf("client disconnected!\n");
                    }
                    if (it != m_ChatConn.clients.end()) {
                        it->second = false;
                    }
                    closesocket(sock);
                    FD_CLR(sock, &m_ChatConn.readfds);
                } else {
                    printf("recv %d bytes from client.\n", recvResult);

                    m_RecvBuf.Set(m_RawRecvBuf, kRECV_BUF_SIZE);
                    uint32 packetSize = m_RecvBuf.ReadUInt32LE();
                    MessageType messageType = static_cast<MessageType>(m_RecvBuf.ReadUInt32LE());

                    if (m_RecvBuf.Size() >= packetSize) {
                        uint32 headerSize = sizeof(uint32) * 2;
                        uint32 payloadSize = packetSize - headerSize;
                        HandleMessage(messageType, sock);
                    }
                }
            }
        }
    }
}

// Handle received messages
void ChatServer::HandleMessage(network::MessageType msgType, SOCKET socket) {
    uint32 headerSize = sizeof(uint32) * 2;  // packet header = packetSize(uint32) + messageType(uint32)
    uint32 payloadSize = m_RecvBuf.Size() - headerSize;
    const void* payloadHead = static_cast<const void*>(m_RecvBuf.ConstData() + headerSize);

    switch (msgType) {
        // received
        case MessageType::kCREATE_ACCOUNT_REQ: {
            uint32 emailLength = m_RecvBuf.ReadUInt32LE();
            std::string email = m_RecvBuf.ReadString(emailLength);
            uint32 passwordLength = m_RecvBuf.ReadUInt32LE();
            std::string password = m_RecvBuf.ReadString(passwordLength);

            // record the socket
            m_UserName2ClientSocketMap[email] = socket;
            m_ClientSocket2UserNameMap[socket] = email;

            printf("try creating account...\n");
            ReqCreateAccountWeb(socket, email, password);
        } break;

        case MessageType::kCREATE_ACCOUNT_WEB_SUCCESS_ACK: {
            auth::CreateAccountWebSuccess msg;
            msg.ParseFromArray(payloadHead, payloadSize);
            uint64 requestId = msg.requestid();  // requestId is the socket

            // find the socket's email
            std::map<SOCKET, std::string>::iterator it = m_ClientSocket2UserNameMap.find(requestId);
            if (it != m_ClientSocket2UserNameMap.end()) {
                std::string email = it->second;
                uint64_t userId = msg.userid();
                printf("'%s' has created account, userId: %llu.\n", email.c_str(), userId);
                AckCreateAccountSuccess(requestId, email, userId);
            }
        } break;

        case MessageType::kCREATE_ACCOUNT_WEB_FAILURE_ACK: {
            auth::CreateAccountWebFailure msg;
            msg.ParseFromArray(payloadHead, payloadSize);
            uint64 requestId = msg.requestid();  // requestId is the socket

            // find the socket's email
            std::map<SOCKET, std::string>::iterator it = m_ClientSocket2UserNameMap.find(requestId);
            if (it != m_ClientSocket2UserNameMap.end()) {
                std::string email = it->second;
                uint16 reason = static_cast<uint16>(msg.reason());
                printf("'%s' failed to create account, reason: %d.\n", email.c_str(), reason);
                AckCreateAccountFailure(requestId, reason, email);
            }
        } break;

        case MessageType::kAUTHENTICATE_ACCOUNT_REQ: {
            uint32 emailLength = m_RecvBuf.ReadUInt32LE();
            std::string email = m_RecvBuf.ReadString(emailLength);
            uint32 passwordLength = m_RecvBuf.ReadUInt32LE();
            std::string password = m_RecvBuf.ReadString(passwordLength);

            ReqAuthenticateAccountWeb(socket, email, password);
        } break;

        case MessageType::kAUTHENTICATE_ACCOUNT_WEB_SUCCESS_ACK: {
            auth::AuthenticateWebSuccess msg;
            msg.ParseFromArray(payloadHead, payloadSize);
            uint64 requestId = msg.requestid();  // requestId is the socket

            // find the socket's email
            std::map<SOCKET, std::string>::iterator it = m_ClientSocket2UserNameMap.find(requestId);
            if (it != m_ClientSocket2UserNameMap.end()) {
                std::string email = it->second;
                printf("'%s' has authenticated.\n", email.c_str());

                AckAuthenticateAccountSuccess(requestId, m_RoomNames);
            } else {
                printf("unknown socket: %llu.\n", requestId);
            }
        } break;

        case MessageType::kAUTHENTICATE_ACCOUNT_WEB_FAILURE_ACK: {
            auth::AuthenticateWebFailure msg;
            msg.ParseFromArray(payloadHead, payloadSize);
            uint64 requestId = msg.requestid();  // requestId is the socket

            // find the socket's email
            std::map<SOCKET, std::string>::iterator it = m_ClientSocket2UserNameMap.find(requestId);
            if (it != m_ClientSocket2UserNameMap.end()) {
                std::string email = it->second;
                uint16 reason = static_cast<uint16>(msg.reason());
                printf("'%s' failed to authenticate, reason: %d.\n", email.c_str(), reason);
                AckAuthenticateAccountFailure(requestId, reason, email);
            } else {
                printf("unknown socket: %llu.\n", requestId);
            }
        } break;

        // received C2S_JoinRoomReqMsg
        case MessageType::kJOIN_ROOM_REQ: {
            uint32 userNameLength = m_RecvBuf.ReadUInt32LE();
            std::string userName = m_RecvBuf.ReadString(userNameLength);
            uint32 roomNameLength = m_RecvBuf.ReadUInt32LE();
            std::string roomName = m_RecvBuf.ReadString(roomNameLength);

            printf("'%s' has joined #%s.\n", userName.c_str(), roomName.c_str());

            // add the user to room
            std::map<std::string, std::set<std::string>>::iterator it = m_RoomMap.find(roomName);
            if (it != m_RoomMap.end()) {
                std::set<std::string>& usersInRoom = it->second;
                usersInRoom.insert(userName);

                // respond with S2C_JoinRoomAckMsg SUCCESS
                std::vector<std::string> userNames{usersInRoom.begin(), usersInRoom.end()};
                AckJoinRoom(socket, MessageStatus::kSUCCESS, roomName, userNames);

                // broadcast event with S2C_JoinRoomNtfMsg
                BroadcastJoinRoom(usersInRoom, roomName, userName);
            } else {
                // respond with S2C_JoinRoomAckMsg FAILURE
                std::vector<std::string> placeHolder{};
                AckJoinRoom(socket, MessageStatus::kFAILURE, roomName, placeHolder);
            }

        } break;

        // received C2S_LeaveRoomReqMsg
        case MessageType::kLEAVE_ROOM_REQ: {
            uint32 roomNameLength = m_RecvBuf.ReadUInt32LE();
            std::string roomName = m_RecvBuf.ReadString(roomNameLength);
            uint32 userNameLength = m_RecvBuf.ReadUInt32LE();
            std::string userName = m_RecvBuf.ReadString(userNameLength);

            printf("'%s' has left #%s.\n", userName.c_str(), roomName.c_str());

            // remove the user from room
            std::map<std::string, std::set<std::string>>::iterator it = m_RoomMap.find(roomName);
            if (it != m_RoomMap.end()) {
                std::set<std::string>& usersInRoom = it->second;
                std::set<std::string>::iterator uit = usersInRoom.find(userName);
                if (uit != usersInRoom.end()) {
                    usersInRoom.erase(uit);
                }

                // respond with S2C_LeaveRoomAckMsg SUCCESS
                AckLeaveRoom(socket, MessageStatus::kSUCCESS, roomName, userName);

                // broadcast event with S2C_LeaveRoomNtfMsg
                BroadcastLeaveRoom(usersInRoom, roomName, userName);
            } else {
                // respond with S2C_LeaveRoomAckMsg FAILURE
                AckLeaveRoom(socket, MessageStatus::kFAILURE, roomName, userName);
            }

        } break;

        // received C2S_ChatInRoomReqMsg
        case MessageType::kCHAT_IN_ROOM_REQ: {
            uint32 roomNameLength = m_RecvBuf.ReadUInt32LE();
            std::string roomName = m_RecvBuf.ReadString(roomNameLength);
            uint32 userNameLength = m_RecvBuf.ReadUInt32LE();
            std::string userName = m_RecvBuf.ReadString(userNameLength);
            uint32 chatLength = m_RecvBuf.ReadUInt32LE();
            std::string chat = m_RecvBuf.ReadString(chatLength);

            printf("'%s' - #%s: %s.\n", userName.c_str(), roomName.c_str(), chat.c_str());

            std::map<std::string, std::set<std::string>>::iterator it = m_RoomMap.find(roomName);
            if (it != m_RoomMap.end()) {
                // respond with S2C_ChatInRoomAckMsg SUCCESS
                AckChatInRoom(socket, MessageStatus::kSUCCESS, roomName, userName);

                // broadcast event with S2C_ChatInRoomNtfMsg
                std::set<std::string>& usersInRoom = it->second;
                BroadcastChatInRoom(usersInRoom, roomName, userName, chat);

            } else {
                // respond with S2C_ChatInRoomAckMsg FAILURE
                AckChatInRoom(socket, MessageStatus::kFAILURE, roomName, userName);
            }

        } break;

        default:
            printf("unknown message.\n");
            break;
    }
}

int ChatServer::ReqCreateAccountWeb(SOCKET chatClientSocket, const std::string& email, const std::string& password) {
    auth::CreateAccountWeb msg;
    msg.set_requestid(chatClientSocket);
    msg.set_email(email);
    msg.set_plaintextpassword(password);

    // Serialize the message
    m_SendBuf.Reset();
    uint32 headerSize = sizeof(uint32) * 2;
    uint32 payloadSize = msg.ByteSizeLong();
    uint32 packetSize = headerSize + payloadSize;
    m_SendBuf.WriteUInt32LE(packetSize);
    m_SendBuf.WriteUInt32LE(static_cast<uint32>(MessageType::kCREATE_ACCOUNT_WEB_REQ));

    char* payloadHead = m_SendBuf.Data() + headerSize;
    msg.SerializeToArray(payloadHead, payloadSize);

    return SendMsg(m_AuthConn.authSocket, packetSize);
}

int ChatServer::ReqAuthenticateAccountWeb(SOCKET chatClientSocket, const std::string& email,
                                          const std::string& password) {
    auth::AuthenticateWeb msg;
    msg.set_requestid(chatClientSocket);
    msg.set_email(email);
    msg.set_plaintextpassword(password);

    // Serialize the message
    m_SendBuf.Reset();
    uint32 headerSize = sizeof(uint32) * 2;
    uint32 payloadSize = msg.ByteSizeLong();
    uint32 packetSize = headerSize + payloadSize;
    m_SendBuf.WriteUInt32LE(packetSize);
    m_SendBuf.WriteUInt32LE(static_cast<uint32>(MessageType::kAUTHENTICATE_ACCOUNT_WEB_REQ));

    char* payloadHead = m_SendBuf.Data() + headerSize;
    msg.SerializeToArray(payloadHead, payloadSize);

    return SendMsg(m_AuthConn.authSocket, packetSize);
}

// [send] S2C_CreateAccountSuccessAckMsg
int ChatServer::AckCreateAccountSuccess(SOCKET clientSocket, const std::string& email, uint64 userId) {
    S2C_CreateAccountSuccessAckMsg msg{email, userId};
    msg.Serialize(m_SendBuf);
    return SendMsg(clientSocket, msg.header.packetSize);
}

// [send] S2C_CreateAccountFailureAckMsg
int ChatServer::AckCreateAccountFailure(SOCKET clientSocket, uint16 reason, const std::string& email) {
    S2C_CreateAccountFailureAckMsg msg{reason, email};
    msg.Serialize(m_SendBuf);
    return SendMsg(clientSocket, msg.header.packetSize);
}

// [send] S2C_AuthenticateAccountSuccessAckMsg
int ChatServer::AckAuthenticateAccountSuccess(SOCKET clientSocket, const std::vector<std::string>& roomNames) {
    S2C_AuthenticateAccountSuccessAckMsg msg{roomNames};
    msg.Serialize(m_SendBuf);
    return SendMsg(clientSocket, msg.header.packetSize);
}

// [send] S2C_AuthenticateAccountFailureAckMsg
int ChatServer::AckAuthenticateAccountFailure(SOCKET clientSocket, uint16 reason, const std::string& email) {
    S2C_AuthenticateAccountFailureAckMsg msg{reason, email};
    msg.Serialize(m_SendBuf);
    return SendMsg(clientSocket, msg.header.packetSize);
}

// [send] S2C_JoinRoomAckMsg
int ChatServer::AckJoinRoom(SOCKET clientSocket, network::MessageStatus status, const std::string& roomName,
                            std::vector<std::string>& userNames) {
    S2C_JoinRoomAckMsg msg{static_cast<uint16>(status), roomName, userNames};
    msg.Serialize(m_SendBuf);
    return SendMsg(clientSocket, msg.header.packetSize);
}

// [send] S2C_JoinRoomNtfMsg
int ChatServer::BroadcastJoinRoom(const std::set<std::string>& usersInRoom, const std::string& roomName,
                                  const std::string& userName) {
    for (const std::string& name : usersInRoom) {
        if (name != userName) {
            std::map<std::string, SOCKET>::iterator it = m_UserName2ClientSocketMap.find(name);
            if (it != m_UserName2ClientSocketMap.end()) {
                S2C_JoinRoomNtfMsg msg{roomName, userName};
                msg.Serialize(m_SendBuf);
                SendMsg(it->second, msg.header.packetSize);
            }
        }
    }
    return 0;
}

// [send] S2C_LeaveRoomAckMsg
int ChatServer::AckLeaveRoom(SOCKET clientSocket, network::MessageStatus status, const std::string& roomName,
                             const std::string& userName) {
    S2C_LeaveRoomAckMsg msg{static_cast<uint16>(status), roomName, userName};
    msg.Serialize(m_SendBuf);
    return SendMsg(clientSocket, msg.header.packetSize);
}

// [send] S2C_LeaveRoomNtfMsg
int ChatServer::BroadcastLeaveRoom(const std::set<std::string>& usersInRoom, const std::string& roomName,
                                   const std::string& userName) {
    for (const std::string& name : usersInRoom) {
        std::map<std::string, SOCKET>::iterator it = m_UserName2ClientSocketMap.find(name);
        if (it != m_UserName2ClientSocketMap.end()) {
            S2C_LeaveRoomNtfMsg msg{roomName, userName};
            msg.Serialize(m_SendBuf);
            SendMsg(it->second, msg.header.packetSize);
        }
    }
    return 0;
}

// [send] S2C_ChatInRoomAckMsg
int ChatServer::AckChatInRoom(SOCKET clientSocket, network::MessageStatus status, const std::string& roomName,
                              const std::string& userName) {
    S2C_ChatInRoomAckMsg msg{MessageStatus::kSUCCESS, roomName, userName};
    msg.Serialize(m_SendBuf);
    return SendMsg(clientSocket, msg.header.packetSize);
}

// [send] S2C_ChatInRoomNtfMsg
int ChatServer::BroadcastChatInRoom(const std::set<std::string>& usersInRoom, const std::string& roomName,
                                    const std::string& userName, const std::string& chat) {
    for (const std::string& name : usersInRoom) {
        std::map<std::string, SOCKET>::iterator it = m_UserName2ClientSocketMap.find(name);
        if (it != m_UserName2ClientSocketMap.end()) {
            S2C_ChatInRoomNtfMsg msg{roomName, userName, chat};
            msg.Serialize(m_SendBuf);
            SendMsg(it->second, msg.header.packetSize);
        }
    }
    return 0;
}

// Send message
int ChatServer::SendMsg(SOCKET sock, uint32 packetSize) {
    // https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
    int sendResult = send(sock, m_SendBuf.ConstData(), packetSize, 0);
    if (sendResult == SOCKET_ERROR) {
        printf("send failed with error %d\n", WSAGetLastError());
        WSACleanup();
    }
    return 0;
}

// Shutdown and cleanup
void ChatServer::Shutdown() {
    printf("shutting down server ...\n");
    freeaddrinfo(m_ChatConn.info);
    closesocket(m_ChatConn.listenSocket);

    FD_CLR(m_ChatConn.listenSocket, &m_ChatConn.readfds);
    for (const std::pair<SOCKET, bool>& kv : m_ChatConn.clients) {
        if (kv.second) {
            closesocket(kv.first);
        }
    }

    WSACleanup();
}
