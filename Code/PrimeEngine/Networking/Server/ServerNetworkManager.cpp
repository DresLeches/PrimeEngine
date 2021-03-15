#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "ServerNetworkManager.h"

// Outer-Engine includes
#include "CharacterControl/ServerGameObjectManagerAddon.h"

// Inter-Engine includes
#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"

#include "PrimeEngine/GameObjectModel/GameObjectManager.h"
#include "PrimeEngine/Scene/DebugRenderer.h"

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Events/StandardKeyboardEvents.h"
#include "PrimeEngine/Events/StandardGhost.h"

#include "PrimeEngine/Game/Client/ClientGame.h"
#include "PrimeEngine/Game/Server/ServerGame.h"

#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/Networking/StreamManager.h"
#include "PrimeEngine/Networking/EventManager.h"
#include "PrimeEngine/Networking/NetworkManager.h"
#include "PrimeEngine/Networking/GhostManager.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"

// Sibling/Children includes
#include "ServerConnectionManager.h"

// additional lua includes needed
extern "C"
{
#include "PrimeEngine/../luasocket_dist/src/socket.h"
#include "PrimeEngine/../luasocket_dist/src/inet.h"
};

#define PE_MAX_PLAYERS 8

static int m_nextGhostId = 0;
static int m_ghostTimer = 0;
static bool latencyDebug = false;
static float totalPacketDroppedAvg = 0.0f;
static float totalNumPacketDropped = 0.0f;
static float totalTime = 0.0f;
static float prevTime = 0.0f;
float PE::Components::ServerNetworkManager::latencyTime = 0.0f;

using namespace PE::Events;

