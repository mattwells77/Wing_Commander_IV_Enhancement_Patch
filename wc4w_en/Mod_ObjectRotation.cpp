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



//_________________________________________________
static float Rotation_to_Radians(LONG rotation_val) {
    //trim exccesive rotations to within -46080 and 46080, 180 deg in either direction.
    LONG temp = rotation_val;
    if (rotation_val < -46080) {
        temp = (46079 - rotation_val) / 92160;
        temp *= 92160;
        rotation_val += temp;
    }
    if (rotation_val > 46080) {
        temp = (46079 + rotation_val) / 92160;
        temp *= 92160;
        temp *= -1;
        rotation_val += temp;
    }
    //convert rotation integer value to radians
    //rotation_val / (46080/PI)
    return  (float)rotation_val / 14667.719555349074144460327632411f;
}


//_________________________________________________________
static void CopyMatrix3x3(float to[3][3], float from[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int k = 0; k < 3; k++)
            to[i][k] = from[i][k];
    }
}

static void CopyMatrix3x3(LONG to[3][3], LONG from[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int k = 0; k < 3; k++)
            to[i][k] = from[i][k];
    }
}

static void CopyMatrix3x3(LONG to[3][3], float from[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int k = 0; k < 3; k++)
            to[i][k] = (LONG)round(from[i][k] * 256);
    }
}

static void CopyMatrix3x3(float to[3][3], LONG from[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int k = 0; k < 3; k++)
            to[i][k] = (float)from[i][k] / 256.0f;
    }
}


//________________________________________________________________________________________
static void MultiplyMatrices3x3(float first[3][3], float second[3][3], float result[3][3]) {

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                result[i][j] += first[i][k] * second[k][j];
            }
        }
    }
}


//________________________________________
static float Vector3_GetNormal(float a[3]) {

    float normal = 0.0f;

    for (int i = 0; i < 3; i++)
        normal += a[i] * a[i];

    return sqrt(normal);
}


//_____________________________________________________
static float Vector3_DotProduct(float a[3], float b[3]) {

    float vecProduct = 0.0f;

    for (int i = 0; i < 3; i++)
        vecProduct += a[i] * b[i];

    return  vecProduct;
}


//_______________________________________________________________________
static void Vector3_CrossProduct(float a[3], float b[3], float result[3]) {
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}


//___________________________________________________________________
static void Vector3_Subtract(float a[3], float b[3], float result[3]) {

    for (int i = 0; i < 3; i++)
        result[i] = a[i] - b[i];
}


//_______________________________________________________________
static void Vector3_Multi(float a[3], float val, float result[3]) {

    for (int i = 0; i < 3; i++)
        result[i] = a[i] * val;
}

/*
//_______________________________________
static void Vector3_Normalize(float a[3]) {

    float norm = Vector3_GetNormal(a);
    a[0] /= norm;
    a[1] /= norm;
    a[2] /= norm;
}


//_________________________________________________________
static float Vector3_DotProductNorm(float a[3], float b[3]) {

    float norm = Vector3_GetNormal(b);
    float vecProduct = 0.0f;

    for (int i = 0; i < 3; i++)
        vecProduct += a[i] * b[i];

    return  vecProduct / (norm * norm);
}


//_________________________________________________________________
static void OrthonormalizeMatrix3x3_GramSchmidt(float matrix[3][3]) {

    float v[3][3]{ 0.0f };
    for (int k = 0; k < 3; k++) {
        for (int i = 0; i < 3; i++) {
            if (k == 0)
                v[k][i] = matrix[k][i];
            else if (k == 1)
                v[k][i] = matrix[k][i] - Vector3_DotProductNorm(matrix[k], v[0]) * v[0][i];
            else if (k == 2)
                v[k][i] = matrix[k][i] - Vector3_DotProductNorm(matrix[k], v[0]) * v[0][i] - Vector3_DotProductNorm(matrix[k], v[1]) * v[1][i];
        }
        for (int i = 0; i < 3; i++)
            matrix[k][i] = v[k][i] * (1.0f / Vector3_GetNormal(v[k]));// Normalise 3 vectors and store to Results
    }
    return;
}
*/

