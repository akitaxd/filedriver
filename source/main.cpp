#include "reqs.h"

UNICODE_STRING     uniName;
OBJECT_ATTRIBUTES  objAttr;
LARGE_INTEGER      byteOffset;
size_t  cb;

void listen_thread(void*)
{
    InitializeObjectAttributes(&objAttr, &uniName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL, NULL);
    unsigned long long lastHandled = 0;
    HANDLE   handle;
    IO_STATUS_BLOCK    ioStatusBlock;
    ZwCreateFile(&handle,
        0,
        &objAttr, &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL, 0);
    while (true)
    {
        byteOffset.LowPart = byteOffset.HighPart = 0;
        Req req;
        ZwReadFile(handle, NULL, NULL, NULL, &ioStatusBlock, PVOID(&req), sizeof(Req), &byteOffset, NULL);
        if (req.unique_id != lastHandled)
        {
            lastHandled = req.unique_id;
            req.handle();
        }
    }
}

NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
    RtlInitUnicodeString(&uniName, L"\\DosDevices\\C:\\LMFAO");

    HANDLE thread = 0;
    PsCreateSystemThread(
        &thread,
        GENERIC_ALL,
        nullptr,
        nullptr,
        nullptr,
        listen_thread,
        nullptr
    );
	return STATUS_SUCCESS;
}
