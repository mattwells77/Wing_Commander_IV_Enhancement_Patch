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

#include "modifications.h"
#include "Display_DX11.h"
#include "libvlc_Movies.h"
#include "input_config.h"
#include "wc4w.h"
#include "memwrite.h"
#include "configTools.h"


LibVlc_Movie* pMovie_vlc = nullptr;


//_____________________________________________________________________________________
static BOOL DrawVideoFrame(VIDframe* vidFrame, RGBQUAD* tBuff, UINT tWidth, DWORD flag) {

    static SCALE_TYPE scale_type = SCALE_TYPE::fit;
    static bool linear_upscaling = false;
    static bool run_once = false;
    if (!run_once) {
        run_once = true;
        if (ConfigReadInt(L"MOVIES", L"ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING", CONFIG_MOVIES_ENABLE_ORIGINAL_MOVIES_LIMITED_SCALING))
            scale_type = SCALE_TYPE::fit_best;
        if (ConfigReadInt(L"MOVIES", L"ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING", CONFIG_MOVIES_ENABLE_ORIGINAL_MOVIES_LINEAR_UPSCALING))
            linear_upscaling = true;
    }

    DWORD height = vidFrame->height;
    DWORD width = vidFrame->width;
    DWORD xan_flags = 4;

    if (!*p_wc4_movie_no_interlace) {
        height += height;
        width += width;
        xan_flags |= 2;
    }

    if (!surface_movieXAN || width != surface_movieXAN->GetWidth() || height != surface_movieXAN->GetHeight()) {
        if (surface_movieXAN)
            delete surface_movieXAN;
        surface_movieXAN = new DrawSurface8_RT(0, 0, width, height, 32, 0x00000000, false, 0);
        surface_movieXAN->ScaleTo((float)clientWidth, (float)clientHeight, scale_type);
        if (!linear_upscaling)
            surface_movieXAN->Set_Default_SamplerState(pd3dPS_SamplerState_Point);
        Debug_Info("surface_movieXAN created");
    }
    //Debug_Info("%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X", vidFrame->unknown00, vidFrame->unknown04, vidFrame->unknown08, vidFrame->width, vidFrame->height, vidFrame->unknown14, vidFrame->bitFlag, vidFrame->unknown1C, vidFrame->unknown20, vidFrame->unknown24);

    BYTE* pSurface = nullptr;
    LONG pitch = 0;

    if (surface_movieXAN->Lock((VOID**)&pSurface, &pitch) != S_OK)
        return FALSE;

    (*p_wc4_xanlib_drawframeXD)(vidFrame, pSurface, pitch, xan_flags);

    surface_movieXAN->Unlock();

    return TRUE;
}
void* p_DrawVideoFrame = &DrawVideoFrame;


//__________________________________
static void UnlockShowMovieSurface() {

    if (surface_gui == nullptr)
        return;
    surface_gui->Unlock();
    Display_Dx_Present(PRESENT_TYPE::movie);
}


//__________________________
static void Movie_Fade_Out() {

    LARGE_INTEGER thisTime = { 0LL };
    LARGE_INTEGER nextTime = { 0LL };
    LARGE_INTEGER update_offset{ 0LL };
    update_offset.QuadPart = p_wc4_frequency->QuadPart / 32;
    QueryPerformanceCounter(&thisTime);
    nextTime.QuadPart = thisTime.QuadPart + update_offset.QuadPart;

    int count = 0;

    while (count < 16) {
        QueryPerformanceCounter(&thisTime);
        if (thisTime.QuadPart >= nextTime.QuadPart) {
            nextTime.QuadPart = thisTime.QuadPart + update_offset.QuadPart;
            count++;
            Set_Movie_Fade_Level(count);
            Display_Dx_Present();
        }
    }

    //clear movie buffers before resetting fade level.
    if (pMovie_vlc) {
        DrawSurface* surface = pMovie_vlc->Get_Currently_Playing_Surface();
        if (surface)
            surface->Clear_Texture(0);
    }
    if (surface_movieXAN)
        surface_movieXAN->Clear_Texture(0);
    //set level to 0 to end fade out.
    Set_Movie_Fade_Level(0);
    Display_Dx_Present();
}


//________________________________________________
static void __declspec(naked) movie_fade_out(void) {

    __asm {
        pushad
        call Movie_Fade_Out
        popad

        ret
    }
}


