/*
 * memlsof.h
 *
 *  Created on: April 1, 2014
 *      Author: Jon Green
 */

#ifndef MEMLSOF_H_
#define MEMLSOF_H_

#define DEBUG 0
#define SWAPPER_PTR 0x003ed3a0		/* Physical address where init_task is found */
#define MEM_OFFSET 0xc0000000		/* Offset from kernel addr to physical addr */

#define NEXT_TASK_OFFSET 0x8c		/* Position of next task */
#define NAME_OFFSET 0x1cd      		/* Position of name */
#define PID_OFFSET 0xc8				/* PID */
#define FILES_OFFSET 0x484			/* Pointer to files struct in process descriptor */

#define FDTABLE_OFFSET 0x4			/* Pointer to fdtable within files_struct */

#define F_MAXENTRY_OFFSET 0x4		/* Max FDs within fdtable struct */

#define VFSMOUNT_OFFSET 0x8			/* vfsmount offset within file struct */
#define DENTRY_OFFSET 0xc			/* dentry offset within file struct */

#define D_INODE_OFFSET 0xC			/* inode struct pointer inside dentry */
#define D_PARENT 0x18				/* parent dentry pointer in dentry */
#define QSTR_LEN_OFFSET 0x20		/* qstr length offset in dentry struct */
#define QSTR_OFFSET 0x24			/* qstr name offset in dentry struct */

#define I_MODE_OFFSET 0x6A			/* Mode offset in inode struct */
#define I_INO_OFFSET 0x20			/* inode number in inode struct */
#define I_SIZE_OFFSET 0x3c			/* size offset in inode struct */
#define I_RDEV_OFFSET 0x34			/* i_rdev offset in inode struct */

#define MOUNTPOINT_OFFSET 0xc		/* mnt_mountpoint offset in vfsmount struct */

#define TASK_COMM_LEN 16       		/* Length of process name, from sched.h on target system */


/* Prototypes */
struct path_t {
    	char name[1024];
    	int length;
};
void fetch(FILE *fd, void *field, int bytes, int offset);
unsigned int find_process_addr_space(FILE *, pid_t, char *);
void get_path(FILE *, unsigned int, struct path_t *);
unsigned int get_vfsmount(FILE *, unsigned int);
unsigned int get_dentry(FILE *, unsigned int);
unsigned int get_inode(FILE *, unsigned int);
void get_type(unsigned short, char *);
void get_mode(unsigned short, char *);

/* from kdev_t.h */
#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)
#define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))

#endif /* MEMLSOF_H_ */
