/*
 * This file is part of the dcload Dreamcast serial loader
 *
 * Copyright (C) 2001 Andrew Kieschnick <andrewk@napalm-x.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include "dc-io.h"
#include "syscalls.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef MAX_OPEN_DIRS
#define MAX_OPEN_DIRS 16
#endif

/* Sigh... KOS treats anything under 100 as invalid for a dirent from dcload, so
   we need to offset by a bit. This aught to do. */
#define DIRENT_OFFSET 1337

static DIR *opendirs[MAX_OPEN_DIRS];

void dc_fstat(void) {
    int filedes;
    struct stat filestat;
    int retval;

    filedes = recv_uint();
    retval = fstat(filedes, &filestat);

    send_uint(filestat.st_dev);
    send_uint(filestat.st_ino);
    send_uint(filestat.st_mode);
    send_uint(filestat.st_nlink);
    send_uint(filestat.st_uid);
    send_uint(filestat.st_gid);
    send_uint(filestat.st_rdev);
    send_uint(filestat.st_size);
#ifdef __MINGW32__
    send_uint(0);
    send_uint(0);
#else
    send_uint(filestat.st_blksize);
    send_uint(filestat.st_blocks);
#endif
    send_uint(filestat.st_atime);
    send_uint(filestat.st_mtime);
    send_uint(filestat.st_ctime);

    send_uint(retval);
}

void dc_write(void) {
    int filedes;
    int retval;
    int count;
    unsigned char *data;

    filedes = recv_uint();
    count = recv_uint();

    data = malloc(count);
    recv_data(data, count, 0);

    retval = write(filedes, data, count);

    send_uint(retval);

    free(data);
}

void dc_read(void) {
    int filedes;
    int retval;
    int count;
    unsigned char *data;

    filedes = recv_uint();
    count = recv_uint();

    data = malloc(count);
    retval = read(filedes, data, count);

    send_data(data, count, 0);

    send_uint(retval);

    free(data);
}

void dc_open(void) {
    int namelen;
    int retval;
    int flags;
    int ourflags = 0;
    int mode;
    unsigned char *pathname;

    namelen = recv_uint();

    pathname = malloc(namelen);

    recv_data(pathname, namelen, 0);

    flags = recv_uint();
    mode = recv_uint();

    /* translate flags */

    if(flags & 0x0001)
        ourflags |= O_WRONLY;
    if(flags & 0x0002)
        ourflags |= O_RDWR;
    if(flags & 0x0008)
        ourflags |= O_APPEND;
    if(flags & 0x0200)
        ourflags |= O_CREAT;
    if(flags & 0x0400)
        ourflags |= O_TRUNC;
    if(flags & 0x0800)
        ourflags |= O_EXCL;

    retval = open((const char *)pathname, ourflags | O_BINARY, mode);

    send_uint(retval);

    free(pathname);
}

void dc_close(void) {
    int filedes;
    int retval;

    filedes = recv_uint();

    retval = close(filedes);

    send_uint(retval);
}

void dc_creat(void) {
    int namelen;
    unsigned char *pathname;
    int retval;
    int mode;

    namelen = recv_uint();

    pathname = malloc(namelen);

    recv_data(pathname, namelen, 0);

    mode = recv_uint();

    retval = creat((const char *)pathname, mode);

    send_uint(retval);

    free(pathname);
}

void dc_link(void) {
    int namelen1, namelen2;
    unsigned char *pathname1, *pathname2;
    int retval;

    namelen1 = recv_uint();
    pathname1 = malloc(namelen1);

    recv_data(pathname1, namelen1, 0);

    namelen2 = recv_uint();
    pathname2 = malloc(namelen2);

    recv_data(pathname2, namelen2, 0);

#ifdef __MINGW32__
    /* Copy the file on Windows */
    retval = CopyFileA(pathname1, pathname2, 0);
#else
    retval = link((const char *)pathname1, (const char *)pathname2);
#endif

    send_uint(retval);

    free(pathname1);
    free(pathname2);
}

