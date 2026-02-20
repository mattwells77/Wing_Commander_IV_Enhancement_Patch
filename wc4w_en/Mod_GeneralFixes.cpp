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
#include "modifications.h"
#include "memwrite.h"
#include "configTools.h"
#include "wc4w.h"


//Look for and load files located in the "theGameDir"\\data folder in place of files located in the .tre archives.
//__________________________________________
static BOOL Load_Data_File(char* pfile_name) {

    //"..\\..\\" signifies that the file is located in a .tre archive.
    if (strncmp(pfile_name, "..\\..\\", 6) == 0) {

        DWORD file_attributes = GetFileAttributesA(pfile_name + 4);
        //check if the file exists under relative path \data
        if (file_attributes != INVALID_FILE_ATTRIBUTES && !(file_attributes & FILE_ATTRIBUTE_DIRECTORY) ) {
            size_t file_name_len = strlen(pfile_name) + 1;

            char* file_name_backup = new char[file_name_len + 1];
            strncpy_s(file_name_backup, file_name_len, pfile_name, file_name_len);
            //change the path removing the path intro leaving .\data\"etc"
            strncpy_s(pfile_name, file_name_len, file_name_backup + 4, file_name_len - 4);
            delete[] file_name_backup;
            Debug_Info("Load_Data_File File FOUND: %s", pfile_name);
        } 
    }
    //Debug_Info("Load_Data_File: %s", pfile_name);
    return wc4_find_file_in_tre(pfile_name);
}


//________________________________________________
static void __declspec(naked) load_data_file(void) {

    __asm {
        mov eax, [esp + 0x4]//pointer to file name in file_class

        push ebp
        push esi
        push ebx

        push eax
        call Load_Data_File
        add esp, 0x4

        pop ebx
        pop esi
        pop ebp

        ret
    }
}


/*
static void Print_Closed_Handle(BOOL close_good, void* p_this_class) {
    Debug_Info("Print_Closed_Handle: %s, close_flag_good_zero:%d", p_this_class, close_good);
}


void* p_close_file_handle = (void*)0x49BB70;
//______________________________________________________
static void __declspec(naked) close_file_handle(void) {

    __asm {
        push eax
        call p_close_file_handle
        add esp, 0x4

        pushad
        push esi
        push eax
        call Print_Closed_Handle
        add esp, 0x8
        popad
        ret
    }
}
*/

//Fixed a code error on a call to the "VirtualProtect" function, where the "lpflOldProtect" parameter was set to NULL when it should point to a place to store the previous access protection value.
//______________________________
static void VirtualProtect_Fix() {
    DWORD oldProtect;
#ifdef VERSION_WC4_DVD
    VirtualProtect((LPVOID)0x4950B0, 0x49C7DE - 0x4950B0, PAGE_EXECUTE_READWRITE, &oldProtect);
#else
    VirtualProtect((LPVOID)0x48D864, 0x494F92 - 0x48D864, PAGE_EXECUTE_READWRITE, &oldProtect);
#endif
}


//____________________________________________________
static void __declspec(naked) virtualprotect_fix(void) {

    __asm {
        pushad
        call VirtualProtect_Fix
        popad
        ret
    }
}


//_____________________________________________________________
//Check if an alterable file exists in either the Application folder or UAC data folder. 
static DWORD __stdcall GetFileAttributes_UAC(LPCSTR lpFileName) {
    const char* pos = StrStrIA(lpFileName, ".WSG");//check if saved game file.
    if (!pos)
        pos = StrStrIA(lpFileName, "SFOSAVED.DAT");//check if settings file.
    if (!pos)
        pos = StrStrIA(lpFileName, "WC4.CFG");//check if default settings file.
    if (pos) {
        //Debug_Info("GetFileAttributes_UAC: %s", lpFileName);
        std::wstring path = GetAppDataPath();
        if (!path.empty()) {
            path.append(L"\\");
            DWORD attributes = INVALID_FILE_ATTRIBUTES;
            size_t num_bytes = 0;
            wchar_t* wchar_buff = new wchar_t[13] {0};
            if (mbstowcs_s(&num_bytes, wchar_buff, 13, lpFileName, 13) == 0) {
                path.append(wchar_buff);
                attributes = GetFileAttributes(path.c_str());
                //Copy the "WSG_NDX.WSG" saved game names file to the UAC data folder if it does not exist.
                if (attributes == INVALID_FILE_ATTRIBUTES && wcsstr(wchar_buff, L"WSG_NDX.WSG")) {
                    if (CopyFile(wchar_buff, path.c_str(), TRUE))
                        attributes = GetFileAttributes(path.c_str());
                }
            }
            delete[] wchar_buff;
            if (attributes != INVALID_FILE_ATTRIBUTES)
                return attributes;
        }
    }
    return GetFileAttributesA(lpFileName);
}
void* p_get_file_attributes_uac = &GetFileAttributes_UAC;


