#ifndef HRD_H
#define HRD_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <infiniband/verbs.h> 
#include <libmemcached/memcached.h>

#define PORT_NUM 1
#define GID_INDEX 3
#define REGISTRY_IP "192.168.2.1"
#define REGISTRY_PORT 8888

#define CPE(val, msg)                                                    \
    if (val) {                                                           \
        fprintf(stderr, msg);                                            \
        fprintf(stderr, " Error %d %s \n", errno, strerror(errno));      \
        exit(1);                                                         \
    }

struct hrd_ctrl_blk { 
    union ibv_gid dgid;
    int gid_index;

    struct ibv_context *ctx;
    char *dev_name;

    int device_id;      // unused
    int dev_port_id;    // unused

    struct ibv_pd *pd;

    struct ibv_cq *cq;

    struct ibv_qp *qp;
};


struct host_attr {
    union ibv_gid dgid;
};

struct hrd_ctrl_blk *hrd_ctrl_blk_init(
    char* dev_name, int gid_index
);
int hrd_create_qp(struct hrd_ctrl_blk *cb);

memcached_st* create_memc();
int hrd_publish_qp(struct hrd_ctrl_blk *cb, char *key);
struct host_attr *hrd_query_qp(char *key);

void hrd_dump_ctrl_blk(struct hrd_ctrl_blk *cb);
void hrd_dump_dgid(union ibv_gid gid);
#endif /* HRD_H */