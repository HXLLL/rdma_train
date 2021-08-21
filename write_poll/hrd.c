#include "hrd.h"

struct hrd_ctrl_blk *hrd_ctrl_blk_init(
    char* dev_name, int gid_index
) {
    fprintf(stderr, "HRD: creating control block, dev_name=%s\n", dev_name);

    struct hrd_ctrl_blk *cb = (struct hrd_ctrl_blk*) malloc(sizeof(struct hrd_ctrl_blk));
    memset(cb, 0, sizeof(struct hrd_ctrl_blk));

    cb->dev_name = dev_name;                                            // set devname

    int num_devices = 0;
    struct ibv_device **devices = NULL;
    devices = ibv_get_device_list(&num_devices);
    if (devices == NULL) {
        fprintf(stderr, "Error getting device list, %s\n", strerror(errno));
        return NULL;
    }
    for (int i=0;i!=num_devices;i++) {
        // printf("%d: %s\n", i, ibv_get_device_name(devices[i]));
        if (strcmp(ibv_get_device_name(devices[i]), dev_name) == 0) {
            cb->ctx = ibv_open_device(devices[i]);                      // set ctx
        }
    }
    ibv_free_device_list(devices);
    CPE(cb->ctx == NULL, "Error opening device");

    cb->gid_index = gid_index;
    ibv_query_gid(cb->ctx, PORT_NUM, gid_index, &cb->dgid);             // set dgid, gid_index

    cb->pd = ibv_alloc_pd(cb->ctx);                                     // set pd
    CPE(cb->pd == NULL, "Allocate PD Error: ")

    hrd_create_qp(cb);                                                  // set cq, qp

    return cb;
}

int hrd_create_qp(struct hrd_ctrl_blk *cb) {

    cb->cq = ibv_create_cq(cb->ctx, 16, NULL, NULL, 0);
    CPE(cb->cq == NULL, "Error Allocating CQ: ");

    struct ibv_qp_init_attr qp_create_attr;
    memset(&qp_create_attr, 0, sizeof(qp_create_attr));

    qp_create_attr.send_cq = cb->cq;
    qp_create_attr.recv_cq = cb->cq;
    qp_create_attr.qp_type = IBV_QPT_RC;
    qp_create_attr.sq_sig_all = 1;

    qp_create_attr.cap.max_send_wr = 255;
    qp_create_attr.cap.max_recv_wr = 255;
    qp_create_attr.cap.max_send_sge = 32;
    qp_create_attr.cap.max_recv_sge = 32;
    qp_create_attr.cap.max_inline_data = 128;

    cb->qp = ibv_create_qp(cb->pd, &qp_create_attr);
    
    return 0;
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

int publish_qp(struct hrd_ctrl_blk *cb, char *key) { 
    struct host_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.dgid = cb->dgid;

    memcached_st *memc = create_memc();
    memcached_return rc;

    rc = memcached_set(memc, key, strlen(key), (char*)&attr, sizeof(attr), 0, 0);
    CPE(rc != MEMCACHED_SUCCESS, "failed to set key-value pair");

    memcached_free(memc);

    return 0;
}

struct host_attr query_qp(char *key, struct host_attr *attr) {

}