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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
			fprintf(stderr, "Could not allocate buffer\n");
			free(befs);
			return(2);
		}
		if ((befs[i].fstab.fs_file = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "Could not allocate buffer\n");
			free(befs);
			return(2);
		}
		if ((befs[i].fstab.fs_vfstype = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "Could not allocate buffer\n");
			free(befs);
			return(2);
		}
		if ((befs[i].fstab.fs_mntops = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "Could not allocate buffer\n");
			free(befs);
			return(2);
		}
		if ((befs[i].fstab.fs_type = calloc(MNAMELEN, 1)) == NULL) {
			fprintf(stderr, "Could not allocate buffer\n");
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
 * Creates a buffer of targets to be handed off to snapfs()
 * This function should be called directly from create(), and provided
 * with a buffer of currently existing filesystems
 */
void
mktargets(bedata *target, int fscount, const char *label) {
	int i;
	struct fstab *current;

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
			if (relabel(&target[i], label) != 0) { 
				fprintf(stderr,"%s [%s:%u] %s: Unable to write label %s to %s!\n",__progname,__FILE__,__LINE__,__func__,label,target[i].fstab.fs_file);
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
}

/* 
 * return 0 if relabeling is done successfully
 * nonzero indicates error, no meaning assigned to 
 * return values yet
 */
int
relabel (bedata *fs, const char *label) {
	char *found;
	int i;

	i = 0;
	found = NULL;
	/* 
	 * this function, along with ish2(), will almost certainly need significant rewriting
	 * as the current mean sof detecting a NULLFS mount vs a HAMMER2 mount are 
	 * ill-defined at this point. It's definitely possible to fail silently, but that shouldn't be 
	 * necessary. 
	 */

	/* simply check for the existence of a boot environment */
	if ((found = strchr(fs->fstab.fs_spec, PFSDELIM)) == NULL) {
		fprintf(stderr, "%s [%s:%u] %s: Are you certain %s mounted %s is a HAMMER2 filesystem?\n", 
				__progname, __FILE__, __LINE__, __func__, fs->fstab.fs_spec, fs->fstab.fs_file);
		/* this can throw false positives if there's no existing snapshot/PFS mountpoints */
		return(-1);
	} else { 
	/* this is incorrect */
		for (i ^= i; *found != 0 && i < NAME_MAX; found++) { 
			fs->curlabel[i] = *found;
			i++;
		} 
		if ((NAME_MAX - 1)< ((unsigned int)i + strlen(label))) {
			fprintf(stderr,"Given name of %s is too long!\n", label);
		} else {
			snprintf(fs->fstab.fs_spec, (NAME_MAX - 1), "%s%c%s", fs->fstab.fs_spec, BESEP, label);
		}
		fs->curlabel[i] = 0; /* ensure NULL termination */
//		fprintf(stderr, "fs->curlabel: %s\n", fs->curlabel);
//		fprintf(stderr, "BEFS: %s\n", fs->fstab.fs_spec);
	}

	return(0);
}

