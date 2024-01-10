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
// Black Ops - Male Assassin
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

extern cvar_t *monster_default_maxrange;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	MASSN_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!

// Weapon flags
#define MASSN_9MMAR					(1 << 0)
#define MASSN_HANDGRENADE			(1 << 1)
#define MASSN_GRENADELAUNCHER		(1 << 2)
#define MASSN_SNIPERRIFLE			(1 << 3)

// Body groups.
#define HEAD_GROUP					1
#define GUN_GROUP					2

// Head values
#define HEAD_WHITE					0
#define HEAD_BLACK					1
#define HEAD_GOGGLES				2

// Gun values
#define GUN_MP5						0
#define GUN_SNIPERRIFLE				1
#define GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MASSN_AE_KICK			( 3 )
#define		MASSN_AE_BURST1			( 4 )
#define		MASSN_AE_BURST2			( 5 )
#define		MASSN_AE_BURST3			( 6 )
#define		MASSN_AE_CAUGHT_ENEMY	( 10 ) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		MASSN_AE_DROP_GUN		( 11 ) // grunt (probably dead) is dropping his mp5.

//=========================================================
// Override a few behaviours to make this grunt silent
//=========================================================
BOOL CMMassn::FOkToSpeak(void)
{
	return FALSE;
}

void CMMassn::IdleSound(void)
{
}

void CMMassn::PainSound(void)
{
}

void CMMassn::DeathSound(void)
{
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMMassn::Classify(void)
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
void CMMassn::Sniperrifle(void)
{
	if (m_hEnemy == 0)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_MONSTER_762, 0); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMMassn::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
	case MASSN_AE_DROP_GUN:
	{
		Vector	vecGunPos;
		Vector	vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		// switch to body group with no gun.
		SetBodygroup(GUN_GROUP, GUN_NONE);
	}
	break;


	case MASSN_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MASSN_9MMAR))
		{
			Shoot();

			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (RANDOM_LONG(0, 1))
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM);
			}
			else
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM);
			}
		}
		else if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
		{
			Sniperrifle();

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM);
		}
	}
	break;

	case MASSN_AE_BURST2:
	case MASSN_AE_BURST3:
		Shoot();
		break;

	case MASSN_AE_KICK:
	{
		edict_t *pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->v.punchangle.x = 15;
			pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			if (UTIL_IsPlayer(pHurt))
				UTIL_TakeDamage( pHurt, pev, pev, gSkillData.massnDmgKick, DMG_CLUB );
			else if (pHurt->v.euser4 != NULL)
			{
				CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pHurt));
				pMonster->TakeDamage( pev, pev, gSkillData.massnDmgKick, DMG_CLUB );
			}
			else
				UTIL_TakeDamageExternal( pHurt, pev, pev, gSkillData.massnDmgKick, DMG_CLUB );
		}
	}
	break;

	case MASSN_AE_CAUGHT_ENEMY:
		break;

	default:
		CMHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMMassn::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/massn.mdl"));
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = !m_bloodColor ? BLOOD_COLOR_RED : m_bloodColor;
	pev->effects = 0;
	if (!pev->health) { pev->health = gSkillData.massnHealth; }
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = -1;

	//m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	
	//m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	if (pev->weapons == 0)
	{
		// weapons not specified, randomize
		switch ( RANDOM_LONG( 0, 2 ) )
		{
			case 0:
				pev->weapons = MASSN_9MMAR | MASSN_HANDGRENADE;
				break;
			case 1:
				pev->weapons = MASSN_9MMAR | MASSN_GRENADELAUNCHER;
				break;
			case 2:
				pev->weapons = MASSN_SNIPERRIFLE;
				break;
		}
	}

	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		SetBodygroup(GUN_GROUP, GUN_SNIPERRIFLE);
		m_cClipSize = 5;

		// if no attack range set, set 2x default
		if (!m_flDistLook)
			m_flDistLook = monster_default_maxrange->value * 2;
	}
	else
	{
		m_cClipSize = MASSN_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;

	if (RANDOM_LONG(0, 99) < 80)
		pev->skin = 0;	// light skin
	else
		pev->skin = 1;	// dark skin

	CMTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		// override for snipers
		m_flDistTooFar = m_flDistLook / 1.33;
	}

	pev->classname = MAKE_STRING( "monster_male_assassin" );
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Male Assassin" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMMassn::Precache()
{
	PRECACHE_MODEL("models/massn.mdl");

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND("weapons/sniper_fire.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 109 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODELINDEX("models/shell.mdl");// brass shell
}

//=========================================================
// Chase enemy failure schedule
//=========================================================
Task_t	tlMassnSniperAttack[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE_ANGRY 		},
	{ TASK_WAIT_FACE_ENEMY ,		(float)1					},
	{ TASK_SET_ACTIVITY,			(float)ACT_RANGE_ATTACK1	},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slMassnSniperAttack[] =
{
	{ 
		tlMassnSniperAttack,
		ARRAYSIZE ( tlMassnSniperAttack ), 
		bits_COND_HEAR_SOUND,
		0,
		"MassnSniperAttack"
	},
};

DEFINE_CUSTOM_SCHEDULES( CMMassn )
{
	slMassnSniperAttack,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMMassn, CMHGrunt );

//=========================================================
// SetActivity 
//=========================================================
void CMMassn :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity )
	{
	case ACT_RANGE_ATTACK1:
		// shooting standing or shooting crouched
		if (FBitSet( pev->weapons, MASSN_SNIPERRIFLE))
		{
			// Always standing
			iSequence = LookupSequence( "standing_m40a1" );
		}
		else
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_mp5" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_mp5" );
			}
		}
		break;
	default:
		CMHGrunt::SetActivity(NewActivity);
		return;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

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
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// GetScheduleOfType - Override schedule for sniper attack
//=========================================================
Schedule_t* CMMassn :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
		{
			if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
			{
				// sniper attack is always standing
				m_fStanding = TRUE;
				return &slMassnSniperAttack[ 0 ];
			}
			
			return CMHGrunt :: GetScheduleOfType ( Type );
		}
	default:
		{
			return CMHGrunt :: GetScheduleOfType ( Type );
		}
	}
}