//____________________________________________________________________________________________________________________________________________________________________________________________________________________________
//Create\Open an alterable file for editing, from the UAC data folder first if present or depending on the DesiredAccess.
static HANDLE __stdcall CreateFile_UAC(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {

    const char* pos = StrStrIA(lpFileName, ".WSG");//check if saved game file.
    if (!pos)
        pos = StrStrIA(lpFileName, "SFOSAVED.DAT");//check if settings file.
    if (!pos)
        pos = StrStrIA(lpFileName, "WC4.CFG");//check if default settings file.
    if (pos) {
        //Debug_Info("CreateFile_UAC: %s, acc:%X", lpFileName, dwDesiredAccess);
        std::wstring path = GetAppDataPath();
        if (!path.empty()) {
            path.append(L"\\");
            HANDLE handle = INVALID_HANDLE_VALUE;
            size_t num_bytes = 0;
            wchar_t* wchar_buff = new wchar_t[13] {0};
            if (mbstowcs_s(&num_bytes, wchar_buff, 13, lpFileName, 13) == 0) {
                path.append(wchar_buff);
                handle = CreateFile(path.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
            }
            delete[] wchar_buff;
            if (handle != INVALID_HANDLE_VALUE)
                return handle;
        }
    }
    return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}
void* p_create_file_uac = &CreateFile_UAC;


//_____________________________________________________
//This function is only called for deleting the temp file "00000102.WSG". Which only exists during missions.
static BOOL __stdcall DeleteFile_UAC(LPCSTR lpFileName) {
    const char* pos = StrStrIA(lpFileName, "00000102.wsg");
    if (pos) {
        //Debug_Info("DeleteFile_UAC: %s", lpFileName);
        std::wstring path = GetAppDataPath();
        if (!path.empty()) {
            path.append(L"\\");
            BOOL retVal = FALSE;
            size_t num_bytes = 0;
            wchar_t* wchar_buff = new wchar_t[13] {0};
            if (mbstowcs_s(&num_bytes, wchar_buff, 13, lpFileName, 13) == 0) {
                path.append(wchar_buff);
                retVal = DeleteFile(path.c_str());
            }
            delete[] wchar_buff;
            if (retVal)
                return retVal;
        }
    }
    return DeleteFileA(lpFileName);
}
void* p_delete_file_uac = &DeleteFile_UAC;


//________________________________
//Rebuilds the save game name list file if it does not exist. Adding detected saved games from UAC appdata and the Application folder.
static BOOL Build_SaveNames_File() {
#define GAME_TITLE_LENGTH   44 // WC3 was 22
#define NUM_SAVES          104 //WC3 was 101
#define FORM_SIZE           16/*save/info header*/ + ( 4/*save number*/ + 4/*text length*/ + GAME_TITLE_LENGTH) * NUM_SAVES 

    bool isUAC = false;

    std::wstring path = GetAppDataPath();
    if (!path.empty()) {
        path.append(L"\\");
        isUAC = true;
    }
    size_t path_length = path.length();

    path.append(L"WSG_NDX.WSG");
    HANDLE h_name_file = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (h_name_file == INVALID_HANDLE_VALUE)
        return FALSE;

    char game_title[GAME_TITLE_LENGTH]{ 0 };

    DWORD num_bytes_written = 0;
    DWORD dw_dat = 0x4D524F46;//FORM text
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = _byteswap_ulong(FORM_SIZE);//form size (switch endianness)
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = 0x45564153;//SAVE text
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = 0x4F464E49;//INFO text
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = _byteswap_ulong(0x04);//info size (switch endianness)
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
    dw_dat = 0x09;//info data. version? wc3 was 6.
    WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);

    //create saved game names using chosen language. english format "SAVE GAME %d."
    const char* save_game_text = "SAVE GAME %d.";
    if (*p_wc4_language_ref == 1)
        save_game_text = "SPIEL SPEICHERNE %d.";
    if (*p_wc4_language_ref == 2)
        save_game_text = "SAUVEGARDER JEUE %d.";


    wchar_t w_game_file_name[16]{ 0 };

    path.resize(path_length);
    path.append(L"00000000.WSG");

    //search for previously saved games.
    for (DWORD i = 0; i < NUM_SAVES; i++) { //valid save names go from 0 to NUM_TITLES-1. For WC4 this weirdly includes the inflight replay save which is set at 102.
        dw_dat = i;
        WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);
        dw_dat = _byteswap_ulong(sizeof(game_title));// max save game title length. (switch endianness) WC3 had an ending null char that was not included in this count.
        WriteFile(h_name_file, &dw_dat, 4, &num_bytes_written, nullptr);

        memset(game_title, '\0', sizeof(game_title));

        swprintf_s(w_game_file_name, L"%08d.WSG", i);


        path.replace(path_length, 12, w_game_file_name);

        //check the app data path for this save, and if not found and UCA enabled also check the app Application dir.
        if (GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES)
            sprintf_s(game_title, save_game_text, i);
        else if (isUAC) {
            if (GetFileAttributes(w_game_file_name) != INVALID_FILE_ATTRIBUTES)
                sprintf_s(game_title, save_game_text, i);
        }

        WriteFile(h_name_file, game_title, sizeof(game_title), &num_bytes_written, nullptr);
    }

    CloseHandle(h_name_file);
    h_name_file = INVALID_HANDLE_VALUE;
    return TRUE;
}


