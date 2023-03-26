/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/

// this is a stripped down "bspfile.c" file containing only entity data similar
// to using a ripent tool to read ents from a bsp, hence the name.
// -Giegue
#include "extdll.h"
#include "ripent.h"
#include "meta_api.h"

#if defined linux
#include <sys/stat.h>
#endif

//=============================================================================

int			entdatasize;
char		dentdata[MAX_MAP_ENTSTRING];
int			dentdata_checksum;

int			num_entities;
entity_t	entities[MAX_MAP_ENTITIES];

//=============================================================================

dheader_t	*header;

int CopyLump(int lump, void *dest, int size)
{
	int		length, ofs;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;

	if (length % size)
	{
		LOG_MESSAGE(PLID, "LoadBSPFile: odd lump size");
		return 0;
	}

	memcpy(dest, (byte *)header + ofs, length);

	return length / size;
}

/*
=============
LoadBSPFile
=============
*/
void	LoadBSPFile(char *filename)
{
	int			i;

	//
	// load the file header
	//
	if (LoadFile(filename, (void **)&header) == -1)
		return;
	
	// UNDONE: there is no need to swap it...? -Giegue
	// swap the header
	/*for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);*/
	
	// game will not load the BSP if it's invalid.
	// so if this is called, it means something went really wrong loading it
	if (header->version != BSPVERSION)
	{
		LOG_MESSAGE(PLID, "%s is version %i, not %i", filename, header->version, BSPVERSION);
		return;
	}
	
	entdatasize = CopyLump(LUMP_ENTITIES, dentdata, 1);
	
	free(header);		// everything has been copied out
}

//============================================================================

/*
=================
ParseEpair
=================
*/
epair_t *ParseEpair(void)
{
	epair_t	*e;

	e = (epair_t*)malloc(sizeof(epair_t));
	memset(e, 0, sizeof(epair_t));

	if (strlen(token) >= MAX_KEY - 1)
	{
		LOG_MESSAGE(PLID, "ParseEpar: token too long [strlen(token) >= MAX_KEY - 1]");
		return NULL;
	}
	e->key = copystring(token);
	GetToken(false);
	if (strlen(token) >= MAX_VALUE - 1)
	{
		LOG_MESSAGE(PLID, "ParseEpar: token too long [strlen(token) >= MAX_VALUE - 1]");
		return NULL;
	}
	e->value = copystring(token);

	return e;
}


/*
================
ParseEntity
================
*/
bool ParseEntity(void)
{
	epair_t		*e;
	entity_t	*mapent;

	if (!GetToken(true))
		return false;

	if (strcmp(token, "{"))
	{
		LOG_MESSAGE(PLID, "ParseEntity: { not found");
		return false;
	}

	if (num_entities == MAX_MAP_ENTITIES)
	{
		LOG_MESSAGE(PLID, "num_entities == MAX_MAP_ENTITIES");
		return false;
	}

	mapent = &entities[num_entities];
	num_entities++;

	do
	{
		if (!GetToken(true))
		{
			LOG_MESSAGE(PLID, "ParseEntity: EOF without closing brace");
			return false;
		}
		if (!strcmp(token, "}"))
			break;
		e = ParseEpair();
		e->next = mapent->epairs;
		mapent->epairs = e;
	} while (1);

	return true;
}

/*
================
ParseEntities

Parses the dentdata string into entities
================
*/
void ParseEntities(void)
{
	num_entities = 0;
	ParseFromMemory(dentdata, entdatasize);

	while (ParseEntity())
	{
	}
}


// --
/* MERGE cmdlib.c AND scriplib.c INTO ripent.cpp */
/* Only add needed functions. */
// --

// -- cmdlib.c --
char		qdir[1024] = { '\0' };
int    LoadFile(char *filename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

	f = SafeOpenRead(filename);
	if (f == NULL)
		return -1; // error
	
#if defined (_WIN32)
	length = filelength(fileno(f));
#else
	struct stat st; stat(filename, &st);
	length = st.st_size;
#endif
	buffer = malloc(length + 1);
	((char *)buffer)[length] = 0;
	SafeRead(f, buffer, length);
	fclose(f);

	*bufferptr = buffer;
	return length;
}
int    LittleLong(int l)
{
	byte    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}
char *copystring(char *s)
{
	char	*b;
	b = (char*)malloc(strlen(s) + 1);
	strcpy(b, s);
	return b;
}

