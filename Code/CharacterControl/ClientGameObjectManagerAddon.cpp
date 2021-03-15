#include "ClientGameObjectManagerAddon.h"

#include "PrimeEngine/PrimeEngineIncludes.h"
#include "PrimeEngine/Networking/Client/ClientNetworkManager.h"

#include "Characters/SoldierNPC.h"
#include "WayPoint.h"
#include "Tank/ClientTank.h"
#include "CharacterControl\ClientPlayer.h"
#include "CharacterControl/Characters/CharacterAnimationControler.h"

#include "CharacterControl/Client/ClientSpaceShip.h"
#include "Characters/DemoAnimationCharacter.h"
#include "PrimeEngine/Scene/SkeletonInstance.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;
using namespace CharacterControl::Components;

namespace CharacterControl{
namespace Components
{
PE_IMPLEMENT_CLASS1(ClientGameObjectManagerAddon, Component); // creates a static handle and GteInstance*() methods. still need to create construct

void ClientGameObjectManagerAddon::addDefaultComponents()
{
	GameObjectManagerAddon::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Event_CreateSoldierNPC, ClientGameObjectManagerAddon::do_CreateSoldierNPC);
	PE_REGISTER_EVENT_HANDLER(Event_CREATE_WAYPOINT, ClientGameObjectManagerAddon::do_CREATE_WAYPOINT);

	// note this component (game obj addon) is added to game object manager after network manager, so network manager will process this event first
	PE_REGISTER_EVENT_HANDLER(PE::Events::Event_SERVER_CLIENT_CONNECTION_ACK, ClientGameObjectManagerAddon::do_SERVER_CLIENT_CONNECTION_ACK);

	PE_REGISTER_EVENT_HANDLER(Event_MoveTank_S_to_C, ClientGameObjectManagerAddon::do_MoveTank);
	PE_REGISTER_EVENT_HANDLER(Event_MovePlayer_S_to_C, ClientGameObjectManagerAddon::do_MovePlayer);
	PE_REGISTER_EVENT_HANDLER(Event_UPDATE, ClientGameObjectManagerAddon::do_UPDATE);
	

	PE_REGISTER_EVENT_HANDLER(Event_CreateDemoAnimationCharacter, ClientGameObjectManagerAddon::do_CREATE_DEMO_ANIMATION_CHARACTER)
}

void ClientGameObjectManagerAddon::do_CreateSoldierNPC(PE::Events::Event *pEvt)
{
	assert(pEvt->isInstanceOf<Event_CreateSoldierNPC>());

	Event_CreateSoldierNPC *pTrueEvent = (Event_CreateSoldierNPC*)(pEvt);

	createSoldierNPC(pTrueEvent);
}

void ClientGameObjectManagerAddon::createSoldierNPC(Vector3 pos, int &threadOwnershipMask)
{
	Event_CreateSoldierNPC evt(threadOwnershipMask);
	evt.m_pos = pos;
	evt.m_u = Vector3(1.0f, 0, 0);
	evt.m_v = Vector3(0, 1.0f, 0);
	evt.m_n = Vector3(0, 0, 1.0f);
	
	StringOps::writeToString( "SoldierTransform.mesha", evt.m_meshFilename, 255);
	StringOps::writeToString( "Soldier", evt.m_package, 255);
	StringOps::writeToString( "mg34.x_mg34main_mesh.mesha", evt.m_gunMeshName, 64);
	StringOps::writeToString( "CharacterControl", evt.m_gunMeshPackage, 64);
	StringOps::writeToString( "", evt.m_patrolWayPoint, 32);
	createSoldierNPC(&evt);
}

void ClientGameObjectManagerAddon::createSoldierNPC(Event_CreateSoldierNPC *pTrueEvent)
{
	PEINFO("CharacterControl: GameObjectManagerAddon: Creating CreateSoldierNPC\n");

	PE::Handle hSoldierNPC("SoldierNPC", sizeof(SoldierNPC));
	SoldierNPC *pSoldierNPC = new(hSoldierNPC) SoldierNPC(*m_pContext, m_arena, hSoldierNPC, pTrueEvent);
	pSoldierNPC->addDefaultComponents();

	// add the soldier as component to the ObjecManagerComponentAddon
	// all objects of this demo live in the ObjecManagerComponentAddon
	addComponent(hSoldierNPC);
}

void ClientGameObjectManagerAddon::do_CREATE_WAYPOINT(PE::Events::Event *pEvt)
{
	PEINFO("GameObjectManagerAddon::do_CREATE_WAYPOINT()\n");

	assert(pEvt->isInstanceOf<Event_CREATE_WAYPOINT>());

	Event_CREATE_WAYPOINT *pTrueEvent = (Event_CREATE_WAYPOINT*)(pEvt);

	PE::Handle hWayPoint("WayPoint", sizeof(WayPoint));
	WayPoint *pWayPoint = new(hWayPoint) WayPoint(*m_pContext, m_arena, hWayPoint, pTrueEvent);
	pWayPoint->addDefaultComponents();

	addComponent(hWayPoint);
}

WayPoint *ClientGameObjectManagerAddon::getWayPoint(const char *name)
{
	PE::Handle *pHC = m_components.getFirstPtr();

	for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
	{
		Component *pC = (*pHC).getObject<Component>();

		if (pC->isInstanceOf<WayPoint>())
		{
			WayPoint *pWP = (WayPoint *)(pC);
			if (StringOps::strcmp(pWP->m_name, name) == 0)
			{
				// equal strings, found our waypoint
				return pWP;
			}
		}
	}
	return NULL;
}


void ClientGameObjectManagerAddon::createTank(int index, int &threadOwnershipMask)
{

	//create hierarchy:
	//scene root
	//  scene node // tracks position/orientation
	//    Tank

	//game object manager
	//  TankController
	//    scene node
	
	PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
	MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);

