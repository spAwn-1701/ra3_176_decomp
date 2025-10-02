#include "g_local.h"

#define ACCURACY_INT(x) ((int)(x))
#define ACCURACY_FRC(x) (int)(((x) - (int)(x)) * 10.0f)

void G_SendEorSingle( gentity_t *ent );
void GetWeaponStats( gentity_t *ent, int statShot, int statHit, int *shot, int *hit, float *acc );

#ifndef Q3_VM

void G_WriteSession( gclient_t *client ) {
	char buf[MAX_TOKEN_CHARS];
	char tmp[128];
	int stats[TS_NUM_STATS];
	int stat;
	int i;
	char *err;
	int minutes;

	err = NULL;

	if ( !client ) {
		return;
	}

	if ( !client->pers.dbId ) {
		return;
	}

	minutes = ( level.time - client->pers.enterTime ) / 60000;

	if ( minutes < g_statsThreshold.integer ) {
		return;
	}

	memset( buf, 0, sizeof( buf ) );
	memset( stats, 0, sizeof( stats ) );

	for ( i = 0; i < TS_NUM_PERIODS; i++ ) {
		for ( stat = 0; stat < TS_NUM_STATS; stat++ ) {
			stats[stat] += client->pers.topStats[i][stat];
		}
	}

	sprintf( buf, "%d", stats[0] );
	i = strlen( buf );

	for ( stat = 1; stat < TS_NUM_STATS; stat++ ) {
		int n = 0;

		memset( tmp, 0, sizeof( tmp ) );
		sprintf( tmp, ",%d", stats[stat] );
		n = strlen( tmp );

		if ( i + n > sizeof( buf ) ) {
			break;
		}

		memcpy( buf + i, tmp, n );
		i += n;
	}

	if ( sqlite_exec_printf( level.db, "INSERT INTO %s VALUES(%d, %d, %d, '%s')", NULL, NULL, &err, tblNames[TBL_SESSIONS], client->pers.dbId,
	                         (int)time( 0 ), minutes, buf ) ) {
		G_LogPrintf( "<DB> Query error: %s\n", err );
	}
}

#endif

void WPToTS( int weapon, int *stats ) {
	/* FIXME weird to check twice */
	if ( !stats || !stats ) {
		return;
	}

	switch ( weapon ) {
	case WP_MACHINEGUN:
		stats[0] = TS_MGHIT;
		stats[1] = TS_MGSHOT;
		break;
	case WP_SHOTGUN:
		stats[0] = TS_SGHIT;
		stats[1] = TS_SGSHOT;
		break;
	case WP_GRENADE_LAUNCHER:
		stats[0] = TS_GLHIT;
		stats[1] = TS_GLSHOT;
		break;
	case WP_ROCKET_LAUNCHER:
		stats[0] = TS_RLHIT;
		stats[1] = TS_RLSHOT;
		break;
	case WP_LIGHTNING:
		stats[0] = TS_LGHIT;
		stats[1] = TS_LGSHOT;
		break;
	case WP_RAILGUN:
		stats[0] = TS_RGHIT;
		stats[1] = TS_RGSHOT;
		break;
	case WP_PLASMAGUN:
		stats[0] = TS_PGHIT;
		stats[1] = TS_PGSHOT;
		break;
	default:
		stats[0] = -1;
		stats[1] = -1;
		break;
	}
}

