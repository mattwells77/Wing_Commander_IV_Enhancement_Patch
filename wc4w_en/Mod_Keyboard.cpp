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

LONG vdu_comms_selected_line = -1;
BOOL vdu_comms_had_focus = FALSE;
BYTE vdu_comms_highlight[3][256]{ 0x0 };


//________________________________
static LONG VDU_Comms_Check_Keys() {

    LONG key = (LONG)*p_wc4_key_scancode;

    switch (key) {
    case 0x2E://[C] key pressed. cycle though comms list.
        if (vdu_comms_had_focus)
            vdu_comms_selected_line++;
        if (vdu_comms_selected_line >= *p_wc4_vdu_comms_list_size)
            vdu_comms_selected_line = 0;
        //Debug_Info("Num Comms: %d %d", comms_lst_current, num_options);
        break;

    case 1://   [esc] key pressed. return to previous list or exit.
        if (vdu_comms_selected_line > 0)
            vdu_comms_selected_line = 0;
        break;
        //number key pressed.
    case 2://   [1] key press
    case 3://   [2]
    case 4://   [3]
    case 5://   [4]
    case 6://   [5]
    case 7://   [6]
    case 8://   [7]
    case 9://   [8]
    case 10://  [9]
        if (vdu_comms_selected_line < 0)//exit if no item is currently selected(-1).
            break;
        if (key - 2 >= *p_wc4_vdu_comms_list_size)
            vdu_comms_selected_line = 0;
        break;
    case 0x1B://']' key pressed. select highlighted list item.
        if (vdu_comms_selected_line < 0)//exit if no item is currently selected(-1).
            break;
        key = vdu_comms_selected_line + 2;// add 2 to set a number key, as their codes start at '2' which equals the [1] key.
        if (vdu_comms_selected_line > 0)
            vdu_comms_selected_line = 0;
        break;
    case 0x1A://'[' key pressed. return to previous list or exit.
        key = 1;//[esc]
        if (vdu_comms_selected_line > 0)
            vdu_comms_selected_line = 0;
        break;
    default:
        break;
    }

    vdu_comms_had_focus = TRUE;
    return key;
}


//______________________________________________________
static void __declspec(naked) vdu_comms_check_keys(void) {

    __asm {
        push edx
        push ebx
        push edi
        push esi
        push ebp
        push eax

        call VDU_Comms_Check_Keys
        mov ecx, eax

        pop eax
        pop ebp
        pop esi
        pop edi
        pop ebx
        pop edx

        ret
    }
}


//_________________________________________________________________________________________
static BYTE* VDU_Comms_Get_Pal_Highlight_Offsets(BYTE hud_colour_type, BYTE* p_pal_offsets) {

    static bool run_once = false;

    if (hud_colour_type >= 3)//return regular colour if coolour type out of range.
        return p_pal_offsets;
    if (run_once)
        return vdu_comms_highlight[hud_colour_type];

    run_once = true;//run setup once
    //vdu_comms_highlight is an 256 byte array of pal offsets, with 0-254 filled with the same pal offset and 255 set to the value 255 as mask colour.
    //setup arrays for each of the 3 hud colour types.
    //green
    memset(vdu_comms_highlight[0], 0x3B, sizeof(vdu_comms_highlight[0]));
    vdu_comms_highlight[0][255] = 0xFF;
    //blue
    memset(vdu_comms_highlight[1], 0x1B, sizeof(vdu_comms_highlight[1]));
    vdu_comms_highlight[1][255] = 0xFF;
    //red
    memset(vdu_comms_highlight[2], 0x5E, sizeof(vdu_comms_highlight[2]));
    vdu_comms_highlight[2][255] = 0xFF;

    return vdu_comms_highlight[hud_colour_type];
}


//________________________________________________________________________________________________________________________________________________________________
static void VDU_Comms_Draw_Menu_Text(BYTE hud_type, LONG line_num, DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    //highlight menu text if line is selected.
    if (line_num - 1 == vdu_comms_selected_line)
        p_pal_offsets = VDU_Comms_Get_Pal_Highlight_Offsets(hud_type, p_pal_offsets);
    wc4_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
}


//__________________________________________________________
static void __declspec(naked) vdu_comms_draw_menu_text(void) {

    __asm {
        push[esp + 0x18]
        push[esp + 0x18]
        push[esp + 0x18]
        push[esp + 0x18]
        push[esp + 0x18]
        push[esp + 0x18]
#ifdef VERSION_WC4_DVD
        push edi //current line num
#else
        push ebp //current line num
#endif
        mov eax, dword ptr ds : [esi + 0x4]
        mov al, byte ptr ds : [eax + 0x18]
        push eax //hud colour type 0-2. 0=green, 1=blue, 2=red
        call VDU_Comms_Draw_Menu_Text
        add esp, 0x20

        ret
    }
}


//______________________________________________________________
static void __declspec(naked) vdu_check_if_comms_had_focus(void) {

    __asm {
        mov eax, p_wc4_vdu_focus
        mov al, byte ptr ds : [eax]
        cmp al, 0x4 //4 == comms
        je exit_func

        mov vdu_comms_had_focus, FALSE
        exit_func :

        ret
    }
}


