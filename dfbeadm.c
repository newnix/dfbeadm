/*
 * Copyright (c) 2017-2018, Exile Heavy Industries
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

/* 
 * This is meant to be a utility similar to beadm(1) on FreeBSD/Illumos
 * however, I'd rather have it written in C than have a massive shell script.
 * So this should be an interesting exercise in getting familiar with hammer2's
 * snapshot feature and managing important system files. 
 *
 * Provided this project actually bears any results, DragonFly BSD development 
 * can proceed even faster as both vkernels and boot environments would 
 * make testing new builds almost completely painless
 */

#ifdef _LINUX
/* needed for strlcpy/strlcat */
#include <bsd/string.h>
#endif
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* holds prototypes */
#ifndef DFBEADM_MAIN_H
#include "dfbeadm.h"
#endif
/* collect the filesystems into a struct buffer */
#ifndef DFBEADM_FSCOLLECT_H
#include "fscollect.h"
#endif
/* functions to destroy boot environments */
#ifndef DFBEADM_FSDESTROY_H
#include "fsdestroy.h"
#endif
/* functions to get the available environments */
#ifndef DFBEADM_DF_LIST_H
#include "fslist.h"
#endif
/* functions to test filesystems */
#ifndef DFBEADM_H2TEST_H
#include "fstest.h"
#endif
/* generate and install the new fstab */
#ifndef DFBEADM_FSUP_H
#include "fsupdate.h"
#endif
/* make the snapshots */
#ifndef DFBEADM_SNAPFS_H
#include "snapfs.h"
#endif

#define NOOPMASK 0x04
#define CREATEBE 0x08
#define ACTIVATE 0x10

extern char *__progname;
extern char **environ;
extern bool dbg;
extern bool noop;
/*
 * TODO: Rework and possibly expand for more robust option parsing
 * ----------------------
 *  exflags layout
 * ----------------------
 * 0 0 0 0 0 0 0 0
 * | | | | | | | |
 * | | | | | | | \- verbosity flag
 * | | | | | | \- verbosity flag
 * | | | | | \- dry run flag
 * | | | | \- create
 * | | | \- activate
 * | | \- delete 
 * | \- reserved
 * \- reserved
 */

static void usage(void);
/* This is where the actual logic processing should take place */
int cook(uint8_t *flags, char *bestring);

bool dbg = false; /* Default to not adding runtime traces */
bool noop = false;

/* 
 * TODO: Remove all but the most rudimentary logic from this function, instead 
 * pass the important data down to cook(), which will then determine how best to proceed
 */
int 
main(int argc, char **argv) { 
	/* a bitmap flag value to pass to other functions */
	uint8_t exflags; 
	int ch, ret;
	char belabel[MNAMELEN];

	exflags = 0;
	ret = ch = 0;

	/* bail early */
	if ( argc == 1 ) { usage(); }
	while((ch = getopt(argc,argv,"a:c:d:hlnrD")) != -1) { 
		switch(ch) { 
			case 'a': 
				exflags |= ACTIVATE;
				strlcpy(belabel,optarg,(MNAMELEN-1));
				break;
			case 'c':
				exflags |= CREATEBE;
				strlcpy(belabel,optarg,(MNAMELEN-1));
				break;
			/*
			 * TODO: Add a config file to read, so users can specify which filesystems they actually want managed
			 * use either '-f' or '-C'
			 */
			case 'd':
				deactivate(optarg);
				break;
			case 'D':
				dbg = true;
				break;
			case 'h':
				usage();
			case 'l':
				list();
				break;
			case 'n':
				/* 
				 * This is the NO-OP flag, it will mostly be useful during the debugging 
				 * process, but also will allow users to get additional information regarding what
				 * could happen, especially with increased verbosity
				 */
				exflags |= NOOPMASK;
				noop = true;
				break;
			default:
				usage();
		}
	}
	/* Pass all the serious logic into cook() */
	argc -= optind;
	argv += optind;
	ret = cook(&exflags, belabel);
	return(ret);
}

int
cook(uint8_t *flags, char *bestring) {
	int retc;

	/* Placeholder logic to quelch compiler warnings */
	if ((flags != NULL) && (bestring != NULL)) {
		retc = 0;
	} else {
		retc = 1;
	}
	switch(*flags) {
		case(ACTIVATE):
			retc = activate(bestring);
			break;
		case(ACTIVATE|NOOPMASK):
			retc = activate(bestring);
			break;
		case(CREATEBE|NOOPMASK):
			retc = create(bestring);
			break;
		default:
			usage();
			break;
	}

	return(retc);
}

/*
 * tell the user how this program works
 */
static void __attribute__((noreturn))
usage(void) { 
	fprintf(stderr,"%s: Utility to create HAMMER2 boot environments.\n",__progname);
	fprintf(stderr,"Usage:\n"
	               "  -a  Activate the given boot environment\n"
	               "  -c  Create a new boot environment with the given label\n"
	               "  -d  Destroy the given boot environment\n"
								 "  -D  Print debugging information during execution\n"
	               "  -h  This help text\n"
	               "  -l  List existing boot environments\n"
								 "  -n  No-op/dry run, only show what would be done\n"
	               "  -r  Remove the given boot environment\n");
	_exit(0);
}
