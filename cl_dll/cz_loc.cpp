// CZ:DS: client-side "#KEY" localization (titles.txt entries like "#valve_game_saved")
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <map>
#include <vector>
#include "hud.h"
#include "cl_util.h"

static std::map<std::string, std::string> g_czLoc;
static bool g_czLocLoaded = false;

static std::string CzLower( const std::string &s )
{
	std::string r = s;
	for( size_t i = 0; i < r.size(); i++ )
		r[i] = (char)tolower( (unsigned char)r[i] );
	return r;
}

static void CzLoc_Parse( const std::string &text )
{
	struct Tok { bool brace; std::string str; };
	std::vector<Tok> toks;
	size_t i = 0, n = text.size();
	while( i < n )
	{
		char c = text[i];
		if( c == '/' && i + 1 < n && text[i + 1] == '/' )
		{
			while( i < n && text[i] != '\n' ) i++;
		}
		else if( c == '{' || c == '}' )
		{
			Tok t; t.brace = true; t.str = std::string( 1, c );
			toks.push_back( t );
			i++;
		}
		else if( c == '"' )
		{
			std::string v;
			i++;
			while( i < n && text[i] != '"' )
			{
				if( text[i] == '\\' && i + 1 < n )
				{
					char e = text[i + 1];
					if( e == 'n' ) v += '\n';
					else if( e == 't' ) v += '\t';
					else v += e;
					i += 2;
				}
				else
					v += text[i++];
			}
			i++;
			Tok t; t.brace = false; t.str = v;
			toks.push_back( t );
		}
		else
			i++;
	}

	for( size_t k = 0; k + 1 < toks.size(); k++ )
	{
		if( toks[k].brace || toks[k + 1].brace )
			continue;
		g_czLoc[CzLower( toks[k].str )] = toks[k + 1].str;
		k++;
	}
}

static void CzLoc_LoadFile( const char *path )
{
	int len = 0;
	char *data = (char *)gEngfuncs.COM_LoadFile( path, 5, &len );
	if( !data )
		return;

	std::string text;
	if( len >= 2 && (unsigned char)data[0] == 0xFF && (unsigned char)data[1] == 0xFE )
	{
		// UTF-16LE -> UTF-8
		for( int p = 2; p + 1 < len; p += 2 )
		{
			unsigned c = (unsigned char)data[p] | ( (unsigned char)data[p + 1] << 8 );
			if( c < 0x80 ) text += (char)c;
			else if( c < 0x800 ) { text += (char)( 0xC0 | ( c >> 6 ) ); text += (char)( 0x80 | ( c & 0x3F ) ); }
			else { text += (char)( 0xE0 | ( c >> 12 ) ); text += (char)( 0x80 | ( ( c >> 6 ) & 0x3F ) ); text += (char)( 0x80 | ( c & 0x3F ) ); }
		}
	}
	else
	{
		int p = ( len >= 3 && (unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB && (unsigned char)data[2] == 0xBF ) ? 3 : 0;
		text.assign( data + p, len - p );
	}
	gEngfuncs.COM_FreeFile( data );
	CzLoc_Parse( text );
}

// key without the leading '#'; returns NULL if unknown
const char *Cz_Localize( const char *key )
{
	if( !g_czLocLoaded )
	{
		g_czLocLoaded = true;
		CzLoc_LoadFile( "resource/czeror_english.txt" );
		CzLoc_LoadFile( "resource/valve_english.txt" );
		CzLoc_LoadFile( "resource/gameui_english.txt" );
		gEngfuncs.Con_DPrintf( "cz_loc: %d strings\n", (int)g_czLoc.size() );
	}

	std::map<std::string, std::string>::const_iterator it = g_czLoc.find( CzLower( key ) );
	if( it != g_czLoc.end() )
		return it->second.c_str();

	// fallback: "valve_game_saved" -> "Game saved"
	static std::string s_fallback;
	s_fallback = key;
	if( s_fallback.compare( 0, 6, "valve_" ) == 0 ) s_fallback.erase( 0, 6 );
	for( size_t i = 0; i < s_fallback.size(); i++ )
		if( s_fallback[i] == '_' ) s_fallback[i] = ' ';
	if( !s_fallback.empty() ) s_fallback[0] = (char)toupper( (unsigned char)s_fallback[0] );
	return s_fallback.c_str();
}