void dc_unlink(void) {
    int namelen;
    unsigned char *pathname;
    int retval;

    namelen = recv_uint();

    pathname = malloc(namelen);

    recv_data(pathname, namelen, 0);

    retval = unlink((const char *)pathname);

    send_uint(retval);

    free(pathname);
}

void dc_chdir(void) {
    int namelen;
    unsigned char *pathname;
    int retval;

    namelen = recv_uint();

    pathname = malloc(namelen);

    recv_data(pathname, namelen, 0);

    retval = chdir((const char *)pathname);

    send_uint(retval);

    free(pathname);
}

void dc_chmod(void) {
    int namelen;
    int mode;
    unsigned char *pathname;
    int retval;

    namelen = recv_uint();

    pathname = malloc(namelen);

    recv_data(pathname, namelen, 0);

    mode = recv_uint();

    retval = chmod((const char *)pathname, mode);

    send_uint(retval);

    free(pathname);
}

void dc_lseek(void) {
    int filedes;
    int offset;
    int whence;
    int retval;

    filedes = recv_uint();
    offset = recv_uint();
    whence = recv_uint();

    retval = lseek(filedes, offset, whence);

    send_uint(retval);
}

void dc_time(void) {
    time_t t;

    time(&t);

    send_uint(t);
}

void dc_stat(void) {
    int namelen;
    unsigned char *filename;
    struct stat filestat;
    int retval;

    namelen = recv_uint();

    filename = malloc(namelen);

    recv_data(filename, namelen, 0);

    retval = stat((const char *)filename, &filestat);

    send_uint(filestat.st_dev);
    send_uint(filestat.st_ino);
    send_uint(filestat.st_mode);
    send_uint(filestat.st_nlink);
    send_uint(filestat.st_uid);
    send_uint(filestat.st_gid);
    send_uint(filestat.st_rdev);
    send_uint(filestat.st_size);
#ifdef __MINGW32__
    send_uint(0);
    send_uint(0);
#else
    send_uint(filestat.st_blksize);
    send_uint(filestat.st_blocks);
#endif
    send_uint(filestat.st_atime);
    send_uint(filestat.st_mtime);
    send_uint(filestat.st_ctime);

    send_uint(retval);

    free(filename);
}

void dc_utime(void) {
    unsigned char *pathname;
    int namelen;
    struct utimbuf tbuf;
    int foo;
    int retval;

    namelen = recv_uint();

    pathname = malloc(namelen);

    recv_data(pathname, namelen, 0);

    foo = recv_uint();

    if(foo) {
        tbuf.actime = recv_uint();
        tbuf.modtime = recv_uint();

        retval = utime((const char *)pathname, &tbuf);
    }
    else {
        retval = utime((const char *)pathname, 0);
    }

    send_uint(retval);

    free(pathname);
}

void dc_opendir(void) {
    unsigned char *dirname;
    int hnd, namelen;

    namelen = recv_uint();

    dirname = malloc(namelen);

    recv_data(dirname, namelen, 0);

    /* Find an open entry */
    for(hnd = 0; hnd < MAX_OPEN_DIRS; ++hnd) {
        if(!opendirs[hnd])
            break;
    }

    if(hnd < MAX_OPEN_DIRS) {
        if(!(opendirs[hnd] = opendir((const char *)dirname)))
            hnd = 0;
        else
            hnd += DIRENT_OFFSET;
    }
    else {
        hnd = 0;
    }

    send_uint((unsigned int)hnd);

    free(dirname);
}

void dc_closedir(void) {
    int hnd, retval;

    hnd = recv_uint();

    if(hnd >= DIRENT_OFFSET && hnd < MAX_OPEN_DIRS + DIRENT_OFFSET) {
        retval = closedir(opendirs[hnd - DIRENT_OFFSET]);
        opendirs[hnd - DIRENT_OFFSET] = NULL;
    }
    else
        retval = -1;

    send_uint(retval);
}

