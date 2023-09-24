// api.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_API_HPP_
#define _MJSYNC_API_HPP_

#ifdef MJSYNC_EXPORTS
#define _MJSYNC_API __declspec(dllexport)
#else // ^^^ MJSYNC_EXPORTS ^^^ / vvv !MJSYNC_EXPORTS vvv
#define _MJSYNC_API __declspec(dllimport)
#endif // MJSYNC_EXPORTS
#endif // _MJSYNC_API_HPP_