//_______________________________________________________
static void __declspec(naked) build_save_names_file(void) {

    __asm {

        push ebx
        push edx
        push ecx
        push esi
        push edi
        push ebp

        call Build_SaveNames_File

        pop ebp
        pop edi
        pop esi
        pop ecx
        pop edx
        pop ebx

        ret
    }
}

#ifndef VERSION_WC4_DVD
//___________________________
void Music_Class_Destructor() {
    //This seems to be missing from the wc4 win95 executable destructor function.
    if (*pp_wc4_music_thread_class) {
        //Debug_Info("Music_Class_Destructor Destructing");
        wc4_music_thread_class_destructor(*pp_wc4_music_thread_class);
        wc4_dealocate_mem01(*pp_wc4_music_thread_class);
        
    }
    *pp_wc4_music_thread_class = nullptr;
    Debug_Info("Music_Class_Destructor Done");
}
#endif


//___________________________________________________________________
static void Modify_Object_LOD_Distance(DWORD* LOD, FILE_STRUCT* file) {

    static int lod_modifier = 100;
    static bool run_once = false;
    if (!run_once) {
        lod_modifier = ConfigReadInt(L"SPACE", L"LOD_LEVEL_DISTANCE_MODIFIER", CONFIG_SPACE_LOD_LEVEL_DISTANCE_MODIFIER);
        if (lod_modifier != 0 && lod_modifier < 100)
            lod_modifier = 100;

        run_once = true;
        Debug_Info("LOD_LEVEL_DISTANCE_MODIFIER SET AT: %d%%", lod_modifier);
    }
    if (*LOD <= 7)//Ignore values 7 or less. LOD dist 0-7 used by afterburner effect animation.
        return;

    if (strstr(file->path, "ASTRD")) {
        Debug_Info("Ignoring LOD mod for Asteroid: ASTRD1 & ASTRD2");
        return;
    }

    if (lod_modifier == 0)
        *LOD = 0;
    else
        *LOD = *LOD * lod_modifier / 100;
    //Debug_Info("Modify_Object_LOD dist:%d", *LOD);
}


//________________________________________________________
static void __declspec(naked) modify_object_lod_dist(void) {

    __asm {
        pushad
#ifdef VERSION_WC4_DVD
        push ebp
#else
        push eax
#endif
        mov ecx, ebx
        add ecx, 0x30
        push ecx
        call Modify_Object_LOD_Distance
        add esp, 0x8
        popad
        //re-insert original code
#ifdef VERSION_WC4_DVD
        mov edi, dword ptr ds : [ebp + 0x90]
#else
        mov eax, dword ptr ds : [eax + 0x90]
#endif
        ret
    }
}


//__________________________________________________________
static LONG MULTI_ARG1_BY_256_DIV_ARG2(LONG arg1, LONG arg2) {
    LONGLONG val = (LONGLONG)arg1 << 8;
    return LONG(val / arg2);
}


