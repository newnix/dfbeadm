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

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#ifndef DFBEADM_FSUP_H
#include "fsupdate.h"
#endif

extern char *__progname;
extern char **environ;
extern bool dbg;
extern bool noop;
/* 
 * TODO: This really should just be "activate()" automatically called by create()
 * Special activation function, for use by the create() chain of functions
 * since we already have all the information necessary to generate and install
 * the fstab data
 */
int
autoactivate(bedata *snapfs, int fscount) {
	/* should probably have an int in there to ensure proper iteration */
	int i, efd, retc;
	char *efstab;

	i = efd = retc = 0;

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with snapfs = %p, fscount = %d\n",
				__progname,__FILE__,__LINE__,__func__,(void *)snapfs,fscount);
	}
	
	if ((efstab = calloc((size_t)512, sizeof(char))) == NULL) { 
		fprintf(stderr,"ERR: %s [%s:%u] %s: Unable to allocate buffer for *efstab!\n",__progname,__FILE__,__LINE__,__func__);
		retc = -1;
	} else {
		/* generate the name of the ephemeral fstab file */
		snprintf(efstab, (size_t)512, "/tmp/.fstab.%u", getpid());

		if ((efd = open(efstab, O_RDWR|O_CREAT|O_NONBLOCK|O_APPEND,S_IRUSR|S_IRGRP|S_IWUSR|S_IROTH)) <= 0) { 
			fprintf(stderr, "ERR: %s [%s:%u] %s: Unable to open %s for writing!\n",__progname,__FILE__,__LINE__,__func__,efstab);
			free(efstab);
			retc = -2;
		} else {
			for (i = 0; i < fscount; i++) {
				dprintf(efd, "%s%s\t%s\t%s\t%s\t%d\t%d\n", snapfs[i].fstab.fs_spec, snapfs[i].snapshot.name, snapfs[i].fstab.fs_file, 
																										 snapfs[i].fstab.fs_vfstype, snapfs[i].fstab.fs_mntops,
																										 snapfs[i].fstab.fs_freq, snapfs[i].fstab.fs_passno);
			}

			/* 
			 * new funciton, specifically dumps current fstab into a file with a timestamp of when it was created
			 * ideally will include the BE label in the future a swell, though that would require some extra parsing
			 */
			fprintf(stdout,"Installing new fstab...\n");
			swapfstab("/etc/fstab", &efd);
			printfs(efstab);
			/* unlink(efstab); Do not unlink, as we can't be sure it's written properly now */
			close(efd);
		}
		free(efstab);
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,retc);
	}
	return(retc);
}

/*
 * activate a given boot environment
 */
int
activate(const char *label) { 
	/* 
	 * the *label parameter is only used for the pfs lookups, by default this is called by create()
	 * we'll scan currently mounted filesystems for the given boot environment label, and use that to 
	 * create the ephemeral fstab, which then replaces the current fstab, putting it in fstab.label
	 * assuming there was a label. If no label exists for the existing fstab, we'll simply call it 
	 * fstab.bak
	 */
	int retc;
	char *efstab;
	FILE *efd;

	retc = 0;

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with label = %s\n",__progname,__FILE__,__LINE__,__func__,label);
	}

	if ((efstab = calloc((size_t)512, sizeof(char))) == NULL) {
		fprintf(stderr,"Error Allocating Ephemeral fstab\n");
	}

	snprintf(efstab, (size_t)512, "/tmp/.fstab.%d.%s", getpid(), label);
	fprintf(stderr, "Created Ephemeral fstab at %s", efstab);

	if ((efd = fopen(efstab, "a")) == NULL) { 
		err(errno, "%s: %s: ", __progname, efstab);
	}

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,retc);
	}
	fclose(efd);
	free(efstab);
	return(retc);
}

/*
 * deactivate the given boot environment
 */
int
deactivate(const char *label) { 
	/* 
	 * I'm not sure what this function will do in particular as the 
	 * naming convention described above is fstab.label, making it trivial to swap boot environments 
	 * This function likely only needs to be called when destroying a boot environment
	 */
	int retc;
	retc = 0;
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with label = %s\n",__progname,__FILE__,__LINE__,__func__,label);
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,retc);
	}
	return(retc);
}

/*
 * delete a given boot environment
 */
int
rmenv(const char *label) { 
	/* TODO: Implement PFS deletion for the given label */
	int retc;
	retc = 0;
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with label = %s\n",__progname,__FILE__,__LINE__,__func__,label);
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,retc);
	}
	return(retc);
}

