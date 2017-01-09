/***************************************

	Joypad/joystick Manager
	Copyright Rebecca Ann Heineman

	Xbox 360 specific version

***************************************/

#include "brjoypad.h"

#if defined(BURGER_XBOX360)
#include "brfixedpoint.h"

#define NOD3D
#define NONET
#include <xtl.h>

/***************************************

	Detect joysticks and init default information

***************************************/

Burger::Joypad::Joypad(GameApp *pAppInstance) :
	m_pAppInstance(pAppInstance),
	m_uDeviceCount(MAXJOYSTICKS)
{
	// Initialize everything
	MemoryClear(m_Data,sizeof(m_Data));
	MemoryClear(m_XInputGamepads,sizeof(m_XInputGamepads));

	//
	// Start allocating joypad devices here
	//
	JoypadData_t *pJoypadData = m_Data;

	Word uXInputCount = MAXJOYSTICKS;
	do {
		pJoypadData->m_uButtonCount = 12;		// XInput manages 12 buttons
		pJoypadData->m_uPOVCount = 1;			// One POV controller
		pJoypadData->m_uAxisCount = 6;			// Two thumbsticks and two triggers
		//pJoypadData->m_bConnected = FALSE;
		++pJoypadData;
	} while (--uXInputCount);

	//
	// All input devices have been logged.
	//


	// Create the digital bounds for all devices
	Word j = 0;
	do {
		Word k = 0;
		do {
			SetDigital(j,k);		// Create the digital bounds
		} while (++k<MAXAXIS);
	} while (++j<MAXJOYSTICKS);

	pAppInstance->AddRoutine(Poll,NULL,this,RunQueue::PRIORITY_JOYPAD);
}

/***************************************

	Shut down the joystick manager

***************************************/

Burger::Joypad::~Joypad()
{
	m_pAppInstance->RemoveRoutine(Poll,this);

	// Make sure the controllers are not rumbling
	XInputStopRumbleOnAllControllers();

	m_uDeviceCount = 0;
}

/***************************************

	Poll the game device

***************************************/

Burger::RunQueue::eReturnCode BURGER_API Burger::Joypad::Poll(void *pData)
{
	Joypad *pThis = static_cast<Joypad *>(pData);

	// Start data scanning here

	JoypadData_t *pJoypadData = pThis->m_Data;

	// Handle XInput devices first

	Word uWhich = 0;
	do {
		XINPUT_STATE State;
		// Test if this was an insertion or removal and report it
		// Get the old and new states

		Word bIsConnected = (XInputGetState(uWhich,&State) == ERROR_SUCCESS);
		Word bWasConnected = pJoypadData->m_bConnected;

		// Save off the states as to how they were processed
		pJoypadData->m_bConnected = static_cast<Word8>(bIsConnected);
		pJoypadData->m_bRemoved = static_cast<Word8>(bWasConnected & (bIsConnected^1));
		pJoypadData->m_bInserted = static_cast<Word8>((bWasConnected^1) & bIsConnected);

		if (bIsConnected) {
			Word32 uButtons;
			Word uXBoxButtons = State.Gamepad.wButtons;

			// The code is the "right" way to do it, but it has
			// too many branches. The code that follows assumes
			// the equates never change, so it uses bit twiddling instead

#if (XINPUT_GAMEPAD_DPAD_LEFT==4) && (XINPUT_GAMEPAD_DPAD_RIGHT==8) && (XINPUT_GAMEPAD_DPAD_UP==1) && (XINPUT_GAMEPAD_DPAD_DOWN==2)
			uButtons = ((uXBoxButtons&(XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_RIGHT))<<6U);	// Bits 8-9
			uButtons += ((uXBoxButtons&(XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN))<<10U);		// Bits 10-11

			uButtons += (uXBoxButtons&(XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_Y));	// Bits 12-15
			uButtons += ((uXBoxButtons&(XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_RIGHT_SHOULDER))<<8U);		// Bits 16-17
			uButtons += ((uXBoxButtons&(XINPUT_GAMEPAD_BACK))<<13U);		// Bit 18
			uButtons += ((uXBoxButtons&(XINPUT_GAMEPAD_START))<<15U);		// Bit 19
			uButtons += ((uXBoxButtons&(XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB))<<14U);			// Bits 20-21
			uButtons += ((uXBoxButtons&(XINPUT_GAMEPAD_BIGBUTTON))<<11);	// Bit 22
#else
			uButtons = 0;
			// The equates have changed! Use the safe code
			if (uXBoxButtons&XINPUT_GAMEPAD_DPAD_LEFT) {
				uButtons = POVLEFT;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_DPAD_RIGHT) {
				uButtons |= POVRIGHT;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_DPAD_UP) {
				uButtons |= POVUP;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_DPAD_DOWN) {
				uButtons |= POVDOWN;
			}

			if (uXBoxButtons&XINPUT_GAMEPAD_A) {
				uButtons |= BUTTON1;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_B) {
				uButtons |= BUTTON2;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_X) {
				uButtons |= BUTTON3;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_Y) {
				uButtons |= BUTTON4;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_LEFT_SHOULDER) {
				uButtons |= BUTTON5;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER) {
				uButtons |= BUTTON6;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_BACK) {
				uButtons |= BUTTON7;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_START) {
				uButtons |= BUTTON8;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_LEFT_THUMB) {
				uButtons |= BUTTON9;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_RIGHT_THUMB) {
				uButtons |= BUTTON10;
			}
			if (uXBoxButtons&XINPUT_GAMEPAD_BIGBUTTON) {
				uButtons |= BUTTON11;
			}
#endif

			// Convert analog directions to digital info
			const JoypadRange_t *pJoypadRange = pJoypadData->m_uAxisDigitalRanges;

			Word uTemp = static_cast<Word16>(State.Gamepad.sThumbLY)^0x8000U;
			pJoypadData->m_uAxis[0] = uTemp;
			if (uTemp<pJoypadRange[0].m_uMin) {		// Test X axis
				uButtons += AXIS1MIN;
			}
			if (uTemp>=pJoypadRange[0].m_uMax) {
				uButtons += AXIS1MAX;
			}

			// Test Y axis
			uTemp = static_cast<Word16>(State.Gamepad.sThumbLX)^0x8000U;
			pJoypadData->m_uAxis[1] = uTemp;
			if (uTemp<pJoypadRange[1].m_uMin) {
				uButtons += AXIS2MIN;
			}
			if (uTemp>=pJoypadRange[1].m_uMax) {
				uButtons += AXIS2MAX;
			}

			uTemp = static_cast<Word16>(State.Gamepad.sThumbRY)^0x8000U;
			pJoypadData->m_uAxis[2] = uTemp;
			if (uTemp<pJoypadRange[2].m_uMin) {		/* Test X axis */
				uButtons += AXIS3MIN;
			}
			if (uTemp>=pJoypadRange[2].m_uMax) {
				uButtons += AXIS3MAX;
			}
			// Test Y axis
			uTemp = static_cast<Word16>(State.Gamepad.sThumbRX)^0x8000U;
			pJoypadData->m_uAxis[3] = uTemp;
			if (uTemp<pJoypadRange[3].m_uMin) {
				uButtons += AXIS4MIN;
			}
			if (uTemp>=pJoypadRange[3].m_uMax) {
				uButtons += AXIS4MAX;
			}

			pJoypadData->m_uAxis[4] = (static_cast<Word32>(State.Gamepad.bLeftTrigger)<<8U)+State.Gamepad.bLeftTrigger;
			pJoypadData->m_uAxis[5] = (static_cast<Word32>(State.Gamepad.bRightTrigger)<<8U)+State.Gamepad.bRightTrigger;

			// Store the button states
			Word32 uCache = (pJoypadData->m_uButtonState ^ uButtons)&uButtons;
			pJoypadData->m_uButtonStatePressed |= uCache;
			pJoypadData->m_uButtonState = uButtons;
		}
		++pJoypadData;
	} while (++uWhich<4);

	return RunQueue::OKAY;
}

