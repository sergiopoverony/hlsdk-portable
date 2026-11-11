/*
cz_fallback.h - list the files the mod takes from other game directories
Copyright (C) 2026 sergiopoverony
*/
#pragma once
#if !defined( CZ_FALLBACK_H )
#define CZ_FALLBACK_H

// wraps the precache functions of the engine table: every precached file that is not
// in the mod's own directory goes to <gamedir>/fallback_files.txt
void CZ_FallbackHook( enginefuncs_t *funcs );

#endif // CZ_FALLBACK_H
