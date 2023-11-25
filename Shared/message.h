#pragma once

#include <string>
#include <vector>

#include "common.h"

namespace network {
// forward declaration
class Buffer;

// Naming convention:
// prefixes:
// C2S = client to server
// S2C = server to client
// S2A = server to AuthServer
// A2S = AuthServer to server
//
// suffixes:
// Req = request, the client request the server for service
// Ack = acknowledge, the server respond the client's request
// Ntf = notify, the server notify the clients

// The message type (protocol unique id)
enum MessageType {
    kCREATE_ACCOUNT_REQ = 1001,             // C2S
    kCREATE_ACCOUNT_SUCCESS_ACK,            // S2C
    kCREATE_ACCOUNT_FAILURE_ACK,            // S2C
    kCREATE_ACCOUNT_WEB_REQ,                // S2A
    kCREATE_ACCOUNT_WEB_SUCCESS_ACK,        // A2S
    kCREATE_ACCOUNT_WEB_FAILURE_ACK,        // A2S
    kAUTHENTICATE_ACCOUNT_REQ,              // C2S
    kAUTHENTICATE_ACCOUNT_SUCCESS_ACK,      // S2C
    kAUTHENTICATE_ACCOUNT_FAILURE_ACK,      // S2C
    kAUTHENTICATE_ACCOUNT_WEB_REQ,          // S2A
    kAUTHENTICATE_ACCOUNT_WEB_SUCCESS_ACK,  // A2S
    kAUTHENTICATE_ACCOUNT_WEB_FAILURE_ACK,  // A2S
    // kLOGIN_REQ,
    // kLOGIN_ACK,
    kJOIN_ROOM_REQ,
    kJOIN_ROOM_ACK,
    kJOIN_ROOM_NTF,
    kLEAVE_ROOM_REQ,
    kLEAVE_ROOM_ACK,
    kLEAVE_ROOM_NTF,
    kCHAT_IN_ROOM_REQ,
    kCHAT_IN_ROOM_ACK,
    kCHAT_IN_ROOM_NTF,

};

// The message status code
enum MessageStatus {
    kSUCCESS = 200,
    kFAILURE = 400,
    kERROR = 500,
};

// The fixed-length packet header
struct PacketHeader {
    uint32 packetSize;
    uint32 messageType;
};

// the Message (aka. protocol) base class
struct Message {
    PacketHeader header;
    virtual void Serialize(Buffer& buf);
};

// CreateAccount req message
struct C2S_CreateAccountReqMsg : public Message {
    uint32 emailLength;
    std::string email;
    uint32 passwordLength;
    std::string password;

    C2S_CreateAccountReqMsg(const std::string& strEmail, const std::string& strPassword);
    void Serialize(Buffer& buf) override;
};

// CreateAccountSuccess ack message
struct S2C_CreateAccountSuccessAckMsg : public Message {
    uint32 emailLength;
    std::string email;
    uint64 userId;

    S2C_CreateAccountSuccessAckMsg(const std::string& strEmail, uint64 lUserId);
    void Serialize(Buffer& buf) override;
};

// CreateAccountFailure ack message
struct S2C_CreateAccountFailureAckMsg : public Message {
    uint16 failureReason;
    uint32 emailLength;
    std::string email;

    S2C_CreateAccountFailureAckMsg(uint16 iReason, const std::string& strEmail);
    void Serialize(Buffer& buf) override;
};

// AuthenticateAccountSuccess ack message
struct S2C_AuthenticateAccountSuccessAckMsg : public Message {
    uint32 roomListLength;
    std::vector<uint32> roomNameLengths;
    std::vector<std::string> roomNames;

    S2C_AuthenticateAccountSuccessAckMsg(const std::vector<std::string>& vecRoomNames);
    void Serialize(Buffer& buf) override;
};

// AuthenticateAccountFailure ack message
struct S2C_AuthenticateAccountFailureAckMsg : public Message {
    uint16 failureReason;
    uint32 emailLength;
    std::string email;