/***************************************

	\brief Turn off rumbling on XInput managed controllers

	If XInput is available, this function will clear the vibration state to
	off on all four controllers.

	This function is useful when the application is exiting or entering pause mode

	\windowsonly

	\return Zero if no error. Any other value means an error occurred, ERROR_CALL_NOT_IMPLEMENTED means XInput was not found
	
***************************************/

Word BURGER_API Burger::XInputStopRumbleOnAllControllers(void)
{
	// Set the vibration to off
	XINPUT_VIBRATION MyVibration;
	MyVibration.wLeftMotorSpeed = 0;
	MyVibration.wRightMotorSpeed = 0;

	Word32 i = 0;
	Word uResult = 0;
	do {
		// Only abort if XInput is not present, otherwise, issue the
		// command to every device, regardless of connection state
		Word uTemp = XInputSetState(i,&MyVibration);
		if (uTemp==ERROR_CALL_NOT_IMPLEMENTED) {
			uResult = uTemp;
			break;
		}
	} while (++i<4);
	return uResult;
}

/***************************************

	\brief Read a controller using XInput

	If XInput is available, this function will read in the data from
	a gamepad controller.

	The XInputGamePad_t structure is assumed to be persistent and initialized by the
	application to zero before calling this function with the data. The function will
	maintain state using the contents of the XInputGamePad_t structure during
	successive calls. Calling this function with an XInputGamePad_t structure
	that had never been initially filled with zeros will yield undefined behavior.

	\windowsonly

	\param uWhich Which controller to read, 0-3 are valid
	\param pXInputGamePad Pointer to a structure that will receive the state.
	\param uDeadZoneType Type of post processing on the raw thumbstick data.
	\return Zero if no error. Any other value means an error occurred, ERROR_CALL_NOT_IMPLEMENTED means XInput was not found
	
***************************************/

