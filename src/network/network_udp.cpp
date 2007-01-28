/* $Id$ */

#ifdef ENABLE_NETWORK

#include "../stdafx.h"
#include "../debug.h"
#include "../string.h"
#include "network_data.h"
#include "../date.h"
#include "../map.h"
#include "network_gamelist.h"
#include "network_udp.h"
#include "../variables.h"
#include "../newgrf_config.h"

#include "core/udp.h"

/**
 * @file network_udp.c This file handles the UDP related communication.
 *
 * This is the GameServer <-> MasterServer and GameServer <-> GameClient
 * communication before the game is being joined.
 */

enum {
	ADVERTISE_NORMAL_INTERVAL = 30000, // interval between advertising in ticks (15 minutes)
	ADVERTISE_RETRY_INTERVAL  =   300, // readvertise when no response after this many ticks (9 seconds)
	ADVERTISE_RETRY_TIMES     =     3  // give up readvertising after this much failed retries
};

NetworkUDPSocketHandler *_udp_client_socket; ///< udp client socket
NetworkUDPSocketHandler *_udp_server_socket; ///< udp server socket
NetworkUDPSocketHandler *_udp_master_socket; ///< udp master socket

///*** Communication with the masterserver ***/

class MasterNetworkUDPSocketHandler : public NetworkUDPSocketHandler {
protected:
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_MASTER_ACK_REGISTER);
public:
	virtual ~MasterNetworkUDPSocketHandler() {}
};

DEF_UDP_RECEIVE_COMMAND(Master, PACKET_UDP_MASTER_ACK_REGISTER)
{
	_network_advertise_retries = 0;
	DEBUG(net, 2, "[udp] advertising on master server successfull");

	/* We are advertised, but we don't want to! */
	if (!_network_advertise) NetworkUDPRemoveAdvertise();
}

///*** Communication with clients (we are server) ***/

class ServerNetworkUDPSocketHandler : public NetworkUDPSocketHandler {
protected:
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_CLIENT_FIND_SERVER);
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_CLIENT_DETAIL_INFO);
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_CLIENT_GET_NEWGRFS);
public:
	virtual ~ServerNetworkUDPSocketHandler() {}
};

DEF_UDP_RECEIVE_COMMAND(Server, PACKET_UDP_CLIENT_FIND_SERVER)
{
	Packet *packet;
	// Just a fail-safe.. should never happen
	if (!_network_udp_server)
		return;

	packet = NetworkSend_Init(PACKET_UDP_SERVER_RESPONSE);

	// Update some game_info
	_network_game_info.game_date     = _date;
	_network_game_info.map_width     = MapSizeX();
	_network_game_info.map_height    = MapSizeY();
	_network_game_info.map_set       = _opt.landscape;
	_network_game_info.companies_on  = ActivePlayerCount();
	_network_game_info.spectators_on = NetworkSpectatorCount();
	_network_game_info.grfconfig     = _grfconfig;

	this->Send_NetworkGameInfo(packet, &_network_game_info);

	// Let the client know that we are here
	this->SendPacket(packet, client_addr);

	free(packet);

	DEBUG(net, 2, "[udp] queried from '%s'", inet_ntoa(client_addr->sin_addr));
}

