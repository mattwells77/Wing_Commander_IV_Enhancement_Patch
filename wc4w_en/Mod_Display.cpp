/*
The MIT License (MIT)
Copyright © 2025-2026 Matt Wells

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

#include "Display_DX11.h"
#include "modifications.h"
#include "memwrite.h"
#include "configTools.h"
#include "libvlc_Movies.h"
#include "libvlc_Music.h"
#include "wc4w.h"
#include "input_config.h"

#define WIN_MODE_STYLE  WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX


BOOL is_nav_view = FALSE;

BOOL clip_cursor = FALSE;
static bool is_cursor_clipped = false;


UINT clientWidth = 0;
UINT clientHeight = 0;

UINT spaceWidth = 0;
UINT spaceHeight = 0;

BOOL space_use_original_aspect = FALSE;

BOOL is_space_scaled = FALSE;
UINT space_scaled_width = 640;
UINT space_scaled_height = 480;

LARGE_INTEGER nav_update_time{ 0 };


//_______________________________________________________
static void Change_Profile_Type(PROFILE_TYPE new_profile) {

    static PROFILE_TYPE last_profile_type = current_pro_type;

    current_pro_type = new_profile;

    //clear keyboard on profile change incase button is down during transition.
    if (last_profile_type != current_pro_type) {
        Clear_Key_States();
        last_profile_type = current_pro_type;
        if (last_profile_type == PROFILE_TYPE::Space)
            Debug_Info("Change_Profile_Type SPACE");
        else if (last_profile_type == PROFILE_TYPE::GUI)
            Debug_Info("Change_Profile_Type GUI");
        else if (last_profile_type == PROFILE_TYPE::NAV)
            Debug_Info("Change_Profile_Type NAV");
    }
}


//___________________________
static BOOL IsMouseInClient() {
    //check if mouse within client rect.
    RECT rcClient;
    POINT p{ 0,0 }, m{ 0,0 };

    GetCursorPos(&m);

    ClientToScreen(*p_wc4_hWinMain, &p);
    GetClientRect(*p_wc4_hWinMain, &rcClient);

    rcClient.left += p.x;
    rcClient.top += p.y;
    rcClient.right += p.x;
    rcClient.bottom += p.y;


    if (m.x < rcClient.left || m.x > rcClient.right)
        return FALSE;
    if (m.y < rcClient.top || m.y > rcClient.bottom)
        return FALSE;
    return TRUE;
}


//___________________________
static BOOL ClipMouseCursor() {

    POINT p{ 0,0 };
    if (!ClientToScreen(*p_wc4_hWinMain, &p))
        return FALSE;
    RECT rcClient;
    if (!GetClientRect(*p_wc4_hWinMain, &rcClient))
        return FALSE;
    rcClient.left += p.x;
    rcClient.top += p.y;
    rcClient.right += p.x;
    rcClient.bottom += p.y;

    return ClipCursor(&rcClient);
}


//________________________________________________________________________________
static void SetWindowTitle(HWND hwnd, const wchar_t* msg, UINT width, UINT height) {
    
    wchar_t winText[64];
    swprintf_s(winText, 64, L"%S  @%ix%i   %s", p_wc4_szAppName, width, height, msg);
    SendMessage(hwnd, WM_SETTEXT, (WPARAM)0, (LPARAM)winText);
}


//_______________________________________________________
static void SetWindowTitle(HWND hwnd, const wchar_t* msg) {

    SetWindowTitle(hwnd, msg, clientWidth, clientHeight);
}


//___________________________________________________________________________________________
static bool Check_Window_GUI_Scaling_Limits(HWND hwnd, RECT* p_rc_win, bool set_window_title) {
    if (!p_rc_win)
        return false;
    bool resized = false;
    DWORD dwStyle = 0;
    DWORD dwExStyle = 0;
    dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    //get the dimensions of the window frame style.
    RECT rc_style{ 0,0,0,0 };
    AdjustWindowRectEx(&rc_style, dwStyle, false, dwExStyle);
    RECT rc_client;
    CopyRect(&rc_client, p_rc_win);
    //subtract the window style rectangle leaving the client rectangle.
    rc_client.left -= rc_style.left;
    rc_client.top -= rc_style.top;
    rc_client.right -= rc_style.right;
    rc_client.bottom -= rc_style.bottom;


    LONG client_width = rc_client.right - rc_client.left;
    LONG client_height = rc_client.bottom - rc_client.top;

    //prevent window dimensions going beyond what is supported by your graphics card.
    if (client_width > (LONG)max_texDim || client_height > (LONG)max_texDim) {
        if (client_width > (LONG)max_texDim)
            client_width = (LONG)max_texDim;
        if (client_height > (LONG)max_texDim)
            client_height = (LONG)max_texDim;
        rc_client.right = rc_client.left + client_width;
        rc_client.bottom = rc_client.top + client_height;
        //add the client and style rects to get the window rect.
        p_rc_win->left = rc_client.left + rc_style.left;
        p_rc_win->top = rc_client.top + rc_style.top;
        p_rc_win->right = rc_client.right + rc_style.right;
        p_rc_win->bottom = rc_client.bottom + rc_style.bottom;
        resized = true;
    }



    //prevent window dimensions going under the minumum values of 640x480.
    if (client_width < GUI_WIDTH || client_height < GUI_HEIGHT) {
        if (client_width < GUI_WIDTH)
            client_width = GUI_WIDTH;
        if (client_height < GUI_HEIGHT)
            client_height = GUI_HEIGHT;

        rc_client.right = rc_client.left + client_width;
        rc_client.bottom = rc_client.top + client_height;
        //add the client and style rects to get the window rect.
        p_rc_win->left = rc_client.left + rc_style.left;
        p_rc_win->top = rc_client.top + rc_style.top;
        p_rc_win->right = rc_client.right + rc_style.right;
        p_rc_win->bottom = rc_client.bottom + rc_style.bottom;
        resized = true;

    }
    if (set_window_title)
        SetWindowTitle(hwnd, L"", client_width, client_height);
    //Debug_Info("Check_Window_GUI_Scaling_Limits w:%d, h:%d", client_width, client_height);
    return resized;
}


//________________________
static bool Display_Exit() {
    if (pMovie_vlc)
        delete pMovie_vlc;
    pMovie_vlc = nullptr;

    if (pMovie_vlc_Inflight)
        delete pMovie_vlc_Inflight;
    pMovie_vlc_Inflight = nullptr;

#ifndef VERSION_WC4_DVD
    Music_Class_Destructor();
#endif
    Display_Dx_Destroy();
    return 0;
}


//_________________________________
static BOOL Window_Setup(HWND hwnd) {
    
    if (!*p_wc4_is_windowed) {
        if (ConfigReadInt_InGame(L"MAIN", L"WINDOWED", CONFIG_MAIN_WINDOWED))
            *p_wc4_is_windowed = true;
    }

    if (!*p_wc4_movie_no_interlace) {
        if (!ConfigReadInt(L"MOVIES", L"SHOW_ORIGINAL_MOVIES_INTERLACED", CONFIG_MOVIES_SHOW_ORIGINAL_INTERLACED))
            *p_wc4_movie_no_interlace = true;
    }


    if (ConfigReadInt(L"SPACE", L"USE_ORIGINAL_ASPECT_RATIO", CONFIG_SPACE_USE_ORIGINAL_ASPECT_RATIO))
        space_use_original_aspect = TRUE;
    
    if (ConfigReadInt(L"SPACE", L"IS_SPACE_SCALED", CONFIG_SPACE_IS_SPACE_SCALED)) {
        is_space_scaled = TRUE;
        space_scaled_height = ConfigReadInt(L"SPACE", L"SCALED_SPACE_HEIGHT", CONFIG_SPACE_SCALED_SPACE_HEIGHT);
        if (space_scaled_height < 3)
            space_scaled_height = 3;
        space_scaled_width = (UINT)(space_scaled_height * (4.0f / 3.0f));
    }

    if (*p_wc4_is_windowed) {
        Debug_Info("Window Setup: Windowed");
        WINDOWPLACEMENT winPlace{ 0 };
        winPlace.length = sizeof(WINDOWPLACEMENT);
        
        SetWindowLongPtr(hwnd, GWL_STYLE, WIN_MODE_STYLE);
        //Debug_Info("is_windowed set style");

        if (ConfigReadWinData(L"MAIN", L"WIN_DATA", &winPlace)) {
            if (winPlace.showCmd != SW_MAXIMIZE)
                winPlace.showCmd = SW_SHOWNORMAL;
        }
        else {
            GetWindowPlacement(hwnd, &winPlace);
            winPlace.showCmd = SW_SHOWNORMAL;
            Debug_Info("is_windowed GetWindowPlacement");
        }
        if (winPlace.showCmd == SW_SHOWNORMAL) //if the window isn't maximized
            Check_Window_GUI_Scaling_Limits(hwnd, &winPlace.rcNormalPosition, false);
        
        SetWindowPlacement(hwnd, &winPlace);
    }
    else {
        Debug_Info("Window Setup: Fullscreen");
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(hwnd, 0, 0, 0, 0, 0, 0);
        ShowWindow(hwnd, SW_MAXIMIZE);
    }


    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    //Get the window client width and height.
    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;

    Display_Dx_Setup(hwnd, clientWidth, clientHeight);

    *p_wc4_mouse_centre_x = (LONG)spaceWidth / 2;
    *p_wc4_mouse_centre_y = (LONG)spaceHeight / 2;

    //QueryPerformanceFrequency(&Frequency);

    //Set the movement update time for Navigation screen, which was unregulated and way to fast on modern computers.
    DXGI_RATIONAL refreshRate{};
    refreshRate.Denominator = 1;
    refreshRate.Numerator = ConfigReadInt(L"SPACE", L"NAV_SCREEN_KEY_RESPONCE_HZ", CONFIG_SPACE_NAV_SCREEN_KEY_RESPONSE_HZ);
    nav_update_time.QuadPart = p_wc4_frequency->QuadPart / refreshRate.Numerator;

    DXGI_RATIONAL refreshRate_Mon{};
    Get_Monitor_Refresh_Rate(hwnd, &refreshRate_Mon);
    //Set the max refresh rate in space, original 24 FPS. Set to zero to use screen refresh rate, a negative value will use the original value.  
    refreshRate.Numerator = ConfigReadInt(L"SPACE", L"SPACE_REFRESH_RATE_HZ", CONFIG_SPACE_SPACE_REFRESH_RATE_HZ);
    if ((int)refreshRate.Numerator < 0)
        refreshRate.Numerator = 24;
    else if (refreshRate.Numerator == 0)
        refreshRate.Numerator = refreshRate_Mon.Numerator, refreshRate.Denominator = refreshRate_Mon.Denominator;

    //ensure that the max refresh rate is not greater than the monitors refresh rate.
    if ((float)refreshRate.Numerator / refreshRate.Denominator > (float)refreshRate_Mon.Numerator / refreshRate_Mon.Denominator)
        refreshRate.Numerator = refreshRate_Mon.Numerator, refreshRate.Denominator = refreshRate_Mon.Denominator;

    Debug_Info("space refresh rate max: n:%d / d:%d, HZ:%f", refreshRate.Numerator, refreshRate.Denominator, (float)refreshRate.Numerator / refreshRate.Denominator);
    p_wc4_space_time_max->QuadPart = p_wc4_frequency->QuadPart * refreshRate.Denominator / refreshRate.Numerator;

    //Debug_Info("frequency: %d, %d, space time max: %d, %d, space time min: %d, %d", p_wc4_frequency->LowPart, p_wc4_frequency->HighPart, p_wc4_space_time_max->LowPart, p_wc4_space_time_max->HighPart, p_wc4_space_time_min->LowPart, p_wc4_space_time_min->HighPart);
    Debug_Info("Window Setup: Done");
    return 1;
}


//__________________________
static void Window_Resized() {

    RECT clientRect;
    GetClientRect(*p_wc4_hWinMain, &clientRect);

    //Get the window client width and height.
    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;

    Display_Dx_Resize(clientWidth, clientHeight);

    *p_wc4_mouse_centre_x = (LONG)spaceWidth / 2;
    *p_wc4_mouse_centre_y = (LONG)spaceHeight / 2;

    if (is_cursor_clipped) {
        //Debug_Info("Window_Resized - is_cursor_clipped");
        if (ClipMouseCursor()) {
            //Debug_Info("Window_Resized - Mouse Cursor Clipped");
        }
    }
    if (*p_wc4_is_windowed) {
        WINDOWPLACEMENT winPlace{ 0 };
        winPlace.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(*p_wc4_hWinMain, &winPlace);
        ConfigWriteWinData(L"MAIN", L"WIN_DATA", &winPlace);
    }
}


//________________________
static BYTE* LockSurface() {

    if (surface_gui == nullptr)
        return nullptr;

    VOID* pSurface = nullptr;

    if (surface_gui->Lock(&pSurface, p_wc4_main_surface_pitch) != S_OK)
        return nullptr;

    return (BYTE*)pSurface;
}


//_____________________________
static void UnlockShowSurface() {

    if (surface_gui == nullptr)
        return;
    surface_gui->Unlock();
    Display_Dx_Present(PRESENT_TYPE::gui);
}


//_____________________________
static void Clear_GUI_Surface() {
    if (surface_gui)
        surface_gui->Clear_Texture(0);
}


//_________________________________________________________
static void DXBlt(BYTE* fBuff, DWORD subY, DWORD subHeight) {

    if (surface_gui == nullptr)
        return;

    LONG fWidth = GUI_WIDTH;

    if (fBuff == NULL || fBuff == (BYTE*)*pp_wc4_DIB_vBits) {
        fBuff = (BYTE*)*pp_wc4_DIB_vBits;
        fWidth = (*pp_wc4_DIB_Bitmapinfo)->bmiHeader.biWidth;
        //Debug_Info("DXBlt - db w=%d, h =%d", fWidth, -(*pp_wc4_DIB_Bitmapinfo)->bmiHeader.biHeight);
    }
    //else
        //Debug_Info("DXBlt - buffer provided");

    BYTE* pSurface = nullptr;

    if (surface_gui->Lock((VOID**)&pSurface, p_wc4_main_surface_pitch) != S_OK)
        return;

    fBuff += subY * fWidth;
    pSurface += subY * *p_wc4_main_surface_pitch;
    for (UINT y = 0; y < subHeight; y++) {
        for (LONG x = 0; x < fWidth; x++)
            pSurface[x] = fBuff[x];

        pSurface += *p_wc4_main_surface_pitch;
        fBuff += fWidth;
    }

    surface_gui->Unlock();
    Display_Dx_Present(PRESENT_TYPE::gui);

}


// Adjust width, height and centre target point of FIRST person POV space views.
// __fastcall used here as class "this" is parsed on the ecx register.
//_____________________________________________________________
static void __fastcall Set_Space_View_POV1(void* p_space_class) {

    WORD* p_view_vars = (WORD*)p_space_class;

    WORD width = (WORD)spaceWidth;
    WORD height = (WORD)spaceHeight;

    p_view_vars[4] = width;
    p_view_vars[5] = height;

    DWORD* p_cockpit_class2 = ((DWORD**)p_space_class)[69];

    LONG xpos = p_cockpit_class2[40];
    LONG ypos = p_cockpit_class2[41];


    p_view_vars[6] = (WORD)(xpos / (float)GUI_WIDTH * width);
    p_view_vars[7] = (WORD)(ypos / (float)GUI_HEIGHT * height);

    //set variables used in the drawing of the highlighted circle when target in cross-hairs.
    LONG target_area_size = p_cockpit_class2[39];
    float x_unit = 0;
    float y_unit = 0;
    float x = 0;
    float y = 0;
    surface_space2D->GetPosition(&x, &y);
    surface_space2D->GetScaledPixelDimensions(&x_unit, &y_unit);

    if (is_space_scaled) {
        float space_scale_x = (float)spaceWidth / clientWidth;
        float space_scale_y = (float)spaceHeight / clientHeight;
        x *= space_scale_x;
        y *= space_scale_y;
        x_unit *= space_scale_x;
        y_unit *= space_scale_y;
    }
    *p_wc4_crosshair_target_x = (LONG)(x + xpos * x_unit);
    *p_wc4_crosshair_target_y = (LONG)(y + ypos * y_unit);
    *p_wc4_crosshair_target_area_size = (LONG)(target_area_size * y_unit);

    //Debug_Info_Flight("Set_Space_View_POV1 centre_x=%d, centre_y=%d, new_centre_x=%d, new_centre_y=%d", xpos, ypos, p_view_vars[6], p_view_vars[7]);
}


//_____________________________________________________
static void __declspec(naked) set_space_view_pov1(void) {
    //structure ptr ecx, holds various details regarding space flight
    //word @ [ecx + 0x8] = view width
    //word @ [ecx + 0xA] = view height
    //word @ [ecx + 0xC] = view x centre
    //word @ [ecx + 0xE] = view y centre
    //ptr & [ecx + 0x10C] ptr to structure containing cockpit_details. view_type is at [cockpit_details + 0x20]. view_type: cockpit = 0, left = 1, rear = 2, right = 3, hud = 4.
    __asm {
        push ebx
        push ebp
        push esi
        push edi


        //push ecx
        call Set_Space_View_POV1
        //add esp, 0x4

        pop edi
        pop esi
        pop ebp
        pop ebx

        ret 0x4
    }
}


// Adjust width, height and centre point of THIRD person POV space views.
// __fastcall used here as class "this" is parsed on the ecx register.
//_____________________________________________________________________________________
static void __fastcall Set_Space_View_POV3(void* p_space_class, DRAW_BUFFER_MAIN* p_db) {

    WORD* p_view_vars = (WORD*)p_space_class;

    //Debug_Info("Set_Space_View_POV3 - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", p_view_vars[0], p_view_vars[1], p_view_vars[2], p_view_vars[3], p_view_vars[4], p_view_vars[5], p_view_vars[6], p_view_vars[7],
    //    p_view_vars[8], p_view_vars[9], p_view_vars[10], p_view_vars[11], p_view_vars[12], p_view_vars[13], p_view_vars[14], p_view_vars[15]);

    WORD width = (WORD)spaceWidth;
    WORD height = (WORD)spaceHeight;

    //rear view vdu screen is 120*100, check for it here to prevent it being resized.
    if (p_db && p_db->rc.right == 119 && p_db->rc.bottom == 99) {
        //Debug_Info_Flight("Set_Space_View_POV3 p_db->rc.right=%d, p_db->rc.bottom=%d", p_db->rc.right, p_db->rc.bottom);
        width = 120;
        height = 100;
    }

    p_view_vars[4] = width;
    p_view_vars[5] = height;

    p_view_vars[6] = width / 2;
    p_view_vars[7] = height / 2;

    //Debug_Info("Set_Space_View_POV3 - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", p_view_vars[0], p_view_vars[1], p_view_vars[2], p_view_vars[3], p_view_vars[4], p_view_vars[5], p_view_vars[6], p_view_vars[7],
    //    p_view_vars[8], p_view_vars[9], p_view_vars[10], p_view_vars[11], p_view_vars[12], p_view_vars[13], p_view_vars[14], p_view_vars[15]);
}


//_____________________________________________________
static void __declspec(naked) set_space_view_pov3(void) {
    //structure ptr ecx, holds various details regarding space flight
    //word @ [ecx + 0x8] = view width
    //word @ [ecx + 0xA] = view height
    //word @ [ecx + 0xC] = view x centre
    //word @ [ecx + 0xE] = view y centre
    __asm {
        push ebx
        push ebp
        push esi
        push edi

        //push ecx
        mov edx, [esp+0x14]
        call Set_Space_View_POV3


        pop edi
        pop esi
        pop ebp
        pop ebx

        ret 0x4
    }
}


//_______________________________
static void Display_Space_Scene() {
    Display_Dx_Present(PRESENT_TYPE::space);
}


//For storing the general buffer and dimensions, that many functions draw too.
//So that we can switch them with the DX11 buffer for drawing 3D space.
DRAW_BUFFER db_3d_backup = { 0 };
BYTE* pbuffer_space_3D = nullptr;


//________________________________
static void Lock_3DSpace_Surface() {

    if (pbuffer_space_3D != nullptr) {
        Debug_Info_Error("Lock_3DSpace_Surface - buffer already locked!!!");
        return;
    }
    LONG buffer_space_3D_pitch = 0;

    if (surface_space3D->Lock((VOID**)&pbuffer_space_3D, &buffer_space_3D_pitch) != S_OK) {
        Debug_Info_Error("Lock_3DSpace_Surface - lock failed!!!");
        return;
    }

    //backup current buffer data
    db_3d_backup.rc_inv.right = (**pp_wc4_db_game_main).rc.right;
    db_3d_backup.rc_inv.bottom = (**pp_wc4_db_game_main).rc.bottom;
    db_3d_backup.rc_inv.left = (**pp_wc4_db_game).rc_inv.left;
    db_3d_backup.rc_inv.top = (**pp_wc4_db_game).rc_inv.top;
    db_3d_backup.buff = (*pp_wc4_db_game)->buff;

    //set buffer for drawing 3d space elements
    (**pp_wc4_db_game).buff = pbuffer_space_3D;

    (**pp_wc4_db_game_main).rc.right = spaceWidth - 1;
    (**pp_wc4_db_game_main).rc.bottom = spaceHeight - 1;

    (**pp_wc4_db_game).rc_inv.left = buffer_space_3D_pitch - 1;
    (**pp_wc4_db_game).rc_inv.top = spaceHeight - 1;

}


//_________________________________________________________
static void Lock_3DSpace_Surface_POV1(void* p_space_class) {
    //Debug_Info("Lock_3DSpace_Surface_POV1 SPACE VIEW POV1 w:%d, h:%d client  w:%d, h:%d", ((WORD*)p_space_class)[4], ((WORD*)p_space_class)[5], clientWidth, clientHeight);
    if (((WORD*)p_space_class)[4] != (WORD)spaceWidth || ((WORD*)p_space_class)[5] != (WORD)spaceHeight) {
        Debug_Info_Flight("Lock_3DSpace_Surface_POV1 RESIZING SPACE VIEW POV1");
        //DWORD* p_cockpit_class = ((DWORD**)p_space_class)[67];
        //Debug_Info("RESIZING SPACE VIEW POV1: %d", p_cockpit_class[8]);
        Set_Space_View_POV1(p_space_class);
    }
    Lock_3DSpace_Surface();
}


//___________________________________________________________
static void __declspec(naked) lock_3dspace_surface_pov1(void) {

    __asm {
        //push edx
        push ebx
        push esi
        
#ifdef VERSION_WC4_DVD
        push edi
        push ebp

        push edi
#else
        push ebx
#endif
        call Lock_3DSpace_Surface_POV1
        add esp, 0x4

#ifdef VERSION_WC4_DVD
        pop ebp
        pop edi
#endif
        pop esi
        pop ebx
        //pop edx
        ret
    }
}


//_________________________________________________________
static void Lock_3DSpace_Surface_POV3(void* p_space_struct) {
    //Debug_Info("Lock_3DSpace_Surface_POV3");
    if (((WORD*)p_space_struct)[4] != (WORD)spaceWidth || ((WORD*)p_space_struct)[5] != (WORD)spaceHeight) {
        //Debug_Info("RESIZING SPACE VIEW POV3");
        Set_Space_View_POV3((WORD*)p_space_struct, nullptr);
    }
    Lock_3DSpace_Surface();
}


//___________________________________________________________
static void __declspec(naked) lock_3dspace_surface_pov3(void) {

    __asm {
        //push edx
        push ebx
        push ebp
        push esi
        push edi

#ifdef VERSION_WC4_DVD
        push esi
#else
        push edi
#endif
        call Lock_3DSpace_Surface_POV3
        add esp, 0x4

        pop edi
        pop esi
        pop ebp
        pop ebx
        //pop edx
        ret
    }
}


//__________________________________
static void UnLock_3DSpace_Surface() {

    if (!pbuffer_space_3D) {
        Debug_Info_Error("UnLock_3DSpace_Surface - buffer wasn't locked!!!");
        return;
    }
    surface_space3D->Unlock();
    pbuffer_space_3D = nullptr;


    //restore backup buffer data
    (**pp_wc4_db_game).buff = db_3d_backup.buff;

    (**pp_wc4_db_game_main).rc.right = db_3d_backup.rc_inv.right;
    (**pp_wc4_db_game_main).rc.bottom = db_3d_backup.rc_inv.bottom;

    (**pp_wc4_db_game).rc_inv.left = db_3d_backup.rc_inv.left;
    (**pp_wc4_db_game).rc_inv.top = db_3d_backup.rc_inv.top;

}


//For storing the general buffer and dimensions, that many functions draw too.
//So that we can switch them with the DX11 buffer for drawing 2D space.
DRAW_BUFFER db_2d_backup = { 0 };
BYTE* pbuffer_space_2D = nullptr;
LONG buffer_space_2D_pitch = 0;

//________________________________
static void Lock_2DSpace_Surface() {
    //Debug_Info("Lock_2DSpace_Surface");
    if (pbuffer_space_2D != nullptr) {
        Debug_Info_Error("Lock_2DSpace_Surface - buffer already locked!!!");
        return;
    }

    if (surface_space2D->Lock((VOID**)&pbuffer_space_2D, &buffer_space_2D_pitch) != S_OK) {
        Debug_Info_Error("Lock_2DSpace_Surface - lock failed!!!");
        return;
    }

    //backup current buffer data
    db_2d_backup.rc_inv.right = (**pp_wc4_db_game_main).rc.right;
    db_2d_backup.rc_inv.bottom = (**pp_wc4_db_game_main).rc.bottom;

    db_2d_backup.rc_inv.left = (**pp_wc4_db_game).rc_inv.left;
    db_2d_backup.rc_inv.top = (**pp_wc4_db_game).rc_inv.top;

    db_2d_backup.buff = (*pp_wc4_db_game)->buff;


    //set buffer for drawing 2d space elements
    (**pp_wc4_db_game).buff = pbuffer_space_2D;

    (**pp_wc4_db_game_main).rc.right = GUI_WIDTH - 1;
    (**pp_wc4_db_game_main).rc.bottom = GUI_HEIGHT - 1;

    (**pp_wc4_db_game).rc_inv.left = buffer_space_2D_pitch - 1;
    (**pp_wc4_db_game).rc_inv.top = GUI_HEIGHT - 1;
}


//__________________________________
static void UnLock_2DSpace_Surface() {
    //Debug_Info("UnLock_2DSpace_Surface");
    if (!pbuffer_space_2D) {
        Debug_Info_Error("UnLock_2DSpace_Surface - buffer wasn't locked!!!");
        return;
    }
    surface_space2D->Unlock();
    pbuffer_space_2D = nullptr;


    //restore backup buffer data
    (**pp_wc4_db_game).buff = db_2d_backup.buff;

    (**pp_wc4_db_game_main).rc.right = db_2d_backup.rc_inv.right;
    (**pp_wc4_db_game_main).rc.bottom = db_2d_backup.rc_inv.bottom;

    (**pp_wc4_db_game).rc_inv.left = db_2d_backup.rc_inv.left;
    (**pp_wc4_db_game).rc_inv.top = db_2d_backup.rc_inv.top;
}


//_________________________________
static void Clear_2DSpace_Surface() {
    if (pbuffer_space_2D == nullptr)
        return;

    memset(pbuffer_space_2D, 0xFF, buffer_space_2D_pitch * GUI_HEIGHT);
    //memset(pbuffer_space_2D, 0x00, buffer_space_2D_pitch * GUI_HEIGHT);

}


//_____________________________________________________________________________
static void __declspec(naked) unlock_3dspace_surface_lock_2dspace_surface(void) {

    __asm {
        push ebx
        push esi

        call UnLock_3DSpace_Surface
        call Lock_2DSpace_Surface
        call Clear_2DSpace_Surface

        pop esi
        pop ebx
        ret
    }
}


//____________________________________________________________________
static void __declspec(naked) unlock_2dspace_surface_and_display(void) {

    __asm {
        push ebx
        push ebp
        push esi

        call UnLock_2DSpace_Surface
        call Display_Space_Scene

        pop esi
        pop ebp
        pop ebx
        ret
    }
}


//__________________________________________________________________________________
static void __declspec(naked) unlock_3dspace_surface_lock_2dspace_surface_pov3(void) {

    __asm {
        push ecx
        push ebx
        push ebp
        push esi

        call UnLock_3DSpace_Surface
        call Lock_2DSpace_Surface
        call Clear_2DSpace_Surface

        pop esi
        pop ebp
        pop ebx
        pop ecx

        call wc4_draw_hud_view_text
        ret
    }
}


//_____________________________________
static void Fix_CockpitViewTargetRect() {

    if (!surface_space2D)
        return;
    float x_unit = 0;
    float y_unit = 0;
    float x = 0;
    float y = 0;
    surface_space2D->GetPosition(&x, &y);
    surface_space2D->GetScaledPixelDimensions(&x_unit, &y_unit);

    if (is_space_scaled) {
        float space_scale_x = (float)spaceWidth / clientWidth;
        float space_scale_y = (float)spaceHeight / clientHeight;
        x *= space_scale_x;
        y *= space_scale_y;
        x_unit *= space_scale_x;
        y_unit *= space_scale_y;
    }
    (**pp_wc4_db_game_main).rc.left = (LONG)((**pp_wc4_db_game_main).rc.left * x_unit + x);
    (**pp_wc4_db_game_main).rc.top = (LONG)((**pp_wc4_db_game_main).rc.top * y_unit + y);
    (**pp_wc4_db_game_main).rc.right = (LONG)((**pp_wc4_db_game_main).rc.right * x_unit + x);
    (**pp_wc4_db_game_main).rc.bottom = (LONG)((**pp_wc4_db_game_main).rc.bottom * y_unit + y);
}


//______________________________________________________________
static void __declspec(naked) fix_cockpit_view_target_rect(void) {

    __asm {
        push ebx
        push edi
        push esi
        call Fix_CockpitViewTargetRect
        pop esi
        pop edi
        pop ebx
        
#ifdef VERSION_WC4_DVD
        mov eax, pp_wc4_db_game_main
        mov eax, dword ptr ds : [eax]
#else
        mov ecx, pp_wc4_db_game_main
        mov ecx, dword ptr ds : [ecx]
#endif
        ret
    }
}


//_____________________________________________________________
static void __declspec(naked) draw_hud_targeting_elements(void) {

    __asm {
        push edx
        push ebx
        push ebp
        push esi
        push edi

        push ecx
        call Lock_3DSpace_Surface
        pop ecx

        call wc4_draw_hud_targeting_elements

        call UnLock_3DSpace_Surface

        pop edi
        pop esi
        pop ebp
        pop ebx
        pop edx
        ret
    }
}


//_________________________________________________________________________________________________________________________________________
static BOOL Draw_HUD_Tractor_Beam_Targeting_Circle(DRAW_BUFFER_MAIN* p_toBuff, LONG x, LONG y, DWORD width, DWORD height, DWORD pal_offset) {
    
    Lock_3DSpace_Surface();

    //fix width and height of tractor circle.
    float length = (float)spaceHeight;
    if (spaceWidth < spaceHeight)
        length = (float)spaceWidth;
    width = (LONG)(length / 480.0f * width);
    height = (LONG)(length / 480.0f * height);

    BOOL ret_val = wc4_draw_circle(p_toBuff, x, y, width, height, pal_offset);

    UnLock_3DSpace_Surface();

    return ret_val;
}


//___________________________________________
static LONG Fix_Hud_Targeting_Rect_Max_Size() {

    float length = (float)spaceHeight;
    if (spaceWidth < spaceHeight)
        length = (float)spaceWidth;
    //28 was the original max value.
    return (LONG)(length / 480.0f * 28.0f);
}


//_________________________________________________________________
static void __declspec(naked) fix_hud_targeting_rect_max_size(void) {

    __asm {
        push eax
        push ecx
        push esi
        push edi
 
        call Fix_Hud_Targeting_Rect_Max_Size
        mov edx, eax

        pop edi
        pop esi
        pop ecx
        pop eax
        ret
    }
}


//___________________________________________________________________
static void __declspec(naked) set_input_profile_nav_map_3d_draw(void) {

    __asm {
        pushad
        push PROFILE_NAV
        call Change_Profile_Type
        add esp, 0x4
        popad

        push ebp
        push edi

        push ecx

        call Lock_3DSpace_Surface

        pop ecx
        call wc4_nav_screen

        call UnLock_3DSpace_Surface

        pop edi
        pop ebp

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


//___________________________________________________________________
static void __declspec(naked) nav_unlock_3d_and_lock_2d_drawing(void) {

    __asm {
        push eax
        push edx
        push ecx
        push ebx
        push esi
        push edi
        push ebp

        call UnLock_3DSpace_Surface
        call Lock_2DSpace_Surface
        call Clear_2DSpace_Surface

        pop ebp
        pop edi
        pop esi
        pop ebx
        pop ecx
        pop edx
        pop eax

#ifdef VERSION_WC4_DVD
        lea eax, [edx + edi]
        add ecx, ebx
#else
        add ecx, edi
        dec edi
        add ecx, ebp
#endif

        ret
    }
}


//_____________________________________________________________________
static void __declspec(naked) nav_unlock_2d_and_display_relock_3d(void) {

    __asm {
        push ebp
        push esi

        call UnLock_2DSpace_Surface
        mov is_nav_view, 1
        call Display_Space_Scene
        mov is_nav_view, 0
        call Lock_3DSpace_Surface

        pop esi
        pop ebp

        ret
    }
}


//____________________________________________
static void SetWindowActivation(BOOL isActive) {

    //When game window loses focus, fullscreen mode needs to temporarily be put into windowed mode in order to appear on the taskbar and alt-tab display.
    if (!*p_wc4_is_windowed) {
        if (isActive == FALSE) {//Convert to windowed mode when app loses focus.
            SetWindowLongPtr(*p_wc4_hWinMain, GWL_EXSTYLE, 0);
            SetWindowLongPtr(*p_wc4_hWinMain, GWL_STYLE, WIN_MODE_STYLE | WS_VISIBLE);
            SetWindowPos(*p_wc4_hWinMain, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
            ShowWindow(*p_wc4_hWinMain, SW_RESTORE);
            //Debug_Info("SetWindowActivation full to win");
        }
        else if (isActive) {//Return to fullscreen mode when app regains focus.
            SetWindowLongPtr(*p_wc4_hWinMain, GWL_EXSTYLE, WS_EX_TOPMOST);
            SetWindowLongPtr(*p_wc4_hWinMain, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(*p_wc4_hWinMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
            ShowWindow(*p_wc4_hWinMain, SW_MAXIMIZE);
            //Debug_Info("SetWindowActivation win to full");
        }
    }
}


//________________________________________
void Set_WindowActive_State(BOOL isActive) {
    SetWindowActivation(isActive);
}


//______________________________________
static void Toggle_WindowMode(HWND hwnd) {

    *p_wc4_is_windowed = 1 - *p_wc4_is_windowed;
    ConfigWriteInt_InGame(L"MAIN", L"WINDOWED", *p_wc4_is_windowed);

    if (*p_wc4_is_windowed) {
        Debug_Info("Toggle_WindowMode: Windowed");
        WINDOWPLACEMENT winPlace{ 0 };
        winPlace.length = sizeof(WINDOWPLACEMENT);

        SetWindowLongPtr(hwnd, GWL_STYLE, WIN_MODE_STYLE);

        if (ConfigReadWinData(L"MAIN", L"WIN_DATA", &winPlace)) {
            if (winPlace.showCmd != SW_MAXIMIZE)
                winPlace.showCmd = SW_SHOWNORMAL;
        }
        else {
            GetWindowPlacement(hwnd, &winPlace);
            winPlace.showCmd = SW_SHOWNORMAL;
            Debug_Info("is_windowed GetWindowPlacement");
        }
        if (winPlace.showCmd == SW_SHOWNORMAL) //if the window isn't maximized
            Check_Window_GUI_Scaling_Limits(hwnd, &winPlace.rcNormalPosition, false);

        SetWindowPlacement(hwnd, &winPlace);

    }
    else {//Return to fullscreen mode when app regains focus.
        Debug_Info("Toggle_WindowMode: Fullscreen");
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
        //SetWindowPos(hwnd, 0, 0, 0, 0, 0, 0);
        ShowWindow(hwnd, SW_MAXIMIZE);
    }
    Display_Dx_Present();
}


//return FALSE to call DefWindowProc
//_____________________________________________________________________________
static BOOL WinProc_Main(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    static bool is_cursor_hidden = true;
    static bool is_in_sizemove = false;

    switch (Message) {
    /*case WM_KEYDOWN:
        if (!(lParam & 0x40000000)) { //The previous key state. The value is 1 if the key is down before the message is sent, or it is zero if the key is up.
            if (wParam == VK_F11) { //Use F11 key to toggle windowed mode.
                if (pMovie_vlc)
                    pMovie_vlc->Pause(true);
                if (pMovie_vlc_Inflight)
                    pMovie_vlc_Inflight->Pause(true);
                Toggle_WindowMode(hwnd);
            }
        }
        break;*/
    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS* winpos = (WINDOWPOS*)lParam;
        //Debug_Info("WM_WINDOWPOSCHANGING size adjusting");
        RECT rcWindow = { winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy };
        Check_Window_GUI_Scaling_Limits(hwnd, &rcWindow, true);
        winpos->x = rcWindow.left;
        winpos->y = rcWindow.top;
        winpos->cx = rcWindow.right - rcWindow.left;
        winpos->cy = rcWindow.bottom - rcWindow.top;
        return FALSE;
    }
    case WM_WINDOWPOSCHANGED: {
        //Debug_Info("WM_WINDOWPOSCHANGED");
        if (IsIconic(hwnd))
            break;
        WINDOWPOS* winpos = (WINDOWPOS*)lParam;

        if (!is_in_sizemove) {
            if (!(winpos->flags & (SWP_NOSIZE))) {
                //Debug_Info("WM_WINDOWPOSCHANGED is_in_sizemove");
                Window_Resized();
                if (pMovie_vlc) {
                    pMovie_vlc->Pause(false);
                    pMovie_vlc->SetScale();
                }
                if (pMovie_vlc_Inflight) {
                    pMovie_vlc_Inflight->Pause(false);
                    pMovie_vlc_Inflight->Update_Display_Dimensions(nullptr);
                }
            }
            SetWindowTitle(hwnd, L"");
        }
        //SetWindowTitle(hwnd, L"");
        return TRUE; //this should prevent calling DefWindowProc, and stop WM_SIZE and WM_MOVE messages being sent.
    }
    case WM_ENTERSIZEMOVE:
        //Debug_Info("WM_ENTERSIZEMOVE");
        is_in_sizemove = true;
        if (pMovie_vlc)
            pMovie_vlc->Pause(true);
        if (pMovie_vlc_Inflight)
            pMovie_vlc_Inflight->Pause(true);
        break;

    case WM_EXITSIZEMOVE:
        //Debug_Info("WM_EXITSIZEMOVE");
        is_in_sizemove = false;
        Window_Resized();
        if (pMovie_vlc) {
            pMovie_vlc->Pause(false);
            pMovie_vlc->SetScale();
        }
        if (pMovie_vlc_Inflight) {
            pMovie_vlc_Inflight->Pause(false);
            pMovie_vlc_Inflight->Update_Display_Dimensions(nullptr);
        }
        SetWindowTitle(hwnd, L"");
        break;
    case WM_CLOSE: {
        if (IsIconic(hwnd)) {//restore window first - game needs focus to exit
            if (SetForegroundWindow(hwnd))
                ShowWindow(hwnd, SW_RESTORE);
        }
        /*if (*pp_wc4_music_thread_class) {
            wc4_music_thread_class_destructor(*pp_wc4_music_thread_class);
            wc4_dealocate_mem01(*pp_wc4_music_thread_class);
            *pp_wc4_music_thread_class = nullptr;
            Debug_Info("pp_wc4_music_thread_class destroyed");
        }*/
        PostMessage(hwnd, WM_DESTROY, 0, 0);
        return TRUE;
    }
    case WM_ENTERMENULOOP://allows system menu keys to fuction
        Set_WindowActive_State(FALSE);
        break;
    case WM_EXITMENULOOP:
        Set_WindowActive_State(TRUE);
        break;
    case WM_DISPLAYCHANGE:
        break;
    case WM_COMMAND:
        switch (wParam) {
        case 40005:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
            break;
        case 40002:
            //Debug_Info("dx message");
            Window_Setup(hwnd);
            SetWindowTitle(hwnd, L"");
            ShowWindow(hwnd, SW_SHOW);
            return 0;
            break;
        case 40004:
            //Debug_Info("40004 Calibrate Joystick");
            //wc4_unknown_func01();
            return 0;
            break;
        default:
            return 0;
            break;
        }
        break;
    case WM_SYSCOMMAND:
        switch ((wParam & 0xFFF0)) {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;
            break;
        case SC_MAXIMIZE:
            //Debug_Info("SC_MAXIMIZE");
        case SC_RESTORE:
            //Debug_Info("SC_MAXIMIZE/SC_RESTORE");
            if (pMovie_vlc)
                pMovie_vlc->Pause(true);
            if (pMovie_vlc_Inflight)
                pMovie_vlc_Inflight->Pause(true);
            return 0;
            break;
        default:
            break;
        }
        break;
    case WM_SETCURSOR: {
        DWORD currentWinStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (GetForegroundWindow() == hwnd && (currentWinStyle & WS_POPUP) || (clip_cursor == TRUE)) {
            if (!is_cursor_clipped) {
                if (ClipMouseCursor()) {
                    is_cursor_clipped = true;
                    //Debug_Info("WM_SETCURSOR Mouse Cursor Clipped");
                }
                //else
                //    Debug_Info("WM_SETCURSOR ClipMouseCursor failed");
            }
            break;
        }
        if (is_cursor_clipped) {
            ClipCursor(nullptr);
            is_cursor_clipped = false;
            //Debug_Info("WM_SETCURSOR Mouse Cursor Un-Clipped");
        }
        if (hWin_Config_Control)
            break;//dont alter the cursor visibility when joy config window open.
        WORD ht = LOWORD(lParam);
        if (HTCLIENT == ht) {

            SetCursor(LoadCursor(nullptr, IDC_ARROW));

            if (IsMouseInClient()) {
                if (!is_cursor_hidden) {
                    is_cursor_hidden = true;
                    ShowCursor(false);
                }
            }
            else {
                if (is_cursor_hidden) {
                    is_cursor_hidden = false;
                    ShowCursor(true);
                }
            }
        }
        else {
            if (is_cursor_hidden) {
                is_cursor_hidden = false;
                ShowCursor(true);
            }
        }
        break;
    }
    case WM_ACTIVATEAPP:
        Set_WindowActive_State(wParam);
        if (wParam == FALSE) {
            Debug_Info("WM_ACTIVATEAPP false");
            if (is_cursor_clipped) {
                ClipCursor(nullptr);
                is_cursor_clipped = false;
                //Debug_Info("WM_ACTIVATEAPP false, Mouse Cursor Un-Clipped");
            }
            if (pMovie_vlc)
                pMovie_vlc->Pause(true);
            if (pMovie_vlc_Inflight)
                pMovie_vlc_Inflight->Pause(true);
            if (p_Music_Player)
                p_Music_Player->Pause(true);
        }
        else {
            Debug_Info("WM_ACTIVATEAPP true");
            if (is_cursor_clipped) {
                if (ClipMouseCursor()) {
                    //Debug_Info("WM_ACTIVATEAPP Mouse Cursor Clipped");
                }
            }
            if (pMovie_vlc)
                pMovie_vlc->Pause(false);
            if (pMovie_vlc_Inflight)
                pMovie_vlc_Inflight->Pause(false);
            if (p_Music_Player)
                p_Music_Player->Pause(false);
        }
        return 0;
        //case WM_ERASEBKGND:
        //    return 1;
        //case WM_DESTROY:
        //    Debug_Info("WM_DESTROY");
        //   return 1;
    case WM_PAINT:
        ValidateRect(hwnd, nullptr);
        return 1;
        break;
    default:
        break;
    }
    return 0;
}


