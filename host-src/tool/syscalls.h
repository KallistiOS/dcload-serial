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

#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

// Log groups for verbose logging
#define LOG_OPEN        (1 << 0)
#define LOG_CLOSE       (1 << 1)
#define LOG_READ        (1 << 2)
#define LOG_WRITE       (1 << 3)
#define LOG_SEEK        (1 << 4)
#define LOG_STAT        (1 << 5)
#define LOG_DELETE      (1 << 6)
#define LOG_LINK        (1 << 7)
#define LOG_CHDIR       (1 << 8)
#define LOG_CHMOD       (1 << 9)
#define LOG_UTIME       (1 << 10)
#define LOG_DIR         (1 << 11)
#define LOG_TIME        (1 << 12)
#define LOG_CDFS        (1 << 13)
#define LOG_ALL         ((1 << 14) - 1)

extern unsigned int enabled_log_groups;

int parse_log_groups(const char *arg);
void help_log_groups(void);

void dc_fstat(void);
void dc_write(void);
void dc_read(void);
void dc_open(void);
void dc_close(void);
void dc_creat(void);
void dc_link(void);
void dc_unlink(void);
void dc_chdir(void);
void dc_chmod(void);
void dc_lseek(void);
void dc_time(void);
void dc_stat(void);
void dc_utime(void);

void dc_opendir(void);
void dc_readdir(void);
void dc_closedir(void);
void dc_rewinddir(void);

void dc_cdfs_redir_read_sectors(int isofd);

void dc_gdbpacket(void);

_Noreturn void dc_exit(void);

#endif
