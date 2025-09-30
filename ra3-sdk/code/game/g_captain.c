#include "g_local.h"

qboolean G_IsTeamOwner( gentity_t *ent );

void Cmd_TeamName_f( gentity_t *ent ) {
	char arg[MAX_TOKEN_CHARS];
	int n;
	int teamNum;
	int arenaNum;

	teamNum = ent->client->ps.persistant[PERS_TEAM];
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( !level.arenas[arenaNum].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"This command is restricted to competition mode.\n\"" );
		return;
	}

	if ( !G_IsTeamOwner( ent ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"You are not the team captain.\n\"" );
		return;
	}

	if ( !level.arenas[arenaNum].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"This command is restricted to competition mode.\n\"" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	n = strlen( arg );

	if ( !n ) {
		trap_SendServerCommand( ent - g_entities,
		                        va( "print \"Current team name: %s.\n\"", level.teams[teamNum].name ) );
	}

	if ( n > (int)sizeof( level.teams[teamNum].name ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"Teamname truncated to 32 characters.\n\"" );
	}

	strncpy( level.teams[teamNum].name, arg, sizeof( level.teams[teamNum].name ) );
}

qboolean G_IsTeamOwner( gentity_t *ent ) {
	int teamNum = ent->client->ps.persistant[PERS_TEAM];

	if ( level.teams[teamNum].captain != ent ) {
		return qfalse;
	}

	return qtrue;
}

void Cmd_TeamMute_f( gentity_t *ent ) {
	int teamNum;
	int arenaNum;

	teamNum = ent->client->ps.persistant[PERS_TEAM];
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( !level.arenas[arenaNum].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"This command is restricted to competition mode.\n\"" );
		return;
	}

	if ( !G_IsTeamOwner( ent ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"You are not the team captain.\n\"" );
		return;
	}

	if ( level.teams[teamNum].muted ) {
		trap_SendServerCommand( ent - g_entities, "print \"Team is already muted.\n\"" );
		return;
	}

	show_string( va( "print \"%s" S_COLOR_WHITE " muted the %s.\n\"",
	                 level.teams[teamNum].captain->client->pers.netname, level.teams[teamNum].name ),
	             arenaNum );

	level.teams[teamNum].muted = 1;
}

void Cmd_TeamUnmute_f( gentity_t *ent ) {
	int teamNum;
	int arenaNum;

	teamNum = ent->client->ps.persistant[PERS_TEAM];
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( !level.arenas[arenaNum].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"This command is restricted to competition mode.\n\"" );
		return;
	}

	if ( !G_IsTeamOwner( ent ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"You are not the team captain.\n\"" );
		return;
	}

	if ( !level.teams[teamNum].muted ) {
		trap_SendServerCommand( ent - g_entities, "print \"Team is already unmuted.\n\"" );
		return;
	}

	show_string( va( "print \"%s" S_COLOR_WHITE " unmuted the %s.\n\"",
	                 level.teams[teamNum].captain->client->pers.netname, level.teams[teamNum].name ),
	             arenaNum );

	level.teams[teamNum].muted = 0;
}

