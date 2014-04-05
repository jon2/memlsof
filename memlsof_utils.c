/*
 * memlsof_utils.c
 *
 *  Created on: April 1, 2014
 *      Author: Jon Green
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memlsof.h"

unsigned int find_process_addr_space(FILE *fd, pid_t pid, char *name) {

	unsigned int addr = SWAPPER_PTR;		/* Set address of initial process */
	pid_t foundPID = 0;
	unsigned int next_task=0;

	do {
		/* PID */
		fetch(fd, (void *)&foundPID, sizeof(foundPID), addr + PID_OFFSET);

		if (foundPID == pid) {
			/* Get process name */
			fetch(fd, (void *)name, TASK_COMM_LEN, addr + NAME_OFFSET);
			return addr;
		}

		/* Calculate next addr */
		fetch(fd, (void *)&next_task, sizeof(next_task), addr + NEXT_TASK_OFFSET);
		addr = next_task - MEM_OFFSET - NEXT_TASK_OFFSET;
	} while (addr != SWAPPER_PTR);

	/* If we got here, no matching PID was found */
	return 0;
}

void fetch(FILE *fd, void *field, int bytes, int offset) {
	if(0 != fseek(fd,offset,SEEK_SET)) {
    	printf("Seek error\n");
    	exit(-1);
    }

    if (1 != fread(field, bytes,1,fd)) {
    	printf("fread() error\n");
    	exit(-1);
    }
}

unsigned int get_dentry(FILE *fd, unsigned int file) {
	unsigned int dentry_ptr=0;

	fetch(fd, (void *)&dentry_ptr, sizeof(dentry_ptr), file + DENTRY_OFFSET);
	dentry_ptr -= MEM_OFFSET;
	if (DEBUG)
		printf("dentry at %08X\n", dentry_ptr);

	return dentry_ptr;
}

unsigned int get_vfsmount(FILE *fd, unsigned int file) {
	unsigned int vfsmount_ptr=0;

	fetch(fd, (void *)&vfsmount_ptr, sizeof(vfsmount_ptr), file + VFSMOUNT_OFFSET);
	vfsmount_ptr -= MEM_OFFSET;
	fetch(fd, (void *)&vfsmount_ptr, sizeof(vfsmount_ptr), vfsmount_ptr + MOUNTPOINT_OFFSET);
	vfsmount_ptr -= MEM_OFFSET;
	if (DEBUG)
		printf("vfsmount at %08X\n", vfsmount_ptr);

	return vfsmount_ptr;
}

unsigned int get_inode(FILE *fd, unsigned int dentry) {
	unsigned int inode_ptr=0;

	fetch(fd, (void *)&inode_ptr, sizeof(inode_ptr), dentry + D_INODE_OFFSET);
	inode_ptr -= MEM_OFFSET;
	if (DEBUG)
		printf("inode at %08X\n", inode_ptr);

	return inode_ptr;
}

void get_path(FILE *fd, unsigned int dentry_ptr, struct path_t *path) {
	unsigned int parent_dentry=0;
	unsigned int qstrLen=0;
	unsigned int qstrName_ptr=0;
	char *qstrName;
	int root=0;

	/* Parent */
	fetch(fd, (void *)&parent_dentry, sizeof(parent_dentry), dentry_ptr + D_PARENT);
	parent_dentry -= MEM_OFFSET;
	if (parent_dentry != dentry_ptr) {
		if (DEBUG)
			printf("Parent: %08X", parent_dentry);
		get_path(fd, parent_dentry, path); /* Recursive */
		root = 0;
	} else {
		root = 1;
	}

	/* This entry */
	fetch(fd, (void *)&qstrLen, sizeof(qstrLen), dentry_ptr + QSTR_LEN_OFFSET);

	if (qstrLen) {
		fetch(fd, (void *)&qstrName_ptr, sizeof(qstrName_ptr), dentry_ptr + QSTR_OFFSET);
		qstrName_ptr -= MEM_OFFSET;
		qstrName = malloc(qstrLen * sizeof(char));
		fetch(fd, (void *)qstrName, qstrLen, qstrName_ptr);
		strncpy(path->name + path->length, qstrName, qstrLen);
		path->length += qstrLen;
		if (!root) {
			/* add slash between path elements */
			strncpy(path->name + path->length, "/", 1);
			path->length++;
		}
		if (DEBUG)
			printf("\tName: %s\n\n", qstrName);
	}
}

void get_type(unsigned short mode, char *typestr) {
	/* from stat.h */
	#define S_IFMT  00170000
	#define S_IFSOCK 0140000
	#define S_IFLNK	 0120000
	#define S_IFREG  0100000
	#define S_IFBLK  0060000
	#define S_IFDIR  0040000
	#define S_IFCHR  0020000
	#define S_IFIFO  0010000
	#define S_ISUID  0004000
	#define S_ISGID  0002000
	#define S_ISVTX  0001000

	switch(mode & S_IFMT) {
	case S_IFSOCK:
		sprintf(typestr, "SOCK");
		break;
	case S_IFLNK:
		sprintf(typestr, "LNK");
		break;
	case S_IFREG:
		sprintf(typestr, "REG");
		break;
	case S_IFBLK:
		sprintf(typestr, "BLK");
		break;
	case S_IFDIR:
		sprintf(typestr, "DIR");
		break;
	case S_IFCHR:
		sprintf(typestr, "CHR");
		break;
	case S_IFIFO:
		sprintf(typestr, "FIFO");
		break;
	}
}

void get_mode(unsigned short mode, char *modestr) {
	/* from stat.h */

#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define S_ISUID  0004000
#define S_ISGID  0002000

	memset(modestr, 45, 9);

	if(mode & S_IRUSR)
		modestr[0] = 'r';
	if(mode & S_ISUID)		/* setuid */
		modestr[0] = 's';
	if(mode & S_IWUSR)
		modestr[1] = 'w';
	if(mode & S_IXUSR)
		modestr[2] = 'x';
	if(mode & S_IRGRP)
		modestr[3] = 'r';
	if(mode & S_ISGID)		/* setgid */
		modestr[3] = 's';
	if(mode & S_IWGRP)
		modestr[4] = 'w';
	if(mode & S_IXGRP)
		modestr[5] = 'x';
	if(mode & S_IROTH)
		modestr[6] = 'r';
	if(mode & S_IWOTH)
		modestr[7] = 'w';
	if(mode & S_IXOTH)
		modestr[8] = 'x';

	memset(modestr+9, 0, 1); /* add null to terminate string */
}