//____________________________________________________
static void __declspec(naked) proccess_key_state(void) {

    __asm {
        mov cl, byte ptr ss : [esp + 0x18]//key transition state
        pushad
        push ecx
        push ebx
        call Set_Key_State
        add esp, 0x8
        popad

        //insert original code
        and bl, 0x7F
        cmp bl, 0x2A//compare key scancode to Left Shift
        ret
    }
}


//__________________________________
static BYTE GUI_Alt_X_Message_Loop() {

	BYTE key = 0;
	//wait for alt+x keys to be released.
	do {
		wc4_update_input_states();
		key = Get_Pressed_Key();
	} while (key != 0);

	//wait for key press
	do {
		wc4_update_input_states();
		key = Get_Pressed_Key();
	} while (key == 0);

	//convert key to char. exit char depends on set language, Y for English.
	return MapVirtualKeyA(MapVirtualKeyA(key, MAPVK_VSC_TO_VK), MAPVK_VK_TO_CHAR);
}


//________________________________________________________
static void __declspec(naked) gui_alt_x_message_loop(void) {

	__asm {
		push ebp
		push ebx
		push esi
		push edi

		call GUI_Alt_X_Message_Loop

		pop edi
		pop esi
		pop ebx
		pop ebp
		ret
	}
}

/*
//___________________________
void Print_Scancode(int code) {
	Debug_Info("Key Scancode: %x", code);
}


//___________________________________________________________________
static void __declspec(naked) print_scancode(void) {
	//insert the joystick message check function along with the main message check function.
	__asm {
		pushad
		push ecx
		call Print_Scancode
		add esp, 0x4
		popad
		mov edx, p_wc4_keyboard_state_main
		mov dl, byte ptr ds : [ecx + edx]
		ret
	}
}
*/

#ifdef VERSION_WC4_DVD
//___________________________
void Modifications_Keyboard() {

    //-----scrollable comms------------------------------------------------
    MemWrite8(0x40F2E8, 0xA1, 0xE8);
    FuncWrite32(0x40F2E9, 0x4BBAE0, (DWORD)&vdu_check_if_comms_had_focus);

    MemWrite16(0x40FC6E, 0x0D8B, 0xE890);
    FuncWrite32(0x40FC70, 0x4C4188, (DWORD)&vdu_comms_check_keys);
    //0x2D == key 'C' - 1// allow for 'C' key to be used as menu selector
    MemWrite8(0x40FC79, 0x2D, 0x2C);

    FuncReplace32(0x411251, 0x080154, (DWORD)&vdu_comms_draw_menu_text);
    //---------------------------------------------------------------------

	//update key state
    MemWrite16(0x488F8A, 0xE380, 0xE890);
    FuncWrite32(0x488F8C, 0xFB80577F, (DWORD)&proccess_key_state);
    MemWrite8(0x488F90, 0x2A, 0x57);//push edi

	//wait for yes no input in Alt+X message loop.
	MemWrite8(0x489240, 0x6A, 0xE9);
	FuncWrite32(0x489241, 0xE8006A01, (DWORD)&gui_alt_x_message_loop);
	MemWrite32(0x489245, 0x17, 0x90909090);

	//print scancodes
	//MemWrite16(0x4ADD0E, 0x918A, 0xE890);
	//FuncWrite32(0x4ADD10, 0x4CE7C0, (DWORD)&print_scancode);
}

#else
//___________________________
void Modifications_Keyboard() {

    //-----scrollable comms------------------------------------------------
    MemWrite8(0x427288, 0xA0, 0xE8);
    FuncWrite32(0x427289, 0x4C1244, (DWORD)&vdu_check_if_comms_had_focus);

    MemWrite16(0x427CBD, 0x0D8B, 0xE890);
    FuncWrite32(0x427CBF, 0x4C0660, (DWORD)&vdu_comms_check_keys);
    //0x2D == key 'C' - 1// allow for 'C' key to be used as menu selector
    MemWrite8(0x427CC8, 0x2D, 0x2C);//done

    FuncReplace32(0x425BA9, 0x067134, (DWORD)&vdu_comms_draw_menu_text);
    //---------------------------------------------------------------------
    
	//update key state
    MemWrite16(0x4ADCFA, 0xE380, 0xE890);
    FuncWrite32(0x4ADCFC, 0x2AFB807F, (DWORD)&proccess_key_state);
	
	//wait for yes no input in Alt+X message loop.
	MemWrite8(0x4AE1A0, 0x6A, 0xE9);
	FuncWrite32(0x4AE1A1, 0xE8006A01, (DWORD)&gui_alt_x_message_loop);
	MemWrite32(0x4AE1A5, 0x17, 0x90909090);

	//print scancodes
	//MemWrite16(0x4ADD0E, 0x918A, 0xE890);
	//FuncWrite32(0x4ADD10, 0x4CE7C0, (DWORD)&print_scancode);
}
#endif