//__________________________________________________________________________
//This method was taken from "Direction Cosine Matrix IMU: Theory" by William Premerlani and Paul Bizard.
static void OrthonormalizeMatrix3x3_PremerlaniBizard(float rot_matrix[3][3]) {

    float x_ori[3]{ rot_matrix[0][0], rot_matrix[1][0], rot_matrix[2][0] };
    float y_ori[3]{ rot_matrix[0][1], rot_matrix[1][1], rot_matrix[2][1] };
    float z_ori[3]{ rot_matrix[0][2], rot_matrix[1][2], rot_matrix[2][2] };

    float x_mod[3]{ rot_matrix[0][0], rot_matrix[1][0], rot_matrix[2][0] };
    float y_mod[3]{ rot_matrix[0][1], rot_matrix[1][1], rot_matrix[2][1] };
    float z_mod[3]{ rot_matrix[0][2], rot_matrix[1][2], rot_matrix[2][2] };

    //x_vec_new = x_vec - (error_xy / 2) * y_vec;
    //y_vec_new = y_vec - (error_xy / 2) * x_vec;
    //adjust to z also, for greater accuracy.

    //get the error between axes xyz.
    float error_xy = 0.5f * Vector3_DotProduct(x_ori, y_ori);
    float error_zy = 0.5f * Vector3_DotProduct(z_ori, y_ori);
    float error_zx = 0.5f * Vector3_DotProduct(z_ori, x_ori);

    float error_v[3]{};

    //adjust x to half the xy error.
    Vector3_Multi(y_ori, error_xy, error_v);
    Vector3_Subtract(x_mod, error_v, x_mod);
    //adjust x to half the zx error.
    Vector3_Multi(z_ori, error_zx, error_v);
    Vector3_Subtract(x_mod, error_v, x_mod);


    //adjust y to half the xy error.
    Vector3_Multi(x_ori, error_xy, error_v);
    Vector3_Subtract(y_mod, error_v, y_mod);
    //adjust y to half the zy error.
    Vector3_Multi(z_ori, error_zy, error_v);
    Vector3_Subtract(y_mod, error_v, y_mod);

    //adjust z to half the zy error.
    //Vector3_Multi(y_ori, error_zy, error_v);
    //Vector3_Subtract(z_mod, error_v, z_mod);
    //adjust z to half the zx error.
    //Vector3_Multi(x_ori, error_zx, error_v);
    //Vector3_Subtract(z_mod, error_v, z_mod);
    Vector3_CrossProduct(x_mod, y_mod, z_mod);


    //scale the rows of the matrix to assure that each has a magnitude equal to one using Taylor’s expansion.
    //x_vec_norm = 0.5 * (3 - dotproduct(x_vec, x_vec)) * x_vec;

    Vector3_Multi(x_mod, (3 - Vector3_DotProduct(x_mod, x_mod)), x_mod);
    Vector3_Multi(x_mod, 0.5, x_mod);

    Vector3_Multi(y_mod, (3 - Vector3_DotProduct(y_mod, y_mod)), y_mod);
    Vector3_Multi(y_mod, 0.5, y_mod);

    Vector3_Multi(z_mod, (3 - Vector3_DotProduct(z_mod, z_mod)), z_mod);
    Vector3_Multi(z_mod, 0.5, z_mod);


    rot_matrix[0][0] = x_mod[0];
    rot_matrix[1][0] = x_mod[1];
    rot_matrix[2][0] = x_mod[2];

    rot_matrix[0][1] = y_mod[0];
    rot_matrix[1][1] = y_mod[1];
    rot_matrix[2][1] = y_mod[2];

    rot_matrix[0][2] = z_mod[0];
    rot_matrix[1][2] = z_mod[1];
    rot_matrix[2][2] = z_mod[2];
}