	pMeshInstance->addDefaultComponents();
	pMeshInstance->initFromFile("kingtiger.x_main_mesh.mesha", "Default", threadOwnershipMask);

	// need to create a scene node for this mesh
	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pSN->addDefaultComponents();

	Vector3 spawnPos(-36.0f + 6.0f * index, 0 , 21.0f);
	pSN->m_base.setPos(spawnPos);
	
	pSN->addComponent(hMeshInstance);
	
	RootSceneNode::Instance()->addComponent(hSN);

	// now add game objects

	PE::Handle hTankController("TankController", sizeof(TankController));
	TankController *pTankController = new(hTankController) TankController(*m_pContext, m_arena, hTankController, 0.05f, spawnPos,  0.05f);
	pTankController->addDefaultComponents();

	addComponent(hTankController);
	
	// add the same scene node to tank controller
	static int alllowedEventsToPropagate[] = {0}; // we will pass empty array as allowed events to propagate so that when we add
	// scene node to the square controller, the square controller doesnt try to handle scene node's events
	// because scene node handles events through scene graph, and is child of square controller just for referencing purposes
	pTankController->addComponent(hSN, &alllowedEventsToPropagate[0]);
}

void ClientGameObjectManagerAddon::createPlayer(int index, int & threadOwnershipMask)
{
	// need to acquire redner context for this code to execute thread-safe
	//m_pContext->getGPUScreen()->AcquireRenderContextOwnership(threadOwnershipMask);

	//mimics creatTank

	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pSN->addDefaultComponents();
	
	//if(this is the current player)
	/*
	{
		PE::Handle hMeshInstanceGun("MeshInstance", sizeof(MeshInstance));
		MeshInstance *pMeshInstanceGun = new(hMeshInstanceGun) MeshInstance(*m_pContext, m_arena, hMeshInstanceGun);

		pMeshInstanceGun->addDefaultComponents();
		pMeshInstanceGun->initFromFile("Cerberus00_Fixed.mesha", "Default", threadOwnershipMask);

		// need to create a scene node for this mesh
		PE::Handle hSN2("SCENE_NODE", sizeof(SceneNode));
		SceneNode *pSN2 = new(hSN2) SceneNode(*m_pContext, m_arena, hSN2);
		pSN2->addDefaultComponents();
		
		Vector3 spawnPos2(0.2f, 1.35f, 0.4f);
		//Vector3 spawnPos2(0.5f, 1.1f, 0.9f);

		pSN2->m_base.setPos(spawnPos2);

		pSN2->m_base.turnLeft(3.14159f / 2.0f);
		pSN2->m_base.rollLeft(3.14159f / 2.0f);
		pSN2->m_base.turnAboutAxis(3.14159f / 2.0f, pSN->m_base.getN());
		pSN2->addComponent(hMeshInstanceGun);
		pSN->addComponent(hSN2);
	}
	*/
	//else
	/*
	{
		//add soldier mesh
		int numskins = 1; // 8
		for (int iSkin = 0; iSkin < numskins; ++iSkin)
		{
			float z = (iSkin / 4) * 1.5f;
			float x = (iSkin % 4) * 1.5f;
			PE::Handle hModelRoot("SCENE_NODE", sizeof(SceneNode));
			SceneNode *pModelRoot = new(hModelRoot) SceneNode(*m_pContext, m_arena, hModelRoot);
			pModelRoot->addDefaultComponents();

			pModelRoot->m_base.setPos(Vector3(x, 0, z));

			// rotation scene node to rotate soldier properly, since soldier from Maya is facing wrong direction
			PE::Handle hRotateSN("SCENE_NODE", sizeof(SceneNode));
			SceneNode *pRotateSN = new(hRotateSN) SceneNode(*m_pContext, m_arena, hRotateSN);
			pRotateSN->addDefaultComponents();

			pModelRoot->addComponent(hRotateSN);

			pRotateSN->m_base.turnLeft(3.1415);

			PE::Handle hCharacterAnimSM("CharacterAnimationSM", sizeof(CharacterAnimationSM));
			CharacterAnimationSM* pCharacterAnimSM = new(hCharacterAnimSM) CharacterAnimationSM(*m_pContext, m_arena, hCharacterAnimSM);
			pCharacterAnimSM->addDefaultComponents();

			//pCharacterAnimSM->m_debugAnimIdOffset = 0;// rand() % 3;

			PE::Handle hSkeletonInstance("SkeletonInstance", sizeof(SkeletonInstance));
			SkeletonInstance *pSkelInst = new(hSkeletonInstance) SkeletonInstance(*m_pContext, m_arena, hSkeletonInstance, hCharacterAnimSM);
			pSkelInst->addDefaultComponents();

			pSkelInst->initFromFiles("soldier_Soldier_Skeleton.skela", "Soldier", threadOwnershipMask);

			pSkelInst->setAnimSet("soldier_Soldier_Skeleton.animseta", "Soldier");

			PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
			MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
			pMeshInstance->addDefaultComponents();

			pMeshInstance->initFromFile("SoldierTransform.mesha", "Soldier", threadOwnershipMask);

			pSkelInst->addComponent(hMeshInstance);

			// add skin to scene node
			pRotateSN->addComponent(hSkeletonInstance);

			//setup the animations
			int STAND_AIM = 1;
			pCharacterAnimSM->animationRoot = pCharacterAnimSM->addAnimation(0, STAND_AIM, PE::LOOPING);

			pSN->addComponent(hModelRoot);

			pSN->m_AABB = pMeshInstance->getFirstParentByTypePtr<Mesh>()->m_AABB;
		}
	}
	*/

	//add the animation controller, sets up hte mesh and animaitons
	PE::Handle hCharacterAnimationControler("CharacterAnimationControler", sizeof(CharacterAnimationControler));
	CharacterAnimationControler* pAnimControler = new(hCharacterAnimationControler) CharacterAnimationControler(*m_pContext, m_arena, hCharacterAnimationControler, pSN, threadOwnershipMask);
	pAnimControler->addDefaultComponents();

	//add the AABB
	pSN->m_AABB = pAnimControler->GetMeshInstance()->getFirstParentByTypePtr<Mesh>()->m_AABB;

	//set the player position
	Vector3 spawnPos(0.2f + 2.0f * index, 1.35f, -20.0f);
	pSN->m_base.setPos(spawnPos);
	pSN->m_base.turnRight(2.61799f);

	//put the player into the world
	RootSceneNode::Instance()->m_playerSNs.push_back(pSN);
	RootSceneNode::Instance()->addComponent(hSN);

	// now add game objects
	PE::Handle hPlayerController("PlayerController", sizeof(PlayerController));
	PlayerController *pPlayerController = new(hPlayerController) PlayerController(*m_pContext, m_arena, hPlayerController, 0.05f, spawnPos, 0.05f);
	pPlayerController->addDefaultComponents();

	addComponent(hPlayerController);

	// add the same scene node to player controller
	static int alllowedEventsToPropagate[] = { 0 }; // we will pass empty array as allowed events to propagate so that when we add
													// scene node to the square controller, the square controller doesnt try to handle scene node's events
													// because scene node handles events through scene graph, and is child of square controller just for referencing purposes
	pPlayerController->addComponent(hSN, &alllowedEventsToPropagate[0]);

	//add the animation controller as a child of the player contoller for referenceing
	pPlayerController->addComponent(hCharacterAnimationControler);

	//m_pContext->getGPUScreen()->ReleaseRenderContextOwnership(threadOwnershipMask);
	pPlayerController->m_playerID = RootSceneNode::Instance()->m_playerSNs.size() - 1;
}

