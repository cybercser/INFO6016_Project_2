#include "message.h"

#include "buffer.h"

namespace network {

void Message::Serialize(Buffer& buf) {
    buf.Reset();
    buf.WriteUInt32LE(header.packetSize);
    buf.WriteUInt32LE(header.messageType);
}

// CreateAccount req message
C2S_CreateAccountReqMsg::C2S_CreateAccountReqMsg(const std::string& strEmail, const std::string& strPassword)
    : email(strEmail), password(strPassword) {
    emailLength = email.size();
    passwordLength = password.size();

    header.messageType = MessageType::kCREATE_ACCOUNT_REQ;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(emailLength) + emailLength;
    header.packetSize += sizeof(passwordLength) + passwordLength;
}

void C2S_CreateAccountReqMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(emailLength);
    buf.WriteString(email, emailLength);
    buf.WriteUInt32LE(passwordLength);
    buf.WriteString(password, passwordLength);
}

// CreateAccountSuccess ack message
S2C_CreateAccountSuccessAckMsg::S2C_CreateAccountSuccessAckMsg(const std::string& strEmail, uint64 lUserId)
    : email(strEmail), userId(lUserId) {
    emailLength = email.size();

    header.messageType = MessageType::kCREATE_ACCOUNT_SUCCESS_ACK;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(emailLength) + emailLength;
    header.packetSize += sizeof(userId);
}

void S2C_CreateAccountSuccessAckMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(emailLength);
    buf.WriteString(email, emailLength);
    buf.WriteUInt64LE(userId);
}

// CreateAccountFailure ack message
S2C_CreateAccountFailureAckMsg::S2C_CreateAccountFailureAckMsg(uint16 iReason, const std::string& strEmail)
    : failureReason(iReason), email(strEmail) {
    emailLength = email.size();

    header.messageType = MessageType::kCREATE_ACCOUNT_FAILURE_ACK;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(failureReason);
    header.packetSize += sizeof(emailLength) + emailLength;
}

void S2C_CreateAccountFailureAckMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt16LE(failureReason);
    buf.WriteUInt32LE(emailLength);
    buf.WriteString(email, emailLength);
}

// AuthenticcateAccountSuccess ack message
S2C_AuthenticateAccountSuccessAckMsg::S2C_AuthenticateAccountSuccessAckMsg(
    const std::vector<std::string>& vecRoomNames) {
    roomListLength = vecRoomNames.size();
    for (const std::string& roomName : vecRoomNames) {
        roomNameLengths.push_back(roomName.size());
        roomNames.push_back(roomName);
    }

    header.messageType = MessageType::kAUTHENTICATE_ACCOUNT_SUCCESS_ACK;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(roomListLength);
    header.packetSize += sizeof(uint32_t) * roomNameLengths.size();
    for (uint32_t len : roomNameLengths) {
        header.packetSize += len;
    }
}

void S2C_AuthenticateAccountSuccessAckMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(roomListLength);
    for (size_t i = 0; i < roomListLength; i++) {
        buf.WriteUInt32LE(roomNameLengths[i]);
    }
    for (size_t i = 0; i < roomListLength; i++) {
        buf.WriteString(roomNames[i], roomNameLengths[i]);
    }
}

S2C_AuthenticateAccountFailureAckMsg::S2C_AuthenticateAccountFailureAckMsg(uint16 iReason, const std::string& strEmail)
    : failureReason(iReason), email(strEmail) {
    emailLength = email.size();

    header.messageType = MessageType::kAUTHENTICATE_ACCOUNT_FAILURE_ACK;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(failureReason);
    header.packetSize += sizeof(emailLength) + emailLength;
}

void S2C_AuthenticateAccountFailureAckMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt16LE(failureReason);
    buf.WriteUInt32LE(emailLength);
    buf.WriteString(email, emailLength);
}

// JoinRoom req message
C2S_JoinRoomReqMsg::C2S_JoinRoomReqMsg(const std::string& strUserName, const std::string& strRoomName)
    : userName(strUserName), roomName(strRoomName) {
    userNameLength = strUserName.size();
    roomNameLength = strRoomName.size();

    header.messageType = MessageType::kJOIN_ROOM_REQ;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(userNameLength) + userNameLength;
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
}

void C2S_JoinRoomReqMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(userNameLength);
    buf.WriteString(userName, userNameLength);
    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
}

// JoinRoom ack message
S2C_JoinRoomAckMsg::S2C_JoinRoomAckMsg(uint16 iStatus, const std::string& strRoomName,
                                       const std::vector<std::string>& vecUserNames)
    : joinStatus(iStatus), roomName(strRoomName) {
    roomNameLength = strRoomName.size();
    userListLength = vecUserNames.size();
    for (const std::string& userName : vecUserNames) {
        userNameLengths.push_back(userName.size());
        userNames.push_back(userName);
    }

    header.messageType = MessageType::kJOIN_ROOM_ACK;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(joinStatus);
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
    header.packetSize += sizeof(userListLength);
    header.packetSize += sizeof(uint32_t) * userNameLengths.size();
    for (uint32_t len : userNameLengths) {
        header.packetSize += len;
    }
}

void S2C_JoinRoomAckMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt16LE(joinStatus);
    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
    buf.WriteUInt32LE(userListLength);
    for (size_t i = 0; i < userListLength; i++) {
        buf.WriteUInt32LE(userNameLengths[i]);
    }
    for (size_t i = 0; i < userListLength; i++) {
        buf.WriteString(userNames[i], userNameLengths[i]);
    }
}

