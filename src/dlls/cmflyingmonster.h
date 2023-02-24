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
// Base class for flying monsters.  This overrides the movement test & execution code from CBaseMonster

#ifndef FLYINGMONSTER_H
#define FLYINGMONSTER_H

#include "cmbasemonster.h"

class CMFlyingMonster : public CMBaseMonster
{
public:
	int 		CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, edict_t *pTarget, float *pflDist );// check validity of a straight move through space
	BOOL		FTriangulate ( const Vector &vecStart , const Vector &vecEnd, float flDist, edict_t *pTargetEnt, Vector *pApex );
	Activity	GetStoppedActivity( void );
	void		Killed( entvars_t *pevAttacker, int iGib );
	void		Stop( void );
	float		ChangeYaw( int speed );
	void		HandleAnimEvent( MonsterEvent_t *pEvent );
	void		MoveExecute( edict_t *pTargetEnt, const Vector &vecDir, float flInterval );
	void		Move( float flInterval = 0.1 );
	BOOL		ShouldAdvanceRoute( float flWaypointDist );

	inline void	SetFlyingMomentum( float momentum ) { m_momentum = momentum; }
	inline void	SetFlyingFlapSound( const char *pFlapSound ) { m_pFlapSound = pFlapSound; }
	inline void	SetFlyingSpeed( float speed ) { m_flightSpeed = speed; }
	float		CeilingZ( const Vector &position );
	float		FloorZ( const Vector &position );
	BOOL		ProbeZ( const Vector &position, const Vector &probe, float *pFraction );
	
	
	// UNDONE:  Save/restore this stuff!!!
protected:
	Vector		m_vecTravel;		// Current direction
	float		m_flightSpeed;		// Current flight speed (decays when not flapping or gliding)
	float		m_stopTime;			// Last time we stopped (to avoid switching states too soon)
	float		m_momentum;			// Weight for desired vs. momentum velocity
	const char	*m_pFlapSound;
};


//=========================================================
// Stukabat
//=========================================================
class CMStukabat : public CMFlyingMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	
	void SetActivity ( Activity NewActivity );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	Schedule_t *GetScheduleOfType ( int Type );

	int  GetBitePitch( void ) { return PITCH_NORM + RANDOM_LONG( 40, 50 ); }
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	
	// Not used
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }

	CUSTOM_SCHEDULES
};

#endif		//FLYINGMONSTER_H