//__________________________________________________________
static LONG MULTI_ARG1_BY_ARG2_DIV_256(LONG arg1, LONG arg2) {
    LONGLONG val = (LONGLONG)arg1 * arg2;
    return LONG(val >> 8);
}


//______________________________________________________________________
static LONG MULTI_ARG1_BY_ARG2_DIV_ARG3(LONG arg1, LONG arg2, LONG arg3) {
    return LONG((LONGLONG)arg1 * arg2 / arg3);
}


//_________________________________________________
static void Debug_Info_WC4(const char* format, ...) {
    __Debug_Info(DEBUG_INFO_ERROR, format);
}


//_______________________________________________
static BYTE* Get_Wav_Data_Chunk(BYTE* p_wav_data) {

    DWORD code = *(DWORD*)p_wav_data;
    if (code != 0x46464952)// FOURCC [RIFF]
        return nullptr;

    p_wav_data += 4;
    DWORD file_size = *(DWORD*)p_wav_data;
    LONGLONG remaining_size = (LONGLONG)file_size;

    p_wav_data += 4;
    remaining_size -= 4;

    code = *(DWORD*)p_wav_data;
    if (code != 0x45564157) {// FOURCC [WAVE]
        Debug_Info("Get_Wav_Data_Chunk: Wav RIFF has No WAVE");
        return nullptr;
    }

    p_wav_data += 4;
    remaining_size -= 4;

    DWORD sec_size = 0;

    while (remaining_size > 0) {
        code = *(DWORD*)p_wav_data;
        p_wav_data += 4;
        remaining_size -= 4;
        sec_size = *(DWORD*)p_wav_data;
        if (code == 0x61746164)// FOURCC [data]
            break;

        p_wav_data += 4;
        remaining_size -= 4;
        p_wav_data += sec_size;
        remaining_size -= sec_size;
    }
    if (code == 0x61746164) // FOURCC [data]
        return p_wav_data;

    Debug_Info("Get_Wav_Data_Chunk: data chunk NOT found");
    return nullptr;

}


//_____________________________________________________
static void __declspec(naked) get_wave_audio_data(void) {
    // Retrieve the WAV audio data buffer and size.
    // Replaces the original method of obtaining wave data and size that did not take into account the presents of other chunks.
    // And was occasionally including non audio data in the buffer, causing popping and static sounds.
    __asm {
        push ebx
        mov eax, dword ptr ds:[ebx+0x24]// wave file data pointer.
        push eax
        call Get_Wav_Data_Chunk
        add esp, 0x4
        cmp eax, 0// returned pointer to data_chunk.
        je failed

        mov ebp, dword ptr ds:[eax]// pointer to the data_chunk size.
        add eax, 0x4// add 4 to move the pointer to the data_chunk data.
        
#ifdef VERSION_WC4_DVD
        mov edx, eax
#else
        mov esi, eax
#endif
        mov eax, 0// set eax to 0, Success.
        jmp exit_func

        failed:// set eax to 1. Failed, initiate the Error Message Box.
        mov eax, 1
        
        exit_func:
        pop ebx
        ret
    }
}


//_________________________________________________________________________________________________________________________
static void Display_Debug_Info_1(DRAW_BUFFER* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {
    
    y = (*pp_wc4_db_game_main)->rc.top + 4;
    wc4_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    
    if (!*pp_wc4_music_thread_class)
        return;
    LONG* p_music_data = (LONG*)*pp_wc4_music_thread_class;

    sprintf_s(text_buff, 240, "Requested Tune: %d", p_music_data[3]);
    y += 10;
    wc4_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);

    sprintf_s(text_buff, 240, "Current Tune: %d", p_music_data[1]);
    y += 10;
    wc4_draw_text_to_buff(p_toBuff, x, y, unk1, text_buff, p_pal_offsets);
    


}


//______________________________________
static DWORD Set_VirtualAlloc_Mem_Size() {

    static bool run_once = false;
    if (!run_once) {
        DWORD vmem_size = ConfigReadInt(L"MAIN", L"VIRTUAL_MEM_SIZE", CONFIG_MAIN_VIRTUAL_MEM_SIZE);
        if (vmem_size > *p_wc4_virtual_alloc_mem_size)
            *p_wc4_virtual_alloc_mem_size = vmem_size;

        run_once = true;
        Debug_Info("Virtual Mem Allocated: %d bytes", *p_wc4_virtual_alloc_mem_size);
    }

    return *p_wc4_virtual_alloc_mem_size;
}