void ClientGameObjectManagerAddon::createSpaceShip(int &threadOwnershipMask)
{

	//create hierarchy:
	//scene root
	//  scene node // tracks position/orientation
	//    SpaceShip

	//game object manager
	//  SpaceShipController
	//    scene node

	PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
	MeshInstance *pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);

	pMeshInstance->addDefaultComponents();
	pMeshInstance->initFromFile("space_frigate_6.mesha", "FregateTest", threadOwnershipMask);

	// need to create a scene node for this mesh
	PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
	SceneNode *pSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
	pSN->addDefaultComponents();

	Vector3 spawnPos(0, 0, 0.0f);
	pSN->m_base.setPos(spawnPos);

	pSN->addComponent(hMeshInstance);

	RootSceneNode::Instance()->addComponent(hSN);

	// now add game objects

	PE::Handle hSpaceShip("ClientSpaceShip", sizeof(ClientSpaceShip));
	ClientSpaceShip *pSpaceShip = new(hSpaceShip) ClientSpaceShip(*m_pContext, m_arena, hSpaceShip, 0.05f, spawnPos,  0.05f);
	pSpaceShip->addDefaultComponents();

	addComponent(hSpaceShip);

	// add the same scene node to tank controller
	static int alllowedEventsToPropagate[] = {0}; // we will pass empty array as allowed events to propagate so that when we add
	// scene node to the square controller, the square controller doesnt try to handle scene node's events
	// because scene node handles events through scene graph, and is child of space ship just for referencing purposes
	pSpaceShip->addComponent(hSN, &alllowedEventsToPropagate[0]);

	pSpaceShip->activate();
}


