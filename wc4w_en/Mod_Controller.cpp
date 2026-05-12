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

#include "input.h"
#include "input_config.h"
#include "wc4w.h"
#include "memwrite.h"


//______________________
static void Joy_Update() {

	Check_Simulated_Key_For_Release();

	if (controller_enhancements_enabled)
		Joysticks.Update();
	else {
		//using original joystick fuctions.
		wc4_update_joystick();
		wc4_proccess_joystick_data();
		//add joy button to mouse button for double-click check.
		static bool left_click = false;
		if (current_pro_type == PROFILE_TYPE::GUI && *p_wc4_joy_buttons & 1) {
			*p_wc4_mouse_button |= 1;
			left_click = true;
			//clear joy button 1 so as not register as clicked after double-click in GUI. 
			*p_wc4_joy_buttons &= ~1;
		}
		else if (left_click) {
			*p_wc4_mouse_button &= ~1;
			left_click = false;
		}
	}

	Check_Mouse_Double_Click();
}

/*
//______________________
static void Joy_Update() {
	Check_Simulated_Key_For_Release();
	Joysticks.Update();

	Check_Mouse_Double_Click();
}*/

/*
//_________________________________________________
static void __declspec(naked) joy_update_main(void) {

	__asm {
		pushad
		call Joy_Update
		popad
		ret
	}
}*/

//_________________________________________________
static void __declspec(naked) joy_update_main(void) {

	__asm {
		mov eax, p_wc4_key_pressed_character_code
		mov byte ptr ds : [eax] , 0

		pushad
		call Joy_Update
		popad
		ret
	}
}


//____________________________________________________
static void __declspec(naked) joy_update_buttons(void) {

	__asm {
		pushad
		call Joy_Update
		popad
		ret
	}
}


//___________________________________
static void Joystick_Setup(LONG flag) {

	//Debug_Info("Joystick_Setup - flag:%d", flag);
	if (flag == -1)
		JoyConfig_Main();

	if (!controller_enhancements_enabled)
		wc4_setup_joystick(flag);
}


//___________________________________________
static void __declspec(naked) joy_setup(void) {

	__asm {
		mov eax, dword ptr ss : [esp + 0x4]
		push ebx
		push ecx
		push edx
		push edi
		push esi
		push ebp

		push eax
		call Joystick_Setup
		add esp, 0x4

		pop ebp
		pop esi
		pop edi
		pop edx
		pop ecx
		pop ebx

		ret 0x4
	}
}


//___________________________________________________
static void __declspec(naked) joy_roll_variable(void) {

	__asm {
		mov edx, dword ptr ds : [esi + 0xF4]
		mov edi, p_wc4_joy_move_r
		cmp dword ptr ds : [edi] , 0
		je exit_func
		mov edi, dword ptr ds : [edi]
		mov dword ptr ds : [edx + 0x14] , edi
		exit_func :
		ret
	}
}


#ifdef VERSION_WC4_DVD

//__________________________________________
void Modifications_Controller_Enhancements() {

	controller_enhancements_enabled = true;
	
	// get throttle value fixes-------------
	//skip over JOYCAPS.wCaps & JOYCAPS_HASZ
	MemWrite16(0x44B28A, 0x05F6, 0x07EB);

	//skip over JOYCAPS.wZmax - JOYCAPS.wZmin and other maniputations
	MemWrite16(0x44B29D, 0x0D8B, 0x27EB);

	//100 - joy throttle, (SUB ECX, EAX) to (SUB ECX, EDX)
	MemWrite16(0x44B2CB, 0xC82B, 0xD129);
	//--------------------------------------


	//make the roll axis variable -------------
	MemWrite16(0x44BD2B, 0x968B, 0xE890);
	FuncWrite32(0x44BD2D, 0xF4, (DWORD)&joy_roll_variable);
	//----------------------------------------
}


