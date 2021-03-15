#ifndef __PrimeEngineClientNetworkManager_H__
#define __PrimeEngineClientNetworkManager_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>

// Sibling/Children includes
#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/Networking/NetworkManager.h"

namespace PE { 
namespace Components {
    struct Component;
}
}

namespace PE {
namespace Components {

struct ClientNetworkManager : public NetworkManager
{
	PE_DECLARE_CLASS(ClientNetworkManager);

	// Constructor -------------------------------------------------------------
	ClientNetworkManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself);

	virtual ~ClientNetworkManager();

	enum EClientState
	{
		ClientState_Disconnected = 0,
		ClientState_Connecting,
		ClientState_Connected,
		ClientState_Count,
		ClientState_ReceiveOnly
	};

	// accessors
	PE::NetworkContext &getNetworkContext(){return m_netContext;}

	// Methods -----------------------------------------------------------------
	virtual void initNetwork();

	const char *EClientStateToString(EClientState state);

	void debugRender(int &threadOwnershipMask, float xoffset = 0, float yoffset = 0);

	void clientConnectToUDPServer(const char *strAddr, int port);


	virtual void createNetworkConnectionContext(t_socket sock, PE::NetworkContext *pNetContext);

	void respawnPlayer(PrimitiveTypes::Int32 hitPlayerId);
	void damagePlayer(PrimitiveTypes::Int32 hitPlayerId,float damage);

	// Component ------------------------------------------------------------
	virtual void addDefaultComponents();

	// Individual events -------------------------------------------------------

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SERVER_CLIENT_CONNECTION_ACK);
	virtual void do_SERVER_CLIENT_CONNECTION_ACK(PE::Events::Event *pEvt);

	// Handle the level load here
	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SERVER_CLIENT_LOAD_LEVEL);
	virtual void do_SERVER_CLIENT_LOAD_LEVEL(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SERVER_CLIENT_RESPAWN);
	virtual void do_SERVER_CLIENT_RESPAWN(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_SERVER_CLIENT_TAKEDAMAGE);
	virtual void do_SERVER_CLIENT_TAKEDAMAGE(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
	virtual void do_UPDATE(Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE_SCORE_UI);
	virtual void do_UPDATE_SCORE_UI(PE::Events::Event *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PLAYER_JOIN);
	virtual void do_PLAYER_JOIN(PE::Events::Event *pEvt);


	// Loading -----------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// Skin Lua Interface
	//////////////////////////////////////////////////////////////////////////
	//
	static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);
	//
	static int l_clientConnectToTCPServer(lua_State *luaVM);
	//
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Member variables 
	//////////////////////////////////////////////////////////////////////////
	EClientState m_state;

	PE::NetworkContext m_netContext; // Client apps have only one context
	Threading::Mutex m_netContextLock;

	int m_clientId;
	Vector3 m_spawnPos;
	bool levelLoaded;

	static float rttLatencyTime;
};
}; // namespace Components
}; // namespace PE
#endif
