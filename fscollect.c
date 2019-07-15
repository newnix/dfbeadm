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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#ifndef DFBEADM_FSCOLLECT_H
#include "fscollect.h"
#endif
#ifndef DFBADM_H2TEST_H
#include "fstest.h"
#endif
#ifndef DFBEADM_SNAPFS_H
#include "snapfs.h"
#endif

extern bool dbg;

/* 
 * create a boot environment
 * returns 0 if successful, 1 if error, >=2 if things have gone horribly wrong
 */
int
create(const char *label) { 
	/* since we can't rely on the VFS layer for all of our fstab data, we need to be sure what exists */
	int i, fstabcount, retc, vfscount;
	struct fstab *fsptr;
	struct statfs *vfsptr;
	bedata *befs;
	
	i = retc = fstabcount = vfscount = 0;
	fsptr = NULL;
	vfsptr = NULL;

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entered with label = %s\n",__progname,__FILE__,__LINE__,__func__,label);
	}
	if ((vfscount = getfsstat(vfsptr, 0, MNT_WAIT)) == 0) { 
		fprintf(stderr, "ERR: %s [%s:%u] %s: Something's wrong, no filesystems found\n",__progname,__FILE__,__LINE__,__func__);
	}
	/* Simple loop to get filesystem count from /etc/fstab */
	while ((fsptr = getfsent()) != NULL) { 
		fstabcount++;
	}
	/* ensure that if we start reading the system fstab again, we start from the top */
	endfsent();

	/* since there's bound to be a swap partition, decrement fstabcount by one */
	if (fstabcount != vfscount) {
		fprintf(stderr, "Filesystem counts differ! May have unintended side-effects!\n"
		                "fstab count: %d\nvfs count: %d\n",fstabcount, vfscount);
	} else {
		fprintf(stdout, "INF: %s [%s:%u] %s: VFS Layer and FSTAB(5) are in agreement, generating list of boot environment targets...\n",__progname,__FILE__,__LINE__,__func__);
	}

	/* now that we have an idea what we're working with, let's go about cloning this data */
	if ((befs = calloc((size_t)fstabcount, sizeof(bedata))) == NULL) {
		fprintf(stderr,"Could not allocate initial buffer!\n");
		retc = 2;
	}

	/* now allocate space for the members */
	for (i = 0; i < fstabcount; i++) {
		if ((befs[i].fstab.fs_spec = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "%s [%s:%u] %s: Could not allocate buffer\n",__progname,__FILE__,__LINE__,__func__);
			free(befs);
			return(2);
		}
		if ((befs[i].fstab.fs_file = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "%s [%s:%u] %s: Could not allocate buffer\n",__progname,__FILE__,__LINE__,__func__);
			free(befs);
			return(2);
		}
		if ((befs[i].fstab.fs_vfstype = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "%s [%s:%u] %s: Could not allocate buffer\n",__progname,__FILE__,__LINE__,__func__);
			free(befs);
			return(2);
		}
		if ((befs[i].fstab.fs_mntops = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "%s [%s:%u] %s: Could not allocate buffer\n",__progname,__FILE__,__LINE__,__func__);
			free(befs);
			return(2);
		}
		if ((befs[i].fstab.fs_type = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "%s [%s:%u] %s: Could not allocate buffer\n",__progname,__FILE__,__LINE__,__func__);
			free(befs);
			return(2);
		}
	}

	/* now pass the buffers to the next step */
	mktargets(befs, fstabcount, label);

	/* ensure we clean up after ourselves */
	for (i ^= i; i < fstabcount; i++) { 
		free(befs[i].fstab.fs_spec);
		free(befs[i].fstab.fs_file);
		free(befs[i].fstab.fs_vfstype);
		free(befs[i].fstab.fs_mntops);
		free(befs[i].fstab.fs_type);
	}
	free(befs);
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,retc);
	}
	return(retc);
}

/* 
 * TODO: Have this function call openfs() prior to exit. 
 * It'll cause a slightly deeper call stack than I'd like, 
 * but fits better logically.
 * Creates a buffer of targets to be handed off to snapfs()
 * This function should be called directly from create(), and provided
 * with a buffer of currently existing filesystems
 */
