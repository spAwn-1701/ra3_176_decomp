// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

// FIXME stubs
void G_TimeShiftAllClients( int time, gentity_t *skip );
void G_UnTimeShiftAllClients( gentity_t *skip );
//

#ifndef Q3_VM

static const char *tblCreate[NUM_TABLES] = {
	"CREATE TABLE IPBans (IP VARCHAR(15), IPMask INT UNSIGNED, Compare INT UNSIGNED, Reason VARCHAR(255), PRIMARY KEY(IPMask, Compare))",
	"CREATE TABLE Callvotes (Votestring VARCHAR(20), Description VARCHAR(255), Command VARCHAR(255), Percentage INT, PRIMARY KEY(Votestring))",
	"CREATE TABLE Players (ID INTEGER PRIMARY KEY, Name VARCHAR(36), GUID CHAR(32), Password VARCHAR(64), Flags INT UNSIGNED, Aliases TEXT, IPAddresses TEXT, Connects INT UNSIGNED, Lastseen DATETIME, Playtime INT UNSIGNED)",
	"CREATE TABLE Sessions (Player INT, Time INT, Duration INT, Data TEXT, PRIMARY KEY (Player, Time))"
};

#endif

level_locals_t	level;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

vmCvar_t	g_gametype;
vmCvar_t	g_dmflags;
vmCvar_t	g_fraglimit;
vmCvar_t	g_timelimit;
vmCvar_t	g_capturelimit;
vmCvar_t	g_location;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_password;
vmCvar_t	g_needpass;
vmCvar_t	g_maxclients;
vmCvar_t	g_maxGameClients;
vmCvar_t	g_dedicated;
vmCvar_t	g_speed;
vmCvar_t	g_gravity;
vmCvar_t	g_cheats;
vmCvar_t	g_knockback;
vmCvar_t	g_quadfactor;
vmCvar_t	g_forcerespawn;
vmCvar_t	g_inactivity;
vmCvar_t	g_debugMove;
vmCvar_t	g_debugDamage;
vmCvar_t	g_debugAlloc;
vmCvar_t	g_weaponRespawn;
vmCvar_t	g_motd;
vmCvar_t	g_synchronousClients;
vmCvar_t	g_warmup;
vmCvar_t	g_doWarmup;
vmCvar_t	g_restarted;
vmCvar_t	g_log;
vmCvar_t	g_logSync;
vmCvar_t	g_blood;
vmCvar_t	g_podiumDist;
vmCvar_t	g_podiumDrop;
vmCvar_t	g_allowVote;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
#ifdef Q3_VM
vmCvar_t	g_banIPs;
#endif
vmCvar_t	g_filterBan;
vmCvar_t	g_smoothClients;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;

vmCvar_t	g_version;
vmCvar_t	g_voteInterval;
vmCvar_t	g_compmodeBlackout;
vmCvar_t	g_timeLeft;
vmCvar_t	g_lightningDamage;
vmCvar_t	net_port;
vmCvar_t	g_chatFlood;
vmCvar_t	g_trackPlayers;
vmCvar_t	g_trackStats;
vmCvar_t	g_statsThreshold;
vmCvar_t	g_rotateLogs;
vmCvar_t	g_funMode;
vmCvar_t	g_autoBalance;
vmCvar_t	g_timeouts;
vmCvar_t	g_httpd;
vmCvar_t	fs_homepath;
vmCvar_t	fs_game;
vmCvar_t	g_votePercent;
vmCvar_t	g_truePing;
vmCvar_t	sv_fps;
vmCvar_t	sv_punkbuster;
vmCvar_t	pb_guids[64];

