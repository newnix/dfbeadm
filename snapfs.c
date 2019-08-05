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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef DFBEADM_SNAPFS_H
#include "snapfs.h"
#endif
#ifndef DFBEADM_FSUP_H
#include "fsupdate.h"
#endif

extern char **environ;
extern char *__progname;
extern bool dbg;
extern bool noop;

/* XXX: This function likely doing too much work */
/* 
 * This should ideall be a filesystem agnostic function
 * to create a snapshot with the given label
 * Honestly, with the current bedata definitions, fscount and label may not be necessary parameters
 */
int
snapfs(bedata *fstarget, int fscount) { 
	/* 
	 * TODO: Turn this function into an abstraction for any CoW filesystem, selected at compile-time
	 * then if I'm able, possibly even available at runtime should multiple CoW filesystems be available
	 * though this could be expanded to any filesystem with the same functionality of snapshots. Possibly 
	 * including both HAMMER and UFS in later versions
	 */
	register int i;
	int retc;

	i = retc = 0;

	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Entering with fstarget = %p, fscount = %d\n",__progname,__FILE__,__LINE__,__func__,(void *)fstarget,fscount);
	}
	/* XXX: Testing fstab installation prior to snapshot creation */
	autoactivate(fstarget, fscount);
	for (i ^= i; i < fscount; i++) {
		/* We use the following ioctl() to actually create a snapshot */
		if (fstarget[i].snap && !noop) {
			if (ioctl(fstarget[i].mountfd, HAMMER2IOC_PFS_SNAPSHOT, &fstarget[i].snapshot) != -1) {
				fprintf(stdout, "INF: %s [%s:%u] %s: Created new snapshot: %s\n",__progname,__FILE__,__LINE__,__func__,fstarget[i].snapshot.name);
			} else {
				fprintf(stderr, "ERR: %s [%s:%u] %s: H2 Snap failed!\n%s\n(target: %s)\n",__progname,__FILE__,__LINE__,__func__,strerror(errno), fstarget[i].snapshot.name);
			}
		} else {
			if (noop) {
				fprintf(stdout, "DBG: %s [%s:%u] %s: Skipping creation of %s for %s\n",__progname,__FILE__,__LINE__,__func__,fstarget[i].snapshot.name,fstarget[i].fstab.fs_file);
				strlcat(fstarget[i].fstab.fs_spec,fstarget[i].snapshot.name,NAME_MAX);
			} else { 
				fprintf(stdout, "INF: %s [%s:%u] %s: Skipping %s as it is not HAMMER2\n",__progname,__FILE__,__LINE__,__func__,fstarget[i].fstab.fs_file);
			}
		}
	}
	/* Now go through and ensure we close all the file descriptors since the snapshots have been created */
	for (i ^= i; i < fscount; i++) {
		if (fstarget[i].mountfd != 0) {
			if (dbg) {
				fprintf(stderr,"DBG: %s [%s:%u] %s: Closing fd %d for %s\n",
						__progname,__FILE__,__LINE__,__func__,fstarget[i].mountfd,fstarget[i].fstab.fs_file);
			}
			close(fstarget[i].mountfd);
		}
	}
	if (dbg) {
		fprintf(stderr,"DBG: %s [%s:%u] %s: Returning %d to caller\n",__progname,__FILE__,__LINE__,__func__,retc);
	}
	return(retc);
}