#ifdef VERSION_WC4_DVD
//Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
//_____________________________________________________________
static void __declspec(naked) winproc_movie_message_check(void) {

    __asm {
        add ecx, 0x111
        cmp ecx, WM_COMMAND
        jne check2
        mov dl, 0
        ret
        check2 :
        cmp ecx, WM_INITMENU
            jne check3
            mov dl, 1
            ret
            check3 :
        /*cmp ecx, WM_LBUTTONDBLCLK
            jne check4
            mov dl, 2
            ret
            check4 :
        cmp ecx, WM_RBUTTONDBLCLK
            jne check5
            mov dl, 3
            ret
            check5 :*/
        cmp ecx, WM_ENTERSIZEMOVE
            jne check6
            mov dl, 1
            ret
            check6 :
        cmp ecx, WM_EXITSIZEMOVE
            jne end_check
            mov dl, 1
            ret
            end_check :
        mov dl, 4
            ret
    }
}

#else
//Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
//_____________________________________________________________
static void __declspec(naked) winproc_movie_message_check(void) {

    __asm {
        add eax, 0x111
        cmp eax, WM_COMMAND
        jne check2
        mov cl, 0
        ret
        check2 :
        cmp eax, WM_INITMENU
            jne check3
            mov cl, 1
            ret
            check3 :
        /*cmp eax, WM_LBUTTONDBLCLK
            jne check4
            mov cl, 2
            ret
            check4 :
        cmp eax, WM_RBUTTONDBLCLK
            jne check5
            mov cl, 3
            ret
            check5 :*/
        cmp eax, WM_ENTERSIZEMOVE
            jne check6
            mov cl, 1
            ret
            check6 :
        cmp eax, WM_EXITSIZEMOVE
            jne end_check
            mov cl, 1
            ret
            end_check :
        mov cl, 4
            ret
    }
}
#endif


