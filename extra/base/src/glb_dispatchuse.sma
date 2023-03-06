#pragma semicolon 1

#include <amxmodx>
#include <engine>
#include <hamsandwich>

public plugin_init()
{
	register_plugin( "GAME-MONSTER: Use Dispatcher", "1.0", "Giegue" );
	
	RegisterHam( Ham_Use, "func_wall", "DispatchUse" );
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
