

#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/RootSceneNode.h"

#include "DemoAnimationCharacter.h"
#include "CharacterAnimationSM.h"

#include "PrimeEngine/PrimeEngineIncludes.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;

namespace CharacterControl {

namespace Events
{

PE_IMPLEMENT_CLASS1(Event_CreateDemoAnimationCharacter, PE::Events::Event_CREATE_SKELETON);

void Event_CreateDemoAnimationCharacter::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
{
	static const struct luaL_Reg l_Event_CreateDemoAnimationCharacter[] = {
		{ "Construct", l_Construct },
		{ NULL, NULL } // sentinel
	};

	// register the functions in current lua table which is the table for Event_CreateDemoAnimationCharacter
	luaL_register(luaVM, 0, l_Event_CreateDemoAnimationCharacter);
}

int Event_CreateDemoAnimationCharacter::l_Construct(lua_State* luaVM)
{
	PE::Handle h("EVENT", sizeof(Event_CreateDemoAnimationCharacter));

	// get arguments from stack
	int numArgs, numArgsConst;
	numArgs = numArgsConst = 18;

	PE::GameContext *pContext = (PE::GameContext*)(lua_touserdata(luaVM, -numArgs--));

	// this function should only be called frm game thread, so we can use game thread thread owenrship mask
	Event_CreateDemoAnimationCharacter *pEvt = new(h) Event_CreateDemoAnimationCharacter(pContext->m_gameThreadThreadOwnershipMask);

	const char* name = lua_tostring(luaVM, -numArgs--);
	const char* package = lua_tostring(luaVM, -numArgs--);

	const char* gunMeshName = lua_tostring(luaVM, -numArgs--);
	const char* gunMeshPackage = lua_tostring(luaVM, -numArgs--);

	float positionFactor = 1.0f / 100.0f;

	Vector3 playerPos, u, v, n;
	playerPos.m_x = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	playerPos.m_y = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	playerPos.m_z = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;

	u.m_x = (float)lua_tonumber(luaVM, -numArgs--); u.m_y = (float)lua_tonumber(luaVM, -numArgs--); u.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	v.m_x = (float)lua_tonumber(luaVM, -numArgs--); v.m_y = (float)lua_tonumber(luaVM, -numArgs--); v.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	n.m_x = (float)lua_tonumber(luaVM, -numArgs--); n.m_y = (float)lua_tonumber(luaVM, -numArgs--); n.m_z = (float)lua_tonumber(luaVM, -numArgs--);

	pEvt->m_peuuid = LuaGlue::readPEUUID(luaVM, -numArgs--);

	/*
	const char* wayPointName = NULL;

	if (!lua_isnil(luaVM, -numArgs))
	{
		// have patrol waypoint name
		wayPointName = lua_tostring(luaVM, -numArgs--);
	}
	else
		// ignore
		numArgs--;
		*/


	// set data values before popping memory off stack
	StringOps::writeToString(name, pEvt->m_meshFilename, 255);
	StringOps::writeToString(package, pEvt->m_package, 255);

	StringOps::writeToString(gunMeshName, pEvt->m_gunMeshName, 64);
	StringOps::writeToString(gunMeshPackage, pEvt->m_gunMeshPackage, 64);
	//StringOps::writeToString(wayPointName, pEvt->m_patrolWayPoint, 32);

	lua_pop(luaVM, numArgsConst); //Second arg is a count of how many to pop

	pEvt->hasCustomOrientation = true;
	pEvt->m_pos = playerPos;
	pEvt->m_u = u;
	pEvt->m_v = v;
	pEvt->m_n = n;

	LuaGlue::pushTableBuiltFromHandle(luaVM, h);

	return 1;
}

};

namespace Components {

PE_IMPLEMENT_CLASS1(DemoAnimationCharacter, Component);

DemoAnimationCharacter::DemoAnimationCharacter(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, Event_CreateDemoAnimationCharacter *pEvt) : Component(context, arena, hMyself)
{
	// need to acquire redner context for this code to execute thread-safe
	m_pContext->getGPUScreen()->AcquireRenderContextOwnership(pEvt->m_threadOwnershipMask);

	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pMainSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pMainSN->addDefaultComponents();

	pMainSN->m_base.setPos(pEvt->m_pos);
	pMainSN->m_base.setU(pEvt->m_u);
	pMainSN->m_base.setV(pEvt->m_v);
	pMainSN->m_base.setN(pEvt->m_n);


	RootSceneNode::Instance()->addComponent(hSN);

	// add the scene node as component of soldier without any handlers. this is just data driven way to locate scnenode for soldier's components
	{
		static int allowedEvts[] = { 0 };
		addComponent(hSN, &allowedEvts[0]);
	}

	int numskins = 1; // 8
	for (int iSkin = 0; iSkin < numskins; ++iSkin)
	{
		float z = (iSkin / 4) * 1.5f;
		float x = (iSkin % 4) * 1.5f;
		PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
		SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
		pSN->addDefaultComponents();

		pSN->m_base.setPos(Vector3(x, 0, z));

		// rotation scene node to rotate soldier properly, since soldier from Maya is facing wrong direction
		PE::Handle hRotateSN("SCENE_NODE", sizeof(SceneNode));
		SceneNode *pRotateSN = new(hRotateSN) SceneNode(*m_pContext, m_arena, hRotateSN);
		pRotateSN->addDefaultComponents();

		pSN->addComponent(hRotateSN);

		pRotateSN->m_base.turnLeft(3.1415f);

		PE::Handle hCharacterAnimSM("CharacterAnimationSM", sizeof(CharacterAnimationSM));
		pCharacterAnimSM = new(hCharacterAnimSM) CharacterAnimationSM(*m_pContext, m_arena, hCharacterAnimSM);
		pCharacterAnimSM->addDefaultComponents();

		pCharacterAnimSM->m_debugAnimIdOffset = 0;// rand() % 3;

		PE::Handle hSkeletonInstance("SkeletonInstance", sizeof(SkeletonInstance));
		SkeletonInstance *pSkelInst = new(hSkeletonInstance) SkeletonInstance(*m_pContext, m_arena, hSkeletonInstance, hCharacterAnimSM);
		pSkelInst->addDefaultComponents();

		//pSkelInst->initFromFiles("soldier_Soldier_Skeleton.skela", "Soldier", pEvt->m_threadOwnershipMask);
		pSkelInst->initFromFiles("PlayerCharacter_CATRigHub001.skela", "PlayerModel", pEvt->m_threadOwnershipMask);

		//pSkelInst->setAnimSet("soldier_Soldier_Skeleton.animseta", "Soldier");
		pSkelInst->setAnimSet("PlayerCharacter_CATRigHub001.animseta", "PlayerModel");

		PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
		MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
		pMeshInstance->addDefaultComponents();

		//pMeshInstance->initFromFile(pEvt->m_meshFilename, pEvt->m_package, pEvt->m_threadOwnershipMask);
		pMeshInstance->initFromFile("Bodyl_mesh.mesha", "PlayerModel", pEvt->m_threadOwnershipMask);

		pSkelInst->addComponent(hMeshInstance);

		// add skin to scene node
		pRotateSN->addComponent(hSkeletonInstance);
		/*
//#if !APIABSTRACTION_D3D11
		{
			PE::Handle hMyGunMesh = PE::Handle("MeshInstance", sizeof(MeshInstance));
			MeshInstance *pGunMeshInstance = new(hMyGunMesh) MeshInstance(*m_pContext, m_arena, hMyGunMesh);

			pGunMeshInstance->addDefaultComponents();
			pGunMeshInstance->initFromFile(pEvt->m_gunMeshName, pEvt->m_gunMeshPackage, pEvt->m_threadOwnershipMask);

			// create a scene node for gun attached to a joint

			PE::Handle hMyGunSN = PE::Handle("SCENE_NODE", sizeof(JointSceneNode));
			JointSceneNode *pGunSN = new(hMyGunSN) JointSceneNode(*m_pContext, m_arena, hMyGunSN, 38);
			pGunSN->addDefaultComponents();

			// add gun to joint
			pGunSN->addComponent(hMyGunMesh);

			// add gun scene node to the skin
			pSkelInst->addComponent(hMyGunSN);
		}
//#endif
*/
		pMainSN->addComponent(hSN);
	}

	m_pContext->getGPUScreen()->ReleaseRenderContextOwnership(pEvt->m_threadOwnershipMask);

	//setup debug blend tree (should add some helper methods to character animation for this)
	/*
	int stand = pCharacterAnimSM->addAnimation(0, STAND, PE::LOOPING);
	int walk = pCharacterAnimSM->addAnimation(0, WALK, PE::LOOPING);
	int run = pCharacterAnimSM->addAnimation(0, RUN, PE::LOOPING);
	int shoot = pCharacterAnimSM->addAnimation(0, STAND_SHOOT, PE::LOOPING);
	int aim = pCharacterAnimSM->addAnimation(0, STAND_AIM, PE::LOOPING);

	int movementTree = pCharacterAnimSM->addBlendNode(walk, run, 0.5f);
	int gunTree = pCharacterAnimSM->addBlendNode(aim, shoot, 0.75f);
	//int actionTree = pCharacterAnimSM->addBlendNode(movementTree, gunTree, 0.5f);
	int actionState = pCharacterAnimSM->addStateNode({ 4, 7 }, { movementTree, gunTree });

	pCharacterAnimSM->animationRoot = debugIndex = pCharacterAnimSM->addBlendNode(stand, actionState, 0.0f);
	debugSMIndex = actionState;
	*/

	/*
	int stand = pCharacterAnimSM->addAnimation(0, STAND, PE::LOOPING);
	int walk = pCharacterAnimSM->addAnimation(0, WALK, PE::LOOPING);
	int shoot = pCharacterAnimSM->addAnimation(0, STAND_SHOOT, PE::LOOPING);
	int movementTree = debugIndex = pCharacterAnimSM->addBlendNode(stand, walk, 0.0f);
	pCharacterAnimSM->animationRoot = debugSMIndex = pCharacterAnimSM->addStateNode({ 4, 7 }, { movementTree, shoot });
	*/

	int a = pCharacterAnimSM->addAnimation(0, IDLE, PE::LOOPING);
	int b = pCharacterAnimSM->addAnimation(0, FORWARD, PE::LOOPING);
	pCharacterAnimSM->animationRoot = debugSMIndex = pCharacterAnimSM->addStateNode({ 4, 7 }, { a, b });

	int g = pCharacterAnimSM->addPartialAnimation(0, DEATH, PE::STAY_ON_ANIMATION_END, 9, 54, 10.0f);
	pCharacterAnimSM->partialAnimationRoot = debugIndex = pCharacterAnimSM->addBlendNode(-1, g, 0.0, 1.0f);
	//pCharacterAnimSM->partialAnimationRoot = debugIndex = pCharacterAnimSM->addStateNode({ 0, 1 }, { -1, g });
	//pCharacterAnimSM->SetState(pCharacterAnimSM->partialAnimationRoot, 1, 10, false);
}

void DemoAnimationCharacter::addDefaultComponents()
{
	Component::addDefaultComponents();

	// custom methods of this component
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, DemoAnimationCharacter::do_UPDATE);
}

void DemoAnimationCharacter::do_UPDATE(PE::Events::Event *pEvt)
{
	debugTimer += 0.3f;
	pCharacterAnimSM->SetBlendWeight(debugIndex, (sinf(debugTimer) * 0.5f + 0.5f) * 0.5f);
	/*
	if (debugTimer > 0.5f)
	{
		//pCharacterAnimSM->SetState(debugIndex, 1, 0.1, true, true);
		debugTimer = 0.0;
	}
	*/
	
	debugWeight += 0.0025f;
	if (debugWeight > 1)
	{
		debugWeight = 0.0f;
			
		pCharacterAnimSM->SetState(debugSMIndex, debugState ? 4.0f : 7.0f, 0.3f);
		debugState = !debugState;
	}

	//pCharacterAnimSM->SetBlendWeight(debugIndex, debugWeight);

}

}; // namespace Components
}; // namespace CharacterControl
