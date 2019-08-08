#define DFBEADM_MAIN_H

#define PFSDELIM '@'
#define BESEP ':'
#define TMAX 18
#define NOTIMP(a) fprintf(stderr,"WRN: %s [%s:%u] %s: -%c is not implemented at this time!\n",__progname,__FILE__,__LINE__,__func__,a)

/* Asserts are a good thing to have across all files */
#include <assert.h>
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
