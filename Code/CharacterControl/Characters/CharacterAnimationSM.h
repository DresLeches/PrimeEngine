#pragma once

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Scene/DefaultAnimationSM.h"

#include "../Events/Events.h"

#include "PrimeEngine/Geometry/SkeletonCPU/AnimationSetCPU.h"

#include <map>
#include <list>

using namespace PE;

namespace CharacterControl {
namespace Components {

struct CharacterAnimationSM : public PE::Components::DefaultAnimationSM
{
public:
	PE_DECLARE_CLASS(CharacterAnimationSM);

	CharacterAnimationSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself);

	// event handling
	virtual void addDefaultComponents();

	void do_SCENE_GRAPH_UPDATE(PE::Events::Event *pEvt) override;

	int addAnimation(PrimitiveTypes::UInt32 animationSetIndex, PrimitiveTypes::UInt32 animationIndex, PrimitiveTypes::UInt32 additionalFlags, float defaultSpeed = 1.0f);
	int CharacterAnimationSM::addPartialAnimation(PrimitiveTypes::UInt32 animationSetIndex, PrimitiveTypes::UInt32 animationIndex, PrimitiveTypes::UInt32 additionalFlags,
													PrimitiveTypes::UInt32 startJoint, PrimitiveTypes::UInt32 endJoint, float defaultSpeed = 1.0f);
	int addBlendNode( PrimitiveTypes::UInt32 nodeAIndex, PrimitiveTypes::UInt32 nodeBIndex, float defaultBlenAmmount = 1.0f, float defaultSpeed = 1.0f);
	int addStateNode(const std::list<int> & stateKeys, const std::list<int> & stateIndices, float defaultSpeed = 1.0);

	void SetPlaybackSpeed(int nodeIndex, float speed);
	void SetBlendWeight(int nodeIndex, float weight);
	void SetState(int nodeIndex, int stateKey, float transitionTime = -1, bool interrupt = true, bool forceRestartAnimation = false);

    PrimitiveTypes::UInt32 animationRoot;
    PrimitiveTypes::UInt32 partialAnimationRoot;


private:
	void AdvanceAnimation(AnimationSlot& slot, float frameTime);
	AnimationSlot getAnimation(PrimitiveTypes::UInt32 animationSetIndex, PrimitiveTypes::UInt32 animationIndex, PrimitiveTypes::UInt32 additionalFlags,
		PrimitiveTypes::UInt32 startJoint = 0, PrimitiveTypes::UInt32 endJoint = 0, bool isPartial = false);

	//pure abstract struct which represents a single node in our animation FSM
	struct AnimationNode
	{
		float playbackSpeed = 1.0f; //each node has a playback speed which propegates to its children

		AnimationNode(float speed) : playbackSpeed(speed) {}

		virtual void Update(CharacterAnimationSM& manager, float deltaTime, float totalWeight = 1.0f, float totalSpeed = 1.0f) = 0;
		virtual void Restart() = 0;
	};

	//wrapper which stores animation
	struct AnimationWrapper : public AnimationNode
	{
	private:
		AnimationSlot anim;

	public:
		AnimationWrapper(AnimationSlot animation, float speed)
			: AnimationNode(speed)
		{
			anim = animation;
		}

		void Update(CharacterAnimationSM& manager, float deltaTime, float totalWeight = 1.0f, float totalSpeed = 1.0f) override
		{
			
			anim.m_weight = totalWeight;
			manager.AdvanceAnimation(anim, deltaTime * totalSpeed * playbackSpeed);

			//only add animations that will actually contribute
			if (anim.isActive() && totalWeight > 0)
			{
				manager.m_animSlots.add(anim);
			}
		}

		void Restart() override
		{
			//restart the animation
			anim.m_frameIndex = 0.0f;
			anim.m_framesLeft = anim.m_numFrames;
			anim.m_iFrameIndex0 = 0;
			anim.m_iFrameIndex1 = 0;
			anim.m_blendFactor = 0;
			anim.m_flags |= PE::ACTIVE;
		}
	};

	//blends between two subtrees of animation
	struct AnimationBlendNode : public AnimationNode
	{
	private:
		AnimationNode* animA;
		AnimationNode* animB;

	public:
		float alpha;

		AnimationBlendNode(AnimationNode* A, AnimationNode* B, float blend, float speed)
			: AnimationNode(speed)
		{
			animA = A;
			animB = B;
			alpha = blend;
		}

