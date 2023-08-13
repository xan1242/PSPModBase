#include <pspsdk.h>
#include <pspuser.h>
#include "pspmallochelper.h"

void* psp_malloc(size_t size)
{
	SceUID uid = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "", PSP_SMEM_Low, size + 8, NULL);
	if (uid >= 0)
	{
		unsigned int* p = (unsigned int*)sceKernelGetBlockHeadAddr(uid);
		*p = uid;
		*(p + 4) = size;
		return (void*)(p + 8);
	}

	return NULL;
}

void psp_free(void* ptr)
{
	if (ptr)
	{
		sceKernelFreePartitionMemory(*((SceUID*)ptr - 8));
	}
}
