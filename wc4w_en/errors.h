/*
The MIT License (MIT)
Copyright � 2025 Matt Wells

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

#pragma once

#define DEBUG_INFO_ERROR    1
#define DEBUG_INFO_GENERAL	2
#define DEBUG_INFO_FLIGHT   4
#define DEBUG_INFO_MOVIE    8
#define DEBUG_INFO_JOY		16

//void Debug_Error(const char* format, ...);
//void Debug_Info(const char* format, ...);
void __Debug_Info(DWORD flags, const char* format, ...);

#define Debug_Info_Error(...)  __Debug_Info( DEBUG_INFO_ERROR, __VA_ARGS__)
#define Debug_Info(...)  __Debug_Info( DEBUG_INFO_GENERAL, __VA_ARGS__)
#define Debug_Info_Flight(...)  __Debug_Info( DEBUG_INFO_FLIGHT, __VA_ARGS__)
#define Debug_Info_Movie(...)  __Debug_Info( DEBUG_INFO_MOVIE, __VA_ARGS__)
#define Debug_Info_Joy(...)  __Debug_Info( DEBUG_INFO_JOY, __VA_ARGS__)



void Error_RecordMemMisMatch(const char* file, int line, DWORD inOffset, const unsigned char* in_expectedData, const unsigned char* in_found_data, size_t inLength);

bool CompareMem_DWORD(const char* file, int line, DWORD* addr, DWORD value);
bool CompareMem_WORD(const char* file, int line, WORD* addr, WORD value);
bool CompareMem_BYTE(const char* file, int line, BYTE* addr, BYTE value);


