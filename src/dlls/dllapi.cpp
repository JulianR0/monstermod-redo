//
// Monster Mod is a modification based on Botman's original "Monster" plugin.
// The "forgotten" modification was made by Rick90.
// This is an attempt to recreate the plugin so it does not become lost again.
//
// Recreated by Giegue.
//
// dllapi.cpp
//

/*
 *	This is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation; either version 2 of the License, or (at
 *	your option) any later version.
 *
 *	This is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this code; if not, write to the Free Software Foundation,
 *	Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307	USA
 *
 *	In addition, as a special exception, the author gives permission to
 *	link the code of this program with the Half-Life Game Engine ("HL
 *	Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *	L.L.C ("Valve"). You must obey the GNU General Public License in all
 *	respects for all of the code used other than the HL Engine and MODs
 *	from Valve.  If you modify this file, you may extend this exception
 *	to your version of the file, but you are not obligated to do so. If
 *	you do not wish to do so, delete this exception statement from your
 *	version.
 *
 */


#include "extdll.h"
#include "dllapi.h"
#include "meta_api.h"

#include "cmbase.h"
#include "cmbasemonster.h"
#include "cmbaseextra.h"
#include "monsters.h"
#include "weapons.h"
#include "hornet.h"
#include "decals.h"
#include "shake.h"
#include "skill.h"
#include "nodes.h"

extern CGraph WorldGraph;

extern globalvars_t	 *gpGlobals;
extern enginefuncs_t g_engfuncs;
extern gamedll_funcs_t *gpGamedllFuncs;

extern cvar_t *dllapi_log;
extern cvar_t *monster_spawn;
extern cvar_t *monster_show_deaths;
extern cvar_t *monster_show_info;

// Player TakeDamage and Killed
int g_DamageMsg;
bool g_DamageActive;
int g_DamageVictim;
int g_DamageBits[33];
bool g_PlayerKilled[33];

// TE_TEXTMESSAGE
float g_NextMessage[33];

cvar_t	*g_psv_gravity = NULL;

DLL_DECALLIST gDecals[] = {
	{ "{shot1",	-1 },		// DECAL_GUNSHOT1 
	{ "{shot2",	-1 },		// DECAL_GUNSHOT2
	{ "{shot3", -1 },			// DECAL_GUNSHOT3
	{ "{shot4",	-1 },		// DECAL_GUNSHOT4
	{ "{shot5",	-1 },		// DECAL_GUNSHOT5
	{ "{lambda01", -1 },		// DECAL_LAMBDA1
	{ "{lambda02", -1 },		// DECAL_LAMBDA2
	{ "{lambda03", -1 },		// DECAL_LAMBDA3
	{ "{lambda04", -1 },		// DECAL_LAMBDA4
	{ "{lambda05", -1 },		// DECAL_LAMBDA5
	{ "{lambda06", -1 },		// DECAL_LAMBDA6
	{ "{scorch1", -1 },		// DECAL_SCORCH1
	{ "{scorch2", -1 },		// DECAL_SCORCH2
	{ "{blood1", -1 },		// DECAL_BLOOD1
	{ "{blood2", -1 },		// DECAL_BLOOD2
	{ "{blood3", -1 },		// DECAL_BLOOD3
	{ "{blood4", -1 },		// DECAL_BLOOD4
	{ "{blood5", -1 },		// DECAL_BLOOD5
	{ "{blood6", -1 },		// DECAL_BLOOD6
	{ "{yblood1", -1 },		// DECAL_YBLOOD1
	{ "{yblood2", -1 },		// DECAL_YBLOOD2
	{ "{yblood3", -1 },		// DECAL_YBLOOD3
	{ "{yblood4", -1 },		// DECAL_YBLOOD4
	{ "{yblood5", -1 },		// DECAL_YBLOOD5
	{ "{yblood6", -1 },		// DECAL_YBLOOD6
	{ "{break1", -1 },		// DECAL_GLASSBREAK1
	{ "{break2", -1 },		// DECAL_GLASSBREAK2
	{ "{break3", -1 },		// DECAL_GLASSBREAK3
	{ "{bigshot1", -1 },		// DECAL_BIGSHOT1
	{ "{bigshot2", -1 },		// DECAL_BIGSHOT2
	{ "{bigshot3", -1 },		// DECAL_BIGSHOT3
	{ "{bigshot4", -1 },		// DECAL_BIGSHOT4
	{ "{bigshot5", -1 },		// DECAL_BIGSHOT5
	{ "{spit1", -1 },		// DECAL_SPIT1
	{ "{spit2", -1 },		// DECAL_SPIT2
	{ "{bproof1", -1 },		// DECAL_BPROOF1
	{ "{gargstomp", -1 },	// DECAL_GARGSTOMP1,	// Gargantua stomp crack
	{ "{smscorch1", -1 },	// DECAL_SMALLSCORCH1,	// Small scorch mark
	{ "{smscorch2", -1 },	// DECAL_SMALLSCORCH2,	// Small scorch mark
	{ "{smscorch3", -1 },	// DECAL_SMALLSCORCH3,	// Small scorch mark
	{ "{mommablob", -1 },	// DECAL_MOMMABIRTH		// BM Birth spray
	{ "{mommablob", -1 },	// DECAL_MOMMASPLAT		// BM Mortar spray?? need decal
};

monster_type_t monster_types[]=
{
	// These are just names. But to keep it consistent
	// with the new KVD format, ensure these are exactly
	// like an actual, entity classname.
	
	// We are going to use this as a list of what entities
	// can be spawned. Monsters should go first.
	// DO NOT ALTER THE ORDER OF ELEMENTS!
	
	"monster_alien_grunt", FALSE, // Original Half-Life Monsters
	"monster_apache", FALSE,
	"monster_barney", FALSE,
	"monster_bigmomma", FALSE,
	"monster_bullsquid", FALSE,
	"monster_alien_controller", FALSE,
	"monster_human_assassin", FALSE,
	"monster_headcrab", FALSE,
	"monster_human_grunt", FALSE,
	"monster_houndeye", FALSE,
	"monster_alien_slave", FALSE,
	"monster_scientist", FALSE,
	"monster_snark", FALSE,
	"monster_zombie", FALSE,
	"monster_gargantua", FALSE,
	"monster_turret", FALSE,
	"monster_miniturret", FALSE,
	"monster_sentry", FALSE,
	"monster_gonome", FALSE, // Opposing Force Monsters
	"monster_male_assassin", FALSE,
	"monster_otis", FALSE,
	"monster_pitdrone", FALSE,
	"monster_shockroach", FALSE,
	"monster_shocktrooper", FALSE,
	"monster_alien_voltigore", FALSE,
	"monster_alien_babyvoltigore", FALSE,
	"monster_babygarg", FALSE, // Sven Co-op Monsters
	"monster_hwgrunt", FALSE,
	"monster_robogrunt", FALSE,
	"monster_stukabat", FALSE,
	"info_node", FALSE, // Nodes
	"info_node_air", FALSE,
	"monstermaker", FALSE, // Extra entities
	"ambient_music", FALSE,
	"squadmaker", FALSE, // Aliases
	"", FALSE
};

monster_t monsters[MAX_MONSTER_ENTS];
int monster_ents_used = 0;

monster_spawnpoint_t monster_spawnpoint[MAX_MONSTERS];
int monster_spawn_count = 0;

node_spawnpoint_t node_spawnpoint[MAX_NODES];
int node_spawn_count = 0;

float check_respawn_time;
float check_graph_time;

bool process_monster_cfg(void);
bool process_monster_precache_cfg(void);


int GetMonsterIndex(void)
{
	int monster_index = -1;

	for (int index = 0; index < MAX_MONSTER_ENTS; index++)
	{
		if (monsters[index].monster_pent == 0)
		{
			monster_index = index;
			break;
		}
	}
	
	if (monster_index == -1)
		return -1;
	
	if (monster_index >= monster_ents_used)
		monster_ents_used = monster_index + 1;  // monster index is 0 based
	
	return monster_index;
}


void FreeMonsterIndex(int index)
{
	delete monsters[index].pMonster;
	
	monsters[index].monster_index = 0;
	monsters[index].monster_pent = NULL;
	monsters[index].killed = FALSE;
	monsters[index].pMonster = NULL;
	
	if (index == monster_ents_used-1)
	{
		while (monsters[index].monster_index == 0)
		{
			index--;
			monster_ents_used--;
			if (monster_ents_used == 0)
				break;
		}
	}
}


void Remove_Entity(edict_t *pEdict)
{
	for (int index = 0; index < monster_ents_used; index++)
	{
		if (monsters[index].monster_pent == pEdict)
		{
			FreeMonsterIndex(index);
			break;
		}
	}
	
	REMOVE_ENTITY(pEdict);
}


void monster_unload(void)
{
	// the plugin is being unloaded, remove any currently spawned monster...
	
	for (int index = 0; index < MAX_MONSTER_ENTS; index++)
	{
		if (monsters[index].pMonster != NULL)
		{
			monsters[index].monster_pent->v.flags |= FL_KILLME;
			
			delete monsters[index].pMonster;
			
			monsters[index].monster_index = 0;
			monsters[index].monster_pent = NULL;
			monsters[index].killed = FALSE;
			monsters[index].pMonster = NULL;
		}
	}
}


