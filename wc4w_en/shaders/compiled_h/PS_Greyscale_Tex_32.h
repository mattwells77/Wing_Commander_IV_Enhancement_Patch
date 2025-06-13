#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Buffer Definitions: 
//
// cbuffer Colour_OPT
// {
//
//   float4 colour_val;                 // Offset:    0 Size:    16
//   float4 colour_opt;                 // Offset:   16 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// Sampler_Main                      sampler      NA          NA             s0      1 
// Texture_Main                      texture  float4          2d             t0      1 
// Colour_OPT                        cbuffer      NA          NA            cb1      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Position              0   xyzw        0      POS   float       
// TEXCOORD                 0   xy          1     NONE   float   xy  
// NORMAL                   0   xyz         2     NONE   float       
// POSITION                 0   xyz         3     NONE   float       
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_TARGET                0   xyzw        0   TARGET   float   xyzw
//
//
// Constant buffer to DX9 shader constant mappings:
//
// Target Reg Buffer  Start Reg # of Regs        Data Conversion
// ---------- ------- --------- --------- ----------------------
// c0         cb1             0         2  ( FLT, FLT, FLT, FLT)
//
//
// Sampler/Resource to DX9 shader sampler mappings:
//
// Target Sampler Source Sampler  Source Resource
// -------------- --------------- ----------------
// s0             s0              t0               
//
//
// Level9 shader bytecode:
//
    ps_2_x
    def c2, 0.212599993, 0.715200007, 0.0722000003, -0.5
    def c3, 0, 0, 0, 0
    dcl t0.xy
    dcl_2d s0
    texld r0, t0, s0
    dp3 r1.w, r0, c2
    mov r1.y, c1.y
    max r2.w, r1.y, c3.x
    lrp r3.w, r2.w, r1.w, -c2.w
    mul r1.xyz, r3.w, c0
    mul_sat r0.xyz, r1, c1.x
    mov oC0, r0

// approximately 8 instruction slots used (1 texture, 7 arithmetic)
ps_4_0
dcl_constantbuffer CB1[2], immediateIndexed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 1
sample r0.xyzw, v1.xyxx, t0.xyzw, s0
dp3 r0.x, r0.xyzx, l(0.212600, 0.715200, 0.072200, 0.000000)
mov o0.w, r0.w
add r0.x, r0.x, l(-0.500000)
max r0.y, cb1[1].y, l(0.000000)
mad r0.x, r0.x, r0.y, l(0.500000)
mul r0.xyz, r0.xxxx, cb1[0].xyzx
mul_sat o0.xyz, r0.xyzx, cb1[1].xxxx
ret 
// Approximately 9 instruction slots used
#endif

