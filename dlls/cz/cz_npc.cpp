#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "player.h"
#include "schedule.h"
#include "saverestore.h"
#include "animation.h"
#include "scripted.h"
#include "studio.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// CZ:DS props and actors on top of the Half-Life monster code.
//
//  item_generic : animated decoration prop.
//  monster_npc  : civilian actor. Plays "idleanim" while idle, fires "usetarget" on +use
//                 (or follows the player, HL scientist style, when there is no usetarget).
//                 Scripted movement and animation use the stock scripted_sequence (CCineMonster).

static int _stricmp_cz(const char *a, const char *b)
{
	for (; *a && *b; a++, b++)
	{
		int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
		if (ca != cb) return ca - cb;
	}
	return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static bool Cz_IsNullName(const char *s)
{
	if (!s || !s[0])
		return true;

	if (strstr(s, "null") || strstr(s, "bogus"))
		return true;

	return !_stricmp_cz(s, "none") || !_stricmp_cz(s, "null") || !_stricmp_cz(s, "_is_null") ||
		!_stricmp_cz(s, "nullnullnull") || !_stricmp_cz(s, "null_null_null_null");
}

// ---------------------------------------------------------------------------
// item_generic
// ---------------------------------------------------------------------------
class CCzItemGeneric: public CBaseAnimating
{
public:
	void Spawn();
	void Precache();
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	int ObjectCaps() { return CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	static TYPEDESCRIPTION m_SaveData[];

	void ApplySequence(const char *name);
	void EXPORT HideIfAppearTarget();
	void EXPORT AnimThink();

	string_t m_sequenceName;
	int m_removeOnUse;
	int m_spawned;
	int m_animated;
};

TYPEDESCRIPTION CCzItemGeneric::m_SaveData[] =
{
	DEFINE_FIELD(CCzItemGeneric, m_sequenceName, FIELD_STRING),
	DEFINE_FIELD(CCzItemGeneric, m_removeOnUse, FIELD_INTEGER),
	DEFINE_FIELD(CCzItemGeneric, m_spawned, FIELD_INTEGER),
	DEFINE_FIELD(CCzItemGeneric, m_animated, FIELD_INTEGER),
};

LINK_ENTITY_TO_CLASS(item_generic, CCzItemGeneric)
IMPLEMENT_SAVERESTORE(CCzItemGeneric, CBaseAnimating)

void CCzItemGeneric::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		m_sequenceName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;

		if (m_spawned)
			ApplySequence(pkvd->szValue);
	}
	else if (FStrEq(pkvd->szKeyName, "removeonuse"))
	{
		m_removeOnUse = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

void CCzItemGeneric::ApplySequence(const char *name)
{
	int seq = -1;

	if (name && name[0])
		seq = LookupSequence(name);

	if (seq < 0)
		seq = 0;

	pev->sequence = seq;
	pev->frame = 0;
	ResetSequenceInfo();
}

void CCzItemGeneric::Precache()
{
	if (!FStringNull(pev->model))
		PRECACHE_MODEL((char *)STRING(pev->model));
}

void CCzItemGeneric::Spawn()
{
	if (FStringNull(pev->model))
	{
		UTIL_Remove(this);
		return;
	}

	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	InitBoneControllers();
	UTIL_SetOrigin(pev, pev->origin);

	ApplySequence(FStringNull(m_sequenceName) ? "idle" : STRING(m_sequenceName));
	m_spawned = 1;

	// the client does not animate a plain prop on its own: advance multi-frame sequences here
	m_animated = 0;
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	if (pmodel)
	{
		studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
		if (pev->sequence >= 0 && pev->sequence < pstudiohdr->numseq)
		{
			mstudioseqdesc_t *pseq = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + pev->sequence;
			m_animated = (pseq->numframes > 1 && pev->framerate > 0.0f) ? 1 : 0;
		}
	}

	// display items that a map reveals later with an env_render ("appear") start hidden
	if (!FStringNull(pev->targetname))
	{
		SetThink(&CCzItemGeneric::HideIfAppearTarget);
		pev->nextthink = gpGlobals->time + 0.3f;
	}
	else if (m_animated)
	{
		SetThink(&CCzItemGeneric::AnimThink);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CCzItemGeneric::AnimThink()
{
	StudioFrameAdvance();

	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

void CCzItemGeneric::HideIfAppearTarget()
{
	CBaseEntity *pRender = NULL;
	while ((pRender = UTIL_FindEntityByClassname(pRender, "env_render")) != NULL)
	{
		if (FStringNull(pRender->pev->target) || !FStrEq(STRING(pRender->pev->target), STRING(pev->targetname)))
			continue;

		if (pRender->pev->renderamt >= 200.0f)
		{
			pev->rendermode = kRenderTransTexture;
			pev->renderamt = 0;
			break;
		}
	}

	if (m_animated)
	{
		SetThink(&CCzItemGeneric::AnimThink);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
		pev->nextthink = 0;
}

void CCzItemGeneric::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_removeOnUse)
		UTIL_Remove(this);
}

// ---------------------------------------------------------------------------
// monster_npc
// ---------------------------------------------------------------------------
#define CZ_NPC_DEFAULT_MODEL "models/npc-scientist.mdl"

class CCzNPC: public CTalkMonster
{
public:
	void Spawn();
	void Precache();
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);

	void SetYawSpeed() { pev->yaw_speed = 90; }
	int Classify() { return CLASS_HUMAN_PASSIVE; }
	int ObjectCaps() { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) { return 0; }
	void HandleAnimEvent(MonsterEvent_t *pEvent) {}
	int ISoundMask() { return 0; }

	void SetActivity(Activity newActivity);
	Schedule_t *GetSchedule();
	void MonsterThink();
	void EXPORT NpcUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void ApplyHeadSkin();
	void ApplyIdleAnim();

	static TYPEDESCRIPTION m_SaveData[];

	string_t m_idleName;
	string_t m_useTarget;
	int m_head;
	int m_idleSeq;
	int m_spawned;
	int m_cantMove;
	float m_lookUntil;
	float m_headYaw;
	void StaticThink();
};

const char *Cz_PickSentence(const char *group);

static void Cz_Say(CBaseEntity *pEnt, const char *group)
{
	const char *snd = Cz_PickSentence(group);
	if (!snd || !pEnt)
		return;
	char path[256];
	snprintf(path, sizeof(path), "%s.wav", snd);
	EMIT_SOUND_DYN2(ENT(pEnt->pev), CHAN_VOICE, path, VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
}

TYPEDESCRIPTION CCzNPC::m_SaveData[] =
{
	DEFINE_FIELD(CCzNPC, m_lookUntil, FIELD_TIME),
	DEFINE_FIELD(CCzNPC, m_headYaw, FIELD_FLOAT),
	DEFINE_FIELD(CCzNPC, m_idleName, FIELD_STRING),
	DEFINE_FIELD(CCzNPC, m_useTarget, FIELD_STRING),
	DEFINE_FIELD(CCzNPC, m_head, FIELD_INTEGER),
	DEFINE_FIELD(CCzNPC, m_idleSeq, FIELD_INTEGER),
	DEFINE_FIELD(CCzNPC, m_spawned, FIELD_INTEGER),
	DEFINE_FIELD(CCzNPC, m_cantMove, FIELD_INTEGER),
};

LINK_ENTITY_TO_CLASS(monster_npc, CCzNPC)
IMPLEMENT_SAVERESTORE(CCzNPC, CTalkMonster)

void CCzNPC::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "idleanim"))
	{
		m_idleName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;

		// trigger_changekeyvalue switches the animation at runtime
		if (m_spawned)
			ApplyIdleAnim();
	}
	else if (FStrEq(pkvd->szKeyName, "usetarget"))
	{
		m_useTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_head = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "cantmove"))
	{
		m_cantMove = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TriggerCondition") ||
		FStrEq(pkvd->szKeyName, "TriggerTarget"))
	{
		pkvd->fHandled = TRUE;
	}
	else
		CTalkMonster::KeyValue(pkvd);
}

