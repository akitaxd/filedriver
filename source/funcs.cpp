#include "funcs.h"

PEPROCESS fromPid(int pid)
{
    PEPROCESS proc;
    PsLookupProcessByProcessId((HANDLE)pid, &proc);
    return proc;
}
DWORD getoffsets()
{
	RTL_OSVERSIONINFOW ver = { 0 };
	RtlGetVersion(&ver);

	switch (ver.dwBuildNumber)
	{
	case WINDOWS_1803:
		return 0x0278;
		break;
	case WINDOWS_1809:
		return 0x0278;
		break;
	case WINDOWS_1903:
		return 0x0280;
		break;
	case WINDOWS_1909:
		return 0x0280;
		break;
	case WINDOWS_2004:
		return 0x0388;
		break;
	case WINDOWS_20H2:
		return 0x0388;
		break;
	case WINDOWS_21H1:
		return 0x0388;
		break;
	default:
		return 0x0388;
	}
}

auto getprocessdirbase(PEPROCESS targetprocess) -> ULONG_PTR
{
	if (!targetprocess)
		return 0;

	PUCHAR process = (PUCHAR)targetprocess;
	ULONG_PTR process_dirbase = *(PULONG_PTR)(process + 0x28);
	if (process_dirbase == 0)
	{
		auto userdiroffset = getoffsets();
		ULONG_PTR process_userdirbase = *(PULONG_PTR)(process + userdiroffset);
		return process_userdirbase;
	}
	return process_dirbase;
}

__forceinline auto readphysaddress(PVOID address, PVOID buffer, SIZE_T size, SIZE_T* read) -> NTSTATUS
{
	if (!address)
		return STATUS_UNSUCCESSFUL;

	MM_COPY_ADDRESS addr = { 0 };
	addr.PhysicalAddress.QuadPart = (LONGLONG)address;
	return MmCopyMemory(buffer, addr, size, MM_COPY_MEMORY_PHYSICAL, read);
}

__forceinline auto writephysaddress(PVOID address, PVOID buffer, SIZE_T size, SIZE_T* written) -> NTSTATUS
{
	if (!address)
		return STATUS_UNSUCCESSFUL;

	PHYSICAL_ADDRESS addr = { 0 };
	addr.QuadPart = (LONGLONG)address;

	auto mapped_mem = MmMapIoSpaceEx(addr, size, PAGE_READWRITE);

	if (!mapped_mem)
		return STATUS_UNSUCCESSFUL;

	memcpy(mapped_mem, buffer, size);
	
	*written = size;
	MmUnmapIoSpace(mapped_mem, size);
	return STATUS_SUCCESS;
}

__forceinline auto translateaddress(unsigned long long processdirbase, unsigned long long address) -> unsigned long long
{
	processdirbase &= ~0xf;

	unsigned long long pageoffset = address & ~(~0ul << PAGE_OFFSET_SIZE);
	unsigned long long pte = ((address >> 12) & (0x1ffll));
	unsigned long long pt = ((address >> 21) & (0x1ffll));
	unsigned long long pd = ((address >> 30) & (0x1ffll));
	unsigned long long pdp = ((address >> 39) & (0x1ffll));

	SIZE_T readsize = 0;
	unsigned long long pdpe = 0;
	readphysaddress((void*)(processdirbase + 8 * pdp), &pdpe, sizeof(pdpe), &readsize);
	if (~pdpe & 1)
		return 0;

	unsigned long long pde = 0;
	readphysaddress((void*)((pdpe & mask) + 8 * pd), &pde, sizeof(pde), &readsize);
	if (~pde & 1)
		return 0;

	if (pde & 0x80)
		return (pde & (~0ull << 42 >> 12)) + (address & ~(~0ull << 30));

	unsigned long long ptraddr = 0;
	readphysaddress((void*)((pde & mask) + 8 * pt), &ptraddr, sizeof(ptraddr), &readsize);
	if (~ptraddr & 1)
		return 0;

	if (ptraddr & 0x80)
		return (ptraddr & mask) + (address & ~(~0ull << 21));

	address = 0;
	readphysaddress((void*)((ptraddr & mask) + 8 * pte), &address, sizeof(address), &readsize);
	address &= mask;

	if (!address)
		return 0;

	return address + pageoffset;
}

