
#include "StandardEvents.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Lua/EventGlue/EventDataCreators.h"

#include "PrimeEngine/Networking/StreamManager.h"

#include "StandardGhost.h"

namespace PE {
namespace Ghosts {

PE_IMPLEMENT_CLASS1(Ghost_SOLDIERS, Ghost)
Ghost_SOLDIERS::Ghost_SOLDIERS(PE::GameContext &context)
	:Ghost(context)
{
}

// 0000 0000 0000 0000
/* Ghost Soldier State Mask Bits
 * Bit 1: New Ghost
 * Bit 2: Delete Ghost
 * Bit 3: Position
 * Bit 4: Rotation
 * Bit 5: Animation State
 * Bit 6: Health state
 */
// Ghost state mask functions
void Ghost_SOLDIERS::setDamageStateUpdated()
{
	m_ghostStateMask |= 0x2;
}

void Ghost_SOLDIERS::setPositionStateUpdated()
{
	m_ghostStateMask |= 0x4;
}

void Ghost_SOLDIERS::setRotationStateUpdated()
{
	m_ghostStateMask |= 0x8;
}

void Ghost_SOLDIERS::setAnimationStateUpdated()
{
	m_ghostStateMask |= 0x10;
}

void Ghost_SOLDIERS::setHealthStateUpdated()
{
	m_ghostStateMask |= 0x20;
}


// Networkable
int Ghost_SOLDIERS::packCreationData(char *pDataStream)
{
	return Ghost::packCreationData(pDataStream);
}

int Ghost_SOLDIERS::constructFromStream(char *pDataStream)
{
	return Ghost::constructFromStream(pDataStream);
}

// Factory function used by network
void* Ghost_SOLDIERS::FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena)
{
	Ghost_SOLDIERS* gSoldiers = new (arena) Ghost_SOLDIERS(context);
	return gSoldiers;
}
    
};
};

