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


//_________________________________________________________________________________________________________
static HANDLE __stdcall FindFirstFile_Saved_Games_UAC(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
    // checks if saved games are present and if so, skips into movie/mission.
    std::wstring path = GetAppDataPath();
    if (!path.empty()) {
        path.append(L"\\");
        HANDLE handle = INVALID_HANDLE_VALUE;
        size_t num_bytes = 0;
        wchar_t* wchar_buff = new wchar_t[13] {0};
        if (mbstowcs_s(&num_bytes, wchar_buff, 13, lpFileName, 13) == 0) {
            path.append(wchar_buff);
            WIN32_FIND_DATA FindFileData;
            handle = FindFirstFile(path.c_str(), &FindFileData);
        }
        delete[] wchar_buff;
        if (handle != INVALID_HANDLE_VALUE)
            return handle;
    }
    return FindFirstFileA(lpFileName, lpFindFileData);
}
void* p_find_first_file_saved_games_uac = &FindFirstFile_Saved_Games_UAC;


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


//______________________________________________________________________________________________________________________________
static void Display_Debug_Info_1(DRAW_BUFFER_MAIN* p_toBuff, DWORD x, DWORD y, DWORD unk1, char* text_buff, BYTE* p_pal_offsets) {
    
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


DWORD vmem_start = 0;
DWORD vmem_end = 0;
//_____________________________________________________________________________________________________________________________
static LPVOID __stdcall VirtualAlloc_Game_Resources(LPVOID lpAddress, SIZE_T dwSize, DWORD  flAllocationType, DWORD  flProtect) {

    LPVOID base_address = VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
    vmem_start = (DWORD)base_address;
    vmem_end = vmem_start + dwSize;

    return base_address;
}
void* p_virtual_alloc_game_resources = &VirtualAlloc_Game_Resources;


//_____________________________________________
//static void print_texture_error(DWORD mem_addr) {
//
//    Debug_Info_Error("BAD_Texture_Addr: %X", mem_addr);
//}


//______________________________________________________
static void __declspec(naked) test_texture_address(void) {

    __asm {
        add eax, edx// add tex_mem_ptr(EAX) and offset(EDX)
        cmp eax, vmem_start
        jb mem_out_of_bounds
        cmp eax, vmem_end
        jb sample_texture

        mem_out_of_bounds :
        //pushad
        //push eax
        //call print_texture_error
        //add esp, 0x4
        //popad
        mov al, 0xFF// set pixel to 255(mask colour) don't draw. 
        jmp end_func

        sample_texture :
        mov al, byte ptr ds : [eax]

        end_func :
        //original code
        add ebp, ebx
        ret
    }
}


//________________________________________________________
static void __declspec(naked) test_texture_address_2(void) {

    __asm {
        //original code
#ifdef VERSION_WC4_DVD
        adc ecx, dword ptr ds : [0x4B943C]
#else
        adc ecx, dword ptr ds : [0x4DB358]
#endif
        

        add eax, edx// add tex_mem_ptr(EAX) and offset(EDX)
        cmp eax, vmem_start
        jb mem_out_of_bounds
        cmp eax, vmem_end
        jb sample_texture

        mem_out_of_bounds :
        //pushad
        //push eax
        //call print_texture_error
        //add esp, 0x4
        //popad
        mov al, 0xFF// set pixel to 255(mask colour) don't draw. 
        ret

        sample_texture :
        mov al, byte ptr ds : [eax]
        ret
    }
}

/*
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
*/

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


    //00470794 | .FF15 B0444D00 CALL DWORD PTR DS : [<&KERNEL32.FindFirstFileA>]
    MemWrite32(0x470796, 0x4D44B0, (DWORD)&p_find_first_file_saved_games_uac);
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


    //---------------random-space-crash-fix--texture-sampler-fix------------------
 
    MemWrite32(0x49E556, 0x4D44A8, (DWORD)&p_virtual_alloc_game_resources);

    // poly draw func 01: texture highlight, large near
    // 8 or greater
    //0
    MemWrite8(0x497558, 0x8A, 0xE8);
    FuncWrite32(0x497559, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49755D, 0x4B943C, 0x90909090);
    //1
    MemWrite8(0x49758A, 0x8A, 0xE8);
    FuncWrite32(0x49758B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49758F, 0x4B943C, 0x90909090);
    //2
    MemWrite8(0x4975BD, 0x8A, 0xE8);
    FuncWrite32(0x4975BE, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4975C2, 0x4B943C, 0x90909090);
    //3
    MemWrite8(0x4975F0, 0x8A, 0xE8);
    FuncWrite32(0x4975F1, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4975F5, 0x4B943C, 0x90909090);
    //4
    MemWrite8(0x497623, 0x8A, 0xE8);
    FuncWrite32(0x497624, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x497628, 0x4B943C, 0x90909090);
    //5
    MemWrite8(0x497656, 0x8A, 0xE8);
    FuncWrite32(0x497657, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49765B, 0x4B943C, 0x90909090);
    //6
    MemWrite8(0x497689, 0x8A, 0xE8);
    FuncWrite32(0x49768A, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49768E, 0x4B943C, 0x90909090);
    //7
    MemWrite8(0x4976BC, 0x8A, 0xE8);
    FuncWrite32(0x4976BD, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4976C1, 0x4B943C, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x49770C, 0x8A, 0xE8);
    FuncWrite32(0x49770D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x497711, 0x4B943C, 0x90909090);
    //1
    MemWrite8(0x49774A, 0x8A, 0xE8);
    FuncWrite32(0x49774B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49774F, 0x4B943C, 0x90909090);
    //2
    MemWrite8(0x497789, 0x8A, 0xE8);
    FuncWrite32(0x49778A, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49778E, 0x4B943C, 0x90909090);
    //3
    MemWrite8(0x4977C8, 0x8A, 0xE8);
    FuncWrite32(0x4977C9, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4977CD, 0x4B943C, 0x90909090);
    //4
    MemWrite8(0x497807, 0x8A, 0xE8);
    FuncWrite32(0x497808, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49780C, 0x4B943C, 0x90909090);
    //5
    MemWrite8(0x497842, 0x8A, 0xE8);
    FuncWrite32(0x497843, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x497847, 0x4B943C, 0x90909090);
    //6
    MemWrite8(0x49787D, 0x8A, 0xE8);
    FuncWrite32(0x49787E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x497882, 0x4B943C, 0x90909090);

    // poly draw func 02: texture, large near
    // 8 or greater
    //0
    MemWrite8(0x497FE7, 0x03, 0xE8);
    FuncWrite32(0x497FE8, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x49800B, 0x03, 0xE8);
    FuncWrite32(0x49800C, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x498030, 0x03, 0xE8);
    FuncWrite32(0x498031, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x498055, 0x03, 0xE8);
    FuncWrite32(0x498056, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x49807A, 0x03, 0xE8);
    FuncWrite32(0x49807B, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x49809F, 0x03, 0xE8);
    FuncWrite32(0x4980A0, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x4980C4, 0x03, 0xE8);
    FuncWrite32(0x4980C5, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x4980E9, 0x03, 0xE8);
    FuncWrite32(0x4980EA, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x49812B, 0x03, 0xE8);
    FuncWrite32(0x49812C, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x49815B, 0x03, 0xE8);
    FuncWrite32(0x49815C, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x49818C, 0x03, 0xE8);
    FuncWrite32(0x49818D, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x4981BD, 0x03, 0xE8);
    FuncWrite32(0x4981BE, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x4981EA, 0x03, 0xE8);
    FuncWrite32(0x4981EB, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x498217, 0x03, 0xE8);
    FuncWrite32(0x498218, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x498244, 0x03, 0xE8);
    FuncWrite32(0x498245, 0x02048AEB, (DWORD)&test_texture_address);

    // poly draw func 03: texture highlight
    // 8 or greater
    //0
    MemWrite8(0x498BD2, 0x8A, 0xE8);
    FuncWrite32(0x498BD3, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498BD7, 0x4B943C, 0x90909090);
    //1
    MemWrite8(0x498C04, 0x8A, 0xE8);
    FuncWrite32(0x498C05, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498C09, 0x4B943C, 0x90909090);
    //2
    MemWrite8(0x498C37, 0x8A, 0xE8);
    FuncWrite32(0x498C38, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498C3C, 0x4B943C, 0x90909090);
    //3
    MemWrite8(0x498C6A, 0x8A, 0xE8);
    FuncWrite32(0x498C6B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498C6F, 0x4B943C, 0x90909090);
    //4
    MemWrite8(0x498C9D, 0x8A, 0xE8);
    FuncWrite32(0x498C9E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498CA2, 0x4B943C, 0x90909090);
    //5
    MemWrite8(0x498CD0, 0x8A, 0xE8);
    FuncWrite32(0x498CD1, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498CD5, 0x4B943C, 0x90909090);
    //6
    MemWrite8(0x498D03, 0x8A, 0xE8);
    FuncWrite32(0x498D04, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498D08, 0x4B943C, 0x90909090);
    //7
    MemWrite8(0x498D36, 0x8A, 0xE8);
    FuncWrite32(0x498D37, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498D3B, 0x4B943C, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x498D86, 0x8A, 0xE8);
    FuncWrite32(0x498D87, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498D8B, 0x4B943C, 0x90909090);
    //1
    MemWrite8(0x498DC4, 0x8A, 0xE8);
    FuncWrite32(0x498DC5, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498DC9, 0x4B943C, 0x90909090);
    //2
    MemWrite8(0x498E03, 0x8A, 0xE8);
    FuncWrite32(0x498E04, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498E08, 0x4B943C, 0x90909090);
    //3
    MemWrite8(0x498E42, 0x8A, 0xE8);
    FuncWrite32(0x498E43, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498E47, 0x4B943C, 0x90909090);
    //4
    MemWrite8(0x498E81, 0x8A, 0xE8);
    FuncWrite32(0x498E82, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498E86, 0x4B943C, 0x90909090);
    //5
    MemWrite8(0x498EBC, 0x8A, 0xE8);
    FuncWrite32(0x498EBD, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498EC1, 0x4B943C, 0x90909090);
    //6
    MemWrite8(0x498EF7, 0x8A, 0xE8);
    FuncWrite32(0x498EF8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x498EFC, 0x4B943C, 0x90909090);

    // texture, large near
    // 8 or greater
    //0
    MemWrite8(0x49988B, 0x03, 0xE8);
    FuncWrite32(0x49988C, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x4998AF, 0x03, 0xE8);
    FuncWrite32(0x4998B0, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x4998D4, 0x03, 0xE8);
    FuncWrite32(0x4998D5, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x4998F9, 0x03, 0xE8);
    FuncWrite32(0x4998FA, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x49991E, 0x03, 0xE8);
    FuncWrite32(0x49991F, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x499943, 0x03, 0xE8);
    FuncWrite32(0x499944, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x499968, 0x03, 0xE8);
    FuncWrite32(0x499969, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x49998D, 0x03, 0xE8);
    FuncWrite32(0x49998E, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x4999CF, 0x03, 0xE8);
    FuncWrite32(0x4999D0, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x4999FF, 0x03, 0xE8);
    FuncWrite32(0x499A00, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x499A30, 0x03, 0xE8);
    FuncWrite32(0x499A31, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x499A61, 0x03, 0xE8);
    FuncWrite32(0x499A62, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x499A8E, 0x03, 0xE8);
    FuncWrite32(0x499A8F, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x499ABB, 0x03, 0xE8);
    FuncWrite32(0x499ABC, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x499AE8, 0x03, 0xE8);
    FuncWrite32(0x499AE9, 0x02048AEB, (DWORD)&test_texture_address);


    // poly draw func 04: texture
    // 8 or greater
    //0
    MemWrite8(0x49A129, 0x03, 0xE8);
    FuncWrite32(0x49A12A, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x49A149, 0x03, 0xE8);
    FuncWrite32(0x49A14A, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x49A16A, 0x03, 0xE8);
    FuncWrite32(0x49A16B, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x49A18B, 0x03, 0xE8);
    FuncWrite32(0x49A18C, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x49A1AC, 0x03, 0xE8);
    FuncWrite32(0x49A1AD, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x49A1CD, 0x03, 0xE8);
    FuncWrite32(0x49A1CE, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x49A1EE, 0x03, 0xE8);
    FuncWrite32(0x49A1EF, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x49A20F, 0x03, 0xE8);
    FuncWrite32(0x49A210, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x49A24D, 0x03, 0xE8);
    FuncWrite32(0x49A24E, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x49A279, 0x03, 0xE8);
    FuncWrite32(0x49A27A, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x49A2A6, 0x03, 0xE8);
    FuncWrite32(0x49A2A7, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x49A2D3, 0x03, 0xE8);
    FuncWrite32(0x49A2D4, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x49A2FC, 0x03, 0xE8);
    FuncWrite32(0x49A2FD, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x49A325, 0x03, 0xE8);
    FuncWrite32(0x49A326, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x49A34E, 0x03, 0xE8);
    FuncWrite32(0x49A34F, 0x02048AEB, (DWORD)&test_texture_address);


    // poly draw func 05: texture highlight
    // 8 or greater
    //0
    MemWrite8(0x49A88F, 0x8A, 0xE8);
    FuncWrite32(0x49A890, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49A894, 0x4B943C, 0x90909090);
    //1
    MemWrite8(0x49A8BD, 0x8A, 0xE8);
    FuncWrite32(0x49A8BE, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49A8C2, 0x4B943C, 0x90909090);
    //2
    MemWrite8(0x49A8EC, 0x8A, 0xE8);
    FuncWrite32(0x49A8ED, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49A8F1, 0x4B943C, 0x90909090);
    //3
    MemWrite8(0x49A91B, 0x8A, 0xE8);
    FuncWrite32(0x49A91C, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49A920, 0x4B943C, 0x90909090);
    //4
    MemWrite8(0x49A94A, 0x8A, 0xE8);
    FuncWrite32(0x49A94B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49A94F, 0x4B943C, 0x90909090);
    //5
    MemWrite8(0x49A979, 0x8A, 0xE8);
    FuncWrite32(0x49A97A, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49A97E, 0x4B943C, 0x90909090);
    //6
    MemWrite8(0x49A9A8, 0x8A, 0xE8);
    FuncWrite32(0x49A9A9, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49A9AD, 0x4B943C, 0x90909090);
    //7
    MemWrite8(0x49A9D7, 0x8A, 0xE8);
    FuncWrite32(0x49A9D8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49A9DC, 0x4B943C, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x49AA23, 0x8A, 0xE8);
    FuncWrite32(0x49AA24, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49AA28, 0x4B943C, 0x90909090);
    //1
    MemWrite8(0x49AA5D, 0x8A, 0xE8);
    FuncWrite32(0x49AA5E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49AA62, 0x4B943C, 0x90909090);
    //2
    MemWrite8(0x49AA98, 0x8A, 0xE8);
    FuncWrite32(0x49AA99, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49AA9D, 0x4B943C, 0x90909090);
    //3
    MemWrite8(0x49AAD3, 0x8A, 0xE8);
    FuncWrite32(0x49AAD4, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49AAD8, 0x4B943C, 0x90909090);
    //4
    MemWrite8(0x49AB0E, 0x8A, 0xE8);
    FuncWrite32(0x49AB0F, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49AB13, 0x4B943C, 0x90909090);
    //5
    MemWrite8(0x49AB45, 0x8A, 0xE8);
    FuncWrite32(0x49AB46, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49AB4A, 0x4B943C, 0x90909090);
    //6
    MemWrite8(0x49AB7C, 0x8A, 0xE8);
    FuncWrite32(0x49AB7D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49AB81, 0x4B943C, 0x90909090);
    //----------------------------------------------------------------------------
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


    //00401EE9 | .FF15 88E34D00 CALL DWORD PTR DS : [<&KERNEL32.FindFirstFileA>]
    MemWrite32(0x401EEB, 0x4DE388, (DWORD)&p_find_first_file_saved_games_uac);
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


    //---------------random-space-crash-fix--texture-sampler-fix------------------

    MemWrite32(0x4AD0C6, 0x4DE368, (DWORD)&p_virtual_alloc_game_resources);

    // poly draw func 01: texture highlight, large near
    // 8 or greater
    //0
    MemWrite8(0x48FD0C, 0x8A, 0xE8);
    FuncWrite32(0x48FD0D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FD11, 0x4DB358, 0x90909090);
    //1
    MemWrite8(0x48FD3E, 0x8A, 0xE8);
    FuncWrite32(0x48FD3F, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FD43, 0x4DB358, 0x90909090);
    //2
    MemWrite8(0x48FD71, 0x8A, 0xE8);
    FuncWrite32(0x48FD72, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FD76, 0x4DB358, 0x90909090);
    //3
    MemWrite8(0x48FDA4, 0x8A, 0xE8);
    FuncWrite32(0x48FDA5, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FDA9, 0x4DB358, 0x90909090);
    //4
    MemWrite8(0x48FDD7, 0x8A, 0xE8);
    FuncWrite32(0x48FDD8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FDDC, 0x4DB358, 0x90909090);
    //5
    MemWrite8(0x48FE0A, 0x8A, 0xE8);
    FuncWrite32(0x48FE0B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FE0F, 0x4DB358, 0x90909090);
    //6
    MemWrite8(0x48FE3D, 0x8A, 0xE8);
    FuncWrite32(0x48FE3E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FE42, 0x4DB358, 0x90909090);
    //7
    MemWrite8(0x48FE70, 0x8A, 0xE8);
    FuncWrite32(0x48FE71, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FE75, 0x4DB358, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x48FEC0, 0x8A, 0xE8);
    FuncWrite32(0x48FEC1, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FEC5, 0x4DB358, 0x90909090);
    //1
    MemWrite8(0x48FEFE, 0x8A, 0xE8);
    FuncWrite32(0x48FEFF, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FF03, 0x4DB358, 0x90909090);
    //2
    MemWrite8(0x48FF3D, 0x8A, 0xE8);
    FuncWrite32(0x48FF3E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FF42, 0x4DB358, 0x90909090);
    //3
    MemWrite8(0x48FF7C, 0x8A, 0xE8);
    FuncWrite32(0x48FF7D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FF81, 0x4DB358, 0x90909090);
    //4
    MemWrite8(0x48FFBB, 0x8A, 0xE8);
    FuncWrite32(0x48FFBC, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FFC0, 0x4DB358, 0x90909090);
    //5
    MemWrite8(0x48FFF6, 0x8A, 0xE8);
    FuncWrite32(0x48FFF7, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x48FFFB, 0x4DB358, 0x90909090);
    //6
    MemWrite8(0x490031, 0x8A, 0xE8);
    FuncWrite32(0x490032, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x490036, 0x4DB358, 0x90909090);

    // poly draw func 02: texture, large near
    // 8 or greater
    //0
    MemWrite8(0x49079B, 0x03, 0xE8);
    FuncWrite32(0x49079C, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x4907BF, 0x03, 0xE8);
    FuncWrite32(0x4907C0, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x4907E4, 0x03, 0xE8);
    FuncWrite32(0x4907E5, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x490809, 0x03, 0xE8);
    FuncWrite32(0x49080A, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x49082E, 0x03, 0xE8);
    FuncWrite32(0x49082F, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x490853, 0x03, 0xE8);
    FuncWrite32(0x490854, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x490878, 0x03, 0xE8);
    FuncWrite32(0x490879, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x49089D, 0x03, 0xE8);
    FuncWrite32(0x49089E, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x4908DF, 0x03, 0xE8);
    FuncWrite32(0x4908E0, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x49090F, 0x03, 0xE8);
    FuncWrite32(0x490910, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x490940, 0x03, 0xE8);
    FuncWrite32(0x490941, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x490971, 0x03, 0xE8);
    FuncWrite32(0x490972, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x49099E, 0x03, 0xE8);
    FuncWrite32(0x49099F, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x4909CB, 0x03, 0xE8);
    FuncWrite32(0x4909CC, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x4909F8, 0x03, 0xE8);
    FuncWrite32(0x4909F9, 0x02048AEB, (DWORD)&test_texture_address);

    // poly draw func 03: texture highlight
    // 8 or greater
    //0
    MemWrite8(0x491386, 0x8A, 0xE8);
    FuncWrite32(0x491387, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49138B, 0x4DB358, 0x90909090);
    //1
    MemWrite8(0x4913B8, 0x8A, 0xE8);
    FuncWrite32(0x4913B9, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4913BD, 0x4DB358, 0x90909090);
    //2
    MemWrite8(0x4913EB, 0x8A, 0xE8);
    FuncWrite32(0x4913EC, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4913F0, 0x4DB358, 0x90909090);
    //3
    MemWrite8(0x49141E, 0x8A, 0xE8);
    FuncWrite32(0x49141F, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x491423, 0x4DB358, 0x90909090);
    //4
    MemWrite8(0x491451, 0x8A, 0xE8);
    FuncWrite32(0x491452, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x491456, 0x4DB358, 0x90909090);
    //5
    MemWrite8(0x491484, 0x8A, 0xE8);
    FuncWrite32(0x491485, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x491489, 0x4DB358, 0x90909090);
    //6
    MemWrite8(0x4914B7, 0x8A, 0xE8);
    FuncWrite32(0x4914B8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4914BC, 0x4DB358, 0x90909090);
    //7
    MemWrite8(0x4914EA, 0x8A, 0xE8);
    FuncWrite32(0x4914EB, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4914EF, 0x4DB358, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x49153A, 0x8A, 0xE8);
    FuncWrite32(0x49153B, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49153F, 0x4DB358, 0x90909090);
    //1
    MemWrite8(0x491578, 0x8A, 0xE8);
    FuncWrite32(0x491579, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49157D, 0x4DB358, 0x90909090);
    //2
    MemWrite8(0x4915B7, 0x8A, 0xE8);
    FuncWrite32(0x4915B8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4915BC, 0x4DB358, 0x90909090);
    //3
    MemWrite8(0x4915F6, 0x8A, 0xE8);
    FuncWrite32(0x4915F7, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4915FB, 0x4DB358, 0x90909090);
    //4
    MemWrite8(0x491635, 0x8A, 0xE8);
    FuncWrite32(0x491636, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49163A, 0x4DB358, 0x90909090);
    //5
    MemWrite8(0x491670, 0x8A, 0xE8);
    FuncWrite32(0x491671, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x491675, 0x4DB358, 0x90909090);
    //6
    MemWrite8(0x4916AB, 0x8A, 0xE8);
    FuncWrite32(0x4916AC, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4916B0, 0x4DB358, 0x90909090);

    // texture, large near
    // 8 or greater
    //0
    MemWrite8(0x49203F, 0x03, 0xE8);
    FuncWrite32(0x492040, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x492063, 0x03, 0xE8);
    FuncWrite32(0x492064, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x492088, 0x03, 0xE8);
    FuncWrite32(0x492089, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x4920AD, 0x03, 0xE8);
    FuncWrite32(0x4920AE, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x4920D2, 0x03, 0xE8);
    FuncWrite32(0x4920D3, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x4920F7, 0x03, 0xE8);
    FuncWrite32(0x4920F8, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x49211C, 0x03, 0xE8);
    FuncWrite32(0x49211D, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x492141, 0x03, 0xE8);
    FuncWrite32(0x492142, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x492183, 0x03, 0xE8);
    FuncWrite32(0x492184, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x4921B3, 0x03, 0xE8);
    FuncWrite32(0x4921B4, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x4921E4, 0x03, 0xE8);
    FuncWrite32(0x4921E5, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x492215, 0x03, 0xE8);
    FuncWrite32(0x492216, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x492242, 0x03, 0xE8);
    FuncWrite32(0x492243, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x49226F, 0x03, 0xE8);
    FuncWrite32(0x492270, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x49229C, 0x03, 0xE8);
    FuncWrite32(0x49229D, 0x02048AEB, (DWORD)&test_texture_address);


    // poly draw func 04: texture
    // 8 or greater
    //0
    MemWrite8(0x4928DD, 0x03, 0xE8);
    FuncWrite32(0x4928DE, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x4928FD, 0x03, 0xE8);
    FuncWrite32(0x4928FE, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x49291E, 0x03, 0xE8);
    FuncWrite32(0x49291F, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x49293F, 0x03, 0xE8);
    FuncWrite32(0x492940, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x492960, 0x03, 0xE8);
    FuncWrite32(0x492961, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x492981, 0x03, 0xE8);
    FuncWrite32(0x492982, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x4929A2, 0x03, 0xE8);
    FuncWrite32(0x4929A3, 0x02048AEB, (DWORD)&test_texture_address);
    //7
    MemWrite8(0x4929C3, 0x03, 0xE8);
    FuncWrite32(0x4929C4, 0x02048AEB, (DWORD)&test_texture_address);
    // less than 8
    //0
    MemWrite8(0x492A01, 0x03, 0xE8);
    FuncWrite32(0x492A02, 0x02048AEB, (DWORD)&test_texture_address);
    //1
    MemWrite8(0x492A2D, 0x03, 0xE8);
    FuncWrite32(0x492A2E, 0x02048AEB, (DWORD)&test_texture_address);
    //2
    MemWrite8(0x492A5A, 0x03, 0xE8);
    FuncWrite32(0x492A5B, 0x02048AEB, (DWORD)&test_texture_address);
    //3
    MemWrite8(0x492A87, 0x03, 0xE8);
    FuncWrite32(0x492A88, 0x02048AEB, (DWORD)&test_texture_address);
    //4
    MemWrite8(0x492AB0, 0x03, 0xE8);
    FuncWrite32(0x492AB1, 0x02048AEB, (DWORD)&test_texture_address);
    //5
    MemWrite8(0x492AD9, 0x03, 0xE8);
    FuncWrite32(0x492ADA, 0x02048AEB, (DWORD)&test_texture_address);
    //6
    MemWrite8(0x492B02, 0x03, 0xE8);
    FuncWrite32(0x492B03, 0x02048AEB, (DWORD)&test_texture_address);


    // poly draw func 05: texture highlight
    // 8 or greater
    //0
    MemWrite8(0x493043, 0x8A, 0xE8);
    FuncWrite32(0x493044, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493048, 0x4DB358, 0x90909090);
    //1
    MemWrite8(0x493071, 0x8A, 0xE8);
    FuncWrite32(0x493072, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493076, 0x4DB358, 0x90909090);
    //2
    MemWrite8(0x4930A0, 0x8A, 0xE8);
    FuncWrite32(0x4930A1, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4930A5, 0x4DB358, 0x90909090);
    //3
    MemWrite8(0x4930CF, 0x8A, 0xE8);
    FuncWrite32(0x4930D0, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4930D4, 0x4DB358, 0x90909090);
    //4
    MemWrite8(0x4930FE, 0x8A, 0xE8);
    FuncWrite32(0x4930FF, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493103, 0x4DB358, 0x90909090);
    //5
    MemWrite8(0x49312D, 0x8A, 0xE8);
    FuncWrite32(0x49312E, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493132, 0x4DB358, 0x90909090);
    //6
    MemWrite8(0x49315C, 0x8A, 0xE8);
    FuncWrite32(0x49315D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493161, 0x4DB358, 0x90909090);
    //7
    MemWrite8(0x49318B, 0x8A, 0xE8);
    FuncWrite32(0x49318C, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493190, 0x4DB358, 0x90909090);
    // less than 8
    //0
    MemWrite8(0x4931D7, 0x8A, 0xE8);
    FuncWrite32(0x4931D8, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4931DC, 0x4DB358, 0x90909090);
    //1
    MemWrite8(0x493211, 0x8A, 0xE8);
    FuncWrite32(0x493212, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493216, 0x4DB358, 0x90909090);
    //2
    MemWrite8(0x49324C, 0x8A, 0xE8);
    FuncWrite32(0x49324D, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493251, 0x4DB358, 0x90909090);
    //3
    MemWrite8(0x493287, 0x8A, 0xE8);
    FuncWrite32(0x493288, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x49328C, 0x4DB358, 0x90909090);
    //4
    MemWrite8(0x4932C2, 0x8A, 0xE8);
    FuncWrite32(0x4932C3, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4932C7, 0x4DB358, 0x90909090);
    //5
    MemWrite8(0x4932F9, 0x8A, 0xE8);
    FuncWrite32(0x4932FA, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x4932FE, 0x4DB358, 0x90909090);
    //6
    MemWrite8(0x493330, 0x8A, 0xE8);
    FuncWrite32(0x493331, 0x0D130204, (DWORD)&test_texture_address_2);
    MemWrite32(0x493335, 0x4DB358, 0x90909090);
    //----------------------------------------------------------------------------


    //MemWrite8(0x4A1BB2, 0x8B, 0xE8);
    //FuncWrite32(0x4A1BB3, 0xFFC88B18, (DWORD)&processes_object);
    //MemWrite16(0x4A1BB7, 0x0453, 0x9090);

}
#endif
