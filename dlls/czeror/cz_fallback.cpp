/*
cz_fallback.cpp - list the files the mod takes from other game directories
Copyright (C) 2026 sergiopoverony

czeror is mounted over cstrike and valve: a file that is missing in czeror/ is
taken from there. To make the mod standalone the files it really uses have to
be known. Every precached model, sprite, sound, event and generic file is looked up
on disk in the mod's own directory; the ones that aren't there are written
once to <gamedir>/fallback_files.txt (and the console).
*/

#include "extdll.h"
#include "util.h"
#include "czeror/cz_fallback.h"

#include <stdio.h>
#include <ctype.h>
#include <string>
#include <set>

static int ( *g_pfnPrecacheModel )( const char *s );
static int ( *g_pfnPrecacheSound )( const char *s );
static int ( *g_pfnPrecacheGeneric )( const char *s );
static unsigned short ( *g_pfnPrecacheEvent )( int type, const char *psz );

static std::set<std::string> g_fallbackSeen;
static std::set<std::string> g_pk3Files;	// lowercase names inside <mod>/*.pk3
static bool g_pk3Loaded = false;

static std::string CZ_Lower( std::string s )
{
	for( size_t i = 0; i < s.size(); i++ )
		s[i] = (char)tolower( (unsigned char)s[i] );
	return s;
}

// the names of the central directory of a zip (pk3) file
static void CZ_ReadPk3( const std::string &path )
{
	FILE *f = fopen( path.c_str(), "rb" );
	if( !f )
		return;

	fseek( f, 0, SEEK_END );
	long size = ftell( f );
	long tail = size < 65557 ? size : 65557;
	std::string buf( tail, '\0' );
	fseek( f, size - tail, SEEK_SET );
	if( fread( &buf[0], 1, tail, f ) != (size_t)tail )
	{
		fclose( f );
		return;
	}

	long eocd = -1;
	for( long i = tail - 22; i >= 0; i-- )
	{
		if( buf[i] == 'P' && buf[i + 1] == 'K' && buf[i + 2] == 5 && buf[i + 3] == 6 )
		{
			eocd = i;
			break;
		}
	}

	if( eocd < 0 )
	{
		fclose( f );
		return;
	}

	const unsigned char *e = (const unsigned char *)&buf[eocd];
	unsigned int count = e[10] | ( e[11] << 8 );
	unsigned int dirSize = e[12] | ( e[13] << 8 ) | ( e[14] << 16 ) | ( (unsigned int)e[15] << 24 );
	unsigned int dirOffset = e[16] | ( e[17] << 8 ) | ( e[18] << 16 ) | ( (unsigned int)e[19] << 24 );

	std::string dir( dirSize, '\0' );
	fseek( f, dirOffset, SEEK_SET );
	if( dirSize && fread( &dir[0], 1, dirSize, f ) == dirSize )
	{
		size_t p = 0;
		for( unsigned int n = 0; n < count && p + 46 <= dir.size(); n++ )
		{
			const unsigned char *c = (const unsigned char *)&dir[p];
			if( c[0] != 'P' || c[1] != 'K' || c[2] != 1 || c[3] != 2 )
				break;
			unsigned int nameLen = c[28] | ( c[29] << 8 );
			unsigned int extraLen = c[30] | ( c[31] << 8 );
			unsigned int commentLen = c[32] | ( c[33] << 8 );
			if( p + 46 + nameLen > dir.size())
				break;
			g_pk3Files.insert( CZ_Lower( dir.substr( p + 46, nameLen )));
			p += 46 + nameLen + extraLen + commentLen;
		}
	}

	fclose( f );
}

static bool CZ_FileExists( const std::string &path )
{
	FILE *f = fopen( path.c_str(), "rb" );
	if( !f )
		return false;
	fclose( f );
	return true;
}

static void CZ_FallbackCheck( const char *prefix, const char *name )
{
	if( !name || !name[0] || name[0] == '*' || name[0] == '!' || name[0] == '#' )
		return;

	std::string rel = std::string( prefix ) + name;
	for( size_t i = 0; i < rel.size(); i++ )
	{
		if( rel[i] == '\\' )
			rel[i] = '/';
	}

	if( g_fallbackSeen.count( rel ))
		return;
	g_fallbackSeen.insert( rel );

	char gamedir[256];
	GET_GAME_DIR( gamedir );

	// GET_GAME_DIR may give a full path or only the directory name
	std::string dir = gamedir;
	const char *slash = strrchr( gamedir, '/' );
	if( !slash )
		slash = strrchr( gamedir, '\\' );
	std::string modName = slash ? std::string( slash + 1 ) : dir;

	if( CZ_FileExists( dir + "/" + rel ) || CZ_FileExists( modName + "/" + rel ))
		return;

	if( !g_pk3Loaded )
	{
		// Xash3D mounts the pk3 archives of the mod directory too
		static const char *pk3s[] = { "extras.pk3", "pak0.pk3", "pak1.pk3", "pak2.pk3" };
		g_pk3Loaded = true;
		for( size_t i = 0; i < sizeof( pk3s ) / sizeof( pk3s[0] ); i++ )
		{
			CZ_ReadPk3( modName + "/" + pk3s[i] );
			CZ_ReadPk3( dir + "/" + pk3s[i] );
		}
	}

	if( g_pk3Files.count( CZ_Lower( rel )))
		return;

	ALERT( at_console, "fallback: %s\n", rel.c_str());

	std::string listPath = modName + "/fallback_files.txt";
	FILE *f = fopen( listPath.c_str(), "a" );
	if( !f )
		f = fopen(( dir + "/fallback_files.txt" ).c_str(), "a" );
	if( f )
	{
		fprintf( f, "%s\n", rel.c_str());
		fclose( f );
	}
}

static int CZ_PrecacheModel( const char *s )
{
	CZ_FallbackCheck( "", s );
	return g_pfnPrecacheModel( s );
}

static int CZ_PrecacheSound( const char *s )
{
	CZ_FallbackCheck( "sound/", s );
	return g_pfnPrecacheSound( s );
}

static int CZ_PrecacheGeneric( const char *s )
{
	CZ_FallbackCheck( "", s );
	return g_pfnPrecacheGeneric( s );
}

static unsigned short CZ_PrecacheEvent( int type, const char *psz )
{
	CZ_FallbackCheck( "", psz );
	return g_pfnPrecacheEvent( type, psz );
}

void CZ_FallbackHook( enginefuncs_t *funcs )
{
	g_pfnPrecacheModel = funcs->pfnPrecacheModel;
	g_pfnPrecacheSound = funcs->pfnPrecacheSound;
	g_pfnPrecacheGeneric = funcs->pfnPrecacheGeneric;
	g_pfnPrecacheEvent = funcs->pfnPrecacheEvent;

	funcs->pfnPrecacheModel = CZ_PrecacheModel;
	funcs->pfnPrecacheSound = CZ_PrecacheSound;
	funcs->pfnPrecacheGeneric = CZ_PrecacheGeneric;
	funcs->pfnPrecacheEvent = CZ_PrecacheEvent;
}
