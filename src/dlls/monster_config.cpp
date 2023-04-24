#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef __linux__
#include <io.h>
#else
#include <unistd.h>
#endif

#include "extdll.h"
#include "dllapi.h"
#include "meta_api.h"

#include "monster_plugin.h"
#include "ripent.h"
#include "globalreplace.h"

extern cvar_t *dllapi_log;
extern cvar_t *monster_entity_config;

extern cvar_t *globalmodellist;
extern cvar_t *globalsoundlist;

extern monster_type_t monster_types[];
extern int monster_spawn_count;
extern int node_spawn_count;

bool get_input(FILE *fp, char *input)
{
	char line[1024];
	int len, pos;

	while (!feof(fp))
	{
		if (fgets(line, 1023, fp) != NULL)
		{
			len = strlen(line);

			if (len == 0)
				continue;  // skip any null lines

			// remove any trailing newline, carriage return or whitespace...
			while ((line[len-1] == '\n') || (line[len-1] == '\r') || isspace(line[len-1]))
			{
				line[len-1] = 0;
				len--;
				if (len == 0)
					break;
			}

			pos = 0;

			while (isspace(line[pos]))
				pos++;  // skip leading blanks

			if ((line[pos] == '/') && (line[pos+1] == '/'))
				continue;  // skip comment lines

			if (line[pos] == 0)
				continue;  // skip empty lines

			strcpy(input, &line[pos]);
			return TRUE;
		}
	}

	return FALSE;  // no input found
}

