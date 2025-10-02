// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#define	MISSILE_PRESTEP_TIME	50

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}

/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	if ( ent->splashDamage ) {
		if ( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent,
							 ent->splashMethodOfDeath ) ) {
			g_entities[ent->r.ownerNum].client->accuracy_hits++;

			if ( ent->s.weapon == WP_ROCKET_LAUNCHER ) {
				G_SetStat( &g_entities[ent->r.ownerNum], TS_RLSPLASH, 1 );
			} else if ( ent->s.weapon == WP_GRENADE_LAUNCHER ) {
				G_SetStat( &g_entities[ent->r.ownerNum], TS_GLSPLASH, 1 );
			}
		}

		switch ( ent->s.weapon ) {
		case WP_ROCKET_LAUNCHER:
			G_UpdateRanks( &g_entities[ent->r.ownerNum], TS_RLSHOT );
			break;
		case WP_GRENADE_LAUNCHER:
			G_UpdateRanks( &g_entities[ent->r.ownerNum], TS_GLSHOT );
			break;
		}

		trap_LinkEntity( ent );
	}
}

/*
================
G_MissileImpact

================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace, vec3_t dir ) {
	gentity_t	*other;
	qboolean	hitClient;

	hitClient = qfalse;
	other = &g_entities[trace->entityNum];

	// spawn clusters of rockets on BFG impact
	if ( ent->s.weapon == WP_BFG ) {
		if ( ent->parent && ent->parent->client ) {
			if ( level.arenas[ent->parent->client->ps.persistant[PERS_ARENA]].settings.excessive ) {
				int			i;
				int			thinkDelay;
				gentity_t	*m;
				vec3_t		n;
				vec3_t		reflect;
				vec3_t		reflectWithNoise;
				vec3_t		mat[3];
				float		spread;

				ent->s.weapon = WP_ROCKET_LAUNCHER;

				VectorCopy( trace->plane.normal, n );

				mat[0][0] = 1.0f - ( n[0] * 2.0f ) * n[0];
				mat[0][1] = n[0] * -2.0f * n[1];
				mat[0][2] = n[0] * -2.0f * n[2];

				mat[1][0] = mat[0][1];
				mat[1][1] = 1.0f - ( n[1] * 2.0f ) * n[1];
				mat[1][2] = n[1] * -2.0f * n[2];

				mat[2][0] = mat[0][2];
				mat[2][1] = mat[1][2];
				mat[2][2] = 1.0f - ( n[2] * 2.0f ) * n[2];

				VectorRotate( dir, mat, reflect );

				spread = DotProduct( reflect, n );
				spread *= 2.0f;

				if ( spread <= 0.0f ) {
					spread = 0.001f;
				}

				for ( i = 0; i < 7; i++ ) {
					thinkDelay = rand() % 300;

					VectorCopy( reflect, reflectWithNoise );
					reflectWithNoise[0] += ( rand() / (float)INT_MAX - 0.5 ) * spread;
					reflectWithNoise[1] += ( rand() / (float)INT_MAX - 0.5 ) * spread;
					reflectWithNoise[2] += ( rand() / (float)INT_MAX - 0.5 ) * spread;
					VectorNormalize( reflectWithNoise );

					m = fire_grenade( ent->parent, trace->endpos, reflectWithNoise, thinkDelay, 0 );

					if ( m->parent->client && m->parent->client->ps.powerups[PW_QUAD] ) {
						m->damage = m->damage * g_quadfactor.value;
						m->splashDamage = m->splashDamage * g_quadfactor.value;
					}

					m->damage = 80;
					m->splashRadius = 80;
					m->splashDamage = 200;
					m->methodOfDeath = MOD_BFG;
					m->splashMethodOfDeath = MOD_BFG;
				}
			}
		}
	}

	// check for bounce
	if ( !other->takedamage &&
		( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		return;
	}

	// impact damage
	if ( other->takedamage && ent->damage ) {
		vec3_t	velocity;

		if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
			g_entities[ent->r.ownerNum].client->accuracy_hits++;

			// record stats
			if ( ent->s.weapon == WP_ROCKET_LAUNCHER || ent->s.weapon == WP_GRENADE_LAUNCHER ) {
				if ( other->client && ent->parent && ent->parent->client ) {
					if ( ent->s.weapon == WP_ROCKET_LAUNCHER ) {
						G_SetStat( ent->parent, TS_RLHIT, 1 );
					} else {
						G_SetStat( ent->parent, TS_GLHIT, 1 );
					}

					if ( other->client->ps.groundEntityNum == ENTITYNUM_NONE ) {
						if ( level.arenas[ent->parent->client->ps.persistant[PERS_ARENA]].state != ROUND_WARMUP ) {
							trace_t	tr;
							vec3_t	end;
							vec3_t	maxs;
							vec3_t	mins;

							VectorSet(maxs, 15.0f, 15.0f, 15.0f);
							VectorSet(mins, -15.0f, -15.0f, -15.0f);

							VectorCopy( other->client->ps.origin, end);
							end[2] -= 4096.0f;

							trap_Trace( &tr, other->client->ps.origin, mins, maxs, end, other - g_entities,
							            MASK_WATER | MASK_DEADSOLID );

							if ( tr.fraction < 1.0 && !tr.startsolid ) {
								float dist1 = Distance( tr.endpos, other->client->ps.origin );
								float dist2 = Distance( ent->parent->client->ps.origin, other->client->ps.origin );

								if ( dist1 >= 150.0f && dist2 >= 150.0f ) {
									ent->parent->client->ps.eFlags &=
										~( EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET |
									       EF_AWARD_HOLYSHIT | EF_AWARD_ACCURACY | EF_AWARD_CAP );

									if ( ent->s.weapon == WP_ROCKET_LAUNCHER ) {
										ent->parent->client->ps.eFlags |= EF_AWARD_ACCURACY;
										ent->parent->client->ps.persistant[PERS_ACCURACY_COUNT]++;
									} else {
										ent->parent->client->ps.eFlags |= EF_AWARD_HOLYSHIT;
										ent->parent->client->ps.persistant[PERS_HOLYSHIT_COUNT]++;
									}

									ent->parent->client->rewardTime = level.time + 2000;
								}
							}
						}
					}
				}
			} else if ( ent->s.weapon == WP_PLASMAGUN ) {
				G_SetStat( &g_entities[ent->r.ownerNum], TS_PGHIT, 1 );
			}

			hitClient = qtrue;
		}

		BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );

		if ( VectorLength( velocity ) == 0 ) {
			velocity[2] = 1; // stepped on a grenade
		}

		G_Damage( other, ent, &g_entities[ent->r.ownerNum], velocity, ent->s.origin, ent->damage, 0,
				  ent->methodOfDeath );
	}

	if (!strcmp(ent->classname, "hook")) {
		gentity_t *nent;
		vec3_t v;

		nent = G_Spawn();

		if ( other->takedamage && other->client ) {

			G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
		} else {
			VectorCopy(trace->endpos, v);
			G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
			ent->enemy = NULL;
		}

		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin( ent, v );
		G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity( ent );
		trap_LinkEntity( nent );

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if ( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, other,
		                     ent->splashMethodOfDeath ) ) {
			if ( !hitClient ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;

				if ( ent->s.weapon == WP_ROCKET_LAUNCHER ) {
					G_SetStat( &g_entities[ent->r.ownerNum], TS_RLSPLASH, 1 );
				} else if ( ent->s.weapon == WP_GRENADE_LAUNCHER ) {
					G_SetStat( &g_entities[ent->r.ownerNum], TS_GLSPLASH, 1 );
				}
			}
		}
	}

	switch ( ent->s.weapon ) {
	case WP_ROCKET_LAUNCHER:
		G_UpdateRanks( &g_entities[ent->r.ownerNum], TS_RLSHOT );
		break;
	case WP_GRENADE_LAUNCHER:
		G_UpdateRanks( &g_entities[ent->r.ownerNum], TS_GLSHOT );
		break;
	case WP_PLASMAGUN:
		G_UpdateRanks( &g_entities[ent->r.ownerNum], TS_PGSHOT );
		break;
	}

	trap_LinkEntity( ent );
}

/*
================
G_RunMissile

================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	vec3_t		dir;

	if ( ent->r.ownerNum >= 0 && ent->r.ownerNum < MAX_CLIENTS &&
	     level.arenas[level.clients[ent->r.ownerNum].ps.persistant[PERS_ARENA]].paused ) {
		ent->nextthink += level.time - level.previousTime;
		ent->s.pos.trTime += level.time - level.previousTime;
		trap_LinkEntity( ent );
		G_RunThink( ent );
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, 
		ent->r.ownerNum, ent->clipmask );

	// save movement direction
	VectorSubtract( origin, ent->r.currentOrigin, dir );
	VectorNormalize( dir );

	// update position based on trace
	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	trap_LinkEntity( ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client->hook == ent)
				ent->parent->client->hook = NULL;
			G_FreeEntity( ent );
			return;
		}

		G_MissileImpact( ent, &tr, dir );
		if ( ent->s.eType != ET_MISSILE ) {
			return;		// exploded
		}
	}

	// check think function after bouncing
	G_RunThink( ent );
}


//=============================================================================

/*
=================
fire_plasma

=================
*/
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->s.otherEntityNum = self->s.number;
	bolt->parent = self;

	if (self->client && level.arenas[self->client->ps.persistant[PERS_ARENA]].settings.excessive) {
		bolt->damage = 60;
		bolt->splashDamage = 60;
		bolt->splashRadius = 30;
	} else {
		bolt->damage = 20;
		bolt->splashDamage = 15;
		bolt->splashRadius = 20;
	}

	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}	

