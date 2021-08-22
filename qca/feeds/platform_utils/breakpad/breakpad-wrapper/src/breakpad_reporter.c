/* Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/inotify.h>
#include <string.h>
#include <limits.h>

#define EVENT_SIZE ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int main()
{
    int fd, wd, length;
    char buffer[EVENT_BUF_LEN];
    char *eventIter = NULL;
    struct inotify_event *event = NULL;
    char hotplugcmd[] = "/etc/hotplug.d/dump/10-breakpad-dump";

    fd = inotify_init();

    if (fd < 0) {
        perror("inotify_init");
    }

    wd = inotify_add_watch(fd, "/tmp", IN_CREATE);

    while(1)
    {
        length = read(fd, buffer, EVENT_BUF_LEN);

        if (length < 0)
        {
            perror( "Unable to Read" );
        }

        for(eventIter = buffer; eventIter < buffer + length; eventIter += EVENT_SIZE + event->len)
        {
            event = (struct inotify_event *) eventIter;

            if (event->len && (event->mask & IN_CREATE))
            {
                if(EndsWithDmp(event->name) == 0)
                {
                    char shellcmd[100];
                    snprintf(shellcmd, 100, "%s %s", hotplugcmd, event->name);
                    system(shellcmd);
                }
            }
        }
    }
    inotify_rm_watch(fd, wd);
    close(fd);
}

int EndsWithDmp(char *string)
{
    string = strrchr(string, '.');
    if(string != NULL)
        return(strcmp(string, ".dmp"));
    return -1;
}
