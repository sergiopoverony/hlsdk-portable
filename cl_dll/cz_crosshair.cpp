// CZ:DS: Counter-Strike style dynamic crosshair and sniper scope overlay.
// Ported in spirit from Velaron/cs16-client (cl_dll/ammo.cpp DrawCrosshair/GetCrosshairGap, hud/sniperscope.cpp).
// No sprites needed: everything is drawn with filled rectangles.
#include <math.h>
#include <stdio.h>
#include "hud.h"
#include "cl_util.h"
#include "ammo.h"
#include "weapon_ids.h"

int g_czOnGround = 1;
int g_czDuck = 0;

static cvar_t *cz_xhair = nullptr;
static cvar_t *cz_xhair_color = nullptr;
static cvar_t *cz_xhair_size = nullptr;
static cvar_t *cz_xhair_thick = nullptr;
static cvar_t *cz_xhair_dyn = nullptr;
static cvar_t *cz_xhair_dot = nullptr;
static cvar_t *cz_scope_line = nullptr;

static void CzCrosshair_Init()
{
	if( cz_xhair )
		return;
	cz_xhair = CVAR_CREATE( "cz_xhair", "1", FCVAR_ARCHIVE );
	cz_xhair_color = CVAR_CREATE( "cz_xhair_color", "50 250 50 200", FCVAR_ARCHIVE );
	cz_xhair_size = CVAR_CREATE( "cz_xhair_size", "5", FCVAR_ARCHIVE );
	cz_xhair_thick = CVAR_CREATE( "cz_xhair_thick", "1", FCVAR_ARCHIVE );
	cz_xhair_dyn = CVAR_CREATE( "cz_xhair_dynamic", "1", FCVAR_ARCHIVE );
	cz_xhair_dot = CVAR_CREATE( "cz_xhair_dot", "0", FCVAR_ARCHIVE );
	cz_scope_line = CVAR_CREATE( "cz_scope_line", "1", FCVAR_ARCHIVE );
}


// minGap/deltaGap per weapon, values taken from cs16-client GetCrosshairGap
static void CzGapParams( int id, float &minGap, float &deltaGap, bool &rifle )
{
	rifle = false;
	switch( id )
	{
	case WEAPON_USP:
	case WEAPON_HEGRENADE:
	case WEAPON_FLASHBANG:
	case WEAPON_SMOKEGRENADE:
		minGap = 8; deltaGap = 3; break;
	case WEAPON_AUG:
		minGap = 3; deltaGap = 3; rifle = true; break;
	case WEAPON_M4A1:
		minGap = 4; deltaGap = 3; rifle = true; break;
	case WEAPON_SHOTGUN:
	case WEAPON_SHOTGUN2:
		minGap = 8; deltaGap = 6; break;
	default:
		minGap = 4; deltaGap = 3; break;
	}
}

static float CzGetGap( WEAPON *pw )
{
	static float gap = 0.0f;
	static int lastClip = -1;
	static int lastId = -1;
	static float prevTime = 0.0f;

	float minGap, deltaGap;
	bool rifle;
	CzGapParams( pw->iId, minGap, deltaGap, rifle );

	const float baseMin = minGap;
	if( cz_xhair_dyn->value )
	{
		const float speed = sqrtf( gHUD.m_velocity.x * gHUD.m_velocity.x + gHUD.m_velocity.y * gHUD.m_velocity.y );
		if( !g_czOnGround )
			minGap *= 2.0f;
		else if( g_czDuck )
			minGap *= 0.5f;
		else if( speed > ( rifle ? 140.0f : 0.0f ) + 10.0f )
			minGap *= 1.5f;
		minGap = Q_max( minGap, baseMin * 0.5f );
	}

	const float now = gEngfuncs.GetClientTime();
	if( prevTime > now )
		prevTime = now;
	const float dt = now - prevTime;
	prevTime = now;

	// shot detection: clip decreased while the same weapon is held
	bool fired = false;
	if( pw->iId == lastId && pw->iMaxClip > 0 && lastClip >= 0 && pw->iClip < lastClip )
		fired = true;
	lastId = pw->iId;
	lastClip = pw->iClip;

	if( fired )
	{
		gap += deltaGap;
		if( gap > 15.0f )
			gap = 15.0f;
	}
	else
	{
		// decay as if running at 100 fps (same formula as CS)
		gap -= ( 100.0f * dt ) * ( 0.013f * gap + 0.1f );
	}
	if( gap < minGap )
		gap = minGap;
	return gap;
}

static int Clamp255( int v ) { return v < 0 ? 0 : ( v > 255 ? 255 : v ); }