cvarTable_t		gameCvarTable[] = {
	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, 0, qfalse },

	// noset vars
	{ NULL, "gamename", "arena", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
	{ NULL, "gamedate", "Apr  1 2004", CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },
	{ NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

	// latched vars
	{ &g_gametype, "g_gametype", "8", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

	{ &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

	// change anytime vars
	{ &g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_fraglimit, "fraglimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_ROM | CVAR_NORESTART, 0, qtrue },
	{ &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

	{ &g_location, "location", "0", CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_friendlyFire, "g_friendlyFire", "1", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },

	{ &g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_doWarmup, "g_doWarmup", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_log, "g_log", "games.log", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_logSync, "g_logSync", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },

#ifdef Q3_VM
	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },
#endif
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

	{ &g_speed, "g_speed", "320", 0, 0, qtrue  },
	{ &g_gravity, "g_gravity", "800", 0, 0, qtrue  },
	{ &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
	{ &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
	{ &g_forcerespawn, "g_forcerespawn", "20", 0, 0, qtrue },
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_motd, "g_motd", "", 0, 0, qfalse },
	{ &g_blood, "com_blood", "1", 0, 0, qfalse },

	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

	{ &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse },

	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse },
	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse },
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse },

	{ &g_version, "g_version", "RA3 1.76 Apr  1 2004 18:00:55", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },
	{ &g_voteInterval, "g_voteInterval", "120", CVAR_ARCHIVE, 0, qfalse },
	{ &g_compmodeBlackout, "g_compmodeBlackout", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_timeLeft, "g_timeLeft", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },
	{ &g_lightningDamage, "g_lightningDamage", "0.875", CVAR_ARCHIVE, 0, qfalse },
	{ &net_port, "net_port", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_chatFlood, "g_chatFlood", "2:2:0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_trackPlayers, "g_trackPlayers", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_trackStats, "g_trackStats", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_statsThreshold, "g_statsThreshold", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_rotateLogs, "g_rotateLogs", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_funMode, "g_funMode", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_autoBalance, "g_autoBalance", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_timeouts, "g_timeouts", "2:120", CVAR_ARCHIVE, 0, qfalse },
	{ &g_httpd, "g_httpd", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &fs_homepath, "fs_homepath", "", CVAR_ARCHIVE, 0, qfalse },
	{ &fs_game, "fs_game", "arena", CVAR_ARCHIVE, 0, qfalse },
	{ &g_votePercent, "g_votePercent", "60", CVAR_ARCHIVE, 0, qfalse },
	{ &g_truePing, "g_truePing", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &sv_fps, "sv_fps", "20", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse },

	{ &sv_punkbuster, "sv_punkbuster", "0", 0, 0, qtrue },
	{ &pb_guids[0], "guid_sv_00", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[1], "guid_sv_01", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[2], "guid_sv_02", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[3], "guid_sv_03", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[4], "guid_sv_04", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[5], "guid_sv_05", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[6], "guid_sv_06", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[7], "guid_sv_07", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[8], "guid_sv_08", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[9], "guid_sv_09", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[10], "guid_sv_10", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[11], "guid_sv_11", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[12], "guid_sv_12", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[13], "guid_sv_13", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[14], "guid_sv_14", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[15], "guid_sv_15", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[16], "guid_sv_16", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[17], "guid_sv_17", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[18], "guid_sv_18", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[19], "guid_sv_19", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[20], "guid_sv_20", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[21], "guid_sv_21", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[22], "guid_sv_22", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[23], "guid_sv_23", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[24], "guid_sv_24", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[25], "guid_sv_25", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[26], "guid_sv_26", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[27], "guid_sv_27", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[28], "guid_sv_28", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[29], "guid_sv_29", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[30], "guid_sv_30", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[31], "guid_sv_31", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[32], "guid_sv_32", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[33], "guid_sv_33", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[34], "guid_sv_34", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[35], "guid_sv_35", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[36], "guid_sv_36", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[37], "guid_sv_37", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[38], "guid_sv_38", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[39], "guid_sv_39", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[40], "guid_sv_40", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[41], "guid_sv_41", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[42], "guid_sv_42", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[43], "guid_sv_43", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[44], "guid_sv_44", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[45], "guid_sv_45", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[46], "guid_sv_46", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[47], "guid_sv_47", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[48], "guid_sv_48", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[49], "guid_sv_49", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[50], "guid_sv_50", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[51], "guid_sv_51", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[52], "guid_sv_52", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[53], "guid_sv_53", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[54], "guid_sv_54", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[55], "guid_sv_55", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[56], "guid_sv_56", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[57], "guid_sv_57", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[58], "guid_sv_58", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[59], "guid_sv_59", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[60], "guid_sv_60", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[61], "guid_sv_61", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[62], "guid_sv_62", "EmptySlot", CVAR_ROM, 0, qfalse },
	{ &pb_guids[63], "guid_sv_63", "EmptySlot", CVAR_ROM, 0, qfalse }
};

int		gameCvarTableSize = sizeof( gameCvarTable ) / sizeof( gameCvarTable[0] );


void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
void CheckExitRules( void );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 ) {
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_CLIENT_CONNECT:
		return (int)ClientConnect( arg0, arg1, arg2 );
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientPreBegin( arg0 );
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );
	}

	return -1;
}


