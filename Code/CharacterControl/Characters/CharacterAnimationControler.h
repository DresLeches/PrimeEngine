#pragma once
//class CharacterAnimationControler

#include "PrimeEngine/Events/Component.h"
#include "CharacterAnimationSM.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Scene/MeshInstance.h"

using namespace PE::Components;
using namespace PE::Events;

namespace CharacterControl {
namespace Components {

struct CharacterAnimationControler : public PE::Components::Component
{
private:
	CharacterAnimationSM* pCharacterAnimSM;
	SceneNode *pModelRoot;
	SceneNode *pViewModelRoot;
	MeshInstance *pBodyMeshInstance;
	MeshInstance *pViewMeshInstance;

	int aFBSwitch;
	int aLRSwitch;
	int aMoveBlend;
	int aStartStop;
	int aFlinchBlend;

	float forwardSpeed;
	float sideSpeed;
	int stopCounter = 0;
	float frameTime = 1;
	Vector3 smoothedVelocity;

	float flinchTimer = 1000.0f;

public:

	PE_DECLARE_CLASS(CharacterAnimationControler);

	CharacterAnimationControler(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, SceneNode* pSN, int & threadOwnershipMask);

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
	virtual void do_UPDATE(PE::Events::Event *pEvt);

	virtual void addDefaultComponents();
	MeshInstance* GetMeshInstance() { return pBodyMeshInstance; }

	void SetIsPlayer(SceneNode* pFirstSN);
	void Move(const Matrix4x4 &oldMat, const Matrix4x4 &newMat);
	void Flinch();
	
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

};

}; // namespace Components
}; // namespace CharacterControl