namespace PE {

namespace Components {

PE_IMPLEMENT_CLASS1(ServerNetworkManager, NetworkManager);

ServerNetworkManager::ServerNetworkManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
: NetworkManager(context, arena, hMyself)
, m_clientConnections(context, arena, PE_SERVER_MAX_CONNECTIONS)
, m_killScores(context, arena, PE_MAX_PLAYERS)
, m_currIntervalTime(0.0f)
, m_incTime(0.002f)
, m_maxIntervalTime(0.016f)
, latencyCounter(0.0f)
, maxLatency(0.0f)
{
	m_state = ServerState_Uninitialized;
}

ServerNetworkManager::~ServerNetworkManager()
{
	if (m_state != ServerState_Uninitialized)
		socket_destroy(&m_sock);
}

void ServerNetworkManager::addDefaultComponents()
{
	NetworkManager::addDefaultComponents();

	// no need to register handler as parent class already has this method registered
	// PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, ServerConnectionManager::do_UPDATE);
	PE_REGISTER_EVENT_HANDLER(Events::Event_CLIENT_SERVER_KILLED, ServerNetworkManager::do_RESPAWN_PLAYER);
	PE_REGISTER_EVENT_HANDLER(Events::Event_CLIENT_SERVER_TAKEDAMAGE, ServerNetworkManager::do_DAMAGE_PLAYER);
	PE_REGISTER_EVENT_HANDLER(Events::Event_CLIENT_SERVER_CONNECTED_ACK, ServerNetworkManager::do_SEND_SERVER_CLIENT_ACK);
	PE_REGISTER_EVENT_HANDLER(Events::Event_KEY_L_HELD, ServerNetworkManager::do_ACTIVATE_LAG_DEBUG);
	PE_REGISTER_EVENT_HANDLER(Events::Event_KEY_M_HELD, ServerNetworkManager::do_DEACTIVATE_LAG_DEBUG);
}

void ServerNetworkManager::initNetwork()
{
	NetworkManager::initNetwork();
	
	serverOpenUDPSocket();
}

void ServerNetworkManager::serverOpenUDPSocket()
{
	bool created = false;
	int numTries = 0;
	int port = PE_SERVER_PORT;
	
	while (!created)
	{
		const char *err = inet_trycreate(&m_sock, SOCK_DGRAM);
		if (err)
		{
			assert(!"error creating socket occurred");
			break;
		}
		
		err = inet_trybind(&m_sock, "0.0.0.0", (unsigned short)(port)); // leaves socket non-blocking
		numTries++;
		if (err) 
		{
			if (numTries >= 10)
			{
				break; // give up
			}
			port++;
		}
		else
		{
			created = true;
			break;
		}
	}
	// HI
	if (created)
	{
		m_serverPort = port;
	}
	else
	{
		assert(!"Could not create server");
		return;
	}

	// create a tribes stack for this connection
	// It's only purpose is to add clients to the game
	m_connectionsMutex.lock();

	createServerNetworkConnectionContext(m_sock, &m_netContext);
	m_connectionsMutex.unlock();

	m_state = ServerState_ConnectionListening;
}


void ServerNetworkManager::createServerNetworkConnectionContext(t_socket sock, PE::NetworkContext *pNetContext)
{
	NetworkManager::createNetworkConnectionContext(sock, pNetContext);

	{
		pNetContext->m_pConnectionManager = new (m_arena) ServerConnectionManager(*m_pContext, m_arena, *pNetContext, Handle());
		pNetContext->getConnectionManager()->addDefaultComponents();
	}

	{
		pNetContext->m_pStreamManager = new (m_arena) StreamManager(*m_pContext, m_arena, *pNetContext, Handle());
		pNetContext->getStreamManager()->addDefaultComponents();
	}

	{
		pNetContext->m_pEventManager = new (m_arena) EventManager(*m_pContext, m_arena, *pNetContext, Handle());
		pNetContext->getEventManager()->addDefaultComponents();
	}

	{
		pNetContext->m_pGhostManager = new (m_arena) GhostManager(*m_pContext, m_arena, *pNetContext, Handle());
		pNetContext->getGhostManager()->addDefaultComponents();
	}

	pNetContext->getConnectionManager()->initializeConnected(sock);
	pNetContext->getConnectionManager()->m_state = ConnectionManager::ConnectionManagerState_ReceiveOnly;

	addComponent(pNetContext->getConnectionManager()->getHandle());
	addComponent(pNetContext->getStreamManager()->getHandle());
	addComponent(pNetContext->getGhostManager()->getHandle());
}

void ServerNetworkManager::createNetworkConnectionContext(t_socket sock, int clientId, PE::NetworkContext *pNetContext)
{
	pNetContext->m_clientId = clientId;
	NetworkManager::createNetworkConnectionContext(sock, pNetContext);

	{
		pNetContext->m_pConnectionManager = new (m_arena) ServerConnectionManager(*m_pContext, m_arena, *pNetContext, Handle());
		pNetContext->getConnectionManager()->addDefaultComponents();
	}

	{
		pNetContext->m_pStreamManager = new (m_arena) StreamManager(*m_pContext, m_arena, *pNetContext, Handle());
		pNetContext->getStreamManager()->addDefaultComponents();
	}

	{
		pNetContext->m_pEventManager = new (m_arena) EventManager(*m_pContext, m_arena, *pNetContext, Handle());
		pNetContext->getEventManager()->addDefaultComponents();
	}

	{
		pNetContext->m_pGhostManager = new (m_arena) GhostManager(*m_pContext, m_arena, *pNetContext, Handle());
		pNetContext->getGhostManager()->addDefaultComponents();
	}

	pNetContext->getConnectionManager()->initializeConnected(sock);
	pNetContext->getConnectionManager()->dstAddr = m_netContext.getConnectionManager()->dstAddr;
	pNetContext->getConnectionManager()->dstLen = m_netContext.getConnectionManager()->dstLen;

	addComponent(pNetContext->getConnectionManager()->getHandle());
	addComponent(pNetContext->getStreamManager()->getHandle());
	addComponent(pNetContext->getGhostManager()->getHandle());
}

void ServerNetworkManager::do_UPDATE(Events::Event *pEvt)
{
	NetworkManager::do_UPDATE(pEvt);

	// Iterate through and check the ghost manager to see if need to send anything to the client
	// TODO: Add a timer to control when the packets are sent

	// Compute the average packet loss over time
	for (unsigned int i = 0; i < m_clientConnections.m_size; i++)
	{
		PE::NetworkContext& netContext = m_clientConnections[i];
		int currNumPacketDropped = netContext.getConnectionManager()->m_numPacketDrop;
		int prevNumPackeDropped = netContext.getConnectionManager()->m_prevNumPacketDrop;
		int tempNumPacketDropped = currNumPacketDropped - prevNumPackeDropped;

		if (tempNumPacketDropped != 0)
		{
			totalNumPacketDropped += tempNumPacketDropped;
			netContext.getConnectionManager()->m_prevNumPacketDrop = currNumPacketDropped;
		}
	}

	float currTime = CharacterControl::Components::ServerGameObjectManagerAddon::m_officialGameTime;
	float timeDuration = currTime - prevTime;
	if (totalNumPacketDropped != 0 && timeDuration != 0.0f)
	{
		totalTime += timeDuration;
		totalPacketDroppedAvg = totalNumPacketDropped / totalTime;
		prevTime = currTime;
	}

	latencyCounter += timeDuration;

	for (unsigned int i = 0; i < m_clientConnections.m_size; i++)
	{
		PE::NetworkContext& netContext = m_clientConnections[i];
		if (netContext.getGhostManager()->haveGhostsToSend() && latencyCounter >= maxLatency)
		{
			netContext.getStreamManager()->sendNextPackets();
			netContext.getGhostManager()->clearGhostStateMask();
			latencyCounter = 0;
		}
	}
}

void ServerNetworkManager::do_SEND_SERVER_CLIENT_ACK(Events::Event *pEvt)
{
	// Check if the incoming port number has been registered
	sockaddr_in incomingAddr = m_netContext.getConnectionManager()->dstAddr;
	for (unsigned int i = 0; i < m_connectClientAddr.size(); i++)
	{
		sockaddr_in sockAddr = m_connectClientAddr[i];
		if (sockAddr.sin_addr.S_un.S_addr == incomingAddr.sin_addr.S_un.S_addr && sockAddr.sin_port == incomingAddr.sin_port)
		{
			PEINFO("A server client connect packet was resent. Ignore it");
			return;
		}
	}

	// Create the socket for the networkcontext
	t_socket sock;
	int clientPort = 8080;
	bool created = false;
	int numTries = 0;

	while (!created)
	{
		const char *err = /*luasocket::*/inet_trycreate(&sock, SOCK_DGRAM);
		if (err)
		{
			assert(!"error creating socket occurred");
		}
		
		err = inet_trybind(&sock, "0.0.0.0", clientPort);
		numTries++;

		if (err)
		{
			if (numTries >= 10)
			{
				// Give up
				PEINFO("Too many attempts. Stop trying to connect to port number");
				return;
			}
			clientPort++;
			PEINFO("Attempting to bind to another port");
		}
		else
		{
			created = true;
			break;
		}
	}

	// Set up the tribes stack
	m_connectionsMutex.lock();
	m_clientConnections.add(PE::NetworkContext());
	int clientIndex = m_clientConnections.m_size - 1;
	PE::NetworkContext &netContext = m_clientConnections[clientIndex];

	// create a tribes stack for this connection
	createNetworkConnectionContext(sock, clientIndex, &netContext);
	m_connectClientAddr.push_back(m_netContext.getConnectionManager()->dstAddr);

	// We got the event we needed. Reset it to accept other incoming clients
	m_netContext.getEventManager()->m_receiverFirstEvtOrderId = 0;
	m_connectionsMutex.unlock();


	// register the soldier ghost
	{
		PE::Ghosts::Ghost_SOLDIERS* pGhost = new (m_arena) PE::Ghosts::Ghost_SOLDIERS(*m_pContext);
		pGhost->m_networkClientId = clientIndex;
		pGhost->m_ghostId = m_nextGhostId++;
		pGhost->m_ghostStateMask = 0x1;

		// Perform the synchronization together
		syncGhostForTargetClient(clientIndex);
		registerGhostForAll(pGhost);

		// Cache the ghost, since we will need it to synchronize everything
		m_ghostCache[pGhost->m_ghostId] = pGhost;

		// For debug: Add the ghost state mask that will be stored
		m_stateMaskDebug.push_back(0);
	}

	// Notify the other clients that a player joined the game and reset their game timers to be in sync with server
	{
		PE::Events::Event_SERVER_CLIENT_PLAYER_JOINED evt(*m_pContext);
		evt.m_newPlayerId = clientIndex;
		//scheduleEventToAllExcept(&evt, m_pContext->getGameObjectManager(), clientIndex, true);
		scheduleEventToAll(&evt, m_pContext->getGameObjectManager(), true);
	}

	// Server-client ack connection event
	{
		// Let the client know that connection was successful
		PE::Events::Event_SERVER_CLIENT_CONNECTION_ACK evt(*m_pContext);
		evt.m_clientId = clientIndex;

		// Initialize the client's score slot
		char clientId[2];
		StringOps::intToStr(evt.m_clientId, clientId, sizeof(clientId));
		m_killScores.add(clientId, 0);

		netContext.getEventManager()->scheduleEvent(&evt, m_pContext->getGameObjectManager(), true);
	}

	// Server-client load level event: Send the event to the client that entered
	{
		// Tell user to load the level and packages
		// Not the best solution for indicating which level but it will work for now
		// TODO: Figure out a better way to send server and client to load a certain level
		PE::Events::Event_SERVER_CLIENT_LOAD_LEVEL evt(*m_pContext);
		evt.m_packageId = 1;
		evt.m_levelId = 1;

		// TODO: This is how we will spawn the player for now until I get some level manager working
		if (clientIndex == 0)
		{
			evt.mSpawnPos = Vector3(2.0f, 0.0f, 5.0f);
		}
		else
		{
			evt.mSpawnPos = Vector3(2.0f, 0.0f, -5.0f);
		}
	
		netContext.getEventManager()->scheduleEvent(&evt, m_pContext->getGameObjectManager(), true);
	}
	
	// Send the events to all the clients right way
	for (PrimitiveTypes::UInt32 i = 0; i < m_clientConnections.m_size; i++)
	{
		m_clientConnections[i].getStreamManager()->sendNextPackets();
	}
}

void ServerNetworkManager::do_RESPAWN_PLAYER(Events::Event *pEvt)
{
	Event_CLIENT_SERVER_KILLED *pRealEvt = (Event_CLIENT_SERVER_KILLED*)(pEvt);

	PrimitiveTypes::UInt32 clientId = pRealEvt->m_clientId;
	PrimitiveTypes::UInt32 killedPlayerId = pRealEvt->m_killedPlayerId;
	PE::NetworkContext networkContext = m_clientConnections[killedPlayerId];

	// Send the respawn event
	{	
		PE::Events::Event_SERVER_CLIENT_RESPAWN evt(*m_pContext);
		Vector3 spawnPos;

		// Hardcoded the respawn location for now for this demo
		if (killedPlayerId == 0) 
		{
			spawnPos = Vector3(2.0f, 0.0f, 5.0f);
		}
		else 
		{
			spawnPos = Vector3(2.0f, 0.0f, -5.0f);
		}
		evt.m_position = spawnPos;
		evt.m_clientId = killedPlayerId;
		
		//networkContext.getEventManager()->scheduleEvent(&evt, m_pContext->getGameObjectManager(), true);
		scheduleEventToAll(&evt, m_pContext->getGameObjectManager(), true);
	}

	// Update the score to everyone
	{
		char killedPlayerIdStr[2];
		StringOps::intToStr(clientId, killedPlayerIdStr, sizeof(killedPlayerIdStr));
		PrimitiveTypes::Int32 ind = m_killScores.findIndex(killedPlayerIdStr);
		m_killScores.m_pairs[ind].m_value++;

		PE::Events::Event_SERVER_CLIENT_SEND_KILL_SCORE evt(*m_pContext);
		evt.m_clientId = clientId;
		evt.m_killUpdate = m_killScores.m_pairs[ind].m_value;

		scheduleEventToAll(&evt, m_pContext->getGameObjectManager(), true);
	}
}

void ServerNetworkManager::do_DAMAGE_PLAYER(Events::Event *pEvt) {

	Event_CLIENT_SERVER_TAKEDAMAGE *pRealEvt = (Event_CLIENT_SERVER_TAKEDAMAGE*)(pEvt);

	PrimitiveTypes::UInt32 damage = pRealEvt->m_damage;
	PrimitiveTypes::UInt32 hitPlayerId = pRealEvt->m_hitPlayerId;
	PE::NetworkContext networkContext = m_clientConnections[hitPlayerId];

	//	PE::Ghosts::Ghost_SOLDIERS* playerGhost = (PE::Ghosts::Ghost_SOLDIERS*) pNM->m_ghostCache[pRealEvt->m_networkClientId];
	PE::Ghosts::Ghost_SOLDIERS* playerGhost = (PE::Ghosts::Ghost_SOLDIERS*) m_ghostCache[hitPlayerId];
	playerGhost->setDamageStateUpdated();

	for (PrimitiveTypes::UInt32 i = 0; i < m_clientConnections.m_size; i++)
	{
		/*
		if (playerGhost->m_ghostId == i)
		{
			continue;
		}
		*/

		updateGhostMaskState(playerGhost->m_ghostId, playerGhost->m_ghostStateMask, i);
	}
	m_stateMaskDebug[playerGhost->m_ghostId] = playerGhost->m_ghostStateMask;
	playerGhost->m_ghostStateMask &= 0x0;

	/*
	for (int i = 0; i < pNM->m_clientConnections.m_size; i++)
	{
		if (playerGhost->m_ghostId == i)
		{
			continue;
		}

		pNM->updateGhostMaskState(playerGhost->m_ghostId, playerGhost->m_ghostStateMask, i);
	}
	pNM->m_stateMaskDebug[playerGhost->m_ghostId] = playerGhost->m_ghostStateMask;
	playerGhost->m_ghostStateMask &= 0x0;
	*/


	PE::Events::Event_SERVER_CLIENT_TAKEDAMAGE evt(*m_pContext);

	evt.m_damage = damage;
	evt.m_clientId = hitPlayerId;
	networkContext.getEventManager()->scheduleEvent(&evt, m_pContext->getGameObjectManager(), true);
}

void ServerNetworkManager::do_ACTIVATE_LAG_DEBUG(Events::Event *pEvt)
{
	//latencyDebug = true;
	//PEINFO("Latency activated");
	maxLatency++;
}

void ServerNetworkManager::do_DEACTIVATE_LAG_DEBUG(Events::Event *pEvt)
{
//	latencyDebug = false;
	//PEINFO("Latency deactivated");
	maxLatency = 0;
}

void ServerNetworkManager::debugRender(int &threadOwnershipMask, float xoffset /* = 0*/, float yoffset /* = 0*/)
{
	sprintf(PEString::s_buf, "Server: Port %d %d Connections", m_serverPort, m_clientConnections.m_size);
	DebugRenderer::Instance()->createTextMesh(
		PEString::s_buf, true, false, false, false, 0,
		Vector3(xoffset, yoffset, 0), 1.0f, threadOwnershipMask);

	float dy = 0.025f;
	float dx = 0.01f;
	float evtManagerDy = 0.15f;
	float ghostStateOffsetX = 0.05f;
	float ghostStateOffsetY = 0.65f;
	float ghostStateDy = 0.05f;
	float packetLossOffsetX = 0.05f;
	float packetLossOffsetY = 0.60f;
	float packetLossDx = 0.05f;
	float packetLossDy = 0.25f;
	float latencyOffsetX = 0.05f;
	float latencyOffsetY = 0.55f;

	// debug render all networking contexts
	m_connectionsMutex.lock();
	for (unsigned int i = 0; i < m_clientConnections.m_size; ++i)
	{
		sprintf(PEString::s_buf, "Connection[%d]:\n", i);
	
		DebugRenderer::Instance()->createTextMesh(
		PEString::s_buf, true, false, false, false, 0,
		Vector3(xoffset, yoffset + dy + evtManagerDy * i, 0), 1.0f, threadOwnershipMask);

		PE::NetworkContext &netContext = m_clientConnections[i];
		netContext.getEventManager()->debugRender(threadOwnershipMask, xoffset + dx, yoffset + dy * 2.0f + evtManagerDy * i);
	}
	
	char tempPackBuff[1024];
	//sprintf(tempPackBuff, "Packet loss: %d\n", m_netContext.getConnectionManager()->m_numPacketDrop);
	sprintf(tempPackBuff, "Packet loss: %.1f", totalPacketDroppedAvg);
	DebugRenderer::Instance()->createTextMesh(
		tempPackBuff, true, false, false, false, 0,
		Vector3(packetLossOffsetX, packetLossOffsetY, 0), 1.0f, threadOwnershipMask);

	for (unsigned int i = 0; i < m_stateMaskDebug.size(); ++i)
	{
		// Figure out the ghost state
		char ghostState = ' ';
		int state = m_stateMaskDebug[i];
		if (state >= 10)
		{
			int diff = m_stateMaskDebug[i] - 10;
			ghostState = 'A' + diff;
			sprintf(PEString::s_buf, "Ghost State %d: 0x%c", i, ghostState);
		}
		else
		{
			ghostState = '0' + m_stateMaskDebug[i];
			sprintf(PEString::s_buf, "Ghost State %d: 0x%c", i, ghostState);
		}

		DebugRenderer::Instance()->createTextMesh(
			PEString::s_buf, true, false, false, false, 0,
			Vector3(ghostStateOffsetX, ghostStateOffsetY + ghostStateDy * i, 0.0f), 1.0f, threadOwnershipMask);
	}

	/*
	// Add the debug render for the network latency
	sprintf(PEString::s_buf, "Network latency: %.1f ms", latencyTime);
	DebugRenderer::Instance()->createTextMesh(
		PEString::s_buf, true, false, false, false, 0,
		Vector3(latencyOffsetX, latencyOffsetY, 0.0f), 1.0f, threadOwnershipMask);
		*/

	m_connectionsMutex.unlock();


}

void ServerNetworkManager::scheduleEventToAllExcept(PE::Networkable *pNetworkable, PE::Networkable *pNetworkableTarget, int exceptClient, bool guaranteed)
{
	for (unsigned int i = 0; i < m_clientConnections.m_size; ++i)
	{
		if ((int)(i) == exceptClient)
			continue;

		PE::NetworkContext &netContext = m_clientConnections[i];
		netContext.getEventManager()->scheduleEvent(pNetworkable, pNetworkableTarget, guaranteed);
	}
}

void ServerNetworkManager::scheduleEventToAll(PE::Networkable *pNetworkable, PE::Networkable *pNetworkableTarget, bool guaranteed)
{
	for (unsigned int i = 0; i < m_clientConnections.m_size; ++i)
	{
		PE::NetworkContext &netContext = m_clientConnections[i];
		netContext.getEventManager()->scheduleEvent(pNetworkable, pNetworkableTarget, guaranteed);
	}
}

void ServerNetworkManager::syncGhostForTargetClient(int targetClient)
{
	PE::NetworkContext &netContext = m_clientConnections[targetClient];
	
	for (auto& it : m_ghostCache)
	{
		PE::Ghosts::Ghost_SOLDIERS* pGhost = new (m_arena) PE::Ghosts::Ghost_SOLDIERS(*m_pContext);
		pGhost->m_networkClientId = it.second->m_networkClientId;
		pGhost->m_ghostId = it.second->m_ghostId;
		pGhost->m_ghostStateMask = it.second->m_ghostStateMask | 0x1;
		netContext.getGhostManager()->registerGhost(pGhost);
	}

}

void ServerNetworkManager::registerGhostForAll(PE::Ghosts::Ghost* pGhost)
{
	for (unsigned int i = 0; i < m_clientConnections.m_size; ++i)
	{
		// Copy the data over to a new ghost
		PE::Ghosts::Ghost_SOLDIERS* ghost = new (m_arena) PE::Ghosts::Ghost_SOLDIERS(*m_pContext);
		ghost->m_networkClientId = pGhost->m_networkClientId;
		ghost->m_ghostId = pGhost->m_ghostId;
		ghost->m_ghostStateMask = pGhost->m_ghostStateMask | 0x1;
		
		PE::NetworkContext &netContext = m_clientConnections[i];
		netContext.getGhostManager()->registerGhost(ghost);
	}
}

void ServerNetworkManager::updateGhostMaskState(int ghostId, PrimitiveTypes::Int32 ghostMask, int clientId)
{

	PE::NetworkContext& context = m_clientConnections[clientId];
	context.getGhostManager()->m_ghostList[ghostId]->m_ghostStateMask |= ghostMask;
}


#if 0 // template
//////////////////////////////////////////////////////////////////////////
// ConnectionManager Lua Interface
//////////////////////////////////////////////////////////////////////////
//
void ConnectionManager::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
{
	
	//static const struct luaL_Reg l_functions[] = {
	//	{"l_clientConnectToTCPServer", l_clientConnectToTCPServer},
	//	{NULL, NULL} // sentinel
	//};

	//luaL_register(luaVM, 0, l_functions);
	
	lua_register(luaVM, "l_clientConnectToTCPServer", l_clientConnectToTCPServer);


	// run a script to add additional functionality to Lua side of Skin
	// that is accessible from Lua
// #if APIABSTRACTION_IOS
// 	LuaEnvironment::Instance()->runScriptWorkspacePath("Code/PrimeEngine/Scene/Skin.lua");
// #else
// 	LuaEnvironment::Instance()->runScriptWorkspacePath("Code\\PrimeEngine\\Scene\\Skin.lua");
// #endif

}

int ConnectionManager::l_clientConnectToTCPServer(lua_State *luaVM)
{
	lua_Number lPort = lua_tonumber(luaVM, -1);
	int port = (int)(lPort);

	const char *strAddr = lua_tostring(luaVM, -2);

	GameContext *pContext = (GameContext *)(lua_touserdata(luaVM, -3));

	lua_pop(luaVM, 3);

	pContext->getConnectionManager()->clientConnectToTCPServer(strAddr, port);

	return 0; // no return values
}

//////////////////////////////////////////////////////////////////////////
#endif
	
}; // namespace Components
}; // namespace PE
