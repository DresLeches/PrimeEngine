#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "ConnectionManager.h"

// Outer-Engine includes

// Inter-Engine includes

#include "../Lua/LuaEnvironment.h"

// additional lua includes needed
extern "C"
{
#include "../../luasocket_dist/src/socket.h"
#include "../../luasocket_dist/src/inet.h"
};

#include "../../../GlobalConfig/GlobalConfig.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Events/StandardKeyboardEvents.h"

// Sibling/Children includes
#include "StreamManager.h"

// Network debug macros
#define PACKET_DROP 1

#if PACKET_DROP

static bool activatePacketDrop = false;

/*
 * PACKET_DROP_SEVERITY_LEVEL_1: You're getting good connection. Your area has good internet
 * PACKET_DROP_SEVERITY_LEVEL_2: More or less, what you should expect on average
 * PACKET_DROP_SEVERITY_LEVEL_3: Wow. Your internet is terrible. Trouble receiving anything.
 */
const bool PACKET_DROP_SEVERITY_LEVEL_1 = false;
const bool PACKET_DROP_SEVERITY_LEVEL_2 = false;
const bool PACKET_DROP_SEVERITY_LEVEL_3 = true;

#endif


#if PACKET_DROP
const static int LUCKY_NUMBER_1 = 4;
const static int LUCKY_NUMBER_2 = 7;
const static int LUCKY_NUMBER_3 = 8;
const static int UPPER_BOUND = 50;
const static int LOWER_BOUND = 0;
#endif

using namespace PE::Events;

namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(ConnectionManager, Component);

ConnectionManager::ConnectionManager(PE::GameContext &context, PE::MemoryArena arena, PE::NetworkContext &netContext, Handle hMyself)
: Component(context, arena, hMyself)
, m_state(ConnectionManagerState_Disconnected)
, m_bytesNeededForNextPacket(0)
, m_bytesBuffered(0)
, m_numPacketDrop(0)
, m_prevNumPacketDrop(0)
{
	m_pNetContext = &netContext;
	memset(&dstAddr, 0, sizeof(dstAddr));
	dstLen = sizeof(dstAddr);
}

ConnectionManager::~ConnectionManager()
{

}

void ConnectionManager::initializeConnected(t_socket sock)
{
	m_sock = sock;
	m_state = ConnectionManagerState_Connected;

	// if socket is blocking we might stall on reads
	socket_setnonblocking(&m_sock);
}

void ConnectionManager::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, ConnectionManager::do_UPDATE);
	PE_REGISTER_EVENT_HANDLER(Events::Event_KEY_P_HELD, ConnectionManager::do_ACTIVATE_PACKET_LOSS);
	PE_REGISTER_EVENT_HANDLER(Events::Event_KEY_O_HELD, ConnectionManager::do_DEACTIVATE_PACKET_LOSS);
}

void ConnectionManager::disconnect()
{
	m_state = ConnectionManagerState_Disconnected;
	socket_destroy(&m_sock);
}

void ConnectionManager::sendPacket(Packet *pPacket, TransmissionRecord *pTransmissionRecord)
{
	if (m_state != ConnectionManagerState_Connected)
	{
		// cant send since not connected
		return;
	}

	t_timeout timeout; // timeout supports managing timeouts of multiple blocking alls by using total.
	// but if total is < 0 it just uses block value for each blocking call
	timeout.block = PE_SOCKET_SEND_TIMEOUT;
	timeout.total = -1.0;
	timeout.start = 0;

	p_timeout tm = &timeout;
	size_t total = 0;
	PrimitiveTypes::Int32 packetSize;
	StreamManager::ReadInt32(&pPacket->m_data[0] /*= &pPacket->m_packetDataSizeInInet*/, packetSize);
	size_t count = packetSize;

	int err = IO_DONE;

	while (total < count && err == IO_DONE) {
		size_t done;
		size_t step = (count-total <= PE_SOCKET_SEND_STEPSIZE)? count-total: PE_SOCKET_SEND_STEPSIZE;

		err = socket_sendto(&m_sock, &pPacket->m_data[total], step, &done, (sockaddr*) &dstAddr, dstLen, tm);
		total += done;
	}

	if (err != IO_DONE)
	{
		if (err == IO_CLOSED)
			PEINFO("PE: Warning: Socket disconnected.\n", socket_strerror(err));
		else
			PEINFO("PE: Warning: Socket error on send: %s. Will disconnect.\n", socket_strerror(err));
		disconnect();
	
		return;
	}

	if (pTransmissionRecord == nullptr)
		return;
}

