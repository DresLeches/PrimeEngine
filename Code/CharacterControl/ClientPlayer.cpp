#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Inter-Engine includes
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Scene/Mesh.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Networking/EventManager.h"
#include "PrimeEngine/Networking/Client/ClientNetworkManager.h"
#include "CharacterControl/Events/Events.h"
#include "PrimeEngine/GameObjectModel/GameObjectManager.h"
#include "PrimeEngine/Events/StandardKeyboardEvents.h"
#include "PrimeEngine/Events/StandardIOSEvents.h"
#include "PrimeEngine/Events/StandardGameEvents.h"
#include "PrimeEngine/Events/EventQueueManager.h"
#include "PrimeEngine/Events/StandardControllerEvents.h"
#include "PrimeEngine/GameObjectModel/DefaultGameControls/DefaultGameControls.h"
#include "CharacterControl/CharacterControlContext.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Math/Vector3.h"
#include <string> 

#include "ClientPlayer.h"
#include "CharacterControl/Client/ClientSpaceShipControls.h"
#include "CharacterControl\Tank\ClientTank.h"
#include "PrimeEngine/Scene/DebugRenderer.h"
#include "PrimeEngine/Scene/SH_DRAW.h"
#include "PrimeEngine/Geometry/ParticleSystemCPU/ExplosionParticleSystemCPU.h"
#include "Characters/CharacterAnimationControler.h"


////
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")


#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>


class SoundClass
{
private:

	struct WaveHeaderType
	{
		char chunkId[4];
		unsigned long chunkSize;
		char format[4];
		char subChunkId[4];
		unsigned long subChunkSize;
		unsigned short audioFormat;
		unsigned short numChannels;
		unsigned long sampleRate;
		unsigned long bytesPerSecond;
		unsigned short blockAlign;
		unsigned short bitsPerSample;
		char dataChunkId[4];
		unsigned long dataSize;
	};

public:
	SoundClass();
	SoundClass(const SoundClass&);
	~SoundClass();
	bool PlayWaveFile();
	bool Initialize(HWND);
	void Shutdown();
	bool PlayWaveFile2();
private:
	bool InitializeDirectSound(HWND);
	void ShutdownDirectSound();
	bool LoadWaveFile2(char*, IDirectSoundBuffer8**);
	bool LoadWaveFile(char*, IDirectSoundBuffer8**);
	void ShutdownWaveFile(IDirectSoundBuffer8**);



private:
	IDirectSound8* m_DirectSound;
	IDirectSoundBuffer* m_primaryBuffer;
	//Note that I only have one secondary buffer as this tutorial only loads in one sound.

	IDirectSoundBuffer8* m_secondaryBuffer1;

	IDirectSoundBuffer8* m_secondaryBuffer2;


};

SoundClass::SoundClass()
{
	m_DirectSound = 0;
	m_primaryBuffer = 0;
	m_secondaryBuffer1 = 0;
}


SoundClass::SoundClass(const SoundClass& other)
{
}


SoundClass::~SoundClass()
{
}

bool SoundClass::Initialize(HWND hwnd)
{
	bool result;
	// Initialize direct sound and the primary sound buffer.
	result = InitializeDirectSound(hwnd);
	if (!result)
	{
		return false;
	}

	// Load a wave audio file onto a secondary buffer.

	result = LoadWaveFile("./Code/gunsound.wav", &m_secondaryBuffer1);
	if (!result)
	{
		return false;
	}

	result = LoadWaveFile2("./Code/grunt.wav", &m_secondaryBuffer2);
	if (!result)
	{
		return false;
	}

	// Play the wave file now that it has been loaded.
	//result = PlayWaveFile();
	//if (!result)
	//{
	//	return false;
	//}

	return true;
}