void scan_monster_cfg(FILE *fp)
{
	// Let's make a full rework of this. -Giegue
	char input[1024];
	float x, y, z;
	bool badent, monster, node;
	
	while (get_input(fp, input))
	{
		badent = monster = node = FALSE;
		if (input[0] == '{')
		{
			// Proper start, initialize entity creation
			// Temporary variables to store entity data
			pKVD *data = (pKVD*)malloc(MAX_KEYVALUES*sizeof(*data)); // Entities should not have more than this many keyvalues
			int kvd_index = 0;
			while (get_input(fp, input))
			{
				// It's the end of the entity structure?
				if (input[0] == '}')
				{
					// Check if the classname of whatever we want to spawn is valid.
					if (strcmp(data[kvd_index-1].key, "classname") == 0)
					{
						int mIndex;
						for (mIndex = 0; monster_types[mIndex].name[0]; mIndex++)
						{
							if (strcmp(data[kvd_index-1].value, monster_types[mIndex].name) == 0)
							{
								// Now that I think about it this looks slow and bad code >.>
								
								// A match is found. What is this?
								if (strncmp(monster_types[mIndex].name, "monster_", 8) == 0)
								{
									// It's a monster, add it to the list
									if (monster_spawn_count == MAX_MONSTERS)
									{
										// Ouch! Not enough room.
										LOG_MESSAGE(PLID, "ERROR: can't add monster, reached MAX_MONSTERS!"); // It will get spammy, sadly.
										badent = TRUE;
									}
									else
									{
										monster_spawnpoint[monster_spawn_count].monster = mIndex;
										monster_types[mIndex].need_to_precache = TRUE;
										monster = TRUE;
									}
								}
								else if (strcmp(monster_types[mIndex].name, "monstermaker") == 0)
								{
									// A monster spawner, add it to the list
									if (monster_spawn_count == MAX_MONSTERS)
									{
										// error.exe
										LOG_MESSAGE(PLID, "ERROR: can't add monstermaker, reached MAX_MONSTERS!");
										badent = TRUE;
									}
									else
									{
										monster_spawnpoint[monster_spawn_count].monster = mIndex;
										monster_types[mIndex].need_to_precache = TRUE;
										monster = TRUE;
									}
								}
								else if (strcmp(monster_types[mIndex].name, "ambient_music") == 0)
								{
									// TODO - Extra entities should go towards a separate counter like nodes
									if (monster_spawn_count == MAX_MONSTERS)
									{
										LOG_MESSAGE(PLID, "ERROR: can't add ambient_music, reached MAX_MONSTERS!");
										badent = TRUE;
									}
									else
									{
										monster_spawnpoint[monster_spawn_count].monster = mIndex;
										monster_types[mIndex].need_to_precache = TRUE;
										monster = TRUE;
									}
								}
								else if (strcmp(monster_types[mIndex].name, "info_node") == 0)
								{
									// Normal node
									if (node_spawn_count == MAX_NODES)
									{
										// The map can't be THAT big can it?
										LOG_MESSAGE(PLID, "ERROR: can't add node, reached MAX_NODES!"); // zee spam bOi
										badent = TRUE;
									}
									else
										node = TRUE;
								}
								else if (strcmp(monster_types[mIndex].name, "info_node_air") == 0)
								{
									// Aerial node
									if (node_spawn_count == MAX_NODES)
									{
										// Ctrl+C --> Ctrl+V
										LOG_MESSAGE(PLID, "ERROR: can't add node, reached MAX_NODES!"); // poppo was here.
										badent = TRUE;
									}
									else
									{
										node_spawnpoint[node_spawn_count].is_air_node = TRUE;
										node = TRUE;
									}
								}
								break;
							}
						}
						if (monster_types[mIndex].name[0] == 0)
						{
							LOG_MESSAGE(PLID, "ERROR: unknown classname: %s", data[kvd_index-1].value); // print conflictive line
							LOG_MESSAGE(PLID, "ERROR: nothing will spawn here!");
							badent = TRUE;
						}
					}
					else
					{
						// What are you doing?!
						LOG_MESSAGE(PLID, "ERROR: BAD ENTITY STRUCTURE! Last line was %s", data[kvd_index-1].key); // print conflictive line
						LOG_MESSAGE(PLID, "ERROR: classname MUST be the last entry of the entity!" );
						badent = TRUE;
					}
					
					if (!badent)
					{
						// Make room for entity-specific keyvalues.
						if (monster)
						{
							// The line is a little too long, no?
							monster_spawnpoint[monster_spawn_count].keyvalue = (pKVD*)calloc(MAX_KEYVALUES, sizeof(*monster_spawnpoint[monster_spawn_count].keyvalue));
						}
						
						// Done. Let's process the keyvalues.
						for (int i = 0; i < (kvd_index-1); i++)
						{
							// Any duplicate keyvalue is overwritten.
							
							if (strcmp(data[i].key, "origin") == 0)
							{
								if (sscanf(data[i].value, "%f %f %f", &x, &y, &z) != 3)
								{
									LOG_MESSAGE(PLID, "ERROR: invalid origin: %s", input); // print conflictive line
									
									// reset origin to g_vecZero
									LOG_MESSAGE(PLID, "ERROR: entity will spawn at 0 0 0");
									x = y = z = 0;
								}
								if (monster)
								{
									monster_spawnpoint[monster_spawn_count].origin[0] = x;
									monster_spawnpoint[monster_spawn_count].origin[1] = y;
									monster_spawnpoint[monster_spawn_count].origin[2] = z;
								}
								else if (node)
								{
									node_spawnpoint[node_spawn_count].origin[0] = x;
									node_spawnpoint[node_spawn_count].origin[1] = y;
									node_spawnpoint[node_spawn_count].origin[2] = z;
								}
							}
							else if (strcmp(data[i].key, "angles") == 0)
							{
								if (monster)
								{
									if (sscanf(data[i].value, "%f %f %f", &x, &y, &z) != 3)
									{
										LOG_MESSAGE(PLID, "ERROR: invalid angles: %s", input); // print conflictive line
										
										// reset angles to g_vecZero
										LOG_MESSAGE(PLID, "ERROR: entity angles will be set to 0 0 0");
										x = y = z = 0;
									}
									monster_spawnpoint[monster_spawn_count].angles[0] = x;
									monster_spawnpoint[monster_spawn_count].angles[1] = y;
									monster_spawnpoint[monster_spawn_count].angles[2] = z;
								}
							}
							else if (strcmp(data[i].key, "spawnflags") == 0)
							{
								if (monster)
								{
									if (sscanf(data[i].value, "%f", &x) != 1)
									{
										LOG_MESSAGE(PLID, "ERROR: invalid spawnflags: %s", input); // print conflictive line
										
										// default to no spawnflags
										LOG_MESSAGE(PLID, "ERROR: entity spawnflags will be set to none (0)");
										x = 0;
									}
									monster_spawnpoint[monster_spawn_count].spawnflags = x;
								}
							}
							else if (strcmp(data[i].key, "model") == 0)
							{
								if (monster)
								{
									// only applicable for normal monsters
									if (strcmp(data[kvd_index-1].value, "monstermaker") != 0)
									{
										// precache the custom model here
										PRECACHE_MODEL( data[i].value );

										// the entity will need the keyvalue
										strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
										strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
									}
								}
							}
							else if (strcmp(data[i].key, "new_model") == 0)
							{
								if (monster)
								{
									// only applicable for monstermaker entity
									if (strcmp(data[kvd_index-1].value, "monstermaker") == 0)
									{
										// precache the custom model
										PRECACHE_MODEL( data[i].value );

										// the entity will need the keyvalue as well
										strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
										strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
									}
								}
							}
							else if (strcmp(data[i].key, "monstertype") == 0)
							{
								if (monster)
								{
									// this keyvalue is only valid for monstermaker entity
									if (strcmp(data[kvd_index-1].value, "monstermaker") == 0)
									{
										// process the entity precache here
										int mIndex;
										for (mIndex = 0; monster_types[mIndex].name[0]; mIndex++)
										{
											if (strcmp(data[i].value, monster_types[mIndex].name) == 0)
											{
												monster_types[mIndex].need_to_precache = TRUE;
												break; // only one monster at a time
											}
										}

										// pass the keyvalue to the entity
										strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
										strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
									}
								}
							}
							else if (strcmp(data[i].key, "message") == 0)
							{
								if (monster)
								{
									// only applicable for ambient_music
									if (strcmp(data[kvd_index - 1].value, "ambient_music") == 0)
									{
										// precache the sound here
										PRECACHE_GENERIC(data[i].value);

										// the entity will need the keyvalue
										strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
										strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
									}
								}
							}
							else
							{
								// We do not know this keyvalue, but an specific entity might use it.
								// Save it for later
								if (monster)
								{
									strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
									strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
								}
							}
						}
						
						if (monster)
						{
							// Spawn right away
							monster_spawnpoint[monster_spawn_count].need_to_respawn = TRUE;
							monster_spawn_count++;
						}
						else if (node)
						{
							// Increase node count
							node_spawn_count++;
						}
						
						// Log on? Print all the entities that were added
						if (dllapi_log->value)
						{
							// Classname only, or we will flood the server!
							// No, I'm not making this idiotproof. Classname should be the last KVD entry on an entity!
							LOG_CONSOLE(PLID, "[DEBUG] Added entity: %s", data[kvd_index-1].value);
						}
					}
					
					free( data );
					break;
				}
				
				// Bruteforce to remove quotes
				char parse[66] = {0};
				int skip = 0;
				for (unsigned i = 0; i < strlen(input); i++)
				{
					if (input[i] == '"')
					{
						skip++;
						continue;
					}
					parse[i-skip] = input[i];
				}
				parse[strlen(parse)] = '\0';
				
				// Copy all keyvalues to the tempvar
				// Key
				char *copy = strtok(parse, " ");
				strcpy(data[kvd_index].key, copy);
				
				// Value
				copy = strtok(NULL, " ");
				strcpy(data[kvd_index].value, "");
				while (copy != NULL)
				{
					// If the value is a vector, append necessary whitespaces
					strcat(data[kvd_index].value, copy);
					copy = strtok(NULL, " ");
					if (copy != NULL)
						strcat(data[kvd_index].value, " ");
				}
				
				// Next KVD
				kvd_index++;
			}
		}
	}
}