void
mktargets(bedata *target, int fscount, const char *label) {
	int i;
	struct fstab *current;

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entered with target = %p, fscount = %d, label = %s\n",
				__progname,__FILE__,__LINE__,__func__,(void *)target,fscount,label);
	}

	/* 
	 * now that we have the number of existing filesystems and the 
	 * bedata struct to fill in, we can go about updating the information
	 * this is still prior to actually generating the ephemeral fstab though 
	 * we're just building the struct.
	 */
	for (i = 0; i < fscount; i++) { 
		current = getfsent();

		/* these steps are done for every filesystem */
		strlcpy(target[i].fstab.fs_spec, current->fs_spec, MNAMELEN);
		strlcpy(target[i].fstab.fs_file, current->fs_file, MNAMELEN);
		strlcpy(target[i].fstab.fs_vfstype, current->fs_vfstype, MNAMELEN);
		strlcpy(target[i].fstab.fs_mntops, current->fs_mntops, MNAMELEN);
		strlcpy(target[i].fstab.fs_type, current->fs_type, MNAMELEN);
		target[i].fstab.fs_freq = current->fs_freq;
		target[i].fstab.fs_passno = current->fs_passno;

		/* now do some additional work in the same loop */
		if (ish2(current->fs_file)) { 
			target[i].snap = true;
			openfs(target[i].fstab.fs_file,&target[i].mountfd);
			if (relabel(&target[i], label) != 0) { 
				fprintf(stderr,"ERR: %s [%s:%u] %s: Unable to write label %s to %s!\n",__progname,__FILE__,__LINE__,__func__,label,target[i].fstab.fs_file);
			}
		} else {
			target[i].snap = false;
		}
	}
	/* 
	 * now everything should be in place to create snapshots 
	 * looping is handled internally
	 */
	snapfs(target, fscount);
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning to caller\n",__progname,__FILE__,__LINE__,__func__);
	}
}

/* 
 * return 0 if relabeling is done successfully
 * nonzero indicates error, no meaning assigned to 
 * return values yet
 */
int
relabel(bedata *fs, const char *label) {
	char *found, fsbuf[NAME_MAX];
	int i, retc;

	i = retc = 0;
	found = NULL;
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with fs = %p, label = %s\n",__progname,__FILE__,__LINE__,__func__,(void *)fs,label);
	}
	/* 
	 * XXX: this function, along with ish2(), will almost certainly need significant rewriting
	 * as the current mean sof detecting a NULLFS mount vs a HAMMER2 mount are 
	 * ill-defined at this point. It's definitely possible to fail silently, but that shouldn't be 
	 * necessary. 
	 */

	/* simply check for the existence of a boot environment */
	if ((found = strchr(fs->fstab.fs_spec, BESEP)) == NULL) {
		fprintf(stderr,"INF: %s [%s:%u] %s: No existing boot environment found for %s\n",
				__progname,__FILE__,__LINE__,__func__,fs->fstab.fs_spec);
		retc = -1; /* TODO: Signifies no existing bootenv snapshots, need to add handling for this case */
	} else { 
		for (i ^= i; *found != 0 && i < NAME_MAX; found++) { 
			fs->curlabel[i] = *found; /* copy the found label one character at a time into fs->curlabel */
			i++;
			/* XXX: once this is found, we need to clear out all data beyond the first instance of BESEP */
		} 
		if (dbg) { 
			fprintf(stderr,"DBG: %s [%s:%u] %s: %d iterations to copy fs->curlabel=(%s)\n",__progname,__FILE__,__LINE__,__func__,i,fs->curlabel);
		}
		/* see if the label is too long to fit in the allocated space */
		if ((NAME_MAX - 1)< ((unsigned int)i + strlen(label))) {
			fprintf(stderr,"ERR: %s [%s:%u] %s: Given name of %s is too long!\n", __progname,__FILE__,__LINE__,__func__,label);
		} else {
			found -= i;
			clearBElabel(found);
			/* write the new file spec into *fsbuf */
			snprintf(fsbuf, (NAME_MAX -1), "%s%c%s",fs->fstab.fs_spec,BESEP,label);
			if (dbg) {
				fprintf(stderr,"DBG: %s [%s:%u] %s: Generated new label of (fsbuf)=%s from (fs->fstab.fs_spec)=%s%s\n",__progname,__FILE__,__LINE__,__func__,fsbuf,fs->fstab.fs_spec,fs->curlabel);
			}
			/* Seems like a good idea, but then snapfs.c:xtractLabel() would need to be updated to handle this properly */
			//memset(fs->fstab.fs_spec,0,(size_t)NAME_MAX); /* clear out the current fstab block device entry prior to being passed to snapfs */
		}
		fs->curlabel[i] = 0; /* ensure NULL termination */
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n", __progname,__FILE__,__LINE__,__func__,retc);
	}
	return(retc);
}

/* 
 * XXX: Ensure that this is properly migrated from snapfs.c
 */
int
openfs(const char *mountpoint, int *fsfd) {
	int retc;
	retc = 0;
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with mountpoint = %s\n",__progname,__FILE__,__LINE__,__func__,mountpoint);
	}
	if ((retc = open(mountpoint,O_RDONLY)) > 0) {
		*fsfd = retc;
		retc ^= retc;
	} else {
		fprintf(stderr,"ERR: %s [%s:%u] %s: Error opening %s: %s\n",__progname,__FILE__,__LINE__,__func__,mountpoint,strerror(errno));
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Opened fd %d for %s, returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,*fsfd,mountpoint,retc);
	}
	return(retc);
}

/* 
 * XXX: Maybe not needed, 0's out the name of 
 * an existing boot environment label
 */
int
clearBElabel(char *label) {
	int retc;
	retc = 0;
	for (;*label != 0; label++) {
		*label ^= *label;
	}
	return(retc);
}
