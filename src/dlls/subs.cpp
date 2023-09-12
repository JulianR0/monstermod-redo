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
/*

===== subs.cpp ========================================================

  frequently used global functions

*/

#include "extdll.h"
#include "util.h"
#include "cmbase.h"
#include "nodes.h"
#include "doors.h"

extern CGraph WorldGraph;

extern BOOL FEntIsVisible(entvars_t* pev, entvars_t* pevTarget);

void Remove_Entity(edict_t *pEdict);


// Landmark class
void CMPointEntity :: Spawn( void )
{
	pev->solid = SOLID_NOT;
//	UTIL_SetSize(pev, g_vecZero, g_vecZero);
}


// This updates global tables that need to know about entities being removed
void CMBaseEntity::UpdateOnRemove( void )
{
	int	i;

	if ( FBitSet( pev->flags, FL_GRAPHED ) )
	{
	// this entity was a LinkEnt in the world node graph, so we must remove it from
	// the graph since we are removing it from the world.
		for ( i = 0 ; i < WorldGraph.m_cLinks ; i++ )
		{
			if ( WorldGraph.m_pLinkPool [ i ].m_pLinkEnt == pev )
			{
				// if this link has a link ent which is the same ent that is removing itself, remove it!
				WorldGraph.m_pLinkPool [ i ].m_pLinkEnt = NULL;
			}
		}
	}
//jlb	if ( pev->globalname )
//jlb		gGlobalState.EntitySetState( pev->globalname, GLOBAL_DEAD );
}

// Convenient way to delay removing oneself
void CMBaseEntity :: SUB_Remove( void )
{
	UpdateOnRemove();
	if (pev->health > 0)
	{
		// this situation can screw up monsters who can't tell their entity pointers are invalid.
		pev->health = 0;
		ALERT( at_aiconsole, "SUB_Remove called on entity with health > 0\n");
	}

//	REMOVE_ENTITY(ENT(pev));
   Remove_Entity(ENT(pev));
}


// Convenient way to explicitly do nothing (passed to functions that require a method)
void CMBaseEntity :: SUB_DoNothing( void )
{
}


void CMBaseDelay :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "delay"))
	{
		m_flDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "killtarget"))
	{
		m_iszKillTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CMBaseEntity::KeyValue( pkvd );
	}
}


/*
==============================
SUB_UseTargets

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Removes all entities with a targetname that match self.killtarget,
and removes them, so some events can remove other triggers.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function (if they have one)

==============================
*/
void CMBaseEntity :: SUB_UseTargets( edict_t *pActivator, USE_TYPE useType, float value )
{
	//
	// fire targets
	//
	if (!FStringNull(pev->target))
	{
		FireTargets( STRING(pev->target), pActivator, this->edict(), useType, value );
	}
}


void FireTargets( const char *targetName, edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value )
{
	edict_t *pentTarget = NULL;
	if ( !targetName )
		return;

	ALERT( at_aiconsole, "Firing: (%s)\n", targetName );

	for (;;)
	{
		pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, targetName);
		if (FNullEnt(pentTarget))
			break;

		// MonsterMod entity
		CMBaseEntity *pTarget = CMBaseEntity::Instance( pentTarget );
		if ( pTarget && !(pTarget->pev->flags & FL_KILLME) )
		{
			ALERT( at_aiconsole, "Found: %s, firing (%s)\n", STRING(pTarget->pev->classname), targetName );
			pTarget->Use( pActivator, pCaller, useType, value );
		}
		// Valid entity but not recognized by monstermod, must be a normal entity
		else if (!(pentTarget->v.flags & FL_KILLME))
		{
			if (CVAR_GET_FLOAT("_glb_use")) // avoid "unknown command" spam
			{
				ALERT( at_aiconsole, "Found: %s, firing (%s)\n", STRING(pentTarget->v.classname), targetName );

				char extCmd[64];
				sprintf( extCmd, "_trigger %i %i %i %i %f\n", ENTINDEX( pentTarget ), ENTINDEX( ENT( pActivator ) ), ENTINDEX( ENT( pCaller ) ), useType, value );
				SERVER_COMMAND( extCmd );
			}
		}
	}
}

