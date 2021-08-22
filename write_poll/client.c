#include "hrd.h"

struct hrd_ctrl_blk *cb;

int main() {
    fprintf(stderr, "Starting client...\n");

    cb = hrd_ctrl_blk_init("mlx5_0", 3);
    
    hrd_publish_qp(cb, "client-qp");

    struct host_attr *server_attr = hrd_query_qp("server-qp");
    if (!server_attr) {
        fprintf(stderr, "Waiting for erver...\n");
        while (!server_attr) {
            server_attr = hrd_query_qp("erver-qp");
            usleep(200000);
        }
    }

    hrd_connect_qp(cb, server_attr);

    return 0;
}