FILE *SafeOpenRead(char *filename)
{
	FILE	*f;

	f = fopen(filename, "rb");

	if (!f)
	{
		LOG_MESSAGE(PLID, "Error opening %s: %s", filename, strerror(errno));
		return NULL;
	}

	return f;
}
void SafeRead(FILE *f, void *buffer, int count)
{
	if (fread(buffer, 1, count, f) != (size_t)count)
	{
		LOG_MESSAGE(PLID, "File read failure");
		return;
	}
}
char *ExpandPath(char *path)
{
	char *psz;
	static char full[1024];
	if (!qdir)
	{
		LOG_MESSAGE(PLID, "ExpandPath called without qdir set");
		return NULL;
	}
	if (path[0] == '/' || path[0] == '\\' || path[1] == ':')
		return path;
	psz = strstr(path, qdir);
	if (psz)
		strcpy(full, path);
	else
		sprintf(full, "%s%s", qdir, path);

	return full;
}

// -- scriplib.c --
typedef struct
{
	char	filename[1024];
	char    *buffer, *script_p, *end_p;
	int     line;
} script_t;

#define	MAX_INCLUDES	8
script_t	scriptstack[MAX_INCLUDES];
script_t	*script;
int			scriptline;

char    token[MAXTOKEN];
bool	endofscript;
bool	tokenready;                     // only true if UnGetToken was just called

void ParseFromMemory(char *buffer, int size)
{
	script = scriptstack;
	script++;
	if (script == &scriptstack[MAX_INCLUDES])
	{
		LOG_MESSAGE(PLID, "script file exceeded MAX_INCLUDES");
		return;
	}
	strcpy(script->filename, "memory buffer");

	script->buffer = buffer;
	script->line = 1;
	script->script_p = script->buffer;
	script->end_p = script->buffer + size;

	endofscript = false;
	tokenready = false;
}
bool EndOfScript(bool crossline)
{
	if (!crossline)
	{
		LOG_MESSAGE(PLID, "Line %i is incomplete\n", scriptline);
		return false;
	}

	if (!strcmp(script->filename, "memory buffer"))
	{
		endofscript = true;
		return false;
	}

	free(script->buffer);
	if (script == scriptstack + 1)
	{
		endofscript = true;
		return false;
	}
	script--;
	scriptline = script->line;
	//printf("returning to %s\n", script->filename);
	return GetToken(crossline);
}
void AddScriptToStack(char *filename)
{
	int            size;

	script++;
	if (script == &scriptstack[MAX_INCLUDES])
	{
		LOG_MESSAGE(PLID, "script file exceeded MAX_INCLUDES");
		return;
	}
	strcpy(script->filename, ExpandPath(filename));

	size = LoadFile(script->filename, (void **)&script->buffer);

	//printf("entering %s\n", script->filename);

	script->line = 1;

	script->script_p = script->buffer;
	script->end_p = script->buffer + size;
}
bool GetToken(bool crossline)
{
	char    *token_p;

	if (tokenready)                         // is a token allready waiting?
	{
		tokenready = false;
		return true;
	}

	if (script->script_p >= script->end_p)
		return EndOfScript(crossline);

	//
	// skip space
	//
skipspace:
	while (*script->script_p <= 32)
	{
		if (script->script_p >= script->end_p)
			return EndOfScript(crossline);
		if (*script->script_p++ == '\n')
		{
			if (!crossline)
			{
				LOG_MESSAGE(PLID, "Line %i is incomplete\n", scriptline);
				return false;
			}
			scriptline = script->line++;
		}
	}

	if (script->script_p >= script->end_p)
		return EndOfScript(crossline);

	if (*script->script_p == ';' || *script->script_p == '#' ||		 // semicolon and # is comment field
		(*script->script_p == '/' && *((script->script_p) + 1) == '/')) // also make // a comment field
	{
		if (!crossline)
		{
			LOG_MESSAGE(PLID, "Line %i is incomplete\n", scriptline);
			return false;
		}
		while (*script->script_p++ != '\n')
			if (script->script_p >= script->end_p)
				return EndOfScript(crossline);
		goto skipspace;
	}

	//
	// copy token
	//
	token_p = token;

	if (*script->script_p == '"')
	{
		// quoted token
		script->script_p++;
		while (*script->script_p != '"')
		{
			*token_p++ = *script->script_p++;
			if (script->script_p == script->end_p)
				break;
			if (token_p == &token[MAXTOKEN])
				LOG_MESSAGE(PLID, "Token too large on line %i\n", scriptline);
		}
		script->script_p++;
	}
	else	// regular token
		while (*script->script_p > 32 && *script->script_p != ';')
		{
			*token_p++ = *script->script_p++;
			if (script->script_p == script->end_p)
				break;
			if (token_p == &token[MAXTOKEN])
				LOG_MESSAGE(PLID, "Token too large on line %i\n", scriptline);
		}

	*token_p = 0;

	if (!strcmp(token, "$include"))
	{
		GetToken(false);
		AddScriptToStack(token);
		return GetToken(crossline);
	}

	return true;
}

