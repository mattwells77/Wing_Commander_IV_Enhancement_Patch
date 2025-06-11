/*
The MIT License (MIT)
Copyright © 2025 Matt Wells

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

#include "wc4w.h"


char* p_wc4_szAppName = nullptr;
HINSTANCE* pp_hinstWC4W = nullptr;

BOOL* p_wc4_window_has_focus = nullptr;
BOOL* p_wc4_is_windowed = nullptr;
HWND* p_wc4_hWinMain = nullptr;
LONG* p_wc4_main_surface_pitch = nullptr;

//DWORD* p_wc4_client_width = nullptr;
//DWORD* p_wc4_client_height = nullptr;

DRAW_BUFFER** pp_wc4_db_game = nullptr;
DRAW_BUFFER_MAIN** pp_wc4_db_game_main = nullptr;

BITMAPINFO** pp_wc4_DIB_Bitmapinfo = nullptr;
VOID** pp_wc4_DIB_vBits = nullptr;

BYTE* p_wc4_main_pal = nullptr;

//LONG* p_wc4_x_centre_cockpit = nullptr;
//LONG* p_wc4_y_centre_cockpit = nullptr;
//LONG* p_wc4_x_centre_rear = nullptr;
//LONG* p_wc4_y_centre_rear = nullptr;
//LONG* p_wc4_x_centre_hud = nullptr;
//LONG* p_wc4_y_centre_hud = nullptr;

SPACE_VIEW_TYPE* p_wc4_space_view_type = nullptr;

BOOL* p_wc4_is_mouse_present = nullptr;
WORD* p_wc4_mouse_button = nullptr;
WORD* p_wc4_mouse_x = nullptr;
WORD* p_wc4_mouse_y = nullptr;

WORD* p_wc4_mouse_button_space = nullptr;
WORD* p_wc4_mouse_x_space = nullptr;
WORD* p_wc4_mouse_y_space = nullptr;

LONG* p_wc4_mouse_centre_x = nullptr;
LONG* p_wc4_mouse_centre_y = nullptr;

LARGE_INTEGER* p_wc4_frequency = nullptr;
LARGE_INTEGER* p_wc4_space_time_max = nullptr;
LARGE_INTEGER* p_wc4_space_time_min = nullptr;
LARGE_INTEGER* p_wc4_space_time_current = nullptr;
LARGE_INTEGER* p_wc4_space_time4 = nullptr;
LARGE_INTEGER* p_wc4_movie_click_time = nullptr;

LONG* p_wc4_joy_dead_zone = nullptr;

DWORD* p_wc4_joy_buttons = nullptr;
DWORD* p_wc4_joy_pov = nullptr;

LONG* p_wc4_joy_move_x = nullptr;
LONG* p_wc4_joy_move_y = nullptr;
LONG* p_wc4_joy_move_r = nullptr;

LONG* p_wc4_joy_move_x_256 = nullptr;
LONG* p_wc4_joy_move_y_256 = nullptr;

LONG* p_wc4_joy_x = nullptr;
LONG* p_wc4_joy_y = nullptr;

LONG* p_wc4_joy_throttle_pos = nullptr;

LONG* p_wc4_ambient_music_volume = nullptr;

BYTE* p_wc4_is_sound_enabled = nullptr;
void* p_wc4_audio_class = nullptr;

DWORD* p_wc4_movie_no_interlace = nullptr;

MOVIE_CLASS_INFLIGHT_01** pp_movie_class_inflight_01 = nullptr;
XAN_CLASS_INFLIGHT_02** p_movie_class_inflight_02 = nullptr;

//a reference to the current sound, 
DWORD* p_wc4_inflight_audio_ref = nullptr;
//not sure what this does, made use of when inserting "Inflight_Movie_Audio_Check" function.
BYTE* p_wc4_inflight_audio_unk01 = nullptr;
//holds a pointer to something relating to the speaking pilot for setting the colour of their targeting rect. 
void** pp_wc4_inflight_audio_ship_ptr_for_rect_colour = nullptr;

//buffer rect structures used for drawing inflight movie frames, re-purposed to create a transparent rect in the cockpit/hud graphic for displaying HR movie's through.
DRAW_BUFFER_MAIN* p_wc4_inflight_draw_buff_main = nullptr;
DRAW_BUFFER* p_wc4_inflight_draw_buff = nullptr;

char** p_wc4_movie_branch_subtitle = nullptr;
BYTE** p_wc4_text_choice_draw_buff = nullptr;
BYTE* p_wc4_movie_colour_bit_level = nullptr;
//return address when playing HR movies to skip over regular movie playback.
void* p_wc4_play_inflight_hr_movie_return_address = nullptr;

void* p_wc4_mouse_struct = nullptr;

void** pp_wc4_music_thread_class = nullptr;
void(__thiscall* wc4_music_thread_class_destructor)(void*) = nullptr;

void(*wc4_dealocate_mem01)(void*) = nullptr;
void(*wc4_unknown_func01)() = nullptr;
void(*wc4_update_input_states)() = nullptr;


BYTE* p_wc4_key_pressed_scancode = nullptr;


void(__thiscall* wc4_draw_hud_targeting_elements)(void*) = nullptr;
void(__thiscall* wc4_draw_hud_view_text)(void*) = nullptr;

void(__thiscall* wc4_nav_screen)(void*) = nullptr;
void(__thiscall* wc4_nav_screen_update_position)(void*) = nullptr;

BYTE(*wc4_translate_messages)(BOOL is_flight_mode, BOOL wait_for_message) = nullptr;

void (*wc4_translate_messages_keys)() = nullptr;

void (*wc4_conversation_decision_loop)() = nullptr;

BOOL(*wc4_message_check_node_add)(BOOL(*)(HWND, UINT, WPARAM, LPARAM)) = nullptr;
BOOL(*wc4_message_check_node_remove)(BOOL(*)(HWND, UINT, WPARAM, LPARAM)) = nullptr;

BOOL(*wc4_movie_messages)(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) = nullptr;

void(*wc4_handle_movie)(BOOL flag) = nullptr;

//BOOL(__thiscall* wc4_sig_movie_play_sequence)(void*, DWORD) = nullptr;

void(*wc4_movie_update_joystick_double_click_exit)() = nullptr;
BOOL(*wc4_movie_exit)() = nullptr;


LONG(*wc4_play_movie)(const char* mve_path, LONG var1, LONG var2, LONG var3, LONG var4, LONG var5, LONG var6) = nullptr;
///BOOL(__thiscall* wc4_movie_set_position)(void*, LONG) = nullptr;///?????????????????????????????
///void(__thiscall* wc4_movie_update_positon)(void*) = nullptr;///?????????????????????????????????????

///CRITICAL_SECTION* p_wc4_movie_criticalsection = nullptr;///??????????????????????????????????
void* p_wc4_movie_class = nullptr;

LONG* p_wc4_movie_branch_list = nullptr;
LONG* p_wc4_movie_branch_current_list_num = nullptr;
//LONG* p_wc4_movie_choice_text_list = nullptr;


LONG* p_wc4_subtitles_enabled = nullptr;
LONG* p_wc4_language_ref = nullptr;

DWORD* p_wc4_key_scancode = nullptr;

bool* p_wc4_movie_halt_flag = nullptr;


BOOL(__thiscall* wc4_load_file_handle)(void*, BOOL print_error_flag, BOOL unknown_flag) = nullptr;
LONG(*wc4_find_file_in_tre)(char* pfile_name) = nullptr;

void (*wc4_copy_rect)(DRAW_BUFFER_MAIN* p_fromBuff, LONG from_x, LONG from_y, DRAW_BUFFER_MAIN* p_toBuff, LONG to_x, LONG to_y, DWORD pal_offset) = nullptr;
LONG(__thiscall* wc4_play_audio_01)(void*, DWORD arg01, DWORD arg02, DWORD arg03, DWORD arg04) = nullptr;
void(__thiscall*wc4_set_music_volume)(void*, LONG level) = nullptr;

void* (*wc4_allocate_mem_main)(DWORD) = nullptr;
void(*wc4_deallocate_mem_main)(void*) = nullptr;

BOOL(**p_wc4_xanlib_drawframeXD)(VIDframe* vidFrame, BYTE* tBuff, UINT tWidth, DWORD flag) = nullptr;
BOOL(__thiscall** p_wc4_xanlib_play)(void*, LONG num) = nullptr;

void(*wc4_draw_choice_text_buff)(void* ptr, BYTE* buff, DWORD flags) = nullptr;
void(*wc4_draw_movie_frame)() = nullptr;


void(*wc4_draw_text_to_buff)(DRAW_BUFFER* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, DWORD unk2) = nullptr;


#ifdef VERSION_WC4_DVD
//_______________
void WC4W_Setup() {

    p_wc4_window_has_focus = (BOOL*)0x4B7784;

    p_wc4_is_windowed = (BOOL*)0x4B720C;

    p_wc4_hWinMain = (HWND*)0x4D20B4;

    p_wc4_szAppName = (char*)0x4B9F40;

    pp_hinstWC4W = (HINSTANCE*)0x4CD6BC;

    p_wc4_main_surface_pitch = (LONG*)0x4B1DC4;

    pp_wc4_DIB_Bitmapinfo = (BITMAPINFO**)0x4C5050;
    pp_wc4_DIB_vBits = (VOID**)0x4C5058;

    pp_wc4_db_game = (DRAW_BUFFER**)0x4C5078;
    pp_wc4_db_game_main = (DRAW_BUFFER_MAIN**)0x4C5074;

    p_wc4_main_pal = (BYTE*)0x4C4D50;
    //p_wc4_x_centre_cockpit = (LONG*)0x4A2D28;
    //p_wc4_y_centre_cockpit = (LONG*)0x4A2D2C;
    //p_wc4_x_centre_rear = (LONG*)0x4A2D48;
    //p_wc4_y_centre_rear = (LONG*)0x4A2D4C;
    //p_wc4_x_centre_hud = (LONG*)0x4A2D38;
    //p_wc4_y_centre_hud = (LONG*)0x4A2D3C;

    p_wc4_space_view_type = (SPACE_VIEW_TYPE*)0x4BB820;

    //p_wc4_client_width = (DWORD*)0x4D40FC;
    //p_wc4_client_height = (DWORD*)0x4D4100;

    p_wc4_is_mouse_present = (BOOL*)0x4CD6AC;

    p_wc4_mouse_button = (WORD*)0x4CD6B0;
    p_wc4_mouse_x = (WORD*)0x4CD6B2;
    p_wc4_mouse_y = (WORD*)0x4CD6B4;

    p_wc4_mouse_button_space = (WORD*)0x4C41A8;
    p_wc4_mouse_x_space = (WORD*)0x4C41AA;
    p_wc4_mouse_y_space = (WORD*)0x4C41AC;

    p_wc4_mouse_centre_x = (LONG*)0x4C4184;
    p_wc4_mouse_centre_y = (LONG*)0x4C4194;

    p_wc4_joy_dead_zone = (LONG*)0x4B7780;

    p_wc4_joy_buttons = (DWORD*)0x4CD1E8;

    p_wc4_joy_move_x = (LONG*)0x4CD1FC;
    p_wc4_joy_move_y = (LONG*)0x4CD1EC;
    p_wc4_joy_move_r = (LONG*)0x4CD3E8;

    p_wc4_joy_move_x_256 = (LONG*)0x4CD3A0;
    p_wc4_joy_move_y_256 = (LONG*)0x4CD200;


    p_wc4_joy_x = (LONG*)0x4CD1A0;
    p_wc4_joy_y = (LONG*)0x4CD3A4;

    p_wc4_joy_throttle_pos = (LONG*)0x4B777C;
    p_wc4_joy_pov = (DWORD*)0x4B7778;

    pp_wc4_music_thread_class = (void**)0x4C269C;
    wc4_music_thread_class_destructor = (void(__thiscall*)(void*))0x444450;

    wc4_dealocate_mem01 = (void(*)(void*))0x48C9E0;

    ///    wc4_unknown_func01 = (void(*)())0x4082C0;

    wc4_update_input_states = (void(*)())0x46F500;

    p_wc4_key_pressed_scancode = (BYTE*)0x4CD69C;


    wc4_translate_messages = (BYTE(*)(BOOL, BOOL))0x489260;

    wc4_translate_messages_keys = (void(*)())0x4890D0;

    wc4_message_check_node_add = (BOOL(*)(BOOL(*pFUNC)(HWND, UINT, WPARAM, LPARAM)))0x489340;
    wc4_message_check_node_remove = (BOOL(*)(BOOL(*pFUNC)(HWND, UINT, WPARAM, LPARAM)))0x489380;

    wc4_movie_messages = (BOOL(*)(HWND, UINT, WPARAM, LPARAM))0x47B320;

    wc4_conversation_decision_loop = (void(*)())0x49DC50;

    wc4_handle_movie = (void(*)(BOOL))0x443A60;

    ///wc4_sig_movie_play_sequence = (BOOL(__thiscall*)(void*, DWORD))0x4DE4E0;

    wc4_movie_update_joystick_double_click_exit = (void(*)())0x47A9E0;
    wc4_movie_exit = (BOOL(*)())0x47AA80;

    wc4_play_movie = (LONG(*)(const char*, LONG, LONG, LONG, LONG, LONG, LONG))0x47ACD0;

    ///    wc4_movie_set_position = (BOOL(__thiscall*)(void*, LONG))0x41EEF0;

    ///    p_wc4_movie_criticalsection = (CRITICAL_SECTION*)0x4A9A80;
    p_wc4_movie_class = (void*)0x4CB9E0;
    ///    wc4_movie_update_positon = (void(__thiscall*)(void*))0x41BDB0;


    p_wc4_movie_branch_list = (LONG*)0x4BC7E8;
    p_wc4_movie_branch_current_list_num = (LONG*)0x4CB9E4;
    //p_wc4_movie_choice_text_list = (LONG*)0x4C7048;


    p_wc4_subtitles_enabled = (LONG*)0x4BC86C;
    p_wc4_language_ref = (LONG*)0x4C41F4;


    p_wc4_key_scancode = (DWORD*)0x4C4188;

    p_wc4_movie_halt_flag = (bool*)0x4CB9D8;


    wc4_draw_choice_text_buff = (void(*)(void*, BYTE*, DWORD))0x469110;


    wc4_draw_hud_targeting_elements = (void(__thiscall*)(void*))0x46B020;

    wc4_draw_hud_view_text = (void(__thiscall*)(void*))0x44ADE0;

    wc4_nav_screen = (void(__thiscall*)(void*))0x445340;

    wc4_nav_screen_update_position = (void(__thiscall*)(void*))0x444900;

    wc4_load_file_handle = (BOOL(__thiscall*)(void*, BOOL, BOOL))0x489730;
    wc4_find_file_in_tre = (LONG(*)(char*))0x48B950;

    p_wc4_frequency = (LARGE_INTEGER*)0x4C4938;
    p_wc4_space_time_max = (LARGE_INTEGER*)0x4C4D48;
    p_wc4_space_time_min = (LARGE_INTEGER*)0x4C4D40;
    p_wc4_space_time_current = (LARGE_INTEGER*)0x4B72A0;
    p_wc4_space_time4 = (LARGE_INTEGER*)0x4C5264;
    p_wc4_movie_click_time = (LARGE_INTEGER*)0x4CB9A0;

    p_movie_class_inflight_02 = (XAN_CLASS_INFLIGHT_02**)0x4BB9C8;
    pp_movie_class_inflight_01 = (MOVIE_CLASS_INFLIGHT_01**)0x4BD448;

    wc4_copy_rect = (void (*)(DRAW_BUFFER_MAIN*, LONG, LONG, DRAW_BUFFER_MAIN*, LONG, LONG, DWORD)) 0x48F64F;
    wc4_play_audio_01 = (LONG(__thiscall*)(void*, DWORD, DWORD, DWORD, DWORD))0x45E6D0;
    wc4_set_music_volume = (void(__thiscall*)(void*, LONG))0x45EA70;

    p_wc4_ambient_music_volume = (LONG*)0x4BD220;
    p_wc4_is_sound_enabled = (BYTE*)0x4C2684;
    p_wc4_audio_class = (void*)0x4C4204;

    p_wc4_inflight_audio_ref = (DWORD*)0x4BD444;
    p_wc4_inflight_audio_unk01 = (BYTE*)0x4B47DC;
    pp_wc4_inflight_audio_ship_ptr_for_rect_colour = (void**)0x4BD440;

    p_wc4_play_inflight_hr_movie_return_address = (void*)0x40D158;

    p_wc4_inflight_draw_buff_main = (DRAW_BUFFER_MAIN*)0x4BB870;
    p_wc4_inflight_draw_buff = (DRAW_BUFFER*)0x4BB910;

    wc4_allocate_mem_main = (void* (*)(DWORD))0x49E6DB;
    wc4_deallocate_mem_main = (void(*)(void*))0x49E862;

    p_wc4_xanlib_drawframeXD = (BOOL(**)(VIDframe*, BYTE*, UINT, DWORD))0x4D45E4;
    p_wc4_xanlib_play = (BOOL(__thiscall**)(void*, LONG))0x4D461C;


    p_wc4_movie_branch_subtitle = (char**)0x4BC730;
    p_wc4_text_choice_draw_buff = (BYTE**)0x4CB9D0;

    p_wc4_movie_colour_bit_level = (BYTE*)0x4CB9C4;

    wc4_draw_movie_frame = (void(*)())0x47B600;

    wc4_draw_text_to_buff = (void(*)(DRAW_BUFFER*, DWORD, DWORD, DWORD, char*, DWORD)) 0x4913A9;

    p_wc4_movie_no_interlace = (DWORD*)0x4C43F0;

    p_wc4_mouse_struct = (void*)0x4D207C;
}

#else
//_______________
void WC4W_Setup() {

    p_wc4_window_has_focus = (BOOL*)0x4DC36C;

    p_wc4_is_windowed = (BOOL*)0x4D40AC;

    p_wc4_hWinMain = (HWND*)0x4DB724;

    p_wc4_szAppName = (char*)0x4DB6F8;

    pp_hinstWC4W = (HINSTANCE*)0x4DBAD4;

    p_wc4_main_surface_pitch = (LONG*)0x4D9834;

    pp_wc4_DIB_Bitmapinfo = (BITMAPINFO**)0x4D4080;
    pp_wc4_DIB_vBits = (VOID**)0x4D4088;

    pp_wc4_db_game = (DRAW_BUFFER**)0x4D40A8;
    pp_wc4_db_game_main = (DRAW_BUFFER_MAIN**)0x4D40A4;

    p_wc4_main_pal = (BYTE*)0x4C01E8;
    //p_wc4_x_centre_cockpit = (LONG*)0x4A2D28;
    //p_wc4_y_centre_cockpit = (LONG*)0x4A2D2C;
    //p_wc4_x_centre_rear = (LONG*)0x4A2D48;
    //p_wc4_y_centre_rear = (LONG*)0x4A2D4C;
    //p_wc4_x_centre_hud = (LONG*)0x4A2D38;
    //p_wc4_y_centre_hud = (LONG*)0x4A2D3C;

    p_wc4_space_view_type = (SPACE_VIEW_TYPE*)0x4D4534;

    //p_wc4_client_width = (DWORD*)0x4D40FC;
    //p_wc4_client_height = (DWORD*)0x4D4100;

    p_wc4_is_mouse_present = (BOOL*)0x4DC35C;
    p_wc4_mouse_button = (WORD*)0x4DC360;
    p_wc4_mouse_x = (WORD*)0x4DC362;
    p_wc4_mouse_y = (WORD*)0x4DC364;

    p_wc4_mouse_button_space = (WORD*)0x4C0670;
    p_wc4_mouse_x_space = (WORD*)0x4C0672;
    p_wc4_mouse_y_space = (WORD*)0x4C0674;

    p_wc4_mouse_centre_x = (LONG*)0x4C0658;
    p_wc4_mouse_centre_y = (LONG*)0x4C0678;

    p_wc4_joy_dead_zone = (LONG*)0x4DC348;

    p_wc4_joy_buttons = (DWORD*)0x4CE3C4;

    p_wc4_joy_move_x = (LONG*)0x4CE560;
    p_wc4_joy_move_y = (LONG*)0x4CE3C0;
    p_wc4_joy_move_r = (LONG*)0x4DC340;

    p_wc4_joy_move_x_256 = (LONG*)0x4CE3B4;
    p_wc4_joy_move_y_256 = (LONG*)0x4CE35C;
  

    p_wc4_joy_x = (LONG*)0x4CE3AC;
    p_wc4_joy_y = (LONG*)0x4CE3BC;

    p_wc4_joy_throttle_pos = (LONG*)0x4DC344;
    p_wc4_joy_pov = (DWORD*)0x4DC33C;

    pp_wc4_music_thread_class = (void**)0x4D961C;
    wc4_music_thread_class_destructor = (void(__thiscall*)(void*))0x487F20;

    wc4_dealocate_mem01 = (void(*)(void*))0x4AF1EF;

///    wc4_unknown_func01 = (void(*)())0x4082C0;

    wc4_update_input_states = (void(*)())0x4132F0;

    p_wc4_key_pressed_scancode = (BYTE*)0x4DC358;


    wc4_translate_messages = (BYTE(*)(BOOL, BOOL))0x4AE1C0;

    wc4_translate_messages_keys = (void(*)())0x4ADE20;

    wc4_message_check_node_add = (BOOL(*)(BOOL(*pFUNC)(HWND, UINT, WPARAM, LPARAM)))0x4AE2A0;
    wc4_message_check_node_remove = (BOOL(*)(BOOL(*pFUNC)(HWND, UINT, WPARAM, LPARAM)))0x4AE2E0;

    wc4_movie_messages = (BOOL(*)(HWND, UINT, WPARAM, LPARAM))0x4827F0;

    wc4_conversation_decision_loop = (void(*)())0x499210;

    wc4_handle_movie = (void(*)(BOOL))0x4754F0;

    ///wc4_sig_movie_play_sequence = (BOOL(__thiscall*)(void*, DWORD))0x4DE4E0;

    wc4_movie_update_joystick_double_click_exit = (void(*)())0x482AE0;
    wc4_movie_exit = (BOOL(*)())0x483340;

    wc4_play_movie = (LONG(*)(const char*, LONG, LONG, LONG, LONG, LONG, LONG))0x483AD0;

///    wc4_movie_set_position = (BOOL(__thiscall*)(void*, LONG))0x41EEF0;

///    p_wc4_movie_criticalsection = (CRITICAL_SECTION*)0x4A9A80;
    p_wc4_movie_class = (void*)0x4D94C8;
///    wc4_movie_update_positon = (void(__thiscall*)(void*))0x41BDB0;


    p_wc4_movie_branch_list = (LONG*)0x4C6FE8;
    p_wc4_movie_branch_current_list_num = (LONG*)0x4D94D0;
    //p_wc4_movie_choice_text_list = (LONG*)0x4C7048;
        
        
    p_wc4_subtitles_enabled = (LONG*)0x4D8720;
    p_wc4_language_ref = (LONG*)0x4B8014;


    p_wc4_key_scancode = (DWORD*)0x4C0660;

    p_wc4_movie_halt_flag = (bool*)0x4D94C0;


    wc4_draw_choice_text_buff = (void(*)(void*, BYTE*, DWORD))0x485260;


    wc4_draw_hud_targeting_elements = (void(__thiscall*)(void*))0x425D90;

    wc4_draw_hud_view_text = (void(__thiscall*)(void*))0x413B20;

    wc4_nav_screen = (void(__thiscall*)(void*))0x41AA00;

    wc4_nav_screen_update_position = (void(__thiscall*)(void*))0x41C340;
  
    wc4_load_file_handle = (BOOL(__thiscall*)(void*, BOOL, BOOL))0x49AA80;
    wc4_find_file_in_tre = (LONG(*)(char*))0x49B5B0;

    p_wc4_frequency = (LARGE_INTEGER*)0x4C01D8;
    p_wc4_space_time_max = (LARGE_INTEGER*)0x4C04E8;
    p_wc4_space_time_min = (LARGE_INTEGER*)0x4C01D0;
    p_wc4_space_time_current = (LARGE_INTEGER*)0x4D4180;
    p_wc4_space_time4 = (LARGE_INTEGER*)0x4D41E8;
    p_wc4_movie_click_time = (LARGE_INTEGER*)0x4C7C80;

    p_movie_class_inflight_02 = (XAN_CLASS_INFLIGHT_02**)0x4D4BD8;
    pp_movie_class_inflight_01 = (MOVIE_CLASS_INFLIGHT_01**)0x4D55D8;

    wc4_copy_rect = (void (*)(DRAW_BUFFER_MAIN*, LONG, LONG, DRAW_BUFFER_MAIN*, LONG, LONG, DWORD)) 0x48AF87;
    wc4_play_audio_01 = (LONG(__thiscall*)(void*, DWORD, DWORD, DWORD, DWORD))0x486420;
    wc4_set_music_volume = (void(__thiscall*)(void*, LONG))0x4867D0;

    p_wc4_ambient_music_volume = (LONG*)0x4C1A98;
    p_wc4_is_sound_enabled = (BYTE*)0x4D95E4;
    p_wc4_audio_class = (void*)0x4B8024;

    p_wc4_inflight_audio_ref = (DWORD*)0x4D55D4;
    p_wc4_inflight_audio_unk01 = (BYTE*)0x4D55B8;
    pp_wc4_inflight_audio_ship_ptr_for_rect_colour = (void**)0x4D55D0;

    p_wc4_play_inflight_hr_movie_return_address = (void*)0x424A58;

    p_wc4_inflight_draw_buff_main = (DRAW_BUFFER_MAIN*)0x4C1190;
    p_wc4_inflight_draw_buff = (DRAW_BUFFER*)0x4C11A8;


    wc4_allocate_mem_main = (void* (*)(DWORD))0x4AD24B;
    wc4_deallocate_mem_main = (void(*)(void*))0x4AD3D2;

    p_wc4_xanlib_drawframeXD = (BOOL(**)(VIDframe*, BYTE*, UINT, DWORD))0x4DE550;
    p_wc4_xanlib_play = (BOOL(__thiscall**)(void*, LONG))0x4DE4E0;


    p_wc4_movie_branch_subtitle = (char**)0x4C7048;
    p_wc4_text_choice_draw_buff = (BYTE**)0x4D94B8;
    p_wc4_movie_colour_bit_level = (BYTE*)0x4D94A8;

    wc4_draw_movie_frame = (void(*)())0x4830D0;

    wc4_draw_text_to_buff = ( void(*)(DRAW_BUFFER*, DWORD, DWORD, DWORD, char*, DWORD)) 0x48CCE1;

    p_wc4_movie_no_interlace = (DWORD*)0x4D3044;

    p_wc4_mouse_struct = (void*)0x4CE848;
}
#endif