void CCzNPC::ApplyHeadSkin()
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	if (!pmodel)
		return;

	studiohdr_t *phdr = (studiohdr_t *)pmodel;

	// bodypart 1 = head
	if (phdr->numbodyparts > 1)
		SetBodygroup(1, m_head);
}

void CCzNPC::ApplyIdleAnim()
{
	m_idleSeq = -1;

	if (!FStringNull(m_idleName) && !Cz_IsNullName(STRING(m_idleName)))
		m_idleSeq = LookupSequence(STRING(m_idleName));

	// force the new animation straight away when the actor is not busy with a script
	if (m_spawned && m_idleSeq >= 0 && m_pCine == NULL && pev->sequence != m_idleSeq)
	{
		pev->sequence = m_idleSeq;
		pev->frame = 0;
		ResetSequenceInfo();
		m_Activity = ACT_IDLE;
		m_IdealActivity = ACT_IDLE;
	}
}

void CCzNPC::SetActivity(Activity newActivity)
{
	if (newActivity == ACT_IDLE && m_idleSeq >= 0)
	{
		if (pev->sequence != m_idleSeq || m_Activity != ACT_IDLE)
		{
			pev->sequence = m_idleSeq;
			pev->frame = 0;
			ResetSequenceInfo();
		}
		m_Activity = ACT_IDLE;
		m_IdealActivity = ACT_IDLE;
		return;
	}

	CTalkMonster::SetActivity(newActivity);
}

