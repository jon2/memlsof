/*
 * memlsof.c
 *
 *  Created on: April 1, 2014
 *      Author: Jon Green
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memlsof.h"


int main(int argc, char *argv[])
{
    FILE* fd = NULL;
    char cmd[TASK_COMM_LEN];
    unsigned int pid=0;
    unsigned int addr=0;
    unsigned int files=0;
    unsigned int fdtable=0;
    unsigned int max_fds=0;
    unsigned int fd_array_ptr=0;
    unsigned int *fd_array=0;
    unsigned int dentry=0;
    unsigned int inode=0;
    unsigned int mountpoint=0;
    unsigned long i_ino=0;
    dev_t i_rdev=0;
    char devstring[16];
    long long i_size=0;
    unsigned short mode;  /* defined in asm-x86/posix_types_32.h */
    char typestr[8];
    char modestr[10];
    int i=0;
    struct path_t path;

    if (argc != 3)
    {
            printf("usage: memlsof <filename> <pid>\n");
            printf("\n");
            exit(-1);
    }

    fd = fopen(argv[1],"r");

    if(NULL == fd) {
      printf("Couldn't open file\n");
      exit(-1);
    }

    pid = atoi(argv[2]);

    /* addr will be the PHYSICAL address of the PID's task_struct
     * In general in this program, address variables will use physical address
     * When we print to the screen, we'll use virtual addresses (add MEM_OFFSET)
     */
    addr=find_process_addr_space(fd, pid, cmd);
    if (addr == 0) {
    	printf("Error: PID not found\n");
    	exit(-1);
    }

  	printf("%-16s %-5s %-4s %-10s %-8s %-8s %-8s %-8s %-20s\n", \
   			"COMMAND", "PID", "FD", "MODE", "TYPE", "DEVICE", "SIZE", "NODE", "NAME");

    /* Get the files_struct pointer */
	fetch(fd, (void *)&files, sizeof(files), addr + FILES_OFFSET);
	if (DEBUG)
		printf("files_struct at: 0x%08X\n", files);
	files -= MEM_OFFSET;

	/* We have a pointer into files_struct
	 * Inside files_struct, read the fdtable pointer.  It *may* point four bytes ahead,
	 * in the same struct, if max_fds is small.  Or it may point somewhere else.  In either
	 * case, following the pointer will get us where we need to be.
	 */
	fetch(fd, (void *)&fdtable, sizeof(fdtable), files + FDTABLE_OFFSET);
	if (DEBUG)
		printf("fdtable at: 0x%08X", fdtable);
	fdtable -= MEM_OFFSET;

	/* Get max FDs and pointer to fd array */
	fetch(fd, (void *)&max_fds, sizeof(max_fds), fdtable);
	fetch(fd, (void *)&fd_array_ptr, sizeof(fd_array_ptr), fdtable + F_MAXENTRY_OFFSET);
	if (DEBUG)
		printf(" with %u max entries.  FD array begins at 0x%8X.\n\n", max_fds, fd_array_ptr);
	fd_array_ptr -= MEM_OFFSET;

	/* Allocate array to hold file pointers, then fill it */
	fd_array = malloc(max_fds * sizeof(unsigned int));
	if (fd_array == NULL) {
		printf("malloc() error\n");
		exit(-1);
	}
	fetch(fd, (void *)fd_array, max_fds * sizeof(unsigned int), fd_array_ptr);

	/* Iterate through the fd_array */
	for(i=0; i<=max_fds; i++) {
		if (fd_array[i] != 0) {
			if (DEBUG)
				printf("Entry %4i is open.  File address: 0x%08X\n", i, fd_array[i]);

			/* Get pointers to main structures that we care about */
			dentry = get_dentry(fd, fd_array[i] - MEM_OFFSET);
			mountpoint = get_vfsmount(fd, fd_array[i] - MEM_OFFSET);
			inode = get_inode(fd, dentry);

			/* Mode */
			fetch(fd, (void *)&mode, sizeof(mode), inode + I_MODE_OFFSET);
			get_type(mode, typestr);
			get_mode(mode, modestr);
			if(DEBUG) {
				printf("Mode: %o  Found at: %08X Type: %s  Perm: %s\n", mode, inode + I_MODE_OFFSET, typestr, modestr);
			}

			/* inode */
			fetch(fd, (void *)&i_ino, sizeof(i_ino), inode + I_INO_OFFSET);

			/* size */
			fetch(fd, (void *)&i_size, sizeof(i_size), inode + I_SIZE_OFFSET);

			/* device */
			fetch(fd, (void *)&i_rdev, sizeof(i_rdev), inode + I_RDEV_OFFSET);
			if ((strcmp(typestr, "CHR") == 0) || (strcmp(typestr, "BLK") == 0)) {
				sprintf(devstring, "%u,%u", MAJOR(i_rdev), MINOR(i_rdev));
			} else {
				sprintf(devstring, "--");
			}

			/* NAME */
			path.length=0;
			memset(path.name, 0, 1024);
			if (!((strcmp(typestr, "FIFO") == 0) || (strcmp(typestr, "SOCK") == 0))) {
				get_path(fd, mountpoint, &path);
				/* Strip trailing / */
				if (path.name[path.length-1] == '/') {
					memset(path.name+path.length-1, 0, 1);
					path.length--;
				}
			}

			get_path(fd, dentry, &path);
			/* Strip trailing / */
			if (path.name[path.length-1] == '/') {
				memset(path.name+path.length-1, 0, 1);
				path.length--;
			}

		  	printf("%-16s %-5u %-4u %-10s %-8s %-8s %-8llu %-8lu %-20s\n", \
		   			cmd, pid, i, modestr, typestr, devstring, i_size, i_ino, path.name);

		}
	}

    fclose(fd);
    return 0;
}