//____________________________________________________________
static void __declspec(naked) set_virtual_alloc_mem_size(void) {

    __asm {
        push edx
        push ebx
        push ecx
        push edi
        push esi
        push ebp

        call Set_VirtualAlloc_Mem_Size

        pop ebp
        pop esi
        pop edi
        pop ecx
        pop ebx
        pop edx

        ret
    }
}


//_________________________________________________
static LONG Num_Watchers_Overide(LONG num_watchers) {

    static int num_watchers_overide = 500;
    static bool run_once = false;
    if (!run_once) {
        num_watchers_overide = ConfigReadInt(L"MAIN", L"NUM_WATCHERS_OVERIDE", CONFIG_MAIN_NUM_WATCHERS_OVERIDE);
        if (num_watchers_overide < 500)
            num_watchers_overide = 500;

        run_once = true;
        Debug_Info("Max Number Of Watches Overide Value: %d", num_watchers_overide);
    }

    if (num_watchers < num_watchers_overide) {
        num_watchers = num_watchers_overide;
        Debug_Info("Max Number Of Watches Set At: %d", num_watchers);
    }
    return num_watchers;
}


//______________________________________________________
static void __declspec(naked) num_watchers_overide(void) {

    __asm {
        push edx
        push ebx
        push ecx
        push edi
        push esi
        push ebp

        push eax
        call Num_Watchers_Overide
        add esp, 0x4

        pop ebp
        pop esi
        pop edi
        pop ecx
        pop ebx
        pop edx

        //re-insert original code
        mov dword ptr ds : [ecx + 0x4], eax
        mov esi, ecx

        ret
    }
}


//_____________________________________________
static void Proccess_Object(DWORD** func_array) {
    static int count = 0;
    Debug_Info("Proccess_Object: %d, func:%X", count, func_array[1]);

    count++;
}


//________________________________
static void Proccess_Object_Pass() {
    static int count = 0;
    Debug_Info("Proccess_Object PASSED: %d", count);

    count++;
}


//__________________________________________________
static void __declspec(naked) processes_object(void) {

    __asm {
        mov ebx, dword ptr ds : [eax]

        pushad
        push ebx
        call Proccess_Object
        add esp, 0x4
        popad


        mov ecx, eax
        call dword ptr ds : [ebx + 0x4]

        //pushad
        //call Proccess_Object_Pass
        //popad

        ret

    }
}


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


//___________________________________________________________________________________________________________________________________________________________
static void VDU_Comms_Draw_Menu_Text(BYTE hud_type, LONG line_num, DRAW_BUFFER* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {
    
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
        mov eax, dword ptr ds:[esi+0x4]
        mov al, byte ptr ds:[eax+0x18]
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
        mov al, byte ptr ds:[eax]
        cmp al , 0x4 //4 == comms
        je exit_func

        mov vdu_comms_had_focus, FALSE
        exit_func :

        ret
    }
}


//__________________________________
//Regulate how often mouse position is sampled for adjusting ship speed.
static BOOL Is_Mouse_Throttle_Time() {

    static LARGE_INTEGER last_throttle_time = { 0 };
    static LARGE_INTEGER update_time{ 0 };
    static bool run_once = false;

    if (!run_once)
        update_time.QuadPart = p_wc4_frequency->QuadPart / 16LL;
    
    LARGE_INTEGER time = { 0 };
    LARGE_INTEGER ElapsedMicroseconds = { 0 };

    QueryPerformanceCounter(&time);

    ElapsedMicroseconds.QuadPart = time.QuadPart - last_throttle_time.QuadPart;
    if (ElapsedMicroseconds.QuadPart < 0 || ElapsedMicroseconds.QuadPart > update_time.QuadPart) {
        last_throttle_time.QuadPart = time.QuadPart;
        return TRUE;
    }
    return FALSE;
}


//_________________________________________________________
static void __declspec(naked) mouse_sub_throttle_time(void) {

    __asm {
        pushad
        call Is_Mouse_Throttle_Time
        cmp eax, 0
        popad
        je exit_func

        //divide mouse y position from centre by 32 for greater precision. this was formerly a division by 4.
#ifdef VERSION_WC4_DVD
        shr edx, 0x5
        sub dword ptr ds : [eax + 0x8] , edx
#else
        shr eax, 0x5
        sub dword ptr ds : [edx + 0x8] , eax
#endif

        exit_func :
        ret
    }
}


