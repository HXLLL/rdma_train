#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

struct host_attr {
    union ibv_gid dgid;
};
union ibv_gid server_gid;

struct ibv_context* create_context(char* name) {
    struct ibv_context* ctx = NULL;
    int num_devices = 0;
    
    struct ibv_device **devices = NULL;
    devices = ibv_get_device_list(&num_devices);
    if (devices == NULL) {
        fprintf(stderr, "Error getting device list, %s\n", strerror(errno));
        return NULL;
    }
    for (int i=0;i!=num_devices;i++) {
        // printf("%d: %s\n", i, ibv_get_device_name(devices[i]));
        if (strcmp(ibv_get_device_name(devices[i]), name) == 0) {
            ctx = ibv_open_device(devices[i]);
        }
    }

    ibv_free_device_list(devices);
    if (!ctx) {
        fprintf(stderr, "Error opening device");
        return NULL;
    }

    ibv_query_gid(ctx, PORT_NUM, GID_INDEX, &server_gid);

    return ctx;
}

struct ibv_qp* create_qp(struct ibv_pd *pd, struct ibv_cq *cq) {
    struct ibv_qp_init_attr qp_create_attr;
    memset(&qp_create_attr, 0, sizeof(qp_create_attr));

    qp_create_attr.send_cq = cq;
    qp_create_attr.recv_cq = cq;
    qp_create_attr.qp_type = IBV_QPT_RC;
    qp_create_attr.sq_sig_all = 1;

    qp_create_attr.cap.max_send_wr = 255;
    qp_create_attr.cap.max_recv_wr = 255;
    qp_create_attr.cap.max_send_sge = 32;
    qp_create_attr.cap.max_recv_sge = 32;
    qp_create_attr.cap.max_inline_data = 128;

    return ibv_create_qp(pd, &qp_create_attr);
}

memcached_st* create_memc() {
    memcached_server_st *servers = NULL;
    memcached_st *memc = memcached_create(NULL);
    memcached_return rc;

    servers = memcached_server_list_append(servers, REGISTRY_IP,
                                           REGISTRY_PORT, &rc);

    rc = memcached_server_push(memc, servers);
    CPE(rc != MEMCACHED_SUCCESS, "failed to push memcached server");

    return memc;
}

int publish_host_attr(struct host_attr *attr, size_t len) { 
    memcached_st *memc = create_memc();
    memcached_return rc;
    char *server_key="server-handler";

    rc = memcached_set(memc, server_key, strlen(server_key), (char*)attr, len, 0, 0);
    CPE(rc != MEMCACHED_SUCCESS, "failed to set key-value pair");

    return 0;
}

int main() { 
    fprintf(stderr, "Starting server...\n");
    fprintf(stderr, "Creating context\n");

    struct ibv_context *ctx = create_context("mlx5_0");

    struct ibv_pd *pd = ibv_alloc_pd(ctx);
    if (pd == NULL) {
        fprintf(stderr, "Allocate PD: %d %s\n", errno, strerror(errno));
        return -1;
    }

    struct ibv_cq *cq = ibv_create_cq(ctx, 16, NULL, NULL, 0);
    if (cq == NULL) {
        fprintf(stderr, "Allocate PD: %d %s\n", errno, strerror(errno)) ;
        return -1;
    }

    struct ibv_qp *qp = create_qp(pd, cq);
    if (qp == NULL) {
        fprintf(stderr, "Create QP: %d %s\n", errno, strerror(errno));
        return -1;
    }

    struct host_attr server_attr;
    memset(&server_attr, 0, sizeof(server_attr));
    server_attr.dgid = server_gid;
    fprintf(stderr, "%llx %llx\n", server_attr.dgid.global.subnet_prefix, server_attr.dgid.global.interface_id);
    publish_host_attr(&server_attr, sizeof(server_attr));

    return 0;
}