void TSToWPTS( int stat, int *stats, int *weapon ) {
	if ( !stats || !weapon ) {
		return;
	}

	switch ( stat ) {
	case TS_MGSHOT:
	case TS_MGHIT:
		weapon[0] = WP_MACHINEGUN;
		stats[0] = TS_MGHIT;
		stats[1] = TS_MGSHOT;
		break;
	case TS_SGSHOT:
	case TS_SGHIT:
		weapon[0] = WP_SHOTGUN;
		stats[0] = TS_SGHIT;
		stats[1] = TS_SGSHOT;
		break;
	case TS_GLHIT:
	case TS_GLSHOT:
		weapon[0] = WP_GRENADE_LAUNCHER;
		stats[0] = TS_GLHIT;
		stats[1] = TS_GLSHOT;
		break;
	case TS_RLSHOT:
	case TS_RLHIT:
		weapon[0] = WP_ROCKET_LAUNCHER;
		stats[0] = TS_RLHIT;
		stats[1] = TS_RLSHOT;
		break;
	case TS_LGSHOT:
	case TS_LGHIT:
		weapon[0] = WP_LIGHTNING;
		stats[0] = TS_LGHIT;
		stats[1] = TS_LGSHOT;
		break;
	case TS_RGSHOT:
	case TS_RGHIT:
		weapon[0] = WP_RAILGUN;
		stats[0] = TS_RGHIT;
		stats[1] = TS_RGSHOT;
		break;
	case TS_PGSHOT:
	case TS_PGHIT:
		weapon[0] = WP_PLASMAGUN;
		stats[0] = TS_PGHIT;
		stats[1] = TS_PGSHOT;
		break;
	default:
		weapon[0] = -1;
		stats[0] = -1;
		stats[1] = -1;
		break;
	}
}

void G_UpdateRanks( gentity_t *ent, int stat ) {
	int		weapon;
	int		stats[2];
	int		shot;
	int		hit;
	int		unknown;
	float	accuracy;

	// FIXME unused
	UNUSED(unknown);

	if ( !ent || !ent->client || level.arenas[ent->client->ps.persistant[PERS_ARENA]].state == ROUND_WARMUP ) {
		return;
	}

	TSToWPTS( stat, stats, &weapon );

	if ( weapon >= 0 ) {
		GetWeaponStats( ent, stats[1], stats[0], &shot, &hit, &accuracy );

		// don't calculate shots until a threshold amount has been fired
		if ( shot < 10 || ( stats[1] == TS_SGSHOT && shot < 110 ) ) {
			return;
		}

		// ent is already inserted, adjust its position
		if ( ent->client->pers.weaponRank[weapon] >= 0 ) {
			qboolean	swapped;
			int			rank;
			int			lowerRank;
			int			higherRank;

			swapped = qfalse;
			rank = ent->client->pers.weaponRank[weapon];
			lowerRank = MIN( rank + 1, MAX_CLIENTS );
			higherRank = MAX( rank - 1, 0 );

			if ( level.topShots[weapon][higherRank] && higherRank != rank ) {
				int		higherShot;
				int		higherHit;
				float	higherAccuracy;

				GetWeaponStats( level.topShots[weapon][higherRank], stats[1], stats[0], &higherShot, &higherHit,
								&higherAccuracy );

				if ( accuracy > higherAccuracy ) {
					gentity_t	*higherEnt;

					higherEnt = level.topShots[weapon][higherRank];

					level.topShots[weapon][higherRank] = ent;
					ent->client->pers.weaponRank[weapon] = higherRank;

					level.topShots[weapon][rank] = higherEnt;
					higherEnt->client->pers.weaponRank[weapon] = rank;

					swapped = qtrue;
				}
			}

			if ( !swapped ) {
				if ( level.topShots[weapon][lowerRank] && lowerRank != rank ) {
					int		lowerShot;
					int		lowerHit;
					float	lowerAccuracy;

					GetWeaponStats( level.topShots[weapon][lowerRank], stats[1], stats[0], &lowerShot, &lowerHit,
									&lowerAccuracy );

					if ( accuracy < lowerAccuracy ) {
						gentity_t	*lowerEnt;

						lowerEnt = level.topShots[weapon][lowerRank];

						level.topShots[weapon][lowerRank] = ent;
						ent->client->pers.weaponRank[weapon] = lowerRank;

						level.topShots[weapon][rank] = lowerEnt;
						lowerEnt->client->pers.weaponRank[weapon] = rank;

						swapped = qtrue;
					}
				}
			}
		}
		// insert ent into sorted topShots array
		else {
			int		i;
			int		otherShot;
			int		otherHit;
			float	otherAccuracy;

			for ( i = 0; i < MAX_CLIENTS; i++ ) {
				// insert ent at the end
				if ( !level.topShots[weapon][i] ) {
					level.topShots[weapon][i] = ent;
					ent->client->pers.weaponRank[weapon] = i;
					return;
				}

				GetWeaponStats( level.topShots[weapon][i], stats[1], stats[0], &otherShot, &otherHit, &otherAccuracy );

				// insert ent here
				if ( accuracy >= otherAccuracy ) {
					int j;

					for ( j = MAX_CLIENTS - 1; j > i; j-- ) {
						if ( level.topShots[weapon][j - 1] ) {
							level.topShots[weapon][j] = level.topShots[weapon][j - 1];

							if ( level.topShots[weapon][j]->client ) {
								level.topShots[weapon][j]->client->pers.weaponRank[weapon] = j;
							}
						}
					}

					level.topShots[weapon][i] = ent;
					ent->client->pers.weaponRank[weapon] = i;

					break;
				}
			}
		}
	}
}