__forceinline auto readprocessmemory(PEPROCESS process, PVOID address, PVOID buffer, SIZE_T size, SIZE_T* read) -> NTSTATUS
{
	auto process_dirbase = getprocessdirbase(process);

	SIZE_T curoffset = 0;
	while (size)
	{
		auto addr = translateaddress(process_dirbase, (ULONG64)address + curoffset);
		if (!addr) return STATUS_UNSUCCESSFUL;

		ULONG64 readsize = min(PAGE_SIZE - (addr & 0xFFF), size);
		SIZE_T readreturn = 0;
		auto readstatus = readphysaddress((void*)addr, (PVOID)((ULONG64)buffer + curoffset), readsize, &readreturn);
		size -= readreturn;
		curoffset += readreturn;
		if (readstatus != STATUS_SUCCESS) break;
		if (readreturn == 0) break;
	}

	*read = curoffset;
	return STATUS_SUCCESS;
}

__forceinline auto writeprocessmemory(PEPROCESS process, PVOID address, PVOID buffer, SIZE_T size, SIZE_T* written) -> NTSTATUS
{
	auto process_dirbase = getprocessdirbase(process);

	SIZE_T curoffset = 0;
	while (size)
	{
		auto addr = translateaddress(process_dirbase, (ULONG64)address + curoffset);
		if (!addr) return STATUS_UNSUCCESSFUL;

		ULONG64 writesize = min(PAGE_SIZE - (addr & 0xFFF), size);
		SIZE_T written = 0;
		auto writestatus = writephysaddress((void*)addr, (PVOID)((ULONG64)buffer + curoffset), writesize, &written);
		size -= written;
		curoffset += written;
		if (writestatus != STATUS_SUCCESS) break;
		if (written == 0) break;
	}

	*written = curoffset;
	return STATUS_SUCCESS;
}
void finish_request(int pid, UINT64 allah)
{
	PEPROCESS process = fromPid(pid);
	bool yes = true;
	SIZE_T returnS = 0;
	writeprocessmemory(process, (void*)allah, (void*)&yes, 1, &returnS);
}
ULONGLONG getprocbase(int pid)
{
	PEPROCESS process = NULL;
	PsLookupProcessByProcessId((HANDLE)pid, &process);
	return (ULONGLONG)PsGetProcessSectionBaseAddress(process);
}
void memoryCopyOperation(int srcPid, UINT64 srcAddr, int tgPid, UINT64 tgAddr, uint64_t size)
{
    PEPROCESS src = fromPid(srcPid);
    PEPROCESS tg = fromPid(tgPid);
	if (!src || !tg)
		return;
    SIZE_T return1 = 0;
	SIZE_T return2 = 0;
	PVOID buffer = ExAllocatePool(NonPagedPool, size);
	if (!buffer)
		return;
	if(!NT_SUCCESS(readprocessmemory(src, (void*)srcAddr, (void*)buffer, size, &return1)))
		goto end;
	if (!NT_SUCCESS(writeprocessmemory(tg, (void*)tgAddr, (void*)buffer, size, &return2)))
		goto end;
end:
	ExFreePoolWithTag(buffer, 0);
}
void get_proc_base(int srcPid, UINT64 srcAddr, int tgPid)
{
	PEPROCESS src = fromPid(srcPid);
	SIZE_T return2 = 0;
	auto value = getprocbase(tgPid);
	writeprocessmemory(src, (void*)srcAddr, (void*)&value, 8, &return2);
}


