#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "animation.h"
#include <string.h>
#include <stdlib.h>

// CZ:DS soldiers (monster_<team>_<faction>_<weapon>): static for now, the AI comes later.
// models/seal.mdl: bodygroup 1 = head, bodygroup 2 = weapon
//   0 mp5, 1 shotgun, 2 law, 3 scout, 4 m4a1, 5 usp, 6 m60, 7 mac-10, 8 blank; skin 3 = SWAT uniform.

struct CzSoldierWeapon
{
	const char *name;
	int body;
};

static const CzSoldierWeapon g_czWeapons[] =
{
	{ "pistol", 5 }, { "smg", 7 }, { "mp5", 0 }, { "mp", 0 }, { "shotgun", 1 }, { "assaultrifle", 4 },
	{ "sniperrifle", 3 }, { "machinegun", 6 }, { "grenader", 0 }, { "grenade", 0 }, { "law", 2 },
	{ "melee", 8 }, { "kamakazi", 8 },
};

class CCzSoldier: public CBaseAnimating
{
public:
	void Spawn();
	void Precache();
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	int ObjectCaps() { return CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];
	void EXPORT AnimThink();
	void Settle();
	void PlayAnim(const char *name);
	void React(int type);
	const char *IdleName();

	int m_head;
	int m_bMade;
	int m_react;
	int m_step;
	float m_stepTime;
	EHANDLE m_hPlat;
	Vector m_platOfs;
};

TYPEDESCRIPTION CCzSoldier::m_SaveData[] =
{
	DEFINE_FIELD(CCzSoldier, m_head, FIELD_INTEGER),
	DEFINE_FIELD(CCzSoldier, m_bMade, FIELD_INTEGER),
	DEFINE_FIELD(CCzSoldier, m_react, FIELD_INTEGER),
	DEFINE_FIELD(CCzSoldier, m_step, FIELD_INTEGER),
	DEFINE_FIELD(CCzSoldier, m_stepTime, FIELD_TIME),
	DEFINE_FIELD(CCzSoldier, m_hPlat, FIELD_EHANDLE),
	DEFINE_FIELD(CCzSoldier, m_platOfs, FIELD_VECTOR),
};

IMPLEMENT_SAVERESTORE(CCzSoldier, CBaseAnimating)

LINK_ENTITY_TO_CLASS(monster_ct_swat_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_swat_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gign_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_gsg9_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_sas_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_ct_spetsnaz_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_asian_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_desert_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_jungle_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_russian_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_pistol, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_smg, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_mp, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_mp5, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_assaultrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_sniperrifle, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_machinegun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_grenader, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_law, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_melee, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_kamakazi, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_arctic_shotgun_shotgun, CCzSoldier)
LINK_ENTITY_TO_CLASS(monster_terrorist_urban_grenade, CCzSoldier)