void G_SetStat( gentity_t *ent, int stat, int value ) {
	if ( !ent || !ent->client || level.arenas[ent->client->ps.persistant[PERS_ARENA]].state == ROUND_WARMUP ) {
		return;
	}

	ent->client->pers.topStats[TS_ROUND][stat] += value;
}

void G_SendEorSingleGiven( gentity_t *ent ) {
	int i;
	int len;
	int n;
	int attacks;
	int entries;
	int skipped;
	char buf[MAX_TOKEN_CHARS];
	char tmp[MAX_TOKEN_CHARS];

	len = 0;
	attacks = 0;
	entries = 0;
	skipped = 0;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( ent->client->pers.combatStats[i].damageGiven > 0 ) {
			if ( attacks < 6 ) {
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					Com_sprintf( tmp, sizeof( tmp ), " %d %d %d", i,
								 ent->client->pers.combatStats[i].damageGiven,
								 ent->client->pers.combatStats[i].meansOfDeath );
				} else {
					Com_sprintf( tmp, sizeof( tmp ), " %d %d %d", -1,
								 ent->client->pers.combatStats[i].damageGiven,
								 ent->client->pers.combatStats[i].meansOfDeath );
				}

				n = strlen( tmp );

				if ( n + len > (int)sizeof( buf ) ) {
					break;
				}

				strcpy( buf + len, tmp );
				len += n;

				entries++;
			} else {
				skipped += ent->client->pers.combatStats[i].damageGiven;
			}

			attacks++;
		}
	}

	if ( entries > 0 ) {
		trap_SendServerCommand( ent - g_entities, va( "eors_g %d%s %d", entries, buf, skipped ) );
	}
}

void G_SendEorSingleTaken( gentity_t *ent ) {
	int i;
	int len;
	int n;
	int attacks;
	int entries;
	int skipped;
	char buf[MAX_TOKEN_CHARS];
	char tmp[MAX_TOKEN_CHARS];

	len = 0;
	attacks = 0;
	entries = 0;
	skipped = 0;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( ent->client->pers.combatStats[i].damageTaken > 0 ) {
			if ( attacks < 6 ) {
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					Com_sprintf( tmp, sizeof( tmp ), " %d %d %d", i,
								 ent->client->pers.combatStats[i].damageTaken,
								 ent->client->pers.combatStats[i].killedMe );
				} else {
					Com_sprintf( tmp, sizeof( tmp ), " %d %d %d", -1,
								 ent->client->pers.combatStats[i].damageTaken,
								 ent->client->pers.combatStats[i].killedMe );
				}

				n = strlen( tmp );

				if ( n + len > (int)sizeof( buf ) ) {
					break;
				}

				strcpy( buf + len, tmp );
				len += n;

				entries++;
			} else {
				skipped += ent->client->pers.combatStats[i].damageTaken;
			}

			attacks++;
		}
	}

	if ( entries > 0 ) {
		trap_SendServerCommand( ent - g_entities, va( "eors_t %d%s %d", entries, buf, skipped ) );
	}
}

