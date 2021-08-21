#include "hrd.h"

struct hrd_ctrl_blk *cb;

int main() { 
    fprintf(stderr, "Starting server...\n");

    cb = hrd_ctrl_blk_init("mlx5_0", 3);

    publish_qp(cb, "server-qp");

    return 0;
}