void check_monster_hurt(edict_t *pAttacker)
{
	int index;
	
	for (index = 0; index < monster_ents_used; index++)
	{
		if (monsters[index].monster_index)
		{
			edict_t *pent = (*g_engfuncs.pfnPEntityOfEntIndex)(monsters[index].monster_index);
		
			if (pent)
			{
				if (pent->v.health < pent->v.fuser4)
				{
					if (pent->v.takedamage != DAMAGE_NO)
					{
						TraceResult tr;
						Vector vecSrc, vecSpot;
						float distance, damage;

						// location of attacker and location of enemy...
						vecSrc = pAttacker->v.origin + pAttacker->v.view_ofs;
						vecSpot = pent->v.origin;

						// distance the blood can travel from the body...
						distance = (vecSpot - vecSrc).Length() + 100.0f;

						// use aiming angles of attacker to trace blood splatter...
						UTIL_MakeVectors(pAttacker->v.v_angle);

						// start just beyond the attacker's body...
						vecSrc = vecSrc + gpGlobals->v_forward * 20;
						vecSpot = vecSrc + gpGlobals->v_forward * distance;

						// trace a line ignoring enemies body...
						UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, pAttacker, &tr );

						damage = pent->v.fuser4 - pent->v.health;

						// restore previous health, then do the damage (again)
						pent->v.health = pent->v.fuser4;

						ClearMultiDamage( );
						monsters[index].pMonster->TraceAttack( VARS(pAttacker), damage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, DMG_BULLET|DMG_NEVERGIB );
						ApplyMultiDamage( VARS(pAttacker), VARS(pAttacker) );
					}

					// save the new current health as previous health...
					pent->v.fuser4 = pent->v.health;
				}
			}
			else
			{
				// the entity no longer exists and we didn't catch it dying
				FreeMonsterIndex(index);
			}
		}
	}
}


void check_monster_dead(edict_t *pAttacker)
{
	for (int index = 0; index < monster_ents_used; index++)
	{
		if (monsters[index].monster_index)
		{
			edict_t *pent = (*g_engfuncs.pfnPEntityOfEntIndex)(monsters[index].monster_index);

			if (pent)
			{
				if (pent->v.flags & FL_KILLME)	// func_wall was "killed"
				{
					if (pent->v.flags & FL_MONSTER)	// is this a monster?
					{
						if (monsters[index].killed == FALSE)
						{
							pent->v.flags &= ~FL_KILLME;  // clear FL_KILLME bit

							pent->v.deadflag = DEAD_NO;   // bring back to life
							
							monsters[index].pMonster->Killed(VARS(pAttacker), 0);
							
							monsters[index].killed = TRUE;
						}
					}
					else	 // normal entity
					{
						FreeMonsterIndex(index);
					}
				}
			}
			else
			{
				FreeMonsterIndex(index);
			}
		}
	}
}