void QDECL G_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	trap_Printf( text );
}

void QDECL G_Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	trap_Error( text );
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=1, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf ("%i teams with %i entities\n", c, c2);
}

/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar )
			cv->modificationCount = cv->vmCvar->modificationCount;
	}

	// check some things
	trap_Cvar_Set( "g_gametype", "8" );
	trap_Cvar_Set( "fraglimit", "0" );

	level.warmupModificationCount = g_warmup.modificationCount;
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	int			unknown;
	char		*err;
	int			rows;
	int			cols;
	char		**res;
	int			clientNum;
	char		userinfo[MAX_INFO_STRING];
	char		buf[2048];
	char		ips[10][32];
	char		*s;
	int			count;
	int			j;
	int			newPlayer;
	int			newPassword;

	// FIXME unused
	unknown = 0;

	err = NULL;
	rows = -1;
	cols = -1;

	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->vmCvar == &g_timelimit && g_timelimit.integer == 0 ) {
					trap_Cvar_Set( "g_timeleft", "0" );
				}

#ifndef Q3_VM
				// FIXME should be < or <= MAX_CLIENTS - 1
				if ( cv->vmCvar >= &pb_guids[0] && cv->vmCvar <= &pb_guids[MAX_CLIENTS] && sv_punkbuster.integer && g_trackPlayers.integer ) {
					clientNum = (int)( cv->vmCvar - pb_guids );
					count = 0;
					newPlayer = qfalse;

					// FIXME should be < MAX_CLIENTS
					if ( clientNum >= 0 && clientNum <= MAX_CLIENTS ) {
						if ( g_entities[clientNum].inuse && level.clients[clientNum].pers.connected == CON_CONNECTED ) {
							newPassword = qfalse;

							if ( sqlite_get_table_printf( level.db, "SELECT ID,IPAddresses,Password FROM %s WHERE GUID='%s'", &res, &rows, &cols, &err,
														  tblNames[TBL_PLAYERS], cv->vmCvar->string ) ) {
								G_LogPrintf( "<DB> Query error: %s\n", err );
								continue;
							}

							memset( ips, 0, sizeof( ips ) );
							memset( buf, 0, sizeof( buf ) );

							trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
							s = Info_ValueForKey( userinfo, "ip" );

							if ( rows > 0 ) {
								level.clients[clientNum].pers.dbId = atoi( res[cols] );

								strcpy( buf, res[cols + 1] );

								count = atoi( Info_ValueForKey( buf, "n" ) );

								if ( res[cols + 2] ) {
									size_t len;

									if ( (int)( len = strlen( res[cols + 2] ) ) < 4 ) {
										newPassword = qtrue;
									} else {
										// FIXME this memcpy / strcpy should use the length of client->pers.password
										memcpy( level.clients[clientNum].pers.password, res[cols + 2], len );
									}
								} else {
									newPassword = qtrue;
								}

								for ( j = 0; j < count; j++ ) {
									strcpy( ips[j], Info_ValueForKey( buf, va( "ip%d", j ) ) );
								}

								for ( j = 0; j < count; j++ ) {
									if ( !strcmp( ips[j], s ) ) {
										newPlayer = qtrue;
										break;
									}
								}

								if ( !newPlayer ) {
									strcpy( ips[count], s );

									// FIXME count comparison should be against 10
									sprintf( buf, "n\\%d\\ip0\\%s\\ip1\\%s\\ip2\\%s\\ip3\\%s\\ip4\\%s\\ip5\\%s\\ip6\\%s\\ip7\\%s\\ip8\\%s\\ip9\\%s",
											 (count + 1 ) < 10 ? count + 1 : 0, ips[0], ips[1], ips[2], ips[3], ips[4], ips[5], ips[6], ips[7], ips[8], ips[9] );
								}

								G_LogPrintf( "KnownPlayer: %d %d: %s.\n", clientNum, level.clients[clientNum].pers.dbId,
											 level.clients[clientNum].pers.netname );

								if ( sqlite_exec_printf( level.db, "UPDATE %s SET Connects=Connects+1,IPAddresses='%s' WHERE ID=%d", NULL, NULL, &err,
														 tblNames[TBL_PLAYERS], buf, level.clients[clientNum].pers.dbId ) ) {
									G_LogPrintf( "<DB> Query error: %s\n", err );
									sqlite_free_table( res );
									continue;
								}
							} else {
								int rowId;

								sprintf( buf, "n\\%d\\ip0\\%s\\ip1\\\\ip2\\\\ip3\\\\ip4\\\\ip5\\\\ip6\\\\ip7\\\\ip8\\\\ip9\\", count + 1,
										 Info_ValueForKey( userinfo, "ip" ) );

								if ( sqlite_exec_printf( level.db,
														 "INSERT INTO %s (Name, GUID, Connects, IPAddresses, Aliases, Playtime) VALUES('%q', '%s', 1, "
														 "'%q', 'n\\0\\a0\\\\a1\\\\a2\\\\a3\\\\a4\\\\a5\\\\a6\\\\a7\\\\a8\\\\a9\\', 0)",
														 NULL, NULL, &err, tblNames[TBL_PLAYERS], level.clients[clientNum].pers.netname, cv->vmCvar->string,
														 buf ) ) {
									G_LogPrintf( "<DB> Query error: %s\n", err );
									sqlite_free_table( res );
									continue;
								}

								rowId = sqlite_last_insert_rowid( level.db );
								level.clients[clientNum].pers.dbId = rowId;

								newPassword = qtrue;
							}

							sqlite_free_table( res );

							DB_UpdateAliases( &g_entities[clientNum] );

							trap_SendServerCommand(
								clientNum,
								va( "print \"^1Notice: ^7This server tracks player names and GUIDs for statistical purposes.\nIf you do not "
									"agree with this, please disconnect now.\n^3Your player-id on this server is %d.\n\"",
									level.clients[clientNum].pers.dbId ) );

							if ( newPassword ) {
								trap_SendServerCommand( clientNum, "print \"^1Warning: ^7You have not set a password on this server, please set a "
																   "password by using the \\chpass command.\n\"" );
							}
						}
					}
				}
