#pragma semicolon 1

#include <amxmodx>
#include <engine>
#include <fakemeta>
#include <hamsandwich>

const bits_MEMORY_SOUNDLIST = ( 1 << 3 );
new Trie:m_Sounds;

public plugin_init()
{
	register_plugin( "HL-MONSTER Soundlist", "1.0", "Giegue" );
	
	/* STOP DUPLICATING CODE FFS */
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
}

public plugin_end()
{
	if ( m_Sounds )
		TrieDestroy( m_Sounds );
}

// has to be in precache or it won't work
public plugin_precache()
{
	// check individual monster soundlists
	register_forward( FM_KeyValue, "ScanSL" );
}

public ScanSL( entid, kvd_handle )
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
			
			// Individual sound replacement
			if ( equal( keyname, "soundlist" ) )
			{
				ProcessSoundList( entid, value );
			}
			
			return FMRES_IGNORED;
		}
		// Monster Makers
		else if ( equal( classname, "monstermaker" ) )
		{
			get_kvd( kvd_handle, KV_KeyName, keyname, charsmax( keyname ) );
			get_kvd( kvd_handle, KV_Value, value, charsmax( value ) );
			
			// Children sound list
			if ( equal( keyname, "soundlist" ) )
			{
				ProcessSoundList( entid, value );
			}
			
			return FMRES_IGNORED;
		}
	}
	
	return FMRES_IGNORED;
}

public ProcessSoundList( entity, const filename[] )
{
	// First time?
	if ( !m_Sounds )
		m_Sounds = TrieCreate();
	
	new fullPath[ 129 ];
	
	new mapName[ 33 ], pFile;
	get_mapname( mapName, charsmax( mapName ) );
	
	// path always starts from sound/[MAPNAME] (SC behaviour)
	formatex( fullPath, charsmax( fullPath ), "sound/%s/%s", mapName, filename );
	pFile = fopen( fullPath, "r" );
	if ( pFile )
	{
		new line[ 258 ], soundSrc[ 129 ], soundDest[ 129 ];
		
		while ( fgets( pFile, line, charsmax( line ) ) )
		{
			// Replace newlines
			replace_all( line, charsmax( line ), "^n", "" );
			
			// Ignore blank lines
			if ( !line[ 0 ] ) continue;
			
			// source --> destination
			parse( line, soundSrc, charsmax( soundSrc ), soundDest, charsmax( soundDest ) );
			
			// Precache destination sound
			// HACK: precache_sound outside of plugin_precache
			engfunc( EngFunc_PrecacheSound, soundDest );
			
			// HACK: prepend the entityID at the beginning of the soundSrc for later identification
			format( soundSrc, charsmax( soundSrc ), "%i#%s", entity, soundSrc );
			
			TrieSetString( m_Sounds, soundSrc, soundDest );
			entity_set_int( entity, EV_INT_impulse, entity_get_int( entity, EV_INT_impulse ) | bits_MEMORY_SOUNDLIST );
		}
		
		fclose( pFile );
		
		// file could be empty
		if ( TrieGetSize( m_Sounds ) )
		{
			register_forward( FM_EmitSound, "ReplaceSound" );
		}
	}
}

public ReplaceSound( entity, channel, const sample[], Float:volume, Float:attn, flags, pitch )
{
	static newSound[ 129 ];
	
	// replace monster sound?
	if ( entity_get_int( entity, EV_INT_impulse ) & bits_MEMORY_SOUNDLIST )
	{
		// get entityID
		static owner, entid;
		owner = entity_get_edict( entity, EV_ENT_owner );
		if ( owner )
			entid = owner;
		else
			entid = entity;
		
		// get sound
		static searchSound[ 129 ];
		formatex( searchSound, charsmax( searchSound ), "%i#%s", entid, sample );
		
		// if found, stick to that one
		if ( TrieGetString( m_Sounds, searchSound, newSound, charsmax( newSound ) ) )
		{
			// emit new sound and supercede this one
			emit_sound( entity, channel, newSound, volume, attn, flags, pitch );
			return FMRES_SUPERCEDE;
		}
	}
	
	return FMRES_IGNORED;
}

/* extra_keyvalues.sma duplication */
public MonsterSpawn_Post( entity )
{
	// monstermaker sets owner after monster spawn, wait next frame
	set_task( 0.000001, "MakerSpawn_Post", entity );
}

public MakerSpawn_Post( entity )
{
	if ( is_valid_ent( entity ) )
	{
		static owner;
		owner = entity_get_edict( entity, EV_ENT_owner );
		if ( owner )
		{
			// monstermaker has soundlist defined?
			if ( entity_get_int( owner, EV_INT_impulse ) & bits_MEMORY_SOUNDLIST )
			{
				// 3 time call
				if ( !( entity_get_int( entity, EV_INT_impulse ) & bits_MEMORY_SOUNDLIST ) )
				{
					// this monster is to use sound replacements
					entity_set_int( entity, EV_INT_impulse, entity_get_int( entity, EV_INT_impulse ) | bits_MEMORY_SOUNDLIST );
				}
			}
		}
	}
}