// S2C_JoinRoomNtfMsg
S2C_JoinRoomNtfMsg::S2C_JoinRoomNtfMsg(const std::string& strRoomName, const std::string& strUserName)
    : roomName(strRoomName), userName(strUserName) {
    roomNameLength = strRoomName.size();
    userNameLength = strUserName.size();

    header.messageType = MessageType::kJOIN_ROOM_NTF;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
    header.packetSize += sizeof(userNameLength) + userNameLength;
}

void S2C_JoinRoomNtfMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
    buf.WriteUInt32LE(userNameLength);
    buf.WriteString(userName, userNameLength);
}

// C2S_LeaveRoomReqMsg
C2S_LeaveRoomReqMsg::C2S_LeaveRoomReqMsg(const std::string& strRoomName, const std::string& strUserName)
    : roomName(strRoomName), userName(strUserName) {
    roomNameLength = strRoomName.size();
    userNameLength = strUserName.size();

    header.messageType = MessageType::kLEAVE_ROOM_REQ;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
    header.packetSize += sizeof(userNameLength) + userNameLength;
}

void C2S_LeaveRoomReqMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
    buf.WriteUInt32LE(userNameLength);
    buf.WriteString(userName, userNameLength);
}

// S2C_LeaveRoomAckMsg
S2C_LeaveRoomAckMsg::S2C_LeaveRoomAckMsg(uint16 iStatus, const std::string& strRoomName, const std::string& strUserName)
    : leaveStatus(iStatus), roomName(strRoomName), userName(strUserName) {
    roomNameLength = strRoomName.size();
    userNameLength = strUserName.size();

    header.messageType = MessageType::kLEAVE_ROOM_ACK;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(leaveStatus);
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
    header.packetSize += sizeof(userNameLength) + userNameLength;
}

void S2C_LeaveRoomAckMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt16LE(leaveStatus);
    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
    buf.WriteUInt32LE(userNameLength);
    buf.WriteString(userName, userNameLength);
}

// S2C_LeaveRoomNtfMsg
S2C_LeaveRoomNtfMsg::S2C_LeaveRoomNtfMsg(const std::string& strRoomName, const std::string& strUserName)
    : roomName(strRoomName), userName(strUserName) {
    roomNameLength = strRoomName.size();
    userNameLength = strUserName.size();

    header.messageType = MessageType::kLEAVE_ROOM_NTF;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
    header.packetSize += sizeof(userNameLength) + userNameLength;
}

void S2C_LeaveRoomNtfMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
    buf.WriteUInt32LE(userNameLength);
    buf.WriteString(userName, userNameLength);
}

// C2S_ChatInRoomReqMsg
C2S_ChatInRoomReqMsg::C2S_ChatInRoomReqMsg(const std::string& strRoomName, const std::string& strUserName,
                                           const std::string& strChat)
    : roomName(strRoomName), userName(strUserName), chat(strChat) {
    roomNameLength = strRoomName.size();
    userNameLength = strUserName.size();
    chatLength = strChat.size();

    header.messageType = MessageType::kCHAT_IN_ROOM_REQ;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
    header.packetSize += sizeof(userNameLength) + userNameLength;
    header.packetSize += sizeof(chatLength) + chatLength;
}

void C2S_ChatInRoomReqMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
    buf.WriteUInt32LE(userNameLength);
    buf.WriteString(userName, userNameLength);
    buf.WriteUInt32LE(chatLength);
    buf.WriteString(chat, chatLength);
}

// S2C_ChatInRoomAckMsg
S2C_ChatInRoomAckMsg::S2C_ChatInRoomAckMsg(uint16 iStatus, const std::string& strRoomName,
                                           const std::string& strUserName)
    : chatStatus(iStatus), roomName(strRoomName), userName(strUserName) {
    roomNameLength = strRoomName.size();
    userNameLength = strUserName.size();

    header.messageType = MessageType::kCHAT_IN_ROOM_ACK;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(chatStatus);
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
    header.packetSize += sizeof(userNameLength) + userNameLength;
}

void S2C_ChatInRoomAckMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt16LE(chatStatus);
    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
    buf.WriteUInt32LE(userNameLength);
    buf.WriteString(userName, userNameLength);
}

// S2C_ChatInRoomNtfMsg
S2C_ChatInRoomNtfMsg::S2C_ChatInRoomNtfMsg(const std::string& strRoomName, const std::string& strUserName,
                                           const std::string& strChat)
    : roomName(strRoomName), userName(strUserName), chat(strChat) {
    roomNameLength = strRoomName.size();
    userNameLength = strUserName.size();
    chatLength = strChat.size();

    header.messageType = MessageType::kCHAT_IN_ROOM_NTF;
    header.packetSize = sizeof(PacketHeader);
    header.packetSize += sizeof(roomNameLength) + roomNameLength;
    header.packetSize += sizeof(userNameLength) + userNameLength;
    header.packetSize += sizeof(chatLength) + chatLength;
}

void S2C_ChatInRoomNtfMsg::Serialize(Buffer& buf) {
    Message::Serialize(buf);

    buf.WriteUInt32LE(roomNameLength);
    buf.WriteString(roomName, roomNameLength);
    buf.WriteUInt32LE(userNameLength);
    buf.WriteString(userName, userNameLength);
    buf.WriteUInt32LE(chatLength);
    buf.WriteString(chat, chatLength);
}

}  // end of namespace network
