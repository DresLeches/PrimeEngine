#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "ClientNetworkManager.h"

// Outer-Engine includes


// Inter-Engine includes
#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"

#include "PrimeEngine/Scene/DebugRenderer.h"

#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Events/Component.h"

#include "PrimeEngine/Networking/Client/ClientConnectionManager.h"
#include "PrimeEngine/Networking/StreamManager.h"
#include "PrimeEngine/Networking/EventManager.h"
#include "PrimeEngine/Networking/GhostManager.h"

#include "PrimeEngine/GameObjectModel/GameObjectManager.h"
#include "PrimeEngine/GameObjectModel/DefaultGameControls/DefaultGameControls.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"

// TODO: Where is that pesky Matrix4x4?
#include "CharacterControl/ClientPlayer.h"
#include "CharacterControl/CharacterControlContext.h"

// additional lua includes needed
extern "C"
{
#include "PrimeEngine/../luasocket_dist/src/socket.h"
#include "PrimeEngine/../luasocket_dist/src/inet.h"
};

// Sibling/Children includes
using namespace PE::Events;
bool showKillFeed;
int killerPlayerID;
int deadPlayerID;
int killFeedTimer;
float PE::Components::ClientNetworkManager::rttLatencyTime = 0.0f;

namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(ClientNetworkManager, NetworkManager);

ClientNetworkManager::ClientNetworkManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
: NetworkManager(context, arena, hMyself)
, m_clientId(-1)
, levelLoaded(false)
{
	m_state = ClientState_Disconnected;
	showKillFeed = false;
	killerPlayerID = -1;
	deadPlayerID = -1;
	killFeedTimer = 0;
}

ClientNetworkManager::~ClientNetworkManager()
{

}

void ClientNetworkManager::initNetwork()
{
	NetworkManager::initNetwork();
}

void ClientNetworkManager::clientConnectToUDPServer(const char *strAddr, int port)
{
	bool created = false;
	int numTries = 0;

	if (port == 0)
		port = PE_SERVER_PORT;

	t_socket sock;
	int clientPort = 1500;
	while (!created)
	{
		const char *err = /*luasocket::*/inet_trycreate(&sock, SOCK_DGRAM);

		if (err)
		{
			assert(!"error creating socket occurred");
			break;
		}

		t_timeout timeout; // timeout supports managind timeouts of multiple blocking alls by using total.
		// but if total is < 0 it just uses block value for each blocking call
		timeout.block = PE_CLIENT_TO_SERVER_CONNECT_TIMEOUT;
		timeout.total = -1.0;
		timeout.start = 0;

		//err = inet_tryconnect(&sock, strAddr, port, &timeout);
		
		err = inet_trybind(&sock, strAddr, clientPort);

		numTries++;
		if (err) {
			PEINFO("PE: Warning: Client Failed to connect to %s:%d reason: %s\n", strAddr, port, err);
			
			if (numTries >= 10)
			{
				break; // give up
			}
			clientPort++;
		}
		else
		{
			PEINFO("PE: Client connected to %s:%d\n", strAddr, port);
			created = true;
			m_netContextLock.lock();
			m_state = ClientState_Connected;
			
			createNetworkConnectionContext(sock, &m_netContext);
			m_netContextLock.unlock();

			// Initialize the connection manager's dstAddr to tell it to connect to server
			sockaddr_in& dstAddr = m_netContext.getConnectionManager()->dstAddr;
			dstAddr.sin_family = AF_INET;
			dstAddr.sin_port = htons(PE_SERVER_PORT);
			pesocket_inet_aton(strAddr, &dstAddr.sin_addr);
			//pesocket_inet_aton("192.168.2.40", &dstAddr.sin_addr);
			
			
			// Send an ack packet to the server that client connected
			Event_CLIENT_SERVER_CONNECTED_ACK evt(*m_pContext);
			evt.m_connected = true;
			m_netContext.getEventManager()->scheduleEvent(&evt, m_pContext->getGameObjectManager(), true);
			m_netContext.getStreamManager()->sendNextPackets(); // Send the packet right away
			break;
		}
	}
}

