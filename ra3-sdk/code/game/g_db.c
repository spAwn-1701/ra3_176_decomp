#include "g_local.h"

#ifndef Q3_VM

void DB_UpdateAliases( gentity_t *ent ) {
	char *err;
	int rows;
	int cols;
	int count;
	int i;
	char **res;
	char buf[2048];
	char aliases[10][MAX_NETNAME];
	char netname[MAX_NETNAME];
	int found;

	err = NULL;
	rows = -1;
	cols = -1;
	count = 0;
	found = 0;

	if ( !ent || !ent->client ) {
		return;
	}

	if ( sqlite_get_table_printf( level.db, "SELECT Name, Aliases FROM %s WHERE ID=%d", &res, &rows, &cols, &err, tblNames[TBL_PLAYERS],
	                              ent->client->pers.dbId ) ) {
		G_LogPrintf( "<DB> Query error: %s\n", err );
	}

	memset( buf, 0, sizeof( buf ) );
	memset( aliases, 0, sizeof( aliases ) );
	strcpy( netname, ent->client->pers.netname );

	if ( rows >= 1 ) {
		strcpy( buf, res[cols + 1] );

		count = atoi( Info_ValueForKey( buf, "n" ) );

		for ( i = 0; i < count; i++ ) {
			strcpy( aliases[i], Info_ValueForKey( buf, va( "a%d", i ) ) );
		}

		if ( !Q_stricmp( netname, res[cols] ) ) {
			found = 1;
		}

		if ( !found ) {
			for ( i = 0; i < count; i++ ) {
				if ( !Q_stricmp( aliases[i], netname ) ) {
					found = 1;
					break;
				}
			}
		}

		if ( !found ) {
			// FIXME count shouldn't wrap to 0, but clamp like in UpdateDBID
			strcpy( aliases[count], netname );

			sprintf( buf, "n\\%d\\a0\\%s\\a1\\%s\\a2\\%s\\a3\\%s\\a4\\%s\\a5\\%s\\a6\\%s\\a7\\%s\\a8\\%s\\a9\\%s",
			         ( count + 1 ) >= 10 ? 0 : count + 1, aliases[0], aliases[1], aliases[2], aliases[3], aliases[4], aliases[5],
			         aliases[6], aliases[7], aliases[8], aliases[9] );
		}

		if ( sqlite_exec_printf( level.db, "UPDATE %s SET Aliases='%q' WHERE ID=%d", NULL, NULL, &err, tblNames[TBL_PLAYERS], buf,
		                         ent->client->pers.dbId ) ) {
			G_LogPrintf( "<DB> Query error: %s\n", err );
		}
	} else {
		G_Error( "Could not find \"%s\" with id %d in the database!", netname, ent - g_entities );
	}

	sqlite_free_table( res );
}

int G_OpenDB( sqlite **db ) {
	char *err;

	err = NULL;

	G_GetCvarMutex( &fs_homepath );
	G_GetCvarMutex( &fs_game );
	*db = sqlite_open( va( "%s/%s/ra3.db", fs_homepath.string, fs_game.string ), 0, &err );
	G_ReleaseCvarMutex( &fs_game );
	G_ReleaseCvarMutex( &fs_homepath );

	if ( *db == NULL ) {
		return 0;
	}

	return 1;
}

#endif
