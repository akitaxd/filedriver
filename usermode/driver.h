#pragma once
#define READ_MEMORY 1
#define WRITE_MEMORY 2
#define PROC_BASE 3


#include <stdint.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>
#include <string>
#include <vector>

int read_memory(
	const uintptr_t address,
	const uintptr_t buffer,
	const size_t	size);

int write_memory(
	const uintptr_t address,
	const uintptr_t buffer,
	const size_t	size);

void init_driver(int pid);


namespace drv {
	inline uintptr_t proc_base_value;
	int proc_base(
		const uintptr_t pid,
		const uintptr_t buffer);
	inline void init(int pid)
	{
		proc_base(pid, uintptr_t(&proc_base_value));
		init_driver(pid);
	}
	template <typename T>
    T read_vm(unsigned long long addr)
	{ 
		T obj;
		read_memory(addr, uintptr_t(&obj), size_t(sizeof(obj)));
		return obj;
		
	}
	static uintptr_t read_gm(unsigned long long addr)
	{
		uintptr_t ptr = 0;ptr = read_vm<uintptr_t>(addr);
		return ptr;
	}
}