void ConnectionManager::receivePackets()
{
	if (m_state != ConnectionManagerState_Connected && m_state != ConnectionManagerState_ReceiveOnly)
	{
		// cant send since not connected

		return;
	}

	t_timeout timeout; // timeout supports managing timeouts of multiple blocking calls by using total.
	// but if total is < 0 it just uses block value for each blocking call
	//timeout.block = PE_SOCKET_RECEIVE_TIMEOUT; // if is 0, then is not blocking
	timeout.block = PE_SOCKET_RECEIVE_TIMEOUT;
	timeout.total = -1.0;
	timeout.start = 0;

    while (true)
    {
        p_timeout tm = &timeout;
        int err = IO_DONE;
        size_t got;

		sockaddr_in receivedFrom;
		memset(&receivedFrom, 0, sizeof(receivedFrom));
		socklen_t receivedFromLen = sizeof(receivedFrom);
		err = socket_recvfrom(&m_sock, &m_buffer[m_bytesBuffered], PE_SOCKET_RECEIVE_BUFFER_SIZE - m_bytesBuffered, &got, (sockaddr*) &dstAddr, &dstLen, tm);
		
        if (err != IO_DONE && err != IO_TIMEOUT && err != WSAECONNRESET)
        {
            if (err == IO_CLOSED)
                PEINFO("PE: Warning: Socket disconnected.\n", socket_strerror(err));
            else
                PEINFO("PE: Warning: Socket error on receive: %s. Will disconnect.\n", socket_strerror(err));
		
            disconnect();
		
            return;
        }
        m_bytesBuffered += got;
        
        bool maybeHaveMoreData = m_bytesBuffered == PE_SOCKET_RECEIVE_BUFFER_SIZE;
	
        PEASSERT(m_bytesBuffered < PE_SOCKET_RECEIVE_BUFFER_SIZE, "Buffer overflow");

	#if PACKET_DROP
		
		if (activatePacketDrop)
		{
			int num = (rand() % (UPPER_BOUND - LOWER_BOUND)) + LOWER_BOUND;

			if (PACKET_DROP_SEVERITY_LEVEL_1 && num == LUCKY_NUMBER_1)
			{
				m_numPacketDrop++;
				return;
			}

			if (PACKET_DROP_SEVERITY_LEVEL_2 && (num == LUCKY_NUMBER_1 || num == LUCKY_NUMBER_2))
			{
				m_numPacketDrop++;
				return;
			}

			if (PACKET_DROP_SEVERITY_LEVEL_3 && (num == LUCKY_NUMBER_1 || num == LUCKY_NUMBER_2 || num == LUCKY_NUMBER_3))
			{
				m_numPacketDrop++;
				return;
			}
		}

	#endif
    
        while (true) // try to process received info
        {
            if (m_bytesBuffered < 4)
            {
                 // nothing to process. we need to know packet size that is 4 bytes
                if (maybeHaveMoreData) break;
                else return;
            }
            
            PrimitiveTypes::Int32 packetSize;
            StreamManager::ReadInt32(m_buffer, packetSize);
	
            if (m_bytesBuffered >= packetSize)
            {
                PE::Packet *pPacket = (PE::Packet *)(pemalloc(m_arena, packetSize));

                memcpy(&pPacket->m_data[0], m_buffer, packetSize);

				if (m_bytesBuffered - packetSize > 0) // if we have data trailing current packet
					memmove(m_buffer, &m_buffer[packetSize], m_bytesBuffered - packetSize);

                m_bytesBuffered -= packetSize;
				m_pNetContext->getStreamManager()->receivePacket(pPacket);

                pefree(m_arena, pPacket);
            }
            else
            {
                if (maybeHaveMoreData) break;
                else return;
            }
        }
    }
}

void ConnectionManager::do_UPDATE(Events::Event *pEvt)
{
	receivePackets();
}

void ConnectionManager::do_ACTIVATE_PACKET_LOSS(Events::Event *pEvt)
{
	activatePacketDrop = true;
}

void ConnectionManager::do_DEACTIVATE_PACKET_LOSS(Events::Event *pEvt)
{
	activatePacketDrop = false;
}

#if 0 // template
//////////////////////////////////////////////////////////////////////////
// ConnectionManager Lua Interface
//////////////////////////////////////////////////////////////////////////
//
void ConnectionManager::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
{
	/*
	static const struct luaL_Reg l_functions[] = {
		{"l_clientConnectToTCPServer", l_clientConnectToTCPServer},
		{NULL, NULL} // sentinel
	};

	luaL_register(luaVM, 0, l_functions);
	*/

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
#endif
//////////////////////////////////////////////////////////////////////////

	
}; // namespace Components
}; // namespace PE
