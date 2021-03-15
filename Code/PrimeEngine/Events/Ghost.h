#ifndef __PrimeEngineGhost_H__
#define __PrimeEngineGhost_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>

// Inter-Engine includes
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"

#include "../Utils/PEUUID.h"
#include "../Utils/PEClassDecl.h"
#include "../Utils/Networkable.h"


// Sibling/Children includes

namespace PE {
namespace Ghosts {

#define GHOST_DEBUG 0

#if GHOST_DEBUG

// Might need this?
enum GhostReturnCodes
{
    DEFAULT = 0,
    STOP_DISTRIBUTION = 1,
};
#endif

struct Ghost : public PEClass, public Networkable
{
    PE_DECLARE_CLASS(Ghost);
	PE_DECLARE_NETWORKABLE_CLASS;

	// Enum to determine the type of ghost
	enum GhostType
	{
		DEFAULT_GHOST,
		SOLDIER_GHOST
	};

    // meta methods: wrap around MetaInfo methods
	int getClassId() { return getClassMetaInfo()->m_classId; }
	const char *getClassName() { return getClassMetaInfo()->getClassName(); }

	template <typename T>
	bool isInstanceOf() {
		return getClassMetaInfo()->isSubClassOf(T::s_metaInfo.m_classId);
	}

	bool isInstanceOf(int classId) {
		return getClassMetaInfo()->isSubClassOf(classId);
	}

	void printClassHierarchy()
	{
		return getClassMetaInfo()->printClassHierarchy();
	}

    Ghost(PE::GameContext &context)
		: Networkable(context, this)
		, m_networkClientId(-1)
		, m_ghostId(-1)
		, m_ghostStateMask(0)
		, m_objPriority(0)
    {
    }
    virtual ~Ghost() {}

    // call this when a ghost is not used anymore
    void releaseGhostData()
    {
        m_dataHandle.release();
    }

    PrimitiveTypes::Bool hasValidData()
    {
        return m_dataHandle.isValid();
    }

	// Networkable
	virtual int packCreationData(char *pDataStream);
	virtual int constructFromStream(char *pDataStream);

	// Factory function used by network
	static void* FactoryConstruct(PE::GameContext& context, PE::MemoryArena arena);

    // Member variables
    Handle m_target; // Refer to the object that we will ghost
    Handle m_dataHandle;
    int m_networkClientId; // Determine which client is which

	// Properties about the ghost
	int m_ghostId;
	int m_objPriority;  // Object priority. Not going to need for a while
	PrimitiveTypes::Int32 m_ghostStateMask;
};

}; // namespace Events
}; // namespace PE

#endif