DEF_UDP_RECEIVE_COMMAND(Server, PACKET_UDP_CLIENT_DETAIL_INFO)
{
	NetworkTCPSocketHandler *cs;
	NetworkClientInfo *ci;
	Packet *packet;
	Player *player;
	byte current = 0;
	int i;

	// Just a fail-safe.. should never happen
	if (!_network_udp_server) return;

	packet = NetworkSend_Init(PACKET_UDP_SERVER_DETAIL_INFO);

	/* Send the amount of active companies */
	NetworkSend_uint8 (packet, NETWORK_COMPANY_INFO_VERSION);
	NetworkSend_uint8 (packet, ActivePlayerCount());

	/* Fetch the latest version of everything */
	NetworkPopulateCompanyInfo();

	/* Go through all the players */
	FOR_ALL_PLAYERS(player) {
		/* Skip non-active players */
		if (!player->is_active) continue;

		current++;

		/* Send the information */
		NetworkSend_uint8(packet, current);

		NetworkSend_string(packet, _network_player_info[player->index].company_name);
		NetworkSend_uint32(packet, _network_player_info[player->index].inaugurated_year);
		NetworkSend_uint64(packet, _network_player_info[player->index].company_value);
		NetworkSend_uint64(packet, _network_player_info[player->index].money);
		NetworkSend_uint64(packet, _network_player_info[player->index].income);
		NetworkSend_uint16(packet, _network_player_info[player->index].performance);

		/* Send 1 if there is a passord for the company else send 0 */
		if (_network_player_info[player->index].password[0] != '\0') {
			NetworkSend_uint8(packet, 1);
		} else {
			NetworkSend_uint8(packet, 0);
		}

		for (i = 0; i < NETWORK_VEHICLE_TYPES; i++)
			NetworkSend_uint16(packet, _network_player_info[player->index].num_vehicle[i]);

		for (i = 0; i < NETWORK_STATION_TYPES; i++)
			NetworkSend_uint16(packet, _network_player_info[player->index].num_station[i]);

		/* Find the clients that are connected to this player */
		FOR_ALL_CLIENTS(cs) {
			ci = DEREF_CLIENT_INFO(cs);
			if (ci->client_playas == player->index) {
				/* The uint8 == 1 indicates that a client is following */
				NetworkSend_uint8(packet, 1);
				NetworkSend_string(packet, ci->client_name);
				NetworkSend_string(packet, ci->unique_id);
				NetworkSend_uint32(packet, ci->join_date);
			}
		}
		/* Also check for the server itself */
		ci = NetworkFindClientInfoFromIndex(NETWORK_SERVER_INDEX);
		if (ci->client_playas == player->index) {
			/* The uint8 == 1 indicates that a client is following */
			NetworkSend_uint8(packet, 1);
			NetworkSend_string(packet, ci->client_name);
			NetworkSend_string(packet, ci->unique_id);
			NetworkSend_uint32(packet, ci->join_date);
		}

		/* Indicates end of client list */
		NetworkSend_uint8(packet, 0);
	}

	/* And check if we have any spectators */
	FOR_ALL_CLIENTS(cs) {
		ci = DEREF_CLIENT_INFO(cs);
		if (!IsValidPlayer(ci->client_playas)) {
			/* The uint8 == 1 indicates that a client is following */
			NetworkSend_uint8(packet, 1);
			NetworkSend_string(packet, ci->client_name);
			NetworkSend_string(packet, ci->unique_id);
			NetworkSend_uint32(packet, ci->join_date);
		}
	}

	/* Also check for the server itself */
	ci = NetworkFindClientInfoFromIndex(NETWORK_SERVER_INDEX);
	if (!IsValidPlayer(ci->client_playas)) {
		/* The uint8 == 1 indicates that a client is following */
		NetworkSend_uint8(packet, 1);
		NetworkSend_string(packet, ci->client_name);
		NetworkSend_string(packet, ci->unique_id);
		NetworkSend_uint32(packet, ci->join_date);
	}

	/* Indicates end of client list */
	NetworkSend_uint8(packet, 0);

	this->SendPacket(packet, client_addr);
	free(packet);
}

/**
 * A client has requested the names of some NewGRFs.
 *
 * Replying this can be tricky as we have a limit of SEND_MTU bytes
 * in the reply packet and we can send up to 100 bytes per NewGRF
 * (GRF ID, MD5sum and NETWORK_GRF_NAME_LENGTH bytes for the name).
 * As SEND_MTU is _much_ less than 100 * NETWORK_MAX_GRF_COUNT, it
 * could be that a packet overflows. To stop this we only reply
 * with the first N NewGRFs so that if the first N + 1 NewGRFs
 * would be sent, the packet overflows.
 * in_reply and in_reply_count are used to keep a list of GRFs to
 * send in the reply.
 */
