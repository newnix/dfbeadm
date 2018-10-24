#define FSUP_H
#ifndef PAGESIZE
#define PAGESIZE 4096
#endif
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

/* 
 * Special activation function, for use by the create() chain of functions
 * since we already have all the information necessary to generate and install
 * the fstab data
 */
static int
autoactivate(bedata *snapfs, int fscount, const char *label) {
	/* should probably have an int in there to ensure proper iteration */
	int i, efd;
	char *efstab;
	
	return(0); /* temporarily disable the activation, as that seems to cause issues */
	
	if ((efstab = calloc((size_t)512, sizeof(char))) == NULL) { 
		fprintf(stderr,"Unable to allocate buffer for *efstab!\n");
		return(-1);
	}

	/* generate the name of the ephemeral fstab file */
	snprintf(efstab, (size_t)512, "/tmp/.fstab.%s_%u", label, getpid());

	if ((efd = open(efstab, O_RDWR|O_CREAT|O_NONBLOCK|O_APPEND)) <= 0) { 
		dbg;
		fprintf(stderr, "Unable to open %s for writing!\n", efstab);
		free(efstab);
		return(-1);
	}

	for (i = 0; i < fscount; i++) {
		dprintf(efd, "%s\t%s\t%s\t%s\t%d\t%d\n", snapfs->fstab.fs_spec, snapfs->fstab.fs_file, 
				                                         snapfs->fstab.fs_vfstype, snapfs->fstab.fs_mntops,
                                                 snapfs->fstab.fs_freq, snapfs->fstab.fs_passno);
	}

	/* 
	 * new funciton, specifically dumps current fstab into a file with a timestamp of when it was created
	 * ideally will include the BE label in the future a swell, though that would require some extra parsing
	 */
	swapfstab("/etc/fstab", &efd, false);

	setfstab(efstab);
	printfs(getfstab());
	/* this may be doable in a more clean manner, but it should work properly */
	fprintf(stdout,"Creating backup fstab...\n");
	/* I'm not sure if this is necessarily correct, but it supposedly guarantees some atomicity in its operation */
	/* 
	 * I suspct the issue is with these rename calls
	 *rename("/etc/fstab", "/etc/fstab.bak");
	 */
	fprintf(stdout,"Installing new fstab...\n");
	rename(efstab, "/etc/fstab");
	unlink(efstab);
	close(efd);
	free(efstab);
	return(0);
}

/*
 * activate a given boot environment
 */
static int
activate(const char *label) { 
	/* 
	 * the *label parameter is only used for the pfs lookups, by default this is called by create()
	 * we'll scan currently mounted filesystems for the given boot environment label, and use that to 
	 * create the ephemeral fstab, which then replaces the current fstab, putting it in fstab.label
	 * assuming there was a label. If no label exists for the existing fstab, we'll simply call it 
	 * fstab.bak
	 */
	char *efstab;
	FILE *efd;

	if ((efstab = calloc((size_t)512, sizeof(char))) == NULL) {
		fprintf(stderr,"Error Allocating Ephemeral fstab\n");
	}

	snprintf(efstab, (size_t)512, "/tmp/.fstab.%d.%s", getpid(), label);
	fprintf(stderr, "Created Ephemeral fstab at %s", efstab);

	if ((efd = fopen(efstab, "a")) == NULL) { 
		err(errno, "%s: %s: ", __progname, efstab);
	}

	fclose(efd);
	free(efstab);
	return(0);
}

/*
 * deactivate the given boot environment
 */
static int
deactivate(const char *label) { 
	/* 
	 * I'm not sure what this function will do in particular as the 
	 * naming convention described above is fstab.label, making it trivial to swap boot environments 
	 * This function likely only needs to be called when destroying a boot environment
	 */
	return(0);
}

/*
 * delete a given boot environment
 */
static int
rmenv(const char *label) { 
	return(0);
}

/*
 * delete the given snapshot
 */
static int
rmsnap(const char *pfs) { 
	return(0);
}

