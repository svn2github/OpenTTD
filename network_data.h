#ifndef NETWORK_DATA_H
#define NETWORK_DATA_H

#include "ttd.h"
#include "network.h"

// Is the network enabled?
#ifdef ENABLE_NETWORK

#define SEND_MTU 1460
#define MAX_TEXT_MSG_LEN 1024 /* long long long long sentences :-) */

// The client-info-server-index is always 1
#define NETWORK_SERVER_INDEX 1
#define NETWORK_EMPTY_INDEX 0

// What version of game-info do we use?
#define NETWORK_GAME_INFO_VERSION 1
// What version of company info is this?
#define NETWORK_COMPANY_INFO_VERSION 1
// What version of master-server-protocol do we use?
#define NETWORK_MASTER_SERVER_VERSION 1

typedef uint16 PacketSize;

typedef struct Packet {
	struct Packet *next;
	PacketSize size;
	PacketSize pos;
	byte buffer[SEND_MTU];
} Packet;

typedef struct CommandPacket {
	struct CommandPacket *next;
	byte player;
	uint32 cmd;
	uint32 p1;
	uint32 p2;
	uint32 tile; // Always make it uint32, so it is bigmap compatible
	uint32 dp[20]; // decode_params
	uint32 frame; // In which frame must this packet be executed?
	byte callback;
} CommandPacket;

typedef enum {
	STATUS_INACTIVE,
	STATUS_AUTH, // This means that the client is authorized
	STATUS_MAP_WAIT, // This means that the client is put on hold because someone else is getting the map
	STATUS_MAP,
	STATUS_DONE_MAP,
	STATUS_PRE_ACTIVE,
	STATUS_ACTIVE,
} ClientStatus;

typedef enum {
	MAP_PACKET_START,
	MAP_PACKET_NORMAL,
	MAP_PACKET_PATCH,
	MAP_PACKET_END,
} MapPacket;

typedef enum {
	NETWORK_RECV_STATUS_OKAY,
	NETWORK_RECV_STATUS_DESYNC,
	NETWORK_RECV_STATUS_SAVEGAME,
	NETWORK_RECV_STATUS_CONN_LOST,
	NETWORK_RECV_STATUS_MALFORMED_PACKET,
	NETWORK_RECV_STATUS_SERVER_ERROR, // The server told us we made an error
	NETWORK_RECV_STATUS_SERVER_FULL,
	NETWORK_RECV_STATUS_CLOSE_QUERY, // Done quering the server
} NetworkRecvStatus;

typedef enum {
	NETWORK_ERROR_GENERAL, // Try to use thisone like never

	// Signals from clients
	NETWORK_ERROR_DESYNC,
	NETWORK_ERROR_SAVEGAME_FAILED,
	NETWORK_ERROR_CONNECTION_LOST,
	NETWORK_ERROR_ILLEGAL_PACKET,

	// Signals from servers
	NETWORK_ERROR_NOT_AUTHORIZED,
	NETWORK_ERROR_NOT_EXPECTED,
	NETWORK_ERROR_WRONG_REVISION,
	NETWORK_ERROR_NAME_IN_USE,
	NETWORK_ERROR_WRONG_PASSWORD,
	NETWORK_ERROR_PLAYER_MISMATCH, // Happens in CLIENT_COMMAND
	NETWORK_ERROR_KICKED,
} NetworkErrorCode;

// Actions that can be used for NetworkTextMessage
typedef enum {
	NETWORK_ACTION_JOIN_LEAVE,
	NETWORK_ACTION_CHAT,
	NETWORK_ACTION_CHAT_PLAYER,
	NETWORK_ACTION_CHAT_CLIENT,
	NETWORK_ACTION_CHAT_TO_CLIENT,
	NETWORK_ACTION_CHAT_TO_PLAYER,
	NETWORK_ACTION_GIVE_MONEY,
	NETWORK_ACTION_NAME_CHANGE,
} NetworkAction;

typedef enum {
	NETWORK_GAME_PASSWORD,
	NETWORK_COMPANY_PASSWORD,
} NetworkPasswordType;

// To keep the clients all together
typedef struct ClientState {
	int socket;
	uint16 index;
	uint32 last_frame;
	uint32 last_frame_server;
	byte lag_test; // This byte is used for lag-testing the client

	ClientStatus status;
	bool writable; // is client ready to write to?
	bool quited;

	Packet *packet_queue; // Packets that are awaiting delivery
	Packet *packet_recv; // Partially received packet

	CommandPacket *command_queue; // The command-queue awaiting delivery
} ClientState;

