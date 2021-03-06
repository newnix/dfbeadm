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

#define DFBEADM_MAIN_H
/* Currently encoded like RGB, but Major, Minor, Patch */
#define DFBEADM_VER 0x000200
#define DFBEADM_VER_STRING "0.2.0-DEV"
#define PFSDELIM '@'
#define BESEP ':'
#define TMAX 18
#define NOTIMP(a) fprintf(stderr,"WRN: %s [%s:%u] %s: -%c is not implemented at this time!\n",__progname,__FILE__,__LINE__,__func__,a)

/* Asserts are a good thing to have across all files */
#include <assert.h>
#include <stdbool.h>
/* necessary inclusions for vfs layer data */
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/ucred.h>

/* for fstab manipulation/verification */
#include <fstab.h>

/* HAMMER2 specific needs */
#include <vfs/hammer2/hammer2_ioctl.h>

/* struct to hold the relevant data to rebuild the fstab */
struct bootenv_data { 
	struct fstab fstab; /* this should be pretty obvious, but this is each PFS's description in the fstab */
	struct hammer2_ioc_pfs snapshot; /* this is the PFS we'll be creating a snapshot with */
	char curlabel[NAME_MAX]; /* this may actually not be necessary, bubt it's the current label of the PFS */
	int mountfd;
	bool snap;
} __packed;

struct efstab_lookup {
	char mounutpoint[1024];
	struct fstab fsent;
};

typedef struct bootenv_data bedata;

extern char *__progname;