DEF_UDP_RECEIVE_COMMAND(Server, PACKET_UDP_CLIENT_GET_NEWGRFS)
{
	uint8 num_grfs;
	uint i;

	const GRFConfig *in_reply[NETWORK_MAX_GRF_COUNT];
	Packet *packet;
	uint8 in_reply_count = 0;
	uint packet_len = 0;

	DEBUG(net, 6, "[udp] newgrf data request from %s:%d", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

	num_grfs = NetworkRecv_uint8 (this, p);
	if (num_grfs > NETWORK_MAX_GRF_COUNT) return;

	for (i = 0; i < num_grfs; i++) {
		GRFConfig c;
		const GRFConfig *f;

		this->Recv_GRFIdentifier(p, &c);

		/* Find the matching GRF file */
		f = FindGRFConfig(c.grfid, c.md5sum);
		if (f == NULL) continue; // The GRF is unknown to this server

		/* If the reply might exceed the size of the packet, only reply
		 * the current list and do not send the other data.
		 * The name could be an empty string, if so take the filename. */
		packet_len += sizeof(c.grfid) + sizeof(c.md5sum) +
				min(strlen((f->name != NULL && !StrEmpty(f->name)) ? f->name : f->filename) + 1, (size_t)NETWORK_GRF_NAME_LENGTH);
		if (packet_len > SEND_MTU - 4) { // 4 is 3 byte header + grf count in reply
			break;
		}
		in_reply[in_reply_count] = f;
		in_reply_count++;
	}

	if (in_reply_count == 0) return;

	packet = NetworkSend_Init(PACKET_UDP_SERVER_NEWGRFS);
	NetworkSend_uint8 (packet, in_reply_count);
	for (i = 0; i < in_reply_count; i++) {
		char name[NETWORK_GRF_NAME_LENGTH];

		/* The name could be an empty string, if so take the filename */
		ttd_strlcpy(name, (in_reply[i]->name != NULL && !StrEmpty(in_reply[i]->name)) ?
				in_reply[i]->name : in_reply[i]->filename, sizeof(name));
	 	this->Send_GRFIdentifier(packet, in_reply[i]);
		NetworkSend_string(packet, name);
	}

	this->SendPacket(packet, client_addr);
	free(packet);
}

///*** Communication with servers (we are client) ***/

class ClientNetworkUDPSocketHandler : public NetworkUDPSocketHandler {
protected:
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_SERVER_RESPONSE);
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_MASTER_RESPONSE_LIST);
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_SERVER_NEWGRFS);
	virtual void HandleIncomingNetworkGameInfoGRFConfig(GRFConfig *config);
public:
	virtual ~ClientNetworkUDPSocketHandler() {}
};

