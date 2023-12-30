// main.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/impl/tinywin.hpp>

int __stdcall DllMain(HMODULE _Module, unsigned long _Reason, void*) {
#if defined(_M_X64) || defined(_M_IX86)
    if (_Reason == DLL_PROCESS_ATTACH) {
        ::DisableThreadLibraryCalls(_Module);
    }

    return 1;
#else // ^^^ x64 or x86 ^^^ / vvv not supported architecture vvv
    return 0;
#endif // defined(_M_X64) || defined(_M_IX86)
}