#endif

				if ( cv->trackChange ) {
					trap_SendServerCommand( -1, va( "print \"Server: %s changed to %s\n\"", cv->cvarName, cv->vmCvar->string ) );
				}
			}
		}
	}
}


/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int		i;
	char	*err;
	int		rows;
	int		cols;
	char	**res;
	char	logFileName[512];
	qtime_t	qt;
	char	serverInfo[MAX_INFO_STRING];

	err = NULL;
	rows = -1;
	cols = -1;

	G_Printf ("------- RA3 Game Initialization -------\n");
	G_Printf ("gamename: %s\n", "RA3 1.76");
	G_Printf ("gamedate: %s\n", "Apr  1 2004");

	srand( randomSeed );

	G_RegisterCvars();

#ifdef Q3_VM
	G_ProcessIPBans();
#endif

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	if ( g_gametype.integer != GT_SINGLE_PLAYER && g_log.string[0] ) {
		if ( g_rotateLogs.integer ) {
			trap_RealTime( &qt );
			Com_snprintf( logFileName, sizeof( logFileName ), "%s_%s_%d%02i%02i_%02i%02i%02i.log", g_log.string, current_mapname(),
			              qt.tm_year + 1900, qt.tm_mon + 1, qt.tm_mday, qt.tm_hour, qt.tm_min, qt.tm_sec );
		} else {
			Com_snprintf( logFileName, sizeof( logFileName ), "%s", g_log.string );
		}

		if ( g_logSync.integer ) {
			trap_FS_FOpenFile( logFileName, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( logFileName, &level.logFile, FS_APPEND );
		}

		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_log.string );
		} else {
			trap_GetServerinfo( serverInfo, sizeof( serverInfo ) );

			G_LogPrintf("------------------------------------------------------------\n" );
			G_LogPrintf("InitGame: %s\n", serverInfo );
		}

		memset( level.logFileName, 0, sizeof( level.logFileName ) );
		strcpy( level.logFileName, logFileName );
	} else {
		G_Printf( "Not logging to disk.\n" );
	}

	G_InitWorldSession();

