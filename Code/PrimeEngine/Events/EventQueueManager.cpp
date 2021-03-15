// APIAbstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes

// Sibling/Children includes
#include "EventQueueManager.h"

namespace PE {
namespace Events {

// Static member variables
Handle EventQueueManager::s_myHandle;


// Constructor -------------------------------------------------------------
EventQueueManager::EventQueueManager(PE::GameContext &context, PE::MemoryArena arena) : m_map(context, arena, 128)
{
	Handle dh = Handle("EVENT_QUEUE", sizeof(EventQueue));
	generalEvtQueue = new(dh) EventQueue();
	m_map.add("general", dh);

	Handle ih = Handle("EVENT_QUEUE", sizeof(EventQueue));
	inputEvtQueue = new(ih) EventQueue();
	m_map.add("input", ih);

	/*
	 * There should be two event queue manager dedicated, one for the server and for the client,
	 * but they're done on one process, and I'm trying to get this work :)
	 */
	// Handles for the servers
	Handle sdh = Handle("EVENT_QUEUE", sizeof(EventQueue));
	serverGeneralEvtQueue = new(sdh) EventQueue();
	m_map.add("serverGeneral", sdh);

	Handle sih = Handle("EVENT_QUEUE", sizeof(EventQueue));
	serverInputEvtQueue = new(sih) EventQueue();
	m_map.add("serverInput", sih);
}

void EventQueueManager::add(Handle hEvt, PrimitiveTypes::UInt32 queueType /*DEF = Events::QT_GENERAL*/)
{
	//Put input events in their own queue
	switch(queueType)
	{
		case Events::QT_INPUT:
		{
			inputEvtQueue->add(hEvt);
			break;

		}
		case Events::QT_GENERAL:
		{
			generalEvtQueue->add(hEvt);
			break;
		}
		case Events::QTS_GENERAL:
		{
			serverGeneralEvtQueue->add(hEvt);
			break;
		}
		case Events::QTS_INPUT:
		{
			serverGeneralEvtQueue->add(hEvt);
		}
		default:
		{
			generalEvtQueue->add(hEvt);
			break;
		}
	}
}
}; // namespace Events
}; // namespace PE
