/*
The MIT License (MIT)
Copyright © 2026 Matt Wells

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pch.h"

#include "configTools.h"
#include "wc4w.h"
#include "input.h"

using namespace std;

BYTE keyboard_state[256]{ 0 };
BYTE keyboard_state_once[256]{ 0 };


BYTE WC4_ACTIONS_KEYS[][2]{
	0x00, 0x00,		// None,
	0x00, 0x00,		// B1_Trigger,
	0x00, 0x00,		// B2_Modifier,
	0x00, 0x00,		// B3_Missile,
	0x00, 0x00,		// B4_Lock_Closest_Enemy_And_Match_Speed,

	0x00, 0x48,		// Pitch_Down,
	0x00, 0x50,		// Pitch_Up,
	0x00, 0x4B,		// Yaw_Left,
	0x00, 0x4D,		// Yaw_Right,

	0x00, 0x47,		// Pitch Down, Yaw Left,
	0x00, 0x49,		// Pitch Down, Yaw Right,
	0x00, 0x4F,		// Pitch Up, Yaw Left,
	0x00, 0x51,		// Pitch Up, Yaw Right,

	0x00, 0x52,		// Roll_Left,
	0x00, 0x53,		// Roll_Right,
	0x00, 0x2A,		// Double_Yaw_Pich_Roll_Rates,

	0x00, 0x3A,		// Auto_slide
	0x00, 0x35,		// Toggle_Auto_slide
	0x00, 0x0D,		// Accelerate
	0x00, 0x0C,		// Decelerate
	0x00, 0x0E,		// Full_stop
	0x00, 0x2B,		// Full_speed
	//0x00, 0x15,		// Match_target_speed
	0x00, 0x0F,		// Afterburner
	0x00, 0x29,		// Toggle_Afterburner
	0x00, 0x1E,		// Autopilot
	0x00, 0x24,		// Jump
	0x1D, 0x2E,		// Cloak
	0x1D, 0x12,		// Eject
	0x38, 0x19,		// Pause
	//0x38, 0x2E,		// Calibrate_Joystick
	0x38, 0x18,		// Options_Menu
	0x00, 0x31,		// Nav_Map

	0x00, 0x14,		// Cycle_targets
	0x00, 0x13,		// Cycle_turrets
	0x00, 0x26,		// Lock_target
	0x1D, 0x1F,		// Toggle_Smart_Targeting
	0x00, 0x22,		// Cycle_guns
	0x00, 0x21,		// Full_guns
	0x1D, 0x22,		// Synchronize_guns
	0x1D, 0x1E,		// Toggle_Auto_Tracking
	0x00, 0x32,		// Config_Cycle_Missile
	0x00, 0x1B,		// Change_Missile__Increase_power_to_selected_component
	0x00, 0x1A,		// Select_Missile__Decrease_power_to_selected_component
	0x00, 0x30,		// Select_All_Missiles
	//0x00, 0x39,		// Fire_guns
	//0x00, 0x1C,		// Fire_missile
	0x00, 0x12,		// Drop_decoy

	0x00, 0x0B,		// Cycle_VDUs
	0x00, 0x1F,		// VDU_Shield
	0x00, 0x2E,		// VDU_Comms
	0x00, 0x20,		// VDU_Damage
	0x00, 0x11,		// VDU_Weapons
	0x00, 0x19,		// VDU_Power
	0x2A, 0x1B,		// Power_Set_Selected_Component_100
	0x2A, 0x1A,		// Power_Reset_Components_25
	0x1D, 0x1B,		// Lock_power_to_selected_component

	0x00, 0x3B,		// View_Front
	0x00, 0x3C,		// View_Left
	0x00, 0x3D,		// View_Righr
	0x00, 0x3E,		// View_Rear_Turret
	0x1D, 0x3E,		// View_Rear_Turret_VDU
	0x00, 0x3F,		// Camera_Chase
	0x00, 0x40,		// Camera_Object
	//0x00, 0x41,		// Tactical_view
	0x00, 0x42,		// Camera_Missile
	0x00, 0x43,		// Camera_Victim
	0x00, 0x44,		// Camera_Track

	0x1D, 0x2F,		// Disable_Video_In_Left_VDU
	0x00, 0x23,		// Toggle_Normal_Special_Guns
	0x00, 0x16,		// Lock_Closest_Enemy

	0x38, 0x30,		// WingMan_Break_And_Attack,
	0x38, 0x21,		// WingMan_Form_On_Wing,
	0x38, 0x20,		// WingMan_Request_Status,
	0x38, 0x23,		// WingMan_Help_Me_Out,
	0x38, 0x1E,		// WingMan_Attack_My_Target,
	0x38, 0x14,		// Enemy_Taunt,

	0x00, 0x00,		// Remap1
	0x00, 0x00,		// Remap2
	0x00, 0x00,		// Remap2

	0x00, 0x1C,		// Select,
	0x00, 0x0F,		// Cycle_Hotspots,
	0x2A, 0x0F,		// Cycle_Hotspots_Reverse,

	0x1D, 0x4B,		// Gamma_Increase,
	0x1D, 0x4D,		// Gamma_Decrease,

	0x1D, 0x32,		// Toggle_Sound_Effects,
	0x1D, 0x48,		// Sound_Effects_Volume_Increase,
	0x1D, 0x50,		// Sound_Effects_Volume_Decrease,

	0x38, 0x32,		// Toggle_Music,
	0x38, 0x48,		// Music_Volume_Increase,
	0x38, 0x50,		// Music_Volume_Decrease,

	0x00, 0x48,		// Save_Load_Up,
	0x00, 0x50,		// Save_Load_Down,
	0x00, 0x49,		// Save_Load_Page_Up,
	0x00, 0x51,		// Save_Load_Page_Down,

	0x38, 0x2D,		// Exit_Game,
	0x00, 0x15,		// Exit_Game_Yes,
	0x00, 0x31,		// Exit_Game_No,

	0x00, 0x1A,		// Zoom_In,
	0x00, 0x1B,		// Zoom_Out,
	0x00, 0x2E,		// NAV_Centre_View,
	0x00, 0x1F,		// NAV_Toggle_Starfield,
	0x00, 0x22,		// NAV_Toggle_Grid,
	0x00, 0x30,		// NAV_Toggle_Background,
	0x00, 0x31,		// NAV_Cycle_Points,
	0x00, 0x19,		// NAV_Cycle_Points_Reverse,

	0x00, 0x01,		// Escape,
};


//______________________________________
void Set_Key_State(BYTE key, BYTE state) {

	//Debug_Info("Set_Key_State %X, %X", key, state);
	keyboard_state[key] = state;
	keyboard_state_once[key] = state;
}


//_____________________
void Clear_Key_States() {
	memset(keyboard_state, 0, sizeof(keyboard_state));
	memset(keyboard_state_once, 0, sizeof(keyboard_state_once));

	memset(p_wc4_keyboard_state_main, 0, 136);
}


//_____________________________________________________________
bool Get_Key_State(BYTE key, BYTE mod_key_flags, BYTE run_once) {

	bool ret_val = false;
	//mod_key_flags &= 0xF7;// ignore flag 0x8

	BYTE mod_keys = 0;
	mod_keys |= keyboard_state[0x2A];//L Shift
	mod_keys |= keyboard_state[0x36];// R Shift
	mod_keys |= (keyboard_state[0x1D] << 1);// Ctrl
	mod_keys |= (keyboard_state[0x38] << 2);// Alt

	if (run_once & 0x10) {
		if (keyboard_state_once[key] != 0)
			ret_val |= true;
		keyboard_state_once[key] = 0;
	}
	else {
		if (keyboard_state[key] != 0)
			ret_val |= true;
	}

	//always allow these actions to pass whether a mod key is down or not. 
	if (key == WC4_ACTIONS_KEYS[static_cast<int>(WC4_ACTIONS::B1_Fire_Guns)][0] ||
		key == WC4_ACTIONS_KEYS[static_cast<int>(WC4_ACTIONS::Fire_Missile)][0] ||
		key == WC4_ACTIONS_KEYS[static_cast<int>(WC4_ACTIONS::Afterburner)][0])
		return ret_val;

	//if mod keys don't match expected mod keys but mod keys expected or if there are mod keys but no mod keys expected.
	if ((!(mod_keys & mod_key_flags) && mod_key_flags) || (mod_keys && !mod_key_flags))
		return false;

	return ret_val;
}


//____________________
BYTE Get_Pressed_Key() {

	for (int i = 1; i < sizeof(keyboard_state); i++) {
		if (keyboard_state[i])
			return i;
	}
	return 0;
}
