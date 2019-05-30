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

#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <vfs/hammer2/hammer2_ioctl.h>
#include <unistd.h>

#ifndef DFBEADM_H2TEST_H
#include "fstest.h"
#endif

extern bool dbg;

/*
 * Determine if the given mountpoint is a HAMMER2 filesystem 
 */
bool
ish2(const char *mountpoint) {
	int mp;
	hammer2_ioc_inode_t h2ino;

	/* 
	 * additional possible utility is the version info
	 * HAMMER2IOC_VERSION_GET, that wouuld return a 
	 * hammer2_ioc_version_t version.version integer
	 * if successful
	 */
	if ((mp = open(mountpoint, O_RDONLY)) == 0) {
		return(false);
	}
	if (ioctl(mp, HAMMER2IOC_INODE_GET, &h2ino) < 0) {
		close(mp);
		return(false);
	} else {
		close(mp);
		return(true);
	}
}

/*
 * Cut down a string to fit in the boot environment limitations
 */
void
fstrunc(char *longstring) { 
	snprintf(longstring, NAME_MAX, "%s", longstring);
}