#ifndef Q3_VM
	// initialize sqlite
	if ( !G_OpenDB( &level.db ) ) {
		// FIXME err is NULL
		G_Error( "<DB> Could not open database: %s\n", err );
	} else {
		G_LogPrintf( "<DB> Database open: %s/%s/ra3.db.\n", fs_homepath.string, fs_game.string );
	}

	for ( i = 0; i < NUM_TABLES; i++ ) {
		G_LogPrintf( "<DB> Checking %s table... ", tblNames[i] );

		if ( sqlite_get_table_printf( level.db, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s'", &res, &rows, &cols, &err,
		                              tblNames[i] ) ) {
			G_LogPrintf( "FAIL (%s).\n", err );
		}
		sqlite_free_table( res );

		if ( rows <= 0 ) {
			G_LogPrintf( "MISSING. Recreating... " );

			if ( sqlite_exec( level.db, tblCreate[i], NULL, NULL, &err ) ) {
				G_LogPrintf( "FAIL (%s).", err );
			} else {
				G_LogPrintf( "SUCCESS.\n" );
			}
		} else {
			G_LogPrintf( "FOUND.\n" );
		}
	}
#endif

	if ( sv_punkbuster.integer ) {
		trap_SendConsoleCommand( EXEC_APPEND, "pb_sv_mod 3\n" );
		trap_SendConsoleCommand( EXEC_APPEND, "pb_sv_specname GTV\n" );
	}

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// initialize top shot stats for this game
	memset( level.topShots, 0, sizeof( level.topShots ) );

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	G_CheckTeamItems();

	SaveRegisteredItems();

	G_Printf ("-----------------------------------\n");

	if( g_gametype.integer == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	arena_init();

	trap_Cvar_Set( "sv_floodprotect", "0" );

#ifndef Q3_VM
	if ( g_dedicated.integer ) {
		G_InitMutexes();
		G_InitThreads();
	}
#endif
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
	int		i;
	char	*err;
	char	lastSeen[MAX_TOKEN_CHARS];
	qtime_t	qt;
	int		playTime;

	G_Printf ("==== ShutdownGame ====\n");

	if ( g_trackPlayers.integer ) {
		for ( i = 0; i < level.maxclients; i++ ) {
			if ( level.clients[i].pers.connected != CON_CONNECTED && level.clients[i].pers.connected != CON_CONNECTING ) {
				continue;
			}

#ifndef Q3_VM
			if ( level.clients[i].pers.dbId ) {
				err = NULL;

				playTime = ( level.time - level.clients[i].pers.enterTime ) / 60000;

				trap_RealTime( &qt );

				sprintf( lastSeen, "%d-%02i-%02i %02i:%02i:%02i", qt.tm_year + 1900, qt.tm_mon + 1, qt.tm_mday, qt.tm_hour, qt.tm_min,
				         qt.tm_sec );

				if ( sqlite_exec_printf( level.db, "UPDATE %s SET Lastseen='%s',Playtime=Playtime+%d WHERE ID=%d", NULL, NULL, &err,
				                         tblNames[TBL_PLAYERS], lastSeen, playTime, level.clients[i].pers.dbId ) ) {
					G_LogPrintf( "<DB> Query error: %s\n", err );
				}

				if ( g_trackStats.integer ) {
					G_WriteSession( &level.clients[i] );
				}
			}
#endif

			trap_SendServerCommand( &g_entities[i] - g_entities, va( "aastop %d", 5 ) );
		}
	}

#ifndef Q3_VM
	if ( level.db ) {
		sqlite_close( level.db );
	}

	if ( g_dedicated.integer ) {
		G_ExitThreads();
		G_DestroyMutexes();
	}
#endif

	if ( level.logFile ) {
		G_LogPrintf("ShutdownGame:\n" );
		G_LogPrintf("------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}
}



//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

void QDECL Com_Error ( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	G_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	G_Printf ("%s", text);
}

#endif

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
	if ( level.intermissiontime ) {
		return;
	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || 
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorTime < nextInLine->sess.spectatorTime ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}


/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}


/*
=======================
AdjustTournamentScores

=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	clientNum = level.sortedClients[0];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
		level.clients[ clientNum ].sess.wins++;
		ClientUserinfoChanged( clientNum );
	}

	clientNum = level.sortedClients[1];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
		level.clients[ clientNum ].sess.losses++;
		ClientUserinfoChanged( clientNum );
	}

}



/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}


	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorTime < cb->sess.spectatorTime ) {
			return -1;
		}
		if ( ca->sess.spectatorTime > cb->sess.spectatorTime ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
	gclient_t	*cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
				level.numVotingClients++;
			}

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
				level.numNonSpectatorClients++;
			
				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;

					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	qsort( level.sortedClients, level.numConnectedClients, 
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( g_gametype.integer >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];

			// FIXME why is the loop still here...
		}
	} else {	
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( g_gametype.integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ||
	     ent->client->sess.spectatorState == SPECTATOR_UNKNOWN ) {
		StopFollowing( ent );
	}

	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;

	Cmd_Stats_f( ent, 0 );
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint ( vec3_origin, level.intermission_origin, level.intermission_angle );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		AdjustTournamentScores();
	}

	level.intermissiontime = level.time;
	FindIntermissionPoint();

	// if single player game
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		UpdateTournamentInfo();
		SpawnModelsOnVictoryPads();
	}

	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			respawn(client);
		}
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();
}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar 

=============
*/
void ExitLevel (void) {
	int		i;

	//bot interbreeding
	BotInterbreedEndMatch();

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		if ( !level.restarted ) {
			RemoveTournamentLoser();
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			level.changemap = NULL;
			level.intermissiontime = 0;
		}
		return;	
	}

	if ( !level.restarting ) {
		trap_SendConsoleCommand( EXEC_APPEND, va( "set nextmap map %s\n", get_next_map( current_mapname() ) ) );
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, va( "set nextmap map %s\n", current_mapname() ) );
	}

	trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	level.changemap = NULL;
	level.intermissiontime = 0;

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< g_maxclients.integer ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024];
	qtime_t		qt;

	trap_RealTime( &qt );
	Com_sprintf( string, sizeof( string ), "[%02d:%02d:%02d] ", qt.tm_hour, qt.tm_min, qt.tm_sec );

	va_start( argptr, fmt );
	vsprintf( string + 11, fmt, argptr );
	va_end( argptr );

	if ( g_dedicated.integer ) {
		G_Printf( "%s", string + 11 );
	}

	if ( !level.logFile ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), level.logFile );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int				i, numSorted;
	gclient_t		*cl;

	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		G_LogPrintf( "red:%i  blue:%i\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n", 
			cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i],
			cl->pers.netname );
	}
}

