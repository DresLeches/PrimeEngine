#ifndef _CLIENTPLAYER_H_
#define _CLIENTPLAYER_H_

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Math/Vector3.h"
#include <vector>

namespace PE {
	namespace Events {
		struct EventQueueManager;
	}
}

namespace CharacterControl {
	namespace Components {

		struct PlayerGameControls : public PE::Components::Component
		{
			PE_DECLARE_CLASS(PlayerGameControls);
		public:

			PlayerGameControls(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself)
				: PE::Components::Component(context, arena, hMyself)
			{
			}

			virtual ~PlayerGameControls() {}
			// Component ------------------------------------------------------------

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
			virtual void do_UPDATE(PE::Events::Event *pEvt);

			virtual void addDefaultComponents();

			//Methods----------------
			void handleIOSDebugInputEvents(PE::Events::Event *pEvt);
			void handleKeyboardDebugInputEvents(PE::Events::Event *pEvt);
			void handleControllerDebugInputEvents(PE::Events::Event *pEvt);

			PE::Events::EventQueueManager *m_pQueueManager;

			PrimitiveTypes::Float32 m_frameTime;
		};

		struct PlayerController : public PE::Components::Component
		{
			// component API
			PE_DECLARE_CLASS(PlayerController);

			PlayerController(PE::GameContext &context, PE::MemoryArena arena,
				PE::Handle myHandle, float speed,
				Vector3 spawnPos, float networkPingInterval); // constructor

			virtual void addDefaultComponents(); // adds default children and event handlers


			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
			virtual void do_UPDATE(PE::Events::Event *pEvt);

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_Player_Move);
			virtual void do_Player_Move(PE::Events::Event *pEvt);

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_Player_Turn);
			virtual void do_Player_Turn(PE::Events::Event *pEvt);

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_Player_Shoot);
			virtual void do_Player_Shoot(PE::Events::Event *pEvt);

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_Player_Hurt);
			virtual void do_Player_Hurt(PE::Events::Event *pEvt);




			void overrideTransform(Matrix4x4 &t);
			void activate();

			float m_timeSpeed;
			float m_time;
			float m_networkPingTimer;
			float m_networkPingInterval;
			Vector2 m_center;
			PrimitiveTypes::UInt32 m_counter;
			Vector3 m_spawnPos;
			bool m_active;
			bool m_overriden;
			bool m_canShoot;
			Matrix4x4 m_transformOverride;
			PE::Components::SphereCollider m_SphereCollider;
			float m_health;
			float m_damage;
			Vector3 m_velocity;
			int m_playerID;
			bool m_canJump;
			int m_score;
			//std::vector<Matrix4x4> m_receivedTrans;
			//PE::Components::AABB m_hitCollider;

			static float m_officialGameTime; // Used when the game gets started
			static bool m_gameStarted;
			//float m_moveUpdateTimer = 0.0f;

		};
	}; // namespace Components
}; // namespace CharacterControl

#endif
