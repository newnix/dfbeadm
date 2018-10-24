#define SNAPFS_H
/* 
 * This should ideall be a filesystem agnostic function
 * to create a snapshot with the given label
 * Honestly, with the current bedata definitions, fscount and label may not be necessary parameters
 */
static int
snapfs(bedata *fstarget, int fscount) { 
	/* 
	 * This likely uses HAMMER2IOC_PFS_SNAPSHOT to create hammer2 snapshots, will need to reference
	 * the hammer2 utility implementation.
	 * TODO: Turn this function into an abstraction for any CoW filesystem, selected at compile-time
	 * then if I'm able, possibly even available at runtime should multiple CoW filesystems be available
	 * though this could be expanded to any filesystem with the same functionality of snapshots. Possibly 
	 * including both HAMMER and UFS in later versions
	 */
	int fd, i;
	char *newfs;

	i = fd = 0;

	if ((newfs = calloc(NAME_MAX, sizeof(char))) == NULL) { 
		dbg;
		return(-1);
	}

	for (i ^= i; i < fscount; i++) {
		if (fstarget[i].snap) {
			/* possibly fixes the locking issue observed in recent builds */
			if ((fd = open(fstarget[i].fstab.fs_file, O_RDONLY|O_NONBLOCK)) <= 0) {
				fprintf(stderr, "Can't open %s!\n%s\n", fstarget[i].fstab.fs_file, strerror(errno));
			}
			fprintf(stderr, "Creating snapshot %s...\nOld label: %s\n", fstarget[i].fstab.fs_spec, fstarget[i].curlabel);

			xtractLabel(fstarget[i].fstab.fs_spec, newfs);
			/* 
			 * This is one of the two sections of this program that actually requires 
			 * root access as far as I'm aware. The other being the obvious installation of the
			 * new fstab file. 
			 */
			strlcpy(fstarget[i].snapshot.name, newfs, NAME_MAX);
			/* We use the following ioctl() to actually create a snapshot */
			if (ioctl(fd, HAMMER2IOC_PFS_SNAPSHOT, &fstarget[i].snapshot) != -1) {
				fprintf(stdout, "Created new snapshot: %s\n", fstarget[i].snapshot.name);
			} else {
				fprintf(stderr, "H2 Snap failed!\n%s\n(target: %s)\n",strerror(errno), fstarget[i].snapshot.name);
			}
			fprintf(stderr, "Closing fd %d for mountpoint %s\n", fd, fstarget[i].fstab.fs_file);
			close(fd);
			memset(newfs, 0, NAME_MAX);
		}
	}
	/* this is to separate activation from creation, despite being the same conceptually */
	autoactivate(fstarget, fscount, newfs);
	free(newfs);
	return(0);
}

static void
xtractLabel(const char *newfs, char *label) {
	/* this function just returns the PFS label of the new snapshot */
	char *pfssep;
	int i;

	if ((pfssep = strchr(newfs, PFSDELIM)) == NULL) {
		dbg;
		/* TODO: Revisit this code, seems to be causing trouble with NULLFS mounts */
		exit(13); /* to be handled by something else later */
	} else {
		for (i = 0, ++pfssep; *pfssep != 0; i++,pfssep++) {
			label[i] = *pfssep;
		}
		label[i] = 0;
		fprintf(stderr,"xtractLabel: %d\nLabel: %s\n", __LINE__, label);
	}
}
