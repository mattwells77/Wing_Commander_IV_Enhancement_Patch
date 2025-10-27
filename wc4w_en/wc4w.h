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

#pragma once

#define GUI_WIDTH 640
#define GUI_HEIGHT 480

struct DRAW_BUFFER {
    BYTE* buff;
    RECT rc_inv;//rect inverted
};

struct DRAW_BUFFER_MAIN {
    DRAW_BUFFER* db;
    RECT rc;
};

struct VIDframe {
    DWORD* unknown00;//0x00
    DWORD unknown04;//0x04
    DWORD unknown08;//0x08
    DWORD width;//0x0C
    DWORD height;//0x10
    DWORD unknown14;//0x14
    DWORD bitFlag;//0x18
    DWORD unknown1C;//0x1C
    DWORD unknown20;//0x20
    DWORD unknown24;//0x24
    BYTE* buff;//0x28
    BYTE* unknown2C;//0x2C
    BYTE* unknown30;//0x30
    BYTE* unknown34;//0x34
    BYTE* unknown38;//0x38
};

//this structure is very incomplete and likely larger than specified.
struct MOVIE_CLASS_01 {
    DWORD unk00;
    DWORD unk04;
    DWORD unk08;
    DWORD unk0C;
    DWORD unk10;
    DWORD unk14;
    DWORD unk18;
    DWORD unk1C;
    DWORD unk20;
    DWORD unk24;//*p_func(Arg1 BYTE *p_buff)
    DWORD unk28;
    DWORD unk2C;
    DWORD unk30;
    DWORD unk34;
    DWORD unk38;
    DWORD unk3C;
    DWORD unk40;
    DWORD unk44;
    DWORD unk48;
    DWORD unk4C;
    DWORD unk50;
    DWORD branch_max;//0x54
    DWORD unk58;
    DWORD unk5C;
    DWORD branch_current;//0x60
    DWORD unk64;
    DWORD unk68;
    DWORD unk6C;
    char* path;//0x70
    DWORD unk74;
    DWORD unk78;
    DWORD unk7C;
};


//this structure is very incomplete and likely larger than specified.
struct HUD_CLASS_01 {
    DWORD unk00;
    DWORD unk04;
    DWORD unk08;
    DWORD unk0C;
    DWORD unk10;
    DWORD unk14;
    DWORD unk18;
    DWORD unk1C;
    DWORD unk20;
    DWORD unk24;
    LONG hud_x;//x1
    LONG hud_y;//y1
    DWORD unk30;
    DWORD unk34;
    DWORD unk38;
    DWORD unk3C;
    DWORD unk40;
    DWORD unk44;
    DWORD unk48;
    DWORD unk4C;
    LONG comm_x;//x2
    LONG comm_y;//y2
    DWORD unk58;
    DWORD unk5C;

};

//this structure is very incomplete and likely larger than specified.
struct MOVIE_CLASS_INFLIGHT_01 {
    DWORD unk00;
    DWORD flags;//0x04
    DWORD timecode_start_of_file_30fps;//0x08
    DWORD timecode_duration_30fps;//0x0C
    DWORD video_frame_offset_15fps_neg;//0x10
    DWORD timecode_start_of_scene_30fps;//0x14
    char file_name[16];//0x18
    DWORD unk28;
    DWORD unk2C;
    DWORD unk30;
    DWORD unk34;
    DWORD unk38;
    DWORD unk3C;
    DWORD unk40;
    DWORD unk44;
    DWORD unk48;
};

//this structure is very incomplete and likely larger than specified.
struct MOVIE_CLASS_INFLIGHT_02 {
    MOVIE_CLASS_01* p_movie_class;//0x00
    char* file_path;//0x04
    DWORD unk08;//0x08
    DWORD width;//0x0C
    DWORD height;//0x10
    LONG current_frame;//0x14
};



struct XAN_CLASS_INFLIGHT_02 {
    DWORD unk00;
    DWORD unk04;
    DWORD unk08;
    DWORD unk0C;
    DWORD unk10;
    DWORD unk14;
    DWORD unk18;
    DWORD unk1C;
    DWORD unk20;
    DWORD width;//0x24
    DWORD height;//0x28
};