void ClientGameObjectManagerAddon::do_SERVER_CLIENT_CONNECTION_ACK(PE::Events::Event *pEvt)
{
	Event_SERVER_CLIENT_CONNECTION_ACK *pRealEvt = (Event_SERVER_CLIENT_CONNECTION_ACK *)(pEvt);
	PE::Handle *pHC = m_components.getFirstPtr();

	int itc = 0;
	for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
	{
		Component *pC = (*pHC).getObject<Component>();

		if (pC->isInstanceOf<TankController>())
		{
			if (itc == pRealEvt->m_clientId) //activate tank controller for local client based on local clients id
			{
				TankController *pTK = (TankController *)(pC);
				pTK->activate();
				break;
			}
			++itc;
		}
		if (pC->isInstanceOf<PlayerController>())
		{
			if (itc == pRealEvt->m_clientId) //activate player controller for local client based on local clients id
			{
				PlayerController *pTK = (PlayerController *)(pC);
				pTK->activate();
				break;
			}
			++itc;
		}
	}
}

void ClientGameObjectManagerAddon::do_MovePlayer(PE::Events::Event* pEvt)
{
	assert(pEvt->isInstanceOf<Event_MovePlayer_S_to_C>());

	Event_MovePlayer_S_to_C* pRealEvt = (Event_MovePlayer_S_to_C*)(pEvt);
	PE::Handle* pHC = m_components.getFirstPtr();

	int itc = 0;
	for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++)
	{
		Component *pC = (*pHC).getObject <Component>();

		if (pC->isInstanceOf<PlayerController>())
		{
			if (itc == pRealEvt->m_clientPlayerId) //activate player controller for local client based on local clients id
			{
				PlayerController *pTK = (PlayerController *)(pC);

				// Get the current time
				PE::GameContext* pClient = &PE::Components::ClientGame::s_context;
				float currTime = PlayerController::m_officialGameTime;

				// Measure the RTT latency time and use that to figure out interpolation value
				float rttTime = abs(currTime - pRealEvt->m_receivedTime) + pRealEvt->m_timeStamp;
				float interpVal = abs(rttTime - PE::Components::ClientNetworkManager::rttLatencyTime);
				interpVal = (interpVal > 1.0f) ? 1.0f : 0.0f;
				PE::Components::ClientNetworkManager::rttLatencyTime = rttTime;
				
				// Client's updated position and forward
				Vector3 pos = pRealEvt->m_transform.getPos();
				Vector3 fwd = pRealEvt->m_transform.getN();

				// Check if this is a new player and needs a new 
				auto findPlayer = m_otherPlayerInfo.find(pRealEvt->m_clientPlayerId);
				if (findPlayer == m_otherPlayerInfo.end())
				{
					ClientPlayerInfo tempInfo;
					tempInfo.m_clientId = pRealEvt->m_clientPlayerId;
					tempInfo.m_pos = pos;
					tempInfo.m_fwd = fwd;
					tempInfo.m_lastUpdatedTime = currTime;
					m_otherPlayerInfo[pRealEvt->m_clientPlayerId] = tempInfo;

					pTK->overrideTransform(pRealEvt->m_transform);
					break;
				}
				
				// Client player info
				ClientPlayerInfo& info = m_otherPlayerInfo[pRealEvt->m_clientPlayerId];

				// Check if the other player's position and forward changed, so we don't have to do lag compensation logic
				if (info.m_pos == pos && info.m_fwd == fwd)
				{
					info.m_lastUpdatedTime = currTime;
					break; // No need to update
				}
				else if (info.m_pos == pos && info.m_fwd != fwd)
				{
					info.m_lastUpdatedTime = currTime;
					info.m_fwd = fwd;
					
					ClientPlayerSnapInfo snap;
					snap.m_transform = pRealEvt->m_transform;
					snap.mPlayer = pTK;
					m_receivedTrans.push_back(snap);
					//pTK->overrideTransform(pRealEvt->m_transform);
					//pTK->m_receivedTrans.push_back(pRealEvt->m_transform);
					break;
				}

				/*
				 * Doing lag compensation is a little tricky to do, since the engine only supports
				 * client-side server. Moreover, because of the network latency, a client can be
				 * ahead than other clients if there is enough latency, so I tried to compensate that
				 * by trying to predict where the other client's position might be depending on the
				 * deltaTime.
				 */
				float prevTime = pRealEvt->m_timeStamp;
				float deltaTime = currTime - prevTime;

				/*
				pos.m_x = info.m_pos.m_x * (1 - interpVal) + pos.m_x * interpVal;
				pos.m_y = info.m_pos.m_y * (1 - interpVal) + pos.m_y * interpVal;
				pos.m_z = info.m_pos.m_z * (1 - interpVal) + pos.m_z * interpVal;
				*/

				/*
				// Check if our packet came pretty late (> 10 ms since last update), move the client a little
				// more forward closer to where he would actually be
				if (deltaTime > 3.0f)
				{
					float extraTime = 0.25f;
					Vector3 relVelocity(fwd.m_x * extraTime, fwd.m_y * extraTime, fwd.m_z * extraTime);
					pos.m_x += relVelocity.m_x;
					pos.m_y += relVelocity.m_y;
					pos.m_z += relVelocity.m_z;
				}
				else if (deltaTime > 2.0f)
				{
					float lagBoost = 0.15f;
					Vector3 relVelocity(fwd.m_x * lagBoost, fwd.m_y * lagBoost, fwd.m_z * lagBoost);
					pos.m_x += relVelocity.m_x;
					pos.m_y += relVelocity.m_y;
					pos.m_z += relVelocity.m_z;
				}
				else if (deltaTime > 1.0f)
				{
					float lagBoost = 0.10f;
					Vector3 relVelocity(fwd.m_x * lagBoost, fwd.m_y * lagBoost, fwd.m_z * lagBoost);
					pos.m_x += relVelocity.m_x;
					pos.m_y += relVelocity.m_y;
					pos.m_z += relVelocity.m_z;
				}
				*/

				// Update the player game state
				info.m_pos = pos;
				info.m_fwd = fwd;
				info.m_lastUpdatedTime = currTime;

				// Set the new position
				pRealEvt->m_transform.setPos(pos);

				ClientPlayerSnapInfo snap;
				snap.m_transform = pRealEvt->m_transform;
				snap.mPlayer = pTK;
				m_receivedTrans.push_back(snap);
				//pTK->m_receivedTrans.push_back(pRealEvt->m_transform);
				//pTK->overrideTransform(pRealEvt->m_transform);
				break;
			}
			++itc;
		}
	}

}