void send_end_of_round_stats( int arenaNum ) {
	int i;
	int unknown;
	gentity_t *ent;

	// FIXME unused
	unknown = 0;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse || ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_ARENA] == arenaNum && !ent->client->pers.killerStats.sent ) {
			G_SendEorSingle( ent );
		}
	}

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse || ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_ARENA] == arenaNum ) {
			ent->client->pers.killerStats.sent = 0;
			ent->client->pers.killerStats.send = 0;

			memset( &ent->client->pers.lastDamage, -1, sizeof( ent->client->pers.lastDamage ) );
		}
	}
}

void G_SendEorSingle( gentity_t *ent ) {
	G_SendEorSingleTaken( ent );
	G_SendEorSingleGiven( ent );

	if ( ent->client->pers.killerStats.send ) {
		trap_SendServerCommand( ent - g_entities,
		                        va( "killer_stats %d %d %d %d %d", ent->client->pers.killerStats.clientNum, ent->client->pers.killerStats.health,
		                            ent->client->pers.killerStats.armor, ent->client->pers.combatStats[ent->client->pers.killerStats.clientNum].damageTaken,
		                            ent->client->pers.killerStats.mod ) );
	} else {
		trap_SendServerCommand( ent - g_entities, "killer_stats -1" );
	}

	ent->client->pers.killerStats.sent = 1;
}

void G_SendStatsupdate( gentity_t *ent, int unknown ) {
	char buf[MAX_TOKEN_CHARS];
	char tmp[MAX_TOKEN_CHARS];
	int len;
	int i;
	int j;
	int n;
	int total;
	gentity_t *other;

	len = 0;

	if ( unknown == 1 ) {
		other = ent;
	} else {
		other = &g_entities[ent->client->ps.clientNum];
	}

	other->client->pers.topStats[TS_WARMUP][TS_AWARDS1] = 0;
	other->client->pers.topStats[TS_WARMUP][TS_AWARDS1] |= ( (byte)other->client->ps.persistant[PERS_ACCURACY_COUNT] ) & 0xFF;
	other->client->pers.topStats[TS_WARMUP][TS_AWARDS1] |= ( (byte)other->client->ps.persistant[PERS_IMPRESSIVE_COUNT] << 8 ) & 0xFF00;
	other->client->pers.topStats[TS_WARMUP][TS_AWARDS1] |= ( (byte)other->client->ps.persistant[PERS_EXCELLENT_COUNT] << 16 ) & 0xFF0000;
	other->client->pers.topStats[TS_WARMUP][TS_AWARDS1] |= ( (byte)other->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT] << 24 ) & 0xFF000000;

	other->client->pers.topStats[TS_WARMUP][TS_AWARDS2] = 0;
	other->client->pers.topStats[TS_WARMUP][TS_AWARDS2] = ( (byte)other->client->ps.persistant[PERS_HOLYSHIT_COUNT] ) & 0xFF;

	{
		for ( i = 0; i < TS_NUM_STATS; i++ ) {
			total = 0;

			if ( i != TS_AWARDS1 && i != TS_AWARDS2 ) {
				for ( j = 0; j < TS_NUM_PERIODS; j++ ) {
					total += other->client->pers.topStats[j][i];
				}
			} else {
				total = other->client->pers.topStats[TS_WARMUP][i];
			}

			Com_sprintf( tmp, sizeof( tmp ), " %i", total );
			n = strlen( tmp );

			if ( len + n > (int)sizeof( buf ) ) {
				break;
			}

			memcpy( buf + len, tmp, n );
			len += n;
		}

		buf[len] = 0;
	}

	trap_SendServerCommand( ent - g_entities, va( "upd_stats_full %d%s", other - g_entities, buf ) );

	{
		memset( buf, 0, sizeof( buf ) );
		len = 0;

		for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
			Com_sprintf( tmp, sizeof( tmp ), " %i", other->client->pers.weaponRank[i] );
			n = strlen( tmp );

			if ( len + n > (int)sizeof( buf ) ) {
				break;
			}

			memcpy( buf + len, tmp, n );
			len += n;
		}

		buf[len] = 0;
	}

	trap_SendServerCommand( ent - g_entities, va( "upd_stats_rank%s", buf ) );
}