void SoundClass::Shutdown()
{
	// Release the secondary buffer.
	ShutdownWaveFile(&m_secondaryBuffer1);

	// Shutdown the Direct Sound API.
	ShutdownDirectSound();

	return;
}
bool SoundClass::InitializeDirectSound(HWND hwnd)
{
	HRESULT result;
	DSBUFFERDESC bufferDesc;
	WAVEFORMATEX waveFormat;


	// Initialize the direct sound interface pointer for the default sound device.
	result = DirectSoundCreate8(NULL, &m_DirectSound, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Set the cooperative level to priority so the format of the primary sound buffer can be modified.
	result = m_DirectSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
	if (FAILED(result))
	{
		return false;
	}
	// Setup the primary buffer description.
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	bufferDesc.dwBufferBytes = 0;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = NULL;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// Get control of the primary sound buffer on the default sound device.
	result = m_DirectSound->CreateSoundBuffer(&bufferDesc, &m_primaryBuffer, NULL);
	if (FAILED(result))
	{
		return false;
	}
	// Setup the format of the primary sound bufffer.
	// In this case it is a .WAV file recorded at 44,100 samples per second in 16-bit stereo (cd audio format).
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 1;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// Set the primary buffer to be the wave format specified.
	result = m_primaryBuffer->SetFormat(&waveFormat);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}


void SoundClass::ShutdownDirectSound()
{
	// Release the primary sound buffer pointer.
	if (m_primaryBuffer)
	{
		m_primaryBuffer->Release();
		m_primaryBuffer = 0;
	}

	// Release the direct sound interface pointer.
	if (m_DirectSound)
	{
		m_DirectSound->Release();
		m_DirectSound = 0;
	}

	return;
}

bool SoundClass::LoadWaveFile2(char* filename, IDirectSoundBuffer8** secondaryBuffer)
{
	int error;
	FILE* filePtr;
	unsigned int count;
	WaveHeaderType waveFileHeader;
	WAVEFORMATEX waveFormat;
	DSBUFFERDESC bufferDesc;
	HRESULT result;
	IDirectSoundBuffer* tempBuffer;
	unsigned char* waveData;
	unsigned char *bufferPtr;
	unsigned long bufferSize;

	// Open the wave file in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if (error != 0)
	{
		return false;
	}

	// Read in the wave file header.
	count = fread(&waveFileHeader, sizeof(waveFileHeader), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Check that the chunk ID is the RIFF format.
	if ((waveFileHeader.chunkId[0] != 'R') || (waveFileHeader.chunkId[1] != 'I') ||
		(waveFileHeader.chunkId[2] != 'F') || (waveFileHeader.chunkId[3] != 'F'))
	{
		return false;
	}

	// Check that the file format is the WAVE format.
	if ((waveFileHeader.format[0] != 'W') || (waveFileHeader.format[1] != 'A') ||
		(waveFileHeader.format[2] != 'V') || (waveFileHeader.format[3] != 'E'))
	{
		return false;
	}

	// Check that the sub chunk ID is the fmt format.
	if ((waveFileHeader.subChunkId[0] != 'f') || (waveFileHeader.subChunkId[1] != 'm') ||
		(waveFileHeader.subChunkId[2] != 't') || (waveFileHeader.subChunkId[3] != ' '))
	{
		return false;
	}

	// Check that the audio format is WAVE_FORMAT_PCM.
	if (waveFileHeader.audioFormat != WAVE_FORMAT_PCM)
	{
		return false;
	}

	// Check that the wave file was recorded in stereo format.
	if (waveFileHeader.numChannels != 2)
	{
		return false;
	}

	// Check that the wave file was recorded at a sample rate of 44.1 KHz.
	if (waveFileHeader.sampleRate != 44100)
	{
		return false;
	}

	// Ensure that the wave file was recorded in 16 bit format.
	if (waveFileHeader.bitsPerSample != 16)
	{
		return false;
	}



	// Check for the data chunk header.
	if ((waveFileHeader.dataChunkId[0] != 'd') || (waveFileHeader.dataChunkId[1] != 'a') ||
		(waveFileHeader.dataChunkId[2] != 't') || (waveFileHeader.dataChunkId[3] != 'a'))
	{
		return false;
	}

	// Set the wave format of secondary buffer that this wave file will be loaded onto.
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 2;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// Set the buffer description of the secondary sound buffer that the wave file will be loaded onto.
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &waveFormat;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// Create a temporary sound buffer with the specific buffer settings.
	result = m_DirectSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Test the buffer format against the direct sound 8 interface and create the secondary buffer.
	result = tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&*secondaryBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the temporary buffer.
	tempBuffer->Release();
	tempBuffer = 0;
	// Move to the beginning of the wave data which starts at the end of the data chunk header.
	fseek(filePtr, sizeof(WaveHeaderType), SEEK_SET);

	// Create a temporary buffer to hold the wave file data.
	waveData = new unsigned char[waveFileHeader.dataSize];
	if (!waveData)
	{
		return false;
	}

	// Read in the wave file data into the newly created buffer.
	count = fread(waveData, 1, waveFileHeader.dataSize, filePtr);
	if (count != waveFileHeader.dataSize)
	{
		return false;
	}

	// Close the file once done reading.
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	// Lock the secondary buffer to write wave data into it.
	result = (*secondaryBuffer)->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0);
	if (FAILED(result))
	{
		return false;
	}

	// Copy the wave data into the buffer.
	memcpy(bufferPtr, waveData, waveFileHeader.dataSize);

	// Unlock the secondary buffer after the data has been written to it.
	result = (*secondaryBuffer)->Unlock((void*)bufferPtr, bufferSize, NULL, 0);
	if (FAILED(result))
	{
		return false;
	}

	// Release the wave data since it was copied into the secondary buffer.
	delete[] waveData;
	waveData = 0;

	return true;
}
bool SoundClass::LoadWaveFile(char* filename, IDirectSoundBuffer8** secondaryBuffer)
{
	int error;
	FILE* filePtr;
	unsigned int count;
	WaveHeaderType waveFileHeader;
	WAVEFORMATEX waveFormat;
	DSBUFFERDESC bufferDesc;
	HRESULT result;
	IDirectSoundBuffer* tempBuffer;
	unsigned char* waveData;
	unsigned char *bufferPtr;
	unsigned long bufferSize;

	// Open the wave file in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if (error != 0)
	{
		return false;
	}

	// Read in the wave file header.
	count = fread(&waveFileHeader, sizeof(waveFileHeader), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Check that the chunk ID is the RIFF format.
	if ((waveFileHeader.chunkId[0] != 'R') || (waveFileHeader.chunkId[1] != 'I') ||
		(waveFileHeader.chunkId[2] != 'F') || (waveFileHeader.chunkId[3] != 'F'))
	{
		return false;
	}

	// Check that the file format is the WAVE format.
	if ((waveFileHeader.format[0] != 'W') || (waveFileHeader.format[1] != 'A') ||
		(waveFileHeader.format[2] != 'V') || (waveFileHeader.format[3] != 'E'))
	{
		return false;
	}

	// Check that the sub chunk ID is the fmt format.
	if ((waveFileHeader.subChunkId[0] != 'f') || (waveFileHeader.subChunkId[1] != 'm') ||
		(waveFileHeader.subChunkId[2] != 't') || (waveFileHeader.subChunkId[3] != ' '))
	{
		return false;
	}

	// Check that the audio format is WAVE_FORMAT_PCM.
	if (waveFileHeader.audioFormat != WAVE_FORMAT_PCM)
	{
		return false;
	}

	// Check that the wave file was recorded in stereo format.
	if (waveFileHeader.numChannels != 1)
	{
		return false;
	}

	// Check that the wave file was recorded at a sample rate of 44.1 KHz.
	if (waveFileHeader.sampleRate != 44100)
	{
		return false;
	}

	// Ensure that the wave file was recorded in 16 bit format.
	if (waveFileHeader.bitsPerSample != 16)
	{
		return false;
	}


	bool a, b, c, d = true;
	if (waveFileHeader.dataChunkId[0] != 'd')
		a = false;
	if (waveFileHeader.dataChunkId[0] != 'a')
		b = false;
	if (waveFileHeader.dataChunkId[0] != 't')
		c = false;
	if (waveFileHeader.dataChunkId[0] != 'a')
		d = false;

	// Check for the data chunk header.
	if ((waveFileHeader.dataChunkId[0] != 'd') || (waveFileHeader.dataChunkId[1] != 'a') ||
		(waveFileHeader.dataChunkId[2] != 't') || (waveFileHeader.dataChunkId[3] != 'a'))
	{
		return false;
	}

	// Set the wave format of secondary buffer that this wave file will be loaded onto.
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 1;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// Set the buffer description of the secondary sound buffer that the wave file will be loaded onto.
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &waveFormat;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// Create a temporary sound buffer with the specific buffer settings.
	result = m_DirectSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Test the buffer format against the direct sound 8 interface and create the secondary buffer.
	result = tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&*secondaryBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the temporary buffer.
	tempBuffer->Release();
	tempBuffer = 0;
	// Move to the beginning of the wave data which starts at the end of the data chunk header.
	fseek(filePtr, sizeof(WaveHeaderType), SEEK_SET);

	// Create a temporary buffer to hold the wave file data.
	waveData = new unsigned char[waveFileHeader.dataSize];
	if (!waveData)
	{
		return false;
	}

	// Read in the wave file data into the newly created buffer.
	count = fread(waveData, 1, waveFileHeader.dataSize, filePtr);
	if (count != waveFileHeader.dataSize)
	{
		return false;
	}

	// Close the file once done reading.
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	// Lock the secondary buffer to write wave data into it.
	result = (*secondaryBuffer)->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0);
	if (FAILED(result))
	{
		return false;
	}

	// Copy the wave data into the buffer.
	memcpy(bufferPtr, waveData, waveFileHeader.dataSize);

	// Unlock the secondary buffer after the data has been written to it.
	result = (*secondaryBuffer)->Unlock((void*)bufferPtr, bufferSize, NULL, 0);
	if (FAILED(result))
	{
		return false;
	}

	// Release the wave data since it was copied into the secondary buffer.
	delete[] waveData;
	waveData = 0;

	return true;
}

void SoundClass::ShutdownWaveFile(IDirectSoundBuffer8** secondaryBuffer)
{
	// Release the secondary sound buffer.
	if (*secondaryBuffer)
	{
		(*secondaryBuffer)->Release();
		*secondaryBuffer = 0;
	}

	return;
}

bool SoundClass::PlayWaveFile()
{
	HRESULT result;


	// Set position at the beginning of the sound buffer.
	result = m_secondaryBuffer1->SetCurrentPosition(0);
	if (FAILED(result))
	{
		return false;
	}

	// Set volume of the buffer to 100%.
	result = m_secondaryBuffer1->SetVolume(DSBVOLUME_MAX);
	if (FAILED(result))
	{
		return false;
	}

	// Play the contents of the secondary sound buffer.
	result = m_secondaryBuffer1->Play(0, 0, 0);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}
bool SoundClass::PlayWaveFile2()
{
	HRESULT result;


	// Set position at the beginning of the sound buffer.
	result = m_secondaryBuffer2->SetCurrentPosition(0);
	if (FAILED(result))
	{
		return false;
	}

	// Set volume of the buffer to 100%.
	result = m_secondaryBuffer2->SetVolume(DSBVOLUME_MAX);
	if (FAILED(result))
	{
		return false;
	}

	// Play the contents of the secondary sound buffer.
	result = m_secondaryBuffer2->Play(0, 0, 0);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}





////


using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

// Arkane Control Values
#define Analog_To_Digital_Trigger_Distance 0.5f
static float Debug_Fly_Speed = 4.0f; //Units per second
#define Debug_Rotate_Speed 2.0f //Radians per second
#define Player_Keyboard_Rotate_Speed 20.0f //Radians per second
float t = 0;
float prevX = 0;
float prevY = 0;
float shootTimer = 0.0f;
float shootFrameTime;
int lockMouseToScreen = -1;
bool keyKLeadingEdge = false;
float keyKleadingEdgeTimer = 0.0f;
float keyKleadingEdgeTime;
bool canJump = true;
int jumpUpCounter = 0;
int jumpDownCounter = 0;
float jumpTimer = 0.0f;
bool isRising = true;
float CharacterControl::Components::PlayerController::m_officialGameTime = 0.0f; // The level of hack rn lol
bool CharacterControl::Components::PlayerController::m_gameStarted = false;
//float CharacterControl::Components::PlayerController::m_moveUpdateTimer = 0.0f;


SoundClass* m_Sound;

namespace CharacterControl {
	namespace Components {




		PE_IMPLEMENT_CLASS1(PlayerGameControls, PE::Components::Component);

		void PlayerGameControls::addDefaultComponents()
		{
			Component::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(Event_UPDATE, PlayerGameControls::do_UPDATE);
		}

		void PlayerGameControls::do_UPDATE(PE::Events::Event *pEvt)
		{
			// Process input events (controller buttons, triggers...)
			PE::Handle iqh = PE::Events::EventQueueManager::Instance()->getEventQueueHandle("input");

			// Process input event -> game event conversion
			while (!iqh.getObject<PE::Events::EventQueue>()->empty())
			{
				PE::Events::Event *pInputEvt = iqh.getObject<PE::Events::EventQueue>()->getFront();
				m_frameTime = ((Event_UPDATE*)(pEvt))->m_frameTime;
				// Have DefaultGameControls translate the input event to GameEvents
				handleKeyboardDebugInputEvents(pInputEvt);
				handleControllerDebugInputEvents(pInputEvt);
				handleIOSDebugInputEvents(pInputEvt);

				iqh.getObject<PE::Events::EventQueue>()->destroyFront();
			}

			// Events are destoryed by destroyFront() but this is called every frame just in case
			iqh.getObject<PE::Events::EventQueue>()->destroy();
		}

		void PlayerGameControls::handleIOSDebugInputEvents(Event *pEvt)
		{
#if APIABSTRACTION_IOS
			m_pQueueManager = PE::Events::EventQueueManager::Instance();
			if (Event_IOS_TOUCH_MOVED::GetClassId() == pEvt->getClassId())
			{
				Event_IOS_TOUCH_MOVED *pRealEvent = (Event_IOS_TOUCH_MOVED *)(pEvt);

				if (pRealEvent->touchesCount > 1)
				{
					PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
					Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

					Vector3 relativeMovement(0.0f, 0.0f, -30.0f * pRealEvent->m_normalized_dy);
					flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
					m_pQueueManager->add(h, QT_GENERAL);
				}
				else
				{
					PE::Handle h("EVENT", sizeof(Event_Player_Turn));
					Event_Player_Turn *rotateCameraEvt = new(h) Event_Player_Turn;

					Vector3 relativeRotate(pRealEvent->m_normalized_dx * 10, 0.0f, 0.0f);
					rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
					m_pQueueManager->add(h, QT_GENERAL);
				}
			}
#endif
		}

		void PlayerGameControls::handleKeyboardDebugInputEvents(Event *pEvt)
		{
			m_pQueueManager = PE::Events::EventQueueManager::Instance();
			
			if (PE::Events::Event_KEY_A_HELD::GetClassId() == pEvt->getClassId())
			{
				/*PE::Handle h("EVENT", sizeof(Event_FLY_CAMERA));
				PE::Events::Event_FLY_CAMERA *flyCameraEvt = new(h) PE::Events::Event_FLY_CAMERA ;*/
				PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
				Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

				Vector3 relativeMovement(-1.0f,0.0f,0.0f);
				flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
			}

			else if (Event_KEY_S_HELD::GetClassId() == pEvt->getClassId())
			{
				PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
				Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

				Vector3 relativeMovement(0.0f, 0.0f, -1.0f);
				flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
			}
			
			else if (Event_KEY_D_HELD::GetClassId() == pEvt->getClassId())
			{
				/*PE::Handle h("EVENT", sizeof(Event_FLY_CAMERA));
				Event_FLY_CAMERA *flyCameraEvt = new(h) Event_FLY_CAMERA ;*/

				PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
				Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

				Vector3 relativeMovement(1.0f,0.0f,0.0f);
				flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
			}

			else if (Event_KEY_W_HELD::GetClassId() == pEvt->getClassId())
			{
				PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
				Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

				Vector3 relativeMovement(0.0f, 0.0f, 1.0f);
				flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
			}
			else if (Event_KEY_K_HELD::GetClassId() == pEvt->getClassId())
			{
				keyKleadingEdgeTimer = 0.0f;
				if (false == keyKLeadingEdge)
				{
					keyKLeadingEdge = true;
					OutputDebugStringA("lockingMouse");
					lockMouseToScreen *= -1;
				}
			}
			else if (Event_KEY_LEFT_HELD::GetClassId() == pEvt->getClassId())
			{
				PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
				Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

				Vector3 relativeMovement(-1.0f, 0.0f, 0.0f);
				flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
				/*
				PE::Handle h("EVENT", sizeof(Event_Player_Turn));
				Event_Player_Turn *rotateCameraEvt = new(h) Event_Player_Turn;

				Vector3 relativeRotate(-1.0f, 0.0f, 0.0f);
				rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
				*/
			}
			else if (Event_KEY_RIGHT_HELD::GetClassId() == pEvt->getClassId())
			{
				PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
				Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

				Vector3 relativeMovement(1.0f, 0.0f, 0.0f);
				flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
				/*
				PE::Handle h("EVENT", sizeof(Event_Player_Turn));
				Event_Player_Turn *rotateCameraEvt = new(h) Event_Player_Turn;

				Vector3 relativeRotate(1.0f, 0.0f, 0.0f);
				rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
				*/
			}
			else if (Event_KEY_DOWN_HELD::GetClassId() == pEvt->getClassId())
			{
				PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
				Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

				Vector3 relativeMovement(0.0f, 0.0f, -1.0f);
				flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
			/*
			PE::Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));
			Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;

			Vector3 relativeRotate(0.0f,-1.0f,0.0f);
			rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
			m_pQueueManager->add(h, QT_GENERAL);
			*/
			}
			else if (Event_KEY_UP_HELD::GetClassId() == pEvt->getClassId())
			{
				PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
				Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

				Vector3 relativeMovement(0.0f, 0.0f, 1.0f);
				flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
			/*
			PE::Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));
			Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;

			Vector3 relativeRotate(0.0f,1.0f,0.0f);
			rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
			m_pQueueManager->add(h, QT_GENERAL);
			*/
			}
			
			else if (Event_MOUSE_LBUTTON_HELD::GetClassId() == pEvt->getClassId())
			{
				PE::Handle h("EVENT", sizeof(Event_Player_Shoot));
				Event_Player_Shoot *playerShootEvt = new(h) Event_Player_Shoot;

				m_pQueueManager->add(h, QT_GENERAL);
			}

			else if (Event_MOUSE_ROTATE::GetClassId() == pEvt->getClassId())
			{
				if (lockMouseToScreen < 0)
				{
				PE::Handle h("EVENT", sizeof(Event_Player_Turn));
				Event_Player_Turn *rotateCameraEvt = new(h) Event_Player_Turn;
				POINT mMove;
				
				//turns off mouse cursor
				ShowCursor(false);
				//gets mouse position
				GetCursorPos(&mMove);
				//SetCursorPos(960, 540);
				
				//Set Player Invert 
				float invertX = 1.0f;
				float invertY = -1.0f;

				//rotate 
				float horizontal = (mMove.x - 960) * 0.09f * invertX;
				float vertical = (mMove.y - 540) * 0.05f * invertY;
					
				//std::string a = std::to_string(horizontal);
				//std::string b = std::to_string(vertical);
				//OutputDebugStringA("\n MOVE x = ");
				//OutputDebugStringA(a.c_str());
				//OutputDebugStringA("\n MOVE y = ");
				//OutputDebugStringA(b.c_str());

				Vector3 relativeRotate(horizontal, 0.0f, 0.0f);
				rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
				m_pQueueManager->add(h, QT_GENERAL);
				
				//vertical rotation needs clamp also a seperate event 
				PE::Handle hV("EVENT", sizeof(Event_ROTATE_CAMERA));
				Event_ROTATE_CAMERA *rotateCameraEvtV = new(hV) Event_ROTATE_CAMERA;

				Vector3 relativeRotateV(0.0f,vertical, 0.0f);
				rotateCameraEvtV->m_relativeRotate = relativeRotateV * Debug_Rotate_Speed * m_frameTime;
				m_pQueueManager->add(hV, QT_GENERAL);


				//set cursor to middle of screen every frame
				
					SetCursorPos(960, 540);
				}

			}

			else if (Event_SPACEBAR_HELD::GetClassId() == pEvt->getClassId())
			{
				if (canJump)
					canJump = false;
			}

			else if (Event_KEY_G_HELD::GetClassId() == pEvt->getClassId())
			{
				getGameContext()->getGPUScreen()->ToggleGraphicsMode();
			}

			else
			{
				Component::handleEvent(pEvt);
			}
			
		}

		void PlayerGameControls::handleControllerDebugInputEvents(Event *pEvt)
		{

			if (Event_ANALOG_L_THUMB_MOVE::GetClassId() == pEvt->getClassId())
			{
				Event_ANALOG_L_THUMB_MOVE *pRealEvent = (Event_ANALOG_L_THUMB_MOVE*)(pEvt);

				//Move
				{
					PE::Handle h("EVENT", sizeof(Events::Event_Player_Move));
					Events::Event_Player_Move *flyCameraEvt = new(h) Events::Event_Player_Move;

					Vector3 relativeMovement(0.0f, 0.0f, pRealEvent->m_absPosition.getY());
					flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
					m_pQueueManager->add(h, QT_GENERAL);
				}

				//turn
				{
					PE::Handle h("EVENT", sizeof(Event_Player_Turn));
					Event_Player_Turn *rotateCameraEvt = new(h) Event_Player_Turn;

					Vector3 relativeRotate(pRealEvent->m_absPosition.getX(), 0.0f, 0.0f);
					rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
					m_pQueueManager->add(h, QT_GENERAL);
				}
			}
			/*
			else if (Event_ANALOG_R_THUMB_MOVE::GetClassId() == pEvt->getClassId())
			{
			Event_ANALOG_R_THUMB_MOVE *pRealEvent = (Event_ANALOG_R_THUMB_MOVE *)(pEvt);

			PE::Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));
			Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;

			Vector3 relativeRotate(pRealEvent->m_absPosition.getX(), pRealEvent->m_absPosition.getY(), 0.0f);
			rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
			m_pQueueManager->add(h, QT_GENERAL);
			}
			else if (Event_PAD_N_DOWN::GetClassId() == pEvt->getClassId())
			{
			}
			else if (Event_PAD_N_HELD::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_N_UP::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_S_DOWN::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_S_HELD::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_S_UP::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_W_DOWN::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_W_HELD::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_W_UP::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_E_DOWN::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_E_HELD::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_PAD_E_UP::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_BUTTON_A_HELD::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_BUTTON_Y_DOWN::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_BUTTON_A_UP::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_BUTTON_B_UP::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_BUTTON_X_UP::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_BUTTON_Y_UP::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_ANALOG_L_TRIGGER_MOVE::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_ANALOG_R_TRIGGER_MOVE::GetClassId() == pEvt->getClassId())
			{

			}
			else if (Event_BUTTON_L_SHOULDER_DOWN::GetClassId() == pEvt->getClassId())
			{

			}
			else
			*/
			{
				Component::handleEvent(pEvt);
			}
		}

		PE_IMPLEMENT_CLASS1(PlayerController, Component);

		PlayerController::PlayerController(PE::GameContext &context, PE::MemoryArena arena,
			PE::Handle myHandle, float speed, Vector3 spawnPos,
			float networkPingInterval)
			: Component(context, arena, myHandle)
			, m_timeSpeed(speed)
			, m_time(0)
			, m_counter(0)
			, m_active(0)
			, m_networkPingTimer(0)
			, m_networkPingInterval(networkPingInterval)
			, m_overriden(false)
		{
			m_spawnPos = spawnPos;
			m_SphereCollider.yOffset = 1.5f;
			m_SphereCollider.radius = 0.35f;
			m_health = 100.0f;
			m_damage = 25.0f;
			m_canShoot = true;
			m_canJump = true;
			m_velocity = Vector3(0.0f, 0.0f, 0.0f);
			m_score = 0;
			RootSceneNode::Instance()->m_playerVelocities.push_back(&m_velocity);
		}

		void PlayerController::addDefaultComponents()
		{
			Component::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, PlayerController::do_UPDATE);

			// note: these event handlers will be registered only when one tank is activated as client tank (i.e. driven by client input on this machine)
			// 	PE_REGISTER_EVENT_HANDLER(Event_Player_Move, PlayerController::do_Tank_Throttle);
			// 	PE_REGISTER_EVENT_HANDLER(Event_Player_Turn, PlayerController::do_Tank_Turn);

		}

		bool SphereAABBCollisionCheck(Vector3 sc, Vector3 minVec, Vector3 maxVec, PrimitiveTypes::Float32 r) {

			/*var x = Math.max(box.minX, Math.min(sphere.x, box.maxX));
			var y = Math.max(box.minY, Math.min(sphere.y, box.maxY));
			var z = Math.max(box.minZ, Math.min(sphere.z, box.maxZ));*/

			PrimitiveTypes::Float32 x = max(minVec.m_values[0], min(sc.m_values[0], maxVec.m_values[0]));
			PrimitiveTypes::Float32 y = max(minVec.m_values[1], min(sc.m_values[1], maxVec.m_values[1]));
			PrimitiveTypes::Float32 z = max(minVec.m_values[2], min(sc.m_values[2], maxVec.m_values[2]));

			/*var distance = Math.sqrt((x - sphere.x) * (x - sphere.x) +
				(y - sphere.y) * (y - sphere.y) +
				(z - sphere.z) * (z - sphere.z));*/

			PrimitiveTypes::Float32 distance = sqrt(pow(x - sc.m_values[0], 2.0f) + pow(y - sc.m_values[1], 2.0f) + pow(z - sc.m_values[2], 2.0f));
			return distance < r;
		}

		void ClosestPtPointOBB(Vector3 p, SceneNode *sn, AABB aabb, Vector3 &q)
		{
			Vector3 c = sn->m_base.getPos() + Vector3(0.0f, aabb.maxY, 0.0f);
			Vector3 d = p - c;
			q = c;
			
			for (int i = 0; i < 3; i++) {

				float dist;
				if (i == 0) {
					Vector3 u0 = sn->m_base.getN() + Vector3(0.0f, aabb.maxY, 0.0f);
					u0.normalize();
					dist = d.dotProduct(u0);
					if (dist > aabb.maxZ) dist = aabb.maxZ;
					if (dist < aabb.minZ) dist = aabb.minZ;
					q += dist * u0;
				}
				else if (i == 1) {
					Vector3 u1 = sn->m_base.getV() + Vector3(0.0f, aabb.maxY, 0.0f);
					u1.normalize();
					dist = d.dotProduct(u1);
					if (dist > aabb.maxY) dist = aabb.maxY;
					if (dist < aabb.minY) dist = aabb.minY;
					q += dist * u1;
				}	

				else if (i == 2) {
					Vector3 u2 = sn->m_base.getU() + Vector3(0.0f, aabb.maxY, 0.0f);
					u2.normalize();
					dist = d.dotProduct(u2);
					if (dist > aabb.maxX) dist = aabb.maxX;
					if (dist < aabb.minX) dist = aabb.minX;
					q += dist * u2;
				}
			}
		}

		bool SphereOBBCollisionCheck(Vector3 sc, SceneNode* sn, AABB aabb, float r) {
			{
				// Find point p on OBB closest to sphere center
				Vector3 p;
				ClosestPtPointOBB(sc, sn, aabb, p);
				// Sphere and OBB intersect if the (squared) distance from sphere
				// center to point p is less than the (squared) sphere radius
				Vector3 v = p - sc;
				return v.dotProduct(v) <= r * r;
			}
		}

		bool SphereIntersectSphere(Vector3 playerLoc, Vector3 enemyLoc, float r) {
			Vector3 lineBetweenSpheres = playerLoc - enemyLoc;
			float x2 = (lineBetweenSpheres.m_x*lineBetweenSpheres.m_x);
			float y2 = (lineBetweenSpheres.m_y*lineBetweenSpheres.m_y);
			float z2 = (lineBetweenSpheres.m_z*lineBetweenSpheres.m_z);
			return (x2 + y2 + z2) <= (4.0f*r*r);
		}

		void PlayerController::do_Player_Move(PE::Events::Event *pEvt)
		{
			Event_Player_Move *pRealEvent = (Event_Player_Move *)(pEvt);

			PE::Handle hFisrtSN = getFirstComponentHandle<SceneNode>();
			if (!hFisrtSN.isValid())
			{
				assert(!"wrong setup. must have scene node referenced");
				return;
			}

			SceneNode *pFirstSN = hFisrtSN.getObject<SceneNode>();
			Vector3 sc = pFirstSN->m_base.getPos() + Vector3(0.0f, m_SphereCollider.yOffset, 0.0f);

			Vector3 fwdPos = sc + pFirstSN->m_base.getN() * pRealEvent->m_relativeMove.getZ();
			Vector3 rightPos = sc + pFirstSN->m_base.getU() * pRealEvent->m_relativeMove.getX();
			//Vector3 upPos = sc + pFirstSN->m_base.getV() * pRealEvent->m_relativeMove.getY();

			bool moveFwd = pRealEvent->m_relativeMove.getZ() != 0.0f;
			bool moveRight = pRealEvent->m_relativeMove.getX() != 0.0f;
			bool moveUp = false; //white dudes can't jump

			for ( PrimitiveTypes::UInt32 i = 0; i < RootSceneNode::Instance()->m_meshSNs.size(); i++) {

				if (!moveFwd && !moveRight && !moveUp) {
					break;
				}

				AABB col = RootSceneNode::Instance()->m_meshSNs[i]->m_AABB;

				Vector3 minVec = RootSceneNode::Instance()->m_meshSNs[i]->m_worldTransform * Vector3(col.minX, col.minY, col.minZ);
				Vector3 maxVec = RootSceneNode::Instance()->m_meshSNs[i]->m_worldTransform * Vector3(col.maxX, col.maxY, col.maxZ);

				if (moveFwd) {
					//if (SphereAABBCollisionCheck(fwdPos, minVec, maxVec, 1.0f)) {
					if(SphereAABBCollisionCheck(fwdPos, minVec, maxVec, 1.0f) || 
						SphereOBBCollisionCheck(fwdPos, RootSceneNode::Instance()->m_meshSNs[i], col, 1.0f)) {
						moveFwd = false;
					}
				}

				if (moveRight) {
					//if (SphereAABBCollisionCheck(rightPos, minVec, maxVec, 1.0f)) {
					if (SphereAABBCollisionCheck(rightPos, minVec, maxVec, 1.0f) || 
						SphereOBBCollisionCheck(rightPos, RootSceneNode::Instance()->m_meshSNs[i], col, 1.0f)) {
						moveRight = false;
					}
				}

				/*if (moveUp) {
					if (SphereAABBCollisionCheck(upPos, minVec, maxVec, m_SphereCollider.radius)) {
						moveUp = false;
					}
				}*/
			}

			for (PrimitiveTypes::UInt32 i = 0; i < RootSceneNode::Instance()->m_playerSNs.size(); i++) {
				if (RootSceneNode::Instance()->m_playerSNs[i] == pFirstSN) {
					continue;
				}

				Vector3 enemyLoc = RootSceneNode::Instance()->m_playerSNs[i]->m_base.getPos() + Vector3(0.0f, m_SphereCollider.yOffset, 0.0f);;

				if (moveFwd) {
					if (SphereIntersectSphere(fwdPos, enemyLoc, m_SphereCollider.radius)) {
						moveFwd = false;
					}
				}

				if (moveRight) {
					if (SphereIntersectSphere(rightPos, enemyLoc, m_SphereCollider.radius)) {
						moveRight = false;
					}
				}
			}

			if (moveFwd) {
				pFirstSN->m_base.moveForward(pRealEvent->m_relativeMove.getZ());
				m_velocity.m_values[2] = pRealEvent->m_relativeMove.getZ();
			}
			if (moveRight) {
				pFirstSN->m_base.moveRight(pRealEvent->m_relativeMove.getX());
				m_velocity.m_values[0] = pRealEvent->m_relativeMove.getX();
			}
			/*if(moveUp)
				pFirstSN->m_base.moveUp(pRealEvent->m_relativeMove.getY());*/
		}

		void PlayerController::do_Player_Turn(PE::Events::Event *pEvt)
		{
			Event_Player_Turn *pRealEvent = (Event_Player_Turn *)(pEvt);

			PE::Handle hFisrtSN = getFirstComponentHandle<SceneNode>();
			if (!hFisrtSN.isValid())
			{
				assert(!"wrong setup. must have scene node referenced");
				return;
			}

			SceneNode *pFirstSN = hFisrtSN.getObject<SceneNode>();
			Matrix4x4 projectedBase = pFirstSN->m_base;
			Vector3 projectedPos = Vector3(0.0f, m_SphereCollider.yOffset, 0.0f);

			projectedBase.turnLeft(-pRealEvent->m_relativeRotate.getX());
			projectedPos += projectedBase.getPos();

			bool turnLeft = true;
			bool turnUp = false; //turnUp for what??

			for ( PrimitiveTypes::UInt32 i = 0; i < RootSceneNode::Instance()->m_meshSNs.size(); i++) {

				if (!turnLeft && !turnUp) {
					break;
				}

				AABB col = RootSceneNode::Instance()->m_meshSNs[i]->m_AABB;

				Vector3 minVec = RootSceneNode::Instance()->m_meshSNs[i]->m_worldTransform * Vector3(col.minX, col.minY, col.minZ);
				Vector3 maxVec = RootSceneNode::Instance()->m_meshSNs[i]->m_worldTransform * Vector3(col.maxX, col.maxY, col.maxZ);

				if (turnLeft) {
					if (SphereAABBCollisionCheck(projectedPos, minVec, maxVec, m_SphereCollider.radius)) {
						turnLeft = false;
					}
				}
			}

			//if(turnUp)
				//pcam->m_base.turnUp(pRealEvent->m_relativeRotate.getY());
			if(turnLeft)
				pFirstSN->m_base.turnLeft(-pRealEvent->m_relativeRotate.getX());

		}

		bool RayIntersectAABB(Vector3 rayOrg, Vector3 rayDir, Vector3 minVec, Vector3 maxVec) {
			float tmin = (minVec.m_x - rayOrg.m_x) / rayDir.m_x;
			float tmax = (maxVec.m_x - rayOrg.m_x) / rayDir.m_x;

			if (tmin > tmax) {
				std::swap(tmin, tmax);
			}

			float tymin = (minVec.m_y - rayOrg.m_y) / rayDir.m_y;
			float tymax = (maxVec.m_y - rayOrg.m_y) / rayDir.m_y;

			if (tymin > tymax) {
				std::swap(tmin, tmax);
			}

			if ((tmin > tymax) || (tymin > tmax))
				return false;

			if (tymin > tmin)
				tmin = tymin;

			if (tymax < tmax)
				tmax = tymax;

			float tzmin = (minVec.m_z - rayOrg.m_z) / rayDir.m_z;
			float tzmax = (maxVec.m_z - rayOrg.m_z) / rayDir.m_z;

			if (tzmin > tzmax) {
				std::swap(tzmin, tzmax);
			}

			if ((tmin > tzmax) || (tzmin > tmax))
				return false;

			if (tzmin > tmin)
				tmin = tzmin;

			if (tzmax < tmax)
				tmax = tzmax;

			return true;
		}

		bool RayIntersectAABB2(Vector3 rayOrg, Vector3 rayDir, Vector3 minVec, Vector3 maxVec) {
			Vector3 dirfrac;
			dirfrac.m_x = 1.0f / rayDir.m_x;
			dirfrac.m_y = 1.0f / rayDir.m_y;
			dirfrac.m_z = 1.0f / rayDir.m_z;
			// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
			// r.org is origin of ray
			float t1 = (minVec.m_x - rayOrg.m_x)*dirfrac.m_x;
			float t2 = (maxVec.m_x - rayOrg.m_x)*dirfrac.m_x;
			float t3 = (minVec.m_y - rayOrg.m_y)*dirfrac.m_y;
			float t4 = (maxVec.m_y - rayOrg.m_y)*dirfrac.m_y;
			float t5 = (minVec.m_z - rayOrg.m_z)*dirfrac.m_z;
			float t6 = (maxVec.m_z - rayOrg.m_z)*dirfrac.m_z;

			float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
			float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

			// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
			if (tmax < 0)
			{
				t = tmax;
				return false;
			}

			// if tmin > tmax, ray doesn't intersect AABB
			if (tmin > tmax)
			{
				t = tmax;
				return false;
			}

			t = tmin;
			return true;
		}

		bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
		{
			float discr = b * b - 4 * a * c;
			if (discr < 0) return false;
			else if (discr == 0) x0 = x1 = -0.5f * b / a;
			else {
				float q = (b > 0.0f) ?
					-0.5f * (b + sqrt(discr)) :
					-0.5f * (b - sqrt(discr));
				x0 = q / a;
				x1 = c / q;
			}
			if (x0 > x1) std::swap(x0, x1);

			return true;
		}


		bool RayIntersectSphere(Vector3 rayOrg, Vector3 rayDir, Vector3 spherePos, float radius) {

			// geometric solution
			Vector3 L = spherePos - rayOrg;
			float tca = L.dotProduct(rayDir);
			if (tca < 0) return false;
			float d2 = L.dotProduct(L) - (tca * tca);
			if (d2 > (radius*radius)) return false;
			float thc = sqrt((radius*radius) - d2);
			float t0 = tca - thc;
			float t1 = tca + thc;

			//analytical solution
			/*Vector3 L = rayOrg - spherePos;
			float a = rayDir.dotProduct(rayDir);
			float b = 2.0f * rayDir.dotProduct(L);
			float c = L.dotProduct(L) - radius2;
			if (!solveQuadratic(a, b, c, t0, t1)) return false;*/

			if (t0 > t1) std::swap(t0, t1);
			if (t0 < 0) {
				t0 = t1; // if t0 is negative, let's use t1 instead 
				if (t0 < 0) return false; // both t0 and t1 are negative 
			}

			float t = t0;
		
			return true;
		}

		bool IntersectRaySphere(Vector3 rayOrigin, Vector3 rayDir, Vector3 sphereCenter, float r, float& tmin, float& tmax)
		{
			Vector3 CO = rayOrigin - sphereCenter;

			float a = rayDir.dotProduct(rayDir);
			float b = 2.0f * CO.dotProduct(rayDir);
			float c = CO.dotProduct(CO) - (r * r);

			float discriminant = b * b - 4.0f * a * c;
			if (discriminant < 0.0f)
				return false;

			tmin = (-b - sqrtf(discriminant)) / (2.0f * a);
			tmax = (-b + sqrtf(discriminant)) / (2.0f * a);
			if (tmin > tmax)
			{
				float temp = tmin;
				tmin = tmax;
				tmax = temp;
			}

			return true;
		}

		bool IntersectRayCapsule(Vector3 rayOrigin, Vector3 rayDir, Vector3 capA, Vector3 capB, float capRadius, Vector3& p1, Vector3& p2, Vector3& n1, Vector3& n2)
		{
			
			Vector3 AB = capB - capA;
			Vector3 AO = rayOrigin - capA;

			float AB_dot_d = AB.dotProduct(rayDir);
			float AB_dot_AO = AB.dotProduct(AO);
			float AB_dot_AB = AB.dotProduct(AB);

			float m = AB_dot_d / AB_dot_AB;
			float n = AB_dot_AO / AB_dot_AB;

			Vector3 Q = rayDir - (AB * m);
			Vector3 R = AO - (AB * n);

			float a = Q.dotProduct(Q);
			float b = 2.0f * Q.dotProduct(R);
			float c = R.dotProduct(R) - (capRadius * capRadius);

			if (a == 0.0f)
			{	
				Vector3 sphereACenter = capA;
				float sphereARadius = capRadius;
				Vector3 sphereBCenter = capB;
				float sphereBRadius = capRadius;

				float atmin, atmax, btmin, btmax;
				if (!IntersectRaySphere(rayOrigin, rayDir, sphereACenter, sphereARadius, atmin, atmax) ||
					!IntersectRaySphere(rayOrigin, rayDir, sphereBCenter, sphereBRadius, btmin, btmax))
				{
					// No intersection with one of the spheres means no intersection at all...
					return false;
				}

				if (atmin < btmin)
				{
					p1 = rayOrigin + (rayDir * atmin);
					n1 = p1 - capA;
					n1.normalize();
				}
				else
				{
					p1 = rayOrigin + (rayDir * btmin);
					n1 = p1 - capB;
					n1.normalize();
				}

				if (atmax > btmax)
				{
					p2 = rayOrigin + (rayDir * atmax);
					n2 = p2 - capA;
					n2.normalize();
				}
				else
				{
					p2 = rayOrigin + (rayDir * btmax);
					n2 = p2 - capB;
					n2.normalize();
				}

				return true;
			}

			float discriminant = b * b - 4.0f * a * c;
			if (discriminant < 0.0f)
			{
				// The ray doesn't hit the infinite cylinder defined by (A, B).
				// No intersection.
				return false;
			}

			float tmin = (-b - sqrtf(discriminant)) / (2.0f * a);
			float tmax = (-b + sqrtf(discriminant)) / (2.0f * a);
			if (tmin > tmax)
			{
				float temp = tmin;
				tmin = tmax;
				tmax = temp;
			}

			// Now check to see if K1 and K2 are inside the line segment defined by A,B
			float t_k1 = tmin * m + n;
			if (t_k1 < 0.0f)
			{
				// On sphere (A, r)...
				Vector3 sCenter = capA;
				float sRadius = capRadius;

				float stmin, stmax;
				if (IntersectRaySphere(rayOrigin, rayDir, sCenter, sRadius, stmin, stmax))
				{
					p1 = rayOrigin + (rayDir * stmin);
					n1 = p1 - capA;
					n1.normalize();
				}
				else
					return false;
			}
			else if (t_k1 > 1.0f)
			{
				// On sphere (B, r)...
				Vector3 sCenter = capB;
				float sRadius = capRadius;

				float stmin, stmax;
				if (IntersectRaySphere(rayOrigin, rayDir, sCenter, sRadius, stmin, stmax))
				{
					p1 = rayOrigin + (rayDir * stmin);
					n1 = p1 - capB;
					n1.normalize();
				}
				else
					return false;
			}
			else
			{
				// On the cylinder...
				p1 = rayOrigin + (rayDir * tmin);

				Vector3 k1 = capA + AB * t_k1;
				n1 = p1 - k1;
				n1.normalize();
			}

			float t_k2 = tmax * m + n;
			if (t_k2 < 0.0f)
			{
				// On sphere (A, r)...
				Vector3 sCenter = capA;
				float sRadius = capRadius;

				float stmin, stmax;
				if (IntersectRaySphere(rayOrigin, rayDir, sCenter, sRadius, stmin, stmax))
				{
					p2 = rayOrigin + (rayDir * stmax);
					n2 = p2 - capA;
					n2.normalize();
				}
				else
					return false;
			}
			else if (t_k2 > 1.0f)
			{
				// On sphere (B, r)...
				Vector3 sCenter = capB;
				float sRadius = capRadius;

				float stmin, stmax;
				if (IntersectRaySphere(rayOrigin, rayDir, sCenter, sRadius, stmin, stmax))
				{
					p2 = rayOrigin + (rayDir * stmax);
					n2 = p2 - capB;
					n2.normalize();
				}
				else
					return false;
			}
			else
			{
				p2 = rayOrigin + (rayDir * tmax);

				Vector3 k2 = capA + AB * t_k2;
				n2 = p2 - k2;
				n2.normalize();
			}

			return true;
		}
		void PlayerController::do_Player_Hurt(PE::Events::Event *pEvt)
		{
			PE::Events::Event_UPDATE *pRealEvt = (PE::Events::Event_UPDATE *)(pEvt);
			m_Sound->PlayWaveFile2();

			OutputDebugStringA("hurt sound\n");
		}

		void PlayerController::do_Player_Shoot(PE::Events::Event *pEvt)
		{
			if (shootTimer != 0.0f) {
				return;
			}
			m_Sound->PlayWaveFile();
			m_canShoot = false;
			
			PE::Handle hFisrtSN = getFirstComponentHandle<SceneNode>();
			if (!hFisrtSN.isValid())
			{
				assert(!"wrong setup. must have scene node referenced");
				return;
			}

			SceneNode* pFirstSN = hFisrtSN.getObject<SceneNode>();
			CameraSceneNode *pCamSN = pFirstSN->getFirstComponent<CameraSceneNode>();
			Vector3 rayOrigin = pFirstSN->m_base.getPos() + Vector3(0.0f, m_SphereCollider.yOffset, 0.0f);
			Vector3 rayDir = Vector3(pFirstSN->m_base.getN().getX(),pCamSN->m_base.getN().getY(),pFirstSN->m_base.getN().getZ());
			rayDir.normalize();

			Vector3 p1, p2, n1, n2;

			/*PE::ExplosionParticleSystemCPU shootFX(*m_pContext, m_arena);
			shootFX.create(rayOrigin + Vector3(1.0f, 0.0f, 0.0f), Vector2(240.0f,240.0f), 0.0f);*/

			bool hitPlayer = false;
			bool hitWall = false;
			SceneNode *hitPlayerSN;
			int hitPlayerId = 0;
			
			float minDistance = 1000000000.0f;
			SceneNode *closestHit;

			
			for (PrimitiveTypes::UInt32 i = 0; i < RootSceneNode::Instance()->m_meshSNs.size(); i++) {
				AABB col = RootSceneNode::Instance()->m_meshSNs[i]->m_AABB;

				Vector3 minVec = RootSceneNode::Instance()->m_meshSNs[i]->m_worldTransform * Vector3(col.minX, col.minY, col.minZ);
				Vector3 maxVec = RootSceneNode::Instance()->m_meshSNs[i]->m_worldTransform * Vector3(col.maxX, col.maxY, col.maxZ);

				if (RayIntersectAABB2(rayOrigin, rayDir, minVec, maxVec)) {

					hitWall = true;
					closestHit = RootSceneNode::Instance()->m_meshSNs[i];
					Vector3 hitPos = closestHit->m_base.getPos() + Vector3(0.0f, m_SphereCollider.yOffset, 0.0f);

					float distance = sqrt(pow(rayOrigin.m_values[0] - hitPos.m_values[0], 2.0f) + 
						pow(rayOrigin.m_values[1] - hitPos.m_values[1], 2.0f) + 
						pow(rayOrigin.m_values[2] - hitPos.m_values[2], 2.0f));

					if (distance < minDistance) {
						minDistance = distance;
					}
				}
			}

			for (PrimitiveTypes::UInt32 i = 0; i < RootSceneNode::Instance()->m_playerSNs.size(); i++) {

				if (RootSceneNode::Instance()->m_playerSNs[i] == pFirstSN) {
					continue;
				}
			
				//Vector3 spherePos = RootSceneNode::Instance()->m_playerSNs[i]->m_base.getPos() + Vector3(0, m_SphereCollider.yOffset, 0);
				AABB col = RootSceneNode::Instance()->m_playerSNs[i]->m_AABB;
				Vector3 minVec = RootSceneNode::Instance()->m_playerSNs[i]->m_worldTransform * Vector3(col.minX, col.minY, col.minZ);
				Vector3 maxVec = RootSceneNode::Instance()->m_playerSNs[i]->m_worldTransform * Vector3(col.maxX, col.maxY, col.maxZ);

				Vector3 capA = Vector3(RootSceneNode::Instance()->m_playerSNs[i]->m_base.getPos().getX(),
					maxVec.getY() - m_SphereCollider.radius, 
					RootSceneNode::Instance()->m_playerSNs[i]->m_base.getPos().getZ());
				Vector3 capB = Vector3(RootSceneNode::Instance()->m_playerSNs[i]->m_base.getPos().getX(), 
					minVec.getY() + m_SphereCollider.radius, 
					RootSceneNode::Instance()->m_playerSNs[i]->m_base.getPos().getZ());

				

				//if (RayIntersectSphere(rayOrigin, rayDir, spherePos, m_SphereCollider.radius)) {
				//if(RayIntersectAABB2(rayOrigin, rayDir, minVec, maxVec)) {
				if(IntersectRayCapsule(rayOrigin, rayDir, capA, capB, m_SphereCollider.radius, p1, p2, n1, n2)) {
					/*float distance = sqrt(pow(rayOrigin.m_values[0] - spherePos.m_values[0], 2.0f) +
						pow(rayOrigin.m_values[1] - spherePos.m_values[1], 2.0f) +
						pow(rayOrigin.m_values[2] - spherePos.m_values[2], 2.0f));*/

					closestHit = RootSceneNode::Instance()->m_playerSNs[i];
					Vector3 hitPos = closestHit->m_base.getPos() + Vector3(0.0f, m_SphereCollider.yOffset, 0.0f);

					float distance = sqrt(pow(rayOrigin.m_values[0] - hitPos.m_values[0], 2.0f) +
						pow(rayOrigin.m_values[1] - hitPos.m_values[1], 2.0f) +
						pow(rayOrigin.m_values[2] - hitPos.m_values[2], 2.0f));

					if (distance < minDistance) {
						hitPlayer = true;
						minDistance = distance;
						hitPlayerSN = RootSceneNode::Instance()->m_playerSNs[i];

						/*
						 * Current player cannot get access to the enemy player's ClientNetworkManager
						 * to get the enemy player's client ID. However, since the index of the enemy
						 * player scene node is the same as the enemy player's client network ID,
						 * stored that index into the hitPlayerId and sent it to the server
						 */
						hitPlayerId = i;
					}
					
				}
			}

			/*std::string s = std::to_string(minDistance);
			OutputDebugStringA(s.c_str());
			OutputDebugStringA("   ");*/

			if (hitPlayer) {
				PlayerController *enemyPlayer = hitPlayerSN->getFirstParentByTypePtr<PlayerController>();
				enemyPlayer->m_health -= m_damage;

				ClientNetworkManager *pNetworkManager = (ClientNetworkManager *)(m_pContext->getNetworkManager());
				pNetworkManager->damagePlayer(hitPlayerId, m_damage);
				
				Vector3 color(1.0f, 0.0f, 0.0f);
				Vector3 line1pts[] = { rayOrigin, color, p1, color, };
				DebugRenderer::Instance()->createLineMesh(false, pFirstSN->m_base, &line1pts[0].m_x, 2, 100.0f);
				
				if (enemyPlayer->m_health <= 0) {
					m_score++;
					//This logic will likely happen on reset but we need a scoring system first
					/*for (int i = 0; i < RootSceneNode::Instance()->m_playerSNs.size(); i++) {
						SceneNode* playerSN = RootSceneNode::Instance()->m_playerSNs[i];
						PlayerController* player = playerSN->getFirstParentByTypePtr<PlayerController>();
						player->m_health = 100.0f;
					}*/
					enemyPlayer->m_health = 100.0f;
					PEINFO("PE: Player killed going to send respawn event");
					pNetworkManager->respawnPlayer(hitPlayerId);
				}

				//make the other player flinch
				enemyPlayer->getFirstComponent<CharacterAnimationControler>()->Flinch();
			
				OutputDebugStringA("Hit player\n");
			}
			else {
				OutputDebugStringA("Missed Shot\n");
				Vector3 color(0.0f, 1.0f, 0.0f);
				Vector3 line1pts[] = { rayOrigin, color, 1000.0f * rayDir, color, };
				DebugRenderer::Instance()->createLineMesh(false, pFirstSN->m_base, &line1pts[0].m_x, 2, 100.0f);
			}

		}




		void PlayerController::do_UPDATE(PE::Events::Event *pEvt)
		{
			PE::Events::Event_UPDATE *pRealEvt = (PE::Events::Event_UPDATE *)(pEvt);

			


			if (m_active)
			{
				m_time += pRealEvt->m_frameTime;
				m_networkPingTimer += pRealEvt->m_frameTime;
				shootFrameTime = pRealEvt->m_frameTime;
				keyKleadingEdgeTime = pRealEvt->m_frameTime;

				if (m_gameStarted)
				{
					m_officialGameTime += pRealEvt->m_frameTime;
				}

				keyKleadingEdgeTimer += keyKleadingEdgeTime;
				if (keyKLeadingEdge && (keyKleadingEdgeTimer > 1.5f))
				{
					keyKLeadingEdge = false;
				}
				//m_moveUpdateTimer += pRealEvt->m_frameTime;
				//m_moveUpdateTimer += pRealEvt->m_frameTime;

			
			}

			if (!m_canShoot) {
				shootTimer += shootFrameTime;
				if (shootTimer >= 0.5f) {
					m_canShoot = true;
					shootTimer = 0.0f;
				}
			}


			/*std::string x1 = std::to_string(m_velocity.getX());
			std::string y1= std::to_string(m_velocity.getY());
			std::string z1 = std::to_string(m_velocity.getZ());
			OutputDebugStringA(x1.c_str());
			OutputDebugStringA("  ");
			OutputDebugStringA(y1.c_str());
			OutputDebugStringA("  ");
			OutputDebugStringA(z1.c_str());
			OutputDebugStringA("\n");*/

			/*if (*RootSceneNode::Instance()->m_playerVelocities[m_playerID] == Vector3(0.0f, 0.0f, 0.0f)) {
				OutputDebugStringA("fuck\n");
			}
			else {
				OutputDebugStringA("hmmm\n");
			}
			/*if (m_velocity.m_values[0] != 0.0f) {
				m_velocity.m_values[0] = 0.0f;
			}
			if (m_velocity.m_values[1] != 0.0f) {
				m_velocity.m_values[1] = 0.0f;
			}
			if (m_velocity.m_values[2] != 0.0f) {
				m_velocity.m_values[2] = 0.0f;
			}*/


			PE::Handle hFisrtSN = getFirstComponentHandle<SceneNode>();
			if (!hFisrtSN.isValid())
			{
				assert(!"wrong setup. must have scene node referenced");
				return;
			}

			SceneNode *pFirstSN = hFisrtSN.getObject<SceneNode>();

			if (!canJump) {
				if (isRising) {

					if (jumpUpCounter <= 100) {
						//pFirstSN->m_base.moveUp(0.015f);
						jumpUpCounter++;
						float sinVal = 3.14159f*(float)jumpUpCounter/200.0f;
						pFirstSN->m_base.setPos(Vector3(pFirstSN->m_base.getPos().m_x, sin(sinVal) * 1.2f, pFirstSN->m_base.getPos().m_z));
					}
					else {
						isRising = false;
						jumpUpCounter = 0;
					}
				}

				else {

					if (jumpDownCounter <= 100) {
						//pFirstSN->m_base.moveDown(0.015f);
						jumpDownCounter++;
						float sinVal = (3.14159f / 2.0f) + (3.14159f*(float)jumpDownCounter/200.0f);
						pFirstSN->m_base.setPos(Vector3(pFirstSN->m_base.getPos().m_x, sin(sinVal) * 1.2f, pFirstSN->m_base.getPos().m_z));
					}
					else {
						isRising = true;
						jumpDownCounter = 0;
						canJump = true;
					}
				}
			}
			

			//Gravity -- TODO: need to implement new collider first before handling gravity
			/*float gravityMovement = -1.0f;
			gravityMovement *= (Debug_Fly_Speed * shootFrameTime);
			bool moveDown = true;

			for (int i = 0; i < RootSceneNode::Instance()->m_meshSNs.size(); i++) {

				if (!moveDown) {
					break;
				}

				AABB col = RootSceneNode::Instance()->m_meshSNs[i]->m_AABB;

				Vector3 minVec = RootSceneNode::Instance()->m_meshSNs[i]->m_worldTransform * Vector3(col.minX, col.minY, col.minZ);
				Vector3 maxVec = RootSceneNode::Instance()->m_meshSNs[i]->m_worldTransform * Vector3(col.maxX, col.maxY, col.maxZ);

				if (SphereAABBCollisionCheck(fwdPos, minVec, maxVec, m_SphereCollider.radius)) {
					moveDown = false;
				}
				
			}

			if(moveDown)
				pFirstSN->m_base.moveUp(gravityMovement);*/

			static float x = 0.0f;
			static float y = m_SphereCollider.yOffset;
			static float z = 0.2f;

			// note we could have stored the camera reference in this object instead of searching for camera scene node
			if (CameraSceneNode *pCamSN = pFirstSN->getFirstComponent<CameraSceneNode>())
			{
				pCamSN->m_base.setPos(Vector3(x, y, z));
			}


			if (!m_overriden)
			{
				/*
				if (m_time > 2.0f*PrimitiveTypes::Constants::c_Pi_F32)
				{
				m_time = 0;
				if (m_counter)
				{
				m_counter = 0;
				m_center = Vector2(0,0);
				}
				else
				{
				m_counter = 1;
				m_center = Vector2(10.0f, 0);
				}
				}

				Vector3 pos = Vector3(m_center.m_x, 0, m_center.m_y);
				pos.m_x += (float)cos(m_time) * 5.0f * (m_counter ? -1.0f : 1.0f);
				pos.m_z += (float)sin(m_time) * 5.0f;
				pos.m_y = 0;

				Vector3 fwrd;
				fwrd.m_x = -(float)sin(m_time)  * (m_counter ? -1.0f : 1.0f);
				fwrd.m_z = (float)cos(m_time);
				fwrd.m_y = 0;

				Vector3 right;
				right.m_x = (float)cos(m_time) * (m_counter ? -1.0f : 1.0f) * (m_counter ? -1.0f : 1.0f);
				right.m_z = (float)sin(m_time) * (m_counter ? -1.0f : 1.0f);
				right.m_y = 0;


				pFirstSN->m_base.setPos(m_spawnPos + pos);
				pFirstSN->m_base.setN(fwrd);
				pFirstSN->m_base.setU(right);
				*/
			}
			else
			{
				//send the movement to the animation
				getFirstComponent<CharacterAnimationControler>()->Move(pFirstSN->m_base, m_transformOverride);
				pFirstSN->m_base = m_transformOverride;
			}

			if (m_networkPingTimer > m_networkPingInterval)
			{
				// send client authoritative position event Move Tank C to S 
				//maybe still use for network? keep it simple need a shoot
				//CharacterControl::Events::Event_MoveTank_C_to_S evt(*m_pContext);
				CharacterControl::Events::Event_MovePlayer_C_to_S evt(*m_pContext);
				evt.m_transform = pFirstSN->m_base;
				evt.m_timeStamp = PlayerController::m_officialGameTime;  // Time stamp the packet so we can interpolate on the server

				ClientNetworkManager *pNetworkManager = (ClientNetworkManager *)(m_pContext->getNetworkManager());
				pNetworkManager->getNetworkContext().getEventManager()->scheduleEvent(&evt, m_pContext->getGameObjectManager(), true);

				m_networkPingTimer = 0.0f;
			}
		}

		void PlayerController::overrideTransform(Matrix4x4 &t)
		{
			m_overriden = true;
			m_transformOverride = t;
		}

		void PlayerController::activate()
		{
			m_active = true;

			// this function is called on client tank. since this is client tank and we have client authoritative movement
			// we need to register event handling for movement here.
			// We have 6 tanks total. we activate tank controls controller (in GOM Addon) that will process input events into tank movement events
			// but we want only one tank to process those events. One way to do it is to dynamically add event handlers
			// to only one tank controller. this is what we do here.
			// another way to do this would be to only hae one tank controller, and have it grab one of tank scene nodes when activated
			PE_REGISTER_EVENT_HANDLER(Event_Player_Move, PlayerController::do_Player_Move);
			PE_REGISTER_EVENT_HANDLER(Event_Player_Turn, PlayerController::do_Player_Turn);
			PE_REGISTER_EVENT_HANDLER(Event_Player_Shoot, PlayerController::do_Player_Shoot);
			PE_REGISTER_EVENT_HANDLER(Event_Player_Hurt, PlayerController::do_Player_Hurt);
			

			PE::Handle hFisrtSN = getFirstComponentHandle<SceneNode>();
			if (!hFisrtSN.isValid())
			{
				assert(!"wrong setup. must have scene node referenced");
				return;
			}

			//create camera
			PE::Handle hCamera("Camera", sizeof(Camera));
			Camera *pCamera = new(hCamera) Camera(*m_pContext, m_arena, hCamera, hFisrtSN);
			pCamera->addDefaultComponents();
			CameraManager::Instance()->setCamera(CameraManager::PLAYER, hCamera);

			CameraManager::Instance()->selectActiveCamera(CameraManager::PLAYER);

			//disable default camera controls

			m_pContext->getDefaultGameControls()->setEnabled(false);
			m_pContext->get<CharacterControlContext>()->getSpaceShipGameControls()->setEnabled(false);

			//m_pContext->get<CharacterControlContext>()->getTankGameControls()->setEnabled(false);
			//enable player controls

			m_pContext->get<CharacterControlContext>()->getPlayerGameControls()->setEnabled(true);

			//disable player model
			getFirstComponent<CharacterAnimationControler>()->SetIsPlayer(hFisrtSN.getObject<SceneNode>());
		
			//sound setup

			HWND m_hwnd = 0;

			WinApplication *pWinApp = static_cast<WinApplication*>(m_pContext->getApplication());
			m_hwnd = pWinApp->getWindowHandle();
			m_active = true;
			m_Sound = 0;
			bool result;
			bool testflag = true;
			// Create the sound object.
			m_Sound = new SoundClass;
			if (!m_Sound)
			{
				testflag = false;
			}

			// Initialize the sound object.
			result = m_Sound->Initialize(m_hwnd);
			if (!result)
			{
				//MessageBox(m_hwnd, L"Could not initialize Direct Sound.", L"Error", MB_OK);
				testflag = false;
			}
			
		
		}


	}
}