/*
=================
CountReadyClients
=================
*/
void CountReadyClients( int *ready, int *notReady, int *readyMask, int *total, int arenaNum ) {
	int			i;
	gclient_t	*cl;

	*ready = 0;
	*notReady = 0;
	*readyMask = 0;
	*total = 0;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		cl = level.clients + i;

		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( ( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) ) {
			continue;
		}

		if ( arenaNum && ( ( cl->ps.persistant[PERS_ARENA] != arenaNum ) ||
		                   ( cl->ps.persistant[PERS_TEAM] != level.arenas[arenaNum].teams[0] &&
		                     cl->ps.persistant[PERS_TEAM] != level.arenas[arenaNum].teams[1] ) ) ) {
			continue;
		}

		if ( cl->readyToExit ) {
			(*ready)++;

			// FIXME broken when g_maxclients >= 16
			if ( i < 16 ) {
				*readyMask |= 1 << i;
			}
		} else {
			(*notReady)++;
		}

		(*total)++;
	}
}

/*
=================
SetClientReadyMasks
=================
*/
void SetClientReadyMasks( int readyMask, int arenaNum ) {
	int			i;
	gclient_t	*cl;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		cl = level.clients + i;

		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( arenaNum && ( cl->ps.persistant[PERS_ARENA] != arenaNum ||
		                   cl->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) ) {
			continue;
		}

		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}
}

