#include "defines.h"
#include <stdint.h>

void memoryCopyOperation(int srcPid, UINT64 srcAddr, int tgPid, UINT64 tgAddr, uint64_t size);
void finish_request(int pid, UINT64 allah);
void get_proc_base(int srcPid, UINT64 srcAddr, int tgPid);