//___________________________________________________________________________
static void Update_Object_Rotation(LONG i_obj_matrix[3][3], LONG* obj_struct) {

    //maintain a float version of the player rotation matrix for greater accuracy.
    static int player_matrix_enabled = ConfigReadInt(L"SPACE", L"ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX", CONFIG_SPACE_ENABLE_ENHANCED_PLAYER_ROTATION_MATRIX);
    static LONG i_player_matrix_prev[3][3]{ 0 };
    static float f_player_matrix[3][3]{ 0.0f };

    LONG p_axis = obj_struct[19];
    LONG y_axis = obj_struct[20];
    LONG r_axis = obj_struct[21];

    bool is_player = false;
    if (player_matrix_enabled && (void*)(obj_struct[2]) == *pp_wc4_player_obj_struct)
        is_player = true;

    float f_y_axis = Rotation_to_Radians(-y_axis);
    float f_p_axis = Rotation_to_Radians(-p_axis);
    float f_r_axis = Rotation_to_Radians(-r_axis);

    float f_calc_matrix[3][3]{ 0.0f };
    float f_ret_matrix[3][3]{ 0.0f };

    float sinP = sin(f_p_axis);
    float cosP = cos(f_p_axis);
    float sinR = sin(f_r_axis);
    float cosR = cos(f_r_axis);
    float sinY = sin(f_y_axis);
    float cosY = cos(f_y_axis);

    f_calc_matrix[0][0] = cosR * cosY;
    f_calc_matrix[0][1] = sinP * sinR * cosY - cosP * sinY;
    f_calc_matrix[0][2] = sinP * sinY + cosP * sinR * cosY;
    f_calc_matrix[1][0] = cosR * sinY;
    f_calc_matrix[1][1] = cosP * cosY + sinP * sinR * sinY;
    f_calc_matrix[1][2] = cosP * sinR * sinY - sinP * cosY;
    f_calc_matrix[2][0] = -sinR;
    f_calc_matrix[2][1] = sinP * cosR;
    f_calc_matrix[2][2] = cosP * cosR;

    if (is_player) {
        //check if matrix was altered outside this function, when auto piloting etc.
        bool matrix_changed = false;
        for (int i = 0; i < 3; i++) {
            for (int k = 0; k < 3; k++)
                if (i_obj_matrix[i][k] != i_player_matrix_prev[i][k])
                    matrix_changed = true;
        }
        //update (float)player matrix if main (integer)matrix was altered outside of this function. 
        if (matrix_changed) {
            //Debug_Info("Player Matrix Changed");
            CopyMatrix3x3(f_player_matrix, i_obj_matrix);
        }
        //add new rotations to return matrix (player).
        MultiplyMatrices3x3(f_calc_matrix, f_player_matrix, f_ret_matrix);

        CopyMatrix3x3(f_player_matrix, f_ret_matrix);
    }
    else {
        float f_obj_matrix[3][3]{ 0.0f };

        CopyMatrix3x3(f_obj_matrix, i_obj_matrix);
        //add new rotation to return matrix (general object).
        MultiplyMatrices3x3(f_calc_matrix, f_obj_matrix, f_ret_matrix);
    }

    //fix accumulating errors when adding new rotations to low res (integer)matrix.
    OrthonormalizeMatrix3x3_PremerlaniBizard(f_ret_matrix);
    //OrthonormalizeMatrix3x3_GramSchmidt(ret_matrix);

    //update object matrix
    CopyMatrix3x3(i_obj_matrix, f_ret_matrix);

    //store the current player (integer)matrix for testing if altered outside this function.
    if (is_player)
        CopyMatrix3x3(i_player_matrix_prev, i_obj_matrix);
}


#ifdef VERSION_WC4_DVD
//________________________________________________________
static void __declspec(naked) update_object_rotation(void) {

    __asm {
        pushad
        mov ecx, dword ptr ds : [esi + 0x8]
        add ecx, 0x34
        push esi
        push ecx
        call Update_Object_Rotation
        add esp, 0x8
        popad

        ret
    }
}

#else
//________________________________________________________
static void __declspec(naked) update_object_rotation(void) {

    __asm {
        pushad
        mov ecx, dword ptr ds : [ebx + 0x8]
        add ecx, 0x34
        push ebx
        push ecx
        call Update_Object_Rotation
        add esp, 0x8
        popad


        ret
    }
}

