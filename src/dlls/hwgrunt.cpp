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
// Heavy Weapons Grunt
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
#include	"customentity.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	HWGRUNT_9MM_CLIP_SIZE		36 // clip ammo per gun
#define HWGRUNT_DGL_CLIP_SIZE		7
#define HWGRUNT_357_CLIP_SIZE		6

// Weapon flags
#define HWGRUNT_MINIGUN				(1 << 0)
#define HWGRUNT_PISTOL_9MM			(1 << 1)
#define HWGRUNT_PISTOL_DGL			(1 << 2)
#define HWGRUNT_PISTOL_357			(1 << 3)

#define GUN_GROUP					1

// Gun values
#define GUN_MINIGUN					0
#define GUN_PISTOL_9MM				1
#define GUN_PISTOL_357				2
#define GUN_PISTOL_DGL				3
#define GUN_NONE					4

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HWGRUNT_AE_DEATH		( 11 )
#define		HWGRUNT_AE_MINIGUN		( 5001 )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMHWGrunt::Classify(void)
{
	if ( m_iClassifyOverride == -1 ) // helper
		return CLASS_NONE;
	else if ( m_iClassifyOverride > 0 )
		return m_iClassifyOverride; // override
	
	return CLASS_HUMAN_MILITARY;
}


//=========================================================
// Shoot
//=========================================================
void CMHWGrunt::Minigun(void)
{
	if (m_hEnemy == 0)
	{
		return;
	}
	
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_12MM); // shoot +-5 degrees
	
	pev->effects |= EF_MUZZLEFLASH;
	
	// Minigunners have infinite ammo
	//m_cAmmoLoaded--;// take away a bullet!
	
	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// TraceAttack - hwgrunts do not wear helmets
//=========================================================
void CMHWGrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CMBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for hwgrunts.
// They are meant to be aggresive, never take cover.
//=========================================================
int CMHWGrunt :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CMBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMHWGrunt::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case HWGRUNT_AE_DEATH:
		break; // don't get rid of gun

	case HWGRUNT_AE_MINIGUN:
	{
		// Sven Co-op uses a modified hassault/hw_gun4.wav for it's fire sound
		Minigun();
		
		// We don't want looping WAVs. Pick a different sound and change pitch on it
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5));
	}
	break;
	
	default:
		CMHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMHWGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hwgrunt.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.hwgruntHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	//m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_flMinigunSpinTime = 0; // be able to spin up/down minigun right away
	m_iSentence = -1;

	//m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	
	//m_fEnemyEluded = FALSE;
	m_fFirstEncounter = FALSE;// false because hwgrunt does not send signals of any kind

	m_HackedGunPos = Vector(0, 0, 55);
	
	// Don't setup pev->weapons, always minigun if not specified
	/*
	if (FBitSet(pev->weapons, HWGRUNT_MINIGUN))
	{
		SetBodygroup(GUN_GROUP, GUN_MINIGUN);
		m_cClipSize = 1;
	}
	*/
	m_cAmmoLoaded = 99;
	
	CMTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
	
	pev->classname = MAKE_STRING( "monster_hwgrunt" );
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Heavy Weapons Grunt" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMHWGrunt::Precache()
{
	PRECACHE_MODEL("models/hwgrunt.mdl");
	
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("hassault/hw_spinup.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");
	
	PRECACHE_SOUND("common/null.wav");

	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 102 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 93; // slight voice change for hwgrunt
}

//=========================================================
// SetActivity - different set than normal hgrunt, adapt
//=========================================================
void CMHWGrunt :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	bool refreshActivity = TRUE;
	
	// PS: This is terrible code. -Giegue

	// Time to die?
	if ( NewActivity < ACT_DIESIMPLE )
	{
		if ( pev->sequence == LookupSequence( "attack" ) )
		{
			// I won't do anything else if I'm attacking!
			refreshActivity = FALSE;
			
			// Unless the enemy has gone out of my sight
			if ( m_hEnemy == 0 || !UTIL_IsAlive( m_hEnemy ) || !UTIL_FVisible( m_hEnemy, ENT(pev) ) )
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_spindown.wav", 0.8, ATTN_NORM);
				m_flMinigunSpinTime = gpGlobals->time + 1.40;
				iSequence = LookupSequence( "spindown" ); // time to relax
			}
		}
		else if ( pev->sequence == LookupSequence( "spindown" ) )
		{
			// Not yet!
			refreshActivity = FALSE;
			
			// Wait until the minigun is no longer spinning before doing something else
			if ( gpGlobals->time > m_flMinigunSpinTime )
			{
				refreshActivity = TRUE;
				m_flMinigunSpinTime = 0; // do spin up again when required
			}
		}
	}

	if (refreshActivity)
	{
		switch (NewActivity)
		{
		case ACT_RANGE_ATTACK1:
			// if carring a gun, either standing or crouched.
			// always standing when firing minigun
			if (pev->weapons > 0) // any pistol
			{
				// same animation regardless of pistol
				if ( m_fStanding )
				{
					// get aimable sequence
					iSequence = LookupSequence( "pistol_shoot" );
				}
				else
				{
					// get crouching shoot
					iSequence = LookupSequence( "pistol_crouchshoot" );
				}
			}
			else // minigun
			{
				if ( m_flMinigunSpinTime == 0 ) // starting to spin up the minigun
				{
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_spinup.wav", 0.8, ATTN_NORM);
					m_flMinigunSpinTime = gpGlobals->time + 1.15;
					iSequence = LookupSequence( "spinup" );
				}
				else if ( gpGlobals->time > m_flMinigunSpinTime ) // spun up, ready to fire
					iSequence = LookupSequence( "attack" );
			}
			break;
		case ACT_IDLE:
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
			{
				NewActivity = ACT_IDLE_ANGRY;
			}
			iSequence = LookupActivity ( NewActivity );
			break;
		case ACT_RUN:
		case ACT_WALK:
		default:
			iSequence = LookupActivity ( NewActivity );
			break;
		}
		
		m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	}

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else if (refreshActivity)
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}
