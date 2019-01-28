#define H2TEST_H
/*
 * Determine if the given mountpoint is a HAMMER2 filesystem 
 */
static bool
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
static void
fstrunc(char *longstring) { 
	snprintf(longstring, NAME_MAX, "%s", longstring);
}