void CCzNPC::Precache()
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING(CZ_NPC_DEFAULT_MODEL);

	PRECACHE_MODEL((char *)STRING(pev->model));

	TalkInit();
	CTalkMonster::Precache();
}

void CCzNPC::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 100;
	pev->view_ofs = Vector(0, 0, 50);
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	pev->spawnflags |= SF_MONSTER_GAG; // no idle chatter
	pev->spawnflags &= ~SF_MONSTER_PRISONER;

	ApplyHeadSkin();
	ApplyIdleAnim();

	MonsterInit();
	SetUse(&CCzNPC::NpcUse);

	m_spawned = 1;
	ApplyIdleAnim();
	pev->takedamage = DAMAGE_NO;
}

Schedule_t *CCzNPC::GetSchedule()
{
	if (m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_ALERT)
	{
		if (IsFollowing())
		{
			if (HasConditions(bits_COND_CLIENT_PUSH))
				return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);

			return GetScheduleOfType(SCHED_TARGET_FACE);
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
			return GetScheduleOfType(SCHED_MOVE_AWAY);
	}

	return CBaseMonster::GetSchedule();
}

// Seated / working actors: no AI at all, so the body never turns or gets pushed away;
// only the head (bone controller 0) follows the player.
void CCzNPC::StaticThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->velocity = g_vecZero;

	if (pev->sequence != m_idleSeq)
	{
		pev->sequence = m_idleSeq;
		pev->frame = 0;
		ResetSequenceInfo();
	}
	StudioFrameAdvance();
	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}

	float target = 0;
	CBaseEntity *pPlayer = UTIL_PlayerByIndex(1);
	if (pPlayer && pPlayer->IsAlive() && (pPlayer->pev->origin - pev->origin).Length() < 256.0f)
	{
		Vector dir = pPlayer->pev->origin - pev->origin;
		float yaw = UTIL_VecToYaw(dir) - pev->angles.y;
		while (yaw > 180) yaw -= 360;
		while (yaw < -180) yaw += 360;
		if (yaw > 55) yaw = 55;
		if (yaw < -55) yaw = -55;
		target = yaw;
	}
	if (m_lookUntil > gpGlobals->time && pPlayer)
	{
		Vector dir = pPlayer->pev->origin - pev->origin;
		float yaw = UTIL_VecToYaw(dir) - pev->angles.y;
		while (yaw > 180) yaw -= 360;
		while (yaw < -180) yaw += 360;
		if (yaw > 55) yaw = 55;
		if (yaw < -55) yaw = -55;
		target = yaw;
	}
	float d = target - m_headYaw;
	if (d > 15) d = 15;
	if (d < -15) d = -15;
	m_headYaw += d;
	SetBoneController(0, m_headYaw);
}

void CCzNPC::MonsterThink()
{
	if (m_idleSeq >= 0 && !m_pCine && m_MonsterState != MONSTERSTATE_SCRIPT && m_IdealMonsterState != MONSTERSTATE_SCRIPT && !IsFollowing())
	{
		StaticThink();
		return;
	}

	CTalkMonster::MonsterThink();

	// keep the map-defined idle pose (sitting, typing, ...) whenever nothing else drives the actor
	if (m_idleSeq >= 0 && !m_pCine && !IsFollowing() && !IsMoving() && pev->sequence != m_idleSeq)
	{
		pev->sequence = m_idleSeq;
		pev->frame = 0;
		ResetSequenceInfo();
		m_Activity = ACT_IDLE;
		m_IdealActivity = ACT_IDLE;
	}
}

void CCzNPC::NpcUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// a map-defined use target wins
	if (!FStringNull(m_useTarget) && !Cz_IsNullName(STRING(m_useTarget)))
	{
		FireTargets(STRING(m_useTarget), pActivator, this, USE_TOGGLE, 0);
		return;
	}

	// seated/working actors never follow the player
	if (m_cantMove || m_idleSeq >= 0)
		return;

	// no use target: the actor follows the player (door-opener hostage)
	BOOL was = IsFollowing();
	FollowerUse(pActivator, pCaller, useType, value);
	if (!was && IsFollowing())
		Cz_Say(this, "NPC_OK_AM");
	else if (was && !IsFollowing())
		Cz_Say(this, "NPC_WAIT_AM");
}

void Cz_NpcSpeak(CBaseEntity *pSpeaker, float seconds)
{
	if (pSpeaker && (FClassnameIs(pSpeaker->pev, "monster_npc") || FClassnameIs(pSpeaker->pev, "monster_hostage") || FClassnameIs(pSpeaker->pev, "monster_generic")))
		((CCzNPC *)pSpeaker)->m_lookUntil = gpGlobals->time + seconds;
}