void Cmd_TeamCaptain_f( gentity_t *ent ) {
	char arg[MAX_TOKEN_CHARS];
	int n;
	int teamNum;
	int arenaNum;
	int clientNum;

	teamNum = ent->client->ps.persistant[PERS_TEAM];
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( !level.arenas[arenaNum].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"This command is restricted to competition mode.\n\"" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	n = strlen( arg );

	if ( n ) {
		if ( !G_IsTeamOwner( ent ) ) {
			trap_SendServerCommand( ent - g_entities, "print \"You are not the team captain.\n\"" );
			return;
		}

		clientNum = atoi( arg );

		if ( g_entities[clientNum].inuse && g_entities[clientNum].client &&
		     g_entities[clientNum].client->pers.connected == CON_CONNECTED &&
		     g_entities[clientNum].client->ps.persistant[PERS_TEAM] == ent->client->ps.persistant[PERS_TEAM] ) {
			level.teams[teamNum].captain = &g_entities[clientNum];

			show_string( va( "print \"%s" S_COLOR_WHITE " is now the team captain of the %s.\n\"",
			                 level.teams[teamNum].captain->client->pers.netname, level.teams[teamNum].name ),
			             arenaNum );
		} else {
			trap_SendServerCommand( ent - g_entities, "print \"Invalid client.\n\"" );
		}
	} else {
		if ( level.teams[teamNum].captain && level.teams[teamNum].captain->client ) {
			trap_SendServerCommand( ent - g_entities,
			                        va( "print \"The current team captain is %s" S_COLOR_WHITE ".\n\"",
			                            level.teams[teamNum].captain->client->pers.netname ) );
		} else {
			trap_SendServerCommand( ent - g_entities, "print \"Your team does not have a team captain.\n\"" );
		}
	}
}

void Cmd_TeamKick_f( gentity_t *ent ) {
	char arg[MAX_TOKEN_CHARS];
	int n;
	int clientNum;
	int teamNum;
	int arenaNum;

	teamNum = ent->client->ps.persistant[PERS_TEAM];
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( !level.arenas[arenaNum].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"This command is restricted to competition mode.\n\"" );
		return;
	}

	if ( !G_IsTeamOwner( ent ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"You are not the team captain.\n\"" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	n = strlen( arg );

	if ( !n ) {
		trap_SendServerCommand( ent - g_entities, "print \"Kick by number:\n\"" );

		for ( clientNum = 0; clientNum < level.maxclients; clientNum++ ) {
			if ( !g_entities[clientNum].inuse || !g_entities[clientNum].client ||
			     g_entities[clientNum].client->pers.connected != CON_CONNECTED ||
			     g_entities[clientNum].client->ps.persistant[PERS_TEAM] != ent->client->ps.persistant[PERS_TEAM] ) {
				continue;
			}

			trap_SendServerCommand(
				ent - g_entities, va( "print \"  %d: %s\n\"", clientNum, g_entities[clientNum].client->pers.netname ) );
		}
	} else {
		clientNum = atoi( arg );

		if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
			trap_SendServerCommand( ent - g_entities, "print \"Invalid player.\n\"" );
			return;
		}

		if ( g_entities[clientNum].inuse && g_entities[clientNum].client &&
		     g_entities[clientNum].client->pers.connected == CON_CONNECTED &&
		     g_entities[clientNum].client->ps.persistant[PERS_TEAM] == ent->client->ps.persistant[PERS_TEAM] ) {
			trap_SendServerCommand( clientNum, "print \"You were kicked from the team.\n\"" );

			G_LeaveTeam( &g_entities[clientNum] );
		} else {
			trap_SendServerCommand( ent - g_entities, "print \"Invalid player.\n\"" );
		}
	}
}

void G_ResetTimeouts( int teamNum ) {
	int timeoutCount;
	int timeoutSeconds;

	timeoutCount = -1;
	timeoutSeconds = -1;

	sscanf( g_timeouts.string, "%d:%d", &timeoutCount, &timeoutSeconds );

	level.teams[teamNum].timeoutsRemaining = timeoutCount;
	level.arenas[level.teams[teamNum].arena].timeoutSeconds = timeoutSeconds;
}

void G_NewTeamCaptain( gentity_t *ent ) {
	gentity_t *other;
	int i;
	int teamNum;
	int arenaNum;

	teamNum = ent->client->ps.persistant[PERS_TEAM];
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( level.arenas[arenaNum].settings.competitionmode && G_IsTeamOwner( ent ) ) {
		level.teams[teamNum].captain = NULL;

		for ( i = 0, other = g_entities; i < level.maxclients; i++, other++ ) {
			if ( other->inuse == qfalse ||
			     other->client->pers.connected != CON_CONNECTED ||
			     other->client->ps.persistant[PERS_ARENA] != arenaNum ||
			     other->client->ps.persistant[PERS_TEAM] != teamNum ||
			     other == ent ) {
				continue;
			}

			level.teams[teamNum].captain = other;
			break;
		}

		if ( !level.teams[teamNum].captain ) {
			show_string( va( "print \"Could not find a new captain for the %s.\n\"", level.teams[teamNum].name ),
						 arenaNum );
			level.teams[teamNum].locked = 0;
			G_ResetTimeouts( teamNum );
		} else {
			show_string( va( "print \"%s" S_COLOR_WHITE " is now the team captain of the %s.\n\"",
							 level.teams[teamNum].captain->client->pers.netname, level.teams[teamNum].name ),
						 arenaNum );
		}
	}
}

void Cmd_TeamLock_f( gentity_t *ent ) {
	int teamNum;
	int arenaNum;

	teamNum = ent->client->ps.persistant[PERS_TEAM];
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( !level.arenas[arenaNum].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"This command is restricted to competition mode.\n\"" );
		return;
	}

	if ( G_IsTeamOwner( ent ) ) {
		level.teams[teamNum].locked = 1;

		show_string( va( "print \"%s" S_COLOR_WHITE " has locked the %s.\n\"", ent->client->pers.netname,
		                 level.teams[teamNum].name ),
		             arenaNum );
	} else {
		trap_SendServerCommand( ent - g_entities, "print \"You are not the team captain.\n\"" );
	}
}

void Cmd_TeamUnlock_f( gentity_t *ent ) {
	int teamNum;
	int arenaNum;

	teamNum = ent->client->ps.persistant[PERS_TEAM];
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( !level.arenas[arenaNum].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"This command is restricted to competition mode.\n\"" );
		return;
	}

	if ( G_IsTeamOwner( ent ) ) {
		level.teams[teamNum].locked = 0;

		show_string( va( "print \"%s" S_COLOR_WHITE " has unlocked the %s.\n\"", ent->client->pers.netname,
		                 level.teams[teamNum].name ),
		             arenaNum );
	} else {
		trap_SendServerCommand( ent - g_entities, "print \"You are not the team captain.\n\"" );
	}
}
