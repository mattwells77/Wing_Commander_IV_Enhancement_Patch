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

#include "libvlc_Music.h"
#include "modifications.h"
#include "memwrite.h"
#include "configTools.h"
#include "wc4w.h"


//_________________________________________________
static DWORD Music_Player(MUSIC_CLASS* music_class) {
    //This function runs in a seperate thread.

    if (p_Music_Player)
        delete p_Music_Player;

    p_Music_Player = new LibVlc_Music(music_class);

    //set flags than wait for music data to load.
    music_class->header.flags |= 0x21;
    while (!(music_class->header.flags & 0xC0)) {
        Sleep(64);
        Debug_Info_Music("Music_Player Init: Waiting...");
    }
    Debug_Info_Music("Music_Player Init: DONE");

    //periodically check for tune changes and update.
    while (!(music_class->header.flags & 0x80)) {
        Sleep(64);
        p_Music_Player->Update_Tune();
    }

    if (p_Music_Player->IsPlaying()) {
        p_Music_Player->Stop();
    }

    music_class->header.flags &= 0xFFFFFFFE;

    if (p_Music_Player)
        delete p_Music_Player;
    p_Music_Player = nullptr;

    Debug_Info_Music("Music_Player DONE");
    return 0;
}


//___________________________________________________
static void __declspec(naked) music_thread_play(void) {

    __asm {
        push ebx
        push edi
        push esi
        push ebp

        push ecx
        call Music_Player
        add esp, 0x4

        pop ebp
        pop esi
        pop edi
        pop ebx

        ret
    }
}


//_________________________________________________________________________________________________________________________________________________________________________
static BOOL Read_Music_Data(LONG tune_num, const char* tune_name, void* This, char* path, DWORD dwDesiredAccess, BOOL halt_on_create_file_error, BOOL dwFlagsAndAttributes) {
    Debug_Info_Music("Read_Music_Data flags %d, %d, %d", dwDesiredAccess, halt_on_create_file_error, dwFlagsAndAttributes);
    Debug_Info_Music("Read_Music_Data: tune num: %d, name: %s", tune_num, tune_name);
    Debug_Info_Music("Read_Music_Data: tune path: %s", path);

    BOOL is_file_loaded = wc4_file_load(This, path, dwDesiredAccess, halt_on_create_file_error, dwFlagsAndAttributes);

    if (is_file_loaded == TRUE) {
        MUSIC_FILE* music_file = static_cast<MUSIC_FILE*>(This);
        if (p_Music_Player)
            p_Music_Player->Load_Tune_Data(tune_num, tune_name, music_file);
        Debug_Info_Music("Read_Music_Data: continuous play: %d, dont_interrupt_tune: %d, ?: %d, ?: %d", music_file->DIGM_CDX_01_flag, music_file->dont_interrupt_tune, music_file->DIGM_CDX_03_flag, music_file->DIGM_CDX_04_unk);
    }
    return is_file_loaded;
}


//_________________________________________________
static void __declspec(naked) read_music_data(void) {

    __asm {
        push ebx
        push edi
        push esi
        push ebp

        push[esp + 0x20] //unknown_flag2
        push[esp + 0x20] //unknown_flag1
        push[esp + 0x20] //print_error_flag
        push[esp + 0x20] //path
        push ecx        //this file ptr
        lea eax, [esp + 0x4C]
        push eax        //tune name
        push edi        //tune num
        call Read_Music_Data
        add esp, 0x1C

        pop ebp
        pop esi
        pop edi
        pop ebx

        ret 0x10
    }
}


#ifndef VERSION_WC4_DVD
//_____________________________________________________
static void __declspec(naked) put_tune_num_in_edi(void) {

    __asm {
        mov edi, eax // store tune num in edi for "Read_Music_Data" function.
        //re-insert original code
        lea eax, [edx * 0x2 + eax]
        lea esi, [eax * 0x8 + ecx]
        ret
    }
}
#endif


#ifdef VERSION_WC4_DVD
//________________________
void Modifications_Music() {

    //replace original xanlib music playback thread function.
    MemWrite8(0x444700, 0x83, 0xE9);
    FuncWrite32(0x444701, 0x555364EC, (DWORD)&music_thread_play);

    //initialization: read music wav file data into buffers.
    FuncReplace32(0x444572, 0x0451BA, (DWORD)&read_music_data);
    //initialization: jump over xanlib stuff.
    MemWrite16(0x444576, 0x4C8B, 0x78EB); //JMP SHORT 004445F0
    MemWrite16(0x444578, 0x1024, 0x9090);

    //music starting value 60, should probably be 63
    MemWrite32(0x4B6450, 0x3C, 63);
    //music volume lowered for comms, original -30.
    //adjusted for vlc for a similar effect to -20.
    MemWrite8(0x44F0EA, 0xE2, -MUSIC_VOLUME_VLC_TALK_SUB);
}

#else
//________________________
void Modifications_Music() {

    //replace original xanlib music playback thread function.
    MemWrite8(0x4881E0, 0x83, 0xE9);
    FuncWrite32(0x4881E1, 0x565364EC, (DWORD)&music_thread_play);
 
    //initialization: store tune num in edi for "read_music_data" function.
    MemWrite16(0x487FE0, 0x048D, 0xE890);
    FuncWrite32(0x487FE2, 0xC1348D50, (DWORD)&put_tune_num_in_edi);
    //initialization: read music wav file data into buffers.
    FuncReplace32(0x488065, 0x012A17, (DWORD)&read_music_data);
    //initialization: jump over xanlib stuff.
    MemWrite16(0x488069, 0x8E8B, 0x7BEB);//JMP SHORT 004880E6
    MemWrite32(0x48806B, 0x98, 0x90909090);

    //music starting value 60, should probably be 63
    MemWrite32(0x4D9638, 0x3C, 63);
    //music volume lowered for comms, original -30.
    //adjusted for vlc for a similar effect to -20.
    MemWrite8(0x441B68, 0x1E, MUSIC_VOLUME_VLC_TALK_SUB);
}
#endif