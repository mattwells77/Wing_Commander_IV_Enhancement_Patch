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

#include "Display_DX11.h"
#include "input.h"
#include "input_config.h"
#include "wc4w.h"
#include "memwrite.h"
#include "configTools.h"

LONG mouse_x_current = 0;
LONG mouse_y_current = 0;


//______________________________________________________________________________________
static LRESULT Update_Mouse_State(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    switch (Message) {

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK: {
        Mouse.Update_Buttons(wParam);

        LONG x = GET_X_LPARAM(lParam);
        LONG y = GET_Y_LPARAM(lParam);

        *p_wc4_mouse_x_space = (WORD)(x * spaceWidth / clientWidth);
        *p_wc4_mouse_y_space = (WORD)(y * spaceHeight / clientHeight);
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
    case WM_MOUSEWHEEL:
        Mouse.Update_Wheel_Vertical(wParam);
        break;
    case WM_MOUSEHWHEEL:
        Mouse.Update_Wheel_Horizontal(wParam);
        break;
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
            *p_wc4_mouse_x_space = (WORD)(ix * spaceWidth / clientWidth);
            ix += client.x;

            fy += y * fheight / GUI_HEIGHT;
            LONG iy = (LONG)fy;
            if ((float)iy != fy)
                iy++;
            *p_wc4_mouse_y_space = (WORD)(iy * spaceHeight / clientHeight);
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

            *p_wc4_mouse_x_space = (WORD)(x * spaceWidth / clientWidth);
            *p_wc4_mouse_y_space = (WORD)(y * spaceHeight / clientHeight);

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


//_______________________________________
static WORD* get_mouse_pos_gui_location() {
    static WORD mouse_pos[3]{ 0 };
    wc4_translate_messages(FALSE, FALSE);
    memcpy(mouse_pos, p_wc4_mouse_button, sizeof(mouse_pos));
    //mouse_pos[0] = 0;
    return mouse_pos;
}

/*
//____________________________________________________
static void __declspec(naked) update_space_mouse(void) {

    __asm {
        mov eax, p_mouse_button_space
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
*/

//__________________________________________________________________________________________
static void Fix_Space_Mouse_Movement(void* p_ship_class, LONG* p_x_roll, LONG* p_y_throttle) {
    // Maximum turn speed was being defined by the screen resolution formally 640x480.
    // Higher resolutions were allowing for a greater mouse range and thus a higher turning speed than what was otherwise defined in game.

    LONG x = *p_wc4_mouse_x_space - *p_wc4_mouse_centre_x;
    LONG y = *p_wc4_mouse_y_space - *p_wc4_mouse_centre_y;

    //keep mouse movement range between -320 and 320 to maintain similar experience as original resolution 640x480.
    LONG range = *p_wc4_mouse_centre_y;
    if (*p_wc4_mouse_centre_y > *p_wc4_mouse_centre_x)
        range = *p_wc4_mouse_centre_x;
    if (range > 320)
        range = 320;

    int dead_zone = Mouse.Deadzone();
    float mouse_unit = 1.25f;

    if (range < 320) {
        dead_zone = range / 320 * dead_zone;
        mouse_unit = (float)range / 256;
    }

    //apply a small dead zone (320 / 32 = 10).
    if (x < dead_zone && x > -dead_zone)
        x = 0;
    if (y < dead_zone && y > -dead_zone)
        y = 0;

    if (x > range)
        x = range;
    else if (x < -range)
        x = -range;

    if (y > range)
        y = range;
    else if (y < -range)
        y = -range;

    //convert mouse movement value to the ships axis range between -256 and 256 (320 / 256 = 1.25).
    x = (LONG)(x / mouse_unit);
    y = (LONG)(y / mouse_unit);

    *p_x_roll = (LONG)x;
    *p_y_throttle = (LONG)y;

    LONG* p_position_class = ((LONG**)p_ship_class)[61];
    p_position_class[4] = (LONG)-x;
    p_position_class[3] = (LONG)-y;
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

    exit_func:
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

    exit_func:
        ret
    }
}


//___________________________________________
static void Nav_Mouse_Control(BYTE* p_struct) {

    static bool being_pressed = 0;
    //rotate view if mouse button 2 pressed.
    if (*p_wc4_mouse_button_space & 0x2) {
        if (!being_pressed)//centre mouse on button click.
            Set_Mouse_Position(320, 240);
        being_pressed = true;

        LONG x = (INT16)*p_wc4_mouse_x_space - *p_wc4_mouse_centre_x;
        LONG y = (INT16)*p_wc4_mouse_y_space - *p_wc4_mouse_centre_y;

        x &= 0xFFFFFFFE;
        *(DWORD*)(p_struct + 0x104) -= x;
        y &= 0xFFFFFFFE;
        *(DWORD*)(p_struct + 0x100) += y;
    }
    else
        being_pressed = false;
}


//___________________________________________________
static void __declspec(naked) nav_mouse_control(void) {

    __asm {
        push ebx
        push ecx
        push esi
        push edi
        push ebp

        push ecx
        call Nav_Mouse_Control
        add esp, 0x4

        pop ebp
        pop edi
        pop esi
        pop ecx
        pop ebx

        //original code, check if joy button 2 pressed.
        mov eax, p_wc4_joy_buttons
        mov al, byte ptr ds : [eax]
        ret

    }
}


//_______________________________________________________________
static void __declspec(naked) centre_mouse_on_mission_start(void) {

    __asm {
        push ebx
        push esi
        push edi
        push ebp

        push 240
        push 320
        call Set_Mouse_Position
        add esp, 0x8

        pop ebp
        pop edi
        pop esi
        pop ebx

        //original code
#ifdef VERSION_WC4_DVD
        add esp, 0x0530
#else
        add esp, 0x0528
#endif
        ret 0x8
    }
}


//_______________________________________________
static void Space_Mouse_Is_Enabled(BYTE is_mouse) {

    *p_wc4_controller_mouse = is_mouse;
    if (*p_wc4_controller_mouse) {
        Debug_Info("SPACE MOUSE IS ENABLED!!!!!!!!!!!");
        Set_Mouse_Position(320, 240);
    }
}


//________________________________________________________
static void __declspec(naked) space_mouse_is_enabled(void) {
    //004383A7 | .  880D EC2D4A00              MOV BYTE PTR DS : [space_mouse_enabled ? ] , CL
    __asm {
        pushad
        push ecx
        call Space_Mouse_Is_Enabled
        add esp, 0x4
        popad

        ret
    }
}


#ifdef VERSION_WC4_DVD
//________________________
void Modifications_Mouse() {

    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x489670, 0x8B, 0xE9);
    FuncWrite32(0x489671, 0x05082444, (DWORD)&Update_Mouse_State);
    MemWrite32(0x489675, 0xFFFFFE00, 0x90909090);

    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x4895A0, 0xA1, 0xE9);
    FuncWrite32(0x4895A1, 0x4CD6AC, (DWORD)&Set_Mouse_Position);

    //prevent setting mouse centre x and y values to 320x240.
    MemWrite16(0x46F59C, 0x05C7, 0x9090);
    MemWrite32(0x46F59E, 0x4C4184, 0x90909090);
    MemWrite32(0x46F5A2, 0x0140, 0x90909090);
    MemWrite16(0x46F5A6, 0x05C7, 0x9090);
    MemWrite32(0x46F5A8, 0x4C4194, 0x90909090);
    MemWrite32(0x46F5AC, 0xF0, 0x90909090);

    //replace set cursor function to allow cursor to freely leave window bounds when in gui mode
    FuncReplace32(0x42B5A0, 0x0736CC, (DWORD)&Update_Cursor_Position);

    //update the cursor position on "ALT+O" space settings screen.
    FuncReplace32(0x49F068, 0xFFFEA534, (DWORD)&Update_Cursor_Position);

    //relate the movement diamond to new space view dimensions rather than original 640x480.
    MemWrite32(0x40AA9E, 0x4C41AC, 0x4CD6B4);
    MemWrite32(0x40AACE, 0x4C41AA, 0x4CD6B2);

    // skip copy "general mouse state" to "space mouse state".
    // "general mouse state" & "space mouse state" are now set in Update_Mouse_State , Set_Mouse_Position, Update_Cursor_Position and Mouse.Update_Buttons.
    MemWrite8(0x46F53E, 0x74, 0xEB);

    //update mouse in space view
    //MemWrite8(0x46F4E0, 0xE8, 0xE9);
    //FuncReplace32(0x46F4E1, 0x01A16B, (DWORD)&update_space_mouse);

    // Mouse turn speed fix--------------------------
    MemWrite16(0x44BE82, 0x2D8B, 0xE890);
    FuncWrite32(0x44BE84, 0x4C4184, (DWORD)&fix_space_mouse_movement);

    //jump over the remaining part of this section
    MemWrite16(0x44BE88, 0xC933, 0x9090);
    MemWrite8(0x44BE8A, 0x66, 0xE9);
    MemWrite32(0x44BE8B, 0x41AA0D8B, 0x80);
    MemWrite16(0x44BE8F, 0x004C, 0x9090);

    //don't multiply x roll value by 16.
    MemWrite16(0x44BF2D, 0xE7C1, 0x9090);
    MemWrite8(0x44BF2F, 0x04, 0x90);
    //don't multiply x roll value by 16.
    MemWrite16(0x44C115, 0xE5C1, 0x9090);
    MemWrite8(0x44C117, 0x04, 0x90);
    //-----------------------------------------------

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

    //add mouse view axis rotation to NAV screen.
    MemWrite8(0x444900, 0xA0, 0xE8);
    FuncWrite32(0x444901, 0x4CD1E8, (DWORD)&nav_mouse_control);

    //centre mouse at the start of missions
    MemWrite16(0x4431F6, 0xC481, 0xE990);
    FuncWrite32(0x4431F8, 0x0530, (DWORD)&centre_mouse_on_mission_start);
}