const BYTE pPS_Greyscale_Tex_32_mem[] =
{
     68,  88,  66,  67, 244, 129, 
    226, 166,   8, 229, 240,  94, 
    129,  52, 108,  31, 236, 174, 
     42, 255,   1,   0,   0,   0, 
     44,   5,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
     64,   1,   0,   0, 160,   2, 
      0,   0,  28,   3,   0,   0, 
     96,   4,   0,   0, 248,   4, 
      0,   0,  65, 111, 110,  57, 
      0,   1,   0,   0,   0,   1, 
      0,   0,   0,   2, 255, 255, 
    204,   0,   0,   0,  52,   0, 
      0,   0,   1,   0,  40,   0, 
      0,   0,  52,   0,   0,   0, 
     52,   0,   1,   0,  36,   0, 
      0,   0,  52,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   1,   2, 255, 255, 
     81,   0,   0,   5,   2,   0, 
     15, 160, 208, 179,  89,  62, 
     89,  23,  55,  63, 152, 221, 
    147,  61,   0,   0,   0, 191, 
     81,   0,   0,   5,   3,   0, 
     15, 160,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   0,   0,   3, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   0,   8,  15, 160, 
     66,   0,   0,   3,   0,   0, 
     15, 128,   0,   0, 228, 176, 
      0,   8, 228, 160,   8,   0, 
      0,   3,   1,   0,   8, 128, 
      0,   0, 228, 128,   2,   0, 
    228, 160,   1,   0,   0,   2, 
      1,   0,   2, 128,   1,   0, 
     85, 160,  11,   0,   0,   3, 
      2,   0,   8, 128,   1,   0, 
     85, 128,   3,   0,   0, 160, 
     18,   0,   0,   4,   3,   0, 
      8, 128,   2,   0, 255, 128, 
      1,   0, 255, 128,   2,   0, 
    255, 161,   5,   0,   0,   3, 
      1,   0,   7, 128,   3,   0, 
    255, 128,   0,   0, 228, 160, 
      5,   0,   0,   3,   0,   0, 
     23, 128,   1,   0, 228, 128, 
      1,   0,   0, 160,   1,   0, 
      0,   2,   0,   8,  15, 128, 
      0,   0, 228, 128, 255, 255, 
      0,   0,  83,  72,  68,  82, 
     88,   1,   0,   0,  64,   0, 
      0,   0,  86,   0,   0,   0, 
     89,   0,   0,   4,  70, 142, 
     32,   0,   1,   0,   0,   0, 
      2,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      0,   0,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      0,   0,   0,   0,  85,  85, 
      0,   0,  98,  16,   0,   3, 
     50,  16,  16,   0,   1,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 104,   0,   0,   2, 
      1,   0,   0,   0,  69,   0, 
      0,   9, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  16,   0, 
      0,  10,  18,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0, 208, 179, 
     89,  62,  89,  23,  55,  63, 
    152, 221, 147,  61,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0, 191,  52,   0,   0,   8, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  26, 128,  32,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,  63,  56,   0, 
      0,   8, 114,   0,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   0,   0,   0,   0, 
     70, 130,  32,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
     56,  32,   0,   8, 114,  32, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,   6, 128,  32,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  62,   0,   0,   1, 
     83,  84,  65,  84, 116,   0, 
      0,   0,   9,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      6,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  82,  68, 
     69,  70,  60,   1,   0,   0, 
      1,   0,   0,   0, 164,   0, 
      0,   0,   3,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    255, 255,   0,   1,   0,   0, 
     19,   1,   0,   0, 124,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
    137,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0, 150,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  83,  97, 
    109, 112, 108, 101, 114,  95, 
     77,  97, 105, 110,   0,  84, 
    101, 120, 116, 117, 114, 101, 
     95,  77,  97, 105, 110,   0, 
     67, 111, 108, 111, 117, 114, 
     95,  79,  80,  84,   0, 171, 
    171, 171, 150,   0,   0,   0, 
      2,   0,   0,   0, 188,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 236,   0,   0,   0, 
      0,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
    248,   0,   0,   0,   0,   0, 
      0,   0,   8,   1,   0,   0, 
     16,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
    248,   0,   0,   0,   0,   0, 
      0,   0,  99, 111, 108, 111, 
    117, 114,  95, 118,  97, 108, 
      0, 171,   1,   0,   3,   0, 
      1,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     99, 111, 108, 111, 117, 114, 
     95, 111, 112, 116,   0,  77, 
    105,  99, 114, 111, 115, 111, 
    102, 116,  32,  40,  82,  41, 
     32,  72,  76,  83,  76,  32, 
     83, 104,  97, 100, 101, 114, 
     32,  67, 111, 109, 112, 105, 
    108, 101, 114,  32,  49,  48, 
     46,  49,   0, 171,  73,  83, 
     71,  78, 144,   0,   0,   0, 
      4,   0,   0,   0,   8,   0, 
      0,   0, 104,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0, 116,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   3,   3, 
      0,   0, 125,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,   7,   0, 
      0,   0, 132,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,   7,   0, 
      0,   0,  83,  86,  95,  80, 
    111, 115, 105, 116, 105, 111, 
    110,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0,  78, 
     79,  82,  77,  65,  76,   0, 
     80,  79,  83,  73,  84,  73, 
     79,  78,   0, 171, 171, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  65,  82,  71,  69, 
     84,   0, 171, 171
};