//________________________________________________________
static BOOL Play_HD_Movie_Sequence_Primary(char* mve_path) {

    //char* mve_path = (char*)((DWORD*)p_wc4_movie_class)[28];
    Debug_Info_Movie("Play_HD_Movie_Sequence: main_path: %s", mve_path);
    Debug_Info_Movie("Play_HD_Movie_Sequence: current_list_num: %d", *p_wc4_movie_branch_current_list_num);
    Debug_Info_Movie("Play_HD_Movie_Sequence: first branch: %d", *p_wc4_movie_branch_list);
    //Debug_Info("max branches:%d", ((LONG*)p_wc4_movie_class)[21]);

    if (pMovie_vlc)
        delete pMovie_vlc;
    std::string movie_name;
    Get_Movie_Name_From_Path(mve_path, &movie_name);
    pMovie_vlc = new LibVlc_Movie(movie_name, p_wc4_movie_branch_list, *p_wc4_movie_branch_current_list_num);

    if (pMovie_vlc->IsError()) {
        delete pMovie_vlc;
        pMovie_vlc = nullptr;
        Debug_Info("Play_HD_Movie_Sequence: Failed");
        return FALSE;
    }

    BOOL exit_flag = FALSE;
    BOOL play_successfull = FALSE;
    MOVIE_STATE movie_state{ 0 };

    if (pMovie_vlc->Play()) {
        play_successfull = TRUE;
        while (!exit_flag) {
            wc4_translate_messages_keys();
            //wc4_movie_update_joystick_double_click_exit();
            exit_flag = *p_wc4_movie_halt_flag;//wc4_movie_exit();
            if (Get_Key_State(0x1, 0, 0))
                exit_flag |= TRUE;
            if (!pMovie_vlc->IsPlaying(&movie_state)) {
                *p_wc4_movie_branch_current_list_num = movie_state.list_num;
                if (!movie_state.hasPlayed) {
                    Debug_Info_Movie("Play_HD_Movie_Sequence: ended BAD, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);
                    play_successfull = FALSE;
                }
                else
                    Debug_Info_Movie("Play_HD_Movie_Sequence: ended OK, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);
                exit_flag = TRUE;
            }
        }
        pMovie_vlc->Stop();
    }

    //delete pMovie_vlc;
    //pMovie_vlc = nullptr;

    Sleep(150);//add a small delay to reduce unintended button clicks after ending a movie by double-clicking.

    Debug_Info_Movie("Play_HD_Movie_Sequence: Done:%d", play_successfull);
    return play_successfull;
}


//____________________________________________________________________
static BOOL Play_HD_Movie_Sequence_Main(char* mve_path, BYTE fade_out) {

    DXGI_RATIONAL refreshRate{};
    refreshRate.Denominator = 1;
    refreshRate.Numerator = 3;
    p_wc4_movie_click_time->QuadPart = p_wc4_frequency->QuadPart * refreshRate.Denominator / refreshRate.Numerator;

    wc4_message_check_node_add(wc4_movie_messages);

    if (surface_gui)
        surface_gui->Clear_Texture(0x00);

    //set colour values used by dialogue choice text.
    //selected colour pal offset = 252, unselected colour offset = 253.
    static BYTE text_colour[]{ 255, 255, 255, 80, 80, 80 };
    Palette_Update_Main(text_colour, 252, 2);

    Debug_Info_Movie("Play_HD_Movie_Sequence_Main");
    BOOL play_successfull = TRUE;
    BOOL exit_flag = FALSE;
    while (!exit_flag) {
        wc4_translate_messages_keys();
        //wc4_movie_update_joystick_double_click_exit();
        exit_flag = *p_wc4_movie_halt_flag;//wc4_movie_exit();
        if (Get_Key_State(0x1, 0, 0))
            exit_flag |= TRUE;
        if (Play_HD_Movie_Sequence_Primary(mve_path)) {
            wc4_draw_movie_frame();
            wc4_handle_movie(0);
            *p_wc4_movie_branch_current_list_num = 0;
            if (p_wc4_movie_branch_list[*p_wc4_movie_branch_current_list_num] == -1)
                exit_flag = TRUE;
        }
        else
            exit_flag = TRUE, play_successfull = FALSE;
    }

    wc4_message_check_node_remove(wc4_movie_messages);

    if (surface_gui)
        surface_gui->Clear_Texture(0);
    if (fade_out && play_successfull)
        Movie_Fade_Out();

    Debug_Info_Movie("Play_HD_Movie_Sequence_Main end");
    return play_successfull;
}


//_____________________________________________________________
static void __declspec(naked) play_hd_movie_sequence_main(void) {

    __asm {
        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp

        mov edx, dword ptr ss : [esp + 0x30]//fade_out flag
        mov ecx, dword ptr ss : [esp + 0x20]//path
        push edx
        push ecx
        call Play_HD_Movie_Sequence_Main
        add esp, 0x8

        pop ebp
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx

        cmp eax, FALSE
        je hd_movie_error
        //hd movie played without error.
        add esp, 0x04 //ditch ret address for this function.
        //The next address on the stack is the ret address for the regular movie play back function.
        ret

        hd_movie_error :

#ifndef VERSION_WC4_DVD
        // hd movie had errors, return to wc4 play_movie function to play regular movie.
        //lea ecx, [esp+0x90]
        pop ecx  //store ret address for this function
            sub esp, 0x154//start prologue code for return to regular movie play back function.
            push ecx //re-insert address for this function
#endif
            ret
    }
}


//___________________________________________________________________________________________________________________
static BOOL Play_HD_Movie_Sequence_Secondary(void* p_wc4_movie_class, void* p_sig_movie_class, DWORD sig_movie_flags) {
    LONG length = sig_movie_flags;
    void* xanlib_this = *((void**)p_wc4_movie_class);
    BOOL(__thiscall * p_wc4_xanlib_set_pos)(void*, LONG, LONG*) = (BOOL(__thiscall*)(void*, LONG, LONG*)) * ((void**)xanlib_this + 0x30);

    char* mve_path = (char*)((DWORD*)p_wc4_movie_class)[28];
    Debug_Info_Movie("Play_HD_Movie_Sequence_Secondary:  sig_movie_flags:%X", sig_movie_flags);
    Debug_Info_Movie("Play_HD_Movie_Sequence_Secondary: main_path: %s", mve_path);
    Debug_Info_Movie("Play_HD_Movie_Sequence_Secondary: current_list_num: %d", *p_wc4_movie_branch_current_list_num);
    Debug_Info_Movie("Play_HD_Movie_Sequence_Secondary: first branch: %d", *p_wc4_movie_branch_list);
    //Debug_Info_Movie("max branches:%d", ((LONG*)p_wc4_movie_class)[21]);

    if (pMovie_vlc)
        delete pMovie_vlc;
    std::string movie_name;
    Get_Movie_Name_From_Path(mve_path, &movie_name);
    pMovie_vlc = new LibVlc_Movie(movie_name, p_wc4_movie_branch_list, *p_wc4_movie_branch_current_list_num);

    BOOL exit_flag = FALSE;
    BOOL play_successfull = FALSE;
    MOVIE_STATE movie_state{ 0 };

    if (pMovie_vlc->Play()) {
        play_successfull = TRUE;
        while (!exit_flag) {
            wc4_translate_messages_keys();
            //wc4_movie_update_joystick_double_click_exit();
            exit_flag = *p_wc4_movie_halt_flag;// = wc4_movie_exit();
            if (Get_Key_State(0x1, 0, 0))
                exit_flag |= TRUE;
            if (!pMovie_vlc->IsPlaying(&movie_state)) {
                *p_wc4_movie_branch_current_list_num = movie_state.list_num;
                if (!movie_state.hasPlayed) {
                    Debug_Info_Movie("Play_HD_Movie_Sequence_Secondary: ended BAD, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);
                    //if branch failed to play, shift to the current branch position so the rest of the movie can be played out using the original player.
///                    if (wc4_movie_set_position(p_wc4_movie_class, p_wc4_movie_branch_list[movie_state.list_num]) == FALSE)
///                        Debug_Info_Movie("Play_Movie_Sequence: wc4_movie_set_position Failed, branch:%d", p_wc4_movie_branch_list[movie_state.list_num]);
                    (*p_wc4_xanlib_set_pos)(xanlib_this, *p_wc4_movie_branch_current_list_num, &length);
                    length--;
                    play_successfull = FALSE;
                }
                else
                    Debug_Info_Movie("Play_HD_Movie_Sequence_Secondary: ended OK, branch:%d, listnum:%d", movie_state.branch, movie_state.list_num);

                exit_flag = TRUE;
            }
        }
        pMovie_vlc->Stop();
    }
    //delete pMovie_vlc;
    //pMovie_vlc = nullptr;

    //if alternate movie failed to play, continue movie using original player.
    if (!play_successfull) {
        delete pMovie_vlc;
        pMovie_vlc = nullptr;
        play_successfull = (*p_wc4_xanlib_play)(p_sig_movie_class, (LONG)length);// wc4_sig_movie_play_sequence(p_sig_movie_class, sig_movie_flags);
    }
    else
        *p_wc4_movie_frame_count += 1;//this global needs to be set to evoke the movie fade out function.

    Sleep(150);//add a small delay to reduce unintended button clicks after ending a movie by double-clicking.

    Debug_Info_Movie("Play_HD_Movie_Sequence_Secondary: Done");
    return play_successfull;
}


//__________________________________________________________________
static void __declspec(naked) play_hd_movie_sequence_secondary(void) {

    __asm {
        push ebx
        push ebp

        push[esp + 0xC]//sig movie flags? 0x7FFFFFFF
        push ecx
        push ebp
        call Play_HD_Movie_Sequence_Secondary
        add esp, 0xC

        pop ebp
        pop ebx

        ret 0x4
    }
}
void* p_play_hd_movie_sequence_secondary = &play_hd_movie_sequence_secondary;


//Finds the number of frames between two SMPTE timecode's, these timecodes are 30 fps.
//SMPTE timecode format hh/mm/ss/frames 30fps
//__________________________________________________________________________________________
static LONG Get_Num_Frames_Between_Timecodes_30fps(DWORD timecode_start, DWORD timecode_end) {

    DWORD temp = 0;
    DWORD seconds_30fps = 0;
    DWORD minuts_30fps = 0;
    DWORD hours_30fps = 0;

    DWORD start = timecode_start % 100;
    temp = timecode_start / 100;

    seconds_30fps = temp % 100;
    seconds_30fps *= 30;

    temp /= 100;
    minuts_30fps = temp % 100;
    minuts_30fps *= 1800;

    temp /= 100;
    hours_30fps = temp % 100;
    hours_30fps *= 108000;

    start = start + seconds_30fps + minuts_30fps + hours_30fps;
    //Debug_Info("Get_Time_Position: start frames: %d", start);

    DWORD end = timecode_end % 100;
    temp = timecode_end / 100;

    seconds_30fps = temp % 100;
    seconds_30fps *= 30;

    temp /= 100;
    minuts_30fps = temp % 100;
    minuts_30fps *= 1800;

    temp /= 100;
    hours_30fps = temp % 100;
    hours_30fps *= 108000;

    end = end + seconds_30fps + minuts_30fps + hours_30fps;
    //Debug_Info("Get_Time_Position: end frames: %d", end);
    if (end < start)
        return 0;

    return (end - start);
}


// The below function makes use of data that has it's origin in inflight profile iff files. These are located on the path "\DATA\PROFILE\" within the games ".tre" files.
// Internally they contain an inflight profile form labelled "PROF" and within that a form labelled "RADI" which contains the communication data.
// The relevant sections are:
//
// "FMV " section.
// format structure for eace listed mve file:
// BYTE     ref;                //number reference within the MSGx sections
// BYTE     flag;               //?
// DWORD    tc_start_of_file;   //SMPTE time-code 30fps for the start of file.
// char     file_name[13];      //mve movie file name
//
// "MSGS", "MSGG" and "MSGF" sections for each language.
// format structure for each listed scene:
// BYTE     sond_ref;           //reference to the played audio in the "SOND" section.
// BYTE     fmv_ref;            //reference to a movie in the FMV " section.
// DWORD    tc_start_30fps;     //SMPTE time-code 30fps for the start of scene.
// DWORD    tc_length_30fps;    //SMPTE time-code 30fps for the duration of scene.
// DWORD    neg_offset_15fps;   //subtracted from the video position offset, 15fps.
// char     text[variable];     //subtitle for the particular language.
//
//___________________________________________________________
static BOOL Play_Inflight_Movie(HUD_CLASS_01* p_hud_class_01) {

    if (*pp_movie_class_inflight_01) {
        static LARGE_INTEGER inflight_audio_play_start_time{ 0 };
        static LARGE_INTEGER inflight_audio_play_start_offset{ 0 };

        if (!(*p_wc4_inflight_draw_buff).buff && !pMovie_vlc_Inflight) {
            Debug_Info_Movie("Play_Inflight_Movie: timecodes: tc_start_of_file:%d, tc_start_of_scene:%d, tc_duration/appendix:%d, scene_video_neg_frame_offset:%d", (*pp_movie_class_inflight_01)->timecode_start_of_file_30fps, (*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps, (*pp_movie_class_inflight_01)->timecode_duration_30fps, (*pp_movie_class_inflight_01)->video_frame_offset_15fps_neg);

            //Get the offset with in video file by subtracting the start_of_scene from the start_of_file, value returned is frames at 30fps.
            //This is on occasion used by pilot heads to jump to different scenes but more often then not they reuse the same footage with at different durations to match the audio. 
            LONG video_start_frame = Get_Num_Frames_Between_Timecodes_30fps((*pp_movie_class_inflight_01)->timecode_start_of_file_30fps, (*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps);
            Debug_Info_Movie("Play_Inflight_Movie: Video start offset frames:%d", video_start_frame);
            if (video_start_frame < 0)
                video_start_frame = 0;

            //Supposed to be subtracted from video_start_frame. Makes more sense to me to add it to audio offset. A value of 3 is used by many Pilot Heads, otherwise this is usually zero.
            LONG audio_start_frame = (*pp_movie_class_inflight_01)->video_frame_offset_15fps_neg * 2;

            //inflight_audio_play_start_offset.QuadPart = 0;
            //If the start frame offset is small, delay audio start instead of moving the video position, as libvlc doesn't shift position well.
            //Two of Rollins Victory communications have a small offset like this, Others Victory communications start a zero.
            if (video_start_frame <= 10) {
                audio_start_frame += video_start_frame;
                video_start_frame = 0;
                Debug_Info_Movie("Play_Inflight_Movie: Fixed Video start offset frames:%d", video_start_frame);
                Debug_Info_Movie("Play_Inflight_Movie: Fixed Audio start offset frames:%d", audio_start_frame);
            }

            inflight_audio_play_start_offset.QuadPart = static_cast<long long>(audio_start_frame) * p_wc4_frequency->QuadPart / 30;

            RECT rc_dest{ p_hud_class_01->hud_x + p_hud_class_01->comm_x, p_hud_class_01->hud_y + p_hud_class_01->comm_y,  p_hud_class_01->hud_x + p_hud_class_01->comm_x + (LONG)(*p_movie_class_inflight_02)->width - 1, p_hud_class_01->hud_y + p_hud_class_01->comm_y + (LONG)(*p_movie_class_inflight_02)->height - 1 };
            //Debug_Info_Movie("size:%d,%d,%d,%d", rc_dest.left, rc_dest.top, rc_dest.right, rc_dest.bottom);

            //iff files modified to play movies divided into scenes DON'T INCLUDE an extension in their file name. 
            //if the movie file name has an extension, DON'T ADD a letter appendix by setting "appendix_offset = -1".
            char* ext = strrchr((*pp_movie_class_inflight_01)->file_name, '.');

            if (ext) {
                DWORD length_frames = Get_Num_Frames_Between_Timecodes_30fps(0, (*pp_movie_class_inflight_01)->timecode_duration_30fps);
                pMovie_vlc_Inflight = new LibVlc_MovieInflight((*pp_movie_class_inflight_01)->file_name, &rc_dest, video_start_frame, length_frames);
            }
            else {
                //appendix offsets for individual scene files are stored in "timecode_duration_30fps" 0-26 for letter code.
                DWORD appendix = (*pp_movie_class_inflight_01)->timecode_duration_30fps;
                pMovie_vlc_Inflight = new LibVlc_MovieInflight((*pp_movie_class_inflight_01)->file_name, &rc_dest, appendix);
            }

            if (!pMovie_vlc_Inflight->Play()) {
                delete pMovie_vlc_Inflight;
                pMovie_vlc_Inflight = nullptr;
                Debug_Info_Movie("Play_Inflight_Movie: play failed");
                return FALSE;
            }

            //(*p_wc4_inflight_draw_buff).buff needs to exist to evoke the inflight movie destructor function.
            if (!(*p_wc4_inflight_draw_buff).buff) {
                (*p_wc4_inflight_draw_buff).buff = (BYTE*)wc4_allocate_mem_main((*p_movie_class_inflight_02)->width * (*p_movie_class_inflight_02)->height);
                (*p_wc4_inflight_draw_buff).rc_inv.left = (LONG)(*p_movie_class_inflight_02)->width - 1;
                (*p_wc4_inflight_draw_buff).rc_inv.top = (LONG)(*p_movie_class_inflight_02)->height - 1;
                (*p_wc4_inflight_draw_buff).rc_inv.right = 0;
                (*p_wc4_inflight_draw_buff).rc_inv.bottom = 0;

                (*p_wc4_inflight_draw_buff_main).db = p_wc4_inflight_draw_buff;
                (*p_wc4_inflight_draw_buff_main).rc.left = 0;
                (*p_wc4_inflight_draw_buff_main).rc.top = 0;
                (*p_wc4_inflight_draw_buff_main).rc.right = (LONG)(*p_movie_class_inflight_02)->width - 1;
                (*p_wc4_inflight_draw_buff_main).rc.bottom = (LONG)(*p_movie_class_inflight_02)->height - 1;
            }

            inflight_audio_play_start_time.QuadPart = 0;
            //setting timecode_start_of_scene_30fps = 1 in order allow the audio delay as timecode_start_of_scene_30fps is used as a flag to start the audio when == 0;
            (*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps = 1;
        }

        if (!pMovie_vlc_Inflight) {
            //Debug_Info_Movie("Play_Inflight_Movie: !pMovie_vlc_Inflight failed");
            return FALSE;
        }

        if (!pMovie_vlc_Inflight->Check_Play_Time()) {
            return TRUE;
        }
        // timecode_start_of_scene_30fps is used as a flag to initiate audio playback.
        // setting this here once playback is initialised and audio start time reached.
        if ((*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps != 0) {
            LARGE_INTEGER inflight_video_play_time{};
            QueryPerformanceCounter(&inflight_video_play_time);
            if (inflight_audio_play_start_time.QuadPart == 0)
                inflight_audio_play_start_time.QuadPart = inflight_video_play_time.QuadPart + inflight_audio_play_start_offset.QuadPart;
            if (inflight_audio_play_start_time.QuadPart < inflight_video_play_time.QuadPart)
                (*pp_movie_class_inflight_01)->timecode_start_of_scene_30fps = 0;
        }

        //check if video display dimensions have changed and update if necessary.
        RECT rc_dest{ p_hud_class_01->hud_x + p_hud_class_01->comm_x, p_hud_class_01->hud_y + p_hud_class_01->comm_y,  p_hud_class_01->hud_x + p_hud_class_01->comm_x + (LONG)(*p_movie_class_inflight_02)->width - 1, p_hud_class_01->hud_y + p_hud_class_01->comm_y + (LONG)(*p_movie_class_inflight_02)->height - 1 };
        pMovie_vlc_Inflight->Update_Display_Dimensions(&rc_dest);

        //set the movie class pointer to null to signal to wc3 that the movie has ended.
        if (pMovie_vlc_Inflight && pMovie_vlc_Inflight->HasPlayed()) {
            *pp_movie_class_inflight_01 = nullptr;
            Debug_Info_Movie("Play_Inflight_Movie: Movie Finished");
            return TRUE;
        }

        //clear the rect on the cockpit/hud so that the movie drawn beneath will be visible.
        wc4_copy_rect(p_wc4_inflight_draw_buff_main, 0, 0, *pp_wc4_db_game_main, p_hud_class_01->hud_x + p_hud_class_01->comm_x, p_hud_class_01->hud_y + p_hud_class_01->comm_y, (BYTE)255);
    }
    return TRUE;
}


//_____________________________________________________
static void __declspec(naked) play_inflight_movie(void) {

    __asm {
        push ebx
        push ecx
        push edi
        push esi

        push esi
        call Play_Inflight_Movie
        add esp, 0x4

        pop esi
        pop edi
        pop ecx
        pop ebx

        cmp eax, FALSE
        je play_mve

        pop eax //pop ret address and skip over regular mve playback code 
        jmp p_wc4_play_inflight_hr_movie_return_address

        play_mve :
        mov eax, p_movie_class_inflight_02
            mov eax, dword ptr ds : [eax]
            ret
    }
}


//_________________________________
static void Inflight_Movie_Unload() {
    //check if the finished hd movie has audio, and if so return the volume of the background music to normal setting.
    if (pMovie_vlc_Inflight) {
        if (pMovie_vlc_Inflight->HasAudio()) {
            Debug_Info_Movie("Inflight_Movie_Unload HasAudio - volume returned to normal");
            wc4_set_music_volume(p_wc4_audio_class, *p_wc4_ambient_music_volume);
        }
        //this needs to be set to null to remove the highlight colour from the talking ships target rect.
        *pp_wc4_inflight_audio_ship_ptr_for_rect_colour = nullptr;
        delete pMovie_vlc_Inflight;
        pMovie_vlc_Inflight = nullptr;
        Debug_Info_Movie("Inflight_Movie_Unload done");
    }
}


//_______________________________________________________
static void __declspec(naked) inflight_movie_unload(void) {

    __asm {
        pushad

        call Inflight_Movie_Unload

        popad

        mov eax, p_wc4_inflight_draw_buff
        mov eax, dword ptr ds : [eax]
        ret
    }
}


//______________________________________
static LONG Inflight_Movie_Audio_Check() {
    //check if the hd movie has audio, and if so lower the volume of the background music while playing.
    if (*p_wc4_inflight_audio_ref == 0 && pMovie_vlc_Inflight && pMovie_vlc_Inflight->HasAudio()) {
        Debug_Info_Movie("Inflight_Movie_Check_Audio HasAudio - volume lowered");
        LONG audio_vol = *p_wc4_ambient_music_volume - 30;
        if (audio_vol < 0)
            audio_vol = 0;
        wc4_set_music_volume(p_wc4_audio_class, audio_vol);

        return FALSE;

    }
    return *p_wc4_inflight_audio_unk01;
}


//____________________________________________________________
static void __declspec(naked) inflight_movie_audio_check(void) {

    __asm {
        push eax

        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp

        call Inflight_Movie_Audio_Check

        pop ebp
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx

        cmp eax, 0

        pop eax
        ret
    }
}


//_________________________________________________________________________________
static void Movie_Clear_Choice_Background(BYTE* fBuff, DWORD subY, DWORD subHeight) {
    surface_gui->Clear_Texture(0x00);
}


//______________________________________________________________________________________________________________________________
static void Movie_Draw_Choice_Text(DRAW_BUFFER_MAIN* p_toBuff, LONG x, LONG y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    if (surface_gui == nullptr)
        return;

    bool is_top = false;
    if (y < 240)
        is_top = true;

    LONG movie_height = 0;
    if (pMovie_vlc) {
        DrawSurface* surface = pMovie_vlc->Get_Currently_Playing_Surface();
        movie_height = (LONG)surface->GetScaledHeight();
    }
    else if (surface_movieXAN)
        movie_height = (LONG)surface_movieXAN->GetScaledHeight();

    LONG text_y = 0;
    LONG text_height = 16;
    LONG black_bar_height = (LONG)((480.0f / clientHeight) * ((clientHeight - movie_height) / 2));

    if (black_bar_height >= text_height) {
        text_y = (clientHeight - movie_height) / 4;
        text_y = (480 * text_y) / clientHeight;

        if (is_top)//draw text in the black area above the movie if there is room.
            text_y -= text_height / 2;
        else//draw text in the black area under the movie if there is room.
            text_y = 480 - text_y - text_height / 2;
    }
    else {
        if (is_top)//otherwise draw text over the movie at the top rather than overlapping the black bar.
            text_y = black_bar_height;
        else//otherwise draw text over the movie at the bottom rather than overlapping the black bar.
            text_y = 480 - black_bar_height - text_height;
    }

    if (text_y < 0)
        text_y = 0;
    else if (text_y > 480 - text_height)
        text_y = 480 - text_height;

    BYTE* pSurface_bak = p_toBuff->db->buff;

    BYTE* pSurface = nullptr;

    if (surface_gui->Lock((VOID**)&pSurface, p_wc4_main_surface_pitch) != S_OK)
        return;

    p_toBuff->db->buff = pSurface;
    wc4_draw_text_to_buff(p_toBuff, x, text_y, unk1, text_buff, p_pal_offsets);
    surface_gui->Unlock();

    p_toBuff->db->buff = pSurface_bak;
    Display_Dx_Present(PRESENT_TYPE::movie);
}


//____________________________________________________________________________________________________________________________________
static void Movie_Draw_Choice_Text_Top(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    if (surface_gui)//to clear choise text if resizing during conversation. 
        surface_gui->Clear_Texture(0x0);
    Movie_Draw_Choice_Text(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
}


//_______________________________________________________________________________________________________________________________________
static void Movie_Draw_Choice_Text_Bottom(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    Movie_Draw_Choice_Text(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    Display_Dx_Present(PRESENT_TYPE::movie);
}


//__________________________________________________________________________________________________________________________________________
static void Movie_Draw_Choice_Text_Highlight(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {

    Movie_Draw_Choice_Text(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    Display_Dx_Present(PRESENT_TYPE::movie);
}


#ifdef VERSION_WC4_DVD
//_________________________
void Modifications_Movies() {

    //draw movie frame main
    MemWrite32(0x47A6FE, 0x4D45E4, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x47A7CD, 0x4D45E4, (DWORD)&p_DrawVideoFrame);
    // draw movie frame
    MemWrite32(0x47B63B, 0x4D45E4, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x47B752, 0x4D45E4, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x47B7FE, 0x4D45E4, (DWORD)&p_DrawVideoFrame);

    //in draw movie func, jump over direct draw stuff
    MemWrite8(0x47A4DC, 0x74, 0xEB);
    //in draw movie func
    FuncReplace32(0x47A552, 0xFFFFBF9A, (DWORD)&UnlockShowMovieSurface);

    //in draw movie frame func
    FuncReplace32(0x47B75A, 0xFFFFAD92, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x47B806, 0xFFFFACE6, (DWORD)&UnlockShowMovieSurface);

    //for drawing subtitles - dont know much about these
    FuncReplace32(0x47C573, 0xFFFF9F79, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x47C99F, 0xFFFF9B4D, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x47CAAA, 0xFFFF9A42, (DWORD)&UnlockShowMovieSurface);

    //clear conversation choice text buffers
    MemWrite8(0x4591BA, 0xE8, 0x90);
    MemWrite32(0x4591BB, 0x01E5B1, 0x90909090);
    FuncReplace32(0x4591D3, 0x01E599, (DWORD)&Movie_Clear_Choice_Background);
    MemWrite8(0x45950C, 0xE8, 0x90);
    MemWrite32(0x45950D, 0x01E25F, 0x90909090);
    FuncReplace32(0x459525, 0x01E247, (DWORD)&Movie_Clear_Choice_Background);

    //draw conversation text
    FuncReplace32(0x4598D6, 0x037ACF, (DWORD)&Movie_Draw_Choice_Text_Top);
    FuncReplace32(0x459915, 0x037A90, (DWORD)&Movie_Draw_Choice_Text_Bottom);
    FuncReplace32(0x459A3D, 0x037968, (DWORD)&Movie_Draw_Choice_Text_Highlight);

    //jump redundant blits after drawing choice text.
    MemWrite16(0x459A50, 0xF883, 0x61EB);//JMP SHORT 00459AB3
    MemWrite8(0x459A52, 0x02, 0x90);

    // HD Movies-----------------------------------------------
    //skip is_vob_playback check
    MemWrite16(0x459199, 0x1D39, 0x9090);
    MemWrite32(0x45919B, 0x4B7490, 0x90909090);
    MemWrite16(0x45919F, 0x850F, 0x9090);
    MemWrite32(0x4591A1, 0xDB, 0x90909090);

    //skip is_vob_playback check
    MemWrite16(0x4594EB, 0x1D39, 0x9090);
    MemWrite32(0x4594ED, 0x4B7490, 0x90909090);
    MemWrite16(0x4594F1, 0x850F, 0x9090);
    MemWrite32(0x4594F3, 0xAC, 0x90909090);

    //skip is_vob_playback check
    MemWrite16(0x459642, 0x840F, 0xE990);

    //Replaces the Play_Movie function similar to the dvd version of WC4.
    MemWrite8(0x47ACD0, 0xA1, 0xE8);
    FuncWrite32(0x47ACD1, 0x4B7490, (DWORD)&play_hd_movie_sequence_main);
    //skip vob_playback to allow original movie playback if hd movie playback fails.
    MemWrite16(0x47ACDE, 0xC33B, 0x9090);
    MemWrite8(0x47ACE0, 0x74, 0xEB);

    //backup if the above fails. Here the original movie sequence will play if a HD movie fails.
    MemWrite32(0x47B150, 0x4D461C, (DWORD)&p_play_hd_movie_sequence_secondary);

    //fade out effect for 8bit movies
    FuncReplace32(0x47B239, 0x05F3, (DWORD)&movie_fade_out);
    //fade out effect for 16bit movies
    FuncReplace32(0x47B240, 0x07BC, (DWORD)&movie_fade_out);

    MemWrite8(0x40CE39, 0xA1, 0xE8);
    FuncWrite32(0x40CE3A, 0x4BB9C8, (DWORD)&play_inflight_movie);

    MemWrite8(0x40D1C0, 0xA1, 0xE8);
    FuncWrite32(0x40D1C1, 0x4BB910, (DWORD)&inflight_movie_unload);

    MemWrite16(0x44F09B, 0x3539, 0xE890);
    FuncWrite32(0x44F09D, 0x4B47DC, (DWORD)&inflight_movie_audio_check);
    //---------------------------------------------------------
}

#else
//_________________________
void Modifications_Movies() {

    //draw movie frame main
    MemWrite32(0x482D43, 0x4DE550, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x482E2C, 0x4DE550, (DWORD)&p_DrawVideoFrame);
    // draw movie frame
    MemWrite32(0x483115, 0x4DE550, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x483139, 0x4DE550, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x48322B, 0x4DE550, (DWORD)&p_DrawVideoFrame);

    //in draw movie func, jump over direct draw stuff
    MemWrite8(0x482EE3, 0x74, 0xEB);
    //in draw movie func
    FuncReplace32(0x482F5A, 0xFFF8CE92, (DWORD)&UnlockShowMovieSurface);

    //in draw movie frame func
    FuncReplace32(0x483244, 0xFFF8CBA8, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x4832F9, 0xFFF8CAF3, (DWORD)&UnlockShowMovieSurface);

    //for drawing subtitles - dont know much about these
    FuncReplace32(0x484947, 0xFFF8B4A5, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x484DDE, 0xFFF8B00E, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x484EEA, 0xFFF8AF02, (DWORD)&UnlockShowMovieSurface);

    //clear conversation choice text buffers
    MemWrite8(0x46C5DF, 0xE8, 0x90);
    MemWrite32(0x46C5E0, 0xFFFA495C, 0x90909090);
    FuncReplace32(0x46C5F8, 0xFFFA4944, (DWORD)&Movie_Clear_Choice_Background);
    MemWrite8(0x46C915, 0xE8, 0x90);
    MemWrite32(0x46C916, 0xFFFA4626, 0x90909090);
    FuncReplace32(0x46C92E, 0xFFFA460E, (DWORD)&Movie_Clear_Choice_Background);

    //draw conversation text
    FuncReplace32(0x46CA75, 0x020268, (DWORD)&Movie_Draw_Choice_Text_Top);
    FuncReplace32(0x46CAB5, 0x020228, (DWORD)&Movie_Draw_Choice_Text_Bottom);
    FuncReplace32(0x46CBE3, 0x0200FA, (DWORD)&Movie_Draw_Choice_Text_Highlight);

    //jump redundant blits after drawing choice text.
    MemWrite16(0x46CBFA, 0x4B6A, 0x57EB);//JMP SHORT 0046CC53

    // HD Movies-----------------------------------------------
    //Replaces the Play_Movie function similar to the dvd version of WC4.
    MemWrite16(0x483AD0, 0xEC81, 0xE890);
    FuncWrite32(0x483AD2, 0x0154, (DWORD)&play_hd_movie_sequence_main);

    //backup if the above fails. Here the original movie sequence will play if a HD movie fails.
    MemWrite32(0x483F2D, 0x4DE4E0, (DWORD)&p_play_hd_movie_sequence_secondary);

    //fade out effect for 8bit movies
    FuncReplace32(0x48401C, 0xFFFFF530, (DWORD)&movie_fade_out);
    //fade out effect for 16bit movies
    FuncReplace32(0x484023, 0xFFFFF6D9, (DWORD)&movie_fade_out);

    MemWrite8(0x424722, 0xA1, 0xE8);
    FuncWrite32(0x424723, 0x4D4BD8, (DWORD)&play_inflight_movie);

    MemWrite8(0x424A84, 0xA1, 0xE8);
    FuncWrite32(0x424A85, 0x4C11A8, (DWORD)&inflight_movie_unload);

    MemWrite8(0x441AFE, 0xA1, 0xE8);
    FuncWrite32(0x441AFF, 0x4D55B8, (DWORD)&inflight_movie_audio_check);
    MemWrite16(0x441B03, 0xC085, 0x9090);
    //---------------------------------------------------------
}
#endif