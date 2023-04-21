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
// Robo Grunt
//=========================================================

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"cmtalkmonster.h"
#include	"effects.h"
#include	"explode.h"
#include	"customentity.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	RGRUNT_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!

// Weapon flags
#define RGRUNT_9MMAR				(1 << 0)
#define RGRUNT_HANDGRENADE			(1 << 1)
#define RGRUNT_GRENADELAUNCHER		(1 << 2)
#define RGRUNT_SHOTGUN				(1 << 3)

// Body groups
#define GUN_GROUP					2

// Gun values
#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_NONE					2

// How many sparks to emit when low on health
#define RGRUNT_MAX_SPARKS			5

//=========================================================
// These sounds are muted for Robo Grunts
//=========================================================
BOOL CMRGrunt::FOkToSpeak(void)
{
	return FALSE;
}

void CMRGrunt::IdleSound(void)
{
}

void CMRGrunt::PainSound(void)
{
}

//=========================================================
// DeathSound
//=========================================================
void CMRGrunt::DeathSound(void)
{
	switch ( RANDOM_LONG(0,1) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "turret/tu_die.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "turret/tu_die2.wav", 1, ATTN_IDLE );	
		break;
	}
	
	// Use this spot to activate the explosion
	int duration = RANDOM_LONG( 3, 9 );
	
	/* Smoke effect */
	// variable smoke balls
	// 1.7X normal size
	// 0 radial distribution
	// instant start
	SmokeCreate( pev->origin, duration * 7, 17, 0, 0 );
	
	// "Gib"
	pev->nextthink = gpGlobals->time + duration;
	SetThink( &CMRGrunt::StartGib );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMRGrunt::Classify(void)
{
	if ( m_iClassifyOverride == -1 ) // helper
		return CLASS_NONE;
	else if ( m_iClassifyOverride > 0 )
		return m_iClassifyOverride; // override
	
	return CLASS_HUMAN_MILITARY;
}

//=========================================================
// Killed - Explode a few seconds after death
//=========================================================
void CMRGrunt::Killed(entvars_t *pevAttacker, int iGib)
{
	// Turn off electricity
	if ( m_flActiveDischarge != 0 )
	{
		pev->renderfx = kRenderFxNone;
		m_flActiveDischarge = 0;
	}

	// Disallow this monster to fade away, need to keep it around for the explosion
	pev->owner = 0;
	pev->spawnflags &= ~SF_MONSTER_FADECORPSE;
	pev->solid = SOLID_NOT; // stop interacting with the world

	CMBaseMonster::Killed(pevAttacker, iGib);
}

void CMRGrunt::StartGib(void)
{
	// derp
	GibMonster();
}

//=========================================================
// GibMonster - Boom!
//=========================================================
void CMRGrunt::GibMonster()
{
	// Don't call this more times than needed
	if ( pev->iuser1 != 0 )
		return;
	pev->iuser1 = 1;

	Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

	// Explosion
	ExplosionCreate( vecSpot, g_vecZero, ENT(pev), 128, 0, 0 );
	
	// Wreckage
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL );

		// position
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );

		// size
		WRITE_COORD( 96 );
		WRITE_COORD( 96 );
		WRITE_COORD( 16 );

		// velocity
		WRITE_COORD( 0 ); 
		WRITE_COORD( 0 );
		WRITE_COORD( 30 );

		// randomization
		WRITE_BYTE( 15 ); 

		// Model
		WRITE_SHORT( m_iBodyGibs );	//model id#

		// # of shards
		WRITE_BYTE( 35 );

		// duration
		WRITE_BYTE( 100 );// 5.0 seconds

		// flags
		WRITE_BYTE( BREAK_METAL );
	MESSAGE_END();

	SetThink( &CMBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// RunAI - Robo Grunt emits sparks when its low on health.
//=========================================================
void CMRGrunt::RunAI(void)
{
	CMBaseMonster::RunAI();
	
	if ( pev->health <= ( pev->max_health / 10 ) ) // below 10% health
	{
		// Spark ON
		if ( gpGlobals->time > m_flNextSpark )
		{
			// Code looks familiar? It's CBaseButton::DoSpark
			for ( int spark_num = 0; spark_num < RGRUNT_MAX_SPARKS; spark_num++ )
			{
				Vector tmp = pev->origin + (pev->mins + pev->maxs) * 0.5; // grab center
				tmp.x += RANDOM_FLOAT( -( pev->size.x / 2 ), pev->size.x / 2); // then randomize
				tmp.y += RANDOM_FLOAT( -( pev->size.y / 2 ), pev->size.y / 2);
				tmp.z += RANDOM_FLOAT( -( pev->size.z / 2 ), pev->size.z / 2);
				UTIL_Sparks( tmp );
			}
			
			switch ( (int)(RANDOM_FLOAT(0,1) * 6) )
			{
				case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark1.wav", 0.6, ATTN_NORM);	break;
				case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark2.wav", 0.6, ATTN_NORM);	break;
				case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark3.wav", 0.6, ATTN_NORM);	break;
				case 3: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark4.wav", 0.6, ATTN_NORM);	break;
				case 4: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark5.wav", 0.6, ATTN_NORM);	break;
				case 5: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark6.wav", 0.6, ATTN_NORM);	break;
			}

			m_flNextSpark = gpGlobals->time + 0.5;
		}
		
		// Glow/Hurt ON
		if ( gpGlobals->time > m_flNextDischarge )
		{
			// Turn on the electric glow
			pev->renderfx = kRenderFxGlowShell;
			pev->rendercolor = Vector( 100, 150, 250 ); // r, g, b
			
			EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/beamstart14.wav", 0.8, ATTN_NORM);

			// Sustain the electricity for this long
			m_flActiveDischarge = gpGlobals->time + RANDOM_FLOAT( 0.3, 0.6 );

			// Discharge again in...
			m_flNextDischarge = gpGlobals->time + RANDOM_FLOAT( 0.9, 2.7 );
		}

		// Glow/Hurt OFF
		if ( gpGlobals->time > m_flActiveDischarge )
		{
			// Turn off electricity
			pev->renderfx = kRenderFxNone;
			m_flActiveDischarge = 0;
		}
	}
}

//=========================================================
// SparkTouch - Hurt players who come too close to it
//=========================================================
void CMRGrunt::SparkTouch( edict_t *pOther )
{
	// No electricity, no harm
	if ( m_flActiveDischarge == 0 )
		return;

	// Only affect players
	if ( UTIL_IsPlayer( pOther ) )
	{
		// Because of Touch(), players are going to be hurt every server frame.
		// Don't be bullshit like Sven Co-op, set the damage REALLY LOW.
		UTIL_TakeDamage( pOther, pev, pev, 1, DMG_SHOCK );
	}
}

//=========================================================
// TraceAttack - Override for robo grunt
// Emit ricochet sparks if getting hurt from bullets
//=========================================================
void CMRGrunt::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	// Absorb damage and emit ricochet if bullets or melee attacks are used
	if ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) )
	{
		if ( RANDOM_LONG( 0, 100 ) < 20 )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 0.5, 1.5 ) );
