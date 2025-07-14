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

#include "Display_DX11.h"
#include "modifications.h"
#include "memwrite.h"
#include "configTools.h"
#include "movies_vlclib.h"
#include "wc4w.h"
#include "joystick_config.h"

#define WIN_MODE_STYLE  WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX




BOOL is_cockpit_view = FALSE;
BOOL is_POV3_view = FALSE;

SCALE_TYPE cockpit_scale_type = SCALE_TYPE::fit;
BOOL crop_cockpit_rect = TRUE;
BOOL is_nav_view = FALSE;

BOOL clip_cursor = FALSE;
static bool is_cursor_clipped = false;

LibVlc_Movie* pMovie_vlc = nullptr;


UINT clientWidth = 0;
UINT clientHeight = 0;

WORD mouse_state_true[3];
WORD* p_mouse_button_true = &mouse_state_true[0];
WORD* p_mouse_x_true = &mouse_state_true[1];
WORD* p_mouse_y_true = &mouse_state_true[2];

LONG mouse_x_current = 0;
LONG mouse_y_current = 0;

LARGE_INTEGER nav_update_time{ 0 };


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

    int COCKPIT_MAINTAIN_ASPECT_RATIO = ConfigReadInt(L"SPACE", L"COCKPIT_MAINTAIN_ASPECT_RATIO", CONFIG_SPACE_COCKPIT_MAINTAIN_ASPECT_RATIO);
    if (COCKPIT_MAINTAIN_ASPECT_RATIO == 0)
        cockpit_scale_type = SCALE_TYPE::fill;
    else if (COCKPIT_MAINTAIN_ASPECT_RATIO < 0)
        crop_cockpit_rect = FALSE;

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

    *p_wc4_mouse_centre_x = (LONG)clientWidth / 2;
    *p_wc4_mouse_centre_y = (LONG)clientHeight / 2;

    Display_Dx_Setup(hwnd, clientWidth, clientHeight);

    //QueryPerformanceFrequency(&Frequency);

    //Set the movement update time for Navigation screen, which was unregulated and way to fast on modern computers.
    DXGI_RATIONAL refreshRate{};
    refreshRate.Denominator = 1;
    refreshRate.Numerator = ConfigReadInt(L"SPACE", L"NAV_SCREEN_KEY_RESPONCE_HZ", CONFIG_SPACE_NAV_SCREEN_KEY_RESPONSE_HZ);
    nav_update_time.QuadPart = p_wc4_frequency->QuadPart / refreshRate.Numerator;

    //Set the max refresh rate in space, original 24 FPS. Set to zero to use screen refresh rate, a negative value will use the original value.  
    refreshRate.Numerator = ConfigReadInt(L"SPACE", L"SPACE_REFRESH_RATE_HZ", CONFIG_SPACE_SPACE_REFRESH_RATE_HZ);
    if ((int)refreshRate.Numerator < 0)
        refreshRate.Numerator = 24;
    else if (refreshRate.Numerator == 0)
        Get_Monitor_Refresh_Rate(hwnd, &refreshRate);

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

    *p_wc4_mouse_centre_x = (LONG)clientWidth / 2;
    *p_wc4_mouse_centre_y = (LONG)clientHeight / 2;

    Display_Dx_Resize(clientWidth, clientHeight);


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


