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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char *__progname;
extern char **environ;

/* 
 * Connects to the bootenv database, sets the 
 * pointer to NULL on failure, will also signal 
 * via a return code of 1
 */
int
connect_bedb(sqlite3 *dbptr) {
	int retc;
	retc = 0;
	return(retc);
}

/*
 * Initializes the bootenvironment database, should 
 * be done post-installation to set up necessary 
 * tables and construct an index with some basic information
 * It should only be possible for this function to fail
 * if somehow it was called without appropriate permissions 
 * to write to /usr/local/etc/dfbeadm
 */
int
init_bedb(void) {
	int retc;
	retc = 0;
	return(retc);
}

/*
 * Read entries out of the database, for purposes 
 * of listing boot environments or performing integrity checks 
 * such as ensuring that no boot environment is listed in the 
 * database that doesn't still exist on disk.
 */
int
read_bedata(const char *belabal) {
	int retc;
	retc = 0;
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
	return(retc);
}