#else
//________________________
void Modifications_Mouse() {

    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x4AE0D0, 0x8B, 0xE9);
    FuncWrite32(0x4AE0D1, 0x56082444, (DWORD)&Update_Mouse_State);

    //replaces original function for mouse window client position with scaled gui position.
    MemWrite8(0x4AE010, 0x83, 0xE9);
    FuncWrite32(0x4AE011, 0x5CA108EC, (DWORD)&Set_Mouse_Position);
    MemWrite8(0x4AE015, 0xC3, 0x90);
    MemWrite16(0x4AE016, 0x004D, 0x9090);

    //prevent setting mouse centre x and y values to 320x240.
    MemWrite16(0x413379, 0x05C7, 0x9090);
    MemWrite32(0x41337B, 0x4C0658, 0x90909090);
    MemWrite32(0x41337F, 0x0140, 0x90909090);
    MemWrite16(0x413383, 0x05C7, 0x9090);
    MemWrite32(0x413385, 0x4C0678, 0x90909090);
    MemWrite32(0x413389, 0xF0, 0x90909090);

    //replace set cursor function to allow cursor to freely leave window bounds when in gui mode
    FuncReplace32(0x466482, 0x0480AA, (DWORD)&Update_Cursor_Position);

    //update the cursor position on "ALT+O" space settings screen.
    FuncReplace32(0x4AAC18, 0x33F4, (DWORD)&Update_Cursor_Position);

    //relate the movement diamond to new space view dimensions rather than original 640x480.
    MemWrite32(0x423326, 0x4C0674, 0x4DC364);
    MemWrite32(0x423300, 0x4C0672, 0x4DC362);

    // skip copy "general mouse state" to "space mouse state".
    // "general mouse state" & "space mouse state" are now set in Update_Mouse_State , Set_Mouse_Position, Update_Cursor_Position and Mouse.Update_Buttons.
    MemWrite8(0x413326, 0x74, 0xEB);

    //update mouse in space view
    //MemWrite8(0x4132C3, 0xE8, 0xE9);
    //FuncReplace32(0x4132C4, 0x09ADE8, (DWORD)&update_space_mouse);

    // Mouse turn speed fix--------------------------
    MemWrite8(0x448FBB, 0xA1, 0xE8);
    FuncWrite32(0x448FBC, 0x4C0658, (DWORD)&fix_space_mouse_movement);

    //jump over the remaining part of this section
    MemWrite16(0x448FC0, 0x8B66, 0x6EEB);
    MemWrite32(0x448FC2, 0x4C06720D, 0x90909090);
    MemWrite8(0x448FC6, 0x00, 0x90);
    //-----------------------------------------------

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

    //add mouse view axis rotation to NAV screen.
    MemWrite8(0x41C343, 0xA0, 0xE8);
    FuncWrite32(0x41C344, 0x4CE3C4, (DWORD)&nav_mouse_control);

    //centre mouse at the start of missions
    MemWrite16(0x40DED1, 0xC481, 0xE990);
    FuncWrite32(0x40DED3, 0x0528, (DWORD)&centre_mouse_on_mission_start);
}
#endif