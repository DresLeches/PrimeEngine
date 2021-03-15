// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#if APIABSTRACTION_D3D9 | APIABSTRACTION_D3D11 | (APIABSTRACTION_OGL && !defined(SN_TARGET_PS3))

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/Events/StandardKeyboardEvents.h"
#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/Render/D3D9Renderer.h"
#include "../../../Lua/LuaEnvironment.h"
// Sibling/Children includes
#include "DX9_KeyboardMouse.h"
#include "PrimeEngine/Application/WinApplication.h"

namespace PE {
using namespace Events;
namespace Components {

PE_IMPLEMENT_CLASS1(DX9_KeyboardMouse, Component);

void DX9_KeyboardMouse::addDefaultComponents()
{
	Component::addDefaultComponents();
	PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, DX9_KeyboardMouse::do_UPDATE);
}

void DX9_KeyboardMouse::do_UPDATE(Events::Event *pEvt)
{
	
	m_pQueueManager = Events::EventQueueManager::Instance();

	generateButtonEvents();
}

void DX9_KeyboardMouse::generateButtonEvents()
{
#if PE_PLAT_IS_WIN32
	WinApplication *pWinApp = static_cast<WinApplication*>(m_pContext->getApplication());
	if(GetFocus() == pWinApp->getWindowHandle())
#endif
	{
		//lock mouse to screen
		/*
		HWND hwndMain = pWinApp->getWindowHandle();
		SetCapture(hwndMain);

		
		HDC hdc;                       // handle to device context
		RECT rcClient;                 // client area rectangle
		POINT ptClientUL;              // client upper left corner
		POINT ptClientLR;              // client lower right corner
		//static POINTS ptsBegin;        // beginning point
		//static POINTS ptsEnd;          // new endpoint
		//static POINTS ptsPrevEnd;      // previous endpoint
		//static BOOL fPrevLine = FALSE; // previous line flag
		//HWND hwndMain = pWinApp->getWindowHandle();
		// Capture mouse input.

		SetCapture(hwndMain);

		// Retrieve the screen coordinates of the client area,
		// and convert them into client coordinates.

		GetClientRect(hwndMain, &rcClient);
		ptClientUL.x = rcClient.left;
		ptClientUL.y = rcClient.top;

		// Add one to the right and bottom sides, because the
		// coordinates retrieved by GetClientRect do not
		// include the far left and lowermost pixels.

		ptClientLR.x = rcClient.right + 1;
		ptClientLR.y = rcClient.bottom + 1;
		ClientToScreen(hwndMain, &ptClientUL);
		ClientToScreen(hwndMain, &ptClientLR);

		// Copy the client coordinates of the client area
		// to the rcClient structure. Confine the mouse cursor
		// to the client area by passing the rcClient structure
		// to the ClipCursor function.

		SetRect(&rcClient, ptClientUL.x, ptClientUL.y,
		ptClientLR.x, ptClientLR.y);
		ClipCursor(&rcClient);
		*/
		 
		//end screen lock


		//Check for Button Down events

		//Check for Button Up events
		
		//Check for Button Held events
		if(GetAsyncKeyState('A') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_A_HELD));
			new (h) Event_KEY_A_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('S') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_S_HELD));
			new (h) Event_KEY_S_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('D') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_D_HELD));
			new (h) Event_KEY_D_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('W') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_W_HELD));
			new (h) Event_KEY_W_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState(VK_LEFT) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_LEFT_HELD));
			new (h) Event_KEY_LEFT_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState(VK_DOWN) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_DOWN_HELD));
			new (h) Event_KEY_DOWN_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_RIGHT_HELD));
			new (h) Event_KEY_RIGHT_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState(VK_UP) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_UP_HELD));
			new (h) Event_KEY_UP_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if (GetAsyncKeyState(VK_SPACE) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_SPACEBAR_HELD));
			new (h) Event_SPACEBAR_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_MOUSE_LBUTTON_HELD));
			new (h) Event_MOUSE_LBUTTON_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		POINT mMove;
		if (GetCursorPos(&mMove))
		{
			Handle h("EVENT", sizeof(Event_MOUSE_ROTATE));
			new (h) Event_MOUSE_ROTATE;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}

		if(GetAsyncKeyState(',') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_COMMA_HELD));
			new (h) Event_KEY_COMMA_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('.') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_PERIOD_HELD));
			new (h) Event_KEY_PERIOD_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('K') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_K_HELD));
			new (h) Event_KEY_K_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if (GetAsyncKeyState('G') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_G_HELD));
			new (h) Event_KEY_G_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		// Hacked the network keybinds here cuz we don't got time
		if (GetAsyncKeyState('L') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_L_HELD));
			new (h) Event_KEY_L_HELD;
			m_pQueueManager->add(h, Events::QTS_GENERAL);
		}
		if (GetAsyncKeyState('M') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_M_HELD));
			new (h) Event_KEY_M_HELD;
			m_pQueueManager->add(h, Events::QTS_GENERAL);
		}
		if (GetAsyncKeyState('P') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_P_HELD));
			new (h) Event_KEY_P_HELD;
			m_pQueueManager->add(h, Events::QTS_GENERAL);
		}
		if (GetAsyncKeyState('O') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_O_HELD));
			new (h) Event_KEY_O_HELD;
			m_pQueueManager->add(h, Events::QTS_GENERAL);
		}
		if (GetAsyncKeyState('C') & 0x8000)
		{
			Handle h("Event", sizeof(Event_KEY_C_HELD));
			new (h) Event_KEY_C_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if (GetAsyncKeyState('Z') & 0x8000)
		{
			Handle h("Event", sizeof(Event_KEY_Z_HELD));
			new (h) Event_KEY_Z_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
	}
}

}; // namespace Components
}; // namespace PE

#endif // API Abstraction
