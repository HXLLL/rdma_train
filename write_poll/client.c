#include "hrd.h"

struct hrd_ctrl_blk *cb;

int main() {
    fprintf(stderr, "Starting client...\n");

    cb = hrd_ctrl_blk_init("mlx5_0", 3);
    
    publish_qp(cb, "client-qp");

    return 0;
}