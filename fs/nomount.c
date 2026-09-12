// SPDX-License-Identifier: GPL-2.0
/*
 * fs/nomount.c - hide mountpoints from /proc mounts visibility
 *
 * Minimal nomount integration for lucreticus: VFS-level mount checks
 * plus procfs filtering and a /proc control toggle.
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/mount.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "nomount.h"

static bool nomount_enabled = true;

static const char * const nomount_hidden_substrs[] = {
	"/data/adb",
	"KSU",
	"ksu",
	"magisk",
	"apatch",
	"nomount",
	NULL,
};

static bool nomount_match_hidden(const char *str)
{
	int i;

	if (!str)
		return false;

	for (i = 0; nomount_hidden_substrs[i]; i++) {
		if (strstr(str, nomount_hidden_substrs[i]))
			return true;
	}

	return false;
}

bool nomount_should_hide_mnt(struct vfsmount *mnt)
{
	struct mount *r;

	if (!nomount_enabled || !mnt || !mnt->mnt_root)
		return false;

	r = real_mount(mnt);
	if (r->mnt_devname && nomount_match_hidden(r->mnt_devname))
		return true;

	if (mnt->mnt_root->d_name.name &&
	    nomount_match_hidden(mnt->mnt_root->d_name.name))
		return true;

	return false;
}
EXPORT_SYMBOL_GPL(nomount_should_hide_mnt);

bool nomount_should_hide_path(const struct path *path)
{
	char *buf;
	char *p;
	bool ret = false;

	if (!nomount_enabled || !path || !path->dentry)
		return false;

	buf = __getname();
	if (!buf)
		return false;

	p = d_path(path, buf, PATH_MAX);
	if (!IS_ERR(p))
		ret = nomount_match_hidden(p);

	__putname(buf);
	return ret;
}
EXPORT_SYMBOL_GPL(nomount_should_hide_path);

int nomount_hook_do_mount(const char *dev_name, const char __user *dir_name,
			  const char *type_page, unsigned long flags)
{
	char kbuf[128];
	long copied = 0;

	if (!nomount_enabled)
		return 0;

	if (dev_name && nomount_match_hidden(dev_name))
		return 0;

	if (dir_name) {
		copied = strncpy_from_user(kbuf, dir_name, sizeof(kbuf) - 1);
		if (copied > 0) {
			kbuf[sizeof(kbuf) - 1] = '\0';
			if (nomount_match_hidden(kbuf))
				return 0;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(nomount_hook_do_mount);

int nomount_hook_do_umount(struct mount *mnt, int flags)
{
	if (!nomount_enabled || !mnt)
		return 0;

	return 0;
}
EXPORT_SYMBOL_GPL(nomount_hook_do_umount);

static int nomount_enabled_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", nomount_enabled ? 1 : 0);
	return 0;
}

static int nomount_enabled_open(struct inode *inode, struct file *file)
{
	return single_open(file, nomount_enabled_show, NULL);
}

static ssize_t nomount_enabled_write(struct file *file,
				     const char __user *buf,
				     size_t count, loff_t *ppos)
{
	char kbuf[4];

	if (count >= sizeof(kbuf))
		return -EINVAL;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	kbuf[count] = '\0';
	if (kbuf[0] == '0')
		nomount_enabled = false;
	else if (kbuf[0] == '1')
		nomount_enabled = true;
	else
		return -EINVAL;

	return count;
}

static const struct file_operations nomount_enabled_fops = {
	.owner = THIS_MODULE,
	.open = nomount_enabled_open,
	.read = seq_read,
	.write = nomount_enabled_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init nomount_init(void)
{
	proc_create("nomount_enabled", 0644, NULL, &nomount_enabled_fops);
	pr_info("nomount: mountpoint hiding hooks active\n");
	return 0;
}
fs_initcall(nomount_init);
