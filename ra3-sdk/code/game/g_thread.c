#include "g_local.h"

#ifndef Q3_VM

#define MAX_THREADS			1
#define MAX_MUTEXES			4
#define MAX_SHARED_CVARS	3

typedef struct {
	vmCvar_t *cvar;
	cvarTable_t *entry;
} sharedCvar_t;

int threadQuit = 0;
int mutexInitComplete = 0;

pthread_mutex_t *mutexTable[MAX_MUTEXES] = {
	&gPrintMutex,
	&gLogPrintMutex,
	&runningThreadsMutex,
	&threadQuitMutex,
};

int mutexTableSize = MAX_MUTEXES;

sharedCvar_t sharedCvars[MAX_SHARED_CVARS] = {
	{ &net_port, NULL },
	{ &fs_homepath, NULL },
	{ &fs_game, NULL },
};

int sharedCvarsSize = MAX_SHARED_CVARS;

pthread_t threadTable[MAX_THREADS];
int runningThreads[MAX_THREADS];

pthread_mutex_t gPrintMutex;
pthread_mutex_t gLogPrintMutex;
pthread_mutex_t runningThreadsMutex;
pthread_mutex_t threadQuitMutex;

// FIXME unused
int threads_e;

int G_HasMutex( vmCvar_t *cvar ) {
	int i;

	for ( i = 0; i < sharedCvarsSize; i++ ) {
		if ( sharedCvars[i].cvar == cvar ) {
			return 1;
		}
	}

	return 0;
}

cvarTable_t *findTableItemS( vmCvar_t *cvar ) {
	int i;

	for ( i = 0; i < sharedCvarsSize; i++ ) {
		if ( sharedCvars[i].cvar == cvar ) {
			return sharedCvars[i].entry;
		}
	}

	return NULL;
}

cvarTable_t *findTableItemC( vmCvar_t *cvar ) {
	int i;

	for ( i = 0; i < gameCvarTableSize; i++ ) {
		if ( gameCvarTable[i].vmCvar == cvar ) {
			return &gameCvarTable[i];
		}
	}

	return NULL;
}

int G_GetMutex( pthread_mutex_t *m ) {
	if ( !mutexInitComplete ) {
		return 1;
	}

	if ( pthread_mutex_lock( m ) ) {
		return 0;
	}

	return 1;
}

int G_ReleaseMutex( pthread_mutex_t *m ) {
	if ( !mutexInitComplete ) {
		return 1;
	}

	return pthread_mutex_unlock( m ) == 0;
}

int G_GetCvarMutex( vmCvar_t *cvar ) {
	cvarTable_t *entry;

	entry = NULL;

	if ( !mutexInitComplete ) {
		return 1;
	}

	entry = findTableItemS( cvar );

	if ( !entry ) {
		return 0;
	}

	if ( pthread_mutex_lock( &entry->mutex ) ) {
		return 0;
	}

	return 1;
}

int G_ReleaseCvarMutex( vmCvar_t *cvar ) {
	cvarTable_t *entry;

	entry = NULL;

	if ( !mutexInitComplete ) {
		return 1;
	}

	entry = findTableItemS( cvar );

	if ( !entry ) {
		return 0;
	}

	return pthread_mutex_unlock( &entry->mutex ) == 0;
}

void G_InitMutexes() {
	int i;
	cvarTable_t *entry;

	for ( i = 0; i < sharedCvarsSize; i++ ) {
		entry = findTableItemC( sharedCvars[i].cvar );

		if ( !entry ) {
			G_Error( "Could not initialize mutexes!" );
		}

		if ( pthread_mutex_init( &entry->mutex, NULL ) ) {
			G_Error( "Could not create mutex: %s.", entry->cvarName );
		}

		sharedCvars[i].entry = entry;
	}

	for ( i = 0; i < mutexTableSize; i++ ) {
		if ( pthread_mutex_init( mutexTable[i], NULL ) ) {
			G_Error( "Could not create mutex: %d.", i );
		}
	}

	mutexInitComplete = 1;
}

void G_DestroyMutexes() {
	int i;
	cvarTable_t *entry;

	mutexInitComplete = 0;

	for ( i = 0; i < sharedCvarsSize; i++ ) {
		entry = findTableItemS( sharedCvars[i].cvar );

		if ( !entry ) {
			G_Error( "Could not deinitialize mutexes!" );
		}

		pthread_mutex_destroy( &entry->mutex );
		sharedCvars[i].entry = NULL;
	}

	for ( i = 0; i < mutexTableSize; i++ ) {
		pthread_mutex_destroy( mutexTable[i] );
	}
}

void *threadFunc( void *arg ) {
	void ( *cb )();
	char unknown[MAX_STRING_CHARS];

	// FIXME unused
	UNUSED(unknown);

	if ( arg ) {
		cb = arg;
		cb();
		pthread_exit( NULL );
	}

	return NULL;
}

void startThread( void *cb, int id ) {
	// FIXME should be >= MAX_THREADS
	if ( id < 0 || id >= 2 ) {
		return;
	}

	memset( &threadTable[id], 0, sizeof( threadTable[id] ) );

	if ( pthread_create( &threadTable[id], 0, threadFunc, cb ) != 0 ) {
		G_Error( "Failed to create thread!" );
	}

	runningThreads[id] = 1;
}

void G_InitThreads() {
	// FIXME unused
	UNUSED(threads_e);

	memset( &runningThreads, 0, sizeof( runningThreads ) );
	memset( &threadTable, 0, sizeof( threadTable ) );

	if ( g_httpd.integer ) {
		startThread( httpServer, 0 );
	}
}

void G_ExitThreads() {
	int done;
	int i;

	done = 0;

	G_GetMutex( &threadQuitMutex );
	threadQuit = 1;
	G_ReleaseMutex( &threadQuitMutex );

	while ( !done ) {
		G_Printf( "Waiting on threads...\n" );

		done = 1;

		for ( i = 0; i < MAX_THREADS; i++ ) {
			if ( !runningThreads[i] ) {
				continue;
			}

			pthread_join( threadTable[i], NULL );
			runningThreads[i] = 0;
		}

		usleep( 1000000 );
	}
}

#endif