void GetWeaponStats( gentity_t *ent, int statShot, int statHit, int *shot, int *hit, float *accuracy ) {
	int i;

	for ( i = 0, *shot = 0; i < TS_NUM_PERIODS; i++ ) {
		*shot += ent->client->pers.topStats[i][statShot];
	}

	for ( i = 0, *hit = 0; i < TS_NUM_PERIODS; i++ ) {
		*hit += ent->client->pers.topStats[i][statHit];
	}

	if ( statHit == TS_RLHIT ) {
		for ( i = 0; i < TS_NUM_PERIODS; i++ ) {
			*hit += ent->client->pers.topStats[i][TS_RLSPLASH];
		}
	}

	if ( statHit == TS_GLHIT ) {
		for ( i = 0; i < TS_NUM_PERIODS; i++ ) {
			*hit += ent->client->pers.topStats[i][TS_GLSPLASH];
		}
	}

	if ( *shot > 0 ) {
		*accuracy = (float)*hit / (float)*shot * 100.0f;
	} else {
		*accuracy = 0;
	}

	if ( *accuracy < 0 ) {
		*accuracy = 0;
	}
}

float GetWeaponAcc( gentity_t *ent, int statShot, int statHit ) {
	int shot;
	int hit;
	float accuracy;

	GetWeaponStats( ent, statShot, statHit, &shot, &hit, &accuracy );

	return accuracy;
}

void TopShotsWp( gentity_t *ent, int weapon, int unknown ) {
	static int align = 1;

	char buf[MAX_TOKEN_CHARS];
	char tmp[128];
	int i;
	int len;
	int stats[2];

	// FIXME unused
	UNUSED(align);

	len = 0;

	if ( !ent ) {
		return;
	}

	memset( buf, 0, sizeof( buf ) );
	WPToTS( weapon, stats );

	if ( stats[0] < 0 || stats[1] < 0 ) {
		return;
	}

	if ( unknown != 1 ) {
		sprintf( buf, S_COLOR_YELLOW "NR Name                     Accuracy    Shots/Hits\n" );
		len = strlen( buf );
	}

	if ( level.topShots[weapon][0] ) {
		for ( i = 0; unknown > 0 ? i < unknown : i < MAX_CLIENTS; i++ ) {
			int n;
			int shot;
			int hit;
			float acc;

			n = 0;

			// end of list
			if ( !level.topShots[weapon][i] ) {
				break;
			}

			GetWeaponStats( level.topShots[weapon][i], stats[1], stats[0], &shot, &hit, &acc );

			memset( tmp, 0, sizeof( tmp ) );

			if ( unknown == 1 ) {
				sprintf( tmp, S_COLOR_GREEN "%-2.2s " S_COLOR_WHITE "%-18.18s         " S_COLOR_GREEN "%3d.%1d" S_COLOR_WHITE "\x01     " S_COLOR_GREEN "%4d" S_COLOR_WHITE "/" S_COLOR_GREEN "%-4d\n",
				         WeapShort( weapon - 2 ), Q_StaticClean( level.topShots[weapon][i]->client->pers.netname ), ACCURACY_INT(acc), ACCURACY_FRC(acc), shot, hit );
			} else {
				sprintf( tmp, S_COLOR_GREEN "%2d "    S_COLOR_WHITE "%-18.18s         " S_COLOR_GREEN "%3d.%1d" S_COLOR_WHITE "\x01     " S_COLOR_GREEN "%4d" S_COLOR_WHITE "/" S_COLOR_GREEN "%-4d\n",
				         level.topShots[weapon][i] - g_entities + 1, Q_StaticClean( level.topShots[weapon][i]->client->pers.netname ), ACCURACY_INT(acc), ACCURACY_FRC(acc), shot, hit );
			}

			n = strlen( tmp );

			if ( len + n > sizeof( buf ) ) {
				break;
			}

			memcpy( buf + len, tmp, n );
			len += n;
		}

		trap_SendServerCommand( ent - g_entities, va( "stprint \"%s\"", buf ) );
	} else {
		if ( unknown == 1 ) {
			trap_SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_GREEN "%-2.2s " S_COLOR_WHITE "No rankings registered.\n\"", WeapShort( weapon - 2 ) ) );
		} else {
			trap_SendServerCommand( ent - g_entities, "print \"No rankings registered for this weapon.\n\"" );
		}
	}
}

