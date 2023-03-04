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
#define	HWGRUNT_9MM_CLIP_SIZE		17 // clip ammo per gun
#define HWGRUNT_DGL_CLIP_SIZE		7
#define HWGRUNT_357_CLIP_SIZE		6

// Weapon flags
#define HWGRUNT_MINIGUN				0
#define HWGRUNT_PISTOL_9MM			1
#define HWGRUNT_PISTOL_DGL			2
#define HWGRUNT_PISTOL_357			3

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
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_HWGRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_HWGRUNT_SWEEP,
	SCHED_HWGRUNT_REPEL,
	SCHED_HWGRUNT_REPEL_ATTACK,
	SCHED_HWGRUNT_REPEL_LAND,
	SCHED_HWGRUNT_WAIT_FACE_ENEMY,
	SCHED_HWGRUNT_ELOF_FAIL,
};

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
// CheckRangeAttack1 - HWGrunt doesn't care about melee
//=========================================================
BOOL CMHWGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 )
	{
		TraceResult	tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, UTIL_BodyTarget(m_hEnemy, vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - HWGrunt does not kick
//=========================================================
BOOL CMHWGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - HWGrunt has no grenades
//=========================================================
BOOL CMHWGrunt :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
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

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/hwgrunt.mdl"));
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
	m_fStanding = TRUE;

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
	m_cClipSize = 99;

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
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t	tlHWGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slHWGruntFail[] =
{
	{
		tlHWGruntFail,
		ARRAYSIZE ( tlHWGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWGrunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlHWGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slHWGruntCombatFail[] =
{
	{
		tlHWGruntCombatFail,
		ARRAYSIZE ( tlHWGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWGrunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlHWGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					}
};

Schedule_t	slHWGruntVictoryDance[] =
{
	{ 
		tlHWGruntVictoryDance,
		ARRAYSIZE ( tlHWGruntVictoryDance ), 
		bits_COND_NEW_ENEMY,
		0,
		"HWGruntVictoryDance"
	},
};


//=========================================================
// ELOF fail, just wait and try again
//=========================================================
Task_t	tlHWGruntELOFFail[] =
{
	{ TASK_STOP_MOVING,				(float)0									},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE								},
	{ TASK_FACE_ENEMY,				(float)0									},
	{ TASK_WAIT,					(float)1.5									},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE	},
};

Schedule_t	slHWGruntELOFFail[] =
{
	{ 
		tlHWGruntELOFFail,
		ARRAYSIZE ( tlHWGruntELOFFail ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWGrunt Failed ELOF"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlHWGruntEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_HWGRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slHWGruntEstablishLineOfFire[] =
{
	{ 
		tlHWGruntEstablishLineOfFire,
		ARRAYSIZE ( tlHWGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		0,
		"HWGruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlHWGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HWGRUNT_SWEEP	},
};

Schedule_t	slHWGruntCombatFace[] =
{
	{ 
		tlHWGruntCombatFace1,
		ARRAYSIZE ( tlHWGruntCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWCombat Face"
	},
};

Task_t	tlHWGruntSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slHWGruntSuppress[] =
{
	{ 
		tlHWGruntSuppress,
		ARRAYSIZE ( tlHWGruntSuppress ), 
		0,
		0,
		"HWSuppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlHWGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slHWGruntWaitInCover[] =
{
	{ 
		tlHWGruntWaitInCover,
		ARRAYSIZE ( tlHWGruntWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWGruntWaitInCover"
	},
};


//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlHWGruntSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slHWGruntSweep[] =
{
	{ 
		tlHWGruntSweep,
		ARRAYSIZE ( tlHWGruntSweep ),
		bits_COND_NEW_ENEMY		|
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWGrunt Sweep"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlHWGruntRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slHWGruntRangeAttack1B[] =
{
	{ 
		tlHWGruntRangeAttack1B,
		ARRAYSIZE ( tlHWGruntRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_OCCLUDED,
		0,
		"HWRange Attack1B"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlHWGruntRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slHWGruntRepel[] =
{
	{ 
		tlHWGruntRepel,
		ARRAYSIZE ( tlHWGruntRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY,
		0,
		"HWRepel"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlHWGruntRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slHWGruntRepelAttack[] =
{
	{ 
		tlHWGruntRepelAttack,
		ARRAYSIZE ( tlHWGruntRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"HWRepel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlHWGruntRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slHWGruntRepelLand[] =
{
	{ 
		tlHWGruntRepelLand,
		ARRAYSIZE ( tlHWGruntRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY,
		0,
		"HWRepel Land"
	},
};


DEFINE_CUSTOM_SCHEDULES( CMHWGrunt )
{
	slHWGruntFail,
	slHWGruntCombatFail,
	slHWGruntVictoryDance,
	slHWGruntELOFFail,
	slHWGruntEstablishLineOfFire,
	slHWGruntCombatFace,
	slHWGruntSuppress,
	slHWGruntWaitInCover,
	slHWGruntSweep,
	slHWGruntRangeAttack1B,
	slHWGruntRepel,
	slHWGruntRepelAttack,
	slHWGruntRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMHWGrunt, CMBaseMonster );

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
			if ( m_flMinigunSpinTime != 0 )
			{
				// if the hwgrunt used his minigun but became unable to attack
				// then spin it down before doing anything else
				refreshActivity = FALSE;

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_spindown.wav", 0.8, ATTN_NORM);
				m_flMinigunSpinTime = gpGlobals->time + 1.40;
				iSequence = LookupSequence( "spindown" );
			}
			else
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

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CMHWGrunt :: GetSchedule( void )
{
	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_HWGRUNT_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_HWGRUNT_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_HWGRUNT_REPEL );
		}
	}

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CMBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_HWGRUNT_SUPPRESS );
				}
				else
				{
					return GetScheduleOfType ( SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE );
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				// Stop believing you have no ammo! -Giegue
				ClearConditions( bits_COND_NO_AMMO_LOADED );
				if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_HWGRUNT_SUPPRESS );
				}
				else
				{
					return GetScheduleOfType ( SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE );
				}
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				// Force attack!
				return GetScheduleOfType ( SCHED_HWGRUNT_SUPPRESS );
			}
// can't see enemy
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CMBaseMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CMHWGrunt :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_HWGRUNT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return &slHWGruntELOFFail[ 0 ];
		}
		break;
	case SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slHWGruntEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// no pistols yet, always do standing attack
			return &slHWGruntRangeAttack1B[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slHWGruntCombatFace[ 0 ];
		}
	case SCHED_HWGRUNT_WAIT_FACE_ENEMY:
		{
			return &slHWGruntWaitInCover[ 0 ];
		}
	case SCHED_HWGRUNT_SWEEP:
		{
			return &slHWGruntSweep[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slHWGruntVictoryDance[ 0 ];
		}
	case SCHED_HWGRUNT_SUPPRESS:
		{
			return &slHWGruntSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slHWGruntCombatFail[ 0 ];
			}

			return &slHWGruntFail[ 0 ];
		}
	case SCHED_HWGRUNT_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slHWGruntRepel[ 0 ];
		}
	case SCHED_HWGRUNT_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slHWGruntRepelAttack[ 0 ];
		}
	case SCHED_HWGRUNT_REPEL_LAND:
		{
			return &slHWGruntRepelLand[ 0 ];
		}
	default:
		{
			return CMBaseMonster :: GetScheduleOfType ( Type );
		}
	}
}