//__________________________________
static void UnlockShowMovieSurface() {

    if (surface_gui == nullptr)
        return;
    surface_gui->Unlock();
    Display_Dx_Present(PRESENT_TYPE::movie);
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


//_______________________________________________________________
static void DXBlt_Movie(BYTE* fBuff, DWORD subY, DWORD subHeight) {

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

    Display_Dx_Present(PRESENT_TYPE::movie);

}


//_____________________________________________________________________________________
static BOOL DrawVideoFrame(VIDframe* vidFrame, RGBQUAD* tBuff, UINT tWidth, DWORD flag) {
    
    DWORD height = vidFrame->height;
    DWORD width = vidFrame->width;
    SCALE_TYPE scale_type = SCALE_TYPE::fit;
    DWORD xan_flags = 4;

    if (!*p_wc4_movie_no_interlace) {
        height += height;
        width += width;
        scale_type = SCALE_TYPE::fit_best;
        xan_flags |= 2;
    }

    if (!surface_movieXAN || width != surface_movieXAN->GetWidth() || height != surface_movieXAN->GetHeight()) {
        if (surface_movieXAN)
            delete surface_movieXAN;
        surface_movieXAN = new DrawSurface8_RT(0, 0, width, height, 32, 0x00000000, false, 0);
        surface_movieXAN->ScaleTo((float)clientWidth, (float)clientHeight, scale_type);
        if (!*p_wc4_movie_no_interlace)
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


// Adjust width, height and centre target point of FIRST person POV space views.
// __fastcall used here as class "this" is parsed on the ecx register.
//_____________________________________________________________
static void __fastcall Set_Space_View_POV1(void* p_space_class) {

    WORD* p_view_vars = (WORD*)p_space_class;

    is_cockpit_view = FALSE;
    is_POV3_view = FALSE;
    SCALE_TYPE scale_type = SCALE_TYPE::fit;


    WORD width = (WORD)clientWidth;
    WORD height = (WORD)clientHeight;

    p_view_vars[4] = width;
    p_view_vars[5] = height;

    //DWORD* p_cockpit_class1 = ((DWORD**)p_space_class)[44];
    DWORD* p_cockpit_class2 = ((DWORD**)p_space_class)[69];

    LONG xpos = p_cockpit_class2[40];
    LONG ypos = p_cockpit_class2[41];


    p_view_vars[6] = (WORD)(xpos / (float)GUI_WIDTH * width);
    p_view_vars[7] = (WORD)(ypos / (float)GUI_HEIGHT * height);

    Debug_Info_Flight("Set_Space_View_POV1 centre_x=%d, centre_y=%d, new_centre_x=%d, new_centre_y=%d", xpos, ypos, p_view_vars[6], p_view_vars[7]);

    surface_space2D->ScaleTo((float)clientWidth, (float)clientHeight, scale_type);
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
    is_cockpit_view = FALSE;

    WORD width = (WORD)clientWidth;
    WORD height = (WORD)clientHeight;

    //rear view vdu screen is 122*100, check for it here to prevent it being resized.
    if (p_db && p_db->rc.right == 121 && p_db->rc.bottom == 99) {
        width = 122;
        height = 100;
        //if in cockpit view, make sure flag is set to enable clipping rect.
        if (*p_wc4_space_view_type == SPACE_VIEW_TYPE::Cockpit)
            is_cockpit_view = TRUE;
    }
    else
        surface_space2D->ScaleTo((float)clientWidth, (float)clientHeight, SCALE_TYPE::fit);// dont alter the scale type when drawing rear view vdu.

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

    (**pp_wc4_db_game_main).rc.right = clientWidth - 1;
    (**pp_wc4_db_game_main).rc.bottom = clientHeight - 1;

    (**pp_wc4_db_game).rc_inv.left = buffer_space_3D_pitch - 1;
    (**pp_wc4_db_game).rc_inv.top = clientHeight - 1;

}


//_________________________________________________________
static void Lock_3DSpace_Surface_POV1(void* p_space_class) {
    is_POV3_view = FALSE;
    //Debug_Info("Lock_3DSpace_Surface_POV1 SPACE VIEW POV1 w:%d, h:%d client  w:%d, h:%d", ((WORD*)p_space_class)[4], ((WORD*)p_space_class)[5], clientWidth, clientHeight);
    if (((WORD*)p_space_class)[4] != (WORD)clientWidth || ((WORD*)p_space_class)[5] != (WORD)clientHeight) {
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
    is_POV3_view = TRUE;
    if (((WORD*)p_space_struct)[4] != (WORD)clientWidth || ((WORD*)p_space_struct)[5] != (WORD)clientHeight) {
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


//______________________________________________________
static void __declspec(naked) fix_nav_scrn_display(void) {

    __asm {

        push ebp
        push edi

        push ecx

        call Lock_3DSpace_Surface

        pop ecx
        call wc4_nav_screen

        call UnLock_3DSpace_Surface

        pop edi
        pop ebp
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
}


//return FALSE to call DefWindowProc
//_____________________________________________________________________________
static BOOL WinProc_Main(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    static bool is_cursor_hidden = true;
    static bool is_in_sizemove = false;

    switch (Message) {
    case WM_KEYDOWN:
        if (!(lParam & 0x40000000)) { //The previous key state. The value is 1 if the key is down before the message is sent, or it is zero if the key is up.
            if (wParam == VK_F11) { //Use F11 key to toggle windowed mode.
                if (pMovie_vlc)
                    pMovie_vlc->Pause(true);
                if (pMovie_vlc_Inflight)
                    pMovie_vlc_Inflight->Pause(true);
                Toggle_WindowMode(hwnd);
            }
        }
        break;
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
        if (hWin_JoyConfig)
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
        cmp ecx, WM_LBUTTONDBLCLK
            jne check4
            mov dl, 2
            ret
            check4 :
        cmp ecx, WM_RBUTTONDBLCLK
            jne check5
            mov dl, 3
            ret
            check5 :
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
        cmp eax, WM_LBUTTONDBLCLK
            jne check4
            mov cl, 2
            ret
            check4 :
        cmp eax, WM_RBUTTONDBLCLK
            jne check5
            mov cl, 3
            ret
            check5 :
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
        Modifications_Joystick();

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


//______________________________________________________________________________________
static LRESULT Update_Mouse_State(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    switch (Message) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP: {
        *p_wc4_mouse_button = 0;
        if (wParam & MK_LBUTTON)
            *p_wc4_mouse_button |= 0x1;
        if (wParam & MK_RBUTTON)
            *p_wc4_mouse_button |= 0x2;
        if (wParam & MK_MBUTTON)
            *p_wc4_mouse_button |= 0x4;
        *p_mouse_button_true = *p_wc4_mouse_button;

        LONG x = GET_X_LPARAM(lParam);
        LONG y = GET_Y_LPARAM(lParam);

        *p_mouse_x_true = (WORD)x;
        *p_mouse_y_true = (WORD)y;
        if (surface_gui) {
            float fx = 0;
            float fy = 0;
            surface_gui->GetPosition(&fx, &fy);
            x = (LONG)((x - fx) * GUI_WIDTH / surface_gui->GetScaledWidth());
            y = (LONG)((y - fy) * GUI_HEIGHT / surface_gui->GetScaledHeight());
        }
        else {
            x = x * GUI_WIDTH / clientWidth;
            y = y * GUI_HEIGHT / clientHeight;
        }

        if (x < 0)
            x = 0;
        else if (x >= GUI_WIDTH)
            x = GUI_WIDTH - 1;
        if (y < 0)
            y = 0;
        else if (y >= GUI_HEIGHT)
            y = GUI_HEIGHT - 1;

        mouse_x_current = x;
        mouse_y_current = y;
        *p_wc4_mouse_x = (WORD)x;
        *p_wc4_mouse_y = (WORD)y;

        break;
    }
    default:
        break;
    }

    return 0;
}


//____________________________________________
static BOOL Set_Mouse_Position(LONG x, LONG y) {

    if (*p_wc4_is_mouse_present) {
        POINT client{ 0,0 };
        if (ClientToScreen(*p_wc4_hWinMain, &client)) {

            float fx = 0;
            float fy = 0;
            float fwidth = (float)clientWidth;
            float fheight = (float)clientHeight;
            if (surface_gui) {
                surface_gui->GetPosition(&fx, &fy);
                fwidth = surface_gui->GetScaledWidth();
                fheight = surface_gui->GetScaledHeight();
            }

            fx += x * fwidth / GUI_WIDTH;
            LONG ix = (LONG)fx;
            if ((float)ix != fx)
                ix++;
            *p_mouse_x_true = (WORD)ix;
            ix += client.x;

            fy += y * fheight / GUI_HEIGHT;
            LONG iy = (LONG)fy;
            if ((float)iy != fy)
                iy++;
            *p_mouse_y_true = (WORD)iy;
            iy += client.y;

            SetCursorPos(ix, iy);
        }

        if (x < 0)
            x = 0;
        else if (x >= GUI_WIDTH)
            x = GUI_WIDTH - 1;
        if (y < 0)
            y = 0;
        else if (y >= GUI_HEIGHT)
            y = GUI_HEIGHT - 1;

        mouse_x_current = x;
        mouse_y_current = y;
        *p_wc4_mouse_x = (WORD)x;
        *p_wc4_mouse_y = (WORD)y;
    }
    return *p_wc4_is_mouse_present;
}


// Replaces a function which was moving the cursor when it strayed beyond the client rect.
// This function allows the mouse to move freely as well as update it's position when outside the client rect.
//________________________________________________
static BOOL Update_Cursor_Position(LONG x, LONG y) {

    //if cursor position is modified by somthing other than the mouse 
    if (x != mouse_x_current || y != mouse_y_current)
        return Set_Mouse_Position(x, y);

    if (*p_wc4_is_mouse_present) {
        POINT p{ 0,0 };
        if (ClientToScreen(*p_wc4_hWinMain, &p)) {
            POINT m{ 0,0 };
            GetCursorPos(&m);

            x = (m.x - p.x);
            y = (m.y - p.y);

            *p_mouse_x_true = (WORD)x;
            *p_mouse_y_true = (WORD)y;

            if (surface_gui) {
                float fx = 0;
                float fy = 0;
                surface_gui->GetPosition(&fx, &fy);
                x = (LONG)((x - fx) * GUI_WIDTH / surface_gui->GetScaledWidth());
                y = (LONG)((y - fy) * GUI_HEIGHT / surface_gui->GetScaledHeight());
            }
            else {
                x = x * GUI_WIDTH / clientWidth;
                y = y * GUI_HEIGHT / clientHeight;
            }
        }

        if (x < 0)
            x = 0;
        else if (x >= GUI_WIDTH)
            x = GUI_WIDTH - 1;
        if (y < 0)
            y = 0;
        else if (y >= GUI_HEIGHT)
            y = GUI_HEIGHT - 1;

        mouse_x_current = x;
        mouse_y_current = y;
        *p_wc4_mouse_x = (WORD)x;
        *p_wc4_mouse_y = (WORD)y;
    }
    return *p_wc4_is_mouse_present;
}


//____________________________________________
static void Conversation_Decision_ClipCursor() {
    clip_cursor = TRUE;
    wc4_conversation_decision_loop();
    clip_cursor = FALSE;
}


//______________________________________________________
static WORD* Translate_Messages_Mouse_ClipCursor_Space() {
    clip_cursor = TRUE;
    wc4_translate_messages(TRUE, FALSE);
    clip_cursor = FALSE;
    return p_mouse_button_true;
}


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


//____________________________________________________
static void __declspec(naked) update_space_mouse(void) {

    __asm {
        mov eax, p_mouse_button_true
        mov edx, p_wc4_mouse_button_space
        mov ecx, dword ptr ds: [eax]
        mov dword ptr ds: [edx], ecx
        mov cx, word ptr ds: [eax+4]
        mov word ptr ds : [edx+4] , cx

#ifndef VERSION_WC4_DVD
        add esp, 0x8
#endif
        ret
    }
}


//_______________________________________________________________________________
static void Palette_Update_Main(BYTE* p_pal_buff, BYTE offset, DWORD num_entries) {
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
            wc4_movie_update_joystick_double_click_exit();
            exit_flag = wc4_movie_exit();

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

    Debug_Info_Movie("Play_HD_Movie_Sequence: Done:%d", play_successfull);
    return play_successfull;
}


//_____________________________________________________
static BOOL Play_HD_Movie_Sequence_Main(char* mve_path) {

    DXGI_RATIONAL refreshRate{};
    refreshRate.Denominator = 1;
    refreshRate.Numerator = 3;
    p_wc4_movie_click_time->QuadPart = p_wc4_frequency->QuadPart * refreshRate.Denominator / refreshRate.Numerator;

    wc4_message_check_node_add(wc4_movie_messages);

    if(surface_gui)
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
        wc4_movie_update_joystick_double_click_exit();
        exit_flag = wc4_movie_exit();

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

    Debug_Info_Movie("Play_HD_Movie_Sequence_Main end");
    return play_successfull;
}


//________________________________________________________
static void __declspec(naked) play_hd_movie_sequence_main(void) {

    __asm {
        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp

        push [esp + 0x20]//path
        call Play_HD_Movie_Sequence_Main
        add esp, 0x4

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

        hd_movie_error:

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
    BOOL(__thiscall*p_wc4_xanlib_set_pos)(void*, LONG, LONG*) = (BOOL(__thiscall*)(void*, LONG, LONG*)) * ((void**)xanlib_this+0x30);

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
            wc4_movie_update_joystick_double_click_exit();
            exit_flag = wc4_movie_exit();

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
    Debug_Info_Movie("Play_HD_Movie_Sequence_Secondary: Done");
    return play_successfull;
}


//__________________________________________________________________
static void __declspec(naked) play_hd_movie_sequence_secondary(void) {

    __asm {
        push ebx
        push ebp

        push [esp+0xC]//sig movie flags? 0x7FFFFFFF
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
        mov eax, dword ptr ds:[eax]
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
        mov eax, dword ptr ds:[eax]
        ret
    }
}


//______________________________________
static LONG Inflight_Movie_Audio_Check() {
    //check if the hd movie has audio, and if so lower the volume of the background music while playing.
    if (*p_wc4_inflight_audio_ref == 0 && pMovie_vlc_Inflight && pMovie_vlc_Inflight->HasAudio()) {
        Debug_Info_Movie("Inflight_Movie_Check_Audio HasAudio - volume lowered");
        LONG audio_vol = *p_wc4_ambient_music_volume - 4;
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


//__________________________________________________________________________________________
static void Fix_Space_Mouse_Movement(void* p_ship_class, LONG* p_x_roll, LONG* p_y_throttle) {
    // Maximum turn speed was being defined by the screen resolution formally 640x480.
    // Higher resolutions were allowing for a greater mouse range and thus a higher turning speed than what was otherwise defined in game.

    LONG x = *p_wc4_mouse_x_space - *p_wc4_mouse_centre_x;
    LONG y = *p_wc4_mouse_y_space - *p_wc4_mouse_centre_y;

    //fix mouse movement range between -16 and 16.
    LONG range = *p_wc4_mouse_centre_y;
    if (*p_wc4_mouse_centre_y > *p_wc4_mouse_centre_x)
        range = *p_wc4_mouse_centre_x;
    //keep mouse movement range less than 640/2 to maintain similar experience as original resolution 640x480.
    if (range > 320)
        range = 320;

    LONG range_block = range / 16;

    x /= range_block;
    y /= range_block;

    if (x > 16)
        x = 16;
    else if (x < -16)
        x = -16;

    if (y > 16)
        y = 16;
    else if (y < -16)
        y = -16;
    
#ifdef VERSION_WC4_DVD
    *p_x_roll = x;
#endif   

    *p_y_throttle = y;

    //differs from wc3 here, multiply values by 16
    x *= 16;
    y *= 16;
#ifndef VERSION_WC4_DVD
    *p_x_roll = x;
#endif  

    LONG* p_position_class = ((LONG**)p_ship_class)[61];
    p_position_class[4] = -x;
    p_position_class[3] = -y;
}


//__________________________________________________________
static void __declspec(naked) fix_space_mouse_movement(void) {

    __asm {
        push esi
        
        sub esp, 0x8 //make room on the stack for roll and throttle value pointers.

        //set pointers on the stack for returning mouse roll and throttle values.
        lea ecx, dword ptr ss : [esp] //*p_x_roll
        lea eax, dword ptr ss : [esp + 0x4]//*p_y_throttle
        push eax
        push ecx
        push esi
        call Fix_Space_Mouse_Movement
        add esp, 0xC

#ifdef VERSION_WC4_DVD
        pop ebp //x for mouse roll
        pop edx //y for mouse throttle
        mov ecx, edx //y for mouse throttle when negative
        neg ecx
        xor ebx, ebx
#else
        pop ecx //x for mouse roll
        pop eax //y for mouse throttle
#endif
        pop esi
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


    //in draw movie func, jump over direct draw stuff
    MemWrite8(0x47A4DC, 0x74, 0xEB);

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


    //draw movie frame main
    MemWrite32(0x47A6FE, 0x4D45E4, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x47A7CD, 0x4D45E4, (DWORD)&p_DrawVideoFrame);
    // draw movie frame
    MemWrite32(0x47B63B, 0x4D45E4, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x47B752, 0x4D45E4, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x47B7FE, 0x4D45E4, (DWORD)&p_DrawVideoFrame);

    //in draw movie func
    FuncReplace32(0x47A552, 0xFFFFBF9A, (DWORD)&UnlockShowMovieSurface);
    //in draw movie frame func
    FuncReplace32(0x47B75A, 0xFFFFAD92, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x47B806, 0xFFFFACE6, (DWORD)&UnlockShowMovieSurface);

    //for drawing subtitles - dont know much about these
    FuncReplace32(0x47C573, 0xFFFF9F79, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x47C99F, 0xFFFF9B4D, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x47CAAA, 0xFFFF9A42, (DWORD)&UnlockShowMovieSurface);

    //replace blit function for movies
    FuncReplace32(0x4591BB, 0x01E5B1, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x4591D3, 0x01E599, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x45950D, 0x01E25F, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x459525, 0x01E247, (DWORD)&DXBlt_Movie);

    //replace blit function for draw choice text
    FuncReplace32(0x459A62, 0x01DD0A, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x459A79, 0x01DCF3, (DWORD)&DXBlt_Movie);


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
    FuncReplace32(0x44530F, 0x2D, (DWORD)&fix_nav_scrn_display);

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


    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x489670, 0x8B, 0xE9);
    FuncWrite32(0x489671, 0x05082444, (DWORD)&Update_Mouse_State);
    MemWrite32(0x489675, 0xFFFFFE00, 0x90909090);

    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x4895A0, 0xA1, 0xE9);
    FuncWrite32(0x4895A1, 0x4CD6AC, (DWORD)&Set_Mouse_Position);


    //replace the main window message checking function for greater functionality.
    MemWrite8(0x476B60, 0x8B, 0xE9);
    FuncWrite32(0x476B61, 0x56082444, (DWORD)&WinProc_Main);

    //Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
    //increase range of message code selection
    MemWrite32(0x47B3DB, 0xF5, 0x121);

    //check for the messages
    MemWrite16(0x47B3E7, 0x918A, 0xE890);
    FuncWrite32(0x47B3E9, 0x47B500, (DWORD)&winproc_movie_message_check);

    //prevent setting mouse centre x and y values to 320x240.
    MemWrite16(0x46F59C, 0x05C7, 0x9090);
    MemWrite32(0x46F59E, 0x4C4184, 0x90909090);
    MemWrite32(0x46F5A2, 0x0140, 0x90909090);
    MemWrite16(0x46F5A6, 0x05C7, 0x9090);
    MemWrite32(0x46F5A8, 0x4C4194, 0x90909090);
    MemWrite32(0x46F5AC, 0xF0, 0x90909090);

    //replace set cursor function to allow cursor to freely leave window bounds when in gui mode
    FuncReplace32(0x42B5A0, 0x0736CC, (DWORD)&Update_Cursor_Position);


    //clip the cursor while a conversation decision is made.
    FuncReplace32(0x49DD33, 0xFFFFFF19, (DWORD)&Conversation_Decision_ClipCursor);


    //Enable cursor clipping for all WM_SETCURSOR messages within calls to this function, used during space flight.
    FuncReplace32(0x46A991, 0x0004B6B, (DWORD)&overide_cursor_clipping);


    //update the cursor position on "ALT+O" space settings screen.
    FuncReplace32(0x49F068, 0xFFFEA534, (DWORD)&Update_Cursor_Position);


    //relate the movement diamond to new space view dimensions rather than original 640x480.
    MemWrite32(0x40AA9E, 0x4C41AC, 0x4CD6B4);
    MemWrite32(0x40AACE, 0x4C41AA, 0x4CD6B2);

    //update mouse in space view
    MemWrite8(0x46F4E0, 0xE8, 0xE9);
    FuncReplace32(0x46F4E1, 0x01A16B, (DWORD)&update_space_mouse);


    //fix control reaction speed on the nav screen.
    FuncReplace32(0x445478, 0xFFFFF484, (DWORD)&nav_screen_movement_speed_fix);

    // Mouse turn speed fix--------------------------
    MemWrite16(0x44BE82, 0x2D8B, 0xE890);
    FuncWrite32(0x44BE84, 0x4C4184, (DWORD)&fix_space_mouse_movement);
   
    //jump over the remaining part of this section
    MemWrite16(0x44BE88, 0xC933, 0x9090);
    MemWrite8(0x44BE8A, 0x66, 0xE9);
    MemWrite32(0x44BE8B, 0x41AA0D8B, 0x80);
    MemWrite16(0x44BE8F, 0x004C, 0x9090);
    //-----------------------------------------------

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

    MemWrite8(0x40CE39, 0xA1, 0xE8);
    FuncWrite32(0x40CE3A, 0x4BB9C8, (DWORD)&play_inflight_movie);

    MemWrite8(0x40D1C0, 0xA1, 0xE8);
    FuncWrite32(0x40D1C1, 0x4BB910, (DWORD)&inflight_movie_unload);

    MemWrite16(0x44F09B, 0x3539, 0xE890);
    FuncWrite32(0x44F09D, 0x4B47DC, (DWORD)&inflight_movie_audio_check);
    //---------------------------------------------------------
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


    //in draw movie func, jump over direct draw stuff
    MemWrite8(0x482EE3, 0x74, 0xEB);

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
    
    //draw movie frame main
    MemWrite32(0x482D43, 0x4DE550, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x482E2C, 0x4DE550, (DWORD)&p_DrawVideoFrame);
    // draw movie frame
    MemWrite32(0x483115, 0x4DE550, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x483139, 0x4DE550, (DWORD)&p_DrawVideoFrame);
    MemWrite32(0x48322B, 0x4DE550, (DWORD)&p_DrawVideoFrame);

    //in draw movie func
    FuncReplace32(0x482F5A, 0xFFF8CE92, (DWORD)&UnlockShowMovieSurface);

    //in draw movie frame func
    FuncReplace32(0x483244, 0xFFF8CBA8, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x4832F9, 0xFFF8CAF3, (DWORD)&UnlockShowMovieSurface);
    
    //for drawing subtitles - dont know much about these
    FuncReplace32(0x484947, 0xFFF8B4A5, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x484DDE, 0xFFF8B00E, (DWORD)&UnlockShowMovieSurface);
    FuncReplace32(0x484EEA, 0xFFF8AF02, (DWORD)&UnlockShowMovieSurface);

    //replace blit function for movies
    FuncReplace32(0x46C5E0, 0xFFFA495C, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x46C5F8, 0xFFFA4944, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x46C916, 0xFFFA4626, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x46C92E, 0xFFFA460E, (DWORD)&DXBlt_Movie);

    //replace blit function for draw choice text
    FuncReplace32(0x46CC0C, 0xFFFA4330, (DWORD)&DXBlt_Movie);
    FuncReplace32(0x46CC24, 0xFFFA4318, (DWORD)&DXBlt_Movie);


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
    FuncReplace32(0x41A9D2, 0x2A, (DWORD)&fix_nav_scrn_display);

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


    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x4AE0D0, 0x8B, 0xE9);
    FuncWrite32(0x4AE0D1, 0x56082444, (DWORD)&Update_Mouse_State);

 
    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x4AE010, 0x83, 0xE9);
    FuncWrite32(0x4AE011, 0x5CA108EC, (DWORD)&Set_Mouse_Position);
    MemWrite8(0x4AE015, 0xC3, 0x90);
    MemWrite16(0x4AE016, 0x004D, 0x9090);


    //replace the main window message checking function for greater functionality.
    MemWrite8(0x40FFA0, 0x8B, 0xE9);
    FuncWrite32(0x40FFA1, 0x53082444, (DWORD)&WinProc_Main);

    //Add  WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE checks to movie message check
    //increase range of message code selection
    MemWrite32(0x482832, 0xF5, 0x121);

    //check for the messages
    MemWrite16(0x48283E, 0x888A, 0xE890);
    FuncWrite32(0x482840, 0x4829E0, (DWORD)&winproc_movie_message_check);

    //prevent setting mouse centre x and y values to 320x240.
    MemWrite16(0x413379, 0x05C7, 0x9090);
    MemWrite32(0x41337B, 0x4C0658, 0x90909090);
    MemWrite32(0x41337F, 0x0140, 0x90909090);
    MemWrite16(0x413383, 0x05C7, 0x9090);
    MemWrite32(0x413385, 0x4C0678, 0x90909090);
    MemWrite32(0x413389, 0xF0, 0x90909090);

    //replace set cursor function to allow cursor to freely leave window bounds when in gui mode
    FuncReplace32(0x466482, 0x0480AA, (DWORD)&Update_Cursor_Position);


    //clip the cursor while a conversation decision is made.
    FuncReplace32(0x499304, 0xFFFFFF08, (DWORD)&Conversation_Decision_ClipCursor);


    //Enable cursor clipping for all WM_SETCURSOR messages within calls to this function, used during space flight.
    FuncReplace32(0x40F1D2, 0x0000411A, (DWORD)&overide_cursor_clipping);


    //update the cursor position on "ALT+O" space settings screen.
    FuncReplace32(0x4AAC18, 0x33F4, (DWORD)&Update_Cursor_Position);


    //relate the movement diamond to new space view dimensions rather than original 640x480.
    MemWrite32(0x423326, 0x4C0674, 0x4DC364);
    MemWrite32(0x423300, 0x4C0672, 0x4DC362);

    //004213D5 | .E8 3A030500   CALL DRAW_IMAGE(*dib_struct, ) ? //draw crosshairs
    //MemWrite8(0x4213D5, 0xE8, 0x90);
    //MemWrite32(0x4213D6, 0x05033A, 0x90909090);

    //update mouse in space view
    MemWrite8(0x4132C3, 0xE8, 0xE9);
    FuncReplace32(0x4132C4, 0x09ADE8, (DWORD)&update_space_mouse);

    //fix control reaction speed on the nav screen.
    FuncReplace32(0x41AB7D, 0x17BF, (DWORD)&nav_screen_movement_speed_fix);

    // Mouse turn speed fix--------------------------
    MemWrite8(0x448FBB, 0xA1, 0xE8);
    FuncWrite32(0x448FBC, 0x4C0658, (DWORD)&fix_space_mouse_movement);

    //jump over the remaining part of this section
    MemWrite16(0x448FC0, 0x8B66, 0x6EEB);
    MemWrite32(0x448FC2, 0x4C06720D, 0x90909090);
    MemWrite8(0x448FC6, 0x00, 0x90);
    //-----------------------------------------------

    // HD Movies-----------------------------------------------
    //Replaces the Play_Movie function similar to the dvd version of WC4.
    MemWrite16(0x483AD0, 0xEC81, 0xE890);
    FuncWrite32(0x483AD2, 0x0154, (DWORD)&play_hd_movie_sequence_main);

    //backup if the above fails. Here the original movie sequence will play if a HD movie fails.
    MemWrite32(0x483F2D, 0x4DE4E0, (DWORD)&p_play_hd_movie_sequence_secondary);

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


