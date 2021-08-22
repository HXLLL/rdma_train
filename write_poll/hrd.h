#ifndef HRD_H
#define HRD_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <infiniband/verbs.h> 
#include <libmemcached/memcached.h>

#define HRD_DEFAULT_PSN 2333

#define MAX_OUT 16
#define BLK_SIZE 16
#define BUFFER_SIZE 2048
#define PORT_NUM 1
#define GID_INDEX 3
#define REGISTRY_IP "192.168.2.1"
#define REGISTRY_PORT 8888

#define CPE(val, msg, ret)                                                    \
    if (val) {                                                           \
        fprintf(stderr, msg);                                            \
        fprintf(stderr, " Error %d %s , ret value: %d\n", errno, strerror(errno), ret);      \
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

    struct ibv_mr *mr;

    void *buffer;
};


struct host_attr {
    union ibv_gid dgid;
    int qpn;
    uint32_t rkey;
    void *mr_addr;
};

struct hrd_ctrl_blk *hrd_ctrl_blk_init(
    char* dev_name, int gid_index
);
int hrd_create_qp(struct hrd_ctrl_blk *cb);
int hrd_connect_qp(struct hrd_ctrl_blk *cb, struct host_attr *remote_qp_attr);

memcached_st* create_memc();
int hrd_publish_qp(struct hrd_ctrl_blk *cb, char *key);
struct host_attr *hrd_query_qp(char *key);

void hrd_dump_ctrl_blk(struct hrd_ctrl_blk *cb);
void hrd_dump_dgid(union ibv_gid gid);
#endif /* HRD_H */