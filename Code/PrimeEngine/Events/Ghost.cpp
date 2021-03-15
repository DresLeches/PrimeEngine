#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "../Lua/LuaEnvironment.h"

// Outer Includes

// Inner Includes
#include "PrimeEngine/Networking/StreamManager.h"

// Child Includes
#include "Ghost.h"


namespace PE {
namespace Ghosts {

PE_IMPLEMENT_CLASS0(Ghost);

// Networkable
int Ghost::packCreationData(char *pDataStream)
{
	// Write into the packet
	int written = 0;
	written += PE::Components::StreamManager::WriteInt32(m_ghostId, &pDataStream[written]);
	written += PE::Components::StreamManager::WriteInt32(m_ghostStateMask, &pDataStream[written]);
	return written;
}
int Ghost::constructFromStream(char *pDataStream)
{
	// Read the packet
	int read = 0;
	read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], m_ghostId);
	read += PE::Components::StreamManager::ReadInt32(&pDataStream[read], m_ghostStateMask);
	return read;
}

// Factory function used by network
void* Ghost::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
{
	Ghost* ghost = new (arena) Ghost(context);
	return ghost;
}

}
}
