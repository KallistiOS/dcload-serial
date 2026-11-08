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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <dirent.h>
#ifdef __MINGW32__
#include <winsock2.h>
#include <stdint.h>
#else
#include <arpa/inet.h>
#endif
#include "syscalls.h"
#include "dc-io.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Sigh... KOS treats anything under 100 as invalid for a dirent from dcload, so
   we need to offset by a bit. This aught to do. */
#define DIRENT_OFFSET   1337

#define MAX_OPEN_DIRS 512
#define MAX_TRACKED_FDS (1024+3) // 3 - num for stdio

typedef struct {
    char *name;
    DIR *dir;
} dir_data;

static dir_data opendirs[MAX_OPEN_DIRS];
// Three first descriptors are for stdio
static char *fd_names[MAX_TRACKED_FDS] = { "stdin", "stdout", "stderr" };

unsigned int enabled_log_groups = 0;

static const struct {
    const char *name;
    unsigned int mask;
} log_group_list[] = {
    { "open",   LOG_OPEN    },
    { "close",  LOG_CLOSE   },
    { "read",   LOG_READ    },
    { "write",  LOG_WRITE   },
    { "seek",   LOG_SEEK    },
    { "stat",   LOG_STAT    },
    { "delete", LOG_DELETE  },
    { "link",   LOG_LINK    },
    { "chdir",  LOG_CHDIR   },
    { "chmod",  LOG_CHMOD   },
    { "utime",  LOG_UTIME   },
    { "dir",    LOG_DIR     },
    { "time",   LOG_TIME    },
    { "cdfs",   LOG_CDFS    },
    { "file",   LOG_OPEN | LOG_CLOSE | LOG_DELETE | LOG_LINK },
    { "io",     LOG_READ | LOG_WRITE | LOG_SEEK },
    { "all",    LOG_ALL     },
};

#define LOG_GROUP_COUNT (sizeof(log_group_list) / sizeof(log_group_list[0]))

static int has_log_group(const char *arg, const char *name) {
    size_t len = strlen(name);
    const char *p = arg;

    while((p = strstr(p, name)) != NULL) {
        if((p == arg || p[-1] == ',') && (p[len] == ',' || p[len] == '\0'))
            return 1;
        p += len;
    }

    return 0;
}

int parse_log_groups(const char *arg) {
    size_t i;
    enabled_log_groups = 0;

    for(i = 0; i < LOG_GROUP_COUNT; i++) {
        enabled_log_groups |= (has_log_group(arg, log_group_list[i].name) ? log_group_list[i].mask : 0);
    }

    // Return -1 if no known log groups were found
    return enabled_log_groups == 0 ? -1 : 0;
}

void help_log_groups(void) {
    size_t i;

    printf("Log groups for -v (comma-separated): ");
    for(i = 0; i < LOG_GROUP_COUNT; i++) {
        const char *del = i == LOG_GROUP_COUNT - 1 ? "\n" : ", ";
        printf("%s%s", log_group_list[i].name, del);
    }
}

