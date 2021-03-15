#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "EventManager.h"

// Outer-Engine includes
#include <queue>

// Inter-Engine includes

#include "../Lua/LuaEnvironment.h"

// additional lua includes needed
extern "C"
{
#include "../../luasocket_dist/src/socket.h"
#include "../../luasocket_dist/src/inet.h"
};

#include "../../../GlobalConfig/GlobalConfig.h"

#include "PrimeEngine/Utils/Networkable.h"

#include "PrimeEngine/Events/StandardEvents.h"

#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/Networking/NetworkManager.h"
#include "PrimeEngine/Networking/StreamManager.h"

#include "PrimeEngine/Scene/DebugRenderer.h"

#include "GhostManager.h"

// Sibling/Children includes
#include "PrimeEngine/Events/StandardGhost.h"

// mask state comparator
struct StateMaskComparator
{
    /* TODO: --Pablo
     *
     * Our game objects in the world are soldiers right now. I may or may not need to factor in the
     * object priority as our game begins to scale in this and the next milestone. So for now, I will
     * let the object status be packed first and every other ghost object will not be sorted
     */
    bool operator() ( const PE::Ghosts::Ghost* lhs, const PE::Ghosts::Ghost* rhs )
    {
        // Add and deleted objects will be the more important
        return ( ( lhs->m_ghostStateMask & 0x1 ) == 0x1 && ( rhs->m_ghostStateMask & 0x1 ) != 0x1 ) ||
            ( ( lhs->m_ghostStateMask & 0x2 ) == 0x2 && ( rhs->m_ghostStateMask & 0x2 ) != 0x2 );
    }
};

namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(GhostManager, Component);

// Constructor -------------------------------------------------------------
GhostManager::GhostManager(PE::GameContext &context, PE::MemoryArena arena, PE::NetworkContext &netContext, Handle hMyself)
	:Component(context, arena, hMyself)
	,m_transmitterNextGhostOrderId(1)
{
	m_pNetContext = &netContext;
}

GhostManager::~GhostManager()
{
}

// Methods -----------------------------------------------------------------
void GhostManager::initialize()
{
}

/// Register the ghost into the client's ghostmanager
void GhostManager::registerGhost(PE::Ghosts::Ghost* pGhost)
{
	auto findGhost = m_ghostList.find(pGhost->m_ghostId);
	if (findGhost == m_ghostList.end())
	{
		m_ghostList[pGhost->m_ghostId] = pGhost;
	}
	else
	{
		assert(!"Ghost already registered");
	}
}

/// Check if the ghost is in scope and exists
bool GhostManager::findGhost(int clientId)
{
	auto findGhost = m_ghostList.find(clientId);
	return findGhost != m_ghostList.end();
}

int GhostManager::haveGhostsToSend()
{
	int res = 0;
	for (auto& it : m_ghostList)
	{
		PE::Ghosts::Ghost* tempGhost = it.second;
		if (tempGhost->m_ghostStateMask != 0x0)
		{
			res++;
		}
	}
	return res;
}

/// clear the ghost state mask 
void GhostManager::clearGhostStateMask()
{
	for (auto& it : m_ghostList)
	{
		it.second->m_ghostStateMask &= 0x0;
	}
}


/// called by StreamManager to put ghost state in packet
int GhostManager::fillInNextPacket(char *pDataStream, TransmissionRecord *pRecord, int packetSizeAllocated, 
										bool &out_usefulDataSent, bool &out_wantToSendMore)
{
	// Return the status after filling in the packet
	out_usefulDataSent = false;
	out_wantToSendMore = false;

	// Do this in the stream manager
	// Build the list of ghosts whose state mask has been modified
	std::priority_queue<PE::Ghosts::Ghost*, std::vector<PE::Ghosts::Ghost*>, StateMaskComparator> modifiedGhost;
	for (auto& it : m_ghostList)
	{
		PE::Ghosts::Ghost* tempGhost = it.second;
		if (tempGhost->m_ghostStateMask != 0x0)
		{
			modifiedGhost.push(it.second);
		}
	}

	// Fill in the number of ghost states that we need to send
	int numGhostToSend = modifiedGhost.size();

	int numGhostReallySent = 0;
	int size = 0;
	size += StreamManager::WriteInt32(numGhostToSend, &pDataStream[size]);

	int sizeLeft = packetSizeAllocated - size;

	for (int i = 0; i < numGhostToSend; i++)
	{
		// Object to store the transmission data
		PE::Ghosts::Ghost* ghost = modifiedGhost.top();
		modifiedGhost.pop();
		GhostTransmissionData gTransmitData;
		int dataSize = 0;

		// Check if the ghsot has been registered to be networkable and write it into packet
		PrimitiveTypes::Int32 classId = ghost->net_getClassMetaInfo()->m_classId;
		if (classId == -1)
		{
			assert(!"Ghost's class id is -1. Add it to global registry");
		}
		dataSize += StreamManager::WriteInt32(classId, &gTransmitData.m_payload[dataSize]);

		dataSize += ghost->packCreationData(&gTransmitData.m_payload[dataSize]);
		gTransmitData.m_size = dataSize;

		if (gTransmitData.m_size > sizeLeft)
		{
			// Can't fit this event, break out
			// Can optimize to include next ghost that can fit
			out_wantToSendMore = true;
			break;
		}

		// Push the transmissiondata into the transmission record
		pRecord->m_sentGhosts.push_back(gTransmitData);
		memcpy(&pDataStream[size], &gTransmitData.m_payload[0], gTransmitData.m_size);
		size += gTransmitData.m_size;
		sizeLeft = packetSizeAllocated - size;

		// Reset the ghost state mask
		ghost->m_ghostStateMask &= 0x0;

		// Count number of ghosts not acked and events really sent
		numGhostReallySent++;
		m_transmitterNumGhostsNotAcked++;
	}

	// Write the number of ghost that was able to fit into the packet
	StreamManager::WriteInt32(numGhostReallySent, &pDataStream[0]);
	out_usefulDataSent = numGhostReallySent > 0;
	return size;
}