void CMBaseDelay :: SUB_UseTargets( edict_t *pActivator, USE_TYPE useType, float value )
{
	//
	// exit immediatly if we don't have a target or kill target
	//
	if (FStringNull(pev->target) && !m_iszKillTarget)
		return;

	//
	// check for a delay
	//
	if (m_flDelay != 0)
	{
		// create a temp object to fire at a later time
		CMBaseDelay *pTemp = CreateClassPtr( (CMBaseDelay *)NULL);

      if (pTemp == NULL)
         return;

		pTemp->pev->classname = MAKE_STRING("DelayedUse");

		pTemp->pev->nextthink = gpGlobals->time + m_flDelay;

		pTemp->SetThink( &CMBaseDelay::DelayThink );
		
		// Save the useType
		pTemp->pev->button = (int)useType;
		pTemp->m_iszKillTarget = m_iszKillTarget;
		pTemp->m_flDelay = 0; // prevent "recursion"
		pTemp->pev->target = pev->target;

		// HACKHACK
		// This wasn't in the release build of Half-Life.  We should have moved m_hActivator into this class
		// but changing member variable hierarchy would break save/restore without some ugly code.
		// This code is not as ugly as that code
		if ( pActivator && UTIL_IsPlayer(pActivator) )		// If a player activates, then save it
		{
			pTemp->pev->owner = pActivator;
		}
		else
		{
			pTemp->pev->owner = NULL;
		}

		return;
	}

	//
	// kill the killtargets
	//

	if ( m_iszKillTarget )
	{
		edict_t *pentKillTarget = NULL;

		ALERT( at_aiconsole, "KillTarget: %s\n", STRING(m_iszKillTarget) );
		pentKillTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(m_iszKillTarget) );
		while ( !FNullEnt(pentKillTarget) )
		{
			UTIL_Remove( CMBaseEntity::Instance(pentKillTarget)->edict() );

			ALERT( at_aiconsole, "killing %s\n", STRING( pentKillTarget->v.classname ) );
			pentKillTarget = FIND_ENTITY_BY_TARGETNAME( pentKillTarget, STRING(m_iszKillTarget) );
		}
	}
	
	//
	// fire targets
	//
	if (!FStringNull(pev->target))
	{
		FireTargets( STRING(pev->target), pActivator, this->edict(), useType, value );
	}
}


/*
QuakeEd only writes a single float for angles (bad idea), so up and down are
just constant angles.
*/
void SetMovedir( entvars_t *pev )
{
	if (pev->angles == Vector(0, -1, 0))
	{
		pev->movedir = Vector(0, 0, 1);
	}
	else if (pev->angles == Vector(0, -2, 0))
	{
		pev->movedir = Vector(0, 0, -1);
	}
	else
	{
		UTIL_MakeVectors(pev->angles);
		pev->movedir = gpGlobals->v_forward;
	}
	
	pev->angles = g_vecZero;
}




void CMBaseDelay::DelayThink( void )
{
	edict_t *pActivator = NULL;

	if ( pev->owner != NULL )		// A player activated this on delay
	{
		pActivator = ENT(pev->owner);
	}
	// The use type is cached (and stashed) in pev->button
	SUB_UseTargets( pActivator, (USE_TYPE)pev->button, 0 );
//	REMOVE_ENTITY(ENT(pev));
   Remove_Entity(ENT(pev));
}


void CMBaseToggle::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "lip"))
	{
		m_flLip = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_sMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "distance"))
	{
		m_flMoveDistance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CMBaseDelay::KeyValue( pkvd );
}