void ClientNetworkManager::createNetworkConnectionContext(t_socket sock, PE::NetworkContext *pNetContext)
{
	NetworkManager::createNetworkConnectionContext(sock, pNetContext);

	{
		pNetContext->m_pConnectionManager = new (m_arena) ClientConnectionManager(*m_pContext, m_arena, *pNetContext, Handle());
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

	addComponent(pNetContext->getConnectionManager()->getHandle());
	addComponent(pNetContext->getStreamManager()->getHandle());
	addComponent(pNetContext->getGhostManager()->getHandle());
}

void ClientNetworkManager::respawnPlayer(PrimitiveTypes::Int32 hitPlayerId)
{
	PE::Events::Event_CLIENT_SERVER_KILLED evt(*m_pContext);
	evt.m_clientId = m_clientId;
	evt.m_killedPlayerId = hitPlayerId;
	//deadPlayerID = hitPlayerId;
	//killerPlayerID = m_clientId;
	m_netContext.getEventManager()->scheduleEvent(&evt, m_pContext->getGameObjectManager(), true);
}

void ClientNetworkManager::damagePlayer(PrimitiveTypes::Int32 hitPlayerId, float damage)
{
	PE::Events::Event_CLIENT_SERVER_TAKEDAMAGE evt(*m_pContext);
	evt.m_damage = damage;
	evt.m_hitPlayerId = hitPlayerId;
	m_netContext.getEventManager()->scheduleEvent(&evt, m_pContext->getGameObjectManager(), true);
}

void ClientNetworkManager::addDefaultComponents()
{
	NetworkManager::addDefaultComponents();

	// no need to register handler as parent class already has this method registered
	// PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, ServerConnectionManager::do_UPDATE);

	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_CONNECTION_ACK, ClientNetworkManager::do_SERVER_CLIENT_CONNECTION_ACK);
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_LOAD_LEVEL, ClientNetworkManager::do_SERVER_CLIENT_LOAD_LEVEL);
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_RESPAWN, ClientNetworkManager::do_SERVER_CLIENT_RESPAWN);
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_TAKEDAMAGE, ClientNetworkManager::do_SERVER_CLIENT_TAKEDAMAGE);
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_SEND_KILL_SCORE, ClientNetworkManager::do_UPDATE_SCORE_UI);
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_PLAYER_JOINED, ClientNetworkManager::do_PLAYER_JOIN)
}

void ClientNetworkManager::do_UPDATE(PE::Events::Event *pEvt)
{
	NetworkManager::do_UPDATE(pEvt);
	if (m_netContext.getConnectionManager())
	{
		if (m_state == ClientState_Connected)
		{
			// Check if we're still connected
			if (!m_netContext.getConnectionManager()->connected())
			{
				// disconnect happened
				// TODO: Create the event letting server know that the player disconnected
				m_state = ClientState_Disconnected;
				m_clientId = -1;
			}

			// Check if we can actually load level
			if (levelLoaded && m_netContext.getEventManager()->m_leftoverEvents == 0)
			{
				levelLoaded = false; // We don't need to load the level anymore
				RootSceneNode::Instance()->m_playerSNs[m_clientId]->m_base.setPos(m_spawnPos);
			}

			// It's all good. Let's send the next packet to the server
			m_netContext.getStreamManager()->sendNextPackets();
		}
	}

	
}

void ClientNetworkManager::do_SERVER_CLIENT_CONNECTION_ACK(PE::Events::Event *pEvt)
{
	Event_SERVER_CLIENT_CONNECTION_ACK *pRealEvt = (Event_SERVER_CLIENT_CONNECTION_ACK *)(pEvt);
	m_clientId = pRealEvt->m_clientId;
	m_state = ClientState_Connected;

}

// Do the level load here
void ClientNetworkManager::do_SERVER_CLIENT_LOAD_LEVEL(PE::Events::Event *pEvt)
{
	Event_SERVER_CLIENT_LOAD_LEVEL *pRealEvt = (Event_SERVER_CLIENT_LOAD_LEVEL*)(pEvt);
	GhostManager* ghostManager = m_netContext.getGhostManager();

	// TODO: Figure out a better way to do this in the next milestone
	if (pRealEvt->m_packageId == 1 && pRealEvt->m_levelId == 1)
	{
		m_pContext->getLuaEnvironment()->runString("LevelLoader.loadLevel('mainlevel.x_level.levela', 'CharacterControl')");
	}
	
	m_spawnPos = pRealEvt->mSpawnPos;
	levelLoaded = true;
	RootSceneNode::Instance()->m_playerSNs[m_clientId]->m_base.setPos(m_spawnPos);
}

