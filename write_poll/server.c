#include "hrd.h"

struct hrd_ctrl_blk *cb;

int main() { 
    fprintf(stderr, "Starting server...\n");

    cb = hrd_ctrl_blk_init("mlx5_0", 0);

    reset_memcached();

    hrd_publish_qp(cb, "server-qp");

    struct host_attr *client_attr = hrd_query_qp("client-qp");
    if (!client_attr) {
        fprintf(stderr, "Waiting for client...\n");
        while (!client_attr) {
            client_attr = hrd_query_qp("client-qp");
            usleep(200000);
        }
    }
    
    fprintf(stderr, "Successfully get client qp!\n");

    hrd_connect_qp(cb, client_attr);

    ((char*)cb->buffer)[0] = '_';
    while (1) {
        write(STDOUT_FILENO, (char*)cb->buffer, 1);
        write(STDOUT_FILENO, "\n", 1);
        usleep(100000);
    }

    return 0;
}