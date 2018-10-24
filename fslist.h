#define DF_LIST_H
/*
 * list the available boot environments
 * returns the number of boot environments found,
 * negative return values indicate errors
 */
static int
list(void) { 
	char *bename;
	int rootfd, found;
	struct hammer2_ioc_pfs h2be;

	found = rootfd = 0;
	bename = NULL;

	if (getenv("UID") != 0) { 
		fprintf(stderr, "You must be root to access this information\n");
		return(-1);
	}
	if ((rootfd = open("/", O_RDONLY|O_NONBLOCK)) < 0) { 
		fprintf(stderr, "Unable to open /, something has gone horribly wrong!\n");
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
		if ((bename = strchr(h2be.name, BESEP)) != NULL) {
			for (++bename; *bename != 0; bename++) {
				fprintf(stdout,"%c",*bename);
			}
			fprintf(stdout, "\n");
			found++;
			bename = NULL;
		}
	}
	close(rootfd);
	return(found);
}
