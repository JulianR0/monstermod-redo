// HUGE thanks to DrBeef for his hlsdk-xash3d-opfor repository!

/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//=========================================================
// Gonome.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"decals.h"
#include	"nodes.h"

#define		GONOME_MELEE_ATTACK_RADIUS		70

enum
{
	TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define GONOME_AE_SLASH_RIGHT	( 1 )
#define GONOME_AE_SLASH_LEFT	( 2 )
#define GONOME_AE_SPIT			( 3 )
#define GONOME_AE_THROW		( 4 )

#define GONOME_AE_BITE1			( 19 )
#define GONOME_AE_BITE2			( 20 )
#define GONOME_AE_BITE3			( 21 )
#define GONOME_AE_BITE4			( 22 )

#define GONOME_SCRIPT_EVENT_SOUND ( 1011 )

void CGonomeGuts :: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "gonomeguts" );

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL( ENT( pev ), "sprites/bigspit.spr" );
	pev->frame = 0;
	pev->scale = 0.5;
	pev->rendercolor.x = 255;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_maxFrame = (float)MODEL_FRAMES( pev->modelindex ) - 1;
}

void CGonomeGuts :: Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->frame++ )
	{
		if ( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

edict_t *CGonomeGuts :: Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGonomeGuts *pSpit = CreateClassPtr( (CGonomeGuts *)NULL );

   if (pSpit == NULL)
      return NULL;

	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink ( &CGonomeGuts::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
	pSpit->SetTouch ( &CGonomeGuts::GutsTouch );
	
	return pSpit->edict();
}

void CGonomeGuts :: GutsTouch( edict_t *pOther )
{
	TraceResult tr;
	int iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	}
	
	if( !pOther->v.takedamage )
	{
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
		UTIL_BloodDrips( tr.vecEndPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
	}
	else
	{
		if (UTIL_IsPlayer(pOther))
			UTIL_TakeDamage( pOther, pev, VARS(pev->owner), gSkillData.gonomeDmgGuts, DMG_GENERIC );
		else if (pOther->v.euser4 != NULL)
		{
			CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pOther));
			pMonster->TakeDamage ( pev, VARS(pev->owner), gSkillData.gonomeDmgGuts, DMG_GENERIC );
		}
		else
			UTIL_TakeDamageExternal( pOther, pev, VARS(pev->owner), gSkillData.gonomeDmgGuts, DMG_GENERIC );
	}
	
	SetThink( &CGonomeGuts::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}


const char* CMGonome::pPainSounds[] = {
	"gonome/gonome_pain1.wav",
	"gonome/gonome_pain2.wav",
	"gonome/gonome_pain3.wav",
	"gonome/gonome_pain4.wav"
};

const char* CMGonome::pIdleSounds[] = {
	"gonome/gonome_idle1.wav",
	"gonome/gonome_idle2.wav",
	"gonome/gonome_idle3.wav"
};

const char* CMGonome::pDeathSounds[] = {
	"gonome/gonome_death2.wav",
	"gonome/gonome_death3.wav",
	"gonome/gonome_death4.wav"
};

const char* CMGonome::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CMGonome::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};


void CMGonome::Killed(entvars_t *pevAttacker, int iGib)
{
	ClearGuts();
	UnlockPlayer();
	CMBaseMonster::Killed(pevAttacker, iGib);
}

void CMGonome::UnlockPlayer()
{
	if (m_fPlayerLocked)
	{
		edict_t *player = 0;
		if (m_lockedPlayer != 0 && UTIL_IsPlayer(m_lockedPlayer))
			player = m_lockedPlayer;
		else // if ehandle is empty for some reason just unlock the first player
			player = UTIL_FindEntityByClassname(0, "player");
		
		if (player)
			player->v.flags &= ~FL_FROZEN;
		
		m_lockedPlayer = 0;
		m_fPlayerLocked = FALSE;
	}
}

