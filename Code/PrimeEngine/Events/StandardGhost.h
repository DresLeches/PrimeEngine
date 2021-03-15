#ifndef __PrimeEngineStandardGhost_H__
#define __PrimeEngineStandardGhost_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "../Utils/PEUUID.h"
#include "../Utils/PEClassDecl.h"

#include "PrimeEngine/Utils/Networkable.h"

// Sibling/Children includes
#include "Ghost.h"

namespace PE {
namespace Ghosts {

// TODO: Do the networkable binding for this ghost
struct Ghost_SOLDIERS : public Ghost {
    PE_DECLARE_CLASS(Ghost_SOLDIERS);

    Ghost_SOLDIERS(PE::GameContext &context);
	virtual ~Ghost_SOLDIERS() {}

    // Ghost state mask functions
	void setDamageStateUpdated();
    void setPositionStateUpdated();
    void setRotationStateUpdated();
    void setAnimationStateUpdated();
	void setHealthStateUpdated();

    // Networkable
    virtual int packCreationData(char *pDataStream);
    virtual int constructFromStream(char *pDataStream);

    // Factory function used by network
    static void* FactoryConstruct(PE::GameContext&, PE::MemoryArena);
	
};

};
};   

#endif