void ClientNetworkManager::do_SERVER_CLIENT_RESPAWN(PE::Events::Event *pEvt)
{
	Event_SERVER_CLIENT_RESPAWN *pRealEvt = (Event_SERVER_CLIENT_RESPAWN*)(pEvt);
	showKillFeed = true;
	if (pRealEvt->m_clientId == 0) {
		killerPlayerID = 1;
		deadPlayerID = 0;
	}
	else {
		killerPlayerID = 0;
		deadPlayerID = 1;
	}
	//assert(pRealEvt->m_clientId == m_clientId);
	if (pRealEvt->m_clientId == m_clientId) {
		RootSceneNode::Instance()->m_playerSNs[m_clientId]->m_base.setPos(pRealEvt->m_position);
		RootSceneNode::Instance()->m_playerSNs[m_clientId]->getFirstParentByTypePtr<CharacterControl::Components::PlayerController>()->m_health = 100;
	}
	

}

void ClientNetworkManager::do_SERVER_CLIENT_TAKEDAMAGE(PE::Events::Event *pEvt)
{
	Event_SERVER_CLIENT_TAKEDAMAGE *pRealEvt = (Event_SERVER_CLIENT_TAKEDAMAGE*)(pEvt);

	assert(pRealEvt->m_clientId == m_clientId);
	RootSceneNode::Instance()->m_playerSNs[m_clientId]->getFirstParentByTypePtr<CharacterControl::Components::PlayerController>()->m_health -= pRealEvt->m_damage;
	RootSceneNode::Instance()->m_playerSNs[m_clientId]->getFirstParentByTypePtr<CharacterControl::Components::PlayerController>()->do_Player_Hurt(pEvt);

}

void ClientNetworkManager::do_UPDATE_SCORE_UI(PE::Events::Event *pEvt)
{
	Event_SERVER_CLIENT_SEND_KILL_SCORE *pRealEvt = (Event_SERVER_CLIENT_SEND_KILL_SCORE*)(pEvt);

	PEINFO("Scored update!");
	PEINFO("Client ID: %d", pRealEvt->m_clientId);
	PEINFO("Score: %d", pRealEvt->m_killUpdate);
}

void ClientNetworkManager::do_PLAYER_JOIN(PE::Events::Event *pEvt)
{
	Event_SERVER_CLIENT_PLAYER_JOINED *pRealEvt = (Event_SERVER_CLIENT_PLAYER_JOINED*)(pEvt);
	CharacterControl::Components::PlayerController::m_gameStarted = true;
	CharacterControl::Components::PlayerController::m_officialGameTime = 0.0f;

	
	// Do any kind of UI with player joined
}

const char *ClientNetworkManager::EClientStateToString(EClientState state)
{
	switch(state)
	{
	case ClientState_Disconnected : return "ClientState_Disconnected";
	case ClientState_Connecting : return "ClientState_Connecting";
	case ClientState_Connected : return "ClientState_Connected";
	case ClientState_ReceiveOnly: return "ClientState_ReceiveOnly";
	default: return "Unknown State";
	};
}

