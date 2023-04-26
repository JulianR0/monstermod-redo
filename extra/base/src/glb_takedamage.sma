#pragma semicolon 1

#include <amxmodx>
#include <engine>
#include <hamsandwich>

public plugin_init()
{
	register_plugin( "GAME-MONSTER: External TakeDamage", "1.0", "Giegue" );
	
	register_cvar( "_glb_takedamage", "1" );
	
	// MonsterMod --> Game
	register_srvcmd( "_takedamage", "TakeDamage" );
}

public TakeDamage()
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
