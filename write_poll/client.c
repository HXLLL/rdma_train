#include "hrd.h"

struct hrd_ctrl_blk *cb;

char send_buffer[BUFFER_SIZE];
struct ibv_mr *send_buffer_mr;
struct ibv_sge *send_sge[MAX_OUT];

void rdma_write_string(struct hrd_ctrl_blk *cb, struct host_attr *attr, char *s, size_t len, int sid) {
    struct ibv_send_wr wr;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = 100 + sid;
    wr.next = NULL;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_RDMA_WRITE;
    wr.send_flags = IBV_SEND_INLINE;
    wr.wr.rdma.remote_addr = (uint64_t)attr->mr_addr;
    wr.wr.rdma.rkey = attr->rkey;

    strncpy(send_buffer + sid*BLK_SIZE, s, len);
    send_sge[sid]->addr = (uint64_t)send_buffer + sid*BLK_SIZE;
    send_sge[sid]->length = len;
    send_sge[sid]->lkey = send_buffer_mr->lkey;
    wr.sg_list = send_sge[sid];

    int ret;
    ret = ibv_post_send(cb->qp, &wr, NULL);
    CPE(ret, "Error posting write operation.", ret);
}

int main() {
    fprintf(stderr, "Starting client...\n");

    cb = hrd_ctrl_blk_init("mlx5_0", 3);
    
    hrd_publish_qp(cb, "client-qp");

    struct host_attr *server_attr = hrd_query_qp("server-qp");
    if (!server_attr) {
        fprintf(stderr, "Waiting for erver...\n");
        while (!server_attr) {
            server_attr = hrd_query_qp("server-qp");
            usleep(200000);
        }
    }

    hrd_connect_qp(cb, server_attr);

    send_buffer_mr = ibv_reg_mr(cb->pd, send_buffer, BUFFER_SIZE, IBV_ACCESS_LOCAL_WRITE);
    CPE(!send_buffer_mr, "Error registering send_buffer", -1);

    int a = 998244353;
    rdma_write_string(cb, server_attr, (char*)&a, 4, 0);
    return 0;
}