//__________________________________________________
static void Start_Display_Setup(BOOL no_full_screen) {

    if(ConfigReadInt(L"MAIN", L"ENABLE_CONTROLLER_ENHANCEMENTS", CONFIG_MAIN_ENABLE_CONTROLLER_ENHANCEMENTS))
        Modifications_Controller_Enhancements(); //Modifications_Joystick();
    if (ConfigReadInt(L"MAIN", L"ENABLE_MUSIC_ENHANCEMENTS", CONFIG_MAIN_ENABLE_MUSIC_ENHANCEMENTS))
        Modifications_Music();

    if (no_full_screen)
        *p_wc4_is_windowed = TRUE;
    else
        *p_wc4_is_windowed = FALSE;

    SendMessage(*p_wc4_hWinMain, WM_COMMAND, (WPARAM)40002, (LPARAM)0);
    ShowCursor(FALSE);
}


//_____________________________________________________
static void __declspec(naked) start_display_setup(void) {

    __asm {
        pushad
        push eax
        call Start_Display_Setup
        add esp, 0x4
        popad
        ret
    }
}


//____________________________________________
static void Conversation_Decision_ClipCursor() {
    clip_cursor = TRUE;
    wc4_conversation_decision_loop();
    clip_cursor = FALSE;
}

/*
//______________________________________________________
static WORD* Translate_Messages_Mouse_ClipCursor_Space() {
    clip_cursor = TRUE;
    wc4_translate_messages(TRUE, FALSE);
    clip_cursor = FALSE;
    return p_wc4_mouse_button_space;
}
*/