enum class SPACE_VIEW_TYPE : WORD {
    Cockpit = 0,
    CockLeft = 1,
    CockRight = 2,
    CockBack = 3,
    Chase = 4,
    Rotate = 5,
    CockHud = 6,
    UNK07 = 7,
    UNK08 = 8,
    NavMap = 9,
    UNK10 = 10,
    Track = 11,
};

//this structure is very incomplete and likely larger than specified.
struct CAMERA_CLASS_01 {
    DWORD unk00;//0x00
    DWORD unk04;//0x04
    DWORD unk08;//0x08
    SPACE_VIEW_TYPE view_type;//0x0C
    //WORD unk05;//0x10
};

void WC4W_Setup();

extern DWORD* p_wc4_virtual_alloc_mem_size;

extern char* p_wc4_szAppName;
extern HINSTANCE* pp_hinstWC4W;

extern BOOL* p_wc4_window_has_focus;
extern BOOL* p_wc4_is_windowed;
extern HWND* p_wc4_hWinMain;
extern LONG* p_wc4_main_surface_pitch;

extern DRAW_BUFFER** pp_wc4_db_game;
extern DRAW_BUFFER_MAIN** pp_wc4_db_game_main;

extern BITMAPINFO** pp_wc4_DIB_Bitmapinfo;
extern VOID** pp_wc4_DIB_vBits;

extern BYTE* p_wc4_main_pal;

extern LARGE_INTEGER* p_wc4_frequency;
extern LARGE_INTEGER* p_wc4_space_time_max;
extern LARGE_INTEGER* p_wc4_movie_click_time;

//extern LONG* p_wc4_x_centre_cockpit;
//extern LONG* p_wc4_y_centre_cockpit;
//extern LONG* p_wc4_x_centre_rear;
//extern LONG* p_wc4_y_centre_rear;
//extern LONG* p_wc4_x_centre_hud;
//extern LONG* p_wc4_y_centre_hud;

extern SPACE_VIEW_TYPE* p_wc4_view_current_dir;
extern CAMERA_CLASS_01* p_wc4_camera_01;

extern void** pp_wc4_player_obj_struct;

extern BOOL* p_wc4_is_mouse_present;

extern WORD* p_wc4_mouse_button;
extern WORD* p_wc4_mouse_x;
extern WORD* p_wc4_mouse_y;

extern WORD* p_wc4_mouse_button_space;
extern WORD* p_wc4_mouse_x_space;
extern WORD* p_wc4_mouse_y_space;

extern LONG* p_wc4_mouse_centre_x;
extern LONG* p_wc4_mouse_centre_y;

extern LONG* p_wc4_joy_dead_zone;

extern DWORD* p_wc4_joy_buttons;
extern DWORD* p_wc4_joy_pov;

extern LONG* p_wc4_joy_move_x;
extern LONG* p_wc4_joy_move_y;
extern LONG* p_wc4_joy_move_r;

extern LONG* p_wc4_joy_move_x_256;
extern LONG* p_wc4_joy_move_y_256;

extern LONG* p_wc4_joy_x;
extern LONG* p_wc4_joy_y;

extern LONG* p_wc4_joy_throttle_pos;


///extern CRITICAL_SECTION* p_wc4_movie_criticalsection;
extern void* p_wc4_movie_class;

extern LONG* p_wc4_movie_branch_list;
extern LONG* p_wc4_movie_branch_current_list_num;
//extern LONG* p_wc4_movie_choice_text_list;


extern bool* p_wc4_movie_halt_flag;

extern DWORD* p_wc4_movie_frame_count;

extern LONG* p_wc4_subtitles_enabled;
extern LONG* p_wc4_language_ref;

extern MOVIE_CLASS_INFLIGHT_01** pp_movie_class_inflight_01;
extern XAN_CLASS_INFLIGHT_02** p_movie_class_inflight_02;

extern LONG* p_wc4_ambient_music_volume;

extern BYTE* p_wc4_is_sound_enabled;
extern void* p_wc4_audio_class;

extern DWORD* p_wc4_movie_no_interlace;

