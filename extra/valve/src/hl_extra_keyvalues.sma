#pragma semicolon 1

#include <amxmodx>
#include <engine>
#include <fakemeta>
#include <hamsandwich>

const bits_MEMORY_SPAWN = ( 1 << 1 );

const BLOOD_COLOR_BLUE = 211;
const BLOOD_COLOR_PINK = 147;
const BLOOD_COLOR_WHITE = 11;
const BLOOD_COLOR_ORANGE = 231;
const BLOOD_COLOR_BLACK = 49; // not 100% black but close enough
const BLOOD_COLOR_DARKGREEN = 181; // GREEN is aliased to YELLOW, use custom name

public plugin_init()
{
	register_plugin( "HL-MONSTER Extra Keyvalues", "1.0", "Giegue" );
	
	/* CLASSIFY */
	// HACK: since Ham_Spawn won't work, this should do as an alternative
	RegisterHam( Ham_SetObjectCollisionBox, "monster_headcrab", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_babycrab", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_bullchicken", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_barnacle", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_bigmomma", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_houndeye", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_alien_slave", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_alien_controller", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_alien_grunt", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_zombie", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_ichthyosaur", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_human_grunt", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_human_assassin", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_barney", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_gman", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_scientist", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_sentry", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_snark", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_miniturret", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_turret", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_apache", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_osprey", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_gargantua", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_nihilanth", "MonsterSpawn_Post", 1 );
	RegisterHam( Ham_SetObjectCollisionBox, "monster_tentacle", "MonsterSpawn_Post", 1 );
	
	/* BLOODCOLOR */
	RegisterHam( Ham_BloodColor, "monster_headcrab", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_babycrab", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_bullchicken", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_barnacle", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_bigmomma", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_houndeye", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_alien_slave", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_alien_controller", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_alien_grunt", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_zombie", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_ichthyosaur", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_human_grunt", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_human_assassin", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_barney", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_gman", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_scientist", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_sentry", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_snark", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_miniturret", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_turret", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_apache", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_osprey", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_gargantua", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_nihilanth", "MonsterBlood" );
	RegisterHam( Ham_BloodColor, "monster_tentacle", "MonsterBlood" );
	
	// "use_monstermod" keyvalue
	register_cvar( "_hl_explicit", "1" );
}

public plugin_precache()
{
	register_forward( FM_KeyValue, "ScanKeys" );
}

public ScanKeys( entid, kvd_handle )
{
	if (is_valid_ent(entid))
	{
		static classname[ 33 ], keyname[ 33 ], value[ 128 ];
		get_kvd( kvd_handle, KV_ClassName, classname, charsmax( classname ) );
		
		// Monsters
		if ( equal( classname, "monster_", 8 ) )
		{
			get_kvd( kvd_handle, KV_KeyName, keyname, charsmax( keyname ) );
			get_kvd( kvd_handle, KV_Value, value, charsmax( value ) );
			
			// Classification override
			if ( equal( keyname, "classify" ) )
				entity_set_int( entid, EV_INT_iuser4, str_to_num( value ) );
			
			// Allied monster
			if ( equal( keyname, "is_player_ally" ) )
			{
				if ( str_to_num( value ) )
					entity_set_int( entid, EV_INT_iuser4, 11 ); // CLASS_PLAYER_ALLY
			}
			
			// Custom blood color
			if ( equal( keyname, "bloodcolor" ) )
			{
				switch ( str_to_num( value ) )
				{
					case -1: entity_set_int( entid, EV_INT_iuser3, DONT_BLEED );
					// 0 = Monster Default
					case 1: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_RED );
					case 2: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_YELLOW );
					case 3: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_BLUE );
					case 4: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_PINK );
					case 5: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_WHITE );
					case 6: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_ORANGE );
					case 7: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_BLACK );
					case 8: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_DARKGREEN );
				}
			}
			
			// Redirect entity to MonsterMod
			if ( equal( keyname, "use_monstermod" ) )
			{
				if ( str_to_num( value ) )
				{
					// MonsterMod will inherit this entity, remove it from here
					remove_entity( entid );
					return FMRES_SUPERCEDE;
				}
			}
			
			return FMRES_IGNORED;
		}
		// Monster Makers
		else if ( equal( classname, "monstermaker" ) )
		{
			get_kvd( kvd_handle, KV_KeyName, keyname, charsmax( keyname ) );
			get_kvd( kvd_handle, KV_Value, value, charsmax( value ) );
			
			// Classification override
			if ( equal( keyname, "classify" ) )
				entity_set_int( entid, EV_INT_iuser4, str_to_num( value ) );
			
			// Allied monster
			if ( equal( keyname, "respawn_as_playerally" ) )
			{
				if ( str_to_num( value ) )
					entity_set_int( entid, EV_INT_iuser4, 11 ); // CLASS_PLAYER_ALLY
			}
			
			// Custom blood color
			if ( equal( keyname, "bloodcolor" ) )
			{
				switch ( str_to_num( value ) )
				{
					case -1: entity_set_int( entid, EV_INT_iuser3, DONT_BLEED );
					// 0 = Monster Default
					case 1: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_RED );
					case 2: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_YELLOW );
					case 3: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_BLUE );
					case 4: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_PINK );
					case 5: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_WHITE );
					case 6: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_ORANGE );
					case 7: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_BLACK );
					case 8: entity_set_int( entid, EV_INT_iuser3, BLOOD_COLOR_DARKGREEN );
				}
			}
			
			// Redirect entity to MonsterMod
			if ( equal( keyname, "use_monstermod" ) )
			{
				if ( str_to_num( value ) )
				{
					// MonsterMod will inherit this entity, remove it from here
					remove_entity( entid );
					return FMRES_IGNORED; // not a typo, must not supercede or it will crash
				}
			}
			
			return FMRES_IGNORED;
		}
	}
	
	return FMRES_IGNORED;
}

public MonsterSpawn_Post( entity )
{
	// Why is it called 3 times? Restrict this to only once
	if ( !( entity_get_int( entity, EV_INT_impulse ) & bits_MEMORY_SPAWN ) )
	{
		// monstermaker sets owner after monster spawn, wait next frame
		set_task( 0.000001, "MakerSpawn_Post", entity );
		
		entity_set_int( entity, EV_INT_impulse, entity_get_int( entity, EV_INT_impulse ) | bits_MEMORY_SPAWN );
	}
}

public MakerSpawn_Post( entity )
{
	if ( is_valid_ent( entity ) )
	{
		static owner;
		owner = entity_get_edict( entity, EV_ENT_owner );
		if ( owner )
		{
			// pass parent configurations to child entity
			entity_set_int( entity, EV_INT_iuser4, entity_get_int( owner, EV_INT_iuser4 ) ); // classify
			entity_set_int( entity, EV_INT_iuser3, entity_get_int( owner, EV_INT_iuser3 ) ); // bloodcolor
		}
	}
}

public MonsterBlood( entity )
{
	static newBlood;
	newBlood = entity_get_int( entity, EV_INT_iuser3 );
	
	if ( newBlood )
	{
		SetHamReturnInteger( newBlood );
		return HAM_OVERRIDE;
	}
	
	return HAM_IGNORED;
}