void check_player_dead( edict_t *pPlayer )
{
	// Death messages are disabled
	if (!monster_show_deaths->value)
		return;
	
	int iPlayerIndex = ENTINDEX( pPlayer );
	
	// Player died?
	if ( !UTIL_IsAlive( pPlayer ) )
	{
		edict_t *pAttacker = pPlayer->v.dmg_inflictor;
		char szMessage[129]; // To allow exactly 128 characters
		
		// Attacker is NULL or message already shown, don't care
		if ( pAttacker == NULL || g_PlayerKilled[ iPlayerIndex ] )
			return;
		
		// Get player's name
		char szPlayerName[33];
		sprintf( szPlayerName, "%s", STRING( pPlayer->v.netname ) );
		
		// Killed by a monster?
		if ( pAttacker->v.flags & FL_MONSTER )
		{
			// Try to get the name of the monster
			char szName[129], szCheck[2];

			CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pAttacker));
			if ( pMonster != NULL )
			{
				// One of our monsters
				strcpy(szName, STRING( pMonster->m_szMonsterName ));
			}
			else
			{
				// Does this monster have a name?
				if ( !FStringNull( pAttacker->v.netname ) )
					strcpy(szName, STRING( pAttacker->v.netname ));
				else
				{
					// No name, use class
					strcpy(szName, STRING( pAttacker->v.classname ));
				}
			}

			// Now, copy the first character to check for 'aeiou'.
			strncpy( szCheck, szName, 1 );
			
			// Make this character lowercase and inspect it. Select which message.
			szCheck[0] = tolower( szCheck[ 0 ] );
			if ( strncmp( szCheck, "a", 1 ) == 0 || strncmp( szCheck, "e", 1 ) == 0 || strncmp( szCheck, "i", 1 ) == 0 || strncmp( szCheck, "o", 1 ) == 0 || strncmp( szCheck, "u", 1 ) == 0 )
				sprintf( szMessage, "* %s was killed by an %s.\n", szPlayerName, szName );
			else
				sprintf( szMessage, "* %s was killed by a %s.\n", szPlayerName, szName );
		}
		else
		{
			// Suicide?
			if ( pAttacker == pPlayer )
				sprintf( szMessage, "* %s commited suicide.\n", szPlayerName );
			// Player killed by another player
			else if ( UTIL_IsPlayer( pAttacker ) )
			{
				// Get attacker name
				char szAttackerName[33];
				strcpy(szAttackerName, STRING(pAttacker->v.netname));

				// Print a very basic death message until we can detect teamkills
				sprintf( szMessage, "* %s was killed by %s.\n", szPlayerName, szAttackerName );
			}
			// An entity killed this player.
			else if ( ENTINDEX( pAttacker ) > 0 )
			{
				// HLSDK: "int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;"
				// When the game sends a "Damage" NetworkMessage, the damageBits
				// gets filtered so only damages that can be shown on HUD will pass
				// through, otherwise it gets neutered to DMG_GENERIC.

				// This means that players will only see a few death messages
				// instead of all possibilities that are written here.

				// If you want to make use of all possible death messages, you
				// will have to override the NetworkMessage. -Giegue

				// Gather damage type and format death message
				if ( g_DamageBits[ iPlayerIndex ] == DMG_GENERIC )
					sprintf( szMessage, "* %s died mysteriously.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_CRUSH )
					sprintf( szMessage, "* %s was crushed.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_BULLET )
					sprintf( szMessage, "* %s was shot.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_SLASH )
					sprintf( szMessage, "* %s has been chopped.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_BURN )
					sprintf( szMessage, "* %s burned down.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_FREEZE )
					sprintf( szMessage, "* %s froze to death.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_FALL )
					sprintf( szMessage, "* %s fell.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_BLAST )
					sprintf( szMessage, "* %s blew up.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_CLUB )
					sprintf( szMessage, "* %s was crowbared.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_SHOCK )
					sprintf( szMessage, "* %s was electrocuted.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_SONIC )
					sprintf( szMessage, "* %s ears popped.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_ENERGYBEAM )
					sprintf( szMessage, "* %s was cut by a laser.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] == DMG_NEVERGIB )
					sprintf( szMessage, "* %s had a painful death.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] == DMG_ALWAYSGIB )
					sprintf( szMessage, "* %s was gibbed.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_DROWN )
					sprintf( szMessage, "* %s drowned.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_PARALYZE )
					sprintf( szMessage, "* %s was paralyzed.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_NERVEGAS )
					sprintf( szMessage, "* %s lost its brain.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_POISON )
					sprintf( szMessage, "* %s has been poisoned.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_RADIATION )
					sprintf( szMessage, "* %s went nuclear.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_ACID )
					sprintf( szMessage, "* %s was melted.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_SLOWBURN )
					sprintf( szMessage, "* %s was baked like a cake.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_SLOWFREEZE )
					sprintf( szMessage, "* %s died of hypothermia.\n", szPlayerName );
				else if ( g_DamageBits[ iPlayerIndex ] & DMG_MORTAR )
					sprintf( szMessage, "* %s was killed by a Mortar Shell.\n", szPlayerName );
				else // other mods could have more DMG_ variants that aren't registered here.
					sprintf( szMessage, "* %s deadly died.\n", szPlayerName );
			}
			// the "world" killed this player
			else
				sprintf( szMessage, "* %s fell or drowned or something.\n", szPlayerName );
		}
		
		// Print the message
		if ( strlen( szMessage ) > 0 )
			UTIL_ClientPrintAll( HUD_PRINTTALK, szMessage );
		g_PlayerKilled[ iPlayerIndex ] = true;
	}
	else
		g_PlayerKilled[ iPlayerIndex ] = false;
}

void check_monster_info( edict_t *pPlayer )
{
	// Monster Info is disabled
	if (!monster_show_info->value)
		return;
	
	// Player must be alive
	if ( UTIL_IsAlive( pPlayer ) )
	{
		// Don't overdo it!
		if ( g_NextMessage[ ENTINDEX( pPlayer ) ] > gpGlobals->time )
			return;
		
		// Get player position and view angle
		Vector origin = pPlayer->v.origin;
		Vector view_angle = pPlayer->v.v_angle;
		Vector view_offset = pPlayer->v.view_ofs;
		
		// Prepare Trace
		TraceResult tr;
		Vector v_src, v_dest;
		
		UTIL_MakeVectors(view_angle);
		
		v_src = origin + view_offset; // Player aiment
		v_dest = v_src + gpGlobals->v_forward * 4096; // Should cover enough distance
		
		UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pPlayer, &tr);
		
		// Hit an entity?
		if (tr.pHit != NULL)
		{
			// It should be alive
			if ( UTIL_IsAlive( tr.pHit ) )
			{
				// Must be a monster (and strictly a monster!)
				if (strncmp( STRING( tr.pHit->v.classname ), "monster_", 8 ) == 0 && tr.pHit->v.flags & FL_MONSTER)
				{
					char szName[129];
					float monsterHealth, monsterFrags;
					int classify;
					BOOL isAlly = FALSE;
					
					// Get monster info
					CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(tr.pHit));
					if ( pMonster != NULL )
					{
						strcpy(szName, STRING( pMonster->m_szMonsterName ));
						classify = pMonster->Classify();
					}
					else
					{
						// A monster that we do not recognize, use its netname
						if ( !FStringNull( tr.pHit->v.netname ) )
							strcpy(szName, STRING( tr.pHit->v.netname ));
						else
						{
							// If all else fails, use classname as monster name
							strcpy(szName, STRING( tr.pHit->v.classname ));
						}
						classify = tr.pHit->v.iuser4;
					}
					monsterHealth = tr.pHit->v.health;
					monsterFrags = tr.pHit->v.frags;
					
					// Unless it is strictly ally to us, treat as enemy monster
					if ( classify == CLASS_HUMAN_PASSIVE || classify == CLASS_PLAYER_ALLY )
						isAlly = TRUE;
					
					// Prepare the message
					char szInfo[257];
					sprintf(szInfo, "%s:  %s\nHealth:  %.0f\nFrags:    %.0f\n", ( isAlly ? "Friend" : "Enemy" ), szName, monsterHealth, monsterFrags );
					
					// Create a TE_TEXTMESSAGE and show the monster information
					MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pPlayer );
					WRITE_BYTE( TE_TEXTMESSAGE );
					WRITE_BYTE( 3 ); // Channel
					WRITE_SHORT( 327 ); // X
					WRITE_SHORT( 4771 ); // Y
					WRITE_BYTE( 0 ); // Effect
					if ( isAlly )
					{
						WRITE_BYTE( 9 ); // R1
						WRITE_BYTE( 172 ); // G1
						WRITE_BYTE( 96 ); // B1
					}
					else
					{
						WRITE_BYTE( 171 ); // R1
						WRITE_BYTE( 23 ); // G1
						WRITE_BYTE( 7 ); // B1
					}
					WRITE_BYTE( 0 ); // A1
					WRITE_BYTE( 207 ); // R2
					WRITE_BYTE( 23 ); // G2
					WRITE_BYTE( 7 ); // B2
					WRITE_BYTE( 255 ); // A2
					WRITE_SHORT( 0 ); // Fade-in Time
					WRITE_SHORT( 15 ); // Fade-out Time
					WRITE_SHORT( 448 ); // Hold time
					WRITE_STRING( szInfo ); // Message
					MESSAGE_END();
					
					// Delay till next scan
					g_NextMessage[ ENTINDEX( pPlayer ) ] = gpGlobals->time + 0.30;
				}
			}
		}
	}
}

edict_t* spawn_monster(int monster_type, Vector origin, Vector angles, int spawnflags, pKVD *keyvalue)
{
	int monster_index;
	edict_t *monster_pent;
	KeyValueData kvd;
	
	if ((monster_index = GetMonsterIndex()) == -1)
	{
		//META_CONS("[MONSTER] ERROR: No FREE Monster edicts!");
		LOG_MESSAGE(PLID, "ERROR: No FREE Monster edicts!");
		return NULL;
	}
	
	// was this monster NOT precached?
	if (monster_types[monster_type].need_to_precache == FALSE)
	{
		char msg[256];

		//META_CONS("[MONSTER] ERROR: You can't spawn monster %s since it wasn't precached!", monster_types[monster_type].name);
		LOG_MESSAGE(PLID, "ERROR: You can't spawn monster %s since it wasn't precached!", monster_types[monster_type].name);
		
		//META_CONS("[MONSTER] valid precached monster names are:");
		LOG_MESSAGE(PLID, "valid precached monster names are:");
		msg[0] = 0;
		for (int index = 0; monster_types[index].name[0]; index++)
		{
			if (monster_types[index].need_to_precache == TRUE)
			{
				strcat(msg, monster_types[index].name);
				strcat(msg, " ");
				if (strlen(msg) > 60)
				{
				   //META_CONS("[MONSTER] %s", msg);
				   LOG_MESSAGE(PLID, "%s", msg);
				   msg[0] = 0;
				}
			}
		}
		if (msg[0])
		{
			//META_CONS("[MONSTER] %s", msg);
			LOG_MESSAGE(PLID, "%s", msg);
		}
		
		return NULL;
	}
	
	switch (monster_type)
	{
		// Monsters
		case 0: monsters[monster_index].pMonster = CreateClassPtr((CMAGrunt *)NULL); break;
		case 1: monsters[monster_index].pMonster = CreateClassPtr((CMApache *)NULL); break;
		case 2: monsters[monster_index].pMonster = CreateClassPtr((CMBarney *)NULL); break;
		case 3: monsters[monster_index].pMonster = CreateClassPtr((CMBigMomma *)NULL); break;
		case 4: monsters[monster_index].pMonster = CreateClassPtr((CMBullsquid *)NULL); break;
		case 5: monsters[monster_index].pMonster = CreateClassPtr((CMController *)NULL); break;
		case 6: monsters[monster_index].pMonster = CreateClassPtr((CMHAssassin *)NULL); break;
		case 7: monsters[monster_index].pMonster = CreateClassPtr((CMHeadCrab *)NULL); break;
		case 8: monsters[monster_index].pMonster = CreateClassPtr((CMHGrunt *)NULL); break;
		case 9: monsters[monster_index].pMonster = CreateClassPtr((CMHoundeye *)NULL); break;
		case 10: monsters[monster_index].pMonster = CreateClassPtr((CMISlave *)NULL); break;
		case 11: monsters[monster_index].pMonster = CreateClassPtr((CMScientist *)NULL); break;
		case 12: monsters[monster_index].pMonster = CreateClassPtr((CMSqueakGrenade *)NULL); break;
		case 13: monsters[monster_index].pMonster = CreateClassPtr((CMZombie *)NULL); break;
		case 14: monsters[monster_index].pMonster = CreateClassPtr((CMGargantua *)NULL); break;
		case 15: monsters[monster_index].pMonster = CreateClassPtr((CMTurret *)NULL); break;
		case 16: monsters[monster_index].pMonster = CreateClassPtr((CMMiniTurret *)NULL); break;
		case 17: monsters[monster_index].pMonster = CreateClassPtr((CMSentry *)NULL); break;
		case 18: monsters[monster_index].pMonster = CreateClassPtr((CMGonome *)NULL); break;
		case 19: monsters[monster_index].pMonster = CreateClassPtr((CMMassn *)NULL); break;
		case 20: monsters[monster_index].pMonster = CreateClassPtr((CMOtis *)NULL); break;
		case 21: monsters[monster_index].pMonster = CreateClassPtr((CMPitdrone *)NULL); break;
		case 22: monsters[monster_index].pMonster = CreateClassPtr((CMShockRoach *)NULL); break;
		case 23: monsters[monster_index].pMonster = CreateClassPtr((CMStrooper *)NULL); break;
		case 24: monsters[monster_index].pMonster = CreateClassPtr((CMVoltigore *)NULL); break;
		case 25: monsters[monster_index].pMonster = CreateClassPtr((CMBabyVoltigore *)NULL); break;
		case 26: monsters[monster_index].pMonster = CreateClassPtr((CMBabyGargantua *)NULL); break;
		case 27: monsters[monster_index].pMonster = CreateClassPtr((CMHWGrunt *)NULL); break;
		case 28: monsters[monster_index].pMonster = CreateClassPtr((CMRGrunt *)NULL); break;
		case 29: monsters[monster_index].pMonster = CreateClassPtr((CMStukabat *)NULL); break;
		// Extra entities
		case 32: monsters[monster_index].pMonster = CreateClassPtr((CMMonsterMaker *)NULL); break;
		case 33: monsters[monster_index].pMonster = CreateClassPtr((CMAmbientMusic *)NULL); break;
	}

	if (monsters[monster_index].pMonster == NULL)
	{
		//META_CONS("[MONSTER] ERROR: Error Creating Monster!" );
		LOG_MESSAGE(PLID, "ERROR: Error Creating Monster!");
		return NULL;
	}

	monster_pent = ENT(monsters[monster_index].pMonster->pev);
	monsters[monster_index].monster_pent = monster_pent;

	monsters[monster_index].monster_index = (*g_engfuncs.pfnIndexOfEdict)(monster_pent);

	monster_pent->v.origin = origin;
	monster_pent->v.angles = angles;
	
	// Pass spawnflags first if no keyvalue data exists for it
	monster_pent->v.spawnflags = spawnflags;
	
	// Keyvalue data
	if (keyvalue != NULL)
	{
		for (int index = 0; index < MAX_KEYVALUES; index++)
		{
			if (strlen(keyvalue[index].key) > 0)
			{
				kvd.szKeyName = keyvalue[index].key;
				kvd.szValue = keyvalue[index].value;
				monsters[monster_index].pMonster->KeyValue( &kvd );
			}
		}
	}
	
	monsters[monster_index].pMonster->Spawn();
	
	// Only modify starting spawnflags for monsters, not for entities!
	if ( monster_index <= 29 )
	{
		// Reverse fadecorpse behaviour
		if ( ( spawnflags & SF_MONSTER_FADECORPSE ) )
			monster_pent->v.spawnflags &= ~SF_MONSTER_FADECORPSE;
		else
			monster_pent->v.spawnflags |= SF_MONSTER_FADECORPSE;
	}

	monster_pent->v.fuser4 = monster_pent->v.health;	 // save the original health

	return monster_pent;
}


void check_respawn(void)
{
	int monster_type;
	Vector origin;
	Vector angles;
	int spawnflags;
	pKVD *keyvalue;
	
	if (!monster_spawn->value)
		return;  // monster_spawn is turned off, retry again later

	for (int index=0; index < monster_spawn_count; index++)
	{
		if (monster_spawnpoint[index].need_to_respawn)
		{
			monster_spawnpoint[index].need_to_respawn = FALSE;

			monster_type = monster_spawnpoint[index].monster;

			origin = monster_spawnpoint[index].origin;

			angles = monster_spawnpoint[index].angles;
			
			spawnflags = monster_spawnpoint[index].spawnflags;
			
			keyvalue = monster_spawnpoint[index].keyvalue;
			
			if (spawn_monster(monster_type, origin, angles, spawnflags, keyvalue) == NULL)
			{
				// spawn_monster failed
				ALERT( at_error, "[MONSTER] Failed to spawn %s at origin %f %f %f\n", monster_types[monster_type].name, origin.x, origin.y, origin.z );
			}
		}
	}
}

DLL_GLOBAL short g_sModelIndexFireball;// holds the index for the fireball
DLL_GLOBAL short g_sModelIndexSmoke;// holds the index for the smoke cloud
DLL_GLOBAL short g_sModelIndexTinySpit;// holds the index for the spore grenade explosion
DLL_GLOBAL short g_sModelIndexWExplosion;// holds the index for the underwater explosion
DLL_GLOBAL short g_sModelIndexBubbles;// holds the index for the bubbles model
DLL_GLOBAL short g_sModelIndexBloodDrop;// holds the sprite index for the initial blood
DLL_GLOBAL short g_sModelIndexBloodSpray;// holds the sprite index for splattered blood
DLL_GLOBAL short g_sModelIndexLaser;// holds the index for the laser beam
DLL_GLOBAL const char *g_pModelNameLaser = "sprites/laserbeam.spr";
DLL_GLOBAL short g_sModelIndexLaserDot;// holds the index for the laser beam dot

// globals.cpp
DLL_GLOBAL const Vector g_vecZero = Vector(0, 0, 0); // null vector
DLL_GLOBAL Vector g_vecAttackDir; // attack direction


void world_precache(void)
{
	g_sModelIndexFireball = PRECACHE_MODELINDEX("sprites/zerogxplode.spr");// fireball
	g_sModelIndexSmoke = PRECACHE_MODELINDEX("sprites/steam1.spr");// smoke
	g_sModelIndexTinySpit = PRECACHE_MODELINDEX("sprites/tinyspit.spr");// spore
	g_sModelIndexWExplosion = PRECACHE_MODELINDEX("sprites/WXplo1.spr");// underwater fireball
	g_sModelIndexBubbles = PRECACHE_MODELINDEX("sprites/bubble.spr");//bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODELINDEX("sprites/bloodspray.spr"); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODELINDEX("sprites/blood.spr"); // splattered blood 

	g_sModelIndexLaser = PRECACHE_MODELINDEX( (char *)g_pModelNameLaser );
	g_sModelIndexLaserDot = PRECACHE_MODELINDEX("sprites/laserdot.spr");

	PRECACHE_MODEL("models/w_grenade.mdl");
}

void MonsterCommand(void)
{
	int index;
	char msg[256];
	int monster_type = -1;

	if (CMD_ARGC() >= 3)
	{
		const char *parg1 = CMD_ARGV(1);

		// check for a valid monster name...
		for (index = 0; monster_types[index].name[0]; index++)
		{
			// ensure it's an actual monster classname
			if (strncmp(parg1, "monster", 7) == 0)
			{
				if (strcmp(parg1, monster_types[index].name) == 0)
				{
					monster_type = index;
					break;
				}
			}
		}

		if (monster_type != -1)
		{
			// check for a valid player name or index...
			const char *parg2 = CMD_ARGV(2);
			int player_index = -1;
			edict_t *pPlayer;
			const char *player_name;
	
			if (*parg2 == '#')	 // player index
			{
				if (sscanf(&parg2[1], "%d", &player_index) != 1)
				player_index = -1;

				if ((player_index < 1) || (player_index > gpGlobals->maxClients))
				{
					//META_CONS("[MONSTER] invalid player index!	 (%d to %d allowed)", 1, gpGlobals->maxClients);
					LOG_MESSAGE(PLID, "invalid player index! (%d to %d allowed)", 1, gpGlobals->maxClients);
					player_index = -1;
				}
			}
			else
			{
				for (index = 1; index <= gpGlobals->maxClients; index++)
				{
				pPlayer = INDEXENT(index);
	
				if (pPlayer && !pPlayer->free)
				{
					if (stricmp(STRING(pPlayer->v.netname), parg2) == 0)
					{
						player_index = index;	// found the matching player name
						break;
					}
				}
				}
	
				if (player_index == -1)
				{
					//META_CONS("[MONSTER] can't find player named \"%s\"!", parg2);
					LOG_MESSAGE(PLID, "can't find player named \"%s\"!", parg2);
					return;
				}
			}

			if (player_index != -1)
			{
				pPlayer = INDEXENT(player_index);

				if ((pPlayer == NULL) || (pPlayer->free))
				{
					//META_CONS("[MONSTER] player index %d is not a valid player!", player_index);
					LOG_MESSAGE(PLID, "player index %d is not a valid player!", player_index);
					return;
				}

				player_name = STRING(pPlayer->v.netname);

				if (player_name[0] == 0)
				{
					//META_CONS("[MONSTER] player index %d is not a valid player!", player_index);
					LOG_MESSAGE(PLID, "player index %d is not a valid player!", player_index);
					return;
				}

				if (!UTIL_IsAlive(pPlayer))
				{
					//META_CONS("[MONSTER] player \"%s\" is not alive or is an observer!", player_name);
					LOG_MESSAGE(PLID, "player \"%s\" is not alive or is an observer!", player_name);
					return;
				}

				TraceResult tr;
				Vector origin = pPlayer->v.origin;
				Vector view_angle = pPlayer->v.v_angle;
				Vector v_src, v_dest;
				Vector monster_angle;
				
				// If spawning a turret, add autostart spawnflag. Zero otherwise
				int spawnflags = 0;
				if (monster_type >= 15 && monster_type <= 17) // Turret, Mini-Turret and Sentry
					spawnflags = SF_MONSTER_TURRET_AUTOACTIVATE;
				
				// try to determine the best place to spawn the monster...

				view_angle.x = 0;  // zero the pitch (level horizontally)

				UTIL_MakeVectors(view_angle);

				v_src = origin + Vector(0, 0, 20);	// up a little bit
				v_dest = v_src + gpGlobals->v_forward * 128;  // in front of player

				UTIL_TraceHull(v_src, v_dest, dont_ignore_monsters, 1, pPlayer, &tr);
	
				if (tr.flFraction >= 1.0)
				{
					v_src = v_dest;
					v_dest = v_dest + Vector(0, 0, -200);  // down to ground

					// try to find the floor...
					UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pPlayer, &tr);

					if (tr.flFraction < 1.0)	 // hit something?
					{
						monster_angle.y = view_angle.y + 180.0f;  // face the player
						if (monster_angle.y > 360)
							monster_angle.y -= 360;
						if (monster_angle.y < 0)
							monster_angle.y += 360;

						spawn_monster(monster_type, v_src, monster_angle, spawnflags, NULL);

						return;
					}
				}

				v_src = origin + Vector(0, 0, 20);	// up a little bit
				// diagonally in front and to the left of the player...
				v_dest = v_src + gpGlobals->v_forward * 90 + gpGlobals->v_right * -90;

				UTIL_TraceHull(v_src, v_dest, dont_ignore_monsters, 1, pPlayer, &tr);

				if (tr.flFraction >= 1.0)
				{
					v_src = v_dest;
					v_dest = v_dest + Vector(0, 0, -200);  // down to ground

					// try to find the floor...
					UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pPlayer, &tr);

					if (tr.flFraction < 1.0)	 // hit something?
					{
						monster_angle.y = view_angle.y - 135.0f;  // face the player
						if (monster_angle.y > 360)
							monster_angle.y -= 360;
						if (monster_angle.y < 0)
							monster_angle.y += 360;

						spawn_monster(monster_type, v_src, monster_angle, spawnflags, NULL);

						return;
					}
				}

				v_src = origin + Vector(0, 0, 20);	// up a little bit
				// diagonally in front and to the right of the player...
				v_dest = v_src + gpGlobals->v_forward * 90 + gpGlobals->v_right * 90;

				UTIL_TraceHull(v_src, v_dest, dont_ignore_monsters, 1, pPlayer, &tr);

				if (tr.flFraction >= 1.0)
				{
					v_src = v_dest;
					v_dest = v_dest + Vector(0, 0, -200);  // down to ground

					// try to find the floor...
					UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pPlayer, &tr);

					if (tr.flFraction < 1.0)	 // hit something?
					{
						monster_angle.y = view_angle.y + 135.0f;  // face the player
						if (monster_angle.y > 360)
							monster_angle.y -= 360;
						if (monster_angle.y < 0)
							monster_angle.y += 360;

						spawn_monster(monster_type, v_src, monster_angle, spawnflags, NULL);

						return;
					}
				}

				v_src = origin + Vector(0, 0, 20);	// up a little bit
				v_dest = v_src + gpGlobals->v_right * 128;	// to the right

				UTIL_TraceHull(v_src, v_dest, dont_ignore_monsters, 1, pPlayer, &tr);

				if (tr.flFraction >= 1.0)
				{
					v_src = v_dest;
					v_dest = v_dest + Vector(0, 0, -200);  // down to ground

					// try to find the floor...
					UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pPlayer, &tr);

					if (tr.flFraction < 1.0)	 // hit something?
					{
						monster_angle.y = view_angle.y + 90.0f;	 // face the player
						if (monster_angle.y > 360)
							monster_angle.y -= 360;
						if (monster_angle.y < 0)
							monster_angle.y += 360;

						spawn_monster(monster_type, v_src, monster_angle, spawnflags, NULL);

						return;
					}
				}

				v_src = origin + Vector(0, 0, 20);	// up a little bit
				v_dest = v_src + gpGlobals->v_right * -128;	 // to the left

				UTIL_TraceHull(v_src, v_dest, dont_ignore_monsters, 1, pPlayer, &tr);

				if (tr.flFraction >= 1.0)
				{
					v_src = v_dest;
					v_dest = v_dest + Vector(0, 0, -200);  // down to ground

					// try to find the floor...
					UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pPlayer, &tr);

					if (tr.flFraction < 1.0)	 // hit something?
					{
						monster_angle.y = view_angle.y - 90.0f;	 // face the player
						if (monster_angle.y > 360)
							monster_angle.y -= 360;
						if (monster_angle.y < 0)
							monster_angle.y += 360;

						spawn_monster(monster_type, v_src, monster_angle, spawnflags, NULL);

						return;
					}
				}

				v_src = origin + Vector(0, 0, 20);	// up a little bit
				v_dest = v_src + gpGlobals->v_forward * -128;  // to the rear

				UTIL_TraceHull(v_src, v_dest, dont_ignore_monsters, 1, pPlayer, &tr);

				if (tr.flFraction >= 1.0)
				{
					v_src = v_dest;
					v_dest = v_dest + Vector(0, 0, -200);  // down to ground

					// try to find the floor...
					UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pPlayer, &tr);

					if (tr.flFraction < 1.0)	 // hit something?
					{
						monster_angle.y = view_angle.y;	 // face the player
						if (monster_angle.y > 360)
							monster_angle.y -= 360;
						if (monster_angle.y < 0)
							monster_angle.y += 360;

						spawn_monster(monster_type, v_src, monster_angle, spawnflags, NULL);

						return;
					}
				}

				//META_CONS("[MONSTER] there's no room to spawn a monster near player \"%s\"!", player_name);
				LOG_MESSAGE(PLID, "there's no room to spawn a monster near player \"%s\"!", player_name);

				return;
			}
		}
	}

	//META_CONS("[MONSTER] usage: monster monster_name player_name | #player_index");
	//META_CONS("[MONSTER] valid monster_names are:");
	LOG_MESSAGE(PLID, "usage: monster monster_name player_name | #player_index");
	LOG_MESSAGE(PLID, "valid monster_names are:");
	msg[0] = 0;
	for (index = 0; monster_types[index].name[0]; index++)
	{
		// limit list to monster entities only
		if (strncmp(monster_types[index].name, "monster", 7) == 0)
		{
			strcat(msg, monster_types[index].name);
			strcat(msg, " ");
			if (strlen(msg) > 60)
			{
				//META_CONS("[MONSTER] %s", msg);
				LOG_MESSAGE(PLID, "%s", msg);
				msg[0] = 0;
			}
		}
	}
	if (msg[0])
	{
		//META_CONS("[MONSTER] %s", msg);
		LOG_MESSAGE(PLID, "%s", msg);
	}
}

void SpawnViewerCommand(void)
{
	int index;
	
	// debug command to spawn a node_viewer at a player's location
	if (CMD_ARGC() >= 2)
	{
		// check for a valid player name or index...
		const char *parg2 = CMD_ARGV(1);
		int player_index = -1;
		edict_t *pPlayer;
		const char *player_name;

		if (*parg2 == '#')	 // player index
		{
			if (sscanf(&parg2[1], "%d", &player_index) != 1)
			player_index = -1;

			if ((player_index < 1) || (player_index > gpGlobals->maxClients))
			{
				//META_CONS("[MONSTER] invalid player index!	 (%d to %d allowed)", 1, gpGlobals->maxClients);
				LOG_MESSAGE(PLID, "invalid player index! (%d to %d allowed)", 1, gpGlobals->maxClients);
				player_index = -1;
				return;
			}
		}
		else
		{
			for (index = 1; index <= gpGlobals->maxClients; index++)
			{
				pPlayer = INDEXENT(index);
	
				if (pPlayer && !pPlayer->free)
				{
					if (stricmp(STRING(pPlayer->v.netname), parg2) == 0)
					{
						player_index = index;	// found the matching player name
						break;
					}
				}
			}

			if (player_index == -1)
			{
				//META_CONS("[MONSTER] can't find player named \"%s\"!", parg2);
				LOG_MESSAGE(PLID, "can't find player named \"%s\"!", parg2);
				return;
			}
		}
		
		if (player_index != -1)
		{
			pPlayer = INDEXENT(player_index);

			if ((pPlayer == NULL) || (pPlayer->free))
			{
				//META_CONS("[MONSTER] player index %d is not a valid player!", player_index);
				LOG_MESSAGE(PLID, "player index %d is not a valid player!", player_index);
				return;
			}

			player_name = STRING(pPlayer->v.netname);

			if (player_name[0] == 0)
			{
				//META_CONS("[MONSTER] player index %d is not a valid player!", player_index);
				LOG_MESSAGE(PLID, "player index %d is not a valid player!", player_index);
				return;
			}

			if (!UTIL_IsAlive(pPlayer))
			{
				//META_CONS("[MONSTER] player \"%s\" is not alive or is an observer!", player_name);
				LOG_MESSAGE(PLID, "player \"%s\" is not alive or is an observer!", player_name);
				return;
			}

			Vector origin = pPlayer->v.origin;
			
			CMBaseEntity *pViewer = CreateClassPtr((CNodeViewer *)NULL);
			if (pViewer == NULL)
			{
				//META_CONS("[MONSTER] ERROR: Error Creating Node!" );
				LOG_MESSAGE(PLID, "ERROR: Error Creating Viewer!");
				return;
			}
			
			pViewer->pev->origin = origin;
			pViewer->Spawn();
			return;
		}
	}

	LOG_MESSAGE(PLID, "usage: node_viewer player_name | #player_index");
	LOG_MESSAGE(PLID, "spawns a node viewer at the player's location");
}

void mmGameDLLInit( void )
{
	// one time initialization stuff here...

	RETURN_META(MRES_IGNORED);
}


int mmDispatchSpawn( edict_t *pent )
{
	int index;
	char *pClassname = (char *)STRING(pent->v.classname);

	if (strcmp(pClassname, "worldspawn") == 0)
	{
		// free any monster class memory not previously freed...
		for (index = 0; index < MAX_MONSTER_ENTS; index++)
		{
			if (monsters[index].pMonster != NULL)
				delete monsters[index].pMonster;
		}
		
		// free any allocated keyvalue memory
		for (index = 0; index < MAX_MONSTERS; index++)
		{
			if (monster_spawnpoint[index].keyvalue != NULL)
				free(monster_spawnpoint[index].keyvalue);
			monster_spawnpoint[index].keyvalue = NULL;
		}
		
		// do level initialization stuff here...

		for (index = 0; monster_types[index].name[0]; index++)
			monster_types[index].need_to_precache = FALSE;

		CVAR_SET_STRING("monster_gmr", "");
		CVAR_SET_STRING("monster_gsr", "");
		REPLACER::Init();

		monster_spawn_count = 0;
		node_spawn_count = 0;
		
		monster_skill_init();

		process_monster_precache_cfg();

		process_monster_cfg();

		// precache last in the event of a GMR being present
		world_precache();

		// node support. -Giegue
		// init the WorldGraph.
		WorldGraph.InitGraph();
		check_graph_time = gpGlobals->time + 2.00; // give enough gap

		check_respawn_time = gpGlobals->time + 4.00;

		for (index = 0; index < MAX_MONSTER_ENTS; index++)
		{
			monsters[index].monster_index = 0;
			monsters[index].monster_pent = NULL;
			monsters[index].killed = FALSE;  // not killed yet
			monsters[index].pMonster = NULL;
		}

		monster_ents_used = 0;

		for (index = 0; index < ARRAYSIZE(gDecals); index++ )
			gDecals[index].index = DECAL_INDEX( gDecals[index].name );
	}

	// 0==Success, -1==Failure ?
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

void mmDispatchThink( edict_t *pent )
{
	for (int index=0; index < monster_ents_used; index++)
	{
		if (pent == monsters[index].monster_pent)
		{
			monsters[index].pMonster->Think();

			check_monster_dead(pent);

			RETURN_META(MRES_SUPERCEDE);
		}
	}
	
	RETURN_META(MRES_IGNORED);
}
// HACKHACK -- this is a hack to keep the node graph entity from "touching" things (like triggers)
// while it builds the graph
BOOL gTouchDisabled = FALSE;
void mmDispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
	if (gTouchDisabled)
		RETURN_META(MRES_SUPERCEDE);
	
	for (int index=0; index < monster_ents_used; index++)
	{
		if ((pentTouched != NULL) && (pentTouched == monsters[index].monster_pent))
		{
			monsters[index].pMonster->Touch(pentOther);

			check_monster_dead(pentOther);

			RETURN_META(MRES_SUPERCEDE);
		}
	}

	RETURN_META(MRES_IGNORED);
}

// pfnUse has been deprecated so the only way to trigger a monstermod
// entity from the outside is to do it manually. ARRGHH! -Giegue
void mmDispatchUse( void )
{
	if ( CMD_ARGC() >= 6 ) // the command itself is an argument, we need 5. so argc == 6
	{
		edict_t *entity = INDEXENT( atoi( CMD_ARGV( 1 ) ) );
		edict_t *caller = INDEXENT( atoi( CMD_ARGV( 2 ) ) );
		edict_t *activator = INDEXENT( atoi( CMD_ARGV( 3 ) ) );
		USE_TYPE useType = USE_TYPE( atoi( CMD_ARGV( 4 ) ) );
		float flValue = atof( CMD_ARGV( 5 ) );
		
		// nevermind the unoptimization that this brings... >C
		for (int index=0; index < monster_ents_used; index++)
		{
			if ((entity != NULL) && (entity == monsters[index].monster_pent))
			{
				if ( FNullEnt( caller ) ) caller = NULL;
				if ( FNullEnt( activator ) ) activator = NULL;

				monsters[index].pMonster->Use( activator, caller, useType, flValue );
				return;
			}
		}
	}
}

void mmDispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
	for (int index=0; index < monster_ents_used; index++)
	{
		if ((pentKeyvalue != NULL) && (pentKeyvalue == monsters[index].monster_pent))
		{
			monsters[index].pMonster->KeyValue( pkvd );
			RETURN_META(MRES_SUPERCEDE);
		}
	}
	
	RETURN_META(MRES_IGNORED);
}

void mmServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	int index;
	
	// Monsters
	CMAGrunt agrunt; // 0
	CMApache apache;
	CMBarney barney;
	CMBigMomma bigmomma;
	CMBullsquid bullsquid;
	CMController controller;
	CMHAssassin hassassin;
	CMHeadCrab headcrab;
	CMHGrunt hgrunt;
	CMHoundeye houndeye;
	CMISlave islave;
	CMScientist scientist;
	CMSqueakGrenade snark;
	CMZombie zombie;
	CMGargantua gargantua;
	CMTurret turret;
	CMMiniTurret miniturret;
	CMSentry sentry;
	CMGonome gonome;
	CMMassn massn;
	CMOtis otis;
	CMPitdrone pitdrone;
	CMShockRoach shockroach;
	CMStrooper strooper;
	CMVoltigore voltigore;
	CMBabyVoltigore babyvoltigore;
	CMBabyGargantua babygargantua;
	CMHWGrunt hwgrunt;
	CMRGrunt rgrunt;
	CMStukabat stukabat; // 29
	
	// Extra entities
	CMMonsterMaker monstermaker; // 32
	CMAmbientMusic ambientmusic;
	
	g_psv_gravity = CVAR_GET_POINTER( "sv_gravity" );

	(g_engfuncs.pfnAddServerCommand)("monster", MonsterCommand);
	(g_engfuncs.pfnAddServerCommand)("node_viewer", SpawnViewerCommand);
	(g_engfuncs.pfnAddServerCommand)("_use", mmDispatchUse);

	for (index = 0; monster_types[index].name[0]; index++)
	{
		if (monster_types[index].need_to_precache)
		{
			if (dllapi_log->value)
			{
				LOG_MESSAGE(PLID, "Precaching %s models & sounds...", monster_types[index].name);
			}

			switch (index)
			{
				case 0: agrunt.Precache(); break;
				case 1: apache.Precache(); break;
				case 2: barney.Precache(); break;
				case 3: bigmomma.Precache(); break;
				case 4: bullsquid.Precache(); break;
				case 5: controller.Precache(); break;
				case 6: hassassin.Precache(); break;
				case 7: headcrab.Precache(); break;
				case 8: hgrunt.Precache(); break;
				case 9: houndeye.Precache(); break;
				case 10: islave.Precache(); break;
				case 11: scientist.Precache(); break;
				case 12: snark.Precache(); break;
				case 13: zombie.Precache(); break;
				case 14: gargantua.Precache(); break;
				case 15: turret.Precache(); break;
				case 16: miniturret.Precache(); break;
				case 17: sentry.Precache(); break;
				case 18: gonome.Precache(); break;
				case 19: massn.Precache(); break;
				case 20: otis.Precache(); break;
				case 21: pitdrone.Precache(); break;
				case 22: shockroach.Precache(); break;
				case 23: strooper.Precache(); break;
				case 24: voltigore.Precache(); break;
				case 25: babyvoltigore.Precache(); break;
				case 26: babygargantua.Precache(); break;
				case 27: hwgrunt.Precache(); break;
				case 28: rgrunt.Precache(); break;
				case 29: stukabat.Precache(); break;
				case 32: monstermaker.Precache(); break;
				//case 33: ambientmusic.Precache(); break;
			}
		}
	}

	for (index = 0; index < MAX_MONSTER_ENTS; index++)
	{
		monsters[index].monster_index = 0;
		monsters[index].monster_pent = NULL;
		monsters[index].killed = FALSE;  // not killed yet
		monsters[index].pMonster = NULL;
	}
	
	for (index = 0; index < 33; index++)
	{
		g_DamageBits[index] = 0;
		g_PlayerKilled[index] = false;
		g_NextMessage[index] = 0.0;
	}

	monster_ents_used = 0;
	
	RETURN_META(MRES_IGNORED);
}

void mmStartFrame( void )
{
	// Don't generate node graph right away
	if (check_graph_time != -1 && check_graph_time <= gpGlobals->time)
	{
		BOOL generateNodes = FALSE;
		
		check_graph_time = -1; // only once
		
		// it could be possible that the mod can generate the node graph
		// on it's own, so we wait a bit before attempting to create ours.
		// if we can use the game's generated graph, stick to that one.
		// if not, then do standard node allocation and spawns. -Giegue

		// make sure the .NOD file is newer than the .BSP file.
		if ( !WorldGraph.CheckNODFile ( ( char * )STRING( gpGlobals->mapname ) ) )
		{
			// NOD file is not present, or is older than the BSP file.
			generateNodes = TRUE;
			WorldGraph.AllocNodes();
		}
		else
		{
			// Load the node graph for this level
			if ( !WorldGraph.FLoadGraph ( (char *)STRING( gpGlobals->mapname ) ) )
			{
				// couldn't load, so alloc and prepare to build a graph.
				generateNodes = TRUE;
				ALERT ( at_console, "*Error opening .NOD file\n" );
				WorldGraph.AllocNodes();
			}
			else
			{
				// node graph is OK, we can spawn the monsters instantly
				check_respawn_time = 0.0;
				ALERT ( at_console, "\n[MONSTER] Graph Loaded!\n" );
			}
		}

		if ( generateNodes )
		{
			// spawn nodes
			int index;
			for (index = 0; index < node_spawn_count; index++)
			{
				CMBaseEntity *pNode;
				pNode = CreateClassPtr((CNodeEnt *)NULL);
				
				if (pNode == NULL)
				{
					//META_CONS("[MONSTER] ERROR: Error Creating Node!" );
					LOG_MESSAGE(PLID, "ERROR: Error Creating Node!");
				}
				else
				{
					pNode->pev->origin = node_spawnpoint[index].origin;
				
					if (node_spawnpoint[index].is_air_node)
						pNode->pev->classname = MAKE_STRING("info_node_air");
					else
						pNode->pev->classname = MAKE_STRING("info_node");
					
					pNode->Spawn();
				}
			}
		}
	}

	// Wait for node graph before spawning the monsters
	if (check_respawn_time <= gpGlobals->time)
	{
		check_respawn_time = gpGlobals->time + 1.0;

		check_respawn();
	}

	RETURN_META(MRES_IGNORED);
}

void mmClientKill_Post( edict_t *pPlayer )
{
	// Show "commited suicide" message
	pPlayer->v.dmg_inflictor = pPlayer;
	check_player_dead( pPlayer );

	RETURN_META(MRES_IGNORED);
}

static DLL_FUNCTIONS gFunctionTable = 
{
	mmGameDLLInit,	//! pfnGameInit()	Initialize the game (one-time call after loading of game .dll)
	mmDispatchSpawn, //! pfnSpawn()
	mmDispatchThink, //! pfnThink
	NULL,			// pfnUse [DEPRECATED]
	mmDispatchTouch, //! pfnTouch
	NULL,			// pfnBlocked
	mmDispatchKeyValue,	//! pfnKeyValue
	NULL,			// pfnSave
	NULL,			// pfnRestore
	NULL,			// pfnSetAbsBox

	NULL,			// pfnSaveWriteFields
	NULL,			// pfnSaveReadFields

	NULL,			// pfnSaveGlobalState
	NULL,			// pfnRestoreGlobalState
	NULL,			// pfnResetGlobalState

	NULL,			// pfnClientConnect
	NULL,			// pfnClientDisconnect
	NULL,			// pfnClientKill
	NULL,			// pfnClientPutInServer
	NULL,			// pfnClientCommand
	NULL,			// pfnClientUserInfoChanged
	mmServerActivate, //! pfnServerActivate()	 (wd) Server is starting a new map
	NULL,			// pfnServerDeactivate

	NULL,			// pfnPlayerPreThink
	NULL,			// pfnPlayerPostThink

	mmStartFrame,	//! pfnStartFrame
	NULL,			// pfnParmsNewLevel
	NULL,			// pfnParmsChangeLevel

	NULL,			// pfnGetGameDescription
	NULL,			// pfnPlayerCustomization

	NULL,			// pfnSpectatorConnect
	NULL,			// pfnSpectatorDisconnect
	NULL,			// pfnSpectatorThink
   
	NULL,			// pfnSys_Error

	NULL,			// pfnPM_Move
	NULL,			// pfnPM_Init
	NULL,			// pfnPM_FindTextureType
   
	NULL,			// pfnSetupVisibility
	NULL,			// pfnUpdateClientData
	NULL,			// pfnAddToFullPack
	NULL,			// pfnCreateBaseline
	NULL,			// pfnRegisterEncoders
	NULL,			// pfnGetWeaponData
	NULL,			// pfnCmdStart
	NULL,			// pfnCmdEnd
	NULL,			// pfnConnectionlessPacket
	NULL,			// pfnGetHullBounds
	NULL,			// pfnCreateInstancedBaselines
	NULL,			// pfnInconsistentFile
	NULL,			// pfnAllowLagCompensation
};


C_DLLEXPORT int GetEntityAPI2( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
	if(!pFunctionTable) {
		UTIL_LogPrintf("GetEntityAPI2 called with null pFunctionTable");
		return(FALSE);
	}
	else if(*interfaceVersion != INTERFACE_VERSION) {
		UTIL_LogPrintf("GetEntityAPI2 version mismatch; requested=%d ours=%d", *interfaceVersion, INTERFACE_VERSION);
		//! Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return(FALSE);
	}
	memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );
	return(TRUE);
}


void mmDispatchThink_Post( edict_t *pent )
{
	check_monster_hurt(pent);
	check_monster_dead(pent);
	
	RETURN_META(MRES_IGNORED);
}

void mmPlayerPostThink_Post( edict_t *pEntity )
{
	check_monster_hurt(pEntity);
	check_monster_dead(pEntity);
	//check_player_dead(pEntity); // too early for damageBits
	check_monster_info(pEntity);
	
	RETURN_META(MRES_IGNORED);
}

static DLL_FUNCTIONS gFunctionTable_Post = 
{
	NULL,			// pfnGameInit()   Initialize the game (one-time call after loading of game .dll)
	NULL,			// pfnSpawn()
	mmDispatchThink_Post, //! pfnThink
	NULL,			// pfnUse [DEPRECATED]
	NULL,			// pfnTouch
	NULL,			// pfnBlocked
	NULL,			// pfnKeyValue
	NULL,			// pfnSave
	NULL,			// pfnRestore
	NULL,			// pfnSetAbsBox

	NULL,			// pfnSaveWriteFields
	NULL,			// pfnSaveReadFields

	NULL,			// pfnSaveGlobalState
	NULL,			// pfnRestoreGlobalState
	NULL,			// pfnResetGlobalState

	NULL,			// pfnClientConnect
	NULL,			// pfnClientDisconnect
	mmClientKill_Post,			//! pfnClientKill
	NULL,			// pfnClientPutInServer
	NULL,			// pfnClientCommand
	NULL,			// pfnClientUserInfoChanged
	NULL,			// pfnServerActivate()	   (wd) Server is starting a new map
	NULL,			// pfnServerDeactivate

	NULL,			// pfnPlayerPreThink
	mmPlayerPostThink_Post, //! pfnPlayerPostThink

	NULL,			// pfnStartFrame
	NULL,			// pfnParmsNewLevel
	NULL,			// pfnParmsChangeLevel

	NULL,			// pfnGetGameDescription
	NULL,			// pfnPlayerCustomization

	NULL,			// pfnSpectatorConnect
	NULL,			// pfnSpectatorDisconnect
	NULL,			// pfnSpectatorThink
   
	NULL,			// pfnSys_Error

	NULL,			// pfnPM_Move
	NULL,			// pfnPM_Init
	NULL,			// pfnPM_FindTextureType
   
	NULL,			// pfnSetupVisibility
	NULL,			// pfnUpdateClientData
	NULL,			// pfnAddToFullPack
	NULL,			// pfnCreateBaseline
	NULL,			// pfnRegisterEncoders
	NULL,			// pfnGetWeaponData
	NULL,			// pfnCmdStart
	NULL,			// pfnCmdEnd
	NULL,			// pfnConnectionlessPacket
	NULL,			// pfnGetHullBounds
	NULL,			// pfnCreateInstancedBaselines
	NULL,			// pfnInconsistentFile
	NULL,			// pfnAllowLagCompensation
};

C_DLLEXPORT int GetEntityAPI2_Post( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
	if(!pFunctionTable) {
		UTIL_LogPrintf("GetEntityAPI2_Post called with null pFunctionTable");
		return(FALSE);
	}
	else if(*interfaceVersion != INTERFACE_VERSION) {
		UTIL_LogPrintf("GetEntityAPI2_Post version mismatch; requested=%d ours=%d", *interfaceVersion, INTERFACE_VERSION);
		//! Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return(FALSE);
	}
	memcpy( pFunctionTable, &gFunctionTable_Post, sizeof( DLL_FUNCTIONS ) );
	return(TRUE);
}


// Some messages seems to be offset by 1. Linux specific? CStrike specific?
int IsCSServer( void )
{
	char mod[16];
	sprintf( mod, "%s", GET_GAME_INFO( PLID, GINFO_NAME ) );
	
	if ( strcmp( mod, "cstrike" ) == 0 || strcmp( mod, "czero" ) == 0 )
		return 1;
	
	return 0;
}

int mmRegUserMsg_Post( const char *pName, int iSize )
{
	int cs_server = IsCSServer();
	
	if ( strcmp( pName, "Damage" ) == 0 )
		g_DamageMsg = META_RESULT_ORIG_RET( int ) - cs_server;
	
	RETURN_META_VALUE( MRES_IGNORED, 0 );
}

void mmMessageBegin_Post( int msg_dest, int msg_type, const float *pOrigin, edict_t *ed )
{
	if ( msg_type == g_DamageMsg )
	{
		// Whatever hurting us must be a valid entity
		if (ed->v.dmg_inflictor != NULL )
		{
			g_DamageActive = true;
			g_DamageVictim = ENTINDEX( ed );
		}
	}
	
	RETURN_META( MRES_IGNORED );
}

void mmWriteLong_Post( int iValue )
{
	if ( g_DamageActive )
		g_DamageBits[ g_DamageVictim ] = iValue;
	
	RETURN_META( MRES_IGNORED );
}

void mmMessageEnd_Post( void )
{
	if ( g_DamageActive )
	{
		check_player_dead( INDEXENT( g_DamageVictim ) );
	}
	g_DamageActive = false;
	
	RETURN_META( MRES_IGNORED );
}
/*
enginefuncs_t meta_engfuncs =
{
	NULL,			// pfnPrecacheModel()
	NULL,			// pfnPrecacheSound()
	NULL,			// pfnSetModel()
	NULL,			// pfnModelIndex()
	NULL,			// pfnModelFrames()

	NULL,			// pfnSetSize()
	NULL,			// pfnChangeLevel()
	NULL,			// pfnGetSpawnParms()
	NULL,			// pfnSaveSpawnParms()

	NULL,			// pfnVecToYaw()
	NULL,			// pfnVecToAngles()
	NULL,			// pfnMoveToOrigin()
	NULL,			// pfnChangeYaw()
	NULL,			// pfnChangePitch()

	NULL,			// pfnFindEntityByString()
	NULL,			// pfnGetEntityIllum()
	NULL,			// pfnFindEntityInSphere()
	NULL,			// pfnFindClientInPVS()
	NULL,			// pfnEntitiesInPVS()

	NULL,			// pfnMakeVectors()
	NULL,			// pfnAngleVectors()

	NULL,			// pfnCreateEntity()
	NULL,			// pfnRemoveEntity()
	NULL,			// pfnCreateNamedEntity()

	NULL,			// pfnMakeStatic()
	NULL,			// pfnEntIsOnFloor()
	NULL,			// pfnDropToFloor()

	NULL,			// pfnWalkMove()
	NULL,			// pfnSetOrigin()

	NULL,			// pfnEmitSound()
	NULL,			// pfnEmitAmbientSound()

	NULL,			// pfnTraceLine()
	NULL,			// pfnTraceToss()
	NULL,			// pfnTraceMonsterHull()
	NULL,			// pfnTraceHull()
	NULL,			// pfnTraceModel()
	NULL,			// pfnTraceTexture()
	NULL,			// pfnTraceSphere()
	NULL,			// pfnGetAimVector()

	NULL,			// pfnServerCommand()
	NULL,			// pfnServerExecute()
	NULL,			// pfnClientCommand()

	NULL,			// pfnParticleEffect()
	NULL,			// pfnLightStyle()
	NULL,			// pfnDecalIndex()
	NULL,			// pfnPointContents()

	NULL,			// pfnMessageBegin()
	NULL,			// pfnMessageEnd()

	NULL,			// pfnWriteByte()
	NULL,			// pfnWriteChar()
	NULL,			// pfnWriteShort()
	NULL,			// pfnWriteLong()
	NULL,			// pfnWriteAngle()
	NULL,			// pfnWriteCoord()
	NULL,			// pfnWriteString()
	NULL,			// pfnWriteEntity()

	NULL,			// pfnCVarRegister()
	NULL,			// pfnCVarGetFloat()
	NULL,			// pfnCVarGetString()
	NULL,			// pfnCVarSetFloat()
	NULL,			// pfnCVarSetString()

	NULL,			// pfnAlertMessage()
	NULL,			// pfnEngineFprintf()

	NULL,			// pfnPvAllocEntPrivateData()
	NULL,			// pfnPvEntPrivateData()
	NULL,			// pfnFreeEntPrivateData()

	NULL,			// pfnSzFromIndex()
	NULL,			// pfnAllocString()

	NULL, 			// pfnGetVarsOfEnt()
	NULL,			// pfnPEntityOfEntOffset()
	NULL,			// pfnEntOffsetOfPEntity()
	NULL,			// pfnIndexOfEdict()
	NULL,			// pfnPEntityOfEntIndex()
	NULL,			// pfnFindEntityByVars()
	NULL,			// pfnGetModelPtr()

	NULL,			// pfnRegUserMsg()

	NULL,			// pfnAnimationAutomove()
	NULL,			// pfnGetBonePosition()

	NULL,			// pfnFunctionFromName()
	NULL,			// pfnNameForFunction()

	NULL,			// pfnClientPrintf()
	NULL,			// pfnServerPrint()

	NULL,			// pfnCmd_Args()
	NULL,			// pfnCmd_Argv()
	NULL,			// pfnCmd_Argc()

	NULL,			// pfnGetAttachment()

	NULL,			// pfnCRC32_Init()
	NULL,			// pfnCRC32_ProcessBuffer()
	NULL,			// pfnCRC32_ProcessByte()
	NULL,			// pfnCRC32_Final()

	NULL,			// pfnRandomLong()
	NULL,			// pfnRandomFloat()

	NULL,			// pfnSetView()
	NULL,			// pfnTime()
	NULL,			// pfnCrosshairAngle()

	NULL,			// pfnLoadFileForMe()
	NULL,			// pfnFreeFile()

	NULL,			// pfnEndSection()
	NULL,			// pfnCompareFileTime()
	NULL,			// pfnGetGameDir()
	NULL,			// pfnCvar_RegisterVariable()
	NULL,			// pfnFadeClientVolume()
	NULL,			// pfnSetClientMaxspeed()
	NULL,			// pfnCreateFakeClient()
	NULL,			// pfnRunPlayerMove()
	NULL,			// pfnNumberOfEntities()

	NULL,			// pfnGetInfoKeyBuffer()
	NULL,			// pfnInfoKeyValue()
	NULL,			// pfnSetKeyValue()
	NULL,			// pfnSetClientKeyValue()

	NULL,			// pfnIsMapValid()
	NULL,			// pfnStaticDecal()
	NULL,			// pfnPrecacheGeneric()
	NULL, 			// pfnGetPlayerUserId()
	NULL,			// pfnBuildSoundMsg()
	NULL,			// pfnIsDedicatedServer()
	NULL,			// pfnCVarGetPointer()
	NULL,			// pfnGetPlayerWONId()

	NULL,			// pfnInfo_RemoveKey()
	NULL,			// pfnGetPhysicsKeyValue()
	NULL,			// pfnSetPhysicsKeyValue()
	NULL,			// pfnGetPhysicsInfoString()
	NULL,			// pfnPrecacheEvent()
	NULL,			// pfnPlaybackEvent()

	NULL,			// pfnSetFatPVS()
	NULL,			// pfnSetFatPAS()

	NULL,			// pfnCheckVisibility()

	NULL,			// pfnDeltaSetField()
	NULL,			// pfnDeltaUnsetField()
	NULL,			// pfnDeltaAddEncoder()
	NULL,			// pfnGetCurrentPlayer()
	NULL,			// pfnCanSkipPlayer()
	NULL,			// pfnDeltaFindField()
	NULL,			// pfnDeltaSetFieldByIndex()
	NULL,			// pfnDeltaUnsetFieldByIndex()

	NULL,			// pfnSetGroupMask()

	NULL, 			// pfnCreateInstancedBaseline()
	NULL,			// pfnCvar_DirectSet()

	NULL,			// pfnForceUnmodified()

	NULL,			// pfnGetPlayerStats()

	NULL,			// pfnAddServerCommand()

	NULL,			// pfnVoice_GetClientListening()
	NULL,			// pfnVoice_SetClientListening()

	NULL,			// pfnGetPlayerAuthId()

	NULL,			// pfnSequenceGet()
	NULL,			// pfnSequencePickSentence()
	NULL,			// pfnGetFileSize()
	NULL,			// pfnGetApproxWavePlayLen()
	NULL,			// pfnIsCareerMatch()
	NULL,			// pfnGetLocalizedStringLength()
	NULL,			// pfnRegisterTutorMessageShown()
	NULL,			// pfnGetTimesTutorMessageShown()
	NULL,			// pfnProcessTutorMessageDecayBuffer()
	NULL,			// pfnConstructTutorMessageDecayBuffer()
	NULL,			// pfnResetTutorMessageDecayData()
	NULL,			// pfnQueryClientCvarValue()
	NULL,			// pfnQueryClientCvarValue2()
	NULL,			// pfnEngCheckParm()
};

C_DLLEXPORT int GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion) 
{
	if(!pengfuncsFromEngine)
	{
		LOG_ERROR(PLID, "GetEngineFunctions called with null pengfuncsFromEngine");
		return(FALSE);
	}
	else if(*interfaceVersion != ENGINE_INTERFACE_VERSION)
	{
		LOG_ERROR(PLID, "GetEngineFunctions version mismatch; requested=%d ours=%d", *interfaceVersion, ENGINE_INTERFACE_VERSION);
		// Tell metamod what version we had, so it can figure out who is 
		// out of date.
		*interfaceVersion = ENGINE_INTERFACE_VERSION;
		return(FALSE);
	}
	memcpy(pengfuncsFromEngine, &meta_engfuncs, sizeof(enginefuncs_t));
	return TRUE;
}
*/
enginefuncs_t meta_engfuncs_post =
{
	NULL,			// pfnPrecacheModel()
	NULL,			// pfnPrecacheSound()
	NULL,			// pfnSetModel()
	NULL,			// pfnModelIndex()
	NULL,			// pfnModelFrames()

	NULL,			// pfnSetSize()
	NULL,			// pfnChangeLevel()
	NULL,			// pfnGetSpawnParms()
	NULL,			// pfnSaveSpawnParms()

	NULL,			// pfnVecToYaw()
	NULL,			// pfnVecToAngles()
	NULL,			// pfnMoveToOrigin()
	NULL,			// pfnChangeYaw()
	NULL,			// pfnChangePitch()

	NULL,			// pfnFindEntityByString()
	NULL,			// pfnGetEntityIllum()
	NULL,			// pfnFindEntityInSphere()
	NULL,			// pfnFindClientInPVS()
	NULL,			// pfnEntitiesInPVS()

	NULL,			// pfnMakeVectors()
	NULL,			// pfnAngleVectors()

	NULL,			// pfnCreateEntity()
	NULL,			// pfnRemoveEntity()
	NULL,			// pfnCreateNamedEntity()

	NULL,			// pfnMakeStatic()
	NULL,			// pfnEntIsOnFloor()
	NULL,			// pfnDropToFloor()

	NULL,			// pfnWalkMove()
	NULL,			// pfnSetOrigin()

	NULL,			// pfnEmitSound()
	NULL,			// pfnEmitAmbientSound()

	NULL,			// pfnTraceLine()
	NULL,			// pfnTraceToss()
	NULL,			// pfnTraceMonsterHull()
	NULL,			// pfnTraceHull()
	NULL,			// pfnTraceModel()
	NULL,			// pfnTraceTexture()
	NULL,			// pfnTraceSphere()
	NULL,			// pfnGetAimVector()

	NULL,			// pfnServerCommand()
	NULL,			// pfnServerExecute()
	NULL,			// pfnClientCommand()

	NULL,			// pfnParticleEffect()
	NULL,			// pfnLightStyle()
	NULL,			// pfnDecalIndex()
	NULL,			// pfnPointContents()

	mmMessageBegin_Post,	//! pfnMessageBegin()
	mmMessageEnd_Post,			//! pfnMessageEnd()

	NULL,			// pfnWriteByte()
	NULL,			// pfnWriteChar()
	NULL,			// pfnWriteShort()
	mmWriteLong_Post,			//! pfnWriteLong()
	NULL,			// pfnWriteAngle()
	NULL,			// pfnWriteCoord()
	NULL,			// pfnWriteString()
	NULL,			// pfnWriteEntity()

	NULL,			// pfnCVarRegister()
	NULL,			// pfnCVarGetFloat()
	NULL,			// pfnCVarGetString()
	NULL,			// pfnCVarSetFloat()
	NULL,			// pfnCVarSetString()

	NULL,			// pfnAlertMessage()
	NULL,			// pfnEngineFprintf()

	NULL,			// pfnPvAllocEntPrivateData()
	NULL,			// pfnPvEntPrivateData()
	NULL,			// pfnFreeEntPrivateData()

	NULL,			// pfnSzFromIndex()
	NULL,			// pfnAllocString()

	NULL, 			// pfnGetVarsOfEnt()
	NULL,			// pfnPEntityOfEntOffset()
	NULL,			// pfnEntOffsetOfPEntity()
	NULL,			// pfnIndexOfEdict()
	NULL,			// pfnPEntityOfEntIndex()
	NULL,			// pfnFindEntityByVars()
	NULL,			// pfnGetModelPtr()

	mmRegUserMsg_Post,			//! pfnRegUserMsg()

	NULL,			// pfnAnimationAutomove()
	NULL,			// pfnGetBonePosition()

	NULL,			// pfnFunctionFromName()
	NULL,			// pfnNameForFunction()

	NULL,			// pfnClientPrintf()
	NULL,			// pfnServerPrint()

	NULL,			// pfnCmd_Args()
	NULL,			// pfnCmd_Argv()
	NULL,			// pfnCmd_Argc()

	NULL,			// pfnGetAttachment()

	NULL,			// pfnCRC32_Init()
	NULL,			// pfnCRC32_ProcessBuffer()
	NULL,			// pfnCRC32_ProcessByte()
	NULL,			// pfnCRC32_Final()

	NULL,			// pfnRandomLong()
	NULL,			// pfnRandomFloat()

	NULL,			// pfnSetView()
	NULL,			// pfnTime()
	NULL,			// pfnCrosshairAngle()

	NULL,			// pfnLoadFileForMe()
	NULL,			// pfnFreeFile()

	NULL,			// pfnEndSection()
	NULL,			// pfnCompareFileTime()
	NULL,			// pfnGetGameDir()
	NULL,			// pfnCvar_RegisterVariable()
	NULL,			// pfnFadeClientVolume()
	NULL,			// pfnSetClientMaxspeed()
	NULL,			// pfnCreateFakeClient()
	NULL,			// pfnRunPlayerMove()
	NULL,			// pfnNumberOfEntities()

	NULL,			// pfnGetInfoKeyBuffer()
	NULL,			// pfnInfoKeyValue()
	NULL,			// pfnSetKeyValue()
	NULL,			// pfnSetClientKeyValue()

	NULL,			// pfnIsMapValid()
	NULL,			// pfnStaticDecal()
	NULL,			// pfnPrecacheGeneric()
	NULL, 			// pfnGetPlayerUserId()
	NULL,			// pfnBuildSoundMsg()
	NULL,			// pfnIsDedicatedServer()
	NULL,			// pfnCVarGetPointer()
	NULL,			// pfnGetPlayerWONId()

	NULL,			// pfnInfo_RemoveKey()
	NULL,			// pfnGetPhysicsKeyValue()
	NULL,			// pfnSetPhysicsKeyValue()
	NULL,			// pfnGetPhysicsInfoString()
	NULL,			// pfnPrecacheEvent()
	NULL,			// pfnPlaybackEvent()

	NULL,			// pfnSetFatPVS()
	NULL,			// pfnSetFatPAS()

	NULL,			// pfnCheckVisibility()

	NULL,			// pfnDeltaSetField()
	NULL,			// pfnDeltaUnsetField()
	NULL,			// pfnDeltaAddEncoder()
	NULL,			// pfnGetCurrentPlayer()
	NULL,			// pfnCanSkipPlayer()
	NULL,			// pfnDeltaFindField()
	NULL,			// pfnDeltaSetFieldByIndex()
	NULL,			// pfnDeltaUnsetFieldByIndex()

	NULL,			// pfnSetGroupMask()

	NULL, 			// pfnCreateInstancedBaseline()
	NULL,			// pfnCvar_DirectSet()

	NULL,			// pfnForceUnmodified()

	NULL,			// pfnGetPlayerStats()

	NULL,			// pfnAddServerCommand()

	NULL,			// pfnVoice_GetClientListening()
	NULL,			// pfnVoice_SetClientListening()

	NULL,			// pfnGetPlayerAuthId()

	NULL,			// pfnSequenceGet()
	NULL,			// pfnSequencePickSentence()
	NULL,			// pfnGetFileSize()
	NULL,			// pfnGetApproxWavePlayLen()
	NULL,			// pfnIsCareerMatch()
	NULL,			// pfnGetLocalizedStringLength()
	NULL,			// pfnRegisterTutorMessageShown()
	NULL,			// pfnGetTimesTutorMessageShown()
	NULL,			// pfnProcessTutorMessageDecayBuffer()
	NULL,			// pfnConstructTutorMessageDecayBuffer()
	NULL,			// pfnResetTutorMessageDecayData()
	NULL,			// pfnQueryClientCvarValue()
	NULL,			// pfnQueryClientCvarValue2()
	NULL,			// pfnEngCheckParm()
};

C_DLLEXPORT int GetEngineFunctions_Post(enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion) 
{
	if(!pengfuncsFromEngine)
	{
		LOG_ERROR(PLID, "GetEngineFunctions_Post called with null pengfuncsFromEngine");
		return(FALSE);
	}
	else if(*interfaceVersion != ENGINE_INTERFACE_VERSION)
	{
		LOG_ERROR(PLID, "GetEngineFunctions_Post version mismatch; requested=%d ours=%d", *interfaceVersion, ENGINE_INTERFACE_VERSION);
		// Tell metamod what version we had, so it can figure out who is 
		// out of date.
		*interfaceVersion = ENGINE_INTERFACE_VERSION;
		return(FALSE);
	}
	memcpy(pengfuncsFromEngine, &meta_engfuncs_post, sizeof(enginefuncs_t));
	return TRUE;
}
