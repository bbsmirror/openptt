/* $Id$ */
#ifndef __STRUCT_H__
#define __STRUCT_H__

#define MAX_HOST_LEN      16

typedef struct {
    int fd;
    char remote_host[MAX_HOST_LEN];
} client_t;

#endif