#endif


//____________________________________________________________________________________________________________________
static void Update_Object_Turret_Rotation(LONG i_obj_matrix[3][3], LONG* p_yaw_axis_offset, LONG* p_pitch_axis_offset) {

    float f_y_axis = Rotation_to_Radians(-*p_yaw_axis_offset);
    float f_p_axis = Rotation_to_Radians(*p_pitch_axis_offset);

    float f_calc_matrix[3][3]{ 0.0f };
    float f_ret_matrix[3][3]{ 0.0f };
    float f_obj_matrix[3][3]{ 0.0f };

    float sinP = sin(f_p_axis);
    float cosP = cos(f_p_axis);
    float sinY = sin(f_y_axis);
    float cosY = cos(f_y_axis);

    f_calc_matrix[0][0] = cosY;
    f_calc_matrix[0][1] = -cosP * sinY;
    f_calc_matrix[0][2] = sinP * sinY;
    f_calc_matrix[1][0] = sinY;
    f_calc_matrix[1][1] = cosP * cosY;
    f_calc_matrix[1][2] = -sinP * cosY;
    f_calc_matrix[2][0] = 0;
    f_calc_matrix[2][1] = sinP;
    f_calc_matrix[2][2] = cosP;

    CopyMatrix3x3(f_obj_matrix, i_obj_matrix);
    //add new rotation to return matrix (general object).
    MultiplyMatrices3x3(f_calc_matrix, f_obj_matrix, f_ret_matrix);

    //fix accumulating errors when adding new rotations to low res (integer)matrix.
    OrthonormalizeMatrix3x3_PremerlaniBizard(f_ret_matrix);
    //OrthonormalizeMatrix3x3_GramSchmidt(ret_matrix);

    //update object matrix
    CopyMatrix3x3(i_obj_matrix, f_ret_matrix);
}


#ifdef VERSION_WC4_DVD
//_______________________________________________________________
static void __declspec(naked) update_object_turret_rotation(void) {

    __asm {
        pushad
        lea ecx, [esi + 0x34]//turret matrix
        push ebp//pitch
        push edi//yaw
        push ecx
        call Update_Object_Turret_Rotation
        add esp, 0xC
        popad

        ret
    }
}

#else
//_______________________________________________________________
static void __declspec(naked) update_object_turret_rotation(void) {

    __asm {
        pushad
        lea ecx, [esi + 0xB0]//pitch
        lea eax, [esi + 0xB8]//yaw
        push ecx
        push eax
        lea ecx, [esi + 0x34]//turret matrix
        push ecx
        call Update_Object_Turret_Rotation
        add esp, 0xC
        popad

        ret
    }
}

#endif


//____________________________________________________________________
static void PC_Rotation_Calulations(void* obj_struct, LONG frame_time) {

    void* ship_properties = ((void**)obj_struct)[1];
    
    LONG ship_pitch_modifier = ((LONG*)ship_properties)[1];
    LONG ship_yaw_modifier = ((LONG*)ship_properties)[2];
    LONG ship_roll_modifier = ((LONG*)ship_properties)[3];

    LONG raw_pitch = ((LONG*)obj_struct)[26];
    LONG raw_yaw = ((LONG*)obj_struct)[27];
    LONG raw_roll = ((LONG*)obj_struct)[28];

    //frame_time represented in units of 256th's of a second.
    //frame_time = 65536 / (frame_rate * 256);
    float f_frame_time = frame_time / 256.0f;

    //preform rotation calulations for next rendering.
    float f_pitch = (float)raw_pitch * ship_pitch_modifier * f_frame_time;
    float f_yaw = (float)raw_yaw * ship_yaw_modifier * f_frame_time;
    float f_roll = (float)raw_roll * ship_roll_modifier * f_frame_time;

    //set new rotation offsets for matrix update.
    ((LONG*)obj_struct)[19] = (LONG)f_pitch; //obj_struct.pitch_axis
    ((LONG*)obj_struct)[20] = (LONG)f_yaw; //obj_struct.yaw_axis
    ((LONG*)obj_struct)[21] = (LONG)f_roll; //obj_struct.roll_axis
}


