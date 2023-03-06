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
/*

===== monsters.cpp ========================================================

  Monster-related utility code

*/

#include "extdll.h"
#include "util.h"
#include "cmbase.h"
#include "cmbasemonster.h"
#include "nodes.h"
#include "monsters.h"
#include "animation.h"
#include "weapons.h"
#include "decals.h"

#define MONSTER_CUT_CORNER_DIST		8 // 8 means the monster's bounding box is contained without the box of the node in WC


Vector VecBModelOrigin( entvars_t* pevBModel );

extern DLL_GLOBAL	BOOL	g_fDrawLines;

extern CGraph WorldGraph;// the world node graph

extern cvar_t *monster_turn_coeficient;



//=========================================================
// Eat - makes a monster full for a little while.
//=========================================================
void CMBaseMonster :: Eat ( float flFullDuration )
{
	m_flHungryTime = gpGlobals->time + flFullDuration;
}

//=========================================================
// FShouldEat - returns true if a monster is hungry.
//=========================================================
BOOL CMBaseMonster :: FShouldEat ( void )
{
	if ( m_flHungryTime > gpGlobals->time )
	{
		return FALSE;
	}

	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - called
// by Barnacle victims when the barnacle pulls their head
// into its mouth
//=========================================================
void CMBaseMonster :: BarnacleVictimBitten ( entvars_t *pevBarnacle )
{
	Schedule_t	*pNewSchedule;

	pNewSchedule = GetScheduleOfType( SCHED_BARNACLE_VICTIM_CHOMP );

	if ( pNewSchedule )
	{
		ChangeSchedule( pNewSchedule );
	}
}

//=========================================================
// BarnacleVictimReleased - called by barnacle victims when
// the host barnacle is killed.
//=========================================================
void CMBaseMonster :: BarnacleVictimReleased ( void )
{
	m_IdealMonsterState = MONSTERSTATE_IDLE;

	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_STEP;
}


//=========================================================
// FValidateHintType - tells use whether or not the monster cares
// about the type of Hint Node given
//=========================================================
BOOL CMBaseMonster :: FValidateHintType ( short sHint )
{
	return FALSE;
}

//=========================================================
// Look - Base class monster function to find enemies or 
// food by sight. iDistance is distance ( in units ) that the 
// monster can see.
//
// Sets the sight bits of the m_afConditions mask to indicate
// which types of entities were sighted.
// Function also sets the Looker's m_pLink 
// to the head of a link list that contains all visible ents.
// (linked via each ent's m_pLink field)
//
//=========================================================
void CMBaseMonster :: Look ( int iDistance )
{
	int	iSighted = 0;

	// DON'T let visibility information from last frame sit around!
	ClearConditions(bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR | bits_COND_SEE_NEMESIS | bits_COND_SEE_CLIENT);

	m_edictList_count = 0;

	edict_t	*pSightEnt = NULL;// the current visible entity that we're dealing with

	// See no evil if prisoner is set
	if ( !FBitSet( pev->spawnflags, SF_MONSTER_PRISONER ) )
	{
		edict_t *pList[100];

		Vector delta = Vector( iDistance, iDistance, iDistance );

		// Find only monsters/clients in box, NOT limited to PVS
		int count = UTIL_EntitiesInBox( pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT|FL_MONSTER );
		for ( int i = 0; i < count; i++ )
		{
			pSightEnt = pList[i];
			// !!!temporarily only considering other monsters and clients, don't see prisoners
			if ( pSightEnt != this->edict()	&& !FBitSet( pSightEnt->v.spawnflags, SF_MONSTER_PRISONER ) && pSightEnt->v.health > 0 )
			{
				// is this a player AND are they alive?
				if (UTIL_IsPlayer(pSightEnt) && UTIL_IsAlive(pSightEnt))
				{
					// the looker will want to consider this entity
					// don't check anything else about an entity that can't be seen.
					if ( UTIL_FInViewCone( pSightEnt, ENT(pev), m_flFieldOfView ) && !FBitSet( pSightEnt->v.flags, FL_NOTARGET ) && UTIL_FVisible( pSightEnt, ENT(pev) ) )
					{
						m_edictList[m_edictList_count] = pSightEnt;
						m_edictList_count++;

						// if we see a client, remember that (mostly for scripted AI)
						iSighted |= bits_COND_SEE_CLIENT;

						// is this monster NOT a scientist?
						if (strcmp(STRING(pev->model), "models/scientist.mdl") != 0)
						{
							iSighted |= bits_COND_SEE_DISLIKE;

							if ( pSightEnt == m_hEnemy )
							{
								// we know this ent is visible, so if it also happens to be our enemy, store that now.
								iSighted |= bits_COND_SEE_ENEMY;
							}
						}
					}
				}
				else if (pSightEnt->v.euser4 != NULL)
				{
					/* MonsterMod monster looking at another MonsterMod monster */
					CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pSightEnt));

					// the looker will want to consider this entity
					// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
					if ( IRelationship( pMonster ) != R_NO && UTIL_FInViewCone( pSightEnt, ENT(pev), m_flFieldOfView ) && !FBitSet( pSightEnt->v.flags, FL_NOTARGET ) && UTIL_FVisible( pSightEnt, ENT(pev) ) )
					{
						m_edictList[m_edictList_count] = pSightEnt;
						m_edictList_count++;

						if ( ENT(pMonster->pev) == m_hEnemy )
						{
							// we know this ent is visible, so if it also happens to be our enemy, store that now.
							iSighted |= bits_COND_SEE_ENEMY;
						}

						// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when
						// we see monsters other than the Enemy.
						switch ( IRelationship ( pMonster ) )
						{
						case	R_NM:
							iSighted |= bits_COND_SEE_NEMESIS;		
							break;
						case	R_HT:		
							iSighted |= bits_COND_SEE_HATE;		
							break;
						case	R_DL:
							iSighted |= bits_COND_SEE_DISLIKE;
							break;
						case	R_FR:
							iSighted |= bits_COND_SEE_FEAR;
							break;
						case    R_AL:
							break;
						default:
							ALERT ( at_aiconsole, "%s can't assess %s\n", STRING(pev->classname), STRING(pMonster->pev->classname ) );
							break;
						}
					}
				}
				else
				{
					/* MonsterMod monster looking at a NON-MonsterMod monster */

					// the looker will want to consider this entity
					// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
					if ( IRelationship( pSightEnt->v.iuser4 ) != R_NO && UTIL_FInViewCone( pSightEnt, ENT(pev), m_flFieldOfView ) && !FBitSet( pSightEnt->v.flags, FL_NOTARGET ) && UTIL_FVisible( pSightEnt, ENT(pev) ) )
					{
						m_edictList[m_edictList_count] = pSightEnt;
						m_edictList_count++;

						if ( pSightEnt == m_hEnemy )
						{
							// we know this ent is visible, so if it also happens to be our enemy, store that now.
							iSighted |= bits_COND_SEE_ENEMY;
						}

						// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when
						// we see monsters other than the Enemy.
						switch ( IRelationship ( pSightEnt->v.iuser4 ) )
						{
						case	R_NM:
							iSighted |= bits_COND_SEE_NEMESIS;		
							break;
						case	R_HT:		
							iSighted |= bits_COND_SEE_HATE;		
							break;
						case	R_DL:
							iSighted |= bits_COND_SEE_DISLIKE;
							break;
						case	R_FR:
							iSighted |= bits_COND_SEE_FEAR;
							break;
						case    R_AL:
							break;
						default:
							ALERT ( at_aiconsole, "%s can't assess %s\n", STRING(pev->classname), STRING(pSightEnt->v.classname ) );
							break;
						}
					}
				}
			}
		}
	}
	
	SetConditions( iSighted );
}


//=========================================================
// Monster Think - calls out to core AI functions and handles this
// monster's specific animation events
//=========================================================
void CMBaseMonster :: MonsterThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;// keep monster thinking.


	RunAI();

	float flInterval = StudioFrameAdvance( ); // animate
// start or end a fidget
// This needs a better home -- switching animations over time should be encapsulated on a per-activity basis
// perhaps MaintainActivity() or a ShiftAnimationOverTime() or something.
	if ( m_MonsterState != MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_DEAD && m_Activity == ACT_IDLE && m_fSequenceFinished )
	{
		int iSequence;

		if ( m_fSequenceLoops )
		{
			// animation does loop, which means we're playing subtle idle. Might need to 
			// fidget.
			iSequence = LookupActivity ( m_Activity );
		}
		else
		{
			// animation that just ended doesn't loop! That means we just finished a fidget
			// and should return to our heaviest weighted idle (the subtle one)
			iSequence = LookupActivityHeaviest ( m_Activity );
		}
		if ( iSequence != ACTIVITY_NOT_AVAILABLE )
		{
			pev->sequence = iSequence;	// Set to new anim (if it's there)
			ResetSequenceInfo( );
		}
	}

	DispatchAnimEvents( flInterval );

	if ( !MovementIsComplete() )
	{
		Move( flInterval );
	}
#if _DEBUG	
	else 
	{
		if ( !TaskIsRunning() && !TaskIsComplete() )
			ALERT( at_error, "Schedule stalled!!\n" );
	}
#endif
}

//=========================================================
// CMBaseMonster - USE - will make a monster angry at whomever
// activated it.
//=========================================================
void CMBaseMonster :: MonsterUse ( edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value )
{
	m_IdealMonsterState = MONSTERSTATE_ALERT;
}

//=========================================================
// Ignore conditions - before a set of conditions is allowed
// to interrupt a monster's schedule, this function removes
// conditions that we have flagged to interrupt the current
// schedule, but may not want to interrupt the schedule every
// time. (Pain, for instance)
//=========================================================
int CMBaseMonster :: IgnoreConditions ( void )
{
	int iIgnoreConditions = 0;

	if ( !FShouldEat() )
	{
		// not hungry? Ignore food smell.
		iIgnoreConditions |= bits_COND_SMELL_FOOD;
	}

	return iIgnoreConditions;
}

//=========================================================
// 	RouteClear - zeroes out the monster's route array and goal
//=========================================================
void CMBaseMonster :: RouteClear ( void )
{
	RouteNew();
	m_movementGoal = MOVEGOAL_NONE;
	m_movementActivity = ACT_IDLE;
	Forget( bits_MEMORY_MOVE_FAILED );
}

//=========================================================
// Route New - clears out a route to be changed, but keeps
//				goal intact.
//=========================================================
void CMBaseMonster :: RouteNew ( void )
{
	m_Route[ 0 ].iType		= 0;
	m_iRouteIndex			= 0;
}

//=========================================================
// FRouteClear - returns TRUE is the Route is cleared out
// ( invalid )
//=========================================================
BOOL CMBaseMonster :: FRouteClear ( void )
{
	if ( m_Route[ m_iRouteIndex ].iType == 0 || m_movementGoal == MOVEGOAL_NONE )
		return TRUE;

	return FALSE;
}

