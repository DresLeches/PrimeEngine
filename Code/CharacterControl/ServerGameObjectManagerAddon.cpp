#include "ServerGameObjectManagerAddon.h"

#include "PrimeEngine/Lua/Server/ServerLuaEnvironment.h"
#include "PrimeEngine/Networking/Server/ServerNetworkManager.h"
#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/GameObjectModel/GameObjectManager.h"
#include "PrimeEngine/Game/Server/ServerGame.h"
#include "PrimeEngine/Events/StandardGhost.h"

#include "Characters/SoldierNPC.h"
#include "WayPoint.h"
#include "Tank/ClientTank.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;
using namespace CharacterControl::Components;

float CharacterControl::Components::ServerGameObjectManagerAddon::m_officialGameTime = 0.0f;
bool CharacterControl::Components::ServerGameObjectManagerAddon::m_gameStarted = false;

namespace CharacterControl{
namespace Components
{
PE_IMPLEMENT_CLASS1(ServerGameObjectManagerAddon, GameObjectManagerAddon); // creates a static handle and GteInstance*() methods. still need to create construct

void ServerGameObjectManagerAddon::addDefaultComponents()
{
	GameObjectManagerAddon::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Event_MoveTank_C_to_S, ServerGameObjectManagerAddon::do_MoveTank);
	PE_REGISTER_EVENT_HANDLER(Event_MovePlayer_C_to_S, ServerGameObjectManagerAddon::do_MovePlayer);
	PE_REGISTER_EVENT_HANDLER(Event_CLIENT_SERVER_CONNECTED_ACK, ServerGameObjectManagerAddon::do_CreateClientPlayerInfo);
}

void ServerGameObjectManagerAddon::do_CreateClientPlayerInfo(PE::Events::Event *pEvt)
{
	// Check if the client connection was successful
	ServerNetworkManager* pNM = (ServerNetworkManager*)(m_pContext->getNetworkManager());
	PrimitiveTypes::UInt32 netContextId = pNM->m_clientConnections.m_size - 1;

	if (m_playerInfo.size() < pNM->m_clientConnections.m_size)
	{
		m_playerInfo.push_back(PlayerInfo());
		PlayerInfo& playerInfo = m_playerInfo[m_playerInfo.size() - 1];
		playerInfo.clientId = netContextId;

		// Hack: To get the server to send the initial position
		playerInfo.pos = Vector3(0.0f, 0.0f, -20.0f);
		playerInfo.fwd = Vector3(0.0f, 0.0f, -90.0f);
	}

	// Dirty the other client's ghost state mask
	PE::Ghosts::Ghost_SOLDIERS* playerGhost = (PE::Ghosts::Ghost_SOLDIERS*) pNM->m_ghostCache[netContextId];
	playerGhost->setPositionStateUpdated();
	playerGhost->setRotationStateUpdated();

	for (PrimitiveTypes::UInt32 i = 0; i < pNM->m_clientConnections.m_size; i++)
	{
		if (playerGhost->m_ghostId == i)
		{
			continue;
		}

		pNM->updateGhostMaskState(playerGhost->m_ghostId, playerGhost->m_ghostStateMask, i);
	}
	pNM->m_stateMaskDebug[playerGhost->m_ghostId] = playerGhost->m_ghostStateMask;
	playerGhost->m_ghostStateMask &= 0x0;

	// Game has started
	if (m_playerInfo.size() >= 2)
	{
		m_officialGameTime = 0.0f;
		m_gameStarted = true;
	}
}

void ServerGameObjectManagerAddon::do_MovePlayer(PE::Events::Event *pEvt)
{
	assert(pEvt->isInstanceOf<Event_MovePlayer_C_to_S>());

	Event_MovePlayer_C_to_S *pRealEvt = (Event_MovePlayer_C_to_S*)(pEvt);

	// Check if the transform actually changed
	PlayerInfo& playerInfo = m_playerInfo[pRealEvt->m_networkClientId];
	
	Vector3 pos = pRealEvt->m_transform.getPos();
	Vector3 fwd = pRealEvt->m_transform.getN();

	// Server and ghost
	ServerNetworkManager *pNM = (ServerNetworkManager *)(m_pContext->getNetworkManager());
	PE::Ghosts::Ghost_SOLDIERS* playerGhost = (PE::Ghosts::Ghost_SOLDIERS*) pNM->m_ghostCache[pRealEvt->m_networkClientId];

	if (playerInfo.pos == pos && playerInfo.fwd == fwd)
	{
		// Don't bother sending. It's useless.
		pNM->m_stateMaskDebug[playerGhost->m_ghostId] = 0x0;
		return;
	}

	// Change the state mask for all the client's ghost manager for now
	// TODO: Add a logic to see if the client is in scope with another
	if (playerInfo.pos != pos)
	{
		playerGhost->setPositionStateUpdated();
	}
	if (playerInfo.fwd != fwd)
	{
		playerGhost->setRotationStateUpdated();
	}

	for (PrimitiveTypes::UInt32 i = 0; i < pNM->m_clientConnections.m_size; i++)
	{
		if (playerGhost->m_ghostId == i)
		{
			continue;
		}

		pNM->updateGhostMaskState(playerGhost->m_ghostId, playerGhost->m_ghostStateMask, i);
	}
	pNM->m_stateMaskDebug[playerGhost->m_ghostId] = playerGhost->m_ghostStateMask;
	playerGhost->m_ghostStateMask &= 0x0;

	// Update the player info
	playerInfo.pos = pos;
	playerInfo.fwd = fwd;

	// Debug: Previous network latency time
	PE::Components::ServerNetworkManager::latencyTime = abs(m_officialGameTime - pRealEvt->m_timeStamp);

	// Schedule the move event
	Event_MovePlayer_S_to_C fwdEvent(*m_pContext);
	fwdEvent.m_transform = pRealEvt->m_transform;

	fwdEvent.m_clientPlayerId = pRealEvt->m_networkClientId;
	fwdEvent.m_receivedTime = m_officialGameTime;  // For measuring the RTT latency
	fwdEvent.m_timeStamp = abs(m_officialGameTime - pRealEvt->m_timeStamp);
	pNM->scheduleEventToAllExcept(&fwdEvent, m_pContext->getGameObjectManager(), pRealEvt->m_networkClientId, false);
	//pNM->scheduleEventToAll(&fwdEvent, m_pContext->getGameObjectManager(), false);
}

void ServerGameObjectManagerAddon::do_MoveTank(PE::Events::Event *pEvt)
{
	assert(pEvt->isInstanceOf<Event_MoveTank_C_to_S>());

	Event_MoveTank_C_to_S *pTrueEvent = (Event_MoveTank_C_to_S*)(pEvt);

	// need to send this event to all clients except the client it came from

	Event_MoveTank_S_to_C fwdEvent(*m_pContext);
	fwdEvent.m_transform = pTrueEvent->m_transform;
	fwdEvent.m_clientTankId = pTrueEvent->m_networkClientId; // need to tell cleints which tank to move

	ServerNetworkManager *pNM = (ServerNetworkManager *)(m_pContext->getNetworkManager());
	pNM->scheduleEventToAllExcept(&fwdEvent, m_pContext->getGameObjectManager(), pTrueEvent->m_networkClientId, false);

}


}
}
