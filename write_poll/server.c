#include <stdio.h>
#include <string.h>
#include <infiniband/verbs.h>

struct ibv_context*  create_context(char* name) {
    struct ibv_context* ctx = NULL;
    int num_devices = 0;
    
    struct ibv_device **devices = NULL;
    devices = ibv_get_device_list(&num_devices);
    if (devices == NULL) {
        fprintf(stderr, "Error getting device list, %s\n", strerror(errno));
        return NULL;
    }
    for (int i=0;i!=num_devices;i++) {
        printf("%d: %s\n", i, ibv_get_device_name(devices[i]));
        if (strcmp(ibv_get_device_name(devices[i])) == 0) {
            ctx = ibv_open_device(devices[i]);
        }
    }
    ibv_free_device_list(&devices);
    if (!ctx) {
        fprintf(stderr, "Error opening device");
        return NULL;
    }
}

int main() { 
    fprintf(stderr, "Starting server...\n");
    fprintf(stderr, "Creating context\n");
    create_context("mlx5_0");
}