#ifdef VERSION_WC4_DVD
//_________________________________________________________
static void __declspec(naked) pc_rotation_calulations(void) {

    __asm {
        push esi
        push edi

        push edi
        push esi
        call PC_Rotation_Calulations
        add esp, 0x8

        pop edi
        pop esi
        ret
    }
}


#else
//_________________________________________________________
static void __declspec(naked) pc_rotation_calulations(void) {

    __asm {
        push ebx
        push ebp

        push ebp
        push ebx

        call PC_Rotation_Calulations
        add esp, 0x8

        pop ebp
        pop ebx
        ret
    }
}
#endif


#ifdef VERSION_WC4_DVD
//_________________________________
void Modifications_ObjectRotation() {

    //-----Player-Object-rotation-improvements-------------------------------
    //fix minor drifting due to integer rounding when performing pc rotation calculations.
    MemWrite16(0x423692, 0x6E8B, 0xE890);
    FuncWrite32(0x423694, 0x6C5E8B68, (DWORD)&pc_rotation_calulations);
    //jump over original rotation calculations.
    MemWrite16(0x423698, 0xF8C1, 0xE990);//JMP 00423834
    MemWrite32(0x42369A, 0x04FDC104, 0x0196);

    //update object rotation matrix.
    MemWrite16(0x423EF3, 0x4E8B, 0xE890);
    FuncWrite32(0x423EF5, 0x50468D08, (DWORD)&update_object_rotation);
    //jump over regular rotation update functions
    MemWrite8(0x423EF9, 0x50, 0xE9);//JMP 00424027
    MemWrite32(0x423EFA, 0xE834C183, 0x0129);
    MemWrite32(0x423EFE, 0x05A0BE, 0x90909090);
    //-----------------------------------------------------------------------

    //-----Player-Object-turret-rotation-improvements------------------------
    MemWrite8(0x4258FD, 0x57, 0xE8);
    FuncWrite32(0x4258FE, 0xE8344E8D, (DWORD)&update_object_turret_rotation);
    //jump over regular turret rotation update functions
    MemWrite16(0x425902, 0x86BA, 0x0BEB);//JMP SHORT 0042590F
    MemWrite16(0x425904, 0x0005, 0x9090);
    //-----------------------------------------------------------------------
}

#else
//_________________________________
void Modifications_ObjectRotation() {

    //-----Player-Object-rotation-improvements-------------------------------
    //fix minor drifting due to integer rounding when performing pc rotation calculations.
    MemWrite16(0x454FBC, 0xF8C1, 0xE890);
    FuncWrite32(0x454FBE, 0x04F9C104, (DWORD)&pc_rotation_calulations);
    //jump over original rotation calculations.
    MemWrite16(0x454FC2, 0x4389, 0xE990);//JMP 00455161
    MemWrite32(0x454FC4, 0x04FFC170, 0x0199);

    //update object rotation matrix.
    MemWrite16(0x455883, 0x438D, 0xE890);
    FuncWrite32(0x455885, 0x084B8B50, (DWORD)&update_object_rotation);
    //jump over regular rotation update functions
    MemWrite8(0x455889, 0x50, 0xE9);//JMP 004559D8
    MemWrite32(0x45588A, 0xE834C183, 0x014A);
    MemWrite32(0x45588E, 0x049FEE, 0x90909090);
    //-----------------------------------------------------------------------

    //-----Player-Object-turret-rotation-improvements------------------------
    MemWrite16(0x464F97, 0x868D, 0xE890);
    FuncWrite32(0x464F99, 0xB8, (DWORD)&update_object_turret_rotation);
    //jump over regular turret rotation update functions
    MemWrite16(0x464F9D, 0x4E8D, 0x16EB);//JMP SHORT 00464FB5
    MemWrite8(0x464F9F, 0x34, 0x90);
    //-----------------------------------------------------------------------
}

#endif