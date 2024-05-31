#pragma once
#include "funcs.h"

#define READ_MEMORY 1
#define WRITE_MEMORY 2
#define GET_PROC_BASE 3

class Req {
public:
    int id;
    int client_pid;
    unsigned long long client_res_addr;
    int target_pid;
    unsigned long long target_addr;
    unsigned long long size;
    unsigned long long finished_handling_address;
    unsigned long long unique_id;
    void handle()
    {
        switch (id)
        {
        case READ_MEMORY:
            memoryCopyOperation(target_pid, UINT64(target_addr), client_pid, UINT64(client_res_addr), SIZE_T(size));
            finish_request(client_pid, finished_handling_address);
            break;
        case WRITE_MEMORY:
            memoryCopyOperation(client_pid, UINT64(client_res_addr), target_pid, UINT64(target_addr), SIZE_T(size));
            finish_request(client_pid, finished_handling_address);
            break;
        case GET_PROC_BASE:
            get_proc_base(client_pid, client_res_addr, target_pid);
            finish_request(client_pid, finished_handling_address);
            break;
        }
    }
};