void ClientGameObjectManagerAddon::do_UPDATE(PE::Events::Event* pEvt)
{
	PE::Events::Event_UPDATE* pRealEvt = (PE::Events::Event_UPDATE*) pEvt;
	m_moveUpdateTimer += pRealEvt->m_frameTime;
	if (m_moveUpdateTimer >= 0.1f)
	{
		if (m_receivedTrans.size())
		{
			int size = m_receivedTrans.size();
			int ind = (int)(size * 0.005);

			ClientPlayerSnapInfo snap = m_receivedTrans[ind];
			snap.mPlayer->overrideTransform(snap.m_transform);
			m_receivedTrans.clear();
		}
		m_moveUpdateTimer = 0.0f;
	}

	
}

void ClientGameObjectManagerAddon::do_MoveTank(PE::Events::Event *pEvt)
{
	assert(pEvt->isInstanceOf<Event_MoveTank_S_to_C>());

	Event_MoveTank_S_to_C *pTrueEvent = (Event_MoveTank_S_to_C*)(pEvt);

	PE::Handle *pHC = m_components.getFirstPtr();

	int itc = 0;
	for (PrimitiveTypes::UInt32 i = 0; i < m_components.m_size; i++, pHC++) // fast array traversal (increasing ptr)
	{
		Component *pC = (*pHC).getObject<Component>();

		if (pC->isInstanceOf<TankController>())
		{
			if (itc == pTrueEvent->m_clientTankId) //activate tank controller for local client based on local clients id
			{
				TankController *pTK = (TankController *)(pC);
				pTK->overrideTransform(pTrueEvent->m_transform);
				break;
			}
			++itc;
		}
		if (pC->isInstanceOf<PlayerController>())
		{
			if (itc == pTrueEvent->m_clientTankId) //activate player controller for local client based on local clients id
			{
				PlayerController *pTK = (PlayerController *)(pC);
				pTK->overrideTransform(pTrueEvent->m_transform);
				break;
			}
			++itc;
		}
	}
}

void ClientGameObjectManagerAddon::do_CREATE_DEMO_ANIMATION_CHARACTER(PE::Events::Event *pEvt)
{
	PEINFO("GameObjectManagerAddon::do_CREATE_DEMO_ANIMATION_CHARACTER()\n");

	assert(pEvt->isInstanceOf<Event_CreateDemoAnimationCharacter>());

	Event_CreateDemoAnimationCharacter *pTrueEvent = (Event_CreateDemoAnimationCharacter*)(pEvt);

	PE::Handle hChara("DemoAnimationCharacter", sizeof(DemoAnimationCharacter));
	DemoAnimationCharacter *pChara = new(hChara) DemoAnimationCharacter(*m_pContext, m_arena, hChara, pTrueEvent);
	pChara->addDefaultComponents();

	addComponent(hChara);
}

}
}