void scan_monster_bsp(void)
{
	// TODO: code duplication galore! optimize this for T5 milestone. -Giegue
	epair_t *kv_pair;
	pKVD data[MAX_KEYVALUES];
	
	int kvd_index;
	bool use_monstermod_explicit;
	bool use_monstermod;
	
	int classname_kvdI, mIndex;
	float x, y, z;
	bool badent, monster, node;
	
	// go through all entities
	for (int ent = 1; ent < num_entities; ent++)
	{
		kv_pair = entities[ent].epairs;
		
		kvd_index = 0;
		use_monstermod_explicit = false;
		use_monstermod = true;
		
		classname_kvdI = 0;
		badent = monster = node = false;
		
		// examine all keys
		while (kv_pair != NULL)
		{
			// entities cannot be this big!
			if (kvd_index >= MAX_KEYVALUES)
			{
				LOG_MESSAGE(PLID, "WARNING: can't process entity #%i - too many keyvalues", ent);
				use_monstermod = false;
				use_monstermod_explicit = false;
				break;
			}

			if (strcmp(kv_pair->key, "classname") == 0)
			{
				// the entity we are trying to spawn could already exist within the game
				// use the engine's CREATE_NAMED_ENTITY to see if it's valid or not
				// 
				// if it is valid, this entity already exists and we should ignore it
				edict_t *existsGAME = CREATE_NAMED_ENTITY( MAKE_STRING( kv_pair->value ) );
				if ( !FNullEnt( existsGAME ) )
				{
					// use REMOVE_ENTITY instead of UTIL_Remove!
					// UTIL_Remove sets FL_KILLME to remove the entity on the next frame, that won't do.
					// REMOVE_ENTITY instead removes it instantly, which is needed to prevent server crashes
					// due to "ED_Alloc: no free edicts" error.
					
					REMOVE_ENTITY( existsGAME ); // get rid of the temporary entity
					use_monstermod = false; // stick with game entity
				}
			}
			else if (strcmp(kv_pair->key, "use_monstermod") == 0)
			{
				if (atoi(kv_pair->value) == 1)
				{
					// EXPLICITY requested to use monstermod for this entity
					use_monstermod_explicit = true;
				}
			}
			
			strcpy(data[kvd_index].key, kv_pair->key);
			strcpy(data[kvd_index].value, kv_pair->value);
			
			kvd_index++;
			kv_pair = kv_pair->next;
		}
		
		// spawn a monstermod entity?
		if (use_monstermod_explicit || use_monstermod)
		{
			// find classname keyvalue
			for (int i = 0; i < kvd_index; i++)
			{
				if (strcmp(data[i].key, "classname") == 0)
				{
					for (mIndex = 0; monster_types[mIndex].name[0]; mIndex++)
					{
						if (strcmp(data[i].value, monster_types[mIndex].name) == 0)
						{
							// Match found, check if it's a node
							if (strcmp(monster_types[mIndex].name, "info_node") == 0)
							{
								// Normal node
								if (node_spawn_count == MAX_NODES)
								{
									LOG_MESSAGE(PLID, "ERROR: can't add node, reached MAX_NODES!");
									badent = true;
								}
								else
									node = true;
							}
							else if (strcmp(monster_types[mIndex].name, "info_node_air") == 0)
							{
								// Aerial node
								if (node_spawn_count == MAX_NODES)
								{
									LOG_MESSAGE(PLID, "ERROR: can't add node, reached MAX_NODES!");
									badent = true;
								}
								else
								{
									node_spawnpoint[node_spawn_count].is_air_node = true;
									node = true;
								}
							}
							else
							{
								// Assume it's a monster and add it to the list
								// (Extra entities are built as CMBaseMonster)
								if (monster_spawn_count == MAX_MONSTERS)
								{
									LOG_MESSAGE(PLID, "ERROR: can't add entity, reached MAX_MONSTERS!");
									badent = true;
								}
								else
								{
									monster_spawnpoint[monster_spawn_count].monster = mIndex;
									monster_types[mIndex].need_to_precache = true;
									monster = true;
								}
							}
							classname_kvdI = i;
							break;
						}
					}
					if (monster_types[mIndex].name[0] == 0)
					{
						LOG_MESSAGE(PLID, "unknown classname: %s", data[i].value);
						badent = true;
					}
				}
			}
			
			if (!badent)
			{
				// Make room for entity-specific keyvalues.
				if (monster)
				{
					// Can I use malloc/calloc again or you are going to crash cuz you feel like it? >.>
					monster_spawnpoint[monster_spawn_count].keyvalue = (pKVD*)calloc(MAX_KEYVALUES, sizeof(*monster_spawnpoint[monster_spawn_count].keyvalue));
				}
				
				// process entity keyvalues
				for (int i = 0; i < kvd_index; i++)
				{
					// duplicates are overwritten
					if (strcmp(data[i].key, "origin") == 0)
					{
						if (sscanf(data[i].value, "%f %f %f", &x, &y, &z) != 3)
						{
							LOG_MESSAGE(PLID, "ERROR: invalid origin: %s", data[i].value); // print conflictive line
							
							// reset origin to g_vecZero
							LOG_MESSAGE(PLID, "ERROR: entity will spawn at 0 0 0");
							x = y = z = 0;
						}
						if (monster)
						{
							monster_spawnpoint[monster_spawn_count].origin[0] = x;
							monster_spawnpoint[monster_spawn_count].origin[1] = y;
							monster_spawnpoint[monster_spawn_count].origin[2] = z;
						}
						else if (node)
						{
							node_spawnpoint[node_spawn_count].origin[0] = x;
							node_spawnpoint[node_spawn_count].origin[1] = y;
							node_spawnpoint[node_spawn_count].origin[2] = z;
						}
					}
					else if (strcmp(data[i].key, "angles") == 0)
					{
						if (monster)
						{
							if (sscanf(data[i].value, "%f %f %f", &x, &y, &z) != 3)
							{
								LOG_MESSAGE(PLID, "ERROR: invalid angles: %s", data[i].value); // print conflictive line
								
								// reset angles to g_vecZero
								LOG_MESSAGE(PLID, "ERROR: entity angles will be set to 0 0 0");
								x = y = z = 0;
							}
							monster_spawnpoint[monster_spawn_count].angles[0] = x;
							monster_spawnpoint[monster_spawn_count].angles[1] = y;
							monster_spawnpoint[monster_spawn_count].angles[2] = z;
						}
					}
					else if (strcmp(data[i].key, "spawnflags") == 0)
					{
						if (monster)
						{
							if (sscanf(data[i].value, "%f", &x) != 1)
							{
								LOG_MESSAGE(PLID, "ERROR: invalid spawnflags: %s", data[i].value); // print conflictive line
								
								// default to no spawnflags
								LOG_MESSAGE(PLID, "ERROR: entity spawnflags will be set to none (0)");
								x = 0;
							}
							monster_spawnpoint[monster_spawn_count].spawnflags = x;
						}
					}
					else if (strcmp(data[i].key, "model") == 0)
					{
						if (monster)
						{
							// only applicable for normal monsters
							if (strcmp(data[classname_kvdI].value, "monstermaker") != 0)
							{
								// precache the custom model here
								PRECACHE_MODEL( data[i].value );

								// the entity will need the keyvalue
								strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
								strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
							}
						}
					}
					else if (strcmp(data[i].key, "new_model") == 0)
					{
						if (monster)
						{
							// only applicable for monstermaker entity
							if (strcmp(data[classname_kvdI].value, "monstermaker") == 0)
							{
								// precache the custom model
								PRECACHE_MODEL( data[i].value );

								// the entity will need the keyvalue as well
								strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
								strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
							}
						}
					}
					else if (strcmp(data[i].key, "monstertype") == 0)
					{
						if (monster)
						{
							// this keyvalue is only valid for monstermaker entity
							if (strcmp(data[classname_kvdI].value, "monstermaker") == 0)
							{
								// process the entity precache here
								for (mIndex = 0; monster_types[mIndex].name[0]; mIndex++)
								{
									if (strcmp(data[i].value, monster_types[mIndex].name) == 0)
									{
										monster_types[mIndex].need_to_precache = TRUE;
										break; // only one monster at a time
									}
								}

								// pass the keyvalue to the entity
								strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
								strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
							}
						}
					}
					else if (strcmp(data[i].key, "message") == 0)
					{
						if (monster)
						{
							// only applicable for ambient_music
							if (strcmp(data[classname_kvdI].value, "ambient_music") == 0)
							{
								// precache the sound here
								PRECACHE_GENERIC(data[i].value);

								// the entity will need the keyvalue
								strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
								strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
							}
						}
					}
					else
					{
						// We do not know this keyvalue, but an specific entity might use it.
						// Save it for later
						if (monster)
						{
							strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].key, data[i].key);
							strcpy(monster_spawnpoint[monster_spawn_count].keyvalue[i].value, data[i].value);
						}
					}
				}
				
				if (monster)
				{
					// Spawn right away
					monster_spawnpoint[monster_spawn_count].need_to_respawn = TRUE;
					monster_spawn_count++;
				}
				else if (node)
				{
					// Increase node count
					node_spawn_count++;
				}
				
				// Log on? Print all the entities that were added
				if (dllapi_log->value)
				{
					// Classname only, or we will flood the server!
					LOG_CONSOLE(PLID, "[DEBUG] Added entity: %s", data[classname_kvdI].value);
				}
			}
		}
	}
}

