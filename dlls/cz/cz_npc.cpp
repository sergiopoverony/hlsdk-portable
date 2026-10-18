#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
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
	void EXPORT NpcUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void ApplyHeadSkin();
	void ApplyIdleAnim();

	static TYPEDESCRIPTION m_SaveData[];

	string_t m_idleName;
	string_t m_useTarget;
	int m_head;
	int m_idleSeq;
	int m_spawned;
};

TYPEDESCRIPTION CCzNPC::m_SaveData[] =
{
	DEFINE_FIELD(CCzNPC, m_idleName, FIELD_STRING),
	DEFINE_FIELD(CCzNPC, m_useTarget, FIELD_STRING),
	DEFINE_FIELD(CCzNPC, m_head, FIELD_INTEGER),
	DEFINE_FIELD(CCzNPC, m_idleSeq, FIELD_INTEGER),
	DEFINE_FIELD(CCzNPC, m_spawned, FIELD_INTEGER),
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
	else if (FStrEq(pkvd->szKeyName, "cantmove") || FStrEq(pkvd->szKeyName, "TriggerCondition") ||
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

void CCzNPC::NpcUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// a map-defined use target wins
	if (!FStringNull(m_useTarget) && !Cz_IsNullName(STRING(m_useTarget)))
	{
		FireTargets(STRING(m_useTarget), pActivator, this, USE_TOGGLE, 0);
		return;
	}

	// no use target: the actor follows the player (door-opener hostage)
	FollowerUse(pActivator, pCaller, useType, value);
}
