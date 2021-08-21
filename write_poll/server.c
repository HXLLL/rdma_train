#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <infiniband/verbs.h>

struct host_attr {
    union ibv_gid dgid;
};

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

int publish_qp(struct host_attr *attr, size_t size) { 

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

    return 0;
}