// ---------------------------------------------------------------------------
// info_hostage_rescue: marks the rescue zone (radius 256)
// ---------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(info_hostage_rescue, CPointEntity)

// ---------------------------------------------------------------------------
// monster_hostage: a civilian that follows the player and is "rescued" in a rescue zone
// ---------------------------------------------------------------------------
class CCzHostage: public CCzNPC
{
public:
	void Spawn();
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void MonsterThink();
	void EXPORT HostageUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Rescue(CBaseEntity *pActivator);

	static TYPEDESCRIPTION m_SaveData[];

	string_t m_rescueTarget;
	int m_rescued;
};

TYPEDESCRIPTION CCzHostage::m_SaveData[] =
{
	DEFINE_FIELD(CCzHostage, m_rescueTarget, FIELD_STRING),
	DEFINE_FIELD(CCzHostage, m_rescued, FIELD_INTEGER),
};

LINK_ENTITY_TO_CLASS(monster_hostage, CCzHostage)
IMPLEMENT_SAVERESTORE(CCzHostage, CCzNPC)

void CCzHostage::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "rescuetarget"))
	{
		m_rescueTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "noautoremove") || FStrEq(pkvd->szKeyName, "classtype"))
		pkvd->fHandled = TRUE;
	else
		CCzNPC::KeyValue(pkvd);
}

void CCzHostage::Spawn()
{
	CCzNPC::Spawn();
	SetUse(&CCzHostage::HostageUse);
}

void CCzHostage::HostageUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_rescued)
		return;

	BOOL was = IsFollowing();
	FollowerUse(pActivator, pCaller, useType, value);
	if (!was && IsFollowing())
		Cz_Say(this, "H_HELP_AM");
	else if (was && !IsFollowing())
		Cz_Say(this, "NPC_WAIT_AM");
}

void CCzHostage::Rescue(CBaseEntity *pActivator)
{
	m_rescued = 1;
	StopFollowing(FALSE);
	SetUse(NULL);
	m_idleSeq = -1;
	Cz_Say(this, "H_THANK_AM");

	if (!FStringNull(m_rescueTarget))
		FireTargets(STRING(m_rescueTarget), pActivator, this, USE_TOGGLE, 0);
}

void CCzHostage::MonsterThink()
{
	CCzNPC::MonsterThink();

	if (m_rescued || !IsFollowing())
		return;

	CBaseEntity *pZone = NULL;
	while ((pZone = UTIL_FindEntityByClassname(pZone, "info_hostage_rescue")) != NULL)
	{
		if ((pZone->pev->origin - pev->origin).Length() < 256.0f)
		{
			Rescue(UTIL_PlayerByIndex(1));
			break;
		}
	}
}


// ---------------------------------------------------------------------------
// Rescue zone HUD icon (original: icon_hostagezone shown while the player is within 256 units
// of info_hostage_rescue; HudIcons bit 1024, always green)
// ---------------------------------------------------------------------------
extern int gmsgStatusIcon;
static int s_czZoneIcon[33];
static float s_czZoneNext[33];

static void Cz_SendZoneIcon(CBasePlayer *pPlayer, int on)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev);
		WRITE_BYTE(on ? 1 : 0);
		WRITE_STRING("icon_hostagezone");
		if (on)
		{
			WRITE_BYTE(50);
			WRITE_BYTE(250);
			WRITE_BYTE(50);
		}
	MESSAGE_END();
}

void Cz_PlayerSpawn(CBasePlayer *pPlayer)
{
	int i = pPlayer->entindex();
	if (i >= 1 && i <= 32)
	{
		s_czZoneIcon[i] = 0;
		s_czZoneNext[i] = 0;
	}
}

void Cz_PlayerThink(CBasePlayer *pPlayer)
{
	int i = pPlayer->entindex();
	if (i < 1 || i > 32 || s_czZoneNext[i] > gpGlobals->time)
		return;
	s_czZoneNext[i] = gpGlobals->time + 0.2f;

	int in = 0;
	if (pPlayer->IsAlive())
	{
		CBaseEntity *pZone = NULL;
		while ((pZone = UTIL_FindEntityByClassname(pZone, "info_hostage_rescue")) != NULL)
		{
			if ((pZone->pev->origin - pPlayer->pev->origin).Length() < 256.0f)
			{
				in = 1;
				break;
			}
		}
	}
	if (in != s_czZoneIcon[i])
	{
		s_czZoneIcon[i] = in;
		Cz_SendZoneIcon(pPlayer, in);
	}
}
