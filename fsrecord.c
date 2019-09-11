/*
 * Copyright (c) 2018, Exile Heavy Industries
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its contributors may be used
 *   to endorse or promote products derived from this software without specific
 *   prior written permission.
 * 
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
 * LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef DFBEADM_RECORD_H
#include "fsrecord.h"
#endif
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char *__progname;
extern char **environ;
extern bool dbg;
extern bool noop;

/* 
 * Connects to the bootenv database, sets the 
 * pointer to NULL on failure, will also signal 
 * via a return code of 1
 */
int
connect_bedb(sqlite3 *dbptr) {
	int retc;
	retc = 0;

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with dbptr = %p\n", __progname, __FILE__, __LINE__, __func__, (void *)dbptr);
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller with dbptr = %p\n", __progname, __FILE__, __LINE__, __func__, retc, (void *)dbptr);
	}
	return(retc);
}

/*
 * TODO: Refactor to simplify control flow
 * Initializes the bootenvironment database, should 
 * be done post-installation to set up necessary 
 * tables and construct an index with some basic information
 * It should only be possible for this function to fail
 * if somehow it was called without appropriate permissions 
 * to write to /usr/local/etc/dfbeadm
 */
int
init_bedb(void) {
	int retc, sqlfd;
	/* sqlbuf is for the mmap(2)'d file, sqlbuf_max is to mark the end of the file */
	char recdb_path[DFBEADM_DB_PATHLEN], *sqlbuf, *sqlbuf_max;
	const char *sqltail; /* Points to the next part of the file to be compiled */
	uid_t cuid;
	mode_t cfgdir_mode;
	struct stat cfgstat;
	sqlite3 *recdb;
	sqlite_stmt *recq;

	retc = sqlfd = 0;
	/* Set config directory to 01755 */
	cfgdir_mode = S_ISVTX|S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH|S_IXOTH|S_IRGRP|S_IXGRP;
	/* Ensure pointers are initialized as useless */
	recdb = NULL; sqlbuf = NULL; sqlbuf_max = NULL; sqltail = NULL; recq = NULL;
	/* This should never fail */
	snprintf(recdb_path,DFBEADM_DB_PATHLEN,"%s/%s",DFBEADM_CONFIG_DIR, DFBEADM_RECORD_DB);

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Initializing database at %s/%s\n", __progname, __FILE__, __LINE__, __func__, DFBEADM_CONFIG_DIR, DFBEADM_RECORD_DB);
	}

	/* 
	 * Exit early if we have the wrong EUID 
	 */
	if ((cuid = geteuid()) != 0) {
		fprintf(stderr,"ERR: %s [%s:%u] %s: Only root can bootstrap the database!\n", __progname, __FILE__, __LINE__, __func__);
		retc = -1;
	} else {
		/* Now that we know we're operating with the appropriate permissions, we can check that the config dir exists */
		if ((retc = stat(DFBEADM_CONFIG_DIR, &cfgstat)) != 0) {
			/* Create the directory, should only fail if /usr/local/etc doesn't exist or is mounted read-only */
			if ((retc = mkdir(DFBEADM_CONFIG_DIR, cfgdir_mode)) != 0) {
				fprintf(stderr,"ERR: %s [%s:%u] %s: %s! Unable to create %s, bailing out\n", 
						__progname, __FILE__, __LINE__, __func__, strerror(errno), DFBEADM_CONFIG_DIR);
				return(retc);
			}
		}
		if ((retc = stat(recdb_path, &cfgstat)) != 0) {
			if ((retc = sqlite3_open_v2(recdb_path, &recdb, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL)) != SQLITE_OK) {
				fprintf(stderr,"ERR: %s [%s:%u] %s: Unable to create record database at %s!\n",
						__progname, __FILE__, __LINE__, __func__, recdb_path);
			}
			/* 
			 * Create the database as specified in dfbeadm.sql, assumed to be in the same working directory 
			 * TODO: Consider allowing a command-line override, also maybe don't use a string literal
			 */
			if (((sqlfd = open("dfbeadm.sql", O_RDONLY|O_EXCL|O_EXLOCK)) <= 0)  && ((retc = stat("dfbeadm.sql",&cfgstat)) != 0)) {
				fprintf(stderr,"ERR: %s [%s:%u] %s: Unable to open %s for reading!\n",
						__progname, __FILE__, __LINE__, __func__, "dfbeadm.sql");
			}
			/* Now attempt to mmap the file */
			if ((sqlbuf = mmap(NULL, (size_t)cfgstat.st_size, PROT_READ|PROT_NONE, MAP_NOCORE|MAP_NOSYNC|MAP_PRIVATE, sqlfd, (off_t)0)) == NULL) {
				/* Managed to not map the file somehow, as the SQL file is only slightly larger than a quarter of a page */
				fprintf(stderr,"ERR: %s [%s:%u] %s: Unable to map %s at fd %d (%ld bytes)!\n", __progname, __FILE__, __LINE__, __func__,
						"dfbeadm.sql", sqlfd, cfgstat.st_size);
			}
			sqlbuf_max = &sqlbuf[cfgstat.st_size]; /* Set sqlbuf_max to the end of the file */

			/* XXX; I really hate this, needs to be split up in a future revision */
			for (; sqlbuf < sqlbuf_max; sqlbuf = (char * const)sqltail) {
				retc = sqlite3_prepare_v2(recdb, sqlbuf, (-1), &recq, &sqltail);
				if (retc == SQLITE_OK) {
					retc = sqlite3_step(recq);
					/* TODO: handle these conditions better than just continuing to progress through the loop
					if (retc != SQLITE_DONE) {
						fprintf(stderr,"ERR: %s [%s:%u] %s: %s\n", __progname, __FILE__, __LINE__, __func__, sqlite3_errstr(retc));
					} else {
						sqlite3_finalize(recq);
					}
				} else {
					fprintf(stderr, "ERR: %s [%s:%u] %s: %s\n", __prognamee, __FILE__, __LINE__, __func__, sqlite3_errstr(retc));
				}
			}
			/* 
			 * Database should now be in a usable state, close connection and clean up
			 * TODO: Should probably reset pointers to NULL, just to ensure no dangling pointers are left beyond the intialization stage
			 */
			sqlite3_close(recdb);
			munmap(sqlbuf);
			close(sqlfd);
		} else {
			/* If the database file exists, attempt to validate its contents */
			if (dbg) {
				fprintf(stderr,"INF: %s [%s:%u] %s: Database already exists at %s! Running validation checks...\n",
						__progname, __FILE__, __LINE__, __func__, recdb_path);
			}
			retc = testdb(recdb_path);
		}
	}

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n", __progname, __FILE__, __LINE__, __func__, retc);
	}
	return(retc);
}