//_________________________________________________________
static void __declspec(naked) mouse_add_throttle_time(void) {

    __asm {
        pushad
        call Is_Mouse_Throttle_Time
        cmp eax, 0
        popad
        je exit_func

        //divide mouse y position from centre by 32 for greater precision. this was formerly a division by 4.
#ifdef VERSION_WC4_DVD
        shr ecx, 0x5
        add dword ptr ds : [eax + 0x8] , ecx
#else
        shr eax, 0x5
        add dword ptr ds : [edx + 0x8] , eax
#endif

        exit_func :
        ret
    }
}


#ifdef VERSION_WC4_DVD
//_______________________________
void Modifications_GeneralFixes() {

    //Load files in place of files located in .tre archives.
    FuncReplace32(0x4897B3, 0x2199, (DWORD)&load_data_file);

    //Fixed a code error on a call to the "VirtualProtect" function, where the "lpflOldProtect" parameter was set to NULL when it should point to a place to store the previous access protection value.
    FuncReplace32(0x4769BE, 0x0A0E, (DWORD)&virtualprotect_fix);

    //-----------------------UAC-Patch---------------------------
    //Alter the save location of files to the RoamingAppData folder. To allow the game to work without admin privileges when installed under ProgramFiles and to seperate game data between different Windows users.
    MemWrite32(0x48BE67, 0x4D4500, (DWORD)&p_get_file_attributes_uac);

    MemWrite32(0x48BF2E, 0x4D43FC, (DWORD)&p_create_file_uac);

    MemWrite32(0x4AC167, 0x4D4414, (DWORD)&p_delete_file_uac);
 
    MemWrite16(0x4529D0, 0xEC81, 0xE990);
    FuncWrite32(0x4529D2, 0x0400, (DWORD)&build_save_names_file);
    //------------------------------------------------------------


    MemWrite16(0x41F507, 0xBD8B, 0xE890);
    FuncWrite32(0x41F509, 0x90, (DWORD)&modify_object_lod_dist);


    //-----Replacement integer math function-------------------------------
    //originals causing crashes when imul/idiv were overflowing.
    MemWrite16(0x47E360, 0x8B55, 0xE990);
    FuncWrite32(0x47E362, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_256_DIV_ARG2);

    MemWrite16(0x47E373, 0x8B55, 0xE990);
    FuncWrite32(0x47E375, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_ARG2_DIV_256);

    MemWrite16(0x47E382, 0x8B55, 0xE990);
    FuncWrite32(0x47E384, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_ARG2_DIV_ARG3);
    //---------------------------------------------------------------------


    //-----Debugging---------------------------------------------
    //For adding debug info to inflight debug overlay. 
    FuncReplace32(0x4791A4, 0x018201, (DWORD)&Display_Debug_Info_1);

    //hijack WC4 Debug info
    MemWrite8(0x4A2080, 0x56, 0xE9);
    FuncWrite32(0x4A2081, 0xA0306857, (DWORD)&Debug_Info_WC4);

    //changed key combo for space debug overlay from "ALT+D" to "CTRL+D".
    MemWrite8(0x440346, 0x03, 0x0C);
    //Remove the need to need for mitchell mode to enable to display space debug overlay "CTRL+D". 
    MemWrite16(0x440353, 0x840F, 0x9090);
    MemWrite32(0x440355, 0x0225, 0x90909090);
    //Prevent the general space overlay from also being displayed when pressing "CTRL+D".
    MemWrite8(0x44036B, 0xA3, 0x90);
    MemWrite32(0x44036C, 0x4C5250, 0x90909090);
    //___________________________________________________________


    //---Fix for some static and popping sounds at the end of playback when playing some audio samples-----
    FuncWrite32(0x45E34C, 0x043FE0, (DWORD)&get_wave_audio_data);
    //Remove old method of obtaining wave data and size that did not take into account the presents of other chunks.
    MemWrite16(0x45E36B, 0x538B, 0x9090);
    MemWrite8(0x45E36D, 0x24, 0x90);
    MemWrite16(0x45E377, 0xC283, 0x9090);
    MemWrite8(0x45E379, 0x2C, 0x90);
    //-----------------------------------------------------------------------------------------------------


    //Increase the allocated general memory size.
    MemWrite8(0x49E542, 0xA1, 0xE8);
    FuncWrite32(0x49E543, 0x4B6D08, (DWORD)&set_virtual_alloc_mem_size);

    //Increase the max number of watchers at a nav point. (max number of active ships and turrets)
    MemWrite8(0x481EE5, 0x8B, 0xE8);
    FuncWrite32(0x481EE6, 0x044689F1, (DWORD)&num_watchers_overide);


    //-----scrollable comms------------------------------------------------
    MemWrite8(0x40F2E8, 0xA1, 0xE8);
    FuncWrite32(0x40F2E9, 0x4BBAE0, (DWORD)&vdu_check_if_comms_had_focus);

    MemWrite16(0x40FC6E, 0x0D8B, 0xE890);
    FuncWrite32(0x40FC70, 0x4C4188, (DWORD)&vdu_comms_check_keys);
    //0x2D == key 'C' - 1// allow for 'C' key to be used as menu selector
    MemWrite8(0x40FC79, 0x2D, 0x2C);

    FuncReplace32(0x411251, 0x080154, (DWORD)&vdu_comms_draw_menu_text);
    //---------------------------------------------------------------------


    //----better-throttle-regulation-when-using-the-mouse------------------
    MemWrite16(0x44C0D6, 0x788B, 0xE890);
    FuncWrite32(0x44C0D8, 0x02EAC108, (DWORD)&mouse_sub_throttle_time);
    MemWrite32(0x44C0DC, 0x7889FA2B, 0x90909090);
    MemWrite8(0x44C0E0, 0x08, 0x90);

    MemWrite16(0x44C0F1, 0x788B, 0xE890);
    FuncWrite32(0x44C0F3, 0x02E9C108, (DWORD)&mouse_add_throttle_time);
    MemWrite16(0x44C0F7, 0xF903, 0x9090);
    MemWrite16(0x44C0FE, 0x7889, 0x9090);
    MemWrite8(0x44C100, 0x08, 0x90);
    //---------------------------------------------------------------------
}

