#ifndef __PrimeEngineGhostTransmissionData_H__
#define __PrimeEngineGhostTransmissionData_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>
#include <vector>
#include <deque>

// Inter-Engine includes

#include "../Events/Component.h"
#include "Packet.h"

extern "C"
{
#include "../../luasocket_dist/src/socket.h"
};

#include "PrimeEngine/Networking/NetworkContext.h"
#include "PrimeEngine/Utils/Networkable.h"

namespace PE {
	namespace Components
	{
		struct Component;
	};
	namespace Ghosts
	{
		struct Ghost;
	};

struct GhostTransmissionData
{
	int m_size;
	char m_payload[PE_MAX_GHOST_PAYLOAD];
};

};

#endif