static void log_msg(unsigned int group, const char *tag, const char *fmt, ...) {
    va_list ap;

    if(!(enabled_log_groups & group))
        return;

    // we use stderr here to flush output right with dc output
    fprintf(stderr, "[%s] ", tag);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

static void store_fd(int fd, const char *name) {
    if(enabled_log_groups && fd > 2 && fd < MAX_TRACKED_FDS) {
        free(fd_names[fd]);
        fd_names[fd] = name ? strdup(name) : NULL;
    }
}

static const char *get_filename(int fd) {
    static char buf[1024];

    if(fd >= 0 && fd < MAX_TRACKED_FDS && fd_names[fd])
        snprintf(buf, sizeof(buf), "%s", fd_names[fd]);
    else
        snprintf(buf, sizeof(buf), "fd %d", fd);

    return buf;
}

void dc_fstat(void) {
    int filedes;
    struct stat filestat;
    int retval;

    filedes = recv_uint();
    retval = fstat(filedes, &filestat);

    log_msg(LOG_STAT, "fstat", "'%s'", get_filename(filedes));

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

    // Skip console output
    if(filedes != 1 && filedes != 2)
        log_msg(LOG_WRITE, "write", "'%s'", get_filename(filedes));

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

    if(filedes != 0)
        log_msg(LOG_READ, "read", "'%s'", get_filename(filedes));

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

    log_msg(LOG_OPEN, "open", "'%s'", pathname);
    store_fd(retval, (const char *)pathname);

    send_uint(retval);

    free(pathname);
}

void dc_close(void) {
    int filedes;
    int retval;

    filedes = recv_uint();

    retval = close(filedes);

    log_msg(LOG_CLOSE, "close", "'%s'", get_filename(filedes));
    if(retval == 0 && filedes > 2)
        store_fd(filedes, NULL);

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

    log_msg(LOG_OPEN, "create", "'%s'", pathname);
    store_fd(retval, (const char *)pathname);

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

    log_msg(LOG_LINK, "link", "'%s' '%s'", pathname1, pathname2);

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

    log_msg(LOG_DELETE, "delete", "'%s'", pathname);

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

    log_msg(LOG_CHDIR, "chdir", "'%s'", pathname);

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

    log_msg(LOG_CHMOD, "chmod", "'%s'", pathname);

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

    if(filedes > 2)
        log_msg(LOG_SEEK, "seek", "'%s'", get_filename(filedes));

    send_uint(retval);
}

void dc_time(void) {
    time_t t;

    time(&t);

    log_msg(LOG_TIME, "time", "%lld", (long long)t);

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

    log_msg(LOG_STAT, "stat", "'%s'", filename);

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

    log_msg(LOG_UTIME, "utime", "'%s'", pathname);

    send_uint(retval);

    free(pathname);
}

void dc_opendir(void) {
    DIR *somedir;
    unsigned char *dirname;
    int namelen;
    uint32_t i;

    namelen = recv_uint();

    dirname = malloc(namelen);

    recv_data(dirname, namelen, 0);

    /* Find an open entry */
    for(i = 0; i < MAX_OPEN_DIRS; ++i) {
        if(!opendirs[i].dir)
            break;
    }

    log_msg(LOG_DIR, "opendir", "'%s'", dirname);

    if(i < MAX_OPEN_DIRS) {
        dir_data *curr_dir = &opendirs[i];

        if(!(curr_dir->dir = opendir((const char *)dirname))) {
            i = 0;
            free(dirname);
        } else {
            curr_dir->name = (char *)dirname;
            i += DIRENT_OFFSET;
        }
    }
    else {
        i = 0;

        free(dirname);
    }

    send_uint(i);
}

void dc_closedir(void) {
    int retval;
    uint32_t i = recv_uint();

    if(i >= DIRENT_OFFSET && i < MAX_OPEN_DIRS + DIRENT_OFFSET) {
        dir_data *curr_dir = &opendirs[i - DIRENT_OFFSET];
        if(curr_dir->dir) {
            log_msg(LOG_DIR, "closedir", "'%s'", curr_dir->name ? curr_dir->name : "<unknown>");

            retval = closedir(curr_dir->dir);
            free(curr_dir->name);
            curr_dir->dir = NULL;
            curr_dir->name = NULL;
        } else {
            retval = 0;
        }
    } else {
        retval = -1;
    }

    send_uint(retval);
}

void dc_readdir(void) {
    struct dirent *somedirent;
    uint32_t i = recv_uint();

    if(i >= DIRENT_OFFSET && i < MAX_OPEN_DIRS + DIRENT_OFFSET) {
        dir_data *curr_dir = &opendirs[i - DIRENT_OFFSET];
        if(curr_dir->dir) {
            somedirent = readdir(curr_dir->dir);

            log_msg(LOG_DIR, "readdir", "'%s'", curr_dir->name ? curr_dir->name : "<unknown>");
        } else {
            somedirent = NULL;
        }
    } else {
        somedirent = NULL;
    }

    /* End of the directory */
    if(!somedirent) {
        send_uint(0);
        return;
    }

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
    send_uint(strlen(somedirent->d_name)+1);
    send_data((unsigned char *)somedirent->d_name, strlen(somedirent->d_name)+1, 0);
}

void dc_rewinddir(void) {
    int retval;
    uint32_t i = recv_uint();

    if(i >= DIRENT_OFFSET && i < MAX_OPEN_DIRS + DIRENT_OFFSET) {
        dir_data *curr_dir = &opendirs[i - DIRENT_OFFSET];
        if(curr_dir->dir) {
            log_msg(LOG_DIR, "rewinddir", "'%s'", curr_dir->name ? curr_dir->name : "<unknown>");

            rewinddir(curr_dir->dir);
        }

        retval = 0;
    } else {
        retval = -1;
    }

    send_uint(retval);
}

void dc_cdfs_redir_read_sectors(int isofd) {
    int start;
    int num;
    unsigned char * buf;

    start = recv_uint();
    num = recv_uint();

    log_msg(LOG_CDFS, "cdfs", "lba %d count %d", start, num);

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
        fflush(stdout); /* stdout may be fully buffered (e.g. pipe in an IDE) */
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

void dc_exit(void) {
    int exit_code = (int32_t) recv_uint();
    printf("Program returned %d\n", exit_code);
    finish_serial();
    exit(exit_code);
}