static void
printfs(const char *fstab) { 
	struct fstab *fsent;

	fsent = NULL;
	while ((fsent = getfsent()) != NULL) {
		fprintf(stdout,"%s\t%s\t%s\t%s\t%s\t%d\t%d\n",
		               fsent->fs_spec, fsent->fs_file, fsent->fs_vfstype,
									 fsent->fs_mntops, fsent->fs_type, fsent->fs_freq,
									 fsent->fs_passno);
	}
	endfsent();
}

static int
swapfstab(const char *current, int * newfd, bool uselabel) {
	/*
	 * thi function will open the old fstab, clean it out after dumpting contents to a new 
	 * backup file, then write the contents of the ephemeral fstab into it
	 */
	int bfd, cfd;
	ssize_t written;
	char tmpbuf[PAGESIZE]; /* work with a page of data at a time, defaulting to 4096 if not otherwise defined */

	bfd = cfd = written = 0;

	if ((cfd = open(current, O_RDWR|O_NONBLOCK)) <= 0) { 
		fprintf(stderr,"%s: unable to open r/w, check your user and file permissions!\n",current);
		return(-1);
	}
	if ((bfd = open("/etc/fstab.bak", O_TRUNC|O_CREAT|O_RDWR|O_NONBLOCK)) <= 0) {
		fprintf(stderr, "/etc/fstab.bak could not ebe created, verify file and user permissions are set properly!\n");
		return(-2);
	}

	/* need to read from cfd into bfd */
	/* at this point, we should have 3 available file descriptors */
	for (;;) {
		read(cfd, &tmpbuf, PAGESIZE);
		/* these check errno for possible errors, may split into seprate functions later */
		switch (errno) {
			case EBADF:
				/* given bad fd */
				fprintf(stderr, "fd %d is not open for reading\n", cfd);
				return(-3);
			case EFAULT:
				/* buffer pointer is outside allowed address space (should never happen) */
				fprintf(stderr, "Something has gone horribly wrong, bailing out\n");
				return(-3);
			case EIO:
				/* error encountered with the IO operations in the filesystem */
				fprintf(stderr, "I/O error reading from fd %d\n", cfd);
				return(-3);
			case EINTR:
				/* read was interrupted by a signal before data arrived */
				fprintf(stderr, "Read was interrupted, no data recieved\n");
				return(-3);
			case EINVAL:
				/* given a negative file descriptor, which should be impossible */
				fprintf(stderr, "This should not be possible, %s, %d\n", __FILE__, __LINE__);
				return(-3);
			case EAGAIN:
				/* this should not cause an issue */
				continue;
			default:
				continue;
		}
		written = write(bfd, &tmpbuf, PAGESIZE);
		switch (errno) {
			case EBADF:
				/* given bad fd */
				fprintf(stderr, "fd %d is not open for reading\n", bfd);
				return(-4);
			case EPIPE:
				/* given fd is a pipe that cannot be written to, should not be possible in this usecase */
				fprintf(stderr, "This should not be possible, %s, %d\n", __FILE__, __LINE__);
				return(-4);
			case EFBIG:
				/* file being written to exceeds size limits */
				fprintf(stderr, "Unable to write to fd %d, size exceeded\n", bfd);
				return(-4);
			case EFAULT:
				/* data being written is outside usable address space, should not be possible */
				fprintf(stderr, "This should not be possible, %s, %d\n", __FILE__, __LINE__);
				return(-4);
			case EINVAL:
				fprintf(stderr, "This should not be possible, %s, %d\n", __FILE__, __LINE__);
				return(-4);
			case ENOSPC:
				fprintf(stderr, "The drive holding /etc has run out of space, bailing out\n");
				return(-4);
			case EDQUOT:
				fprintf(stderr, "The filesystem holding /etc has reached its quota, bailing out\n");
				return(-4);
			case EIO:
				fprintf(stderr, "I/O error writing to fd %d\n", bfd);
				return(-4);
			case EINTR:
				fprintf(stderr, "Interrupted before writing any data\n");
				return(-4);
			case EAGAIN:
				continue;
			case EROFS:
			default:
				continue;
		}
		if (written == 0) {
			fprintf(stderr, "/etc/fstab should now be backed up\n");
			break;
		}
	}

	/* next step is writing the ephemeral fstab to the actual fstab */
	return(0);
}
