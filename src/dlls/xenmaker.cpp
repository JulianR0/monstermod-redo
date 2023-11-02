//=========================================================
// Xen Maker - Sven Co-op's env_xenmaker.
// Spawns a monster with visual/auditive teleportation effects.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cmbase.h"
#include "cmbasemonster.h"
#include "cmbaseextra.h"
#include "monsters.h"

// Xenmaker spawnflags
#define	SF_XENMAKER_TRY_ONCE	1 // only one attempt to spawn each time it is fired
#define	SF_XENMAKER_NO_SPAWN	2 // don't spawn anything, only do effects

extern monster_type_t monster_types[];
extern edict_t* spawn_monster(int monster_type, Vector origin, Vector angles, int spawnflags, pKVD *keyvalue);


// ========================================================
void CMXenMaker::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "monstertype"))
	{
		// Process monster_index
		int mIndex;
		for (mIndex = 0; monster_types[mIndex].name[0]; mIndex++)
		{
			if (strcmp(pkvd->szValue, monster_types[mIndex].name) == 0)
			{
				m_iMonsterIndex = mIndex;
				break; // grab the first entry we find
			}
		}
		if (monster_types[mIndex].name[0] == 0)
		{
			ALERT(at_logged, "[MONSTER] XenMaker - %s is not a valid monster type!\n", pkvd->szValue);
			m_iMonsterIndex = -1;
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flBeamRadius"))
	{
		m_flBeamRadius = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iBeamAlpha"))
	{
		m_iBeamAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iBeamCount"))
	{
		m_iBeamCount = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vBeamColor"))
	{
		UTIL_StringToVector(m_vBeamColor, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flLightRadius"))
	{
		m_flLightRadius = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vLightColor"))
	{
		UTIL_StringToVector(m_vLightColor, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flStartSpriteFramerate"))
	{
		m_flStartSpriteFramerate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flStartSpriteScale"))
	{
		m_flStartSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartSpriteAlpha"))
	{
		m_iStartSpriteAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vStartSpriteColor"))
	{
		UTIL_StringToVector(m_vStartSpriteColor, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flEndSpriteFramerate"))
	{
		m_flEndSpriteFramerate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flEndSpriteScale"))
	{
		m_flEndSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iEndSpriteAlpha"))
	{
		m_iEndSpriteAlpha = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_vEndSpriteColor"))
	{
		UTIL_StringToVector(m_vEndSpriteColor, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CMBaseMonster::KeyValue(pkvd);
}


void CMXenMaker::Spawn()
{
	// likely omitted keyvalue, but it could truly be an alien grunt spawn
	if (m_iMonsterIndex == 0)
	{
		if (!monster_types[0].need_to_precache)
		{
			// monstertype was not defined, it may be intentional if nothing is to spawn here
			if (!FBitSet(pev->spawnflags, SF_XENMAKER_NO_SPAWN))
				ALERT(at_logged, "[MONSTER] Spawned a env_xenmaker entity without a monstertype! targetname: \"%s\"\n", STRING(pev->targetname));
			m_iMonsterIndex = -1;
		}
	}

	pev->solid = SOLID_NOT;
	
	Precache();
	
	SetUse(&CMXenMaker::CyclicUse); // drop one monster each time we fire
	SetThink(&CMXenMaker::SUB_DoNothing);
	
	pev->classname = MAKE_STRING("env_xenmaker");
}

void CMXenMaker::Precache(void)
{
	m_iBeamIndex = PRECACHE_MODELINDEX("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/fexplo1.spr");
	PRECACHE_MODEL("sprites/xflare1.spr");

	PRECACHE_SOUND("debris/beamstart7.wav");
	PRECACHE_SOUND("debris/beamstart2.wav");

	CMBaseMonster::Precache();
	// choosen monster is auto-precached
}

//=========================================================
// StartEffect - spawns the monster and starts the effects
//=========================================================
void CMXenMaker::StartEffect(void)
{
	if (!FBitSet(pev->spawnflags, SF_XENMAKER_NO_SPAWN))
	{
		// monstermaker incorrectly setup
		if (m_iMonsterIndex == -1)
		{
			ALERT(at_console, "[MONSTER] NULL Ent in XenMaker!\n");
			return;
		}

		edict_t *pent;

		Vector mins = pev->origin - Vector(34, 34, 34);
		Vector maxs = pev->origin + Vector(34, 34, 34);

		edict_t *pList[2];
		int count = UTIL_EntitiesInBox(pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER);
		if (!count)
		{
			// Attempt to spawn monster
			pent = spawn_monster(m_iMonsterIndex, pev->origin, pev->angles, 0, NULL);
			if (pent == NULL)
			{
				ALERT(at_console, "[MONSTER] XenMaker - failed to spawn monster! targetname: \"%s\"\n", STRING(pev->targetname));
			}
			else
				pent->v.spawnflags |= SF_MONSTER_FADECORPSE;
		}
		else if (!FBitSet(pev->spawnflags, SF_XENMAKER_TRY_ONCE))
		{
			// wait until spawnpoint is clear
			pev->nextthink = gpGlobals->time + 1;
			SetUse(NULL);
			SetThink(&CMXenMaker::RetryThink);
			return; // don't do effects
		}
	}

	// BEAM EFFECT
	for (int beam = 0; beam < m_iBeamCount; beam++)
	{
		SpawnBeam();
	}
	
	// LIGHT EFFECT
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_BYTE((int)(m_flLightRadius / 10));
	WRITE_BYTE((int)m_vLightColor.x);
	WRITE_BYTE((int)m_vLightColor.y);
	WRITE_BYTE((int)m_vLightColor.z);
	WRITE_BYTE(10); // life
	WRITE_BYTE(0); // decay rate
	MESSAGE_END();

	// SPRITE EFFECT
	CMSprite *pSprite = CMSprite::SpriteCreate("sprites/fexplo1.spr", pev->origin, FALSE);
	if (pSprite)
	{
		pSprite->SetScale(m_flStartSpriteScale);
		pSprite->SetTransparency(kRenderGlow, (int)m_vStartSpriteColor.x, (int)m_vStartSpriteColor.y, (int)m_vStartSpriteColor.z, m_iStartSpriteAlpha, kRenderFxNoDissipation);
		pSprite->AnimateAndDie(m_flStartSpriteFramerate);
	}

	// SOUND EFFECT
	EMIT_SOUND_DYN(ENT(pev), CHAN_AUTO, "debris/beamstart7.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	pev->nextthink = gpGlobals->time + 0.5;
	SetUse(NULL);
	SetThink(&CMXenMaker::MiddleEffect);
}

//=========================================================
// MiddleEffect - second set of effects
//=========================================================
void CMXenMaker::MiddleEffect(void)
{
	// SPRITE EFFECT
	CMSprite *pSprite = CMSprite::SpriteCreate("sprites/xflare1.spr", pev->origin, FALSE);
	if (pSprite)
	{
		pSprite->SetScale(m_flEndSpriteScale);
		pSprite->SetTransparency(kRenderGlow, (int)m_vEndSpriteColor.x, (int)m_vEndSpriteColor.y, (int)m_vEndSpriteColor.z, m_iEndSpriteAlpha, kRenderFxNoDissipation);
		pSprite->AnimateAndDie(m_flEndSpriteFramerate);
	}

	pev->nextthink = gpGlobals->time + 0.5;
	SetThink(&CMXenMaker::EndEffect);
}

//=========================================================
// EndEffect - final set of effects
//=========================================================
void CMXenMaker::EndEffect(void)
{
	// SOUND EFFECT
	EMIT_SOUND_DYN(ENT(pev), CHAN_AUTO, "debris/beamstart2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	SetUse(&CMXenMaker::CyclicUse);
	SetThink(&CMXenMaker::SUB_DoNothing);
}

//=========================================================
// CyclicUse - drops one monster from the xen maker
// each time we call this.
//=========================================================
void CMXenMaker::CyclicUse(edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value)
{
	StartEffect();
}

//=========================================================
// RetryThink - try spawning again if spawn was obstructed
//=========================================================
void CMXenMaker::RetryThink(void)
{
	SetUse(&CMXenMaker::CyclicUse);
	SetThink(&CMXenMaker::SUB_DoNothing);

	StartEffect();
}

//=========================================================
// SpawnBeam - calculates beam end position and creates it.
// starting position is the origin of the xenmaker itself.
//=========================================================
void CMXenMaker::SpawnBeam(void)
{
	// CLightning::RandomArea
	for (int iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecSrc = pev->origin;

		Vector vecDir1 = Vector(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecDir1 * m_flBeamRadius, ignore_monsters, ENT(pev), &tr1);

		if (tr1.flFraction == 1.0)
			continue;

		Vector vecDir2;
		do
		{
			vecDir2 = Vector(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
		} while (DotProduct(vecDir1, vecDir2) > 0);
		vecDir2 = vecDir2.Normalize();
		TraceResult tr2;
		UTIL_TraceLine(vecSrc, vecSrc + vecDir2 * m_flBeamRadius, ignore_monsters, ENT(pev), &tr2);

		if (tr2.flFraction == 1.0)
			continue;

		if ((tr1.vecEndPos - tr2.vecEndPos).Length() < m_flBeamRadius * 0.1)
			continue;

		UTIL_TraceLine(tr1.vecEndPos, tr2.vecEndPos, ignore_monsters, ENT(pev), &tr2);

		if (tr2.flFraction != 1.0)
			continue;

		// CLightning::Zap
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_COORD(tr1.vecEndPos.x);
		WRITE_COORD(tr1.vecEndPos.y);
		WRITE_COORD(tr1.vecEndPos.z);
		WRITE_COORD(tr2.vecEndPos.x);
		WRITE_COORD(tr2.vecEndPos.y);
		WRITE_COORD(tr2.vecEndPos.z);
		WRITE_SHORT(m_iBeamIndex);
		WRITE_BYTE(0); // starting frame
		WRITE_BYTE(10); // framerate
		WRITE_BYTE(10); // life
		WRITE_BYTE(16); // width
		WRITE_BYTE(64); // noise
		WRITE_BYTE((int)m_vBeamColor.x);
		WRITE_BYTE((int)m_vBeamColor.y);
		WRITE_BYTE((int)m_vBeamColor.z);
		WRITE_BYTE(m_iBeamAlpha);
		WRITE_BYTE(15); // speed
		MESSAGE_END();
		break;
	}
}
