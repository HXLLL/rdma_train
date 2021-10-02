#include "hrd.h"

struct hrd_ctrl_blk *cb;

char send_buffer[BUFFER_SIZE];
struct ibv_mr *send_buffer_mr;
struct ibv_sge *send_sge[MAX_OUT];

void rdma_write_string(struct hrd_ctrl_blk *cb, struct host_attr *attr, char *s, size_t len) {
    struct ibv_send_wr wr;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = 100;
    wr.next = NULL;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_RDMA_WRITE;
    wr.send_flags = IBV_SEND_INLINE;
    wr.wr.rdma.remote_addr = (uint64_t)attr->mr_addr;
    wr.wr.rdma.rkey = attr->rkey;

    struct ibv_sge *send_sge = (struct ibv_sge*) malloc(sizeof(struct ibv_sge));
    strncpy(send_buffer, s, len);
    send_sge->addr = (uint64_t)send_buffer;
    send_sge->length = len;
    send_sge->lkey = send_buffer_mr->lkey;
    wr.sg_list = send_sge;

    int ret;
    ret = ibv_post_send(cb->qp, &wr, NULL);
    CPE(ret, "Error posting write operation.", ret);

    free(send_sge);
}

int main() {
    fprintf(stderr, "Starting client...\n");

    cb = hrd_ctrl_blk_init("mlx5_0", 0);
    
    hrd_publish_qp(cb, "client-qp");

    struct host_attr *server_attr = hrd_query_qp("server-qp");
    if (!server_attr) {
        fprintf(stderr, "Waiting for server...\n");
        while (!server_attr) {
            server_attr = hrd_query_qp("server-qp");
            usleep(200000);
        }
    }

    hrd_connect_qp(cb, server_attr);

    send_buffer_mr = ibv_reg_mr(cb->pd, send_buffer, BUFFER_SIZE, IBV_ACCESS_LOCAL_WRITE);
    CPE(!send_buffer_mr, "Error registering send_buffer", -1);

    char *s = "hello world!";
    int len=strlen(s);
    for (int i=0;i!=len;++i) {
        printf("writing %c\n", s[i]);
        rdma_write_string(cb, server_attr, s+i, 1);
        usleep(400000);
    }

    return 0;
}