/*
=================
ClearClientReady
=================
*/
void ClearClientReady( int arenaNum ) {
	int			i;
	gclient_t	*cl;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		cl = level.clients + i;

		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( arenaNum && cl->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		cl->ps.stats[STAT_CLIENTS_READY] = 0;
		cl->readyToExit = qfalse;
	}
}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready;
	int			notReady;
	int			total;
	int			i;
	gclient_t	*cl;
	int			readyMask;

	// FIXME
	UNUSED(i);
	UNUSED(cl);

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return;
	}

	CountReadyClients( &ready, &notReady, &readyMask, &total, 0 );
	SetClientReadyMasks( readyMask, 0 );

	// no one left, exit immediately
	if ( !total ) {
		ExitLevel();
		return;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	// if noone has readied, start a long timeout
	if ( !ready ) {
		if ( !level.readyToExit ) {
			level.exitTime = level.time + 30000;
			level.readyToExit = qtrue;
		}
	}
	// once someone readies, shorten the timeout
	else if ( !level.readyToExit || level.exitTime > level.time + 10000 ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time + 10000;
	}

	// if everyone is ready, go now
	if ( !notReady ) {
		ExitLevel();
		return;
	}

	if ( level.time < level.exitTime ) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	return qfalse;

	// FIXME unnecessary
	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}
	
	if ( g_gametype.integer >= GT_TEAM ) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void ) {
	int			i;
	gclient_t	*cl;

	// FIXME unused
	UNUSED(cl);

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit();
		return;
	}

	if ( level.intermissionQueued ) {
		if ( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	if ( g_timelimit.integer ) {
		if ( level.time >= level.lastTimeLeftUpdate + 60000 ) {
			level.lastTimeLeftUpdate = level.time;

			trap_Cvar_Set( "g_timeLeft", va( "%d", MAX( ( g_timelimit.integer * 60000 - ( level.time - level.startTime ) ) / 60000, 0 ) ) );
		}
	}

	if ( g_timelimit.integer && !level.warmupTime ) {
		if ( level.time - level.startTime >= g_timelimit.integer * 60000 ) {
			if ( g_gametype.integer != GT_CTF && ScoreIsTied() ) {
				return;
			}

			for ( i = 0; i <= level.lastArena; i++ ) {
				if ( level.arenas[i].settings.type != AT_CLANARENA || level.arenas[i].state == ROUND_INIT ||
				     level.arenas[i].state == ROUND_COUNTDOWN_MATCH ) {
					continue;
				}

				if ( level.arenas[i].settings.competitionmode ) {
					if ( level.time - level.startTime >= g_timelimit.integer * 60000 + 900000 ) {
						break;
					}
				} else if ( level.time - level.startTime >= g_timelimit.integer * 60000 + 300000 ) {
					break;
				}

				// timelimit not yet hit
				return;
			}

			trap_SendServerCommand( -1, "print \"Timelimit hit.\n\"" );
			LogExit( "Timelimit hit." );
		}
	}
}

/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


/*
=============
CheckTournement

Once a frame, check for changes in tournement player state
=============
*/
void CheckTournement( void ) {
	if ( level.numPlayingClients == 0 ) {
		return;
	}

	if ( g_gametype.integer == GT_TOURNAMENT ) {

		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 ) {
			AddTournamentPlayer();
		}

		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	} else if ( g_gametype.integer != GT_SINGLE_PLAYER && g_doWarmup.integer ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;

		if ( g_gametype.integer > GT_TEAM ) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED );

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return; // still waiting for team members
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}