//=========================================================
// FRefreshRoute - after calculating a path to the monster's
// target, this function copies as many waypoints as possible
// from that path to the monster's Route array
//=========================================================
BOOL CMBaseMonster :: FRefreshRoute ( void )
{
	edict_t	*pPathCorner;
	int			i;
	BOOL		returnCode;

	RouteNew();

	returnCode = FALSE;

	switch( m_movementGoal )
	{
		case MOVEGOAL_PATHCORNER:
			{
				// monster is on a path_corner loop
				pPathCorner = m_pGoalEnt;
				i = 0;

				while ( pPathCorner && i < ROUTE_SIZE )
				{
					m_Route[ i ].iType = bits_MF_TO_PATHCORNER;
					m_Route[ i ].vecLocation = pPathCorner->v.origin;

					pPathCorner = UTIL_GetNextTarget(pPathCorner);

					// Last path_corner in list?
					if ( !pPathCorner )
						m_Route[i].iType |= bits_MF_IS_GOAL;
					
					i++;
				}
			}
			returnCode = TRUE;
			break;

		case MOVEGOAL_ENEMY:
			returnCode = BuildRoute( m_vecEnemyLKP, bits_MF_TO_ENEMY, m_hEnemy );
			break;

		case MOVEGOAL_LOCATION:
			returnCode = BuildRoute( m_vecMoveGoal, bits_MF_TO_LOCATION, NULL );
			break;

		case MOVEGOAL_TARGETENT:
			if (m_hTargetEnt != NULL)
			{
				returnCode = BuildRoute( m_hTargetEnt->v.origin, bits_MF_TO_TARGETENT, m_hTargetEnt );
			}
			break;

		case MOVEGOAL_NODE:
			returnCode = FGetNodeRoute( m_vecMoveGoal );
//			if ( returnCode )
//				RouteSimplify( NULL );
			break;
	}

	return returnCode;
}


BOOL CMBaseMonster::MoveToEnemy( Activity movementAct, float waitTime )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;
	
	m_movementGoal = MOVEGOAL_ENEMY;
	return FRefreshRoute();
}


BOOL CMBaseMonster::MoveToLocation( Activity movementAct, float waitTime, const Vector &goal )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;
	
	m_movementGoal = MOVEGOAL_LOCATION;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}


BOOL CMBaseMonster::MoveToTarget( Activity movementAct, float waitTime )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;
	
	m_movementGoal = MOVEGOAL_TARGETENT;
	return FRefreshRoute();
}


BOOL CMBaseMonster::MoveToNode( Activity movementAct, float waitTime, const Vector &goal )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_NODE;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}


int ShouldSimplify( int routeType )
{
	routeType &= ~bits_MF_IS_GOAL;

	if ( (routeType == bits_MF_TO_PATHCORNER) || (routeType & bits_MF_DONT_SIMPLIFY) )
		return FALSE;
	return TRUE;
}