//___________________________
void Modifications_Joystick() {

	FuncReplace32(0x41ABB7, 0x00054565, (DWORD)&joy_setup);
	FuncReplace32(0x44028C, 0x0002EE90, (DWORD)&joy_setup);
	FuncReplace32(0x4564D8, 0x00018C44, (DWORD)&joy_setup);
	FuncReplace32(0x457F45, 0x000171D7, (DWORD)&joy_setup);
	FuncReplace32(0x45B140, 0x00013FDC, (DWORD)&joy_setup);
	FuncReplace32(0x46F408, 0xFFFFFD14, (DWORD)&joy_setup);
	FuncReplace32(0x470218, 0xFFFFEF04, (DWORD)&joy_setup);
	FuncReplace32(0x4705C2, 0xFFFFEB5A, (DWORD)&joy_setup);

	//update controllers before checking window messages.
	MemWrite16(0x489276, 0x05C6, 0xE890);
	FuncWrite32(0x489278, 0x4CD69C, (DWORD)&joy_update_main);
	MemWrite8(0x48927C, 0x00, 0x90);

	//disable calls to update joystick function. This is now done in "joy_update_main". 
	MemWrite8(0x46F4D0, 0xE9, 0xC3);
	MemWrite32(0x46F4D1, 0x01960B, 0x90909090);

	MemWrite8(0x49D4E5, 0xE8, 0x90);
	MemWrite32(0x49D4E6, 0xFFFEB5F6, 0x90909090);

	MemWrite8(0x49F1C9, 0xE8, 0x90);
	MemWrite32(0x49F1CA, 0xFFFE9912, 0x90909090);

	//disable calls to proccess joystick data function. This is now done in "joy_update_main".  
	MemWrite8(0x46F519, 0xE8, 0x90);
	MemWrite32(0x46F51A, 0x000198B2, 0x90909090);

	MemWrite8(0x49D4EC, 0xE8, 0x90);
	MemWrite32(0x49D4ED, 0xFFFEB8DF, 0x90909090);

	MemWrite8(0x49F1D0, 0xE8, 0x90);
	MemWrite32(0x49F1D1, 0xFFFE9BFB, 0x90909090);

	//disable calls to update joystick buttons, for movies. This is now done in "joy_update_main".  
	MemWrite8(0x488BD0, 0x83, 0xC3);
	MemWrite16(0x488BD1, 0x34EC, 0x9090);

	//disable double-click check for movies. This is now done in "Check_Mouse_Double_Click".
	MemWrite8(0x47A9E0, 0xA0, 0xC3);
	MemWrite32(0x47A9E1, 0x4B7774, 0x90909090);


}

#else

//__________________________________________
void Modifications_Controller_Enhancements() {

	controller_enhancements_enabled = true;

	// get throttle value fixes-------------
	//skip over JOYCAPS.wCaps & JOYCAPS_HASZ
	MemWrite16(0x4482CB, 0x28A1, 0x07EB);

	//skip over JOYCAPS.wZmax - JOYCAPS.wZmin
	MemWrite16(0x4482DD, 0x0D8B, 0x0EEB);

	//skip over other maniputations
	MemWrite16(0x4482F2, 0xC22B, 0x0DEB);
	//--------------------------------------


	//make the roll axis variable -------------
	MemWrite16(0x448D2E, 0x968B, 0xE890);
	FuncWrite32(0x448D30, 0xF4, (DWORD)&joy_roll_variable);
	//----------------------------------------
}


//___________________________
void Modifications_Joystick() {

	FuncReplace32(0x401947, 0x00011505, (DWORD)&joy_setup);
	FuncReplace32(0x401D06, 0x00011146, (DWORD)&joy_setup);
	FuncReplace32(0x40AE8D, 0x00007FBF, (DWORD)&joy_setup);
	FuncReplace32(0x4131D8, 0x0FFFFFC74, (DWORD)&joy_setup);
	FuncReplace32(0x43CC38, 0x0FFFD6214, (DWORD)&joy_setup);
	FuncReplace32(0x46B2C8, 0x0FFFA7B84, (DWORD)&joy_setup);
	FuncReplace32(0x46E2E0, 0x0FFFA4B6C, (DWORD)&joy_setup);
	FuncReplace32(0x471CFF, 0x0FFFA114D, (DWORD)&joy_setup);
	FuncReplace32(0x47EF98, 0x0FFF93EB4, (DWORD)&joy_setup);

	//update controllers before checking window messages.
	MemWrite16(0x4AE1D9, 0x1D88, 0xE890);
	FuncWrite32(0x4AE1DB, 0x4DC358, (DWORD)&joy_update_main);

	//disable calls to update joystick function. This is now done in "joy_update_main". 
	MemWrite8(0x4132B0, 0xE9, 0xC3);
	MemWrite32(0x4132B1, 0x09A4EB, 0x90909090);

	MemWrite8(0x498A15, 0xE8, 0x90);
	MemWrite32(0x498A16, 0x014D86, 0x90909090);

	MemWrite8(0x4AAAAE, 0xE8, 0x90);
	MemWrite32(0x4AAAAF, 0x2CED, 0x90909090);

	//disable calls to proccess joystick data function. This is now done in "joy_update_main".  
	MemWrite8(0x413309, 0xE8, 0x90);
	MemWrite32(0x41330A, 0x09A7F2, 0x90909090);

	MemWrite8(0x498A1C, 0xE8, 0x90);
	MemWrite32(0x498A1D, 0x0150DF, 0x90909090);

	MemWrite8(0x4AAAB5, 0xE8, 0x90);
	MemWrite32(0x4AAAB6, 0x3046, 0x90909090);

	//disable calls to update joystick buttons, for movies. This is now done in "joy_update_main".  
	MemWrite8(0x4AD8B0, 0x83, 0xC3);
	MemWrite16(0x4AD8B1, 0x34EC, 0x9090);

	//disable double-click check for movies. This is now done in "Check_Mouse_Double_Click".
	MemWrite8(0x482AE0, 0x83, 0xC3);
	MemWrite16(0x482AE1, 0x10EC, 0x9090);
}
#endif