/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	float passVotes;

	if ( !level.voteTime ) {
		return;
	}

	if ( level.time - level.voteTime >= VOTE_TIME ) {
		trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
		level.voteFailTime = level.time;
	} else {
		passVotes = level.numVotingClients * ( level.votePassPercent > 0 ? level.votePassPercent / 100.0f : g_votePercent.integer / 100.0f );

		if ( level.voteYes >= passVotes ) {
			trap_SendServerCommand( -1, "print \"Vote passed.\n\"" );

			if ( strstr( level.voteString, "map_restart" ) == level.voteString ) {
				level.restarting = 1;
				LogExit( "Restarting Map..." );
			} else if ( strstr( level.voteString, "map" ) == level.voteString ) {
				trap_SendConsoleCommand( 2, va( "%s\n", level.voteString ) );
			} else if ( strstr( level.voteString, "kick" ) == level.voteString ||
			            strstr( level.voteString, "clientkick" ) == level.voteString ) {
				trap_SendConsoleCommand( 2, va( "%s\n", level.voteString ) );
			} else if ( strstr( level.voteString, "nextmap" ) == level.voteString ) {
				LogExit( "Moving to next map..." );
			} else {
				trap_SendConsoleCommand( 2, va( "%s\n", level.voteString ) );
			}
		} else if ( level.voteNo >= passVotes ) {
			trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
			level.voteFailTime = level.time;
		} else {
			// still waiting for a majority
			return;
		}
	}

	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );
}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;
	static int blackoutMod = -1;

	if ( g_compmodeBlackout.modificationCount != blackoutMod ) {
		blackoutMod = g_compmodeBlackout.modificationCount;

		if ( g_compmodeBlackout.integer >= 0 && g_compmodeBlackout.integer <= 1 ) {
			trap_SetConfigstring( CS_COMPMODE_BLACKOUT, g_compmodeBlackout.string );
		} else {
			G_Printf( "Invalid value %d for g_compmodeBlackout\n", g_compmodeBlackout.integer );
		}
	}

	if ( g_password.modificationCount != lastMod ) {
		lastMod = g_password.modificationCount;
		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > level.time) {
		return;
	}
	
	ent->nextthink = 0;
	if (!ent->think) {
		G_Error ( "NULL ent->think");
	}
	ent->think (ent);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
	int			msec;
	int			start, end;

	// FIXME unused
	UNUSED(start);
	UNUSED(end);

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	// FIXME unused
	msec = level.time - level.previousTime;

	// get any cvar changes
	G_UpdateCvars();

	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS ) {
			G_RunClient( ent );
			continue;
		}

		G_RunThink( ent );
	}

	// NOW run the missiles, with all players backward-reconciled
	// to the positions they were in exactly 50ms ago, at the end
	// of the last server frame
	G_TimeShiftAllClients( level.previousTime, 0 );

	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
		}
	}

	G_UnTimeShiftAllClients( NULL );

	arena_think_all();

	// perform final fixups on the players
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// for tracking changes
	CheckCvars();

	// record the time at the end of this frame - it should be about
	// the time the next frame begins - when the server starts
	// accepting commands from connected clients
	level.frameStartTime = trap_Milliseconds();
}