// What packet types are there
// WARNING: The first 3 packets can NEVER change order again
//   it protects old clients from joining newer servers (because SERVER_ERROR
//   is the respond to a wrong revision)
typedef enum {
	PACKET_SERVER_FULL,
	PACKET_CLIENT_JOIN,
	PACKET_SERVER_ERROR,
	PACKET_CLIENT_COMPANY_INFO,
	PACKET_SERVER_COMPANY_INFO,
	PACKET_SERVER_CLIENT_INFO,
	PACKET_SERVER_NEED_PASSWORD,
	PACKET_CLIENT_PASSWORD,
	PACKET_SERVER_WELCOME,
	PACKET_CLIENT_GETMAP,
	PACKET_SERVER_WAIT,
	PACKET_SERVER_MAP,
	PACKET_CLIENT_MAP_OK,
	PACKET_SERVER_JOIN,
	PACKET_SERVER_FRAME,
	PACKET_SERVER_SYNC,
	PACKET_CLIENT_ACK,
	PACKET_CLIENT_COMMAND,
	PACKET_SERVER_COMMAND,
	PACKET_CLIENT_CHAT,
	PACKET_SERVER_CHAT,
	PACKET_CLIENT_SET_PASSWORD,
	PACKET_CLIENT_SET_NAME,
	PACKET_CLIENT_QUIT,
	PACKET_CLIENT_ERROR,
	PACKET_SERVER_QUIT,
	PACKET_SERVER_ERROR_QUIT,
	PACKET_SERVER_SHUTDOWN,
	PACKET_SERVER_NEWGAME,
	PACKET_END // Should ALWAYS be on the end of this list!! (period)
} PacketType;

typedef enum {
	DESTTYPE_BROADCAST,
	DESTTYPE_PLAYER,
	DESTTYPE_CLIENT
} DestType;

CommandPacket *_local_command_queue;

SOCKET _udp_client_socket; // udp client socket

// Here we keep track of the clients
//  (and the client uses [0] for his own communication)
ClientState _clients[MAX_CLIENTS];
#define DEREF_CLIENT(i) (&_clients[i])
// This returns the NetworkClientInfo from a ClientState
#define DEREF_CLIENT_INFO(cs) (&_network_client_info[cs - _clients])

// Macros to make life a bit more easier
#define DEF_CLIENT_RECEIVE_COMMAND(type) NetworkRecvStatus NetworkPacketReceive_ ## type ## _command(Packet *p)
#define DEF_CLIENT_SEND_COMMAND(type) void NetworkPacketSend_ ## type ## _command(void)
#define DEF_CLIENT_SEND_COMMAND_PARAM(type) void NetworkPacketSend_ ## type ## _command
#define DEF_SERVER_RECEIVE_COMMAND(type) void NetworkPacketReceive_ ## type ## _command(ClientState *cs, Packet *p)
#define DEF_SERVER_SEND_COMMAND(type) void NetworkPacketSend_ ## type ## _command(ClientState *cs)
#define DEF_SERVER_SEND_COMMAND_PARAM(type) void NetworkPacketSend_ ## type ## _command

#define SEND_COMMAND(type) NetworkPacketSend_ ## type ## _command
#define RECEIVE_COMMAND(type) NetworkPacketReceive_ ## type ## _command

#define FOR_ALL_CLIENTS(cs) for (cs = _clients; cs != &_clients[MAX_CLIENTS] && cs->socket != INVALID_SOCKET; cs++)

Packet *NetworkSend_Init(PacketType type);
void NetworkSend_uint8(Packet *packet, uint8 data);
void NetworkSend_uint16(Packet *packet, uint16 data);
void NetworkSend_uint32(Packet *packet, uint32 data);
void NetworkSend_uint64(Packet *packet, uint64 data);
void NetworkSend_string(Packet *packet, const char* data);
void NetworkSend_Packet(Packet *packet, ClientState *cs);

uint8 NetworkRecv_uint8(Packet *packet);
uint16 NetworkRecv_uint16(Packet *packet);
uint32 NetworkRecv_uint32(Packet *packet);
uint64 NetworkRecv_uint64(Packet *packet);
void NetworkRecv_string(Packet *packet, char* buffer, size_t size);
Packet *NetworkRecv_Packet(ClientState *cs, NetworkRecvStatus *status);

bool NetworkSend_Packets(ClientState *cs);
void NetworkExecuteCommand(CommandPacket *cp);
void NetworkAddCommandQueue(ClientState *cs, CommandPacket *cp);

// from network.c
void CloseClient(ClientState *cs);
void CDECL NetworkTextMessage(NetworkAction action, uint16 color, const char *name, const char *str, ...);
void NetworkGetClientName(char *clientname, size_t size, ClientState *cs);
uint NetworkCalculateLag(const ClientState *cs);
byte NetworkGetCurrentLanguageIndex();
NetworkClientInfo *NetworkFindClientInfoFromIndex(uint16 client_index);
ClientState *NetworkFindClientStateFromIndex(uint16 client_index);
unsigned long NetworkResolveHost(const char *hostname);

#endif /* ENABLE_NETWORK */

#endif // NETWORK_DATA_H
