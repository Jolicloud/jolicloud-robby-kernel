/*
 * AppArmor security module
 *
 * This file contains AppArmor auditing function definitions.
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#ifndef __AA_AUDIT_H
#define __AA_AUDIT_H

#include <linux/audit.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "file.h"

struct aa_profile;

extern const char *audit_mode_names[];
#define AUDIT_MAX_INDEX 5

#define AUDIT_APPARMOR_AUTO 0	/* auto choose audit message type */

enum audit_mode {
	AUDIT_NORMAL,		/* follow normal auditing of accesses */
	AUDIT_QUIET_DENIED,	/* quiet all denied access messages */
	AUDIT_QUIET,		/* quiet all messages */
	AUDIT_NOQUIET,		/* do not quiet audit messages */
	AUDIT_ALL		/* audit all accesses */
};

extern const char *op_table[];
enum aa_ops {
	OP_NULL,

	OP_SYSCTL,
	OP_CAPABLE,

	OP_UNLINK,
	OP_MKDIR,
	OP_RMDIR,
	OP_MKNOD,
	OP_TRUNC,
	OP_LINK,
	OP_SYMLINK,
	OP_RENAME_SRC,
	OP_RENAME_DEST,
	OP_CHMOD,
	OP_CHOWN,
	OP_GETATTR,
	OP_OPEN,

	OP_FPERM,
	OP_FLOCK,
	OP_FMMAP,
	OP_FMPROT,

	OP_CREATE,
	OP_POST_CREATE,
	OP_BIND,
	OP_CONNECT,
	OP_LISTEN,
	OP_ACCEPT,
	OP_SENDMSG,
	OP_RECVMSG,
	OP_GETSOCKNAME,
	OP_GETPEERNAME,
	OP_GETSOCKOPT,
	OP_SETSOCKOPT,
	OP_SOCK_SHUTDOWN,

	OP_PTRACE,

	OP_EXEC,
	OP_CHANGE_HAT,
	OP_CHANGE_PROFILE,
	OP_CHANGE_ONEXEC,

	OP_SETPROCATTR,
	OP_SETRLIMIT,

	OP_PROF_REPL,
	OP_PROF_LOAD,
	OP_PROF_RM,
};

/*
 * aa_audit - AppArmor auditing structure
 * Structure is populated by access control code and passed to aa_audit which
 * provides for a single point of logging.
 */
struct aa_audit {
	struct task_struct *task;
	int error;
	int op;
	const char *name;
	const char *info;
	union {
		long pos;
		int cap;
		int rlimit;
		struct {
			pid_t tracer, tracee;
		} ptrace;
		struct {
			const char *path;
			const char *target;
			u16 request;
			u16 denied;
			uid_t ouid;
		} fs;
		struct {
			int family, type, protocol;
		} net;
	};
};

int aa_audit(int type, struct aa_profile *profile, gfp_t gfp,
	     struct aa_audit *sa,
	     void (*cb) (struct audit_buffer *, struct aa_audit *));

static inline int complain_error(int error)
{
	if (error == -EPERM || error == -EACCES)
		return 0;
	return error;
}

#endif /* __AA_AUDIT_H */