//=============================================================================


/*
=================
fire_grenade
=================
*/
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t dir, int thinkDelay, int bounceHalf) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + thinkDelay;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;

	if ( bounceHalf ) {
		bolt->s.eFlags = EF_BOUNCE_HALF;
	} else {
		bolt->s.eFlags = 0;
	}

	bolt->r.ownerNum = self->s.number;
	bolt->s.otherEntityNum = self->s.number;
	bolt->parent = self;

	if (self->client && level.arenas[self->client->ps.persistant[PERS_ARENA]].settings.excessive) {
		bolt->damage = 1000;
		bolt->splashDamage = 200;
		bolt->splashRadius = 200;
	} else {
		bolt->damage = 100;
		bolt->splashDamage = 100;
		bolt->splashRadius = 150;
	}

	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_bfg
=================
*/
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;
	qboolean	excessive;

	excessive = self->client && level.arenas[self->client->ps.persistant[PERS_ARENA]].settings.excessive;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "bfg";

	if (excessive) {
		bolt->nextthink = level.time + 30000;
	} else {
		bolt->nextthink = level.time + 10000;
	}

	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->s.otherEntityNum = self->s.number;
	bolt->parent = self;

	if (excessive) {
		bolt->damage = 125;
	} else {
		bolt->damage = 100;
		bolt->splashDamage = 100;
		bolt->splashRadius = 120;
	}

	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_rocket
=================
*/
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->s.otherEntityNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 900, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_grapple
=================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*hook;
	int			startTime;

	VectorNormalize (dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;
	hook->s.otherEntityNum = self->s.number;

	if (self->client) {
		startTime = self->client->pers.cmd.serverTime + MISSILE_PRESTEP_TIME;
	} else {
		startTime = level.time - MISSILE_PRESTEP_TIME;
	}

	hook->s.pos.trTime = startTime;
	hook->s.pos.trType = TR_LINEAR;
	hook->s.otherEntityNum = self->s.number; // use to match beam in client
	VectorCopy( start, hook->s.pos.trBase );
	VectorScale( dir, 800, hook->s.pos.trDelta );
	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	self->client->hook = hook;

	return hook;
}