void Cmd_TopShots_f( gentity_t *ent ) {
	char arg[MAX_TOKEN_CHARS];

	trap_Argv( 1, arg, sizeof( arg ) );

	if ( !arg[0] ) {
		trap_SendServerCommand( ent - g_entities, va( "stprint \"%s\"", "^3WP Name                     Accuracy    Shots/Hits\n" ) );
		TopShotsWp( ent, WP_MACHINEGUN, 1 );
		TopShotsWp( ent, WP_SHOTGUN, 1 );
		TopShotsWp( ent, WP_GRENADE_LAUNCHER, 1 );
		TopShotsWp( ent, WP_ROCKET_LAUNCHER, 1 );
		TopShotsWp( ent, WP_LIGHTNING, 1 );
		TopShotsWp( ent, WP_RAILGUN, 1 );
		TopShotsWp( ent, WP_PLASMAGUN, 1 );
	} else if ( !Q_stricmp( arg, "MG" ) ) {
		TopShotsWp( ent, WP_MACHINEGUN, -1 );
	} else if ( !Q_stricmp( arg, "SG" ) ) {
		TopShotsWp( ent, WP_SHOTGUN, -1 );
	} else if ( !Q_stricmp( arg, "GL" ) ) {
		TopShotsWp( ent, WP_GRENADE_LAUNCHER, -1 );
	} else if ( !Q_stricmp( arg, "RL" ) ) {
		TopShotsWp( ent, WP_ROCKET_LAUNCHER, -1 );
	} else if ( !Q_stricmp( arg, "LG" ) ) {
		TopShotsWp( ent, WP_LIGHTNING, -1 );
	} else if ( !Q_stricmp( arg, "RG" ) ) {
		TopShotsWp( ent, WP_RAILGUN, -1 );
	} else if ( !Q_stricmp( arg, "PG" ) ) {
		TopShotsWp( ent, WP_PLASMAGUN, -1 );
	} else {
		trap_SendServerCommand( ent - g_entities, "print \"Error: Invalid weapon abbreviation given. Syntax example: /topshots RG\n\"" );
	}
}

