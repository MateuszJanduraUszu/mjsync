// main.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/details/tinywin.hpp>

int __stdcall DllMain(HMODULE, unsigned long, void*) {
#if defined(_M_X64) || defined(_M_IX86)
    return 1;
#else // ^^^ x64 or x86 ^^^ / vvv not supported architecture vvv
    return 0;
#endif // defined(_M_X64) || defined(_M_IX86)
}