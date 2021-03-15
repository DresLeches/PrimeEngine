#ifndef __PrimeEngineServerNetworkManager_H__
#define __PrimeEngineServerNetworkManager_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>
#include <unordered_map>

// Inter-Engine includes
#include "PrimeEngine/Utils/PEMap.h"

// Sibling/Children includes
#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/Networking/NetworkManager.h"

// forward declaration
namespace PE 
{

// namespace Components
namespace Components 
{ 
    struct Component; 
}

// namespace Ghosts
namespace Ghosts
{
    struct Ghost;
}

}

namespace PE {
namespace Components {


struct ServerNetworkManager : public NetworkManager
{
	PE_DECLARE_CLASS(ServerNetworkManager);

	// Constructor -------------------------------------------------------------
	ServerNetworkManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself);

	virtual ~ServerNetworkManager();

	enum EServerState
	{
		ServerState_Uninitialized,
		ServerState_ConnectionListening,
		ServerState_Count
	};

	enum EGhostMaskState
	{
		Position
	};

	// Methods -----------------------------------------------------------------
	virtual void initNetwork();

	void serverOpenUDPSocket();

	virtual void createNetworkConnectionContext(t_socket sock, int clientId, PE::NetworkContext *pNetContext);
	
	virtual void createServerNetworkConnectionContext(t_socket sock, PE::NetworkContext *pNetContext);

	void debugRender(int &threadOwnershipMask, float xoffset = 0, float yoffset = 0);

	// forward to event manager
	void scheduleEventToAllExcept(PE::Networkable *pNetworkable, PE::Networkable *pNetworkableTarget, int exceptClient, bool guaranteed);

	// Forward to everyone
	void scheduleEventToAll(PE::Networkable *pNetworkable, PE::Networkable *pNetworkableTarget, bool guaranteed);

	// Sync the ghost of the current client
	void syncGhostForTargetClient(int targetClient);

	// Register the ghost for all the clients
	void registerGhostForAll(PE::Ghosts::Ghost* pGhost);

	// Update the bit mask
	void updateGhostMaskState(int ghostId, PrimitiveTypes::Int32 ghostMask, int clientId);

	// Component ------------------------------------------------------------
	virtual void addDefaultComponents();

	// Individual events -------------------------------------------------------
	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
	virtual void do_UPDATE(Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_RESPAWN_PLAYER);
	virtual void do_RESPAWN_PLAYER(Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_DAMAGE_PLAYER);
	virtual void do_DAMAGE_PLAYER(Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SEND_SERVER_CLIENT_ACK);
	virtual void do_SEND_SERVER_CLIENT_ACK(Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_ACTIVATE_LAG_DEBUG);
	virtual void do_ACTIVATE_LAG_DEBUG(Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_DEACTIVATE_LAG_DEBUG);
	virtual void do_DEACTIVATE_LAG_DEBUG(Events::Event *pEvt);

	// Loading -----------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// Skin Lua Interface
	//////////////////////////////////////////////////////////////////////////
	//
	//static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);
	//
	//static int l_clientConnectToTCPServer(lua_State *luaVM);
	//
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Member variables 
	//////////////////////////////////////////////////////////////////////////
	unsigned short m_serverPort; // for server only

	/*luasocket::*/t_socket m_sock;
	EServerState m_state;

	PE::NetworkContext m_netContext;
	Array<PE::NetworkContext> m_clientConnections;
	PEMap<PrimitiveTypes::UInt32> m_killScores;
	std::unordered_map<int, PE::Ghosts::Ghost*> m_ghostCache;
	std::vector<sockaddr_in> m_connectClientAddr;
	PrimitiveTypes::Float32 m_currIntervalTime;
	const PrimitiveTypes::Float32 m_incTime;
	const PrimitiveTypes::Float32 m_maxIntervalTime;
	std::vector<PrimitiveTypes::UInt32> m_stateMaskDebug;
	
	//std::vector<PE::Ghosts::Ghost*> m_ghostCache; // Used to synchronize with new clients
	Threading::Mutex m_connectionsMutex;

	// Time to hack
	static float latencyTime;
	float maxLatency;
	float latencyCounter;
};
}; // namespace Components
}; // namespace PE
#endif