#else
//_______________________________
void Modifications_GeneralFixes() {

    //Load files in place of files located in .tre archives.
    FuncReplace32(0x49AB03, 0x0AA9, (DWORD)&load_data_file);
    //check if files are being closed.
    //FuncReplace32(0x49ABEA, 0x0F82, (DWORD)&close_file_handle);

    //Fixed a code error on a call to the "VirtualProtect" function, where the "lpflOldProtect" parameter was set to NULL when it should point to a place to store the previous access protection value.
    FuncReplace32(0x410B29, 0xFFFFFC93, (DWORD)&virtualprotect_fix);


    //-----------------------UAC-Patch---------------------------
    //Alter the save location of files to the RoamingAppData folder. To allow the game to work without admin privileges when installed under ProgramFiles and to seperate game data between different Windows users.
    MemWrite32(0x49BA88, 0x4DE3D0, (DWORD)&p_get_file_attributes_uac);
    
    MemWrite32(0x49BB44, 0x4DE3D8, (DWORD)&p_create_file_uac);
    
    MemWrite32(0x4B746C, 0x4DE3B4, (DWORD)&p_delete_file_uac);

    MemWrite16(0x47A780, 0xEC81, 0xE990);
    FuncWrite32(0x47A782, 0x0400, (DWORD)&build_save_names_file);
    //------------------------------------------------------------


    //Fix for dvd version of the game which does not include the original movies in their .tre archives. Jump over file checking section in "handle_movie" function.
    MemWrite8(0x475721, 0xC7, 0xE9);
    MemWrite32(0x475722, 0x017C2484, 0x03E3);
    MemWrite16(0x475726, 0x0000, 0x9090);
    MemWrite32(0x475728, 0x00000000, 0x90909090);


    MemWrite16(0x42A1F3, 0x808B, 0xE890);
    FuncWrite32(0x42A1F5, 0x90, (DWORD)&modify_object_lod_dist);


    //-----Replacement integer math function-------------------------------
    //originals causing crashes when imul/idiv were overflowing.
    MemWrite16(0x49D73C, 0x8B55, 0xE990);
    FuncWrite32(0x49D73E, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_256_DIV_ARG2);

    MemWrite16(0x49D74F, 0x8B55, 0xE990);
    FuncWrite32(0x49D751, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_ARG2_DIV_256);

    MemWrite16(0x49D75E, 0x8B55, 0xE990);
    FuncWrite32(0x49D760, 0x08458BEC, (DWORD)&MULTI_ARG1_BY_ARG2_DIV_ARG3);
    //---------------------------------------------------------------------


    //-----Debugging---------------------------------------------
    //For adding debug info to inflight debug overlay. 
    FuncReplace32(0x4129A1, 0x07A33C, (DWORD)&Display_Debug_Info_1);

    //hijack WC4 Debug info
    MemWrite8(0x4AEE47, 0x56, 0xE9);
    FuncWrite32(0x4AEE48, 0xC8586857, (DWORD)&Debug_Info_WC4);

    //changed key combo for space debug overlay from "ALT+D" to "CTRL+D".
    MemWrite8(0x40AD17, 0x03, 0x0C);
    //Remove the need to need for mitchell mode to enable to display space debug overlay "CTRL+D". 
    MemWrite16(0x40AD25, 0x840F, 0x9090);
    MemWrite32(0x40AD27, 0x0359, 0x90909090);
    //Prevent the general space overlay from also being displayed when pressing "CTRL+D".
    MemWrite8(0x40AD3B, 0xA3, 0x90);
    MemWrite32(0x40AD3C, 0x4D41B4, 0x90909090);
    //___________________________________________________________


    //---Fix for some static and popping sounds at the end of playback when playing some audio samples-----
    FuncWrite32(0x48609C, 0x028ED8, (DWORD)&get_wave_audio_data);
    //Remove old method of obtaining wave data and size that did not take into account the presents of other chunks.
    MemWrite16(0x4860C5, 0x738B, 0x9090);
    MemWrite32(0x4860C7, 0x2CC68324, 0x90909090);
    //-----------------------------------------------------------------------------------------------------


    //fix long pause occurring after auto pilot.
    //changed return from "proccess tune" function from 1 to 0.
    //this now matches code from dvd and wc3 ksaga versions.
    MemWrite32(0x487E31, 0x01, 0x00);

    //Increase the allocated general memory size.
    MemWrite8(0x4AD0B2, 0xA1, 0xE8);
    FuncWrite32(0x4AD0B3, 0x4D3018, (DWORD)&set_virtual_alloc_mem_size);

    //Increase the max number of watchers at a nav point. (max number of active ships and turrets)
    MemWrite8(0x4A14B5, 0x89, 0xE8);
    FuncWrite32(0x4A14B6, 0xF18B0441, (DWORD)&num_watchers_overide);


    //MemWrite8(0x4A1BB2, 0x8B, 0xE8);
    //FuncWrite32(0x4A1BB3, 0xFFC88B18, (DWORD)&processes_object);
    //MemWrite16(0x4A1BB7, 0x0453, 0x9090);


    //-----scrollable comms------------------------------------------------
    MemWrite8(0x427288, 0xA0, 0xE8);
    FuncWrite32(0x427289, 0x4C1244, (DWORD)&vdu_check_if_comms_had_focus);

    MemWrite16(0x427CBD, 0x0D8B, 0xE890);
    FuncWrite32(0x427CBF, 0x4C0660, (DWORD)&vdu_comms_check_keys);
    //0x2D == key 'C' - 1// allow for 'C' key to be used as menu selector
    MemWrite8(0x427CC8, 0x2D, 0x2C);//done

    FuncReplace32(0x425BA9, 0x067134, (DWORD)&vdu_comms_draw_menu_text);
    //---------------------------------------------------------------------


    //----better-throttle-regulation-when-using-the-mouse------------------
    MemWrite16(0x449260, 0xE8C1, 0xE890);
    FuncWrite32(0x449262, 0x085A8B02, (DWORD)&mouse_sub_throttle_time);
    MemWrite32(0x449266, 0x5A89D82B, 0x90909090);
    MemWrite8(0x44926A, 0x08, 0x90);

    MemWrite16(0x449285, 0xE8C1, 0xE890);
    FuncWrite32(0x449287, 0x085A8B02, (DWORD)&mouse_add_throttle_time);
    MemWrite32(0x44928B, 0x5A89D803, 0x90909090);
    MemWrite8(0x44928F, 0x08, 0x90);
    //---------------------------------------------------------------------
}
#endif
