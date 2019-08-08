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

#ifndef DFBEADM_DF_LIST_H
#include "fslist.h"
#endif

extern char **environ;
extern char *__progname;
extern bool dbg;

/*
 * list the available boot environments
 * returns the number of boot environments found,
 * negative return values indicate errors
 */
int
list(void) { 
	int rootfd, found;
	struct hammer2_ioc_pfs h2be;

	found = rootfd = 0;

	if (geteuid() != 0) {
		fprintf(stderr,"%s [%s:%u] %s: You must be root or run with sudo/doas for this tool to work! (Current EUID: %u)\n",
				__progname,__FILE__,__LINE__,__func__,geteuid());
	}
	if ((rootfd = open("/", O_RDONLY|O_NONBLOCK)) < 0) { 
		fprintf(stderr, "%s [%s:%u] %s: Unable to open \"/\"!\n%s\n", __progname,__FILE__,__LINE__,__func__,strerror(errno));
		return(-3);
	}
	memset(&h2be, 0, sizeof(h2be));

	/*
	 * this method works, but will repeat multiple entries with the same label if there's more
	 * than one PFS on the root partition
	 */
	for (; h2be.name_key != (hammer2_key_t)-1; h2be.name_key = h2be.name_next) { 
		if (ioctl(rootfd, HAMMER2IOC_PFS_GET, &h2be) < 0) {
			fprintf(stderr, "Unable to get any pfs data from /, is it a HAMMER2 FS?\n");
			return(-3);
		}
		/* scan for hammer2 pfs subtype of HAMMER2_PFSSUBTYPE_SNAPSHOT, as defined in hammer2_disk.h */
		if (h2be.pfs_subtype == HAMMER2_PFSSUBTYPE_SNAPSHOT) {
			fprintf(stdout,"%s\n",h2be.name);
			found++;
		}
	}
	fprintf(stdout,"%s: Found a total of %d managed snapshots for the PFS mounted on \'/\'\n",__progname,found);
	/* Enforce return code of 0 */
	found = (found != 0) ? found ^ found : found;
	close(rootfd);
	return(found);
}