CGonomeGuts* CMGonome::GetGonomeGuts(entvars_t *pevOwner, const Vector &pos)
{
	if (m_pGonomeGuts)
		return m_pGonomeGuts;
	edict_t *pEdict = CGonomeGuts::Shoot( pevOwner, g_vecZero, g_vecZero );
	CGonomeGuts *pGuts = GetClassPtr((CGonomeGuts*)VARS(pEdict));
	pGuts->Spawn();

	UTIL_SetOrigin( pGuts->pev, pos );

	m_pGonomeGuts = pGuts;
	return m_pGonomeGuts;
}

void CMGonome::ClearGuts()
{
	if (m_pGonomeGuts)
	{
		UTIL_Remove( m_pGonomeGuts->edict() );
		m_pGonomeGuts = 0;
	}
}

void CMGonome::PainSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	if( RANDOM_LONG( 0, 5 ) < 2 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, pitch );
}

void CMGonome::DeathSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, pitch );
}

void CMGonome::IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	// Play a random idle sound
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1.0, ATTN_NORM, 0, pitch );
}

void CMGonome::AlertSound( void )
{
	const int iPitch = RANDOM_LONG(0, 9) + 95;
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1, ATTN_NORM, 0, iPitch);
}

void CMGonome::SetActivity( Activity NewActivity )
{
	Activity OldActivity = m_Activity;
	int iSequence = ACTIVITY_NOT_AVAILABLE;

	if (NewActivity != ACT_RANGE_ATTACK1)
	{
		ClearGuts();
	}
	if (NewActivity == ACT_MELEE_ATTACK1 && m_hEnemy != 0)
	{
		// special melee animations
		if ((pev->origin - m_hEnemy->v.origin).Length2D() >= 48 )
		{
			m_meleeAttack2 = false;
			iSequence = LookupSequence("attack1");
		}
		else
		{
			m_meleeAttack2 = true;
			iSequence = LookupSequence("attack2");
		}
	}
	else
	{
		UnlockPlayer();

		if (NewActivity == ACT_RUN && m_hEnemy != 0)
		{
			// special run animations
			if ((pev->origin - m_hEnemy->v.origin).Length2D() <= 512 )
			{
				iSequence = LookupSequence("runshort");
			}
			else
			{
				iSequence = LookupSequence("runlong");
			}
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if( !( OldActivity == ACT_WALK || OldActivity == ACT_RUN ) || !( NewActivity == ACT_WALK || NewActivity == ACT_RUN ) )
				pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT( at_aiconsole, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CMGonome::Classify(void)
{
	if ( m_iClassifyOverride == -1 ) // helper
		return CLASS_NONE;
	else if ( m_iClassifyOverride > 0 )
		return m_iClassifyOverride; // override
	
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CMGonome::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	// HACK HACK -- until we fix this.
	if( IsAlive() )
		PainSound();
	return CMBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CMGonome::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist < 256)
		return FALSE;

	if (IsMoving() && flDist >= 512)
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextThrowTime)
	{
		if (m_hEnemy != 0)
		{
			if (fabs(pev->origin.z - m_hEnemy->v.origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextThrowTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextThrowTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - both gonome's melee attacks are ACT_MELEE_ATTACK1
//=========================================================
BOOL CMGonome::CheckMeleeAttack2(float flDot, float flDist)
{
	return FALSE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMGonome::SetYawSpeed( void )
{
	pev->yaw_speed = 120;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMGonome::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case GONOME_SCRIPT_EVENT_SOUND:
		if (m_Activity != ACT_MELEE_ATTACK1)
			EMIT_SOUND(ENT(pev), CHAN_BODY, pEvent->options, 1, ATTN_NORM);
		break;
	case GONOME_AE_SPIT:
	{
		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		if (GetGonomeGuts(pev, vecArmPos))
		{
			m_pGonomeGuts->pev->skin = entindex();
			m_pGonomeGuts->pev->body = 1;
			m_pGonomeGuts->pev->aiment = ENT(pev);
			m_pGonomeGuts->pev->movetype = MOVETYPE_FOLLOW;
		}
		UTIL_BloodDrips( vecArmPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
	}
	break;
	case GONOME_AE_THROW:
	{
		UTIL_MakeVectors(pev->angles);
		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);
		
		if (GetGonomeGuts(pev, vecArmPos))
		{
			Vector	vecSpitDir;

			Vector vecEnemyPosition;
			if (m_hEnemy != 0)
				vecEnemyPosition = (m_hEnemy->v.origin + m_hEnemy->v.view_ofs);
			else
				vecEnemyPosition = m_vecEnemyLKP;
			vecSpitDir = (vecEnemyPosition - vecArmPos).Normalize();

			vecSpitDir.x += RANDOM_FLOAT(-0.05, 0.05);
			vecSpitDir.y += RANDOM_FLOAT(-0.05, 0.05);
			vecSpitDir.z += RANDOM_FLOAT(-0.05, 0);

			m_pGonomeGuts->pev->body = 0;
			m_pGonomeGuts->pev->skin = 0;
			m_pGonomeGuts->pev->owner = ENT( pev );
			m_pGonomeGuts->pev->aiment = 0;
			m_pGonomeGuts->pev->movetype = MOVETYPE_FLY;
			m_pGonomeGuts->pev->velocity = vecSpitDir * 900;
			m_pGonomeGuts->SetThink( &CGonomeGuts::Animate );
			m_pGonomeGuts->pev->nextthink = gpGlobals->time + 0.1;
			UTIL_SetOrigin(m_pGonomeGuts->pev, vecArmPos);

			m_pGonomeGuts = 0;
		}
		UTIL_BloodDrips( vecArmPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
	}
	break;

	case GONOME_AE_SLASH_LEFT:
	{
		edict_t *pHurt = CheckTraceHullAttack(GONOME_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (FBitSet(pHurt->v.flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->v.punchangle.z = 9;
				pHurt->v.punchangle.x = 5;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_right * 25;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5));
		}
		else
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5));
		}
	}
	break;

	case GONOME_AE_SLASH_RIGHT:
	{
		edict_t *pHurt = CheckTraceHullAttack(GONOME_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (FBitSet(pHurt->v.flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->v.punchangle.z = -9;
				pHurt->v.punchangle.x = 5;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_right * -25;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5));
		}
		else
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5));
		}
	}
	break;

	case GONOME_AE_BITE1:
	case GONOME_AE_BITE2:
	case GONOME_AE_BITE3:
	case GONOME_AE_BITE4:
		{
			int iPitch;
			edict_t *pHurt = CheckTraceHullAttack(GONOME_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneBite, DMG_SLASH);

			if (pHurt)
			{
				// croonchy bite sound
				iPitch = RANDOM_FLOAT(90, 110);
				switch (RANDOM_LONG(0, 1))
				{
				case 0:
					EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite2.wav", 1, ATTN_NORM, 0, iPitch);
					break;
				case 1:
					EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite3.wav", 1, ATTN_NORM, 0, iPitch);
					break;
				}

				if (FBitSet(pHurt->v.flags, FL_MONSTER|FL_CLIENT))
				{
					if (pEvent->event == GONOME_AE_BITE4)
					{
						pHurt->v.punchangle.x = 15;
						pHurt->v.velocity = pHurt->v.velocity - gpGlobals->v_forward * 75;
					}
					else
					{
						pHurt->v.punchangle.x = 9;
						pHurt->v.velocity = pHurt->v.velocity - gpGlobals->v_forward * 25;
					}
				}
				// lock player
				if (pEvent->event == GONOME_AE_BITE4)
				{
					UnlockPlayer();
				}
				else if (UTIL_IsPlayer( pHurt ) && UTIL_IsAlive( pHurt ))
				{
					if (!m_fPlayerLocked)
					{
						edict_t *player = pHurt;
						player->v.flags |= FL_FROZEN;
						m_lockedPlayer = player;
						m_fPlayerLocked = TRUE;
					}
				}
			}
		}
		break;

	default:
		CMBaseMonster::HandleAnimEvent(pEvent);
	}
}

#define GONOME_FLINCH_DELAY 2

int CMGonome::IgnoreConditions( void )
{
	int iIgnore = CMBaseMonster::IgnoreConditions();

	if( m_Activity == ACT_MELEE_ATTACK1 )
	{
		if( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
	}

	if( ( m_Activity == ACT_SMALL_FLINCH ) || ( m_Activity == ACT_BIG_FLINCH ) )
	{
		if( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + GONOME_FLINCH_DELAY;
	}

	return iIgnore;
}

//=========================================================
// Spawn
//=========================================================
void CMGonome::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/gonome.mdl"));
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = !m_bloodColor ? BLOOD_COLOR_YELLOW : m_bloodColor;
	pev->effects = 0;
	if (!pev->health) { pev->health = gSkillData.gonomeHealth; }
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_flNextThrowTime = gpGlobals->time;

	MonsterInit();
	
	pev->classname = MAKE_STRING( "monster_gonome" );
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Gonome" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMGonome::Precache()
{
	PRECACHE_MODEL("models/gonome.mdl");

	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("gonome/gonome_eat.wav");
	PRECACHE_SOUND("gonome/gonome_jumpattack.wav");
	PRECACHE_SOUND("gonome/gonome_melee1.wav");
	PRECACHE_SOUND("gonome/gonome_melee2.wav");

	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	PRECACHE_SOUND("gonome/gonome_run.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");

	PRECACHE_SOUND("bullchicken/bc_bite2.wav");
	PRECACHE_SOUND("bullchicken/bc_bite3.wav");

	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t *CMGonome::GetSchedule( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CMBaseMonster::GetSchedule();
			}

			if( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}

			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
			}

			return GetScheduleOfType( SCHED_CHASE_ENEMY );
			break;
		}
	default:
			break;
	}

	return CMBaseMonster::GetSchedule();
}

// primary range attack
Task_t tlGonomeRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slGonomeRangeAttack1[] =
{
	{
		tlGonomeRangeAttack1,
		ARRAYSIZE( tlGonomeRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Gonome Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlGonomeChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },// !!!OEM - this will stop nasty squid oscillation.
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slGonomeChaseEnemy[] =
{
	{
		tlGonomeChaseEnemy1,
		ARRAYSIZE( tlGonomeChaseEnemy1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_SMELL_FOOD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED,
		0,
		"Gonome Chase Enemy"
	},
};

// victory dance (eating body)
Task_t tlGonomeVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE,	(float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE }
};

Schedule_t slGonomeVictoryDance[] =
{
	{
		tlGonomeVictoryDance,
		ARRAYSIZE( tlGonomeVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"GonomeVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES( CMGonome )
{
	slGonomeRangeAttack1,
	slGonomeChaseEnemy,
	slGonomeVictoryDance,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMGonome, CMBaseMonster )

Schedule_t* CMGonome::GetScheduleOfType(int Type)
{
	switch ( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slGonomeRangeAttack1[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slGonomeChaseEnemy[0];
		break;
	case SCHED_VICTORY_DANCE:
		return &slGonomeVictoryDance[0];
		break;
	default:
		break;
	}
	return CMBaseMonster::GetScheduleOfType(Type);
}

void CMGonome::RunTask(Task_t *pTask)
{
	// HACK to stop Gonome from playing attack sound twice
	if (pTask->iTask == TASK_MELEE_ATTACK1)
	{
		if (!m_playedAttackSound)
		{
			const char* sample = NULL;
			if (m_meleeAttack2)
			{
				sample = "gonome/gonome_melee2.wav";
			}
			else
			{
				sample = "gonome/gonome_melee1.wav";
			}
			EMIT_SOUND(ENT(pev), CHAN_BODY, sample, 1, ATTN_NORM);
			m_playedAttackSound = true;
		}
	}
	else
	{
		m_playedAttackSound = false;
	}
	CMBaseMonster::RunTask(pTask);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CMGonome::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE:
		{
			UTIL_MakeVectors( pev->angles );
			if( BuildRoute( m_vecEnemyLKP - gpGlobals->v_forward * 40, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else
			{
				ALERT( at_aiconsole, "GonomeGetPathToEnemyCorpse failed!!\n" );
				TaskFail();
			}
		}
		break;
	default:
		CMBaseMonster::StartTask(pTask);
		break;

	}
}
