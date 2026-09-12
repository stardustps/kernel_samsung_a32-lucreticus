#ifndef _FS_NOMOUNT_H
#define _FS_NOMOUNT_H

#include <linux/seq_file.h>
#include <linux/mount.h>
#include <linux/path.h>

#ifdef CONFIG_NOMOUNT
bool nomount_should_hide_mnt(struct vfsmount *mnt);
bool nomount_should_hide_path(const struct path *path);
int nomount_hook_do_mount(const char *dev_name, const char __user *dir_name,
			  const char *type_page, unsigned long flags);
int nomount_hook_do_umount(struct mount *mnt, int flags);
#else
static inline bool nomount_should_hide_mnt(struct vfsmount *mnt)
{
	return false;
}

static inline bool nomount_should_hide_path(const struct path *path)
{
	return false;
}

static inline int nomount_hook_do_mount(const char *dev_name,
					const char __user *dir_name,
					const char *type_page,
					unsigned long flags)
{
	return 0;
}

static inline int nomount_hook_do_umount(struct mount *mnt, int flags)
{
	return 0;
}
#endif /* CONFIG_NOMOUNT */

#endif /* _FS_NOMOUNT_H */