//_________________________________________________________
static void __declspec(naked) overide_cursor_clipping(void) {

    __asm {
        //pushad
        //call Print_Space_Time
        //popad
        mov clip_cursor, TRUE
        call wc4_update_input_states
        mov clip_cursor, FALSE
        ret
    }
}


//_________________________________________________________________
static void __declspec(naked) set_input_profile_space_mission(void) {

    __asm {
        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad

        call wc4_space_mission

        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        ret
    }
}


//__________________________________________________________________
static void __declspec(naked) options_screen_set_input_profile(void) {

    __asm {
        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        call wc4_options_screen

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


//_________________________________________________________________________
static void __declspec(naked) set_input_profile_space_exit_game_start(void) {
    //0040AA73 | .A1 D8E84B00                  MOV EAX, DWORD PTR DS : [space_exit_game_flag]
    __asm {

        mov eax, p_wc4_space_exit_game_option_flag
        mov eax, dword ptr ds : [eax]
        cmp eax, 0
        je exit_func
        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        exit_func :
        ret
    }
}


//_______________________________________________________________________
static void __declspec(naked) set_input_profile_space_exit_game_end(void) {
    //0040AB32 | .A3 D8E84B00                  MOV DWORD PTR DS : [space_exit_game_flag] , EAX
    __asm {
#ifdef VERSION_WC4_DVD
        mov eax, p_wc4_space_exit_game_option_flag
        mov dword ptr ds:[eax], ebx
#else
        mov ebx, p_wc4_space_exit_game_option_flag
        mov dword ptr ds:[ebx], eax
#endif

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


//__________________________________________________________________________
static void __declspec(naked) set_input_profile_space_pause_game_start(void) {

    __asm {

        mov eax, p_wc4_space_pause_game_option_flag
        mov eax, dword ptr ds : [eax]
        cmp eax, 0
        je exit_func
        pushad
        push PROFILE_GUI
        call Change_Profile_Type
        add esp, 0x4
        popad

        exit_func :
        ret
    }
}


//_________________________________________________________________________
static void __declspec(naked) set_input_profile_space_pause_game_end(void) {

    __asm {
#ifdef VERSION_WC4_DVD
        mov eax, p_wc4_space_pause_game_option_flag
        mov dword ptr ds : [eax] , ebx
#else
        mov ebx, p_wc4_space_pause_game_option_flag
        mov dword ptr ds : [ebx] , eax
#endif

        pushad
        push PROFILE_SPACE
        call Change_Profile_Type
        add esp, 0x4
        popad
        ret
    }
}


//_________________________________________________________________________________________________
static void Mio_Screen_Loop(DWORD Arg1, DWORD Arg2, DWORD Arg3, DWORD Arg4, DWORD Arg5, DWORD Arg6) {
    
    PROFILE_TYPE last_pro_type = current_pro_type;

    Change_Profile_Type(PROFILE_TYPE::GUI);
    //Debug_Info("Mio_Screen_Loop %X,%X,%X,%X,%X,%X", Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
    wc4_mio_screen_loop(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
    Change_Profile_Type(last_pro_type);
}


// Throttle movement speed on the Navigation screen, which was unregulated and way to fast on modern computers.
// __fastcall used here as class "this" is parsed on the ecx register.
//__________________________________________________________________
static void __fastcall NavScreen_Movement_Speed_Fix(void*p_this_nav) {

    static LARGE_INTEGER lastPresentTime = { 0 };
    LARGE_INTEGER time = { 0 };
    LARGE_INTEGER ElapsedMicroseconds = { 0 };

    QueryPerformanceCounter(&time);

    ElapsedMicroseconds.QuadPart = time.QuadPart - lastPresentTime.QuadPart;
    if (ElapsedMicroseconds.QuadPart < 0 || ElapsedMicroseconds.QuadPart > nav_update_time.QuadPart) {
        lastPresentTime.QuadPart = time.QuadPart;
        wc4_nav_screen_update_position(p_this_nav);
        return;
    }

    return;
}


//_______________________________________________________________
static void __declspec(naked) nav_screen_movement_speed_fix(void) {

    __asm {
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp

        call NavScreen_Movement_Speed_Fix

        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx

        ret
    }
}


//________________________________________________________________________
void Palette_Update_Main(BYTE* p_pal_buff, BYTE offset, DWORD num_entries) {
    int main_offset = (int)offset * 3;
    if ((LONG)num_entries > 0) {
        for (int i = 0; i < (int)num_entries * 3; i += 3) {
            p_wc4_main_pal[main_offset + i] = p_pal_buff[i];
            p_wc4_main_pal[main_offset + i + 1] = p_pal_buff[i + 1];
            p_wc4_main_pal[main_offset + i + 2] = p_pal_buff[i + 2];
        }
    }
    Palette_Update(p_pal_buff, offset, num_entries);
}


//_________________________________________________________
static void Check_System_Keys(WPARAM wParam, LPARAM lParam) {

    //Debug_Info("Check_System_Keys wParam%X, lParam%X", wParam, lParam);
    if ((lParam & (1 << 29)) != 0) { //if ALT key is down.
        if ((lParam & (1 << 31)) != 0) {//if key is down
            if (wParam == VK_RETURN) { //toggle windowed mode.
                if (pMovie_vlc)
                    pMovie_vlc->Pause(true);
                if (pMovie_vlc_Inflight)
                    pMovie_vlc_Inflight->Pause(true);
                Toggle_WindowMode(*p_wc4_hWinMain);
            }
            else if (wParam == 'J') {//controller setup
                if (is_cursor_clipped)
                    ClipCursor(nullptr);
                JoyConfig_Main();
                if (is_cursor_clipped)
                    ClipMouseCursor();
            }
        }
    }
}


//_______________________________________________
static void __declspec(naked) check_sys_key(void) {

    __asm {
        mov edx, dword ptr ss : [esp + 0x18]//lParam
        
        pushad
        push ecx
        push edx
        call Check_System_Keys
        add esp, 0x8
        popad

        //insert original code
#ifdef VERSION_WC4_DVD
        mov edx, eax
        not ecx
        shr edx, 0x18
#else
        not eax
        shr eax, 0x1F
#endif
        ret
    }
}


#ifdef VERSION_WC4_DVD
//__________________________
void Modifications_Display() {

    MemWrite8(0x416B20, 0xA1, 0xE9);
    FuncWrite32(0x416B21, 0x4BBD44, (DWORD)&Display_Exit);

    MemWrite8(0x477400, 0x8B, 0xE9);
    FuncWrite32(0x477401, 0x530C2444, (DWORD)&Palette_Update_Main);

    MemWrite8(0x4763F0, 0xA1, 0xE9);
    FuncWrite32(0x4763F1, 0x4B720C, (DWORD)&LockSurface);

    MemWrite8(0x4764F0, 0xA1, 0xE9);
    FuncWrite32(0x4764F1, 0x4B720C, (DWORD)&UnlockShowSurface);

    //disable un-needed colour_fill function.
    MemWrite16(0x417450, 0xEC83, 0xC033);//xor eax, eax
    MemWrite8(0x417452, 0x64, 0xC3);

    //replace 8bit clear_screen function.
    MemWrite8(0x4775B0, 0xA1, 0xE9);
    FuncWrite32(0x4775B1, 0x4B720C, (DWORD)&Clear_GUI_Surface);
    //replace 16bit clear_screen function.
    MemWrite8(0x477670, 0xA1, 0xE9);
    FuncWrite32(0x477671, 0x4B720C, (DWORD)&Clear_GUI_Surface);

    MemWrite16(0x47A7E0, 0xEC83, 0x9090);
    MemWrite8(0x47A7E2, 0x6C, 0x90);
    MemWrite8(0x47A7E3, 0xA1, 0xE9);
    FuncWrite32(0x47A7E4, 0x4BBD50, (DWORD)&LockSurface);

    MemWrite16(0x47A830, 0xEC81, 0xE990);
    FuncWrite32(0x47A832, 0x84, (DWORD)&UnlockShowSurface);

    //prevent direct draw - create surface func call
    MemWrite16(0x417490, 0xEC83, 0xC033);//xor eax, eax
    MemWrite8(0x417492, 0x6C, 0xC3);

    //replace direct draw - blit func
    MemWrite8(0x477770, 0xA1, 0xE9);
    FuncWrite32(0x477771, 0x4B720C, (DWORD)&DXBlt);

    //replace direct draw - blit 16bit func
    MemWrite16(0x477970, 0xA151, 0xE990);
    FuncWrite32(0x477972, 0x4CBAD8, (DWORD)&DXBlt);


    //--Prevent 16 and 24 bit drawing routines from taking effect as this patch handles such things-----------------
    //prevent direct draw - palette update func call
    MemWrite8(0x417430, 0xA1, 0xC3);

    //Prevent cmd line arg true colour mode from being enabled.
    MemWrite16(0x46FA51, 0x1D89, 0xA390);//set to eax

    //Force current video colour bit level to stay at 1(8 bit colour) in set game flags function.
    MemWrite32(0x46AA3A, 0x02, 0x01);

    //prevent movie and gameflow bit level being set to 16bit in settings menu.
    //movie_video_colour_bit_level
    //MemWrite32(0x45B62D, 0x02, 0x01);
    //gameflow_video_colour_bit_level
    //MemWrite32(0x45B6C8, 0x02, 0x01);
    //--------------------------------------------------------------------------------------------------------------

    //replace space first person view setup function
    MemWrite8(0x409A10, 0x8B, 0xE9);
    FuncWrite32(0x409A11, 0x56042444, (DWORD)&set_space_view_pov1);

    //replace direct draw lock surface in draw space first person view function
    MemWrite16(0x409BD2, 0x1D39, 0xE890);
    FuncWrite32(0x409BD4, 0x4B7214, (DWORD)&lock_3dspace_surface_pov1);
    MemWrite8(0x409BD8, 0x74, 0xEB);//jmp over ddraw stuff

    //replace direct draw stuff in draw space first person view function - unlock 3d space surface then lock 2d surface for hud etc.
    MemWrite16(0x409CCC, 0x840F, 0x9090);//prevent jumping before this is called
    MemWrite32(0x409CCE, 0xFB, 0x90909090);

    MemWrite16(0x409CD2, 0x1D39, 0xE890);
    FuncWrite32(0x409CD4, 0x4BBD4C, (DWORD)&unlock_3dspace_surface_lock_2dspace_surface);
    MemWrite16(0x409CD8, 0x840F, 0xE990);//jmp over ddraw stuff

    //replace direct draw stuff in draw space first person view function - unlock 2d surface then display.
    FuncReplace32(0x409E22, 0x06C6CA, (DWORD)&unlock_2dspace_surface_and_display);

    //draw targeting elements to 3d space
    FuncReplace32(0x40EB5E, 0x05C4BE, (DWORD)&draw_hud_targeting_elements);

    //draw tractor beam targeting circle to 3D space
    FuncReplace32(0x40ED0F, 0x080ED5, (DWORD)&Draw_HUD_Tractor_Beam_Targeting_Circle);

    //fix the max size of targeting rect to match the ratio between it and the original screen size.
    MemWrite8(0x46C06D, 0xBA, 0xE8);
    FuncWrite32(0x46C06E, 0x1C, (DWORD)&fix_hud_targeting_rect_max_size);

    MemWrite8(0x4067E4, 0xA1, 0xE8);
    FuncWrite32(0x4067E5, 0x4B7214, (DWORD)&lock_3dspace_surface_pov3);
    MemWrite16(0x4067E9, 0xC085, 0x9090);
    MemWrite8(0x4067EB, 0x74, 0xEB);//jmp over ddraw stuff

    //replace direct draw stuff in draw space third person view function - unlock 3d space surface then lock 2d surface for text etc.
    FuncReplace32(0x4068D8, 0x044504, (DWORD)&unlock_3dspace_surface_lock_2dspace_surface_pov3);

    //replace direct draw stuff in draw space third person view function - unlock 2d surface then display.
    FuncReplace32(0x406901, 0x06FBEB, (DWORD)&unlock_2dspace_surface_and_display);

    //replace space third person view setup function
    MemWrite16(0x482260, 0x448B, 0xE990);
    FuncWrite32(0x482262, 0x8B660424, (DWORD)&set_space_view_pov3);
    MemWrite16(0x482266, 0x0C50, 0x9090);

    //fix display rectangle for targeting elements
    MemWrite8(0x46B0B8, 0xA1, 0xE8);
    FuncWrite32(0x46B0B9, 0x4C5074, (DWORD)&fix_cockpit_view_target_rect);

    //draw nav screen space view to 3d surface, seperate from 2d elements
    FuncReplace32(0x44530F, 0x2D, (DWORD)&set_input_profile_nav_map_3d_draw);

    //set 2d surface for drawing nav screen 2d elements
    MemWrite8(0x445564, 0x8D, 0xE8);
    FuncWrite32(0x445565, 0xCB031704, (DWORD)&nav_unlock_3d_and_lock_2d_drawing);

    //set 3d surface after drawing nav screen 2d elements
    MemWrite16(0x4457B4, 0x15FF, 0xE890);
    FuncWrite32(0x4457B6, 0x4B7238, (DWORD)&nav_unlock_2d_and_display_relock_3d);

    //Set space subtitle text background colour to 0. As original 255 coflicts with the mask colour being used to draw all cockpit/hud elements to a seperate surface.
    MemWrite8(0x44AF4D, 0xFF, 0x00);

    //disables setup window function.
    MemWrite8(0x477C70, 0xA1, 0xC3);
    //jmp over call to initialize window/dx in message box error function.
    MemWrite8(0x4A1993, 0x75, 0xEB);

    //start display setup passing "-no_full_screen" cmd line value.
    MemWrite8(0x4769E0, 0x74, 0xEB);

    MemWrite8(0x4769FB, 0x55, 0x90);
    MemWrite16(0x4769FC, 0x15FF, 0xE890);
    FuncWrite32(0x4769FE, 0x4D4548, (DWORD)&start_display_setup);

    //replace the main window message checking function for greater functionality.
    MemWrite8(0x476B60, 0x8B, 0xE9);
    FuncWrite32(0x476B61, 0x56082444, (DWORD)&WinProc_Main);

    //Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
    //increase range of message code selection
    MemWrite32(0x47B3DB, 0xF5, 0x121);

    //check for the messages
    MemWrite16(0x47B3E7, 0x918A, 0xE890);
    FuncWrite32(0x47B3E9, 0x47B500, (DWORD)&winproc_movie_message_check);

    //clip the cursor while a conversation decision is made.
    FuncReplace32(0x49DD33, 0xFFFFFF19, (DWORD)&Conversation_Decision_ClipCursor);

    //Enable cursor clipping for all WM_SETCURSOR messages within calls to this function, used during space flight.
    FuncReplace32(0x46A991, 0x0004B6B, (DWORD)&overide_cursor_clipping);

    //fix control reaction speed on the nav screen.
    FuncReplace32(0x445478, 0xFFFFF484, (DWORD)&nav_screen_movement_speed_fix);

    //----fix-highlight-when-target-in-cross-hairs----
    //remove setting of highlight variables here.
    //highlight variables are set in "Set_Space_View_POV1" function.
    MemWrite16(0x40EB0D, 0x8B8B, 0x9090);
    MemWrite32(0x40EB0F, 0x9C, 0x90909090);
    MemWrite16(0x40EB13, 0x0D89, 0x9090);
    MemWrite32(0x40EB15, 0x4BB9E0, 0x90909090);
    MemWrite16(0x40EB19, 0x938B, 0x9090);
    MemWrite32(0x40EB1B, 0xA0, 0x90909090);
    MemWrite16(0x40EB1F, 0x1589, 0x9090);
    MemWrite32(0x40EB21, 0x4BB9F0, 0x90909090);
    MemWrite16(0x40EB25, 0x838B, 0x9090);
    MemWrite32(0x40EB27, 0xA4, 0x90909090);
    MemWrite8(0x40EB2B, 0xA3, 0x90);
    MemWrite32(0x40EB2C, 0x4BB9E4, 0x90909090);
    //------------------------------------------------

    //set input profile to space when flying a mission.
    FuncReplace32(0x470850, 0xFFFE4093, (DWORD)&set_input_profile_space_mission);

    //set input profile to gui when starting exit game screen.
    MemWrite8(0x440053, 0xA1, 0xE8);
    FuncWrite32(0x440054, 0x4BC6B0, (DWORD)&set_input_profile_space_exit_game_start);
    //set input profile back to space when ending exit game screen.
    MemWrite16(0x440108, 0x1D89, 0xE890);
    FuncWrite32(0x44010A, 0x4BC6B0, (DWORD)&set_input_profile_space_exit_game_end);

    //set input profile to gui when starting pause game screen.
    MemWrite16(0x44011A, 0x1D39, 0xE890);
    FuncWrite32(0x44011C, 0x4BC6C8, (DWORD)&set_input_profile_space_pause_game_start);
    //set input profile back to space when ending pause game screen.
    MemWrite16(0x44018D, 0x1D89, 0xE890);
    FuncWrite32(0x44018F, 0x4BC6C8, (DWORD)&set_input_profile_space_pause_game_end);

    //set input profile to gui when using options screen(Alt+O)
    FuncWrite32(0x40717B, 0x04EA71, (DWORD)&options_screen_set_input_profile);

    //mio screen loop Options screen (Alt+O)
    FuncReplace32(0x455E75, 0x048EB7, (DWORD)&Mio_Screen_Loop);
    //mio screen loop Replay screen
    FuncReplace32(0x450562, 0x04E7CA, (DWORD)&Mio_Screen_Loop);
    //mio screen loop Options screen GUI
    FuncReplace32(0x45BC6B, 0x0430C1, (DWORD)&Mio_Screen_Loop);
    //mio screen loop Mission Options screen
    FuncReplace32(0x453F5F, 0x04ADCD, (DWORD)&Mio_Screen_Loop);

    //check for windowed mode toggle key combo(Alt+Enter) and controller setup key combo(Alt+J) in keyboard procedure.
    MemWrite16(0x4891C9, 0xD08B, 0x9090);
    MemWrite8(0x4891CB, 0xF7, 0xE8);
    FuncWrite32(0x4891CC, 0x18FAC1D1, (DWORD)&check_sys_key);
}

#else
//__________________________
void Modifications_Display() {

    MemWrite16(0x4873F0, 0xA153, 0xE990);
    FuncWrite32(0x4873F2, 0x4D9818, (DWORD)&Display_Exit);

    MemWrite8(0x410B80, 0x8B, 0xE9);
    FuncWrite32(0x410B81, 0x530C244C, (DWORD)&Palette_Update_Main);

    MemWrite16(0x40FCE0, 0xEC83, 0x9090);
    MemWrite8(0x40FCE2, 0x6C, 0x90);
    MemWrite8(0x40FCE3, 0xA1, 0xE9);
    FuncWrite32(0x40FCE4, 0x4D40AC, (DWORD)&LockSurface);

    MemWrite16(0x40FDF0, 0xA153, 0xE990);
    FuncWrite32(0x40FDF2, 0x4D40AC, (DWORD)&UnlockShowSurface);

    //disable un-needed colour_fill function.
    MemWrite16(0x487C90, 0x4C8B, 0xC033);//xor eax, eax
    MemWrite16(0x487C92, 0x0824, 0xC390);

    //replace 8bit clear_screen function.
    MemWrite16(0x410D50, 0xA157, 0xE990);
    FuncWrite32(0x410D52, 0x4D40AC, (DWORD)&Clear_GUI_Surface);
    //replace 16bit clear_screen function.
    MemWrite16(0x410E00, 0xA153, 0xE990);
    FuncWrite32(0x410E02, 0x4D40AC, (DWORD)&Clear_GUI_Surface);

    MemWrite16(0x4825F0, 0xEC83, 0x9090);
    MemWrite8(0x4825F2, 0x6C, 0x90);
    MemWrite8(0x4825F3, 0xA1, 0xE9);
    FuncWrite32(0x4825F4, 0x4D9824, (DWORD)&LockSurface);

    MemWrite16(0x482640, 0xEC81, 0xE990);
    FuncWrite32(0x482642, 0x84, (DWORD)&UnlockShowSurface);

    //prevent direct draw - create surface func call
    MemWrite16(0x487CE0, 0xEC83, 0xC033);//xor eax, eax
    MemWrite8(0x487CE2, 0x6C, 0xC3);

    //replace direct draw - blit func
    MemWrite16(0x410F40, 0xA153, 0xE990);
    FuncWrite32(0x410F42, 0x4D40AC, (DWORD)&DXBlt);

    //replace direct draw - blit 16bit func
    MemWrite8(0x4110E0, 0x83, 0xE9);
    FuncWrite32(0x4110E1, 0xA0A108EC, (DWORD)&DXBlt);
    MemWrite16(0x4110E5, 0x4DC3, 0x9090);
    MemWrite8(0x4110E7, 0x00, 0x90);

    //--Prevent 16 and 24 bit drawing routines from taking effect as this patch handles such things-----------------
    //prevent direct draw - palette update func call
    MemWrite8(0x487C60, 0xA1, 0xC3);

    //Prevent cmd line arg true colour mode from being enabled.
    MemWrite32(0x4013E5, 0x01, 0x00);
    //Force current video colour bit level to stay at 1(8 bit colour) in set game flags function.
    MemWrite32(0x40F296, 0x02, 0x01);

    //prevent movie and gameflow bit level being set to 16bit in settings menu.
    //movie_video_colour_bit_level
    //MemWrite32(0x46E82B, 0x02, 0x01);
    //gameflow_video_colour_bit_level
    //MemWrite32(0x46E8D3, 0x02, 0x01);
    //--------------------------------------------------------------------------------------------------------------
    
    //replace space first person view setup function
    MemWrite8(0x4165D0, 0x8B, 0xE9);
    FuncWrite32(0x4165D1, 0x53042454, (DWORD)&set_space_view_pov1);

    //replace direct draw lock surface in draw space first person view function
    MemWrite16(0x4167A0, 0x6D74, 0x9090);//prevent jumping before this is called

    MemWrite8(0x4167A2, 0xA1, 0xE8);
    FuncWrite32(0x4167A3, 0x4D9820, (DWORD)&lock_3dspace_surface_pov1);
    MemWrite16(0x4167A7, 0xC085, 0x9090);
    MemWrite8(0x4167A9, 0x74, 0xEB);//jmp over ddraw stuff

    //replace direct draw stuff in draw space first person view function - unlock 3d space surface then lock 2d surface for hud etc.
    MemWrite16(0x4168A4, 0x840F, 0x9090);//prevent jumping before this is called
    MemWrite32(0x4168A6, 0x0113, 0x90909090);

    MemWrite8(0x4168AA, 0xA1, 0xE8);
    FuncWrite32(0x4168AB, 0x4D9820, (DWORD)&unlock_3dspace_surface_lock_2dspace_surface);
    MemWrite16(0x4168AF, 0xC085, 0x9090);
    MemWrite16(0x4168B1, 0x840F, 0xE990);//jmp over ddraw stuff

    //replace direct draw stuff in draw space first person view function - unlock 2d surface then display.
    FuncReplace32(0x416A1A, 0xFFFF93D2, (DWORD)&unlock_2dspace_surface_and_display);

    //draw targeting elements to 3d space
    FuncReplace32(0x420981, 0x540B, (DWORD)&draw_hud_targeting_elements);

    //draw tractor beam targeting circle to 3D space
    FuncReplace32(0x420B58, 0x06A9C4, (DWORD)&Draw_HUD_Tractor_Beam_Targeting_Circle);

    //fix the max size of targeting rect to match the ratio between it and the original screen size.
    MemWrite8(0x427018, 0xBA, 0xE8);
    FuncWrite32(0x427019, 0x1C, (DWORD)&fix_hud_targeting_rect_max_size);

    //replace direct draw lock surface in draw space third person view function
    MemWrite16(0x4140CC, 0x6374, 0x9090);//prevent jumping before this is called

    MemWrite8(0x4140CE, 0xA1, 0xE8);
    FuncWrite32(0x4140CF, 0x4D9820, (DWORD)&lock_3dspace_surface_pov3);
    MemWrite16(0x4140D3, 0xC085, 0x9090);
    MemWrite8(0x4140D5, 0x74, 0xEB);//jmp over ddraw stuff
 
    //replace direct draw stuff in draw space third person view function - unlock 3d space surface then lock 2d surface for text etc.
    FuncReplace32(0x4141C4, 0xFFFFF958, (DWORD)&unlock_3dspace_surface_lock_2dspace_surface_pov3);

    //replace direct draw stuff in draw space third person view function - unlock 2d surface then display.
    FuncReplace32(0x4141ED, 0xFFFFBBFF, (DWORD)&unlock_2dspace_surface_and_display);

    //replace space third person view setup function
    MemWrite8(0x4A1000, 0x56, 0xE9);
    FuncWrite32(0x4A1001, 0x0824748B, (DWORD)&set_space_view_pov3);

    //fix display rectangle for targeting elements
    MemWrite16(0x425E28, 0x0D8B, 0xE890);
    FuncWrite32(0x425E2A, 0x4D40A4, (DWORD)&fix_cockpit_view_target_rect);

    //draw nav screen space view to 3d surface, seperate from 2d elements
    FuncReplace32(0x41A9D2, 0x2A, (DWORD)&set_input_profile_nav_map_3d_draw);

    //set 2d surface for drawing nav screen 2d elements
    MemWrite8(0x41AC9E, 0x03, 0xE8);
    FuncWrite32(0x41AC9F, 0xCD034FCF, (DWORD)&nav_unlock_3d_and_lock_2d_drawing);
  
    //set 3d surface after drawing nav screen 2d elements
    MemWrite16(0x41AEE1, 0x15FF, 0xE890);
    FuncWrite32(0x41AEE3, 0x4D4114, (DWORD)&nav_unlock_2d_and_display_relock_3d);


    //Set space subtitle text background colour to 0. As original 255 coflicts with the mask colour being used to draw all cockpit/hud elements to a seperate surface.
    MemWrite8(0x413CB8, 0xFF, 0x00);

    //disables setup window function.
    MemWrite8(0x4113F0, 0x56, 0xC3);
    //jmp over call to initialize window/dx in message box error function.
    MemWrite8(0x4983F1, 0x75, 0xEB);

    //start display setup passing "-no_full_screen" cmd line value.
    MemWrite8(0x410B4B, 0x74, 0xEB);

    MemWrite16(0x410B65, 0x006A, 0x9090);
    MemWrite16(0x410B67, 0x15FF, 0xE890);
    FuncWrite32(0x410B69, 0x4DE464, (DWORD)&start_display_setup);

    //replace the main window message checking function for greater functionality.
    MemWrite8(0x40FFA0, 0x8B, 0xE9);
    FuncWrite32(0x40FFA1, 0x53082444, (DWORD)&WinProc_Main);

    //Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
    //increase range of message code selection
    MemWrite32(0x482832, 0xF5, 0x121);

    //check for the messages
    MemWrite16(0x48283E, 0x888A, 0xE890);
    FuncWrite32(0x482840, 0x4829E0, (DWORD)&winproc_movie_message_check);

    //clip the cursor while a conversation decision is made.
    FuncReplace32(0x499304, 0xFFFFFF08, (DWORD)&Conversation_Decision_ClipCursor);

    //Enable cursor clipping for all WM_SETCURSOR messages within calls to this function, used during space flight.
    FuncReplace32(0x40F1D2, 0x0000411A, (DWORD)&overide_cursor_clipping);

    //004213D5 | .E8 3A030500   CALL DRAW_IMAGE(*dib_struct, ) ? //draw crosshairs
    //MemWrite8(0x4213D5, 0xE8, 0x90);
    //MemWrite32(0x4213D6, 0x05033A, 0x90909090);

    //fix control reaction speed on the nav screen.
    FuncReplace32(0x41AB7D, 0x17BF, (DWORD)&nav_screen_movement_speed_fix);


    //----fix-highlight-when-target-in-cross-hairs----
    //remove setting of highlight variables here.
    //highlight variables are set in "Set_Space_View_POV1" function.
    MemWrite16(0x420930, 0x8E8B, 0x9090);
    MemWrite32(0x420932, 0x9C, 0x90909090);
    MemWrite16(0x420936, 0x0D89, 0x9090);
    MemWrite32(0x420938, 0x4C0FC8, 0x90909090);
    MemWrite16(0x42093C, 0x968B, 0x9090);
    MemWrite32(0x42093E, 0xA0, 0x90909090);
    MemWrite16(0x420942, 0x1589, 0x9090);
    MemWrite32(0x420944, 0x4C10B0, 0x90909090);
    MemWrite16(0x420948, 0x868B, 0x9090);
    MemWrite32(0x42094A, 0xA4, 0x90909090);
    MemWrite8(0x42094E, 0xA3, 0x90);
    MemWrite32(0x42094F, 0x4C10B4, 0x90909090);
    //------------------------------------------------

    //set input profile to space when flying a mission.
    FuncReplace32(0x401FDC, 0x072A55, (DWORD)&set_input_profile_space_mission);

    //set input profile to gui when starting exit game screen.
    MemWrite8(0x40AA73, 0xA1, 0xE8);
    FuncWrite32(0x40AA74, 0x4BE8D8, (DWORD)&set_input_profile_space_exit_game_start);
    //set input profile back to space when ending exit game screen.
    MemWrite8(0x40AB32, 0xA3, 0xE8);
    FuncWrite32(0x40AB33, 0x4BE8D8, (DWORD)&set_input_profile_space_exit_game_end);

    //set input profile to gui when starting pause game screen.
    MemWrite8(0x40AB43, 0xA1, 0xE8);
    FuncWrite32(0x40AB44, 0x4BE898, (DWORD)&set_input_profile_space_pause_game_start);
    //set input profile back to space when ending pause game screen.
    MemWrite8(0x40ABB6, 0xA3, 0xE8);
    FuncWrite32(0x40ABB7, 0x4BE898, (DWORD)&set_input_profile_space_pause_game_end);

    //set input profile to gui when using options screen(Alt+O)
    FuncWrite32(0x414698, 0x027BE4, (DWORD)&options_screen_set_input_profile);

    //mio screen loop Options screen (Alt+O)
    FuncReplace32(0x43C522, 0x06E261, (DWORD)&Mio_Screen_Loop);
    //mio screen loop Replay screen
    FuncReplace32(0x43D3CB, 0x06D3B8, (DWORD)&Mio_Screen_Loop);
    //mio screen loop Options screen GUI
    FuncReplace32(0x46EE98, 0x03B8EB, (DWORD)&Mio_Screen_Loop);
    //mio screen loop Mission Options screen
    FuncReplace32(0x473F77, 0x03680C, (DWORD)&Mio_Screen_Loop);
    //mio screen loop install screen
    FuncReplace32(0x4822F0, 0x028493, (DWORD)&Mio_Screen_Loop);

    //check for windowed mode toggle key combo(Alt+Enter) and controller setup key combo(Alt+J) in keyboard procedure.
    MemWrite8(0x4ADF22, 0xF7, 0xE8);
    FuncWrite32(0x4ADF23, 0x1FE8C1D0, (DWORD)&check_sys_key);
}
#endif