void scan_extra_cfg(FILE *fp)
{
	char input[1024];
	
	while (get_input(fp, input))
	{
		char *cmd = strtok(input, " ");
		char *value = strtok(NULL, " ");
		if (value == NULL)
			continue; // command with no value, skip

		// Remove all quotes from "value"
		char parse[128] = {0};
		int skip = 0;
		for (unsigned i = 0; i < strlen(value); i++)
		{
			if (value[i] == '"')
			{
				skip++;
				continue;
			}
			parse[i-skip] = value[i];
		}
		parse[strlen(parse)] = '\0';
		strcpy(value, parse);

		if (strcmp(cmd, "globalmodellist") == 0)
		{
			// ugh...
			//globalmodellist->string = value;
			CVAR_SET_STRING( "monster_gmr", value );

			// Verbose if logging is enabled
			if (dllapi_log->value)
				LOG_CONSOLE(PLID, "[DEBUG] Using global model replacement file: %s", value);
		}
		else if (strcmp(cmd, "globalsoundlist") == 0)
		{
			//globalsoundlist->string = value;
			CVAR_SET_STRING( "monster_gsr", value );

			// Verbose if logging is enabled
			if (dllapi_log->value)
				LOG_CONSOLE(PLID, "[DEBUG] Using global sound replacement file: %s", value);
		}
	}
}

