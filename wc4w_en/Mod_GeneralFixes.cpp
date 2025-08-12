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


//________________________________________________
static void Modify_Object_LOD_Distance(DWORD* LOD) {

    static int lod_modifier = 100;
    static bool run_once = false;
    if (!run_once) {
        lod_modifier = ConfigReadInt(L"SPACE", L"LOD_LEVEL_DISTANCE_MODIFIER", CONFIG_SPACE_LOD_LEVEL_DISTANCE_MODIFIER);
        if (lod_modifier < 100)
            lod_modifier = 100;

        run_once = true;
        Debug_Info("LOD_LEVEL_DISTANCE_MODIFIER SET AT: %d%%", lod_modifier);
    }
    if (*LOD > 7)//Ignore values 7 or less. LOD dist 0-7 used by afterburner effect animation.
        *LOD = *LOD * lod_modifier / 100;
    //Debug_Info("Modify_Object_LOD dist:%d", *LOD);
}


//________________________________________________________
static void __declspec(naked) modify_object_lod_dist(void) {

    __asm {
        pushad
        mov ecx, ebx
        add ecx, 0x30
        push ecx
        call Modify_Object_LOD_Distance
        add esp, 0x4
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
}

#else
//_______________________________
void Modifications_GeneralFixes() {

    //Load files in place of files located in .tre archives.
    FuncReplace32(0x49AB03, 0x0AA9, (DWORD)&load_data_file);
    //check if files are being closed.
    //FuncReplace32(0x49ABE9, 0x0F82, (DWORD)&close_file_handle);

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
}
#endif
