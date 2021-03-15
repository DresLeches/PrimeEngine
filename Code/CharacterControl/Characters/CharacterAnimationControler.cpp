#include "CharacterAnimationControler.h"

#include "PrimeEngine/PrimeEngineIncludes.h"

#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

//#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/RootSceneNode.h"

#include "CharacterAnimationSM.h"

//#include "PrimeEngine/PrimeEngineIncludes.h"
//#include "PrimeEngine/Lua/LuaEnvironment.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;
using namespace CharacterControl::Components;

namespace CharacterControl {
namespace Components {

PE_IMPLEMENT_CLASS1(CharacterAnimationControler, Component);

		
CharacterAnimationControler::CharacterAnimationControler(PE::GameContext & context, PE::MemoryArena arena, PE::Handle hMyself, SceneNode* pSN, int & threadOwnershipMask)
	: Component(context, arena, hMyself)
{
	//add soldier mesh
	PE::Handle hModelRoot("SCENE_NODE", sizeof(SceneNode));
	pModelRoot = new(hModelRoot) SceneNode(*m_pContext, m_arena, hModelRoot);
	pModelRoot->addDefaultComponents();
	pModelRoot->m_base.setPos(Vector3(0, 0, 0));
	pSN->addComponent(hModelRoot);

	// rotation scene node to rotate soldier properly, since soldier from Maya is facing wrong direction
	PE::Handle hRotateSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pRotateSN = new(hRotateSN) SceneNode(*m_pContext, m_arena, hRotateSN);
	pRotateSN->addDefaultComponents();

	pModelRoot->addComponent(hRotateSN);

	//pRotateSN->m_base.turnLeft(3.1415);

	PE::Handle hCharacterAnimSM("CharacterAnimationSM", sizeof(CharacterAnimationSM));
	pCharacterAnimSM = new(hCharacterAnimSM) CharacterAnimationSM(*m_pContext, m_arena, hCharacterAnimSM);
	pCharacterAnimSM->addDefaultComponents();

	PE::Handle hSkeletonInstance("SkeletonInstance", sizeof(SkeletonInstance));
	SkeletonInstance *pSkelInst = new(hSkeletonInstance) SkeletonInstance(*m_pContext, m_arena, hSkeletonInstance, hCharacterAnimSM);
	pSkelInst->addDefaultComponents();
	pSkelInst->initFromFiles("PlayerCharacter_CATRigHub001.skela", "PlayerModel", threadOwnershipMask);
	pSkelInst->setAnimSet("PlayerCharacter_CATRigHub001.animseta", "PlayerModel");

	// add skin to scene node
	pRotateSN->addComponent(hSkeletonInstance);

	//make the body
	PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
	pBodyMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
	pBodyMeshInstance->addDefaultComponents();
	pBodyMeshInstance->initFromFile("Bodyl_mesh.mesha", "PlayerModel", threadOwnershipMask);
	pSkelInst->addComponent(hMeshInstance);

	//make the world view gun
	PE::Handle hMeshInstanceGun("MeshInstance", sizeof(MeshInstance));
	MeshInstance *pMeshInstanceGun = new(hMeshInstanceGun) MeshInstance(*m_pContext, m_arena, hMeshInstanceGun);
	pMeshInstanceGun->addDefaultComponents();
	pMeshInstanceGun->initFromFile("Tommy_gun_mesh.mesha", "PlayerModel", threadOwnershipMask);
	//pSkelInst->addComponent(hMeshInstanceGun);

	PE::Handle hMyGunSN = PE::Handle("SCENE_NODE", sizeof(JointSceneNode));
	JointSceneNode *pGunSN = new(hMyGunSN) JointSceneNode(*m_pContext, m_arena, hMyGunSN, 19);
	pGunSN->addDefaultComponents();
	pGunSN->m_base.importScale(0.225f, 0.225f, 0.225f);
	pGunSN->m_base.setPos(Vector3(-0.47f, 1.2f, 0.09f));
	pGunSN->m_base.rollLeft(3.14159f * 165.0f / 180.0f);
	pGunSN->m_base.turnLeft(3.14159f * 10.0f / 180.0f);
	pGunSN->addComponent(hMeshInstanceGun); // add gun to joint
	pSkelInst->addComponent(hMyGunSN); // add gun scene node to the skin

	//make the model view gun
	PE::Handle hViewMeshInstance("MeshInstance", sizeof(MeshInstance));
	pViewMeshInstance = new(hViewMeshInstance) MeshInstance(*m_pContext, m_arena, hViewMeshInstance);
	pViewMeshInstance->addDefaultComponents();
	pViewMeshInstance->initFromFile("Cerberus00_Fixed.mesha", "Default", threadOwnershipMask);

	PE::Handle hViewModelRotate("SCENE_NODE", sizeof(SceneNode));
	SceneNode* pViewModelRotate = new(hViewModelRotate) SceneNode(*m_pContext, m_arena, hViewModelRotate);
	pViewModelRotate->addDefaultComponents();
	/*
	pViewModelRotate->m_base.importScale(0.25, 0.25, 0.25);
	pViewModelRotate->m_base.setPos(Vector3(0.2f, -0.2f, 0.4f));
	pViewModelRotate->m_base.rollLeft(3.14159f / 2.0f);
	pViewModelRotate->m_base.turnAboutAxis(3.14159f / 2.0f, pSN->m_base.getU());
	*/

	pViewModelRotate->m_base.setPos(Vector3(0.4f, -0.2f, 0.4f));
	pViewModelRotate->m_base.turnLeft(3.14159f / 2.0f);
	pViewModelRotate->m_base.rollLeft(3.14159f / 2.0f);
	pViewModelRotate->m_base.turnAboutAxis(3.14159f / 2.0f, pSN->m_base.getN());
	pViewModelRotate->addComponent(hViewMeshInstance);
	pViewMeshInstance->setEnabled(false); //don't draw it for now

	PE::Handle hViewModelRoot("SCENE_NODE", sizeof(SceneNode));
	pViewModelRoot = new(hViewModelRoot) SceneNode(*m_pContext, m_arena, hViewModelRoot);
	pViewModelRoot->addDefaultComponents();
	pViewModelRoot->addComponent(hViewModelRotate);
	//pViewModelRoot->m_base.importScale(0, 0, 0); //hide it for now

	//setup the animaion tree
	int forward = pCharacterAnimSM->addAnimation(0, FORWARD, PE::LOOPING);
	int back = pCharacterAnimSM->addAnimation(0, BACK, PE::LOOPING);
	aFBSwitch = pCharacterAnimSM->addBlendNode(forward, back, 0.0f);

	int left = pCharacterAnimSM->addAnimation(0, LEFT, PE::LOOPING);
	int right = pCharacterAnimSM->addAnimation(0, RIGHT, PE::LOOPING);
	aLRSwitch = pCharacterAnimSM->addBlendNode(right, left, 0.0f);

	aMoveBlend = pCharacterAnimSM->addBlendNode(aFBSwitch, aLRSwitch, 0.0f, 2.0f);
	int idle = pCharacterAnimSM->addAnimation(0, IDLE, PE::LOOPING);

	pCharacterAnimSM->animationRoot = aStartStop = pCharacterAnimSM->addStateNode({ 0, 1 }, { idle, aMoveBlend });

	//partial animations for flinching
	int g = pCharacterAnimSM->addPartialAnimation(0, DEATH, PE::STAY_ON_ANIMATION_END, 9, 54, 100.0f);
	pCharacterAnimSM->partialAnimationRoot = aFlinchBlend = pCharacterAnimSM->addBlendNode(-1, g, 0.0, 1.0f);
}

void CharacterAnimationControler::do_UPDATE(PE::Events::Event * pEvt)
{
	frameTime  = ((Event_UPDATE*)(pEvt))->m_frameTime;

	//update animation state
	bool isStill = abs(forwardSpeed) < 0.01 && abs(sideSpeed) < 0.01;
	pCharacterAnimSM->SetState(aStartStop, isStill ? 0 : 1, 0.25, 0.5f);    // TODO: Have no idea what this is doing

	pCharacterAnimSM->SetBlendWeight(aFBSwitch, forwardSpeed > 0.0f ? 0.0f : 1.0f);
	pCharacterAnimSM->SetBlendWeight(aLRSwitch, sideSpeed > 0.0f ? 0.0f : 1.0f);

	float mobeBlendWeight = abs(abs(atan2f(forwardSpeed, sideSpeed) * 2.0f / 3.14159255f) - 1.0f);
	pCharacterAnimSM->SetBlendWeight(aMoveBlend, mobeBlendWeight);

	flinchTimer += frameTime * 2 * 3.14159f / 0.5f; //the flinch should last 0.5 seconds
	float flinchWeight = flinchTimer > 3.14159f * 2.0f ? 0.0f : (1.0f - cos(flinchTimer)) * 0.5f / 2.0f;
	pCharacterAnimSM->SetBlendWeight(aFlinchBlend, flinchWeight);
}

void CharacterAnimationControler::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, CharacterAnimationControler::do_UPDATE);
}

void CharacterAnimationControler::SetIsPlayer(SceneNode* pFirstSN)
{
	//put the gun on the camera
	CameraSceneNode *pCamSN = pFirstSN->getFirstComponent<CameraSceneNode>();
	pCamSN->addComponent(pViewModelRoot);

	pModelRoot->m_base.importScale(0, 0, 0); //hide the player model
	pViewMeshInstance->setEnabled(true);
}

void CharacterAnimationControler::Move(const Matrix4x4 &oldMat, const Matrix4x4 &newMat)
{
	Vector3 delta = newMat.getPos() - oldMat.getPos();
	//float blendWeight = pow(0.25, frameTime * 100);
	float blendWeight = 0.5f;
	smoothedVelocity = smoothedVelocity * (1 - blendWeight) + delta * blendWeight;

	forwardSpeed = smoothedVelocity.dotProduct(newMat.getN());
	sideSpeed = smoothedVelocity.dotProduct(newMat.getU());
}

void CharacterAnimationControler::Flinch()
{
	flinchTimer = 0.0f;
}


}; // namespace Components
}; // namespace CharacterControl