		void Update(CharacterAnimationSM& manager, float deltaTime, float totalWeight = 1.0f, float totalSpeed = 1.0f) override
		{
			if (animA != nullptr) { animA->Update(manager, deltaTime, (1.0f - alpha) * totalWeight, totalSpeed * playbackSpeed); }
			if (animB != nullptr) { animB->Update(manager, deltaTime, alpha * totalWeight, totalSpeed * playbackSpeed); }
		}

		void Restart() override
		{
			if (animA != nullptr) { animA->Restart(); }
			if (animB != nullptr) { animB->Restart(); }
		}
	};

	
	struct AnimationStateNode : public AnimationNode
	{
	private:
		std::map<int, AnimationNode*> states;
		int currentState;
		int oldState;
		float progress;
		float transitionTime;

	public:
		AnimationStateNode(int key, AnimationNode* state, float speed = 1.0)
			: AnimationNode(speed)
		{
			currentState = key;
			oldState = key;

			//add all the states to the map
			states.emplace(key, state);
		}

		bool AddState(int key, AnimationNode* state)
		{
			return states.emplace(key, state).second;
		}

		void Update(CharacterAnimationSM& manager, float deltaTime, float totalWeight = 1.0f, float totalSpeed = 1.0f) override
		{
			if (currentState == oldState) //if we are not transitioning states
			{
				if (states.at(currentState) != nullptr)
				{
					states.at(currentState)->Update(manager, deltaTime, totalWeight, totalSpeed * playbackSpeed); //update the current state
				}
				
				return;
			}

			progress += deltaTime; //progress with the transition

			if (progress >= transitionTime) //if we finished
			{
				oldState = currentState; //stop transitioning
				if (states.at(currentState) != nullptr)
				{
					states.at(currentState)->Update(manager, deltaTime, totalWeight, totalSpeed * playbackSpeed); //update the current state
				}
				return;
			}

			float percentage = progress / transitionTime; //get how far into the transition we are

			//blend between the old and new states based on the progress to transition
			if (states.at(currentState) != nullptr)
			{
				states.at(currentState)->Update(manager, deltaTime, percentage * totalWeight, totalSpeed * playbackSpeed);
			}

			if (states.at(oldState) != nullptr)
			{
				states.at(oldState)->Update(manager, deltaTime, (1.0f - percentage) * totalWeight, totalSpeed * playbackSpeed);
			}
		}

		void Restart() override
		{
			oldState = currentState; //don't resume any transitions
			if (states.at(currentState) != nullptr) { states.at(currentState)->Restart(); }//restart the state we are in
		}

		void ChangeState(int newState, float time, bool interrupt, bool forceRestartAnimation)
		{
			//don't transition to invalid states and don't interrupt the current transition if we aren't allowed to
			if (states.find(newState) == states.end() || (!interrupt && currentState != oldState)) { return; }

			if (currentState == newState) //if we were already transitioning into this state
			{
				if (forceRestartAnimation)
				{
					progress = 0;
					if (states.at(currentState) != nullptr) { states.at(currentState)->Restart(); }
				}
				else
				{
					progress = (progress / transitionTime) * time; //maintain the percentage of progress
				}
				
			}
			else if (oldState == newState) //if we are returning to the state we are transitioning from
			{
				if (forceRestartAnimation)
				{
					progress = 0;
					if (states.at(oldState) != nullptr) { states.at(currentState)->Restart(); }
				}
				else
				{
					progress = ((transitionTime - progress) / transitionTime) * time; //maintian precentage of progress inverted
				}
			}
			else //if we are going to a state not involved in our current transition
			{
				//start the animation for the state we are entering from the beginning
				progress = 0;
				if (states.at(currentState) != nullptr) { states.at(currentState)->Restart(); }
			}

			//update the states and transition time
			oldState = currentState;
			currentState = newState;
			transitionTime = time;
		}
	};

	Array<AnimationNode*, 1> m_AnimationNodes;
	
	/*
	AnimationSlot* setAnimationSimple(
		PrimitiveTypes::UInt32 animationSetIndex,
		PrimitiveTypes::UInt32 animationIndex,
		PrimitiveTypes::UInt32 slotIndex,
		PrimitiveTypes::UInt32 additionalFlags = 0,
		PrimitiveTypes::Float32 weight = 1.0f);
		*/
};

};
};

