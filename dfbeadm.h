#define DFBEADM

#define PFSDELIM '@'
#define BESEP ':'
#define TMAX 18

/* necessary inclusions for vfs layer data */
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/ucred.h>

/* for fstab manipulation/verification */
#include <fstab.h>

/* HAMMER2 specific needs */
#include <vfs/hammer2/hammer2_ioctl.h>

/* borderline useless "debug" printout */
/* should be reworked as needed */
#define dbg fprintf(stderr, "Error in %s:%s: %d\n", __func__, __FILE__, __LINE__);

/* struct to hold the relevant data to rebuild the fstab */
struct bootenv_data { 
	struct fstab fstab; /* this should be pretty obvious, but this is each PFS's description in the fstab */
	char curlabel[NAME_MAX]; /* this may actually not be necessary, bubt it's the current label of the PFS */
	struct hammer2_ioc_pfs snapshot; /* this is the PFS we'll be creating a snapshot with */
	bool snap;
} __packed;

struct efstab_lookup {
	char mounutpoint[1024];
	struct fstab fsent;
};

typedef struct bootenv_data bedata;
static int autoactivate(bedata *snapfs, int fscount, const char *label);
static int create(const char *label);
static bool ish2(const char *mountpoint);
static void fstrunc(char *longstring);
static void mktargets(bedata *target, int fscount, const char *label);
static void printfs(const char *fstab);
static int relabel(bedata *fs, const char *label);
static int snapfs(bedata *snapfs, int fscount);
static void xtractLabel(const char *fs, char *label);
static int swapfstab(const char *current, int * newfd, bool uselabel);

extern char *__progname;


/*
 * tell the user how this program works
 */
static void __attribute__((noreturn))
usage(void) { 
	fprintf(stderr,"%s: Utility to create HAMMER2 boot environments.\n",__progname);
	fprintf(stderr,"Usage:\n"
	               "  -a  Activate the given boot environment\n"
	               "  -c  Create a new boot environment with the given label\n"
	               "  -d  Destroy the given boot environment\n"
								 "  -D  Print debugging information during execution\n"
	               "  -h  This help text\n"
	               "  -l  List existing boot environments\n"
								 "  -n  No-op/dry run, only show what would be done\n"
	               "  -r  Remove the given boot environment\n");
	exit(0);
}
