#pragma semicolon 1

#include <amxmodx>
#include <engine>
#include <hamsandwich>

// (monster.h from HLSDK) monster to monster relationship types
const R_AL = -2; // (ALLY) pals. Good alternative to R_NO when applicable.
const R_FR = -1; // (FEAR) will run.
const R_NO = 0; // (NO RELATIONSHIP) disregard.
const R_DL = 1; // (DISLIKE) will attack.
const R_HT = 2; // (HATE) will attack this character instead of any visible DISLIKEd characters.
//const R_NM = 3; // (NEMESIS) a monster will ALWAYS attack its nemesis, no matter what.

new Trie:g_HLDefaultNames;

public plugin_init()
{
	register_plugin( "HL-MONSTER Bridge", "1.1", "Giegue" );
	
	RegisterHam( Ham_IRelationship, "monster_alien_controller", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_alien_grunt", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_alien_slave", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_apache", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_babycrab", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_barnacle", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_barney", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_bigmomma", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_bullchicken", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_gargantua", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_headcrab", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_houndeye", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_human_assassin", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_human_grunt", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_ichthyosaur", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_miniturret", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_scientist", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_sentry", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_snark", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_turret", "mmIRelationship" );
	RegisterHam( Ham_IRelationship, "monster_zombie", "mmIRelationship" );
	
	g_HLDefaultNames = TrieCreate();
	
	// Better than a nest of if-else if i guess...
	TrieSetString( g_HLDefaultNames, "monster_headcrab", "Head Crab" );
	TrieSetString( g_HLDefaultNames, "monster_babycrab", "Baby Head Crab" );
	TrieSetString( g_HLDefaultNames, "monster_bullchicken", "Bullsquid" );
	TrieSetString( g_HLDefaultNames, "monster_barnacle", "Barnacle" );
	TrieSetString( g_HLDefaultNames, "monster_bigmomma", "Big Momma" );
	TrieSetString( g_HLDefaultNames, "monster_houndeye", "Houndeye" );
	TrieSetString( g_HLDefaultNames, "monster_alien_slave", "Alien Slave" );
	TrieSetString( g_HLDefaultNames, "monster_alien_controller", "Alien Controller" );
	TrieSetString( g_HLDefaultNames, "monster_alien_grunt", "Alien Grunt" );
	TrieSetString( g_HLDefaultNames, "monster_zombie", "Zombie" );
	TrieSetString( g_HLDefaultNames, "monster_ichthyosaur", "Ichthyosaur" );
	TrieSetString( g_HLDefaultNames, "monster_human_grunt", "Human Grunt" );
	TrieSetString( g_HLDefaultNames, "monster_human_assassin", "Female Assassin" );
	TrieSetString( g_HLDefaultNames, "monster_barney", "Barney" );
	TrieSetString( g_HLDefaultNames, "monster_gman", "Goverment Man" );
	TrieSetString( g_HLDefaultNames, "monster_scientist", "Scientist" );
	TrieSetString( g_HLDefaultNames, "monster_sentry", "Sentry Turret" );
	TrieSetString( g_HLDefaultNames, "monster_snark", "Snark" );
	TrieSetString( g_HLDefaultNames, "monster_miniturret", "Mini-Turret" );
	TrieSetString( g_HLDefaultNames, "monster_turret", "Turret" );
	TrieSetString( g_HLDefaultNames, "monster_apache", "Apache" );
	TrieSetString( g_HLDefaultNames, "monster_osprey", "Osprey Helicopter" );
	TrieSetString( g_HLDefaultNames, "monster_gargantua", "Gargantua" );
	TrieSetString( g_HLDefaultNames, "monster_nihilanth", "Nihilanth" );
	TrieSetString( g_HLDefaultNames, "monster_tentacle", "Tentacle" );
	
	set_task( 0.3, "hlScan", 0, "", 0, "b" );
	register_srvcmd( "monster_hurt_entity", "hlTakeDamage" );
	
	RegisterHam( Ham_Killed, "player", "PlayerKilled", 1 );
}
public plugin_end()
{
	TrieDestroy( g_HLDefaultNames ); // free the handle
}

public mmIRelationship( entity, other )
{
	new selfClassify, otherClassify;
	
	selfClassify = entity_get_int( entity, EV_INT_iuser4 );
	otherClassify = entity_get_int( other, EV_INT_iuser4 );
	
	if ( entity_get_int( other, EV_INT_flags ) & FL_CLIENT )
		otherClassify = 2;
	
	// Get proper relationship
	SetHamReturnInteger( IRelationshipByClass( selfClassify, otherClassify ) );
	return HAM_OVERRIDE;
}

public hlScan()
{
	new entity, szClassname[ 33 ], szDisplayname[ 129 ], bool:found;
	for ( entity = 0; entity < get_global_int( GL_maxEntities ); entity++ )
	{
		// Nothing deleted me?
		if ( is_valid_ent( entity ) )
		{
			entity_get_string( entity, EV_SZ_classname, szClassname, charsmax( szClassname ) );
			if ( equal( szClassname, "monster_", 8 ) )
			{
				// Classify not overriden?
				if ( !entity_get_int( entity, EV_INT_iuser4 ) )
				{
					// Set default
					entity_set_int( entity, EV_INT_iuser4, ExecuteHam( Ham_Classify, entity ) );
				}
				
				// Blood color not overriden?
				if ( !entity_get_int( entity, EV_INT_iuser3 ) )
				{
					// Set default
					entity_set_int( entity, EV_INT_iuser3, ExecuteHam( Ham_BloodColor, entity ) );
				}
				
				// No name set?
				entity_get_string( entity, EV_SZ_netname, szDisplayname, charsmax( szDisplayname ) );
				if ( !strlen( szDisplayname ) )
				{
					// Find a default name
					found = TrieGetString( g_HLDefaultNames, szClassname, szDisplayname, charsmax( szDisplayname ) );
					
					// Use this name
					entity_set_string( entity, EV_SZ_netname, ( found ? szDisplayname : "Monster" ) );
				}
			}
		}
	}
}

stock IRelationshipByClass( classEntity, classOther )
{
	if ( classEntity == -1 )
		classEntity = 0; // CLASS_FORCE_NONE
	
	new iEnemy[16][16] =
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
	
	return iEnemy[ classEntity ][ classOther ];
}

public hlTakeDamage()
{
	if ( read_argc() == 6 )
	{
		new victim, inflictor, attacker;
		new Float:damage;
		new damageBits;
		
		victim = read_argv_int( 1 );
		inflictor = read_argv_int( 2 );
		attacker = read_argv_int( 3 );
		damage = read_argv_float( 4 );
		damageBits = read_argv_int( 5 );
		
		// attacker and inflictor can be null, but never allow victim to be null
		if ( !is_valid_ent( victim ) )
			return;
		
		if ( !is_valid_ent( inflictor ) )
			inflictor = 0;
		if ( !is_valid_ent( attacker ) )
			attacker = 0;
		
		ExecuteHamB( Ham_TakeDamage, victim, inflictor, attacker, damage, damageBits );
	}
}

public PlayerKilled( victim, attacker, shouldgib )
{
	// don't obstruct monstermod
	if ( victim == attacker )
		return HAM_IGNORED;
	
	// fix monster score
	if ( entity_get_int( attacker, EV_INT_flags ) & FL_MONSTER )
		entity_set_float( attacker, EV_FL_frags, entity_get_float( attacker, EV_FL_frags ) + 2 );
	
	entity_set_edict( victim, EV_ENT_dmg_inflictor, attacker );
	return HAM_IGNORED;
}