//=========================================================
// RouteSimplify
//
// Attempts to make the route more direct by cutting out
// unnecessary nodes & cutting corners.
//
//=========================================================
void CMBaseMonster :: RouteSimplify( edict_t *pTargetEnt )
{
	// BUGBUG: this doesn't work 100% yet
	int			i, count, outCount;
	Vector		vecStart;
	WayPoint_t	outRoute[ ROUTE_SIZE * 2 ];	// Any points except the ends can turn into 2 points in the simplified route

	count = 0;

	for ( i = m_iRouteIndex; i < ROUTE_SIZE; i++ )
	{
		if ( !m_Route[i].iType )
			break;
		else
			count++;
		if ( m_Route[i].iType & bits_MF_IS_GOAL )
			break;
	}
	// Can't simplify a direct route!
	if ( count < 2 )
	{
//		DrawRoute( pev, m_Route, m_iRouteIndex, 0, 0, 255 );
		return;
	}

	outCount = 0;
	vecStart = pev->origin;
	for ( i = 0; i < count-1; i++ )
	{
		// Don't eliminate path_corners
		if ( !ShouldSimplify( m_Route[m_iRouteIndex+i].iType ) )
		{
			outRoute[outCount] = m_Route[ m_iRouteIndex + i ];
			outCount++;
		}
		else if ( CheckLocalMove ( vecStart, m_Route[m_iRouteIndex+i+1].vecLocation, pTargetEnt, NULL ) == LOCALMOVE_VALID )
		{
			// Skip vert
			continue;
		}
		else
		{
			Vector vecTest, vecSplit;

			// Halfway between this and next
			vecTest = (m_Route[m_iRouteIndex+i+1].vecLocation + m_Route[m_iRouteIndex+i].vecLocation) * 0.5;

			// Halfway between this and previous
			vecSplit = (m_Route[m_iRouteIndex+i].vecLocation + vecStart) * 0.5;

			int iType = (m_Route[m_iRouteIndex+i].iType | bits_MF_TO_DETOUR) & ~bits_MF_NOT_TO_MASK;
			if ( CheckLocalMove ( vecStart, vecTest, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecTest;
			}
			else if ( CheckLocalMove ( vecSplit, vecTest, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecSplit;
				outRoute[outCount+1].iType = iType;
				outRoute[outCount+1].vecLocation = vecTest;
				outCount++; // Adding an extra point
			}
			else
			{
				outRoute[outCount] = m_Route[ m_iRouteIndex + i ];
			}
		}
		// Get last point
		vecStart = outRoute[ outCount ].vecLocation;
		outCount++;
	}
	ASSERT( i < count );
	outRoute[outCount] = m_Route[ m_iRouteIndex + i ];
	outCount++;
	
	// Terminate
	outRoute[outCount].iType = 0;
	ASSERT( outCount < (ROUTE_SIZE*2) );

// Copy the simplified route, disable for testing
	m_iRouteIndex = 0;
	for ( i = 0; i < ROUTE_SIZE && i < outCount; i++ )
	{
		m_Route[i] = outRoute[i];
	}

	// Terminate route
	if ( i < ROUTE_SIZE )
		m_Route[i].iType = 0;

// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT( "simplify" ) != 0 )
		DrawRoute( pev, outRoute, 0, 255, 0, 0 );
//	else
		DrawRoute( pev, m_Route, m_iRouteIndex, 0, 255, 0 );
#endif
}

//=========================================================
// FBecomeProne - tries to send a monster into PRONE state.
// right now only used when a barnacle snatches someone, so 
// may have some special case stuff for that.
//=========================================================
BOOL CMBaseMonster :: FBecomeProne ( void )
{
	if ( FBitSet ( pev->flags, FL_ONGROUND ) )
	{
		pev->flags -= FL_ONGROUND;
	}

	m_IdealMonsterState = MONSTERSTATE_PRONE;
	return TRUE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CMBaseMonster :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist > 64 && flDist <= 784 && flDot >= 0.5 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CMBaseMonster :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( flDist > 64 && flDist <= 512 && flDot >= 0.5 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CMBaseMonster :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= 64 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->v.flags, FL_ONGROUND ) )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================
BOOL CMBaseMonster :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckAttacks - sets all of the bits for attacks that the
// monster is capable of carrying out on the passed entity.
//=========================================================
void CMBaseMonster :: CheckAttacks ( edict_t *pTarget, float flDist )
{
	Vector2D	vec2LOS;
	float		flDot;

	UTIL_MakeVectors ( pev->angles );

	vec2LOS = ( pTarget->v.origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	// we know the enemy is in front now. We'll find which attacks the monster is capable of by
	// checking for corresponding Activities in the model file, then do the simple checks to validate
	// those attack types.
	
	// Clear all attack conditions
	ClearConditions( bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK1 |bits_COND_CAN_MELEE_ATTACK2 );

	if ( m_afCapability & bits_CAP_RANGE_ATTACK1 )
	{
		if ( CheckRangeAttack1 ( flDot, flDist ) )
			SetConditions( bits_COND_CAN_RANGE_ATTACK1 );
	}
	if ( m_afCapability & bits_CAP_RANGE_ATTACK2 )
	{
		if ( CheckRangeAttack2 ( flDot, flDist ) )
			SetConditions( bits_COND_CAN_RANGE_ATTACK2 );
	}
	if ( m_afCapability & bits_CAP_MELEE_ATTACK1 )
	{
		if ( CheckMeleeAttack1 ( flDot, flDist ) )
			SetConditions( bits_COND_CAN_MELEE_ATTACK1 );
	}
	if ( m_afCapability & bits_CAP_MELEE_ATTACK2 )
	{
		if ( CheckMeleeAttack2 ( flDot, flDist ) )
			SetConditions( bits_COND_CAN_MELEE_ATTACK2 );
	}
}

//=========================================================
// CanCheckAttacks - prequalifies a monster to do more fine
// checking of potential attacks. 
//=========================================================
BOOL CMBaseMonster :: FCanCheckAttacks ( void )
{
	if ( HasConditions(bits_COND_SEE_ENEMY) && !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckEnemy - part of the Condition collection process,
// gets and stores data and conditions pertaining to a monster's
// enemy. Returns TRUE if Enemy LKP was updated.
//=========================================================
int CMBaseMonster :: CheckEnemy ( edict_t *pEnemy )
{
	float	flDistToEnemy;
	int		iUpdatedLKP;// set this to TRUE if you update the EnemyLKP in this function.

	iUpdatedLKP = FALSE;
	ClearConditions ( bits_COND_ENEMY_FACING_ME );
	
	if ( !UTIL_FVisible( pEnemy, ENT(pev) ) )
	{
		ASSERT(!HasConditions(bits_COND_SEE_ENEMY));
		SetConditions( bits_COND_ENEMY_OCCLUDED );
	}
	else
		ClearConditions( bits_COND_ENEMY_OCCLUDED );

	if ( !UTIL_IsAlive(pEnemy) )
	{
		SetConditions ( bits_COND_ENEMY_DEAD );
		ClearConditions( bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED );
		return FALSE;
	}

	Vector vecEnemyPos = pEnemy->v.origin;
	// distance to enemy's origin
	flDistToEnemy = ( vecEnemyPos - pev->origin ).Length();
	vecEnemyPos.z += pEnemy->v.size.z * 0.5;
	// distance to enemy's head
	float flDistToEnemy2 = (vecEnemyPos - pev->origin).Length();
	if (flDistToEnemy2 < flDistToEnemy)
		flDistToEnemy = flDistToEnemy2;
	else
	{
		// distance to enemy's feet
		vecEnemyPos.z -= pEnemy->v.size.z;
		float flDistToEnemy2 = (vecEnemyPos - pev->origin).Length();
		if (flDistToEnemy2 < flDistToEnemy)
			flDistToEnemy = flDistToEnemy2;
	}

	if ( HasConditions( bits_COND_SEE_ENEMY ) )
	{
		edict_t *pEnemyMonster;

		iUpdatedLKP = TRUE;
		m_vecEnemyLKP = pEnemy->v.origin;

		pEnemyMonster = pEnemy;

		if ( pEnemyMonster )
		{
			if ( UTIL_FInViewCone(pEnemyMonster, ENT(this->pev), m_flFieldOfView) )
			{
				SetConditions ( bits_COND_ENEMY_FACING_ME );
			}
			else
				ClearConditions( bits_COND_ENEMY_FACING_ME );
		}

		if (pEnemy->v.velocity != Vector( 0, 0, 0))
		{
			// trail the enemy a bit
			m_vecEnemyLKP = m_vecEnemyLKP - pEnemy->v.velocity * RANDOM_FLOAT( -0.05, 0 );
		}
		else
		{
			// UNDONE: use pev->oldorigin?
		}
	}
	else if ( !HasConditions(bits_COND_ENEMY_OCCLUDED|bits_COND_SEE_ENEMY) && ( flDistToEnemy <= 256 ) )
	{
		// if the enemy is not occluded, and unseen, that means it is behind or beside the monster.
		// if the enemy is near enough the monster, we go ahead and let the monster know where the
		// enemy is. 
		iUpdatedLKP = TRUE;
		m_vecEnemyLKP = pEnemy->v.origin;
	}

	if ( flDistToEnemy >= m_flDistTooFar )
	{
		// enemy is very far away from monster
		SetConditions( bits_COND_ENEMY_TOOFAR );
	}
	else
		ClearConditions( bits_COND_ENEMY_TOOFAR );

	if ( FCanCheckAttacks() )	
	{
		CheckAttacks ( m_hEnemy, flDistToEnemy );
	}

	if ( m_movementGoal == MOVEGOAL_ENEMY )
	{
		for ( int i = m_iRouteIndex; i < ROUTE_SIZE; i++ )
		{
			if ( m_Route[ i ].iType == (bits_MF_IS_GOAL|bits_MF_TO_ENEMY) )
			{
				// UNDONE: Should we allow monsters to override this distance (80?)
				if ( (m_Route[ i ].vecLocation - m_vecEnemyLKP).Length() > 80 )
				{
					// Refresh
					FRefreshRoute();
					return iUpdatedLKP;
				}
			}
		}
	}

	return iUpdatedLKP;
}

//=========================================================
// PushEnemy - remember the last few enemies, always remember the player
//=========================================================
void CMBaseMonster :: PushEnemy( edict_t *pEnemy, Vector &vecLastKnownPos )
{
	int i;

	if (pEnemy == NULL)
		return;

	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for (i = 0; i < MAX_OLD_ENEMIES; i++)
	{
		if (m_hOldEnemy[i] == pEnemy)
			return;
		if (m_hOldEnemy[i] == NULL) // someone died, reuse their slot
			break;
	}
	if (i >= MAX_OLD_ENEMIES)
		return;

	m_hOldEnemy[i] = pEnemy;
	m_vecOldEnemy[i] = vecLastKnownPos;
}

//=========================================================
// PopEnemy - try remembering the last few enemies
//=========================================================
BOOL CMBaseMonster :: PopEnemy( )
{
	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for (int i = MAX_OLD_ENEMIES - 1; i >= 0; i--)
	{
		if (m_hOldEnemy[i] != NULL)
		{
			if (UTIL_IsAlive(m_hOldEnemy[i])) // cheat and know when they die
			{
				m_hEnemy = m_hOldEnemy[i];
				m_vecEnemyLKP = m_vecOldEnemy[i];
				// ALERT( at_console, "remembering\n");
				return TRUE;
			}
			else
			{
				m_hOldEnemy[i] = NULL;
			}
		}
	}
	return FALSE;
}

//=========================================================
// SetActivity 
//=========================================================
void CMBaseMonster :: SetActivity ( Activity NewActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( NewActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if ( !(m_Activity == ACT_WALK || m_Activity == ACT_RUN) || !(NewActivity == ACT_WALK || NewActivity == ACT_RUN))
				pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_aiconsole, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	
	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;


}

//=========================================================
// SetSequenceByName
//=========================================================
void CMBaseMonster :: SetSequenceByName ( char *szSequence )
{
	int	iSequence;

	iSequence = LookupSequence ( szSequence );

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
		ALERT ( at_aiconsole, "%s has no sequence named:%f\n", STRING(pev->classname), szSequence );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// CheckLocalMove - returns TRUE if the caller can walk a 
// straight line from its current origin to the given 
// location. If so, don't use the node graph!
//
// if a valid pointer to a int is passed, the function
// will fill that int with the distance that the check 
// reached before hitting something. THIS ONLY HAPPENS
// IF THE LOCAL MOVE CHECK FAILS!
//
// !!!PERFORMANCE - should we try to load balance this?
// DON"T USE SETORIGIN! 
//=========================================================
#define	LOCAL_STEP_SIZE	16
int CMBaseMonster :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, edict_t *pTarget, float *pflDist )
{
	Vector	vecStartPos;// record monster's position before trying the move
	float	flYaw;
	float	flDist;
	float	flStep, stepSize;
	int		iReturn;

	vecStartPos = pev->origin;
	
	
	flYaw = UTIL_VecToYaw ( vecEnd - vecStart );// build a yaw that points to the goal.
	flDist = ( vecEnd - vecStart ).Length2D();// get the distance.
	iReturn = LOCALMOVE_VALID;// assume everything will be ok.

	// move the monster to the start of the local move that's to be checked.
	UTIL_SetOrigin( pev, vecStart );// !!!BUGBUG - won't this fire triggers? - nope, SetOrigin doesn't fire

	if ( !(pev->flags & (FL_FLY|FL_SWIM)) )
	{
		DROP_TO_FLOOR( ENT( pev ) );//make sure monster is on the floor!
	}

	//pev->origin.z = vecStartPos.z;//!!!HACKHACK

//	pev->origin = vecStart;

/*
	if ( flDist > 1024 )
	{
		// !!!PERFORMANCE - this operation may be too CPU intensive to try checks this large.
		// We don't lose much here, because a distance this great is very likely
		// to have something in the way.

		// since we've actually moved the monster during the check, undo the move.
		pev->origin = vecStartPos;
		return FALSE;
	}
*/
	// this loop takes single steps to the goal.
	for ( flStep = 0 ; flStep < flDist ; flStep += LOCAL_STEP_SIZE )
	{
		stepSize = LOCAL_STEP_SIZE;

		if ( (flStep + LOCAL_STEP_SIZE) >= (flDist-1) )
			stepSize = (flDist - flStep) - 1;
		
//		UTIL_ParticleEffect ( pev->origin, g_vecZero, 255, 25 );

		if ( !WALK_MOVE( ENT(pev), flYaw, stepSize, WALKMOVE_CHECKONLY ) )
		{// can't take the next step, fail!

			if ( pflDist != NULL )
			{
				*pflDist = flStep;
			}
			if ( pTarget && pTarget == gpGlobals->trace_ent )
			{
				// if this step hits target ent, the move is legal.
				iReturn = LOCALMOVE_VALID;
				break;
			}
			else
			{
				// If we're going toward an entity, and we're almost getting there, it's OK.
//				if ( pTarget && fabs( flDist - iStep ) < LOCAL_STEP_SIZE )
//					fReturn = TRUE;
//				else
				iReturn = LOCALMOVE_INVALID;
				break;
			}

		}
	}

	if ( iReturn == LOCALMOVE_VALID && 	!(pev->flags & (FL_FLY|FL_SWIM) ) && (!pTarget || (pTarget->v.flags & FL_ONGROUND)) )
	{
		// The monster can move to a spot UNDER the target, but not to it. Don't try to triangulate, go directly to the node graph.
		// UNDONE: Magic # 64 -- this used to be pev->size.z but that won't work for small creatures like the headcrab
		if ( fabs(vecEnd.z - pev->origin.z) > 64 )
		{
			iReturn = LOCALMOVE_INVALID_DONT_TRIANGULATE;
		}
	}
	/*
	// uncommenting this block will draw a line representing the nearest legal move.
	WRITE_BYTE(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(MSG_BROADCAST, TE_SHOWLINE);
	WRITE_COORD(MSG_BROADCAST, pev->origin.x);
	WRITE_COORD(MSG_BROADCAST, pev->origin.y);
	WRITE_COORD(MSG_BROADCAST, pev->origin.z);
	WRITE_COORD(MSG_BROADCAST, vecStart.x);
	WRITE_COORD(MSG_BROADCAST, vecStart.y);
	WRITE_COORD(MSG_BROADCAST, vecStart.z);
	*/

	// since we've actually moved the monster during the check, undo the move.
	UTIL_SetOrigin( pev, vecStartPos );

	return iReturn;
}


//=========================================================
// AdvanceRoute - poorly named function that advances the 
// m_iRouteIndex. If it goes beyond ROUTE_SIZE, the route 
// is refreshed. 
//=========================================================
void CMBaseMonster :: AdvanceRoute ( float distance )
{

	if ( m_iRouteIndex == ROUTE_SIZE - 1 )
	{
		// time to refresh the route.
		if ( !FRefreshRoute() )
		{
			ALERT ( at_aiconsole, "Can't Refresh Route!!\n" );
		}
	}
	else
	{
		if ( ! (m_Route[ m_iRouteIndex ].iType & bits_MF_IS_GOAL) )
		{
			// If we've just passed a path_corner, advance m_pGoalEnt
			if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_PATHCORNER )
				m_pGoalEnt = UTIL_GetNextTarget(m_pGoalEnt);

			// IF both waypoints are nodes, then check for a link for a door and operate it.
			//
			if (  (m_Route[m_iRouteIndex].iType   & bits_MF_TO_NODE) == bits_MF_TO_NODE
			   && (m_Route[m_iRouteIndex+1].iType & bits_MF_TO_NODE) == bits_MF_TO_NODE)
			{
				//ALERT(at_aiconsole, "SVD: Two nodes. ");

				int iSrcNode  = WorldGraph.FindNearestNode(m_Route[m_iRouteIndex].vecLocation, this );
				int iDestNode = WorldGraph.FindNearestNode(m_Route[m_iRouteIndex+1].vecLocation, this );

				int iLink;
				WorldGraph.HashSearch(iSrcNode, iDestNode, iLink);

				if ( iLink >= 0 && WorldGraph.m_pLinkPool[iLink].m_pLinkEnt != NULL )
				{
					//ALERT(at_aiconsole, "A link. ");
					if ( WorldGraph.HandleLinkEnt ( iSrcNode, WorldGraph.m_pLinkPool[iLink].m_pLinkEnt, m_afCapability, CGraph::NODEGRAPH_DYNAMIC ) )
					{
						//ALERT(at_aiconsole, "usable.");
						entvars_t *pevDoor = WorldGraph.m_pLinkPool[iLink].m_pLinkEnt;
						if (pevDoor)
						{
//							m_flMoveWaitFinished = OpenDoorAndWait( pevDoor );
//							ALERT( at_aiconsole, "Wating for door %.2f\n", m_flMoveWaitFinished-gpGlobals->time );
						}
					}
				}
				//ALERT(at_aiconsole, "\n");
			}
			m_iRouteIndex++;
		}
		else	// At goal!!!
		{
			if ( distance < m_flGroundSpeed * 0.2 /* FIX */ )
			{
				MovementComplete();
			}
		}
	}
}


int CMBaseMonster :: RouteClassify( int iMoveFlag )
{
	int movementGoal;

	movementGoal = MOVEGOAL_NONE;

	if ( iMoveFlag & bits_MF_TO_TARGETENT )
		movementGoal = MOVEGOAL_TARGETENT;
	else if ( iMoveFlag & bits_MF_TO_ENEMY )
		movementGoal = MOVEGOAL_ENEMY;
	else if ( iMoveFlag & bits_MF_TO_PATHCORNER )
		movementGoal = MOVEGOAL_PATHCORNER;
	else if ( iMoveFlag & bits_MF_TO_NODE )
		movementGoal = MOVEGOAL_NODE;
	else if ( iMoveFlag & bits_MF_TO_LOCATION )
		movementGoal = MOVEGOAL_LOCATION;

	return movementGoal;
}

//=========================================================
// BuildRoute
//=========================================================
BOOL CMBaseMonster :: BuildRoute ( const Vector &vecGoal, int iMoveFlag, edict_t *pTarget )
{
	float	flDist;
	Vector	vecApex;
	int		iLocalMove;

	RouteNew();
	m_movementGoal = RouteClassify( iMoveFlag );

// so we don't end up with no moveflags
	m_Route[ 0 ].vecLocation = vecGoal;
	m_Route[ 0 ].iType = iMoveFlag | bits_MF_IS_GOAL;

// check simple local move
	iLocalMove = CheckLocalMove( pev->origin, vecGoal, pTarget, &flDist );

	if ( iLocalMove == LOCALMOVE_VALID )
	{
		// monster can walk straight there!
		return TRUE;
	}
// try to triangulate around any obstacles.
	else if ( iLocalMove != LOCALMOVE_INVALID_DONT_TRIANGULATE && FTriangulate( pev->origin, vecGoal, flDist, pTarget, &vecApex ) )
	{
		// there is a slightly more complicated path that allows the monster to reach vecGoal
		m_Route[ 0 ].vecLocation = vecApex;
		m_Route[ 0 ].iType = (iMoveFlag | bits_MF_TO_DETOUR);

		m_Route[ 1 ].vecLocation = vecGoal;
		m_Route[ 1 ].iType = iMoveFlag | bits_MF_IS_GOAL;

			/*
			WRITE_BYTE(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(MSG_BROADCAST, TE_SHOWLINE);
			WRITE_COORD(MSG_BROADCAST, vecApex.x );
			WRITE_COORD(MSG_BROADCAST, vecApex.y );
			WRITE_COORD(MSG_BROADCAST, vecApex.z );
			WRITE_COORD(MSG_BROADCAST, vecApex.x );
			WRITE_COORD(MSG_BROADCAST, vecApex.y );
			WRITE_COORD(MSG_BROADCAST, vecApex.z + 128 );
			*/

		RouteSimplify( pTarget );
		return TRUE;
	}

// last ditch, try nodes
	if ( FGetNodeRoute( vecGoal ) )
	{
//		ALERT ( at_console, "Can get there on nodes\n" );
		m_vecMoveGoal = vecGoal;
		RouteSimplify( pTarget );
		return TRUE;
	}

	// b0rk
	return FALSE;
}


//=========================================================
// InsertWaypoint - Rebuilds the existing route so that the
// supplied vector and moveflags are the first waypoint in
// the route, and fills the rest of the route with as much
// of the pre-existing route as possible
//=========================================================
void CMBaseMonster :: InsertWaypoint ( Vector vecLocation, int afMoveFlags )
{
	int			i, type;

	
	// we have to save some Index and Type information from the real
	// path_corner or node waypoint that the monster was trying to reach. This makes sure that data necessary 
	// to refresh the original path exists even in the new waypoints that don't correspond directy to a path_corner
	// or node. 
	type = afMoveFlags | (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK);

	for ( i = ROUTE_SIZE-1; i > 0; i-- )
		m_Route[i] = m_Route[i-1];

	m_Route[ m_iRouteIndex ].vecLocation = vecLocation;
	m_Route[ m_iRouteIndex ].iType = type;
}

//=========================================================
// FTriangulate - tries to overcome local obstacles by 
// triangulating a path around them.
//
// iApexDist is how far the obstruction that we are trying
// to triangulate around is from the monster.
//=========================================================
BOOL CMBaseMonster :: FTriangulate ( const Vector &vecStart , const Vector &vecEnd, float flDist, edict_t *pTargetEnt, Vector *pApex )
{
	Vector		vecDir;
	Vector		vecForward;
	Vector		vecLeft;// the spot we'll try to triangulate to on the left
	Vector		vecRight;// the spot we'll try to triangulate to on the right
	Vector		vecTop;// the spot we'll try to triangulate to on the top
	Vector		vecBottom;// the spot we'll try to triangulate to on the bottom
	Vector		vecFarSide;// the spot that we'll move to after hitting the triangulated point, before moving on to our normal goal.
	int			i;
	float		sizeX, sizeZ;

	// If the hull width is less than 24, use 24 because CheckLocalMove uses a min of
	// 24.
	sizeX = pev->size.x;
	if (sizeX < 24.0)
		sizeX = 24.0;
	else if (sizeX > 48.0)
		sizeX = 48.0;
	sizeZ = pev->size.z;
	//if (sizeZ < 24.0)
	//	sizeZ = 24.0;

	vecForward = ( vecEnd - vecStart ).Normalize();

	Vector vecDirUp(0,0,1);
	vecDir = CrossProduct ( vecForward, vecDirUp);

	// start checking right about where the object is, picking two equidistant starting points, one on
	// the left, one on the right. As we progress through the loop, we'll push these away from the obstacle, 
	// hoping to find a way around on either side. pev->size.x is added to the ApexDist in order to help select
	// an apex point that insures that the monster is sufficiently past the obstacle before trying to turn back
	// onto its original course.

	vecLeft = pev->origin + ( vecForward * ( flDist + sizeX ) ) - vecDir * ( sizeX * 3 );
	vecRight = pev->origin + ( vecForward * ( flDist + sizeX ) ) + vecDir * ( sizeX * 3 );
	if (pev->movetype == MOVETYPE_FLY)
	{
		vecTop = pev->origin + (vecForward * flDist) + (vecDirUp * sizeZ * 3);
		vecBottom = pev->origin + (vecForward * flDist) - (vecDirUp *  sizeZ * 3);
	}

	vecFarSide = m_Route[ m_iRouteIndex ].vecLocation;
	
	vecDir = vecDir * sizeX * 2;
	if (pev->movetype == MOVETYPE_FLY)
		vecDirUp = vecDirUp * sizeZ * 2;

	for ( i = 0 ; i < 8; i++ )
	{
// Debug, Draw the triangulation
#if 0
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( vecRight.x );
			WRITE_COORD( vecRight.y );
			WRITE_COORD( vecRight.z );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( vecLeft.x );
			WRITE_COORD( vecLeft.y );
			WRITE_COORD( vecLeft.z );
		MESSAGE_END();
#endif

#if 0
		if (pev->movetype == MOVETYPE_FLY)
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_SHOWLINE );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( vecTop.x );
				WRITE_COORD( vecTop.y );
				WRITE_COORD( vecTop.z );
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_SHOWLINE );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( vecBottom.x );
				WRITE_COORD( vecBottom.y );
				WRITE_COORD( vecBottom.z );
			MESSAGE_END();
		}
#endif

		if ( CheckLocalMove( pev->origin, vecRight, pTargetEnt, NULL ) == LOCALMOVE_VALID )
		{
			if ( CheckLocalMove ( vecRight, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				if ( pApex )
				{
					*pApex = vecRight;
				}

				return TRUE;
			}
		}
		if ( CheckLocalMove( pev->origin, vecLeft, pTargetEnt, NULL ) == LOCALMOVE_VALID )
		{
			if ( CheckLocalMove ( vecLeft, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				if ( pApex )
				{
					*pApex = vecLeft;
				}

				return TRUE;
			}
		}

		if (pev->movetype == MOVETYPE_FLY)
		{
			if ( CheckLocalMove( pev->origin, vecTop, pTargetEnt, NULL ) == LOCALMOVE_VALID)
			{
				if ( CheckLocalMove ( vecTop, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
				{
					if ( pApex )
					{
						*pApex = vecTop;
						//ALERT(at_aiconsole, "triangulate over\n");
					}

					return TRUE;
				}
			}
#if 1
			if ( CheckLocalMove( pev->origin, vecBottom, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				if ( CheckLocalMove ( vecBottom, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
				{
					if ( pApex )
					{
						*pApex = vecBottom;
						//ALERT(at_aiconsole, "triangulate under\n");
					}

					return TRUE;
				}
			}
#endif
		}

		vecRight = vecRight + vecDir;
		vecLeft = vecLeft - vecDir;
		if (pev->movetype == MOVETYPE_FLY)
		{
			vecTop = vecTop + vecDirUp;
			vecBottom = vecBottom - vecDirUp;
		}
	}

	return FALSE;
}

//=========================================================
// Move - take a single step towards the next ROUTE location
//=========================================================
#define DIST_TO_CHECK	200

void CMBaseMonster :: Move ( float flInterval ) 
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	Vector		vecDir;
	Vector		vecApex;
	edict_t	*pTargetEnt;

	// Don't move if no valid route
	if ( FRouteClear() )
	{
		// If we still have a movement goal, then this is probably a route truncated by SimplifyRoute()
		// so refresh it.
		if ( m_movementGoal == MOVEGOAL_NONE || !FRefreshRoute() )
		{
			ALERT( at_aiconsole, "Tried to move with no route!\n" );
			TaskFail();
			return;
		}
	}
	
	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;

// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT("stopmove" ) != 0 )
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();
		return;
	}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 200, 0 );
#endif

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	// local move to waypoint.
	vecDir = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
	flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();
	
	MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );
	ChangeYaw ( pev->yaw_speed );

	// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
	if ( flWaypointDist < DIST_TO_CHECK )
	{
		flCheckDist = flWaypointDist;
	}
	else
	{
		flCheckDist = DIST_TO_CHECK;
	}
	
	if ( (m_Route[ m_iRouteIndex ].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY )
	{
		// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
		pTargetEnt = m_hEnemy;
	}
	else if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT )
	{
		pTargetEnt = m_hTargetEnt;
	}

	// !!!BUGBUG - CheckDist should be derived from ground speed.
	// If this fails, it should be because of some dynamic entity blocking this guy.
	// We've already checked this path, so we should wait and time out if the entity doesn't move
	flDist = 0;
	if ( CheckLocalMove ( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
	{
		// Can't move, stop
		Stop();
		// Blocking entity is in global trace_ent
		CMBaseMonster *pBlocker = GetClassPtr((CMBaseMonster *)VARS(gpGlobals->trace_ent));
		if (pBlocker)
		{
			Blocked( pBlocker->edict() );
		}

		if ( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0 )
		{
			// Can we still move toward our target?
			if ( flDist < m_flGroundSpeed )
			{
				// No, Wait for a second
				m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
				return;
			}
			// Ok, still enough room to take a step
		}
		else 
		{
			// try to triangulate around whatever is in the way.
			if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist, pTargetEnt, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
				RouteSimplify( pTargetEnt );
			}
			else
			{
//				ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
				Stop();
				// Only do this once until your route is cleared
				if ( m_moveWaitTime > 0 && !(m_afMemory & bits_MEMORY_MOVE_FAILED) )
				{
					FRefreshRoute();
					if ( FRouteClear() )
					{
						TaskFail();
					}
					else
					{
						// Don't get stuck
						if ( (gpGlobals->time - m_flMoveWaitFinished) < 0.2 )
							Remember( bits_MEMORY_MOVE_FAILED );

						m_flMoveWaitFinished = gpGlobals->time + 0.1;
					}
				}
				else
				{
//jlb					TaskFail();
					ALERT( at_aiconsole, "%s Failed to move (%d)!\n", STRING(pev->classname), HasMemory( bits_MEMORY_MOVE_FAILED ) );
					//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z );
				}
				return;
			}
		}
	}

	// close enough to the target, now advance to the next target. This is done before actually reaching
	// the target so that we get a nice natural turn while moving.
	if ( ShouldAdvanceRoute( flWaypointDist ) )///!!!BUGBUG- magic number
	{
		AdvanceRoute( flWaypointDist );
	}

	// Might be waiting for a door
	if ( m_flMoveWaitFinished > gpGlobals->time )
	{
		Stop();
		return;
	}

	// UNDONE: this is a hack to quit moving farther than it has looked ahead.
	if (flCheckDist < m_flGroundSpeed * flInterval)
	{
		flInterval = flCheckDist / m_flGroundSpeed;
		// ALERT( at_console, "%.02f\n", flInterval );
	}
	MoveExecute( pTargetEnt, vecDir, flInterval );

	if ( MovementIsComplete() )
	{
		Stop();
		RouteClear();
	}
}


BOOL CMBaseMonster:: ShouldAdvanceRoute( float flWaypointDist )
{
	if ( flWaypointDist <= MONSTER_CUT_CORNER_DIST )
	{
		// ALERT( at_console, "cut %f\n", flWaypointDist );
		return TRUE;
	}

	return FALSE;
}


void CMBaseMonster::MoveExecute( edict_t *pTargetEnt, const Vector &vecDir, float flInterval )
{
//	float flYaw = UTIL_VecToYaw ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin );// build a yaw that points to the goal.
//	WALK_MOVE( ENT(pev), flYaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );
	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	float flStep;
	while (flTotal > 0.001)
	{
		// don't walk more than 16 units or stairs stop working
		flStep = min( 16.0f, flTotal );
		UTIL_MoveToOrigin ( ENT(pev), m_Route[ m_iRouteIndex ].vecLocation, flStep, MOVE_NORMAL );
		flTotal -= flStep;
	}
	// ALERT( at_console, "dist %f\n", m_flGroundSpeed * pev->framerate * flInterval );
}


//=========================================================
// MonsterInit - after a monster is spawned, it needs to 
// be dropped into the world, checked for mobility problems,
// and put on the proper path, if any. This function does
// all of those things after the monster spawns. Any
// initialization that should take place for all monsters
// goes here.
//=========================================================
void CMBaseMonster :: MonsterInit ( void )
{
	// Set fields common to all monsters
	pev->effects		= 0;
	pev->takedamage		= DAMAGE_AIM;
	pev->ideal_yaw		= pev->angles.y;
	pev->max_health		= pev->health;
	pev->deadflag		= DEAD_NO;
	m_IdealMonsterState	= MONSTERSTATE_IDLE;// Assume monster will be idle, until proven otherwise

	m_IdealActivity = ACT_IDLE;

	SetBits (pev->flags, FL_MONSTER);
	if ( pev->spawnflags & SF_MONSTER_HITMONSTERCLIP )
		pev->flags |= FL_MONSTERCLIP;
	
	ClearSchedule();
	RouteClear();
	InitBoneControllers( ); // FIX: should be done in Spawn

	m_iHintNode			= NO_NODE;

	m_afMemory			= MEMORY_CLEAR;

	m_hEnemy			= NULL;
	m_hTargetEnt			= NULL;

	for (int i=0; i < MAX_OLD_ENEMIES; i++)
		m_hOldEnemy[ i ] = NULL;

	m_flDistTooFar		= 1024.0;
	m_flDistLook		= 2048.0;

	// set eye position
	SetEyePosition();

	SetThink( &CMBaseMonster::MonsterInitThink );
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse ( &CMBaseMonster::MonsterUse );
}

//=========================================================
// MonsterInitThink - Calls StartMonster. Startmonster is 
// virtual, but this function cannot be 
//=========================================================
void CMBaseMonster :: MonsterInitThink ( void )
{
	StartMonster();
}

//=========================================================
// StartMonster - final bit of initization before a monster 
// is turned over to the AI. 
//=========================================================
void CMBaseMonster :: StartMonster ( void )
{
	// update capabilities
	if ( LookupActivity ( ACT_RANGE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK1;
	}
	if ( LookupActivity ( ACT_RANGE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK2;
	}
	if ( LookupActivity ( ACT_MELEE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK1;
	}
	if ( LookupActivity ( ACT_MELEE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK2;
	}

	// Raise monster off the floor one unit, then drop to floor
	if ( (pev->movetype != MOVETYPE_FLY) && !FBitSet( pev->spawnflags, SF_MONSTER_FALL_TO_GROUND ) )
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR ( ENT(pev) );

		// Try to move the monster to make sure it's not stuck in a brush.
		if (!WALK_MOVE ( ENT(pev), 0, 0, WALKMOVE_NORMAL ) )
		{
//jlb			ALERT(at_console, "Monster %s stuck in wall", STRING(pev->classname));
//jlb stuck
//jlb			pev->effects = EF_BRIGHTFIELD;
		}
	}
	else 
	{
		pev->flags &= ~FL_ONGROUND;
	}
	
	if ( !FStringNull(pev->target) )// this monster has a target
	{
		// Find the monster's initial target entity, stash it
		m_pGoalEnt = FIND_ENTITY_BY_TARGETNAME ( NULL, STRING( pev->target ) );

		if ( !m_pGoalEnt )
		{
			ALERT(at_error, "ReadyMonster()--%s couldn't find target %s", STRING(pev->classname), STRING(pev->target));
		}
		else
		{
			// Monster will start turning towards his destination
			MakeIdealYaw ( m_pGoalEnt->v.origin );

			// JAY: How important is this error message?  Big Momma doesn't obey this rule, so I took it out.
#if 0
			// At this point, we expect only a path_corner as initial goal
			if (!FClassnameIs( m_pGoalEnt->pev, "path_corner"))
			{
				ALERT(at_warning, "ReadyMonster--monster's initial goal '%s' is not a path_corner", STRING(pev->target));
			}
#endif

			// set the monster up to walk a path corner path. 
			// !!!BUGBUG - this is a minor bit of a hack.
			// JAYJAY
			m_movementGoal = MOVEGOAL_PATHCORNER;
			
			if ( pev->movetype == MOVETYPE_FLY )
				m_movementActivity = ACT_FLY;
			else
				m_movementActivity = ACT_WALK;

			if ( !FRefreshRoute() )
			{
				ALERT ( at_aiconsole, "Can't Create Route!\n" );
			}
			SetState( MONSTERSTATE_IDLE );
			ChangeSchedule( GetScheduleOfType( SCHED_IDLE_WALK ) );
		}
	}
	
	//SetState ( m_IdealMonsterState );
	//SetActivity ( m_IdealActivity );

	// Delay drop to floor to make sure each door in the level has had its chance to spawn
	// Spread think times so that they don't all happen at the same time (Carmack)
	SetThink ( &CMBaseMonster::CallMonsterThink );
	pev->nextthink += RANDOM_FLOAT(0.1, 0.4); // spread think times.
	
	if ( !FStringNull(pev->targetname) )// wait until triggered
	{
		SetState( MONSTERSTATE_IDLE );
		// UNDONE: Some scripted sequence monsters don't have an idle?
		SetActivity( ACT_IDLE );
		ChangeSchedule( GetScheduleOfType( SCHED_WAIT_TRIGGER ) );
	}

	// Notify normal game engine of monster classify
	pev->iuser4 = Classify();
}


void CMBaseMonster :: MovementComplete( void ) 
{ 
	switch( m_iTaskStatus )
	{
	case TASKSTATUS_NEW:
	case TASKSTATUS_RUNNING:
		m_iTaskStatus = TASKSTATUS_RUNNING_TASK;
		break;

	case TASKSTATUS_RUNNING_MOVEMENT:
		TaskComplete();
		break;
	
	case TASKSTATUS_RUNNING_TASK:
		ALERT( at_error, "Movement completed twice!\n" );
		break;

	case TASKSTATUS_COMPLETE:		
		break;
	}
	m_movementGoal = MOVEGOAL_NONE;
}


int CMBaseMonster::TaskIsRunning( void )
{
	if ( m_iTaskStatus != TASKSTATUS_COMPLETE && 
		 m_iTaskStatus != TASKSTATUS_RUNNING_MOVEMENT )
		 return 1;

	return 0;
}

//=========================================================
// IRelationship - returns an integer that describes the 
// relationship between two types of monster.
//=========================================================
int CMBaseMonster::IRelationship ( CMBaseEntity *pTarget )
{
	return IRelationshipByClass( pTarget->Classify() );
}
int CMBaseMonster::IRelationship ( int iTargetClass )
{
	return IRelationshipByClass( iTargetClass );
}
int CMBaseMonster::IRelationshipByClass ( int iClass )
{
	static int iEnemy[16][16] =
	{			 //   NONE	 MACH	 PLYR	 HPASS	 HMIL	 AMIL	 APASS	 AMONST	APREY	 APRED	 INSECT	PLRALY	PBWPN	ABWPN	RXPIT	RXSHK
	/*NONE*/		{ R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO,	R_NO,	R_NO,	R_NO,	R_NO	},
	/*MACHINE*/		{ R_NO	,R_NO	,R_DL	,R_DL	,R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_DL,	R_DL,	R_DL,	R_DL,	R_DL	},
	/*PLAYER*/		{ R_NO	,R_DL	,R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO,	R_DL,	R_DL,	R_DL,	R_DL	},
	/*HUMANPASSIVE*/{ R_NO	,R_FR	,R_AL	,R_AL	,R_HT	,R_DL	,R_DL	,R_HT	,R_DL	,R_DL	,R_NO	,R_AL,	R_NO,	R_NO,	R_FR,	R_FR	},
	/*HUMANMILITAR*/{ R_NO	,R_NO	,R_DL	,R_DL	,R_AL	,R_HT	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_DL,	R_NO,	R_NO,	R_HT,	R_HT	},
	/*ALIENMILITAR*/{ R_NO	,R_DL	,R_DL	,R_DL	,R_HT	,R_AL	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_DL,	R_NO,	R_NO,	R_DL,	R_HT	},
	/*ALIENPASSIVE*/{ R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO,	R_NO,	R_NO,	R_NO,	R_NO	},
	/*ALIENMONSTER*/{ R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_NO	,R_DL,	R_NO,	R_NO,	R_NO,	R_NO	},
	/*ALIENPREY   */{ R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO	,R_NO	,R_NO	,R_FR	,R_NO	,R_DL,	R_NO,	R_NO,	R_FR,	R_NO	},
	/*ALIENPREDATO*/{ R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO	,R_NO	,R_HT	,R_NO	,R_NO	,R_DL,	R_NO,	R_NO,	R_DL,	R_DL	},
	/*INSECT*/		{ R_NO	,R_FR	,R_FR	,R_FR	,R_FR	,R_NO	,R_FR	,R_FR	,R_FR	,R_FR	,R_NO	,R_FR,	R_NO,	R_NO,	R_NO,	R_NO	},
	/*PLAYERALLY*/	{ R_NO	,R_DL	,R_AL	,R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO,	R_NO,	R_NO,	R_DL,	R_DL	},
	/*PBIOWEAPON*/	{ R_NO	,R_NO	,R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_NO,	R_NO,	R_DL,	R_DL,	R_DL	},
	/*ABIOWEAPON*/	{ R_NO	,R_NO	,R_DL	,R_DL	,R_DL	,R_AL	,R_NO	,R_DL	,R_DL	,R_NO	,R_NO	,R_DL,	R_DL,	R_NO,	R_DL,	R_DL	},
	/*RXPITDRONE*/	{ R_NO	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_DL	,R_NO	,R_DL	,R_DL	,R_NO	,R_DL,	R_NO,	R_NO,	R_AL,	R_AL	},
	/*RXSHOCKTRP*/	{ R_NO	,R_DL	,R_HT	,R_DL	,R_HT	,R_HT	,R_DL	,R_NO	,R_NO	,R_DL	,R_NO	,R_DL,	R_NO,	R_NO,	R_AL,	R_AL	}
	};

	return iEnemy[ Classify() ][ iClass ];
}

//=========================================================
// FindCover - tries to find a nearby node that will hide
// the caller from its enemy. 
//
// If supplied, search will return a node at least as far
// away as MinDist, but no farther than MaxDist. 
// if MaxDist isn't supplied, it defaults to a reasonable 
// value
//=========================================================
// UNDONE: Should this find the nearest node?

//float CGraph::PathLength( int iStart, int iDest, int iHull, int afCapMask )

BOOL CMBaseMonster :: FindCover ( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist )
{
	int i;
	int iMyHullIndex;
	int iMyNode;
	int iThreatNode;
	float flDist;
	Vector	vecLookersOffset;
	TraceResult tr;

	if ( !flMaxDist )
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if ( flMinDist > 0.5 * flMaxDist)
	{
#if _DEBUG
		ALERT ( at_console, "FindCover MinDist (%.0f) too close to MaxDist (%.0f)\n", flMinDist, flMaxDist );
#endif
		flMinDist = 0.5 * flMaxDist;
	}

	if ( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
	{
		ALERT ( at_aiconsole, "Graph not ready for findcover!\n" );
		return FALSE;
	}

	iMyNode = WorldGraph.FindNearestNode( pev->origin, this );
	iThreatNode = WorldGraph.FindNearestNode ( vecThreat, this );
	iMyHullIndex = WorldGraph.HullIndex( this );

	if ( iMyNode == NO_NODE )
	{
		ALERT ( at_aiconsole, "FindCover() - %s has no nearest node!\n", STRING(pev->classname));
		return FALSE;
	}
	if ( iThreatNode == NO_NODE )
	{
		// ALERT ( at_aiconsole, "FindCover() - Threat has no nearest node!\n" );
		iThreatNode = iMyNode;
		// return FALSE;
	}

	vecLookersOffset = vecThreat + vecViewOffset;// calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for ( i = 0 ; i < WorldGraph.m_cNodes ; i++ )
	{
		int nodeNumber = (i + WorldGraph.m_iLastCoverSearch) % WorldGraph.m_cNodes;

		CNode &node = WorldGraph.Node( nodeNumber );
		WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.

		// could use an optimization here!!
		flDist = ( pev->origin - node.m_vecOrigin ).Length();

		// DON'T do the trace check on a node that is farther away than a node that we've already found to 
		// provide cover! Also make sure the node is within the mins/maxs of the search.
		if ( flDist >= flMinDist && flDist < flMaxDist )
		{
			UTIL_TraceLine ( node.m_vecOrigin + vecViewOffset, vecLookersOffset, ignore_monsters, ignore_glass,  ENT(pev), &tr );

			// if this node will block the threat's line of sight to me...
			if ( tr.flFraction != 1.0 )
			{
				// ..and is also closer to me than the threat, or the same distance from myself and the threat the node is good.
				if ( ( iMyNode == iThreatNode ) || WorldGraph.PathLength( iMyNode, nodeNumber, iMyHullIndex, m_afCapability ) <= WorldGraph.PathLength( iThreatNode, nodeNumber, iMyHullIndex, m_afCapability ) )
				{
					if ( FValidateCover ( node.m_vecOrigin ) && MoveToLocation( ACT_RUN, 0, node.m_vecOrigin ) )
					{
						/*
						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
							WRITE_BYTE( TE_SHOWLINE);
							
							WRITE_COORD( node.m_vecOrigin.x );
							WRITE_COORD( node.m_vecOrigin.y );
							WRITE_COORD( node.m_vecOrigin.z );

							WRITE_COORD( vecLookersOffset.x );
							WRITE_COORD( vecLookersOffset.y );
							WRITE_COORD( vecLookersOffset.z );
						MESSAGE_END();
						*/

						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}


//=========================================================
// BuildNearestRoute - tries to build a route as close to the target
// as possible, even if there isn't a path to the final point.
//
// If supplied, search will return a node at least as far
// away as MinDist from vecThreat, but no farther than MaxDist. 
// if MaxDist isn't supplied, it defaults to a reasonable 
// value
//=========================================================
BOOL CMBaseMonster :: BuildNearestRoute ( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist )
{
	int i;
	int iMyHullIndex;
	int iMyNode;
	float flDist;
	Vector	vecLookersOffset;
	TraceResult tr;

	if ( !flMaxDist )
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if ( flMinDist > 0.5 * flMaxDist)
	{
#if _DEBUG
		ALERT ( at_console, "FindCover MinDist (%.0f) too close to MaxDist (%.0f)\n", flMinDist, flMaxDist );
#endif
		flMinDist = 0.5 * flMaxDist;
	}

	if ( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
	{
		ALERT ( at_aiconsole, "Graph not ready for BuildNearestRoute!\n" );
		return FALSE;
	}

	iMyNode = WorldGraph.FindNearestNode( pev->origin, this );
	iMyHullIndex = WorldGraph.HullIndex( this );

	if ( iMyNode == NO_NODE )
	{
		ALERT ( at_aiconsole, "BuildNearestRoute() - %s has no nearest node!\n", STRING(pev->classname));
		return FALSE;
	}

	vecLookersOffset = vecThreat + vecViewOffset;// calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for ( i = 0 ; i < WorldGraph.m_cNodes ; i++ )
	{
		int nodeNumber = (i + WorldGraph.m_iLastCoverSearch) % WorldGraph.m_cNodes;

		CNode &node = WorldGraph.Node( nodeNumber );
		WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.

		// can I get there?
		if (WorldGraph.NextNodeInRoute( iMyNode, nodeNumber, iMyHullIndex, 0 ) != iMyNode)
		{
			flDist = ( vecThreat - node.m_vecOrigin ).Length();

			// is it close?
			if ( flDist > flMinDist && flDist < flMaxDist)
			{
				// can I see where I want to be from there?
				UTIL_TraceLine( node.m_vecOrigin + pev->view_ofs, vecLookersOffset, ignore_monsters, edict(), &tr );

				if (tr.flFraction == 1.0)
				{
					// try to actually get there
					if ( BuildRoute ( node.m_vecOrigin, bits_MF_TO_LOCATION, NULL ) )
					{
						flMaxDist = flDist;
						m_vecMoveGoal = node.m_vecOrigin;
						return TRUE; // UNDONE: keep looking for something closer!
					}
				}
			}
		}
	}

	return FALSE;
}



//=========================================================
// BestVisibleEnemy - this functions searches the link
// list whose head is the caller's m_pLink field, and returns
// a pointer to the enemy entity in that list that is nearest the 
// caller.
//
// !!!UNDONE - currently, this only returns the closest enemy.
// we'll want to consider distance, relationship, attack types, back turned, etc.
//=========================================================
edict_t *CMBaseMonster :: BestVisibleEnemy ( void )
{
	int			iNearest;
	int			iDist;
	int			iBestRelationship;
	edict_t *pReturn;
	edict_t *pEnt;
	int edictList_index = 0;

	iNearest = 8192;// so first visible entity will become the closest.

	iBestRelationship = R_NO;

	pReturn = NULL;

	while (edictList_index < m_edictList_count)
	{
		pEnt = m_edictList[edictList_index];
		
		if ( UTIL_IsPlayer(pEnt) )
		{
			// it's a player...
			if ( UTIL_IsAlive(pEnt) )
			{
				// repeat2
				if ( IRelationshipByClass( CLASS_PLAYER ) > iBestRelationship )
				{
					iBestRelationship = IRelationshipByClass( CLASS_PLAYER );
					iNearest = ( pEnt->v.origin - pev->origin ).Length();
					pReturn = pEnt;
				}
				else if ( IRelationshipByClass( CLASS_PLAYER ) == iBestRelationship )
				{
					iDist = ( pEnt->v.origin - pev->origin ).Length();
					
					if ( iDist <= iNearest )
					{
						iNearest = iDist;
						iBestRelationship = IRelationshipByClass( CLASS_PLAYER );
						pReturn = pEnt;
					}
				}
			}
		}
		else if (pEnt->v.euser4 != NULL)
		{
			// it's a monstermod monster...
			CMBaseMonster *pNextEnt = GetClassPtr((CMBaseMonster *)VARS(pEnt));
			if ( pNextEnt->IsAlive() )
			{
				if ( IRelationship( pNextEnt ) > iBestRelationship )
				{
					// this entity is disliked MORE than the entity that we 
					// currently think is the best visible enemy. No need to do 
					// a distance check, just get mad at this one for now.
					iBestRelationship = IRelationship ( pNextEnt );
					iNearest = ( pNextEnt->pev->origin - pev->origin ).Length();
					pReturn = pEnt;
				}
				else if ( IRelationship( pNextEnt ) == iBestRelationship )
				{
					// this entity is disliked just as much as the entity that
					// we currently think is the best visible enemy, so we only
					// get mad at it if it is closer.
					iDist = ( pNextEnt->pev->origin - pev->origin ).Length();

					if ( iDist <= iNearest )
					{
						iNearest = iDist;
						iBestRelationship = IRelationship ( pNextEnt );
						pReturn = pEnt;
					}
				}
			}
		}
		else
		{
			// it's a normal game entity...
			if ( UTIL_IsAlive(pEnt) )
			{
				//repeat3
				if ( IRelationship( pEnt->v.iuser4 ) > iBestRelationship )
				{
					iBestRelationship = IRelationship( pEnt->v.iuser4 );
					iNearest = ( pEnt->v.origin - pev->origin ).Length();
					pReturn = pEnt;
				}
				else if ( IRelationship( pEnt->v.iuser4 ) == iBestRelationship )
				{
					iDist = ( pEnt->v.origin - pev->origin ).Length();

					if ( iDist <= iNearest )
					{
						iNearest = iDist;
						iBestRelationship = IRelationship( pEnt->v.iuser4 );
						pReturn = pEnt;
					}
				}
			}
		}

		edictList_index++;
	}

	return pReturn;
}


//=========================================================
// MakeIdealYaw - gets a yaw value for the caller that would
// face the supplied vector. Value is stuffed into the monster's
// ideal_yaw
//=========================================================
void CMBaseMonster :: MakeIdealYaw( Vector vecTarget )
{
	Vector	vecProjection;
	
	// strafing monster needs to face 90 degrees away from its goal
	if ( m_movementActivity == ACT_STRAFE_LEFT )
	{
		vecProjection.x = -vecTarget.y;
		vecProjection.y = vecTarget.x;

		pev->ideal_yaw = UTIL_VecToYaw( vecProjection - pev->origin );
	}
	else if ( m_movementActivity == ACT_STRAFE_RIGHT )
	{
		vecProjection.x = vecTarget.y;
		vecProjection.y = vecTarget.x;

		pev->ideal_yaw = UTIL_VecToYaw( vecProjection - pev->origin );
	}
	else
	{
		pev->ideal_yaw = UTIL_VecToYaw ( vecTarget - pev->origin );
	}
}

//=========================================================
// FlYawDiff - returns the difference ( in degrees ) between
// monster's current yaw and ideal_yaw
//
// Positive result is left turn, negative is right turn
//=========================================================
float	CMBaseMonster::FlYawDiff ( void )
{
	float	flCurrentYaw;

	flCurrentYaw = UTIL_AngleMod( pev->angles.y );

	if ( flCurrentYaw == pev->ideal_yaw )
	{
		return 0;
	}


	return UTIL_AngleDiff( pev->ideal_yaw, flCurrentYaw );
}


//=========================================================
// Changeyaw - turns a monster towards its ideal_yaw
//=========================================================
float CMBaseMonster::ChangeYaw ( int yawSpeed )
{
	float		ideal, current, move, speed;

	current = UTIL_AngleMod( pev->angles.y );
	ideal = pev->ideal_yaw;
	if (current != ideal)
	{
		// -SamVanheer
		if ( m_flLastYawTime == 0 )
		{
			m_flLastYawTime = gpGlobals->time - gpGlobals->frametime;
		}

		float delta = gpGlobals->time - m_flLastYawTime;
		if ( delta > 0.25 )
			delta = 0.25;
		
		// let server operators modify the multiplier coeficient -Giegue
		float multiplier = monster_turn_coeficient->value;
		if ( multiplier < 0.1 || multiplier > 10.0 )
			multiplier = 1.75;

		speed = (float)yawSpeed * delta * multiplier;
		move = ideal - current;

		if (ideal > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}

		if (move > 0)
		{// turning to the monster's left
			if (move > speed)
				move = speed;
		}
		else
		{// turning to the monster's right
			if (move < -speed)
				move = -speed;
		}
		
		pev->angles.y = UTIL_AngleMod (current + move);

		// turn head in desired direction only if they have a turnable head
		if (m_afCapability & bits_CAP_TURN_HEAD)
		{
			float yaw = pev->ideal_yaw - pev->angles.y;
			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;
			// yaw *= 0.8;
			SetBoneController( 0, yaw );
		}
	}
	else
		move = 0;

	return move;
}

//=========================================================
// VecToYaw - turns a directional vector into a yaw value
// that points down that vector.
//=========================================================
float	CMBaseMonster::VecToYaw ( Vector vecDir )
{
	if (vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0)
		return pev->angles.y;

	return UTIL_VecToYaw( vecDir );
}


//=========================================================
// SetEyePosition
//
// queries the monster's model for $eyeposition and copies
// that vector to the monster's view_ofs
//
//=========================================================
void CMBaseMonster :: SetEyePosition ( void )
{
	Vector  vecEyePosition;
	void	*pmodel = GET_MODEL_PTR( ENT(pev) );

	GetEyePosition( pmodel, vecEyePosition );

	pev->view_ofs = vecEyePosition;

	if ( pev->view_ofs == g_vecZero )
	{
		ALERT ( at_aiconsole, "%s has no view_ofs!\n", STRING ( pev->classname ) );
	}
}

void CMBaseMonster :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case MONSTER_EVENT_BODYDROP_HEAVY:
		if ( pev->flags & FL_ONGROUND )
		{
			if ( RANDOM_LONG( 0, 1 ) == 0 )
			{
				EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "common/bodydrop3.wav", 1, ATTN_NORM, 0, 90 );
			}
			else
			{
				EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "common/bodydrop4.wav", 1, ATTN_NORM, 0, 90 );
			}
		}
		break;

	case MONSTER_EVENT_BODYDROP_LIGHT:
		if ( pev->flags & FL_ONGROUND )
		{
			if ( RANDOM_LONG( 0, 1 ) == 0 )
			{
				EMIT_SOUND( ENT(pev), CHAN_BODY, "common/bodydrop3.wav", 1, ATTN_NORM );
			}
			else
			{
				EMIT_SOUND( ENT(pev), CHAN_BODY, "common/bodydrop4.wav", 1, ATTN_NORM );
			}
		}
		break;

	case MONSTER_EVENT_SWISHSOUND:
		{
			// NO MONSTER may use this anim event unless that monster's precache precaches this sound!!!
			EMIT_SOUND( ENT(pev), CHAN_BODY, "zombie/claw_miss2.wav", 1, ATTN_NORM );
			break;
		}

	default:
		ALERT( at_aiconsole, "Unhandled animation event %d for %s\n", pEvent->event, STRING(pev->classname) );
		break;

	}
}


// Combat

Vector CMBaseMonster :: GetGunPosition( )
{
	UTIL_MakeVectors(pev->angles);

	// Vector vecSrc = pev->origin + gpGlobals->v_forward * 10;
	//vecSrc.z = pevShooter->absmin.z + pevShooter->size.z * 0.7;
	//vecSrc.z = pev->origin.z + (pev->view_ofs.z - 4);
	Vector vecSrc = pev->origin 
					+ gpGlobals->v_forward * m_HackedGunPos.y 
					+ gpGlobals->v_right * m_HackedGunPos.x 
					+ gpGlobals->v_up * m_HackedGunPos.z;

	return vecSrc;
}





//=========================================================
// NODE GRAPH
//=========================================================





//=========================================================
// FGetNodeRoute - tries to build an entire node path from
// the callers origin to the passed vector. If this is 
// possible, ROUTE_SIZE waypoints will be copied into the
// callers m_Route. TRUE is returned if the operation 
// succeeds (path is valid) or FALSE if failed (no path 
// exists )
//=========================================================
BOOL CMBaseMonster :: FGetNodeRoute ( Vector vecDest )
{
	int iPath[ MAX_PATH_SIZE ];
	int iSrcNode, iDestNode;
	int iResult;
	int i;
	int iNumToCopy;

	iSrcNode = WorldGraph.FindNearestNode ( pev->origin, this );
	iDestNode = WorldGraph.FindNearestNode ( vecDest, this );

	if ( iSrcNode == -1 )
	{
		// no node nearest self
//		ALERT ( at_aiconsole, "FGetNodeRoute: No valid node near self!\n" );
		return FALSE;
	}
	else if ( iDestNode == -1 )
	{
		// no node nearest target
//		ALERT ( at_aiconsole, "FGetNodeRoute: No valid node near target!\n" );
		return FALSE;
	}

	// valid src and dest nodes were found, so it's safe to proceed with
	// find shortest path
	int iNodeHull = WorldGraph.HullIndex( this ); // make this a monster virtual function
	iResult = WorldGraph.FindShortestPath ( iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability );

	if ( !iResult )
	{
#if 1
		ALERT ( at_aiconsole, "No Path from %d to %d!\n", iSrcNode, iDestNode );
		return FALSE;
#else
		BOOL bRoutingSave = WorldGraph.m_fRoutingComplete;
		WorldGraph.m_fRoutingComplete = FALSE;
		iResult = WorldGraph.FindShortestPath(iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability);
		WorldGraph.m_fRoutingComplete = bRoutingSave;
		if ( !iResult )
		{
			ALERT ( at_aiconsole, "No Path from %d to %d!\n", iSrcNode, iDestNode );
			return FALSE;
		}
		else
		{
			ALERT ( at_aiconsole, "Routing is inconsistent!" );
		}
#endif
	}

	// there's a valid path within iPath now, so now we will fill the route array
	// up with as many of the waypoints as it will hold.
	
	// don't copy ROUTE_SIZE entries if the path returned is shorter
	// than ROUTE_SIZE!!!
	if ( iResult < ROUTE_SIZE )
	{
		iNumToCopy = iResult;
	}
	else
	{
		iNumToCopy = ROUTE_SIZE;
	}
	
	for ( i = 0 ; i < iNumToCopy; i++ )
	{
		m_Route[ i ].vecLocation = WorldGraph.m_pNodes[ iPath[ i ] ].m_vecOrigin;
		m_Route[ i ].iType = bits_MF_TO_NODE;
	}
	
	if ( iNumToCopy < ROUTE_SIZE )
	{
		m_Route[ iNumToCopy ].vecLocation = vecDest;
		m_Route[ iNumToCopy ].iType |= bits_MF_IS_GOAL;
	}

	return TRUE;
}

//=========================================================
// FindHintNode
//=========================================================
int CMBaseMonster :: FindHintNode ( void )
{
	int i;
	TraceResult tr;

	if ( !WorldGraph.m_fGraphPresent )
	{
		ALERT ( at_aiconsole, "find_hintnode: graph not ready!\n" );
		return NO_NODE;
	}

	if ( WorldGraph.m_iLastActiveIdleSearch >= WorldGraph.m_cNodes )
	{
		WorldGraph.m_iLastActiveIdleSearch = 0;
	}

	for ( i = 0; i < WorldGraph.m_cNodes ; i++ )
	{
		int nodeNumber = (i + WorldGraph.m_iLastActiveIdleSearch) % WorldGraph.m_cNodes;
		CNode &node = WorldGraph.Node( nodeNumber );

		if ( node.m_sHintType )
		{
			// this node has a hint. Take it if it is visible, the monster likes it, and the monster has an animation to match the hint's activity.
			if ( FValidateHintType ( node.m_sHintType ) )
			{
				if ( !node.m_sHintActivity || LookupActivity ( node.m_sHintActivity ) != ACTIVITY_NOT_AVAILABLE )
				{
					UTIL_TraceLine ( pev->origin + pev->view_ofs, node.m_vecOrigin + pev->view_ofs, ignore_monsters, ENT(pev), &tr );

					if ( tr.flFraction == 1.0 )
					{
						WorldGraph.m_iLastActiveIdleSearch = nodeNumber + 1; // next monster that searches for hint nodes will start where we left off.
						return nodeNumber;// take it!
					}
				}
			}
		}
	}

	WorldGraph.m_iLastActiveIdleSearch = 0;// start at the top of the list for the next search.

	return NO_NODE;
}
			

void CMBaseMonster::ReportAIState( void )
{
	ALERT_TYPE level = at_console;

	static const char *pStateNames[] = { "None", "Idle", "Combat", "Alert", "Hunt", "Prone", "Scripted", "Dead" };

	ALERT( level, "%s: ", STRING(pev->classname) );
	if ( (int)m_MonsterState < ARRAYSIZE(pStateNames) )
		ALERT( level, "State: %s, ", pStateNames[m_MonsterState] );
	int i = 0;
	while ( activity_map[i].type != 0 )
	{
		if ( activity_map[i].type == (int)m_Activity )
		{
			ALERT( level, "Activity %s, ", activity_map[i].name );
			break;
		}
		i++;
	}

	if ( m_pSchedule )
	{
		const char *pName = NULL;
		pName = m_pSchedule->pName;
		if ( !pName )
			pName = "Unknown";
		ALERT( level, "Schedule %s, ", pName );
		Task_t *pTask = GetTask();
		if ( pTask )
			ALERT( level, "Task %d (#%d), ", pTask->iTask, m_iScheduleIndex );
	}
	else
		ALERT( level, "No Schedule, " );

	if ( m_hEnemy != NULL )
		ALERT( level, "\nEnemy is %s", STRING(m_hEnemy->v.classname) );
	else
		ALERT( level, "No enemy" );

	if ( IsMoving() )
	{
		ALERT( level, " Moving " );
		if ( m_flMoveWaitFinished > gpGlobals->time )
			ALERT( level, ": Stopped for %.2f. ", m_flMoveWaitFinished - gpGlobals->time );
		else if ( m_IdealActivity == GetStoppedActivity() )
			ALERT( level, ": In stopped anim. " );
	}

	ALERT( level, "\n" );
	ALERT( level, "Yaw speed:%3.1f,Health: %3.1f\n", pev->yaw_speed, pev->health );
	if ( pev->spawnflags & SF_MONSTER_PRISONER )
		ALERT( level, " PRISONER! " );
	if ( pev->spawnflags & SF_MONSTER_PREDISASTER )
		ALERT( level, " Pre-Disaster! " );
	ALERT( level, "\n" );
}

//=========================================================
// KeyValue
//
// !!! netname entvar field is used in squadmonster for groupname!!!
//=========================================================
void CMBaseMonster :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "TriggerTarget"))
	{
		m_iszTriggerTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TriggerCondition"))
	{
		m_iTriggerCondition = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "displayname"))
	{
		m_szMonsterName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "classify"))
	{
		m_iClassifyOverride = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "model"))
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CMBaseToggle::KeyValue( pkvd );
	}
}

//=========================================================
// FCheckAITrigger - checks the monster's AI Trigger Conditions,
// if there is a condition, then checks to see if condition is 
// met. If yes, the monster's TriggerTarget is fired.
//
// Returns TRUE if the target is fired.
//=========================================================
BOOL CMBaseMonster :: FCheckAITrigger ( void )
{
	BOOL fFireTarget;

	if ( m_iTriggerCondition == AITRIGGER_NONE )
	{
		// no conditions, so this trigger is never fired.
		return FALSE; 
	}

	fFireTarget = FALSE;

	switch ( m_iTriggerCondition )
	{
	case AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER:
		if ( m_hEnemy != NULL && UTIL_IsPlayer(m_hEnemy) && HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_SEEPLAYER_UNCONDITIONAL:
		if ( HasConditions ( bits_COND_SEE_CLIENT ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_SEEPLAYER_NOT_IN_COMBAT:
		if ( HasConditions ( bits_COND_SEE_CLIENT ) && 
			 m_MonsterState != MONSTERSTATE_COMBAT	&& 
			 m_MonsterState != MONSTERSTATE_PRONE	&& 
			 m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_TAKEDAMAGE:
		if ( m_afConditions & ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_DEATH:
		if ( pev->deadflag != DEAD_NO )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_HALFHEALTH:
		if ( IsAlive() && pev->health <= ( pev->max_health / 2 ) )
		{
			fFireTarget = TRUE;
		}
		break;
/*

  // !!!UNDONE - no persistant game state that allows us to track these two. 

	case AITRIGGER_SQUADMEMBERDIE:
		break;
	case AITRIGGER_SQUADLEADERDIE:
		break;
*/
	case AITRIGGER_HEARWORLD:
		break;
	case AITRIGGER_HEARPLAYER:
		break;
	case AITRIGGER_HEARCOMBAT:
		break;
	}

	if ( fFireTarget )
	{
		// fire the target, then set the trigger conditions to NONE so we don't fire again
		ALERT ( at_aiconsole, "AI Trigger Fire Target\n" );
		FireTargets( STRING( m_iszTriggerTarget ), this->edict(), this->edict(), USE_TOGGLE, 0 );
		m_iTriggerCondition = AITRIGGER_NONE;
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// FindLateralCover - attempts to locate a spot in the world
// directly to the left or right of the caller that will
// conceal them from view of pSightEnt
//=========================================================
#define	COVER_CHECKS	5// how many checks are made
#define COVER_DELTA		48// distance between checks

BOOL CMBaseMonster :: FindLateralCover ( const Vector &vecThreat, const Vector &vecViewOffset )
{
	TraceResult	tr;
	Vector	vecBestOnLeft;
	Vector	vecBestOnRight;
	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	int		i;

	UTIL_MakeVectors ( pev->angles );
	vecStepRight = gpGlobals->v_right * COVER_DELTA;
	vecStepRight.z = 0; 
	
	vecLeftTest = vecRightTest = pev->origin;

	for ( i = 0 ; i < COVER_CHECKS ; i++ )
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine( vecThreat + vecViewOffset, vecLeftTest + pev->view_ofs, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr);
		
		if (tr.flFraction != 1.0)
		{
			if ( FValidateCover ( vecLeftTest ) && CheckLocalMove( pev->origin, vecLeftTest, NULL, NULL ) == LOCALMOVE_VALID )
			{
				if ( MoveToLocation( ACT_RUN, 0, vecLeftTest ) )
				{
					return TRUE;
				}
			}
		}
		
		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine(vecThreat + vecViewOffset, vecRightTest + pev->view_ofs, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr);
		
		if ( tr.flFraction != 1.0 )
		{
			if (  FValidateCover ( vecRightTest ) && CheckLocalMove( pev->origin, vecRightTest, NULL, NULL ) == LOCALMOVE_VALID )
			{
				if ( MoveToLocation( ACT_RUN, 0, vecRightTest ) )
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}


Vector CMBaseMonster :: ShootAtEnemy( const Vector &shootOrigin )
{
	edict_t *pEnemy = m_hEnemy;

	if ( pEnemy )
	{
		return ( (UTIL_BodyTarget(pEnemy, shootOrigin ) - pEnemy->v.origin) + m_vecEnemyLKP - shootOrigin ).Normalize();
	}
	else
		return gpGlobals->v_forward;
}



//=========================================================
// FacingIdeal - tells us if a monster is facing its ideal
// yaw. Created this function because many spots in the 
// code were checking the yawdiff against this magic
// number. Nicer to have it in one place if we're gonna
// be stuck with it.
//=========================================================
BOOL CMBaseMonster :: FacingIdeal( void )
{
	if ( fabs( FlYawDiff() ) <= 0.006 )//!!!BUGBUG - no magic numbers!!!
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// FCanActiveIdle
//=========================================================
BOOL CMBaseMonster :: FCanActiveIdle ( void )
{
	/*
	if ( m_MonsterState == MONSTERSTATE_IDLE && m_IdealMonsterState == MONSTERSTATE_IDLE && !IsMoving() )
	{
		return TRUE;
	}
	*/
	return FALSE;
}


void CMBaseMonster::PlaySentence( const char *pszSentence, float duration, float volume, float attenuation )
{
	if ( pszSentence && IsAlive() )
	{
		if ( pszSentence[0] == '!' )
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, PITCH_NORM );
//jlb		else
//jlb			SENTENCEG_PlayRndSz( edict(), pszSentence, volume, attenuation, 0, PITCH_NORM );
	}
}


void CMBaseMonster::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CMBaseEntity *pListener )
{ 
	PlaySentence( pszSentence, duration, volume, attenuation );
}


void CMBaseMonster::SentenceStop( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "common/null.wav", 1.0, ATTN_IDLE );
}


void CMBaseMonster::CorpseFallThink( void )
{
	if ( pev->flags & FL_ONGROUND )
	{
		SetThink ( NULL );

		SetSequenceBox( );
		UTIL_SetOrigin( pev, pev->origin );// link into world.
	}
	else
		pev->nextthink = gpGlobals->time + 0.1;
}

// Call after animation/pose is set up
void CMBaseMonster :: MonsterInitDead( void )
{
	InitBoneControllers();

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_TOSS;// so he'll fall to ground

	pev->frame = 0;
	ResetSequenceInfo( );
	pev->framerate = 0;
	
	// Copy health
	pev->max_health		= pev->health;
	pev->deadflag		= DEAD_DEAD;
	
	UTIL_SetSize(pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );

	// Setup health counters, etc.
	BecomeDead();
	SetThink( &CMBaseMonster::CorpseFallThink );
	pev->nextthink = gpGlobals->time + 0.5;
}

//=========================================================
// BBoxIsFlat - check to see if the monster's bounding box
// is lying flat on a surface (traces from all four corners
// are same length.)
//=========================================================
BOOL CMBaseMonster :: BBoxFlat ( void )
{
	TraceResult	tr;
	Vector		vecPoint;
	float		flXSize, flYSize;
	float		flLength;
	float		flLength2;

	flXSize = pev->size.x / 2;
	flYSize = pev->size.y / 2;

	vecPoint.x = pev->origin.x + flXSize;
	vecPoint.y = pev->origin.y + flYSize;
	vecPoint.z = pev->origin.z;

	UTIL_TraceLine ( vecPoint, vecPoint - Vector ( 0, 0, 100 ), ignore_monsters, ENT(pev), &tr );
	flLength = (vecPoint - tr.vecEndPos).Length();

	vecPoint.x = pev->origin.x - flXSize;
	vecPoint.y = pev->origin.y - flYSize;

	UTIL_TraceLine ( vecPoint, vecPoint - Vector ( 0, 0, 100 ), ignore_monsters, ENT(pev), &tr );
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if ( flLength2 > flLength )
	{
		return FALSE;
	}
	flLength = flLength2;

	vecPoint.x = pev->origin.x - flXSize;
	vecPoint.y = pev->origin.y + flYSize;
	UTIL_TraceLine ( vecPoint, vecPoint - Vector ( 0, 0, 100 ), ignore_monsters, ENT(pev), &tr );
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if ( flLength2 > flLength )
	{
		return FALSE;
	}
	flLength = flLength2;

	vecPoint.x = pev->origin.x + flXSize;
	vecPoint.y = pev->origin.y - flYSize;
	UTIL_TraceLine ( vecPoint, vecPoint - Vector ( 0, 0, 100 ), ignore_monsters, ENT(pev), &tr );
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if ( flLength2 > flLength )
	{
		return FALSE;
	}
	flLength = flLength2;

	return TRUE;
}

//=========================================================
// Get Enemy - tries to find the best suitable enemy for the monster.
//=========================================================
BOOL CMBaseMonster :: GetEnemy ( void )
{
	edict_t *pNewEnemy;

	if ( HasConditions(bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_NEMESIS) )
	{
		pNewEnemy = BestVisibleEnemy();

		if ( pNewEnemy != m_hEnemy && pNewEnemy != NULL)
		{
			// DO NOT mess with the monster's m_hEnemy pointer unless the schedule the monster is currently running will be interrupted
			// by COND_NEW_ENEMY. This will eliminate the problem of monsters getting a new enemy while they are in a schedule that doesn't care,
			// and then not realizing it by the time they get to a schedule that does. I don't feel this is a good permanent fix. 

			if ( m_pSchedule )
			{
				if ( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
				{
					PushEnemy( m_hEnemy, m_vecEnemyLKP );
					SetConditions(bits_COND_NEW_ENEMY);
					m_hEnemy = pNewEnemy;
					m_vecEnemyLKP = m_hEnemy->v.origin;
				}
				// if the new enemy has an owner, take that one as well
//jlb				if (pNewEnemy->v.owner != NULL)
//jlb				{
//jlb					edict_t *pOwner = pNewEnemy->v.owner;
//jlb					if ( pOwner && (pOwner->v.flags & FL_MONSTER) && IRelationship( pOwner ) != R_NO )
//jlb						PushEnemy( pOwner, m_vecEnemyLKP );
//jlb				}
			}
		}
	}

	// do we see a player?  Allies use this to set the m_hEnemy pointer...
	if (HasConditions(bits_COND_SEE_CLIENT) && (m_hEnemy == NULL))
	{
		m_hEnemy = BestVisibleEnemy();

		// the player we've just seen might not always be our enemy
		if ( m_hEnemy != NULL )
		{
			m_hTargetEnt = m_hEnemy;
			m_vecEnemyLKP = m_hEnemy->v.origin;
		}
	}

	// remember old enemies
	if (m_hEnemy == NULL && PopEnemy( ))
	{
		if ( m_pSchedule )
		{
			if ( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
			{
				SetConditions(bits_COND_NEW_ENEMY);
			}
		}
	}

	if ( m_hEnemy != NULL )
	{
		// monster has an enemy.
		return TRUE;
	}

	return FALSE;// monster has no enemy
}


BOOL CMBaseMonster :: ShouldFadeOnDeath( void )
{
	// if flagged to fade out or I have an owner (I came from a monster spawner)
	if ( (pev->spawnflags & SF_MONSTER_FADECORPSE) || !FNullEnt( pev->owner ) )
		return TRUE;

	return FALSE;
}


void CMBaseMonster :: TaskFail( void )
{
   SetConditions(bits_COND_TASK_FAILED);
}

