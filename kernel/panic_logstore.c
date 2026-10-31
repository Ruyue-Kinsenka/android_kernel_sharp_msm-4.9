// SPDX-License-Identifier: GPL-2.0
/*
 * Panic logstore for pucci bring-up debugging.
 *
 * On panic() this dumps the kernel ring buffer to three places:
 *
 *  1. The Sharp "abootlog" buffer format at phys 0xa0080000
 *     (shrlog_region@a0000000 + shrlog-abootlog-offset 0x80000).
 *     After a warm reboot, a working Sharp-kernel recovery exposes it via
 *     the "abootlog" sysfs attribute of the sharp,shrlog device.
 *
 *  2. Raw scratch partitions (inactive-slot dtbo / misc), when /dev nodes
 *     exist at panic time.  Readable later via dd from any root shell and
 *     survives cold reboots.
 *
 *  3. Nothing else needed - CONFIG_PSTORE already covers the DT ramoops
 *     region when pstore registered before the crash.
 */
#define pr_fmt(fmt) "panic_logstore: " fmt
#include <linux/types.h>
#include <linux/cred.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kmsg_dump.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>

#if IS_ENABLED(CONFIG_SECURITY_SELINUX_DEVELOP)
extern int selinux_enforcing;
#define sel_set_enforce(v) do { selinux_enforcing = (v) ? 1 : 0; } while (0)
#else
static void sel_set_enforce(int v) { }
#endif

/* Sharp abootlog wire format (see drivers/soc/qcom/sharp/shrlog.c) */
#define ABOOTLOG_PHYS		0xa0080000ULL
#define ABOOTLOG_DATA_MAX	4096
struct abootlog_hdr {
	u32 bufsize;
	u32 cur_idx;
	u32 total_size;
	char data[ABOOTLOG_DATA_MAX];
};

/* scratch partitions; inactive-slot dtbo is freely reflashable */
static const char * const store_paths[] = {
	"/dev/block/bootdevice/by-name/dtbo_b",
	"/dev/block/bootdevice/by-name/dtbo_a",
	"/dev/block/bootdevice/by-name/misc",
	NULL,
};
#define STORE_MAGIC	"PUCCIPANIC\n"
#define STORE_MAX_BYTES	(512 * 1024)

static void logstore_to_memory(void)
{
	static struct abootlog_hdr hdr;
	struct kmsg_dumper dumper = {0};
	void __iomem *p;
	char line[1024];
	size_t len, off = 0;

	p = memremap(ABOOTLOG_PHYS, sizeof(hdr), MEMREMAP_WB);
	if (!p) {
		pr_err("memremap of abootlog region failed\n");
		return;
	}
	memset(&hdr, 0, sizeof(hdr));
	dumper.active = true;
	/* keep the tail of the log - most relevant lines are last */
	while (kmsg_dump_get_line(&dumper, false, line, sizeof(line), &len)) {
		if (off + len <= ABOOTLOG_DATA_MAX) {
			memcpy(hdr.data + off, line, len);
			off += len;
		} else {
			/* shift down to keep the tail */
			size_t drop = off + len - ABOOTLOG_DATA_MAX;

			memmove(hdr.data, hdr.data + drop,
				ABOOTLOG_DATA_MAX - drop);
			memcpy(hdr.data + ABOOTLOG_DATA_MAX - len, line, len);
			off = ABOOTLOG_DATA_MAX;
		}
	}
	hdr.bufsize = off;
	hdr.cur_idx = 0;
	hdr.total_size = off;
	memcpy_toio(p, &hdr, sizeof(hdr));
	memunmap(p);
	pr_info("wrote %u bytes to abootlog region\n", hdr.bufsize);
}

static void logstore_to_partitions(void)
{
	const struct cred *saved_cred, *root_cred;
	struct file *f;
	char buf[1024];
	size_t len;
	int i, ret;
	loff_t pos;
	struct kmsg_dumper dumper;

	sel_set_enforce(0);
	root_cred = prepare_kernel_cred(NULL);
	if (!root_cred)
		return;
	saved_cred = override_creds(root_cred);

	for (i = 0; store_paths[i]; i++) {
		f = filp_open(store_paths[i], O_WRONLY, 0);
		if (IS_ERR(f))
			continue;
		pr_info("writing panic log to %s\n", store_paths[i]);
		pos = 0;
		ret = kernel_write(f, STORE_MAGIC, sizeof(STORE_MAGIC) - 1, pos);
		pos += (ret > 0) ? ret : 0;
		memset(&dumper, 0, sizeof(dumper));
		dumper.active = true;
		while (kmsg_dump_get_line(&dumper, false, buf, sizeof(buf), &len)) {
			if (pos >= STORE_MAX_BYTES)
				break;
			ret = kernel_write(f, buf, len, pos);
			if (ret != len)
				break;
			pos += ret;
		}
		vfs_fsync(f, 0);
		filp_close(f, NULL);
	}

	revert_creds(saved_cred);
	sel_set_enforce(1);
}

void do_logstore(void)
{
	pr_info("logstore start\n");
	logstore_to_memory();
	logstore_to_partitions();
	pr_info("logstore done\n");
}

static int logstore_trigger_store(
		const char *buf, const struct kernel_param *kp)
{
	do_logstore();
	return 0;
}
static struct kernel_param_ops logstore_trigger_ops = {
	.set = &logstore_trigger_store,
};
module_param_cb(trigger, &logstore_trigger_ops, NULL, 0644);
