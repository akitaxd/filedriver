#include "driver.h"
inline unsigned long long index = 0;
class Req {
public:
	int id;
	int client_pid;
	unsigned long long client_res_addr;
	int target_pid;
	unsigned long long target_addr;
	unsigned long long size;
	unsigned long long handled_address;
	unsigned long long unique;
};


void queue(Req req)
{
	std::ofstream file("C:\\LMFAO");
	file.write(reinterpret_cast<char*>(&req), sizeof(req));
	file.close();
}
static int process_id;
void init_driver(int pid)
{
	process_id = pid;
}

int read_memory(
	const uintptr_t address,
	const uintptr_t buffer,
	const size_t	size)
{
	auto pid = GetCurrentProcessId();
	auto req = Req{};
	auto handled = false;
	req.id = READ_MEMORY;
	req.client_pid = pid;
	req.client_res_addr = buffer;
	req.target_pid = process_id;
	req.target_addr = address;
	req.size = size;
	req.handled_address = UINT64(&handled);
	req.unique = index++;
	queue(req);
	while (!handled)
	{}
	return 0;
}
int drv::proc_base(
	const uintptr_t pidd,
	const uintptr_t buffer)
{
	auto pid = GetCurrentProcessId();
	auto req = Req{};
	auto handled = false;
	req.id = PROC_BASE;
	req.client_pid = pid;
	req.client_res_addr = buffer;
	req.target_pid = pidd;
	req.target_addr = 0;
	req.size = 0;
	req.handled_address = UINT64(&handled);
	req.unique = index++;
	queue(req);
	while (!handled)
	{}
	return 0;

}
int write_memory(
	const uintptr_t address,
	const uintptr_t buffer,
	const size_t	size)
{
	auto pid = GetCurrentProcessId();
	auto req = Req{};
	auto handled = false;
	req.id = WRITE_MEMORY;
	req.client_pid = pid;
	req.client_res_addr = buffer;
	req.target_pid = process_id;
	req.target_addr = address;
	req.size = size;
	req.handled_address = UINT64(&handled);
	req.unique = index++;
	queue(req);
	while (!handled)
	{}
	return 0;

}