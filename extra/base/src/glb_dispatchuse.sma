#pragma semicolon 1

#include <amxmodx>
#include <engine>
#include <hamsandwich>

public plugin_init()
{
	register_plugin( "GAME-MONSTER: Use Dispatcher", "1.1", "Giegue" );
	
	register_cvar( "_glb_use", "1" );
	
	// Game --> MonsterMod
	RegisterHam( Ham_Use, "func_wall", "DispatchUse" );
	
	// MonsterMod --> Game
	register_srvcmd( "_trigger", "FireTargets" );
}

public DispatchUse( entity, caller, activator, useType, Float:value )
{
	// all monstermod entities have this set
	if ( entity_get_edict( entity, EV_ENT_euser4 ) )
	{
		server_cmd( "_use %i %i %i %i %f", entity, caller, activator, useType, value );
		return HAM_SUPERCEDE;
	}
	
	return HAM_IGNORED;
}

public FireTargets()
{
	if ( read_argc() == 6 )
	{
		new entity, caller, activator;
		new Float:value;
		new useType;
		
		entity = read_argv_int( 1 );
		caller = read_argv_int( 2 );
		activator = read_argv_int( 3 );
		value = read_argv_float( 4 );
		useType = read_argv_int( 5 );
		
		// caller and activator can be null, but never allow entity to be null
		if ( !is_valid_ent( entity ) )
			return;
		
		if ( !is_valid_ent( caller ) )
			caller = 0;
		if ( !is_valid_ent( activator ) )
			activator = 0;
		
		ExecuteHamB( Ham_Use, entity, caller, activator, useType, value );
	}
}