void scan_monster_replace(FILE *fp, bool toGSR )
{
	char input[1024];
	
	while (get_input(fp, input))
	{
		char *source = strtok(input, " ");
		char *destination = strtok(NULL, " ");

		// Remove all quotes
		char parse[128] = {0};
		int skip;

		// source
		skip = 0;
		for (unsigned i = 0; i < strlen(source); i++)
		{
			if (source[i] == '"')
			{
				skip++;
				continue;
			}
			parse[i-skip] = source[i];
		}
		parse[strlen(parse)] = '\0';
		strcpy(source, parse);

		// destination
		memset(parse, 0, sizeof(parse));
		skip = 0;
		for (unsigned i = 0; i < strlen(destination); i++)
		{
			if (destination[i] == '"')
			{
				skip++;
				continue;
			}
			parse[i-skip] = destination[i];
		}
		parse[strlen(parse)] = '\0';
		strcpy(destination, parse);

		if ( toGSR )
			REPLACER::AddGlobalSound( source, destination );
		else
			REPLACER::AddGlobalModel( source, destination );
	}
}

bool process_monster_cfg(void)
{
	char game_dir[256];
	char BSPfilename[256]; // to read ents directly from BSP
	char CFGfilename[256]; // read ents from MAPNAME_monster.cfg file
	char EXTfilename[256]; // extra map configs from MAPNAME.cfg
	FILE *fp = NULL;

	monster_spawn_count = 0;
	node_spawn_count = 0;

	// find the directory name of the currently running MOD...
	(*g_engfuncs.pfnGetGameDir)(game_dir);

	// build route...
	strcpy(CFGfilename, game_dir);
	strcat(CFGfilename, "/maps/");
	strcat(CFGfilename, STRING(gpGlobals->mapname));
	strcpy(BSPfilename, CFGfilename);
	strcpy(EXTfilename, CFGfilename);

	strcat(BSPfilename, ".bsp");
	strcat(CFGfilename, "_monster.cfg");
	strcat(EXTfilename, ".cfg");

	// process config files?
	// -1 = don't process monster config, dynamic spawns only
	// 0 = read entities from BSP file
	// 1 = read entities from CFG file
	// 2 = read entities from both, BSP first, then CFG file
	if (monster_entity_config->value >= 0)
	{
		// read from bsp? (mode 0 or 2)
		if (monster_entity_config->value != 1)
		{
			LoadBSPFile(BSPfilename);
			ParseEntities();
			
			scan_monster_bsp();
		}
		
		// read from cfg? (mode 1 or 2)
		if (monster_entity_config->value > 0)
		{
			// check if the map specific filename exists...
			if (access(CFGfilename, 0) == 0)
			{
				if (dllapi_log->value)
				{
					//META_CONS("[MONSTER] Processing config file=%s", filename);
					LOG_MESSAGE(PLID, "Processing config file '%s'", CFGfilename);
				}

				if ((fp = fopen(CFGfilename, "r")) == NULL)
				{
					//META_CONS("[MONSTER] ERROR: Could not open \"%s\"!", filename);
					LOG_MESSAGE(PLID, "ERROR: Could not open \"%s\" file!", CFGfilename);
					return TRUE; // error
				}

				scan_monster_cfg(fp);

				fclose(fp);
			}
		}
	}
	
	/* The code is only getting worse from here, I have to finish T4 quickly
	* so I can move into making actual clean and optimized code for the final tier...
	* -Giegue */

	// extra map configs
	if (access(EXTfilename, 0) == 0)
	{
		// first read configs
		if ((fp = fopen(EXTfilename, "r")) != NULL)
		{
			scan_extra_cfg(fp);
			fclose(fp);
		}
		
		// then process them here
		if (strlen(globalmodellist->string))
		{
			char gmrPath[192];

			// SC globalmodellist path starts from models/MAPNAME
			sprintf(gmrPath, "%s/models/%s/%s", game_dir, STRING(gpGlobals->mapname), globalmodellist->string);

			if (access(gmrPath, 0) == 0)
			{
				if ((fp = fopen(gmrPath, "r")) != NULL)
				{
					scan_monster_replace(fp, false);
					fclose(fp);
				}
			}
		}
		if (strlen(globalsoundlist->string))
		{
			char gsrPath[192];

			// SC globalsoundlist path starts from sound/MAPNAME
			sprintf(gsrPath, "%s/sound/%s/%s", game_dir, STRING(gpGlobals->mapname), globalsoundlist->string);

			if (access(gsrPath, 0) == 0)
			{
				if ((fp = fopen(gsrPath, "r")) != NULL)
				{
					scan_monster_replace(fp, true);
					fclose(fp);
				}
			}
		}
	}

	return FALSE; // all ok
}


