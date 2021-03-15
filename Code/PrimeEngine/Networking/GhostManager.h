#ifndef __PrimeEngineGhostManager_H__
#define __PrimeEngineGhostManager_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <deque>

// Inter-Engine includes
//#include "PrimeEngine/Networking/NetworkContext.h"

//#include "PrimeEngine/Utils/Networkable.h"

#include "../Events/Component.h"

extern "C"
{
#include "../../luasocket_dist/src/socket.h"
};

/*
// Sibling/Children includes
#include "Packet.h"
#include "PrimeEngine/Events/StandardGhost.h"
#include "GhostTransmissionData.h"
*/

// forward declaration
namespace PE {

namespace Components
{
    struct NetworkContext;
}

namespace Ghosts 
{
    struct Ghost;
}

}

namespace PE{
namespace Components {

struct GhostManager : public Component {

    PE_DECLARE_CLASS(GhostManager);

    // Constructor -------------------------------------------------------------
	GhostManager(PE::GameContext &context, PE::MemoryArena arena, PE::NetworkContext &netContext, Handle hMyself);

	virtual ~GhostManager();

	// Methods -----------------------------------------------------------------
	virtual void initialize();

	/// Register the soldier ghost into the ghostmanager
	void registerGhost(PE::Ghosts::Ghost* pGhost);

	/// Check if the ghost is in scope and exists
	bool findGhost(int clientId);

	/// How many ghost with changed status have to be setn
	int haveGhostsToSend();

	/// called by StreamManager to put ghost state in packet
	int fillInNextPacket(char *pDataStream, TransmissionRecord *pRecord, int packetSizeAllocated, 
							bool &out_usefulDataSent, bool &out_wantToSendMore);

	/// called by StreamManager to process transmission record deliver notification
	void processNotification(TransmissionRecord *pTransmittionRecord, bool delivered);

	/// called by StreamManager to process the received packet
	int receiveNextPacket(char *pDataStream);

	/// clear the ghost state mask 
	void clearGhostStateMask();

	/// render the debug information onto the screen
	void debugRender(int &threadOwnershipMask, float xoffset = 0, float yoffset = 0);
	
	// Component ------------------------------------------------------------
	virtual void addDefaultComponents();
	
	// Update events
	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
	virtual void do_UPDATE(Events::Event *pEvt);

	// Individual ghost -------------------------------------------------------

	// Loading ----------------------------------------------------------------- 	

    //////////////////////////////////////////////////////////////////////////
	// Member variables 
	//////////////////////////////////////////////////////////////////////////

	std::unordered_map<int, PE::Ghosts::Ghost*> m_ghostList;  // Hash the ghost ID and store the pointer to it
	std::vector<PE::Ghosts::Ghost*> m_receivedGhost;

	int m_transmitterNumGhostsNotAcked;
	int m_transmitterNextGhostOrderId;
	PE::NetworkContext *m_pNetContext;
}; 
        
}; // namespace Components
}; // namespace PE

#endif
