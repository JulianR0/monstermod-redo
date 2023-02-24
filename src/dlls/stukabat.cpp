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
// Stukabat - Xen Birb
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmflyingmonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define	STUKABAT_AE_BITE		1
#define STUKABAT_AE_FLAP		8

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMStukabat :: Classify ( void )
{
	if ( m_iClassifyOverride == -1 ) // helper
		return CLASS_NONE;
	else if ( m_iClassifyOverride > 0 )
		return m_iClassifyOverride; // override
	
	return CLASS_ALIEN_PREDATOR;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMStukabat :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_HOVER:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMStukabat :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case STUKABAT_AE_BITE:
		{
			edict_t *pHurt = CheckTraceHullAttack( 70, gSkillData.stukabatDmgBite, DMG_SLASH|DMG_POISON );
			if ( pHurt )
			{
				// Play bite sound
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "headcrab/hc_headbite.wav", 1.0, ATTN_NORM, 0, GetBitePitch() );
			}
		}
		break;
	case STUKABAT_AE_FLAP:
		{
			m_flightSpeed = gSkillData.stukabatSpeed; // set our own speed
		}
		break;
	default:
		CMFlyingMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMStukabat :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/stukabat.mdl"));
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags			|= FL_FLY;
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	pev->health			= gSkillData.stukabatHealth;
	pev->view_ofs		= Vector ( 0, 0, 22 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	
	m_pFlapSound = "stukabat/stukabat_flap1.wav";
	
	MonsterInit();

	pev->classname = MAKE_STRING( "monster_stukabat" );
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Stukabat" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMStukabat :: Precache()
{
	PRECACHE_MODEL("models/stukabat.mdl");

	PRECACHE_SOUND("stukabat/stukabat_flap1.wav"); // flying sound
	PRECACHE_SOUND("headcrab/hc_headbite.wav"); // bite sound
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

/* Chase */
Task_t tlStukabatChaseEnemy[] = 
{
	{ TASK_GET_PATH_TO_ENEMY,	(float)128		}, // is the 128 number really used?
	{ TASK_SET_ACTIVITY,		(float)ACT_FLY	},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},

};
Schedule_t slStukabatChaseEnemy[] =
{
	{ 
		tlStukabatChaseEnemy,
		ARRAYSIZE ( tlStukabatChaseEnemy ),
		bits_COND_NEW_ENEMY			|
		bits_COND_TASK_FAILED,
		0,
		"StukabatChaseEnemy"
	},
};

/* Fail */
Task_t	tlStukabatFail[] =
{
	{ TASK_STOP_MOVING,			0					},
	{ TASK_SET_ACTIVITY,		(float)ACT_HOVER	},
	{ TASK_WAIT,				(float)2			},
	{ TASK_WAIT_PVS,			(float)0			},
};
Schedule_t	slStukabatFail[] =
{
	{
		tlStukabatFail,
		ARRAYSIZE ( tlStukabatFail ),
		0,
		0,
		"StukabatFail"
	},
};


DEFINE_CUSTOM_SCHEDULES( CMStukabat )
{
	slStukabatChaseEnemy,
	slStukabatFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMStukabat, CMFlyingMonster );

//=========================================================
// SetActivity 
//=========================================================
void CMStukabat :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	
	switch ( NewActivity )
	{
	case ACT_IDLE:
		return; // refuse
	case ACT_FLY:
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
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
// GetScheduleOfType
//=========================================================
Schedule_t* CMStukabat :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slStukabatChaseEnemy;
	case SCHED_FAIL:
		return slStukabatFail;
	}

	return CMBaseMonster :: GetScheduleOfType( Type );
}


//=========================================================
// CheckRangeAttack1  - Poisonous Bite
//=========================================================
BOOL CMStukabat :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDot > 0.7 && flDist <= 64 )
	{
		return TRUE;
	}
	return FALSE;
}
