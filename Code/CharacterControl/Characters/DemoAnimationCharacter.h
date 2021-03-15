#pragma once

#include "PrimeEngine/Events/Component.h"
#include "../Events/Events.h"

#include "CharacterAnimationSM.h"

namespace CharacterControl {

namespace Events
{

struct Event_CreateDemoAnimationCharacter : public PE::Events::Event_CREATE_MESH
{
	PE_DECLARE_CLASS(Event_CreateDemoAnimationCharacter);

	Event_CreateDemoAnimationCharacter(int &threadOwnershipMask) : PE::Events::Event_CREATE_MESH(threadOwnershipMask) {}
	// override SetLuaFunctions() since we are adding custom Lua interface
	static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);

	// Lua interface prefixed with l_
	static int l_Construct(lua_State* luaVM);

	//int m_npcType;
	char m_gunMeshName[64];
	char m_gunMeshPackage[64];
	//char m_patrolWayPoint[32];
};

};

namespace Components {

struct DemoAnimationCharacter : public PE::Components::Component
{
	PE_DECLARE_CLASS(DemoAnimationCharacter);

	DemoAnimationCharacter(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, Events::Event_CreateDemoAnimationCharacter *pEvt);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
	virtual void do_UPDATE(PE::Events::Event *pEvt);

	virtual void addDefaultComponents();

	private:
	CharacterAnimationSM* pCharacterAnimSM;

	enum AnimId
	{
		NONE = -1,
		IDLE = 0,
		FORWARD = 5,
		BACK = 1,
		LEFT = 4,
		RIGHT = 7,
		SHOOT = 6,
		JUMP = 3,
		DEATH = 2
	};

    // TODO: Why is this index negative?
	int debugIndex = -1;
	int debugSMIndex = -1;
	float debugWeight = 0.0;
	bool debugState = false;
	float debugTimer = 0.0;
};

}; // namespace Components
}; // namespace CharacterControl