//a reference to the current sound, 
extern DWORD* p_wc4_inflight_audio_ref;
//not sure what this does, made use of when inserting "Inflight_Movie_Audio_Check" function.
extern BYTE* p_wc4_inflight_audio_unk01;
//holds a pointer to something relating to the speaking pilot for setting the colour of their targeting rect. 
extern void** pp_wc4_inflight_audio_ship_ptr_for_rect_colour;


//buffer rect structures used for drawing inflight movie frames, re-purposed to create a transparent rect in the cockpit/hud graphic for displaying HR movie's through.
extern DRAW_BUFFER_MAIN* p_wc4_inflight_draw_buff_main;
extern DRAW_BUFFER* p_wc4_inflight_draw_buff;

extern char** p_wc4_movie_branch_subtitle;
extern BYTE** p_wc4_text_choice_draw_buff;
extern BYTE* p_wc4_movie_colour_bit_level;

//return address when playing HR movies to skip over regular movie playback.
extern void* p_wc4_play_inflight_hr_movie_return_address;

extern void* p_wc4_mouse_struct;


extern void(__thiscall* wc4_draw_hud_targeting_elements)(void*);
extern void(__thiscall* wc4_draw_hud_view_text)(void*);

extern void(__thiscall* wc4_nav_screen)(void*);

extern void** pp_wc4_music_thread_class;
extern void(__thiscall* wc4_music_thread_class_destructor)(void*);


extern void(*wc4_dealocate_mem01)(void*);
extern void(*wc4_unknown_func01)();
extern void(*wc4_update_input_states)();
extern BYTE(*wc4_translate_messages)(BOOL is_flight_mode, BOOL wait_for_message);
extern void (*wc4_translate_messages_keys)();
extern void (*wc4_conversation_decision_loop)();
extern void(*wc4_draw_choice_text_buff)(void* ptr, BYTE* buff, DWORD flags);
extern void(*wc4_draw_movie_frame)();
extern void(*wc4_handle_movie)(BOOL flag);


extern void(__thiscall* wc4_nav_screen_update_position)(void*);

///extern BOOL(__thiscall* wc4_movie_set_position)(void*, LONG);
///extern void(__thiscall* wc4_movie_update_positon)(void*);
///extern BOOL(__thiscall* wc4_sig_movie_play_sequence)(void*, DWORD);

extern void(*wc4_movie_update_joystick_double_click_exit)();
extern BOOL(*wc4_movie_exit)();

extern BOOL (*wc4_message_check_node_add)(BOOL(*)(HWND, UINT, WPARAM, LPARAM));
extern BOOL (*wc4_message_check_node_remove)(BOOL(*)(HWND, UINT, WPARAM, LPARAM));

extern BOOL(*wc4_movie_messages)(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

extern BOOL(__thiscall* wc4_load_file_handle)(void*, BOOL print_error_flag, BOOL unknown_flag);
extern LONG(*wc4_find_file_in_tre)(char* pfile_name);
//pal_offset  colour (0-255) for filled rect, if pal_offset above 255 from-buffer is copied to to-buff.
extern void (*wc4_copy_rect)(DRAW_BUFFER_MAIN* p_fromBuff, LONG from_x, LONG from_y, DRAW_BUFFER_MAIN* p_toBuff, LONG to_x, LONG to_y, DWORD pal_offset);
extern LONG(__thiscall* wc4_play_audio_01)(void*, DWORD arg01, DWORD arg02, DWORD arg03, DWORD arg04);

extern void(__thiscall*wc4_set_music_volume)(void*, LONG level);

extern void*(*wc4_allocate_mem_main)(DWORD);
extern void(*wc4_deallocate_mem_main)(void*);

extern BOOL(**p_wc4_xanlib_drawframeXD)(VIDframe* vidFrame, BYTE* tBuff, UINT tWidth, DWORD flag);
extern BOOL(__thiscall** p_wc4_xanlib_play)(void*, LONG num);

extern void(*wc4_draw_text_to_buff)(DRAW_BUFFER* p_toBuff, DWORD x, DWORD y, DWORD unk1 ,char* text_buff, DWORD unk2);