/*
 * Read entries out of the database, for purposes 
 * of listing boot environments or performing integrity checks 
 * such as ensuring that no boot environment is listed in the 
 * database that doesn't still exist on disk.
 */
int
read_bedata(const char *belabel) {
	int retc;
	retc = 0;
	
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with belabel = %s\n", __progname, __FILE__, __LINE__, __func__, belabel);
	}

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n", __progname, __FILE__, __LINE__, __func__, retc);
	}
	return(retc);
}

/* 
 * Write entries to the database, such as when 
 * creating a new boot environment, the 
 * buffer holding all the bedata structures is 
 * passed in and converted to a table entry.
 */
int
write_bedata(bedata *bootenv) {
	int retc;
	retc = 0;
	if (dbg) { 
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with bedata at %p\n", __progname, __FILE__, __LINE__, __func__, (void *)bootenv);
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n", __progname, __FILE__, __LINE__, __func__, retc);
	}
	return(retc);
}

/* 
 * Delete an entry from the database, 
 * either when explicitly deleting a boot
 * environment or when pruning entries 
 * that no longer exist.
 */
int
drop_bootenv(const char *belabel) {
	int retc;
	retc = 0;
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with belabel = %s\n", __progname, __FILE__, __LINE__, __func__, belabel);
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n", __progname, __FILE__, __LINE__, __func__, retc);
	}
	return(retc);
}