    S2C_AuthenticateAccountFailureAckMsg(uint16 iReason, const std::string& strEmail);
    void Serialize(Buffer& buf) override;
};

// JoinRoom req message
struct C2S_JoinRoomReqMsg : public Message {
    uint32 userNameLength;
    std::string userName;
    uint32 roomNameLength;
    std::string roomName;

    C2S_JoinRoomReqMsg(const std::string& strUserName, const std::string& strRoomName);
    void Serialize(Buffer& buf) override;
};

// JoinRoom ack message
struct S2C_JoinRoomAckMsg : public Message {
    uint16 joinStatus;
    uint32 roomNameLength;
    std::string roomName;
    uint32 userListLength;
    std::vector<uint32> userNameLengths;
    std::vector<std::string> userNames;

    S2C_JoinRoomAckMsg(uint16 iStatus, const std::string& strRoomName, const std::vector<std::string>& vecUserNames);
    void Serialize(Buffer& buf) override;
};

// JoinRoom ntf message
// to broadcast the event that someone has joined the room
struct S2C_JoinRoomNtfMsg : public Message {
    uint32 roomNameLength;
    std::string roomName;
    uint32 userNameLength;
    std::string userName;

    S2C_JoinRoomNtfMsg(const std::string& strRoomName, const std::string& strUserName);
    void Serialize(Buffer& buf) override;
};

// LeaveRoom req message
struct C2S_LeaveRoomReqMsg : public Message {
    uint32 roomNameLength;
    std::string roomName;
    uint32 userNameLength;
    std::string userName;

    C2S_LeaveRoomReqMsg(const std::string& strRoomName, const std::string& strUserName);
    void Serialize(Buffer& buf) override;
};

// LeaveRoom ack message
struct S2C_LeaveRoomAckMsg : public Message {
    uint16 leaveStatus;
    uint32 roomNameLength;
    std::string roomName;
    uint32 userNameLength;
    std::string userName;

    S2C_LeaveRoomAckMsg(uint16 iStatus, const std::string& strRoomName, const std::string& strUserName);
    void Serialize(Buffer& buf) override;
};

// LeaveRoom ntf message
// to broadcast the event that someone has left the room
struct S2C_LeaveRoomNtfMsg : public Message {
    uint32 roomNameLength;
    std::string roomName;
    uint32 userNameLength;
    std::string userName;

    S2C_LeaveRoomNtfMsg(const std::string& strRoomName, const std::string& strUserName);
    void Serialize(Buffer& buf) override;
};

// ChatInRoom req message
struct C2S_ChatInRoomReqMsg : public Message {
    uint32 roomNameLength;
    std::string roomName;
    uint32 userNameLength;
    std::string userName;
    uint32 chatLength;
    std::string chat;

    C2S_ChatInRoomReqMsg(const std::string& strRoomName, const std::string& strUserName, const std::string& strChat);
    void Serialize(Buffer& buf) override;
};

// ChatInRoom ack message
struct S2C_ChatInRoomAckMsg : public Message {
    uint16 chatStatus;
    uint32 roomNameLength;
    std::string roomName;
    uint32 userNameLength;
    std::string userName;

    S2C_ChatInRoomAckMsg(uint16 iStatus, const std::string& strRoomName, const std::string& strUserName);
    void Serialize(Buffer& buf) override;
};

// ChatInRoom ntf message
// to broadcast someone's chat in a room
struct S2C_ChatInRoomNtfMsg : public Message {
    uint32 roomNameLength;
    std::string roomName;
    uint32 userNameLength;
    std::string userName;
    uint32 chatLength;
    std::string chat;

    S2C_ChatInRoomNtfMsg(const std::string& strRoomName, const std::string& strUserName, const std::string& strChat);
    void Serialize(Buffer& buf) override;
};

}  // end of namespace network