/// called by StreamManager to process transmission record deliver notification
void GhostManager::processNotification(TransmissionRecord *pTransmittionRecord, bool delivered)
{
	for (unsigned int i = 0; i < pTransmittionRecord->m_sentGhosts.size(); i++)
	{
		GhostTransmissionData &ghost = pTransmittionRecord->m_sentGhosts[i];

		if (delivered)
		{
			// We don't have to worry about it
			m_transmitterNumGhostsNotAcked--;
		}
		else
		{
			// We need to try resending again
			assert(!"Support later");
		}
	}
}

/// called by StreamManager to process the received packet
int GhostManager::receiveNextPacket(char *pDataStream)
{
	int read = 0;
	PrimitiveTypes::Int32 numGhosts;
	read += StreamManager::ReadInt32(&pDataStream[read], numGhosts);

	// Loop through and get the ghost state mask
	for (int i = 0; i < numGhosts; i++)
	{	
		// Reconstruct the ghost
		PrimitiveTypes::Int32 classId;
		read += StreamManager::ReadInt32(&pDataStream[read], classId);

		GlobalRegistry *globalRegistry = GlobalRegistry::Instance();
		MetaInfo *pMetaInfo = globalRegistry->getMetaInfo(classId);
		if (!pMetaInfo->getFactoryConstructFunction())
		{
			assert(!"Received network creation command but don't have factory create function associated with the given class");
		}
		void *ptr = (pMetaInfo->getFactoryConstructFunction())(*m_pContext, m_arena);
		if (!ptr)
		{
			// For some reason, the factor construct is using the base class?
			assert(!"Factory construct function returned null");
		}
		
		// Assign the network client ID
		PE::Ghosts::Ghost *pGhost = (PE::Ghosts::Ghost*)(ptr);
		read += pGhost->constructFromStream(&pDataStream[read]);
		pGhost->m_networkClientId = m_pNetContext->getClientId(); // DEBUG: will be id of client on server

		// Clear and push_back the received ghost which will be processed
		m_receivedGhost.push_back(pGhost);
	}

	// Process the received ghost
	for (int i = 0; i < m_receivedGhost.size(); i++)
	{
		PE::Ghosts::Ghost* ghostPtr = m_receivedGhost[i];

		if (ghostPtr->m_ghostStateMask != 0x0)
		{
			// Check if the ghost came into scope
			if (ghostPtr->m_ghostStateMask & 0x1)
			{
				registerGhost(ghostPtr);
				
				// Clear out the ghost's create state mask properly
				PrimitiveTypes::Int32 mask = ~0x1;
				ghostPtr->m_ghostStateMask = mask & 0x1;
			}
			else
			{
				m_ghostList[ghostPtr->m_ghostId]->m_ghostStateMask = ghostPtr->m_ghostStateMask;
				pefree(m_arena, m_receivedGhost[i]); // free the ghost when we're done
			}
		}
		else
		{
			assert(!"Should not have sent a ghost that didn't change states");
		}
	}
	m_receivedGhost.clear();
    
	return read;
}

void GhostManager::addDefaultComponents()
{
	Component::addDefaultComponents();

	// Add any default components we ened
}

void GhostManager::do_UPDATE(Events::Event *pEvt)
{

}
void GhostManager::debugRender(int &threadOwnershipMask, float xoffset, float yoffset)
{
	// Check 
	float dy1 = yoffset;
	int i = 0;
	for (auto &it : m_ghostList)
	{
		// Bytes for the ghost manager
		char bytesPlace[4];
		PrimitiveTypes::Int32 firstMask = it.second->m_ghostStateMask & 0xF;
		if (firstMask >= 0xA)
		{
			bytesPlace[0] = 'A' + (firstMask - 0xA);
		}
		else
		{
			bytesPlace[0] = '0' + firstMask;
		}

		sprintf(PEString::s_buf, "Ghost ID: %d State: 0x%c", it.second->m_ghostId, bytesPlace[0]);
		DebugRenderer::Instance()->createTextMesh(
			PEString::s_buf, true, false, false, false, 0,
			Vector3(xoffset, yoffset + i * 0.05f, 0), 1.0f, threadOwnershipMask);
		i++;
	}
}

        
};   
};