void dc_readdir(void) {
    int hnd;
    struct dirent *somedirent;

    hnd = recv_uint();

    if(hnd >= DIRENT_OFFSET && hnd < MAX_OPEN_DIRS + DIRENT_OFFSET)
        somedirent = readdir(opendirs[hnd - DIRENT_OFFSET]);
    else
        somedirent = NULL;

    if(somedirent) {
        send_uint(1);
        send_uint(somedirent->d_ino);
#ifdef _WIN32
        send_uint(0);
        send_uint(0);
        send_uint(0);
#else
#ifdef __APPLE_CC__
        send_uint(0);
#else
#if !defined(__FreeBSD__) && !defined(__CYGWIN__)
        send_uint(somedirent->d_off);
#endif
#endif
#ifndef __CYGWIN__
        send_uint(somedirent->d_reclen);
#endif
        send_uint(somedirent->d_type);
#endif
        send_uint(strlen(somedirent->d_name) + 1);
        send_data((unsigned char *)somedirent->d_name, strlen(somedirent->d_name) + 1, 0);
        return;
    }

    send_uint(0);
}

void dc_rewinddir(void) {
    uint32_t hnd;
    int retval;

    hnd = recv_uint();

    if(hnd >= DIRENT_OFFSET && hnd < MAX_OPEN_DIRS + DIRENT_OFFSET) {
        rewinddir(opendirs[hnd - DIRENT_OFFSET]);
        retval = 0;
    }
    else {
        retval = -1;
    }

    send_uint(retval);
}

void dc_cdfs_redir_read_sectors(int isofd) {
    int start;
    int num;
    unsigned char *buf;

    start = recv_uint();
    num = recv_uint();

    start -= 150;

    lseek(isofd, start * 2048, SEEK_SET);

    buf = malloc(num * 2048);

    read(isofd, buf, num * 2048);

    send_data(buf, num * 2048, 0);
    free(buf);
}

#define GDBBUFSIZE 1024
#ifndef __MINGW32__
extern int gdb_server_socket;
extern int socket_fd;
#else
extern SOCKET gdb_server_socket;
extern SOCKET socket_fd;
#endif

void dc_gdbpacket(void) {
    size_t in_size, out_size;

    static char gdb_buf[GDBBUFSIZE];

    int count, size, retval = 0;

    in_size = recv_uint();
    out_size = recv_uint();

    if(in_size)
        recv_data(gdb_buf, in_size > GDBBUFSIZE ? GDBBUFSIZE : in_size, 0);

#ifdef __MINGW32__
    if(gdb_server_socket == INVALID_SOCKET) {
#else
    if(gdb_server_socket < 0) {
#endif
        send_uint(-1);
        return;
    }

    if(socket_fd == 0) {
        printf("waiting for gdb client connection...\n");
        socket_fd = accept(gdb_server_socket, NULL, NULL);

        if(socket_fd == 0) {
            perror("error accepting gdb server connection");
            send_uint(-1);
            return;
        }
    }

    if(in_size)
        send(socket_fd, gdb_buf, in_size, 0);

    if(out_size) {
        retval = recv(socket_fd, gdb_buf, out_size > GDBBUFSIZE ? GDBBUFSIZE : out_size, 0);
        if(retval == 0)
            socket_fd = -1;
    }
#ifdef __MINGW32__
    if(retval == SOCKET_ERROR) {
        fprintf(stderr, "Got socket error: %d\n", WSAGetLastError());
        return;
    }
#else
    if(retval == -1) {
        fprintf(stderr, "Got socket error: %s\n", strerror(errno));
        return;
    }
#endif
    send_uint(retval);
    if(retval > 0)
        send_data((unsigned char *)gdb_buf, retval, 0);
}