static void CzGetColor( int &r, int &g, int &b, int &a )
{
	int c[4] = { 50, 250, 50, 200 };
	if( sscanf( cz_xhair_color->string, "%d %d %d %d", &c[0], &c[1], &c[2], &c[3] ) < 3 )
	{
		c[0] = 50; c[1] = 250; c[2] = 50; c[3] = 200;
	}
	r = Clamp255( c[0] );
	g = Clamp255( c[1] );
	b = Clamp255( c[2] );
	a = Clamp255( c[3] );
}

static void CzDrawScope()
{
	const int W = ScreenWidth;
	const int H = ScreenHeight;
	const float cx = W * 0.5f;
	const float cy = H * 0.5f;
	const float R = H * 0.5f;

	// black outside of the circle, drawn in horizontal strips
	const int step = 2;
	for( int y = 0; y < H; y += step )
	{
		const float dy = ( y + step * 0.5f ) - cy;
		float w = 0.0f;
		if( fabsf( dy ) < R )
			w = sqrtf( R * R - dy * dy );
		const int left = (int)( cx - w );
		const int right = (int)( cx + w );
		if( left > 0 )
			CHud::Renderer().FillRGBA( 0, y, left, step, 0, 0, 0, 255 );
		if( right < W )
			CHud::Renderer().FillRGBA( right, y, W - right, step, 0, 0, 0, 255 );
	}

	if( cz_scope_line->value )
	{
		const int t = Q_max( 1, H / 540 );
		CHud::Renderer().FillRGBA( (int)( cx - R ), (int)( cy - t / 2 ), (int)( 2 * R ), t, 0, 0, 0, 255 );
		CHud::Renderer().FillRGBA( (int)( cx - t / 2 ), 0, t, H, 0, 0, 0, 255 );
	}
}

void CZ_DrawCrosshair( float time, WEAPON *pw )
{
	CzCrosshair_Init();

	if( !pw || gHUD.m_fPlayerDead )
		return;
	if( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;
	if( gEngfuncs.IsSpectateOnly() )
		return;

	// zoomed (AUG etc): sprite crosshair from weapon_*.txt (zoom), no dynamic lines.
	// Sniper scope (FOV <= 40) is drawn by CZ_DrawScopeOverlay, so no crosshair here.
	if( gHUD.m_iFOV <= 40 )
	{
		wrect_t n = { 0, 0, 0, 0 };
		gEngfuncs.pfnSetCrosshair( 0, n, 0, 0, 0 );
		return;
	}
	if( gHUD.m_iFOV < 90 && pw->hZoomedCrosshair )
	{
		gEngfuncs.pfnSetCrosshair( pw->hZoomedCrosshair, pw->rcZoomedCrosshair, 255, 255, 255 );
		return;
	}
	{
		wrect_t n = { 0, 0, 0, 0 };
		gEngfuncs.pfnSetCrosshair( 0, n, 0, 0, 0 );
	}

	if( !cz_xhair->value )
		return;

	const float scale = (float)ScreenHeight / 480.0f;
	int gap = (int)( CzGetGap( pw ) * scale + 0.5f );
	int length = (int)( cz_xhair_size->value * scale + 0.5f );
	int thick = (int)( cz_xhair_thick->value * scale + 0.5f );
	if( thick < 1 )
		thick = 1;
	if( length < 1 )
		length = 1;

	const int cx = ScreenWidth / 2;
	const int cy = ScreenHeight / 2;
	int r, g, b, a;
	CzGetColor( r, g, b, a );

	auto Fill = []( int x, int y, int w, int h, int cr, int cg, int cb, int ca )
	{
		CHud::Renderer().FillRGBA( x, y, w, h, cr, cg, cb, ca );
	};

	const int t2 = thick / 2;
	// north, south, west, east
	Fill( cx - t2, cy - gap - length, thick, length, r, g, b, a );
	Fill( cx - t2, cy + gap, thick, length, r, g, b, a );
	Fill( cx - gap - length, cy - t2, length, thick, r, g, b, a );
	Fill( cx + gap, cy - t2, length, thick, r, g, b, a );
	if( cz_xhair_dot->value )
		Fill( cx - t2, cy - t2, thick, thick, r, g, b, a );
}

// Called from CHud::Redraw every frame, independent of the ammo HUD element.
void CZ_DrawScopeOverlay()
{
	CzCrosshair_Init();
	if( gHUD.m_iFOV > 40 || gHUD.m_fPlayerDead || gEngfuncs.IsSpectateOnly() )
		return;
	if( gHUD.m_iFOV <= 0 )
		return;
	static int s_logged = 0;
	if( !s_logged )
	{
		s_logged = 1;
		gEngfuncs.Con_DPrintf( "CZ scope overlay: fov %d\n", gHUD.m_iFOV );
	}
	CzDrawScope();
}