void CCzSoldier::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "cantmove") || FStrEq(pkvd->szKeyName, "usetarget") ||
		FStrEq(pkvd->szKeyName, "TriggerTarget") || FStrEq(pkvd->szKeyName, "TriggerCondition") ||
		FStrEq(pkvd->szKeyName, "alertradius") || FStrEq(pkvd->szKeyName, "alertable") ||
		FStrEq(pkvd->szKeyName, "classtype"))
	{
		pkvd->fHandled = true; // AI keys, not used yet
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

void CCzSoldier::Precache()
{
	PRECACHE_MODEL("models/seal.mdl");
	PRECACHE_MODEL("models/terror.mdl");
	PRECACHE_SOUND("ct/ct_blinded_am.wav");
	PRECACHE_SOUND("ct/ct_incoming_am.wav");
	PRECACHE_SOUND("ct/ct_damnsmoke_am.wav");
	PRECACHE_SOUND("t/t_coughc_am.wav");
}

void CCzSoldier::Spawn()
{
	Precache();

	const char *cls = STRING(pev->classname);
	const bool isTerror = strstr(cls, "terrorist") != NULL;
	const char *mdl = isTerror ? "models/terror.mdl" : "models/seal.mdl";
	pev->model = MAKE_STRING(mdl);
	SET_MODEL(ENT(pev), mdl);

	m_bMade = pev->iuser4 == 0x435A; // set by the monstermaker
	pev->movetype = MOVETYPE_NONE;
	pev->solid = m_bMade ? SOLID_NOT : SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_NO;
	pev->health = 100;
	pev->skin = (!isTerror && strstr(cls, "_swat_")) ? 3 : 0;

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetOrigin(pev, pev->origin);
	if (!m_bMade)
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR(ENT(pev));
	}

	InitBoneControllers();

	int body = 4;
	const char *p = strrchr(cls, '_');
	if (p)
	{
		// "shotgun_shotgun" ends with the weapon too
		for (size_t i = 0; i < sizeof(g_czWeapons) / sizeof(g_czWeapons[0]); i++)
		{
			if (!strcmp(p + 1, g_czWeapons[i].name))
			{
				body = g_czWeapons[i].body;
				break;
			}
		}
	}

	SetBodygroup(1, m_head);
	SetBodygroup(2, body);

	int seq = LookupSequence("idle1");
	if (seq < 0)
		seq = LookupSequence("combatidle");
	if (seq < 0)
		seq = 0;

	pev->sequence = seq;
	pev->frame = 0;
	ResetSequenceInfo();

	m_react = 0;
	Settle();
	ALERT(at_console, "soldier: %s made=%d modelindex=%d origin %.0f %.0f %.0f sequence %d\n", cls, m_bMade, pev->modelindex, pev->origin.x, pev->origin.y, pev->origin.z, pev->sequence);
	SetThink(&CCzSoldier::AnimThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

static cvar_t cz_cage_drop = { "cz_cage_drop", "100", 0, 100.0f, nullptr };
static cvar_t cz_grenade_react = { "cz_grenade_react_radius", "250", 0, 250.0f, nullptr };

// cage soldier: ride the func_door that contains the maker spot
void CCzSoldier::Settle()
{
	m_hPlat = nullptr;
	if (!m_bMade)
		return;
	static bool reg = false;
	if (!reg)
	{
		reg = true;
		CVAR_REGISTER(&cz_cage_drop);
		CVAR_REGISTER(&cz_grenade_react);
	}
	CBaseEntity *best = nullptr;
	float bestTop = -99999.0f;
	CBaseEntity *e = nullptr;
	while ((e = UTIL_FindEntityByClassname(e, "func_door")) != nullptr)
	{
		if (e->pev->targetname && !strncmp(STRING(e->pev->targetname), "gunrange_door", 13))
			continue;
		Vector mn = e->pev->absmin, mx = e->pev->absmax;
		if (pev->origin.x < mn.x - 8 || pev->origin.x > mx.x + 8 || pev->origin.y < mn.y - 8 || pev->origin.y > mx.y + 8)
			continue;
		if (pev->origin.z < mn.z - 24 || pev->origin.z > mx.z + 24)
			continue;
		if (mx.z > bestTop && mx.z <= pev->origin.z + 24)
		{
			bestTop = mx.z;
			best = e;
		}
	}
	if (best)
	{
		m_hPlat = best;
		m_platOfs = pev->origin - best->pev->origin;
		ALERT(at_console, "soldier: rides %s (%s) at %.0f %.0f %.0f\n", STRING(best->pev->classname), STRING(best->pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
	}
	else
		ALERT(at_console, "soldier: maker spawn at %.0f %.0f %.0f, no door under\n", pev->origin.x, pev->origin.y, pev->origin.z);
}

const char *CCzSoldier::IdleName()
{
	return LookupSequence("idle1") >= 0 ? "idle1" : "combatidle";
}

void CCzSoldier::PlayAnim(const char *name)
{
	int seq = LookupSequence(name);
	if (seq < 0)
		return;
	pev->sequence = seq;
	pev->frame = 0;
	ResetSequenceInfo();
}

void CCzSoldier::React(int type)
{
	m_react = type;
	m_step = 0;
	if (type == 1)
	{
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "ct/ct_blinded_am.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		PlayAnim("smflinch");
		m_stepTime = gpGlobals->time + 6.0f;
	}
	else
	{
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "ct/ct_incoming_am.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		PlayAnim("cower");
		m_stepTime = gpGlobals->time + 2.5f;
	}
}

void CCzSoldier::AnimThink()
{
	StudioFrameAdvance();

	if (m_react == 1)
	{
		if (gpGlobals->time >= m_stepTime)
		{
			m_react = 0;
			PlayAnim(IdleName());
		}
		else if (m_fSequenceFinished)
		{
			static const char *flinch[] = { "smflinch", "leftarmflinch", "rightarmflinch", "leftlegsmflinch", "rightlegsmflinch" };
			PlayAnim(flinch[RANDOM_LONG(0, 4)]);
		}
	}
	else if (m_react == 2)
	{
		if (m_step == 0 && gpGlobals->time >= m_stepTime)
		{
			m_step = 1;
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, "ct/ct_damnsmoke_am.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			PlayAnim("choking");
			m_stepTime = gpGlobals->time + 1.5f;
		}
		else if (m_step == 1 && gpGlobals->time >= m_stepTime)
		{
			m_step = 2;
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, "t/t_coughc_am.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			m_stepTime = gpGlobals->time + 2.5f;
		}
		else if (m_step == 2 && gpGlobals->time >= m_stepTime)
		{
			m_react = 0;
			PlayAnim(IdleName());
		}
	}
	else if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}

	CBaseEntity *plat = m_hPlat;
	if (plat)
	{
		Vector want = plat->pev->origin + m_platOfs;
		want.z -= cz_cage_drop.value;
		if ((want - pev->origin).Length() > 0.1f)
			UTIL_SetOrigin(pev, want);
		pev->nextthink = gpGlobals->time + 0.02f;
		return;
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}

bool Cz_IsSoldierClass(const char *cls)
{
	return cls && (!strncmp(cls, "monster_ct_", 11) || !strncmp(cls, "monster_terrorist_", 18));
}

// called by CMonsterMaker: create a soldier at the maker's spot, the child shares the maker's netname as targetname
CBaseEntity *Cz_MakeSoldier(const char *cls, const Vector &origin, const Vector &angles, string_t name)
{
	edict_t *pent = CREATE_NAMED_ENTITY(MAKE_STRING(cls));
	if (FNullEnt(pent))
		return NULL;
	entvars_t *pevNew = VARS(pent);
	pevNew->origin = origin;
	pevNew->angles = angles;
	pevNew->iuser4 = 0x435A;
	if (!FStringNull(name))
		pevNew->targetname = name;
	DispatchSpawn(pent);
	return CBaseEntity::Instance(pent);
}

// Reaction of soldiers to grenades (kind: 1 flash, 2 smoke)
void Cz_GrenadeEvent(int kind, const Vector& origin, CBaseEntity* owner)
{
	(void)owner;
	float radius = cz_grenade_react.value > 0 ? cz_grenade_react.value : 250.0f;
	CBaseEntity *e = NULL;
	while ((e = UTIL_FindEntityInSphere(e, origin, radius)) != NULL)
	{
		if (!Cz_IsSoldierClass(STRING(e->pev->classname)))
			continue;
		CCzSoldier *sol = (CCzSoldier *)e;
		if (sol->m_bMade)
			sol->React(kind);
	}
}