bool scan_monster_precache_cfg(FILE *fp)
{
	char input[1024];
	bool found;

	while (get_input(fp, input))
	{
		found = FALSE;

		for (int index=0; monster_types[index].name[0]; index++)
		{
			if (strcmp(input, monster_types[index].name) == 0)
			{
				monster_types[index].need_to_precache = TRUE;
				found = TRUE;
				break;
			}
		}

		if (found == FALSE)
		{
			//META_CONS("[MONSTER] ERROR: invalid precache monster name: %s", input);
			LOG_MESSAGE(PLID, "ERROR: invalid precache monster name: %s", input);
		}
	}

	return FALSE;
}


bool process_monster_precache_cfg(void)
{
	char game_dir[256];
	char filename[256];
	FILE *fp = NULL;
	bool status = FALSE;  // no error

	// find the directory name of the currently running MOD...
	(*g_engfuncs.pfnGetGameDir)(game_dir);

	strcpy(filename, game_dir);
	strcat(filename, "/monster_precache.cfg");

	// check if the map specific filename exists...
	if (access(filename, 0) == 0)
	{
		if (dllapi_log->value)
		{
			//META_CONS("[MONSTER] Processing config file=%s", filename);
			LOG_MESSAGE(PLID, "Processing config file=%s", filename);
		}

		if ((fp = fopen(filename, "r")) == NULL)
		{
			//META_CONS("[MONSTER] ERROR: Could not open \"%s\"!", filename);
			LOG_MESSAGE(PLID, "ERROR: Could not open \"%s\" file!", filename);

			return TRUE;  // return bad status
		}

		status = scan_monster_precache_cfg(fp);

		fclose(fp);
	}

	return status;
}
