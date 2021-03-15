#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "CharacterAnimationSM.h"

#include "PrimeEngine/FileSystem/FileReader.h"
#include "PrimeEngine/APIAbstraction/GPUMaterial/GPUMaterialSet.h"
#include "PrimeEngine/APIAbstraction/Texture/Texture.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/Geometry/SkeletonCPU/AnimationSetCPU.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "PrimeEngine/Scene/Skeleton.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/VertexBufferGPUManager.h"
#include "PrimeEngine/Scene/DebugRenderer.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Geometry/SkeletonCPU/SkeletonCPU.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/AnimSetBufferGPU.h"

using namespace PE;
using namespace PE::Components;
using namespace PE::Events;

namespace CharacterControl {

namespace Components {

PE_IMPLEMENT_CLASS1(CharacterAnimationSM, DefaultAnimationSM);

CharacterAnimationSM::CharacterAnimationSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself)
	: DefaultAnimationSM(context, arena, hMyself),
	m_AnimationNodes(context, arena, 1)
{
	animationRoot = -1;
	partialAnimationRoot = -1;
}

void CharacterAnimationSM::addDefaultComponents()
{
	DefaultAnimationSM::addDefaultComponents(); //want to change some events so don't use direct inhertance here
}

void CharacterAnimationSM::AdvanceAnimation(AnimationSlot& slot, float frameTime)
{
	Handle hParentSkinInstance = getFirstParentByType<SkeletonInstance>();
	PEASSERT(hParentSkinInstance.isValid(), "SM has to belong to skeleton instance");

	SkeletonInstance *pSkelInstance = hParentSkinInstance.getObject<SkeletonInstance>();
	//Skeleton *pSkeleton = pSkelInstance->getFirstParentByTypePtr<Skeleton>();

	//SkeletonCPU *pSkelCPU = pSkeleton->m_hSkeletonCPU.getObject<SkeletonCPU>();
	/*
	if (m_curPalette.m_size == 0)
	{
		m_curPalette.reset(pSkelCPU->m_numJoints);
		m_modelSpacePalette.reset(pSkelCPU->m_numJoints);
		m_curPalette.m_size = m_modelSpacePalette.m_size = pSkelCPU->m_numJoints;
	}
	*/

	//m_instanceCSJobIndex = -1; // reset this value
							   //dbg
							   //SkeletonCPU *pSkelCPU = pParentSkin->m_hSkeletonCPU.getObject<SkeletonCPU>();
	Array<Handle> &hAnimSets = pSkelInstance->m_hAnimationSetGPUs;

	if (hAnimSets.m_size == 0)
		return;

	//AnimationSlot &slot = m_animSlots[iSlot];
	if (!(slot.m_flags & ACTIVE))
	{
		return;
	}

	PrimitiveTypes::UInt32 animIndex = slot.m_animationIndex;
	PrimitiveTypes::Float32 frame = slot.m_frameIndex;

	PrimitiveTypes::Float32 dFrame = frameTime / 0.03333333333f; // pRealEvent->m_frameTime / 0.0333333f;

	// could scale speed here..
	//dFrame *= scale factor

	frame += dFrame;
	PrimitiveTypes::Float32 framesLeft = slot.m_framesLeft;
	framesLeft -= dFrame;

	AnimSetBufferGPU *pAnimSetBufferGPU = hAnimSets[slot.m_animationSetIndex].getObject<AnimSetBufferGPU>();
	AnimationSetCPU *pAnimSetCPU = pAnimSetBufferGPU->m_hAnimationSetCPU.getObject<AnimationSetCPU>();

	AnimationCPU &anim = pAnimSetCPU->m_animations[animIndex];

	bool doFrameCalculations = false;
	// if looping make sure that the last frame plays deserved time, i.e. 31 frames play frame_time * 31 where last frame is blended with 0th frame
	//if (frame >= (PrimitiveTypes::Float32)(anim.m_frames.m_size - (slot.m_looping ? 0 : 1)))

	if (framesLeft < 0.0f)
	{
		if (slot.isLooping() || slot.needToStayOnLastFrame())
		{
			if (slot.isLooping())
			{
				while (framesLeft < 0)
				{
					framesLeft += slot.m_numFrames;
					frame -= slot.m_numFrames;
				}
			}
			else
			{
				framesLeft = 0.5f;
				frame = slot.m_numFrames - 1.0f;
			}
			doFrameCalculations = true;
		}
		else
		{
			// end of animtion
			slot.setNotActive();
			if (slot.needToNotifyOnAnimationEnd())
			{
				Event_ANIMATION_ENDED evt;
				evt.m_hModel = this->m_hMyself;
				evt.m_animIndex = animIndex;
				Handle hComponentParent = getFirstParentByType<SceneNode>();
				if (hComponentParent.isValid())
				{
					hComponentParent.getObject<Component>()->handleEvent(&evt);
				}
				if (slot.m_framesLeft < 0)
				{
					// still < 0 -> nothing was done in the event handler
				}
				else
				{
					// the animation slot was reset -> TODO: need to apply the time difference
					doFrameCalculations = true;
					frame = slot.m_frameIndex;
					framesLeft = slot.m_framesLeft;
				}
			}
		}
	}
	else
	{
		/*
		if (slot.isFadingAway())
		{
			slot.m_weight -= slot.m_weight * (dFrame / slot.m_framesLeft);
		}
		else */
		if (slot.needToNotifyOnAnimationEnd() && slot.m_framesLeft < slot.m_numFramesToNotifyBeforeEnd)
		{
			Event_ANIMATION_ENDED evt;
			evt.m_hModel = this->m_hMyself;
			evt.m_animIndex = animIndex;

			Handle hComponentParent = getFirstParentByType<SceneNode>();
			if (hComponentParent.isValid())
			{
				hComponentParent.getObject<Component>()->handleEvent(&evt);
			}
			if (slot.m_framesLeft < slot.m_numFramesToNotifyBeforeEnd)
			{
				// still < 0 -> nothing was done in the event handler
			}
			else
			{
				// the animation slot was reset -> TODO: need to apply the time difference
				doFrameCalculations = true;
				frame = slot.m_frameIndex;
				framesLeft = slot.m_framesLeft;
			}

		}

		doFrameCalculations = true;
	}

	if (doFrameCalculations)
	{
		slot.m_frameIndex = frame;
		slot.m_framesLeft = framesLeft;
		PrimitiveTypes::Float32 fframeIndex = floor(slot.m_frameIndex);
		PrimitiveTypes::Float32 alpha = slot.m_frameIndex - fframeIndex;
		PrimitiveTypes::UInt32 frameIndex0 = (PrimitiveTypes::UInt32)(fframeIndex);
		while (frameIndex0 >= anim.m_frames.m_size)
			frameIndex0 -= anim.m_frames.m_size;

		PrimitiveTypes::UInt32 frameIndex1 = frameIndex0 + 1 < anim.m_frames.m_size ? frameIndex0 + 1 : 0;
		slot.m_iFrameIndex0 = frameIndex0;
		slot.m_iFrameIndex1 = frameIndex1;
		slot.m_blendFactor = alpha;

		if (frameIndex0 >= anim.m_frames.m_size)
		{
			assert(0);
		}
	}
}

AnimationSlot CharacterAnimationSM::getAnimation(PrimitiveTypes::UInt32 animationSetIndex, PrimitiveTypes::UInt32 animationIndex, PrimitiveTypes::UInt32 additionalFlags,
	PrimitiveTypes::UInt32 startJoint, PrimitiveTypes::UInt32 endJoint, bool isPartial)
{
	Handle hParentSkinInstance = getFirstParentByType<SkeletonInstance>();
	PEASSERT(hParentSkinInstance.isValid(), "SM has to belong to skeleton instance");
	SkeletonInstance *pSkelInstance = hParentSkinInstance.getObject<SkeletonInstance>();
	AnimSetBufferGPU *pAnimSetBufferGPU = pSkelInstance->m_hAnimationSetGPUs[animationSetIndex].getObject<AnimSetBufferGPU>();
	AnimationSetCPU *pAnimSet = pAnimSetBufferGPU->m_hAnimationSetCPU.getObject<AnimationSetCPU>();
	AnimationCPU &anim = pAnimSet->m_animations[(animationIndex + m_debugAnimIdOffset)];

	if (isPartial)
	{
		additionalFlags |= PARTIAL_BODY_ANIMATION;
	}
	else
	{
		startJoint = anim.m_startJoint;
		endJoint = anim.m_endJoint;
	}

	return AnimationSlot(animationSetIndex, (animationIndex + m_debugAnimIdOffset), 0, (PrimitiveTypes::Float32)(anim.m_frames.m_size - 1),
						startJoint, endJoint, ACTIVE | additionalFlags /*| PARTIAL_BODY_ANIMATION | NOTIFY_ON_ANIMATION_END*/, 1.0);
}

int CharacterAnimationSM::addAnimation(PrimitiveTypes::UInt32 animationSetIndex, PrimitiveTypes::UInt32 animationIndex, PrimitiveTypes::UInt32 additionalFlags, float defaultSpeed)
{
	CharacterAnimationSM::AnimationWrapper* p = new CharacterAnimationSM::AnimationWrapper(getAnimation(animationSetIndex, animationIndex, additionalFlags), defaultSpeed);
	m_AnimationNodes.add(p);
	return m_AnimationNodes.m_size - 1;
}

int CharacterAnimationSM::addPartialAnimation(PrimitiveTypes::UInt32 animationSetIndex, PrimitiveTypes::UInt32 animationIndex,PrimitiveTypes::UInt32 additionalFlags,
												PrimitiveTypes::UInt32 startJoint, PrimitiveTypes::UInt32 endJoint, float defaultSpeed)
{
	CharacterAnimationSM::AnimationWrapper* p = new CharacterAnimationSM::AnimationWrapper(getAnimation(animationSetIndex, animationIndex, additionalFlags, startJoint, endJoint, true), defaultSpeed);
	m_AnimationNodes.add(p);
	return m_AnimationNodes.m_size - 1;
}

int CharacterAnimationSM::addBlendNode( PrimitiveTypes::UInt32 nodeAIndex, PrimitiveTypes::UInt32 nodeBIndex, float defaultBlenAmmount, float defaultSpeed)
{
	/*
	//if the indecies provided are invalid
	if (nodeAIndex < 0 || nodeAIndex >= m_AnimationNodes.m_size || nodeBIndex < 0 || nodeBIndex >= m_AnimationNodes.m_size)
	{
		return -1; //don't add anything and return
	}
	*/

	AnimationNode* a = (nodeAIndex < 0 || nodeAIndex >= m_AnimationNodes.m_size) ? nullptr : m_AnimationNodes[nodeAIndex];
	AnimationNode* b = (nodeBIndex < 0 || nodeBIndex >= m_AnimationNodes.m_size) ? nullptr : m_AnimationNodes[nodeBIndex];

	CharacterAnimationSM::AnimationBlendNode* p = new AnimationBlendNode(a, b, defaultBlenAmmount, defaultSpeed);
	m_AnimationNodes.add(p);
	return m_AnimationNodes.m_size - 1;
}

// TODO HACK FIXME: maybe redo how the state nodes are initialized
int CharacterAnimationSM::addStateNode(const std::list<int> & stateKeys, const std::list<int> & stateIndices, float defaultSpeed)
{
	if (stateKeys.size() < 1 || stateKeys.size() != stateIndices.size())
	{
		return -1;
	}

	//create the state node
	int index = stateIndices.front();
	//if (index < 0 || index >= m_AnimationNodes.m_size) { return -1; } //return if the index is invalide
	AnimationNode* state = (index < 0 || index >= m_AnimationNodes.m_size) ? nullptr : m_AnimationNodes[stateIndices.front()];
	CharacterAnimationSM::AnimationStateNode* p = new AnimationStateNode(stateKeys.front(), state, defaultSpeed);

	std::list<int>::const_iterator itKey = stateKeys.begin()++; //start at the seccond element
	std::list<int>::const_iterator itIdx = stateIndices.begin()++;

	//add every other state
	while (itKey != stateKeys.end()) {
		index = *itIdx;

		/*
		//if the index is invalid cancel
		if (index < 0 || index >= m_AnimationNodes.m_size)
		{
			delete(p);
			return -1;
		}
		*/

		state = (index < 0 || index >= m_AnimationNodes.m_size) ? nullptr : m_AnimationNodes[index];

		p->AddState(*itKey, state);

		itKey++;
		itIdx++;
	}

	m_AnimationNodes.add(p);
	return m_AnimationNodes.m_size - 1;
}

void CharacterAnimationSM::SetPlaybackSpeed(int nodeIndex, float speed)
{
	m_AnimationNodes[nodeIndex]->playbackSpeed = speed;
}

void CharacterAnimationSM::SetBlendWeight(int nodeIndex, float weight)
{
	CharacterAnimationSM::AnimationBlendNode* p = (CharacterAnimationSM::AnimationBlendNode*) m_AnimationNodes[nodeIndex];
	p->alpha = weight;
}

void CharacterAnimationSM::SetState(int nodeIndex, int stateKey, float transitionTime, bool interrupt, bool forceRestartAnimation)
{
	CharacterAnimationSM::AnimationStateNode* p = (CharacterAnimationSM::AnimationStateNode*) m_AnimationNodes[nodeIndex];
	p->ChangeState(stateKey, transitionTime, interrupt, forceRestartAnimation);
}

/*
AnimationSlot *CharacterAnimationSM::setAnimationSimple(
	PrimitiveTypes::UInt32 animationSetIndex,
	PrimitiveTypes::UInt32 animationIndex,
	PrimitiveTypes::UInt32 slotIndex,
	PrimitiveTypes::UInt32 additionalFlags,
	PrimitiveTypes::Float32 weight)
{
	Handle hParentSkinInstance = getFirstParentByType<SkeletonInstance>();
	PEASSERT(hParentSkinInstance.isValid(), "SM has to belong to skeleton instance");

	SkeletonInstance *pSkelInstance = hParentSkinInstance.getObject<SkeletonInstance>();
	Skeleton *pSkeleton = pSkelInstance->getFirstParentByTypePtr<Skeleton>();

	SkeletonCPU *pSkelCPU = pSkeleton->m_hSkeletonCPU.getObject<SkeletonCPU>();

	AnimSetBufferGPU *pAnimSetBufferGPU = pSkelInstance->m_hAnimationSetGPUs[animationSetIndex].getObject<AnimSetBufferGPU>();
	AnimationSetCPU *pAnimSet = pAnimSetBufferGPU->m_hAnimationSetCPU.getObject<AnimationSetCPU>();
	AnimationCPU &anim = pAnimSet->m_animations[(animationIndex + m_debugAnimIdOffset)];
	AnimationSlot slot(animationSetIndex, (animationIndex + m_debugAnimIdOffset), 0, (PrimitiveTypes::Float32)(anim.m_frames.m_size - 1), anim.m_startJoint, anim.m_endJoint, ACTIVE | additionalFlags, weight);
	setSlot(slotIndex, slot);
	return &m_animSlots[slotIndex];
}
*/

void CharacterAnimationSM::do_SCENE_GRAPH_UPDATE(PE::Events::Event * pEvt)
{
	PE::Events::Event_UPDATE *pRealEvent = (PE::Events::Event_UPDATE*)(pEvt);
	Handle hParentSkinInstance = getFirstParentByType<SkeletonInstance>();
	PEASSERT(hParentSkinInstance.isValid(), "SM has to belong to skeleton instance");

	SkeletonInstance *pSkelInstance = hParentSkinInstance.getObject<SkeletonInstance>();
	Skeleton *pSkeleton = pSkelInstance->getFirstParentByTypePtr<Skeleton>();

	SkeletonCPU *pSkelCPU = pSkeleton->m_hSkeletonCPU.getObject<SkeletonCPU>();
	if (m_curPalette.m_size == 0)
	{
		m_curPalette.reset(pSkelCPU->m_numJoints);
		m_modelSpacePalette.reset(pSkelCPU->m_numJoints);
		m_curPalette.m_size = m_modelSpacePalette.m_size = pSkelCPU->m_numJoints;
	}

	m_instanceCSJobIndex = -1; // reset this value

	m_animSlots.reset(32);
	//DefaultAnimationSM::do_SCENE_GRAPH_UPDATE(pEvt);

	if (animationRoot >= 0 && animationRoot < m_AnimationNodes.m_size)
	{
		m_AnimationNodes[animationRoot]->Update(*this, pRealEvent->m_frameTime);
	}

	if (partialAnimationRoot >= 0 && partialAnimationRoot < m_AnimationNodes.m_size)
	{
		m_AnimationNodes[partialAnimationRoot]->Update(*this, pRealEvent->m_frameTime);
	}
}

};
};