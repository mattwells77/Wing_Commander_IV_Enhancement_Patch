/*
The MIT License (MIT)
Copyright � 2023 Matt Wells

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the �Software�), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

Texture2D Texture_Main : register(t0);
Texture2D Texture_Pal : register(t1);

sampler Sampler_Main : register(s0);

cbuffer Palette : register(b0) {
    float4 pal_colour;

};

cbuffer Colour_OPT : register(b1)
{
    float4 colour_val;
    float4 colour_opt;
};
struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 TexCoords : TEXCOORD0;
    float3 Normal : NORMAL0;
    float3 WorldPos : POSITION0;
};