void ClientNetworkManager::debugRender(int &threadOwnershipMask, float xoffset /* = 0*/, float yoffset /* = 0*/)
{
	// Client ID
	sprintf(PEString::s_buf, "Client: %s Id: %d", EClientStateToString(m_state), m_clientId);
	DebugRenderer::Instance()->createTextMesh(
		PEString::s_buf, true, false, false, false, 0,
		Vector3(xoffset, yoffset, 0), 1.0f, threadOwnershipMask);
	

	if (m_clientId == -1)
		return;


	//Render Health and score to the screen
	char bufHealth[80]; char bufScore[80];

    // TODO HACK FIXME: Why is playerHealth an int?
	int playerHealth = RootSceneNode::Instance()->m_playerSNs[m_clientId]->getFirstParentByTypePtr<CharacterControl::Components::PlayerController>()->m_health;
	int playerScore = RootSceneNode::Instance()->m_playerSNs[m_clientId]->getFirstParentByTypePtr<CharacterControl::Components::PlayerController>()->m_score;
	sprintf(bufHealth, "Health: %d", playerHealth);
	sprintf(bufScore, "Score: %d", playerScore);
	DebugRenderer::Instance()->createTextMesh(
		bufHealth, true, false, false, false, 0,
		Vector3(0.05f, 0.85f, 0), 1.0f, threadOwnershipMask);
	DebugRenderer::Instance()->createTextMesh(
		bufScore, true, false, false, false, 0,
		Vector3(0.05f, 0.9f, 0), 1.0f, threadOwnershipMask);

	if (showKillFeed) {
		char bufKillFeed[80];
		/*if (m_clientId == killerPlayerID) {
			sprintf(bufKillFeed, "You killed Player%d!", deadPlayerID+1);
		}
		else if (m_clientId == deadPlayerID) {
			sprintf(bufKillFeed, "Player%d killed you!", killerPlayerID+1);
		}*/
		sprintf(bufKillFeed, "Player%d killed Player%d!", killerPlayerID+1,deadPlayerID+1);
		DebugRenderer::Instance()->createTextMesh(
			bufKillFeed, true, false, false, false, 0,
			Vector3(0.4f, 0.7f, 0), 1.0f, threadOwnershipMask);

		killFeedTimer++;
		if (killFeedTimer > 150) {
			showKillFeed = false;
			killFeedTimer = 0;
		}
	}

	float dy = 0.025f;
	float dx = 0.01f;
	float dy1 = 0.30f;
	float dx1 = 0.03f;
	float latencyOffsetX = 0.05f;
	float latencyOffsetY = 0.55f;

	m_netContextLock.lock();
	
	m_netContext.getEventManager()->debugRender(threadOwnershipMask, xoffset + dx, yoffset + dy);

	// Add the debug render for the network latency
	sprintf(PEString::s_buf, "RTT Latency: %.1f ms", rttLatencyTime);
	DebugRenderer::Instance()->createTextMesh(
		PEString::s_buf, true, false, false, false, 0,
		Vector3(latencyOffsetX, latencyOffsetY, 0.0f), 1.0f, threadOwnershipMask);
	//m_netContext.getGhostManager()->debugRender(threadOwnershipMask, xoffset + dx1, yoffset + dy1);
	
	m_netContextLock.unlock();
}

//////////////////////////////////////////////////////////////////////////
// ConnectionManager Lua Interface
//////////////////////////////////////////////////////////////////////////
//
void ClientNetworkManager::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
{
	/*
	static const struct luaL_Reg l_functions[] = {
	{"l_clientConnectToTCPServer", l_clientConnectToTCPServer},
	{NULL, NULL} // sentinel
	};
	luaL_register(luaVM, 0, l_functions);

	VS

	lua_register(luaVM, "l_clientConnectToTCPServer", l_clientConnectToTCPServer);

	Note: registering with luaL_register is preferred since it is putting functions into
	table associated with this class
	using lua_register puts functions into global space
	*/
	static const struct luaL_Reg l_functions[] = {
		{"l_clientConnectToTCPServer", l_clientConnectToTCPServer},
		{NULL, NULL} // sentinel
	};

	luaL_register(luaVM, 0, l_functions);

	// run a script to add additional functionality to Lua side of the class
	// that is accessible from Lua
// #if APIABSTRACTION_IOS
// 	LuaEnvironment::Instance()->runScriptWorkspacePath("Code/PrimeEngine/Scene/Skin.lua");
// #else
// 	LuaEnvironment::Instance()->runScriptWorkspacePath("Code\\PrimeEngine\\Scene\\Skin.lua");
// #endif

}

int ClientNetworkManager::l_clientConnectToTCPServer(lua_State *luaVM)
{
	lua_Number lPort = lua_tonumber(luaVM, -1);
	int port = (int)(lPort);

	const char *strAddr = lua_tostring(luaVM, -2);

	GameContext *pContext = (GameContext *)(lua_touserdata(luaVM, -3));

	lua_pop(luaVM, 3);

	ClientNetworkManager *pClientNetwManager = (ClientNetworkManager *)(pContext->getNetworkManager());
	pClientNetwManager->clientConnectToUDPServer(strAddr, port);

	return 0; // no return values
}

}; // namespace Components
}; // namespace PE