DEF_UDP_RECEIVE_COMMAND(Client, PACKET_UDP_SERVER_RESPONSE)
{
	extern const char _openttd_revision[];
	NetworkGameList *item;

	// Just a fail-safe.. should never happen
	if (_network_udp_server) return;

	DEBUG(net, 4, "[udp] server response from %s:%d", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

	// Find next item
	item = NetworkGameListAddItem(inet_addr(inet_ntoa(client_addr->sin_addr)), ntohs(client_addr->sin_port));

	this->Recv_NetworkGameInfo(p, &item->info);

	item->info.compatible = true;
	{
		/* Checks whether there needs to be a request for names of GRFs and makes
		 * the request if necessary. GRFs that need to be requested are the GRFs
		 * that do not exist on the clients system and we do not have the name
		 * resolved of, i.e. the name is still UNKNOWN_GRF_NAME_PLACEHOLDER.
		 * The in_request array and in_request_count are used so there is no need
		 * to do a second loop over the GRF list, which can be relatively expensive
		 * due to the string comparisons. */
		const GRFConfig *in_request[NETWORK_MAX_GRF_COUNT];
		const GRFConfig *c;
		uint in_request_count = 0;
		struct sockaddr_in out_addr;

		for (c = item->info.grfconfig; c != NULL; c = c->next) {
			if (HASBIT(c->flags, GCF_NOT_FOUND)) item->info.compatible = false;
			if (!HASBIT(c->flags, GCF_NOT_FOUND) || strcmp(c->name, UNKNOWN_GRF_NAME_PLACEHOLDER) != 0) continue;
			in_request[in_request_count] = c;
			in_request_count++;
		}

		if (in_request_count > 0) {
			/* There are 'unknown' GRFs, now send a request for them */
			uint i;
			Packet *packet = NetworkSend_Init(PACKET_UDP_CLIENT_GET_NEWGRFS);

			NetworkSend_uint8 (packet, in_request_count);
			for (i = 0; i < in_request_count; i++) {
				this->Send_GRFIdentifier(packet, in_request[i]);
			}

			out_addr.sin_family      = AF_INET;
			out_addr.sin_port        = htons(item->port);
			out_addr.sin_addr.s_addr = item->ip;
			this->SendPacket(packet, &out_addr);
			free(packet);
		}
	}

	if (item->info.hostname[0] == '\0')
		snprintf(item->info.hostname, sizeof(item->info.hostname), "%s", inet_ntoa(client_addr->sin_addr));

	/* Check if we are allowed on this server based on the revision-match */
	item->info.version_compatible =
		strcmp(item->info.server_revision, _openttd_revision) == 0 ||
		strcmp(item->info.server_revision, NOREV_STRING) == 0;
	item->info.compatible &= item->info.version_compatible; // Already contains match for GRFs

	item->online = true;

	UpdateNetworkGameWindow(false);
}

DEF_UDP_RECEIVE_COMMAND(Client, PACKET_UDP_MASTER_RESPONSE_LIST)
{
	int i;
	struct in_addr ip;
	uint16 port;
	uint8 ver;

	/* packet begins with the protocol version (uint8)
	 * then an uint16 which indicates how many
	 * ip:port pairs are in this packet, after that
	 * an uint32 (ip) and an uint16 (port) for each pair
	 */

	ver = NetworkRecv_uint8(this, p);

	if (ver == 1) {
		for (i = NetworkRecv_uint16(this, p); i != 0 ; i--) {
			ip.s_addr = TO_LE32(NetworkRecv_uint32(this, p));
			port = NetworkRecv_uint16(this, p);

			/* Somehow we reached the end of the packet */
			if (this->HasClientQuit()) return;
			NetworkUDPQueryServer(inet_ntoa(ip), port);
		}
	}
}

/** The return of the client's request of the names of some NewGRFs */
DEF_UDP_RECEIVE_COMMAND(Client, PACKET_UDP_SERVER_NEWGRFS)
{
	uint8 num_grfs;
	uint i;

	DEBUG(net, 6, "[udp] newgrf data reply from %s:%d", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

	num_grfs = NetworkRecv_uint8 (this, p);
	if (num_grfs > NETWORK_MAX_GRF_COUNT) return;

	for (i = 0; i < num_grfs; i++) {
		char *unknown_name;
		char name[NETWORK_GRF_NAME_LENGTH];
		GRFConfig c;

		this->Recv_GRFIdentifier(p, &c);
		NetworkRecv_string(this, p, name, sizeof(name));

		/* An empty name is not possible under normal circumstances
		 * and causes problems when showing the NewGRF list. */
		if (StrEmpty(name)) continue;

		/* Finds the fake GRFConfig for the just read GRF ID and MD5sum tuple.
		 * If it exists and not resolved yet, then name of the fake GRF is
		 * overwritten with the name from the reply. */
		unknown_name = FindUnknownGRFName(c.grfid, c.md5sum, false);
		if (unknown_name != NULL && strcmp(unknown_name, UNKNOWN_GRF_NAME_PLACEHOLDER) == 0) {
			ttd_strlcpy(unknown_name, name, NETWORK_GRF_NAME_LENGTH);
		}
	}
}

void ClientNetworkUDPSocketHandler::HandleIncomingNetworkGameInfoGRFConfig(GRFConfig *config)
{
	/* Find the matching GRF file */
	const GRFConfig *f = FindGRFConfig(config->grfid, config->md5sum);
	if (f == NULL) {
		/* Don't know the GRF, so mark game incompatible and the (possibly)
		 * already resolved name for this GRF (another server has sent the
		 * name of the GRF already */
		config->name     = FindUnknownGRFName(config->grfid, config->md5sum, true);
		SETBIT(config->flags, GCF_NOT_FOUND);
	} else {
		config->filename = f->filename;
		config->name     = f->name;
		config->info     = f->info;
	}
	SETBIT(config->flags, GCF_COPY);
}

// Close UDP connection
void NetworkUDPCloseAll(void)
{
	DEBUG(net, 1, "[udp] closed listeners");

	_udp_server_socket->Close();
	_udp_master_socket->Close();
	_udp_client_socket->Close();

	_network_udp_server = false;
	_network_udp_broadcast = 0;
}

// Broadcast to all ips
static void NetworkUDPBroadCast(NetworkUDPSocketHandler *socket)
{
	Packet* p = NetworkSend_Init(PACKET_UDP_CLIENT_FIND_SERVER);
	uint i;

	for (i = 0; _broadcast_list[i] != 0; i++) {
		struct sockaddr_in out_addr;

		out_addr.sin_family = AF_INET;
		out_addr.sin_port = htons(_network_server_port);
		out_addr.sin_addr.s_addr = _broadcast_list[i];

		DEBUG(net, 4, "[udp] broadcasting to %s", inet_ntoa(out_addr.sin_addr));

		socket->SendPacket(p, &out_addr);
	}

	free(p);
}


// Request the the server-list from the master server
void NetworkUDPQueryMasterServer(void)
{
	struct sockaddr_in out_addr;
	Packet *p;

	if (!_udp_client_socket->IsConnected()) {
		if (!_udp_client_socket->Listen(0, 0, true)) return;
	}

	p = NetworkSend_Init(PACKET_UDP_CLIENT_GET_LIST);

	out_addr.sin_family = AF_INET;
	out_addr.sin_port = htons(NETWORK_MASTER_SERVER_PORT);
	out_addr.sin_addr.s_addr = NetworkResolveHost(NETWORK_MASTER_SERVER_HOST);

	// packet only contains protocol version
	NetworkSend_uint8(p, NETWORK_MASTER_SERVER_VERSION);

	_udp_client_socket->SendPacket(p, &out_addr);

	DEBUG(net, 2, "[udp] master server queried at %s:%d", inet_ntoa(out_addr.sin_addr),ntohs(out_addr.sin_port));

	free(p);
}

// Find all servers
void NetworkUDPSearchGame(void)
{
	// We are still searching..
	if (_network_udp_broadcast > 0) return;

	// No UDP-socket yet..
	if (!_udp_client_socket->IsConnected()) {
		if (!_udp_client_socket->Listen(0, 0, true)) return;
	}

	DEBUG(net, 0, "[udp] searching server");

	NetworkUDPBroadCast(_udp_client_socket);
	_network_udp_broadcast = 300; // Stay searching for 300 ticks
}

NetworkGameList *NetworkUDPQueryServer(const char* host, unsigned short port)
{
	struct sockaddr_in out_addr;
	Packet *p;
	NetworkGameList *item;

	// No UDP-socket yet..
	if (!_udp_client_socket->IsConnected()) {
		if (!_udp_client_socket->Listen(0, 0, true)) return NULL;
	}

	out_addr.sin_family = AF_INET;
	out_addr.sin_port = htons(port);
	out_addr.sin_addr.s_addr = NetworkResolveHost(host);

	// Clear item in gamelist
	item = NetworkGameListAddItem(inet_addr(inet_ntoa(out_addr.sin_addr)), ntohs(out_addr.sin_port));
	memset(&item->info, 0, sizeof(item->info));
	ttd_strlcpy(item->info.server_name, host, lengthof(item->info.server_name));
	ttd_strlcpy(item->info.hostname, host, lengthof(item->info.hostname));
	item->online = false;

	// Init the packet
	p = NetworkSend_Init(PACKET_UDP_CLIENT_FIND_SERVER);

	_udp_client_socket->SendPacket(p, &out_addr);

	free(p);

	UpdateNetworkGameWindow(false);
	return item;
}

/* Remove our advertise from the master-server */
void NetworkUDPRemoveAdvertise(void)
{
	struct sockaddr_in out_addr;
	Packet *p;

	/* Check if we are advertising */
	if (!_networking || !_network_server || !_network_udp_server) return;

	/* check for socket */
	if (!_udp_master_socket->IsConnected()) {
		if (!_udp_master_socket->Listen(0, 0, false)) return;
	}

	DEBUG(net, 1, "[udp] removing advertise from master server");

	/* Find somewhere to send */
	out_addr.sin_family = AF_INET;
	out_addr.sin_port = htons(NETWORK_MASTER_SERVER_PORT);
	out_addr.sin_addr.s_addr = NetworkResolveHost(NETWORK_MASTER_SERVER_HOST);

	/* Send the packet */
	p = NetworkSend_Init(PACKET_UDP_SERVER_UNREGISTER);
	/* Packet is: Version, server_port */
	NetworkSend_uint8(p, NETWORK_MASTER_SERVER_VERSION);
	NetworkSend_uint16(p, _network_server_port);
	_udp_master_socket->SendPacket(p, &out_addr);

	free(p);
}

/* Register us to the master server
     This function checks if it needs to send an advertise */
void NetworkUDPAdvertise(void)
{
	struct sockaddr_in out_addr;
	Packet *p;

	/* Check if we should send an advertise */
	if (!_networking || !_network_server || !_network_udp_server || !_network_advertise)
		return;

	/* check for socket */
	if (!_udp_master_socket->IsConnected()) {
		if (!_udp_master_socket->Listen(0, 0, false)) return;
	}

	if (_network_need_advertise) {
		_network_need_advertise = false;
		_network_advertise_retries = ADVERTISE_RETRY_TIMES;
	} else {
		/* Only send once every ADVERTISE_NORMAL_INTERVAL ticks */
		if (_network_advertise_retries == 0) {
			if ((_network_last_advertise_frame + ADVERTISE_NORMAL_INTERVAL) > _frame_counter)
				return;
			_network_advertise_retries = ADVERTISE_RETRY_TIMES;
		}

		if ((_network_last_advertise_frame + ADVERTISE_RETRY_INTERVAL) > _frame_counter)
			return;
	}

	_network_advertise_retries--;
	_network_last_advertise_frame = _frame_counter;

	/* Find somewhere to send */
	out_addr.sin_family = AF_INET;
	out_addr.sin_port = htons(NETWORK_MASTER_SERVER_PORT);
	out_addr.sin_addr.s_addr = NetworkResolveHost(NETWORK_MASTER_SERVER_HOST);

	DEBUG(net, 1, "[udp] advertising to master server");

	/* Send the packet */
	p = NetworkSend_Init(PACKET_UDP_SERVER_REGISTER);
	/* Packet is: WELCOME_MESSAGE, Version, server_port */
	NetworkSend_string(p, NETWORK_MASTER_SERVER_WELCOME_MESSAGE);
	NetworkSend_uint8(p, NETWORK_MASTER_SERVER_VERSION);
	NetworkSend_uint16(p, _network_server_port);
	_udp_master_socket->SendPacket(p, &out_addr);

	free(p);
}

void NetworkUDPInitialize(void)
{
	_udp_client_socket = new ClientNetworkUDPSocketHandler();
	_udp_server_socket = new ServerNetworkUDPSocketHandler();
	_udp_master_socket = new MasterNetworkUDPSocketHandler();

	_network_udp_server = false;
	_network_udp_broadcast = 0;
}

void NetworkUDPShutdown(void)
{
	NetworkUDPCloseAll();

	delete _udp_client_socket;
	delete _udp_server_socket;
	delete _udp_master_socket;
}

#endif /* ENABLE_NETWORK */