//			EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, pRicSounds[ RANDOM_LONG(0,ARRAYSIZE(pRicSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_NORM );
		}
		flDamage *= (1.00f - gSkillData.rgruntArmor); // cut damage
	}
	// Lower protection against explosions
	else if ( bitsDamageType & DMG_BLAST )
		flDamage *= (1.00f - (gSkillData.rgruntArmor / 2.00f)); // 50% less protection
	// No protection at all against other types of damages

	CMBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );

}

//=========================================================
// TakeDamage - Robo Grunts should not take cover as soon
// as they take damage.
//=========================================================
int CMRGrunt::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CMBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Spawn
//=========================================================
void CMRGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/rgrunt.mdl"));
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	pev->effects = 0;
	pev->health = gSkillData.rgruntHealth;
	pev->max_health = pev->health; // to determine when sparks should be emitted
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_flNextSpark = gpGlobals->time; // when to emit sparks again
	m_flNextDischarge = gpGlobals->time; // when electric shell should activate
	m_flActiveDischarge = 0; // how long to sustain the electricity
	m_iSentence = -1;

	//m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	
	//m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	if (pev->weapons == 0)
	{
		// weapons not specified, randomize
		switch(RANDOM_LONG(0, 2))
		{
			case 0:
				pev->weapons = RGRUNT_9MMAR | RGRUNT_HANDGRENADE;
				break;
			case 1:
				pev->weapons = RGRUNT_SHOTGUN;
				break;
			case 2:
				pev->weapons = RGRUNT_9MMAR | RGRUNT_GRENADELAUNCHER;
				break;
		}
	}

	if (FBitSet(pev->weapons, RGRUNT_SHOTGUN))
	{
		SetBodygroup(GUN_GROUP, GUN_SHOTGUN);
		m_cClipSize = 8;
	}
	else
	{
		m_cClipSize = RGRUNT_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;
	
	CMTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
	
	SetTouch( &CMRGrunt::SparkTouch );

	pev->classname = MAKE_STRING( "monster_robogrunt" );
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Robo Grunt" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMRGrunt::Precache()
{
	PRECACHE_MODEL("models/rgrunt.mdl");
	
	m_iBodyGibs = PRECACHE_MODELINDEX( "models/metalplategibs_green.mdl" );

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");
	
	PRECACHE_SOUND("turret/tu_die.wav");
	PRECACHE_SOUND("turret/tu_die2.wav");
	
	PRECACHE_SOUND("buttons/spark1.wav");
	PRECACHE_SOUND("buttons/spark2.wav");
	PRECACHE_SOUND("buttons/spark3.wav");
	PRECACHE_SOUND("buttons/spark4.wav");
	PRECACHE_SOUND("buttons/spark5.wav");
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/beamstart14.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sbarrel1.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event
	
	/*
	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 109 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 100;
	*/

	m_iBrassShell = PRECACHE_MODELINDEX("models/shell.mdl");// brass shell
}
