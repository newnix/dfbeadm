# DFBEADM(1) A Boot Environment Manager for HAMMER2
This is a tool inspired by the `beadm` utility for FreeBSD/Illumos systems that creates and manages
ZFS boot environments. This utility in contrast is written from the ground up in C, this should
provide better performance, integration, and extensibility than the POSIX `sh` and `awk` script
it was inspired by. During the time this project has been worked on, `beadm` has been superceeded by
`bectl` on FreeBSD. After hammering out some of the outstanding internal logic issues, I might look at
providing a similar interface to the command as `bectl`.

## Outline
The general process works as follows:
	* Run `sudo dfbeadm -c 2019.08.01`
	* `dfbeadm` scans all mounted filesystems for HAMMER2 volumes
	* Mountpoints for all HAMMER2 mounts are opend
	* Existing boot environment labels are cleared if found
	* The PFS label is preserved, the label given on the command-line is added onto the end
	* The buffer holding all snapshot structures is passed into `snapfs()` which loops over the filesystems
	* If a filesystem structure has the `snap` member set to `true`, a HAMMER2 snapshot is created
	* The existing `fstab(5)` is copied to `/etc/fstab.bak`
	* A new `fstab` is generated under `/tmp`
	* Once the new `fstab` is tested, it's installed to `/etc/fstab`
	* TODO: Handle the update of `loader.conf(5)` to point to the new boot environment

## Usage
Currently, the `dfbeadm` utility will create snapshots of all mounted HAMMER2 filesystems with a consistent label,
this is done by adding the string `:${LABEL}` to the end of the current PFS label. For example a PFS of `nvme0s1d@ROOT` 
turns into `nvme0s1d@ROOT:20190801` if invoked as `dfbeadm -c 20190801`.

The only other supported operation at this time is the `-l` flag, which opens the HAMMER2 filesystem mounted at `/` and
reads off all the snapshots visible, it's assumed that all snapshots are part of a full "boot environment"

## Limitations
The `dfbeadm` utility will generate and install a new `/etc/fstab` after copying the existing file to `/etc/fstab.bak`,
to ensure that the proper configuration exists after rebooting into the new boot environment this is done prior to creating the 
snapshots. A rollback and cleanup process is planned, but not currently implemented, so if boot environment creation fails,
you'll have to manually replace the `/etc/fstab` with `/etc/fstab.bak`. 

It also doesn't yet manage `/boot/loader.conf` so the entry `vfs.root.mountfrom` will need to be updated manually to point to the 
new boot environment as well. Using the above example, you'd have an entry like `vfs.root.mountfrom="hammer2:nvme0s1d@ROOT:20190801"`.

Both of these limitations will be removed in a future version, and will not be major long-term blockers for future development.

There's also an odd issue that I'll need to look into for future developments. It only applies to specific filesystem layouts,
if you have your own home directory on its own PFS, the permissions will be set to `root:wheel 000` after booting into the new boot environment.
So you'll have to reset permissions after reboot, I'm not sure what the best solution will be, but I'm considering using a server/client model to
have a privileged process able to reset permissions properly after reboot as well as remove the need for privilege escalation to even list the existing boot environments.

Since there's currently no way to exclude filesystems from a boot environment, it may be desirable to manually modify the new `/etc/fstab` to
prune certain directories from the boot environment until that functionality is included. Alternatively, it may be best to create a new boot environment prior to shutting down or rebooting.
