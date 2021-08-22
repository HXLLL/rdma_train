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
    CPE(cb->ctx == NULL, "Error opening device", -1);

    cb->gid_index = gid_index;
    ibv_query_gid(cb->ctx, PORT_NUM, gid_index, &cb->dgid);             // set dgid, gid_index

    cb->pd = ibv_alloc_pd(cb->ctx);                                     // set pd
    CPE(cb->pd == NULL, "Allocate PD Error: ", -1)

    cb->buffer = malloc(BUFFER_SIZE);

    cb->mr = ibv_reg_mr(cb->pd, cb->buffer, BUFFER_SIZE, IBV_ACCESS_LOCAL_WRITE 
                         | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    CPE(!cb->mr, "Error registering MR.", -1);

    hrd_create_qp(cb);                                                  // set cq, qp

    return cb;
}

int hrd_create_qp(struct hrd_ctrl_blk *cb) {

    cb->cq = ibv_create_cq(cb->ctx, 16, NULL, NULL, 0);
    CPE(cb->cq == NULL, "Error Allocating CQ: ", -1);

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
    CPE(cb->qp == NULL, "Error creating QP ", -1);

    struct ibv_qp_attr init_attr;
    memset(&init_attr, 0, sizeof(init_attr));
    init_attr.qp_state = IBV_QPS_INIT;
    init_attr.pkey_index = 0;
    init_attr.port_num = PORT_NUM;
    init_attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

    int ret;
    ret = ibv_modify_qp(cb->qp, &init_attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);
    CPE(ret != 0, "Failed to modify QP to INIT state", ret);
    
    return 0;
}

int hrd_connect_qp(struct hrd_ctrl_blk *cb, struct host_attr *remote_qp_attr) {
    struct ibv_qp_attr rtr_attr;
    memset(&rtr_attr, 0, sizeof(rtr_attr));
    rtr_attr.qp_state = IBV_QPS_RTR;
    rtr_attr.path_mtu = IBV_MTU_2048;
    rtr_attr.dest_qp_num = remote_qp_attr->qpn;
    rtr_attr.rq_psn = HRD_DEFAULT_PSN;

    rtr_attr.ah_attr.is_global = 1;
    rtr_attr.ah_attr.sl = 0;
    rtr_attr.ah_attr.port_num = PORT_NUM;
    rtr_attr.ah_attr.grh.dgid = remote_qp_attr->dgid;
    rtr_attr.ah_attr.grh.hop_limit = 1;
    rtr_attr.ah_attr.grh.sgid_index = cb->gid_index;

    rtr_attr.max_dest_rd_atomic = 16;
    rtr_attr.min_rnr_timer = 12;

    int ret;
    ret = ibv_modify_qp(cb->qp, &rtr_attr, IBV_QP_STATE | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN
                    | IBV_QP_RQ_PSN | IBV_QP_AV | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);
    CPE(ret != 0, "Failed to modify QP to RTR", ret);

    if (cb->qp->state == IBV_QPS_RTR) {
        fprintf(stderr, "Successfully modify QP to RTR\n");
    }

    struct ibv_qp_attr rts_attr;
    memset(&rts_attr, 0, sizeof(rts_attr));
    rts_attr.qp_state = IBV_QPS_RTS;
    rts_attr.timeout = 14;
    rts_attr.retry_cnt = 7;
    rts_attr.rnr_retry = 7;
    rts_attr.sq_psn = HRD_DEFAULT_PSN;
    rts_attr.max_rd_atomic = 16;
    ret = ibv_modify_qp(cb->qp, &rts_attr, IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT
                    | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC);
    CPE(ret != 0, "Failed to modify QP to RTS", ret);

    if (cb->qp->state == IBV_QPS_RTS) {
        fprintf(stderr, "Successfully modify QP to RTS");
    }

    return 0;
}

memcached_st* create_memc() {
    memcached_server_st *servers = NULL;
    memcached_st *memc = memcached_create(NULL);
    memcached_return rc;

    servers = memcached_server_list_append(servers, REGISTRY_IP,
                                           REGISTRY_PORT, &rc);

    rc = memcached_server_push(memc, servers);
    CPE(rc != MEMCACHED_SUCCESS, "failed to push memcached server", rc);

    return memc;
}

int hrd_publish_qp(struct hrd_ctrl_blk *cb, char *key) { 
    struct host_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.dgid = cb->dgid;
    attr.qpn = cb->qp->qp_num;
    attr.rkey = cb->mr->rkey;
    attr.mr_addr = cb->mr->addr;

    memcached_st *memc = create_memc();
    memcached_return rc;

    rc = memcached_set(memc, key, strlen(key), (char*)&attr, sizeof(attr), 0, 0);
    CPE(rc != MEMCACHED_SUCCESS, "failed to set key-value pair", rc);

    memcached_free(memc);

    return 0;
}

struct host_attr *hrd_query_qp(char *key) {
    memcached_st *memc = create_memc();
    memcached_return rc;

    struct host_attr *remote_qp = 
        (struct host_attr*)memcached_get(memc, key, strlen(key), NULL, NULL, &rc);

    memcached_free(memc);
    return remote_qp;
}

void hrd_dump_ctrl_blk(struct hrd_ctrl_blk *cb) { 
    fprintf(stderr, "Device: %s\n", cb->dev_name);
    fprintf(stderr, "gid index: %d\n", cb->gid_index);
    for (int i=0;i!=16;++i) {
        fprintf(stderr, "%2x:", cb->dgid.raw[i]);
    }
    fprintf(stderr, "\n");
}

void hrd_dump_dgid(union ibv_gid gid) {
    for (int i=0;i!=16;++i) {
        fprintf(stderr, "%2x:", gid.raw[i]);
    }
    fprintf(stderr, "\n");
}