void Cmd_AccStats_f( gentity_t *ent ) {
#define FORMAT_WP_ARGS(w) \
	level.topShots[w][0] == other ? S_COLOR_RED : other == ent ? S_COLOR_CYAN : S_COLOR_GREEN, \
	ACCURACY_INT(acc[w - 2]), \
	ACCURACY_FRC(acc[w - 2])

	char buf[MAX_TOKEN_CHARS];
	int i;
	gentity_t *other;
	int arenaNum;
	float acc[7];

	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	Com_sprintf( buf, 100, "^3ID Name                     ^3MG     SG     GL     RL     LG     RG     PG\n" );
	trap_SendServerCommand( ent - g_entities, va( "stprint \"%s\"", buf ) );

	for ( i = 0, other = g_entities; i < level.maxclients; i++, other++ ) {
		if ( other->inuse == qfalse ||
		     other->client->pers.connected != CON_CONNECTED ||
		     other->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		acc[0] = GetWeaponAcc( other, TS_MGSHOT, TS_MGHIT );
		acc[1] = GetWeaponAcc( other, TS_SGSHOT, TS_SGHIT );
		acc[2] = GetWeaponAcc( other, TS_GLSHOT, TS_GLHIT );
		acc[3] = GetWeaponAcc( other, TS_RLSHOT, TS_RLHIT );
		acc[4] = GetWeaponAcc( other, TS_LGSHOT, TS_LGHIT );
		acc[5] = GetWeaponAcc( other, TS_RGSHOT, TS_RGHIT );
		acc[6] = GetWeaponAcc( other, TS_PGSHOT, TS_PGHIT );

		Com_sprintf( buf, sizeof( buf ),
		             other == ent ? S_COLOR_CYAN  "%2d " S_COLOR_CYAN  "%-18.18s   %s%3d.%1d" S_COLOR_CYAN  "\x01 %s%3d.%1d" S_COLOR_CYAN  "\x01 %s%3d.%1d" S_COLOR_CYAN  "\x01 %s%3d.%1d" S_COLOR_CYAN  "\x01 %s%3d.%1d" S_COLOR_CYAN  "\x01 %s%3d.%1d" S_COLOR_CYAN  "\x01 %s%3d.%1d" S_COLOR_CYAN  "\x01\n" S_COLOR_WHITE :
		                            S_COLOR_GREEN "%2d " S_COLOR_WHITE "%-18.18s   %s%3d.%1d" S_COLOR_WHITE "\x01 %s%3d.%1d" S_COLOR_WHITE "\x01 %s%3d.%1d" S_COLOR_WHITE "\x01 %s%3d.%1d" S_COLOR_WHITE "\x01 %s%3d.%1d" S_COLOR_WHITE "\x01 %s%3d.%1d" S_COLOR_WHITE "\x01 %s%3d.%1d" S_COLOR_WHITE "\x01\n" S_COLOR_WHITE,
		             i,
		             Q_StaticClean( other->client->pers.netname ),
		             FORMAT_WP_ARGS(WP_MACHINEGUN),
		             FORMAT_WP_ARGS(WP_SHOTGUN),
		             FORMAT_WP_ARGS(WP_GRENADE_LAUNCHER),
		             FORMAT_WP_ARGS(WP_ROCKET_LAUNCHER),
		             FORMAT_WP_ARGS(WP_LIGHTNING),
		             FORMAT_WP_ARGS(WP_RAILGUN),
		             FORMAT_WP_ARGS(WP_PLASMAGUN) );

		trap_SendServerCommand( ent - g_entities, va( "stprint \"%s\"", buf ) );
	}

#undef FORMAT_WP_ARGS
}

void Cmd_StatsAll_f( gentity_t *ent, gentity_t *target ) {
#define FORMAT_WP_ARGS(w) \
	level.topShots[w][0] == target ? S_COLOR_RED : S_COLOR_GREEN, \
	ACCURACY_INT(acc[w - 2]), \
	ACCURACY_FRC(acc[w - 2])

	char buf[MAX_TOKEN_CHARS];
	float kdr;
	int kills;
	int deaths;
	int i;
	int shot[7];
	int hit[7];
	float acc[7];

	kills = 0;
	deaths = 0;

	for ( i = 0; i < TS_NUM_PERIODS; i++ ) {
		deaths += target->client->pers.topStats[i][TS_DEATHS];
		kills += target->client->pers.topStats[i][TS_KILLS];
	}

	if ( deaths > 0 ) {
		kdr = kills / ( (float)kills + deaths );
	} else if ( kills > 0 ) {
		kdr = 1.0f;
	} else {
		kdr = 0.0f;
	}

	Com_sprintf( buf, sizeof( buf ), S_COLOR_WHITE "Complete stats for: %s" S_COLOR_WHITE "\n%s",
	             target->client->pers.netname, "            ^3MG       SG       GL       RL       LG       RG       PG\n" );

	GetWeaponStats( target, TS_MGSHOT, TS_MGHIT, &shot[0], &hit[0], &acc[0] );
	GetWeaponStats( target, TS_SGSHOT, TS_SGHIT, &shot[1], &hit[1], &acc[1] );
	GetWeaponStats( target, TS_GLSHOT, TS_GLHIT, &shot[2], &hit[2], &acc[2] );
	GetWeaponStats( target, TS_RLSHOT, TS_RLHIT, &shot[3], &hit[3], &acc[3] );
	GetWeaponStats( target, TS_LGSHOT, TS_LGHIT, &shot[4], &hit[4], &acc[4] );
	GetWeaponStats( target, TS_RGSHOT, TS_RGHIT, &shot[5], &hit[5], &acc[5] );
	GetWeaponStats( target, TS_PGSHOT, TS_PGHIT, &shot[6], &hit[6], &acc[6] );

	Com_sprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
	             S_COLOR_YELLOW "Shots" S_COLOR_GREEN "     %4d     %4d     %4d     %4d     %4d     %4d     %4d\n"
	             S_COLOR_YELLOW "Hits" S_COLOR_GREEN "      %4d     %4d     %4d     %4d     %4d     %4d     %4d\n"
	             S_COLOR_YELLOW "Acc     %s%3d.%1d" S_COLOR_WHITE
	                              "\x1   %s%3d.%1d" S_COLOR_WHITE
	                              "\x1   %s%3d.%1d" S_COLOR_WHITE
	                              "\x1   %s%3d.%1d" S_COLOR_WHITE
	                              "\x1   %s%3d.%1d" S_COLOR_WHITE
	                              "\x1   %s%3d.%1d" S_COLOR_WHITE
	                              "\x1   %s%3d.%1d" S_COLOR_WHITE "\x1\n",
	             shot[0], shot[1], shot[2], shot[3], shot[4], shot[5], shot[6],
	             hit[0], hit[1], hit[2], hit[3], hit[4], hit[5], hit[6],
	             FORMAT_WP_ARGS(WP_MACHINEGUN),
	             FORMAT_WP_ARGS(WP_SHOTGUN),
	             FORMAT_WP_ARGS(WP_GRENADE_LAUNCHER),
	             FORMAT_WP_ARGS(WP_ROCKET_LAUNCHER),
	             FORMAT_WP_ARGS(WP_LIGHTNING),
	             FORMAT_WP_ARGS(WP_RAILGUN),
	             FORMAT_WP_ARGS(WP_PLASMAGUN) );

	Com_sprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
	             "\n"
	                    S_COLOR_YELLOW "Kills   Deaths   Efficiency\n"
	                "  " S_COLOR_GREEN "%3d      %3d         %1.2f\n"
	                    S_COLOR_YELLOW "Accuracy Impressive Excellent Gauntlet Holyshit\n"
	             "     " S_COLOR_GREEN "%3d        %3d       %3d      %3d      %3d\n",
	             kills, deaths, kdr,
	             target->client->ps.persistant[PERS_ACCURACY_COUNT],
	             target->client->ps.persistant[PERS_IMPRESSIVE_COUNT],
	             target->client->ps.persistant[PERS_EXCELLENT_COUNT],
	             target->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
	             target->client->ps.persistant[PERS_HOLYSHIT_COUNT] );

	trap_SendServerCommand( ent - g_entities, va( "stprint \"%s\n\"", buf ) );

#undef FORMAT_WP_ARGS
}

void Cmd_Stats_f( gentity_t *ent, int hasArg1 ) {
	char arg[MAX_TOKEN_CHARS];
	int clientNum;

	if ( hasArg1 ) {
		trap_Argv( 1, arg, sizeof( arg ) );
	}

	if ( !arg[0] || !hasArg1 ) {
		Cmd_AccStats_f( ent );
		return;
	}

	clientNum = atoi( arg );

	if ( clientNum < MAX_CLIENTS && clientNum >= 0 && g_entities[clientNum].inuse && g_entities[clientNum].client->pers.connected == CON_CONNECTED ) {
		Cmd_StatsAll_f( ent, &g_entities[clientNum] );
	} else {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid client number.\n\"" );
	}
}