Word BURGER_API Burger::XInputGetGamepadState(Word uWhich,XInputGamePad_t *pXInputGamePad,eXInputDeadZoneType uDeadZoneType)
{
	Word32 uResult;
	if ((uWhich >= 4) || !pXInputGamePad) {
		uResult = static_cast<Word32>(E_FAIL);
	} else {
		// Read in the data from the game pad
		XINPUT_STATE GamepadState;
		uResult = XInputGetState(uWhich,&GamepadState);

		// Test if XInput is present
		if (uResult!=ERROR_CALL_NOT_IMPLEMENTED) {

			// Test if this was an insertion or removal and report it

			// Get the old and new states
			Word bWasConnected = pXInputGamePad->m_bConnected!=0;	// Force boolean for & below
			Word bIsConnected = (uResult == ERROR_SUCCESS);

			// Save off the states as to how they were processed
			pXInputGamePad->m_bConnected = static_cast<Word8>(bIsConnected);
			pXInputGamePad->m_bRemoved = static_cast<Word8>(bWasConnected & (bIsConnected^1));
			pXInputGamePad->m_bInserted = static_cast<Word8>((bWasConnected^1) & bIsConnected);

			// No error from here on out.
			uResult = 0;

			// Don't update rest of the state if not connected
			if (bIsConnected) {

				// Was this an insertion? If so, nuke the state
				// of the entire structure from orbit, just to be sure

				if (pXInputGamePad->m_bInserted) {
					MemoryClear(pXInputGamePad,sizeof(XInputGamePad_t));
					// Restore these two values because the MemoryClear()
					// erased them
					pXInputGamePad->m_bConnected = TRUE;
					pXInputGamePad->m_bInserted = TRUE;
				}

				// Load in the thumbstick values
				
				Int32 iThumbLX = GamepadState.Gamepad.sThumbLX;
				Int32 iThumbLY = GamepadState.Gamepad.sThumbLY;
				Int32 iThumbRX = GamepadState.Gamepad.sThumbRX;
				Int32 iThumbRY = GamepadState.Gamepad.sThumbRY;
				
				// Do any special processing for the thumb sticks

				// Handle dead zones on a per axis basis
				switch (uDeadZoneType) {
				case XINPUTDEADZONE_CARDINAL:
					// If the values are in the dead zone, set to zero
					if (Abs(iThumbLX) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
						iThumbLX = 0;
					}
					if (Abs(iThumbLY) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
						iThumbLY = 0;
					}
					// Note: The constant is different for the right thumbstick vs the left
					if (Abs(iThumbRX) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
						iThumbRX = 0;
					}
					if (Abs(iThumbRY) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
						iThumbRY = 0;
					}
					break;
				case XINPUTDEADZONE_CENTER:
					// Handle dead zones on a center region basis
					if ((Abs(iThumbLX) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) &&
						(Abs(iThumbLY) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)) {
						iThumbLX = 0;
						iThumbLY = 0;
					}

					if ((Abs(iThumbRX) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) &&
						(Abs(iThumbRY) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)) {
						iThumbRX = 0;
						iThumbRY = 0;
					}
					break;
				default:
					break;
				}

				// Store the final result in the structure
				pXInputGamePad->m_iThumbLX = iThumbLX;
				pXInputGamePad->m_iThumbLY = iThumbLY;
				pXInputGamePad->m_iThumbRX = iThumbRX;
				pXInputGamePad->m_iThumbRY = iThumbRY;
				
				// Store the floating point versions
				pXInputGamePad->m_fThumbLX = static_cast<float>(iThumbLX) * (1.0f/32767.0f);
				pXInputGamePad->m_fThumbLY = static_cast<float>(iThumbLY) * (1.0f/32767.0f);
				pXInputGamePad->m_fThumbRX = static_cast<float>(iThumbRX) * (1.0f/32767.0f);
				pXInputGamePad->m_fThumbRY = static_cast<float>(iThumbRY) * (1.0f/32767.0f);

				// Process the pressed buttons
				Word32 bButtons = GamepadState.Gamepad.wButtons;
				// Using the previous buttons, determine the ones "pressed"
				pXInputGamePad->m_uPressedButtons = (pXInputGamePad->m_uButtons ^ bButtons) & bButtons;
				pXInputGamePad->m_uButtons = bButtons;

				// Process the left trigger
				Word uTriggerValue = GamepadState.Gamepad.bLeftTrigger;
				Word bTriggerIsPressed = (uTriggerValue > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
				Word bTriggerWasPressed = (pXInputGamePad->m_uLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
				pXInputGamePad->m_uLeftTrigger = static_cast<Word8>(uTriggerValue);
				pXInputGamePad->m_bPressedLeftTrigger = static_cast<Word8>(bTriggerIsPressed & (bTriggerWasPressed^1));

				// Process the right trigger
				uTriggerValue = GamepadState.Gamepad.bRightTrigger;
				bTriggerIsPressed = (uTriggerValue > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
				bTriggerWasPressed = (pXInputGamePad->m_uRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
				pXInputGamePad->m_uRightTrigger = static_cast<Word8>(uTriggerValue);
				pXInputGamePad->m_bPressedRightTrigger = static_cast<Word8>(bTriggerIsPressed & (bTriggerWasPressed^1));
			}
		} else {
			// Zap the buffer if there is no XInput
			MemoryClear(pXInputGamePad,sizeof(XInputGamePad_t));
		}
	}
	return uResult;
}
#endif