/*
 * delete the given snapshot
 */
int
rmsnap(const char *pfs) { 
	/* TODO: Implement PFS deletion for the given label */
	int retc;
	retc = 0;
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with pfs = %s\n",__progname,__FILE__,__LINE__,__func__,pfs);
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,retc);
	}
	return(retc);
}

void
printfs(const char *fstab) { 
	struct fstab *fsent;

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with fstab = %s\n",__progname,__FILE__,__LINE__,__func__,fstab);
	}

	fsent = NULL;
	setfstab(fstab);
	while ((fsent = getfsent()) != NULL) {
		fprintf(stdout,"%s\t%s\t%s\t%s\t%s\t%d\t%d\n",
		               fsent->fs_spec, fsent->fs_file, fsent->fs_vfstype,
									 fsent->fs_mntops, fsent->fs_type, fsent->fs_freq,
									 fsent->fs_passno);
	}
	endfsent();

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning to caller\n",__progname,__FILE__,__LINE__,__func__);
	}
}

/* 
 * TODO: Verify proper control logic
 */
int
swapfstab(const char *current, int *newfd) {
	/*
	 * this function will open the old fstab, clean it out after dumpting contents to a new 
	 * backup file, then write the contents of the ephemeral fstab into it
	 */
	int bfd, cfd, retc;
	struct stat curfstab;
	ssize_t written;
	size_t readsize;
	off_t writepoint;
	char tmpbuf[PAGESIZE]; /* work with a page of data at a time, defaulting to 4096 if not otherwise defined */

	readsize = 0;
	written = 0; writepoint = 0;
	bfd = cfd = retc = 0;

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with current = %s, newfd = %d\n",__progname,__FILE__,__LINE__,__func__,current,*newfd);
	}
	/* First ensure the fstab even exists, with no dynamic allocations, we can simply bail early */
	if ((stat(current,&curfstab)) != 0) {
		fprintf(stderr,"ERR: %s [%s:%u] %s: Unable to stat %s (%s)\n",
				__progname,__FILE__,__LINE__,__func__,current,strerror(errno));
		retc = -1;
	}
	/* Detect whether or not we should read in a full PAGE */
	readsize = (curfstab.st_size > PAGESIZE) ? PAGESIZE : curfstab.st_size;
	if ((cfd = open(current, O_RDWR)) <= 0) { 
		fprintf(stderr,"ERR: %s [%s:%u] %s: Unable to open %s r/w, %s\n",__progname,__FILE__,__LINE__,__func__,current,strerror(errno));
		retc = -1;
	}
	if (retc == 0 && (bfd = open("/etc/fstab.bak", O_TRUNC|O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) <= 0) {
		fprintf(stderr, "ERR: %s [%s:%u] %s: /etc/fstab.bak could not be created, verify file and user permissions are set properly!\n",__progname,__FILE__,__LINE__,__func__);
		retc = -2;
	}

	/* read cfd into bfd, close bfd, rewind cfd, dump newfd into it */
	for (;!noop && retc == 0 && written < curfstab.st_size;) {
		pread(cfd, &tmpbuf, readsize, writepoint);
		written += pwrite(bfd, &tmpbuf, readsize, writepoint);
		writepoint = written; /* after writing, record the location to read from in the next iteration */
	}
	writepoint ^= writepoint; written ^= written;
	/* Now we can close bfd, and dump newfd into cfd */
	close(bfd);
	/* stat the efstab, reuising curfstab struct */
	if ((fstat(*newfd,&curfstab)) != 0) {
		fprintf(stderr,"ERR: %s [%s:%u] %s: Unable to stat new fstab fd %d (%s)\n",__progname,__FILE__,__LINE__,__func__,*newfd,strerror(errno));
		retc = -3;
	}
	for (;!noop && written < curfstab.st_size;) {
		pread(*newfd, &tmpbuf, readsize, writepoint);
		written += pwrite(cfd, &tmpbuf, readsize, writepoint);
		writepoint = written;
		if (dbg) {
			fprintf(stderr,"DBG: %s [%s:%u] %s: writepoint = %lu, filesize = %lu\n",__progname,__FILE__,__LINE__,__func__,writepoint,curfstab.st_size);
		}
	}

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,retc);
	}
	close(cfd);
	return(retc);
}
