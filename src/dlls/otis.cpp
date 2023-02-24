// HUGE thanks to DrBeef for his hlsdk-xash3d-opfor repository!

/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"weapons.h"

#define		NUM_OTIS_HEADS		2 // heads available for otis model

#define		GUN_GROUP			1
#define		HEAD_GROUP			2

#define		HEAD_HAIR			0
#define		HEAD_BALD			1

#define		GUN_NONE			0
#define		GUN_EAGLE			1
#define		GUN_DONUT			2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is Otis dying for scripted sequences?
#define		OTIS_AE_DRAW		( 2 )
#define		OTIS_AE_SHOOT		( 3 )
#define		OTIS_AE_HOLSTER		( 4 )

#define OTIS_BODY_GUNHOLSTERED	0
#define OTIS_BODY_GUNDRAWN		1
#define OTIS_BODY_DONUT			2

//=========================================================
// ALertSound - otis says "Freeze!"
//=========================================================
void CMOtis::AlertSound(void)
{
	if (m_hEnemy != 0)
	{
		if (FOkToSpeak())
		{
			PlaySentence("OT_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy otis is facing.
//=========================================================
void CMOtis::BarneyFirePistol(void)
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_357);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CMOtis::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case OTIS_AE_SHOOT:
		BarneyFirePistol();
		break;
	
	case OTIS_AE_DRAW:
		// otis' bodygroup switches here so he can pull gun from holster
		// pev->body = OTIS_BODY_GUNDRAWN;
		SetBodygroup( GUN_GROUP, GUN_EAGLE );
		m_fGunDrawn = TRUE;
		break;

	case OTIS_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		// pev->body = OTIS_BODY_GUNHOLSTERED;
		SetBodygroup( GUN_GROUP, GUN_NONE );
		m_fGunDrawn = FALSE;
		break;

	default:
		CMBarney::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CMOtis::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/otis.mdl"));
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.otisHealth;
	pev->view_ofs		= Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster
	m_fGunDrawn = FALSE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	// Make sure hands are white.
	pev->skin = 0;

	// Select a random head.
	if (head == -1)
	{
		SetBodygroup(HEAD_GROUP, RANDOM_LONG(0, NUM_OTIS_HEADS - 1));
	}
	else
	{
		SetBodygroup(HEAD_GROUP, head);
	}

	if (bodystate == -1)
	{
		SetBodygroup(GUN_GROUP, RANDOM_LONG(OTIS_BODY_GUNHOLSTERED, OTIS_BODY_GUNDRAWN)); // don't random donut
	}
	else
	{
		SetBodygroup(GUN_GROUP, bodystate);
	}

	MonsterInit();
	
	pev->classname = MAKE_STRING( "monster_otis" );
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Otis" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMOtis::Precache()
{
	PRECACHE_MODEL("models/otis.mdl");

	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");

	// every new otis must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CMTalkMonster::Precache();
}

// Init talk data
void CMOtis::TalkInit()
{
	CMTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]		= "OT_ANSWER";
	m_szGrp[TLK_QUESTION]	= "OT_QUESTION";
	m_szGrp[TLK_IDLE]		= "OT_IDLE";
	m_szGrp[TLK_STARE]		= "OT_STARE";
	m_szGrp[TLK_USE]		= "OT_OK";
	m_szGrp[TLK_UNUSE]		= "OT_WAIT";
	m_szGrp[TLK_STOP]		= "OT_STOP";

	m_szGrp[TLK_NOSHOOT]	= "OT_SCARED";
	m_szGrp[TLK_HELLO]		= "OT_HELLO";

	m_szGrp[TLK_PLHURT1]	= "!OT_CUREA";
	m_szGrp[TLK_PLHURT2]	= "!OT_CUREB";
	m_szGrp[TLK_PLHURT3]	= "!OT_CUREC";

	m_szGrp[TLK_PHELLO]		= NULL;
	m_szGrp[TLK_PIDLE]		= NULL;	
	m_szGrp[TLK_PQUESTION]	= NULL;

	m_szGrp[TLK_SMELL]		= "OT_SMELL";

	m_szGrp[TLK_WOUND]		= "OT_WOUND";
	m_szGrp[TLK_MORTAL]		= "OT_MORTAL";

	// get voice for head - just one otis voice for now
	m_voicePitch = 100;
}


int CMOtis::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CMTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT))
	{
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( ( m_hEnemy != NULL ) && UTIL_IsPlayer(m_hEnemy) )
		{
			Remember( bits_MEMORY_PROVOKED );
		}
	}

	return ret;
}

void CMOtis::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10: // Otis wears no helmet, so do not prevent taking headshot damage.
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	default:
		break;
	}
	CMTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CMOtis::Killed(entvars_t *pevAttacker, int iGib)
{
	if (GetBodygroup(GUN_GROUP) != OTIS_BODY_GUNHOLSTERED)
	{
		// drop the gun!
		SetBodygroup(GUN_GROUP, OTIS_BODY_GUNHOLSTERED);
	}
	
	CMTalkMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CMOtis::GetSchedule(void)
{
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("OT_KILL", 4, VOL_NORM, ATTN_NORM);
	}
	
	return CMBarney::GetSchedule();
}

void CMOtis::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		head = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CMBarney::KeyValue(pkvd);
}