/*
=============
LinearMove

calculate pev->velocity and pev->nextthink to reach vecDest from
pev->origin traveling at flSpeed
===============
*/
void CMBaseToggle ::  LinearMove( Vector	vecDest, float flSpeed )
{
	ASSERTSZ(flSpeed != 0, "LinearMove:  no speed is defined!");
//	ASSERTSZ(m_pfnCallWhenMoveDone != NULL, "LinearMove: no post-move function defined");
	
	m_vecFinalDest = vecDest;

	// Already there?
	if (vecDest == pev->origin)
	{
		LinearMoveDone();
		return;
	}
		
	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - pev->origin;
	
	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink( &CMBaseToggle::LinearMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->velocity = vecDestDelta / flTravelTime;
}


/*
============
After moving, set origin to exact final destination, call "move done" function
============
*/
void CMBaseToggle :: LinearMoveDone( void )
{
	UTIL_SetOrigin(pev, m_vecFinalDest);
	pev->velocity = g_vecZero;
	pev->nextthink = -1;
	if ( m_pfnCallWhenMoveDone )
		(this->*m_pfnCallWhenMoveDone)();
}

BOOL CMBaseToggle :: IsLockedByMaster( void )
{
		return FALSE;
}

/*
=============
AngularMove

calculate pev->velocity and pev->nextthink to reach vecDest from
pev->origin traveling at flSpeed
Just like LinearMove, but rotational.
===============
*/
void CMBaseToggle :: AngularMove( Vector vecDestAngle, float flSpeed )
{
	ASSERTSZ(flSpeed != 0, "AngularMove:  no speed is defined!");
//	ASSERTSZ(m_pfnCallWhenMoveDone != NULL, "AngularMove: no post-move function defined");
	
	m_vecFinalAngle = vecDestAngle;

	// Already there?
	if (vecDestAngle == pev->angles)
	{
		AngularMoveDone();
		return;
	}
	
	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDestAngle - pev->angles;
	
	// divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to AngularMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink( &CMBaseToggle::AngularMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->avelocity = vecDestDelta / flTravelTime;
}


/*
============
After rotating, set angle to exact final angle, call "move done" function
============
*/
void CMBaseToggle :: AngularMoveDone( void )
{
	pev->angles = m_vecFinalAngle;
	pev->avelocity = g_vecZero;
	pev->nextthink = -1;
	if ( m_pfnCallWhenMoveDone )
		(this->*m_pfnCallWhenMoveDone)();
}


float CMBaseToggle :: AxisValue( int flags, const Vector &angles )
{
	if ( FBitSet(flags, SF_DOOR_ROTATE_Z) )
		return angles.z;
	if ( FBitSet(flags, SF_DOOR_ROTATE_X) )
		return angles.x;

	return angles.y;
}


void CMBaseToggle :: AxisDir( entvars_t *pev )
{
	if ( FBitSet(pev->spawnflags, SF_DOOR_ROTATE_Z) )
		pev->movedir = Vector ( 0, 0, 1 );	// around z-axis
	else if ( FBitSet(pev->spawnflags, SF_DOOR_ROTATE_X) )
		pev->movedir = Vector ( 1, 0, 0 );	// around x-axis
	else
		pev->movedir = Vector ( 0, 1, 0 );		// around y-axis
}


float CMBaseToggle :: AxisDelta( int flags, const Vector &angle1, const Vector &angle2 )
{
	if ( FBitSet (flags, SF_DOOR_ROTATE_Z) )
		return angle1.z - angle2.z;
	
	if ( FBitSet (flags, SF_DOOR_ROTATE_X) )
		return angle1.x - angle2.x;

	return angle1.y - angle2.y;
}


/*
=============
FEntIsVisible

returns TRUE if the passed entity is visible to caller, even if not infront ()
=============
*/
	BOOL
FEntIsVisible(
	entvars_t*		pev,
	entvars_t*		pevTarget)
	{
	Vector vecSpot1 = pev->origin + pev->view_ofs;
	Vector vecSpot2 = pevTarget->origin + pevTarget->view_ofs;
	TraceResult tr;

	UTIL_TraceLine(vecSpot1, vecSpot2, ignore_monsters, ENT(pev), &tr);
	
	if (tr.fInOpen && tr.fInWater)
		return FALSE;                   // sight line crossed contents

	if (tr.flFraction == 1)
		return TRUE;

	return FALSE;
	}


