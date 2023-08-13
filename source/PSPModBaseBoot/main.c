//
// PSP userspace plugin bootstraper
// A kernel plugin which waits for the game to load before starting the userspace module.
// 
// by xan1242 / Tenjoin 
// 
// Portions of code taken from pspdev/pspsdk 'prxloader' sample and psplinkusb.
// 
// NOTE: this is intended for use with PSP CFW only! You do not need this on PPSSPP!
//


#include <pspsdk.h>
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>

// Define the name of the game's main module here
// This is the same one we define in PSPModBase
// This is also the name of the module we use to wait for until it loads.
#define MODULE_NAME_INTERNAL "TargetModule"

// Define the name of your userspace module here
// This is the name of the module that we'll boot up.
// This is the module at the same path as this booter module.
#define MODULE_BOOT_TARGET "PSPModBase.prx"

// (optional) Define the name of the game's main thread here
// If defined, the bootstrapper will suspend the thread defined here
// It will wait until the target module starts.
//#define MODULE_INTERNAL_THREAD_NAME "user_main"

// This is the name of this module that will be presented to the PSP OS.
#define MODULE_NAME "PSPModBaseBoot"
// This is the name of the thread that will be presented to the PSP OS.
#define BOOTER_THREAD_NAME "ThModBoot"

#define MODULE_VERSION_MAJOR 1
#define MODULE_VERSION_MINOR 0

// We ignore Intellisense here to reduce squiggles in VS
#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_KERNEL, MODULE_VERSION_MAJOR, MODULE_VERSION_MINOR);
// We define the main thread as a kernel thread just in case.
PSP_MAIN_THREAD_ATTR(0);
#endif

SceUID load_module(const char *path, int flags, int type)
{
	SceKernelLMOption option;
	SceUID mpid;

	/* If the type is 0, then load the module in the kernel partition, otherwise load it
	   in the user partition. */
	if (type == 0) {
		mpid = 1;
	} else {
		mpid = 2;
	}

	memset(&option, 0, sizeof(option));
	option.size = sizeof(option);
	option.mpidtext = mpid;
	option.mpiddata = mpid;
	option.position = 0;
	option.access = 1;

	return sceKernelLoadModule(path, flags, type > 0 ? &option : NULL);
}

int load_start_module2(const char *name, SceSize args, void *argp, int type)
{
	Kprintf(MODULE_NAME": Launching module: %s", name);
	SceUID mod = load_module(name, 0, type);
	if (mod < 0)
	{
		Kprintf("lsm2: Error loading module %s %08X\n", name, mod);
		return mod;
	}

	mod = sceKernelStartModule(mod, args, argp, NULL, NULL);
	if (mod < 0)
	{
		Kprintf("lsm2: Error starting module %s %08X\n", name, mod);
		return mod;
	}

	return mod;
}

#ifdef MODULE_INTERNAL_THREAD_NAME
SceUID FindThreadByName(const char* name)
{
	SceUID ids[100];
	int ret;
	int count;

	memset(ids, 0, 100 * sizeof(SceUID));
	ret = sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, ids, 100, &count);
	if (ret >= 0)
	{
		for (int i = 0; i < count; i++)
		{
			SceKernelThreadInfo info;
			memset(&info, 0, sizeof(info));
			info.size = sizeof(info);
			ret = sceKernelReferThreadStatus(ids[i], &info);
			if (ret == 0)
			{
				if (strcmp(name, info.name) == 0)
					return ids[i];
			}
		}
	}
	return 0;
}
#endif

int _main(SceSize args, void *argp)
{
	SceUID modid;
	char path[1024];
	char* slash;

	// this is crucial -- we MUST wait for the game module to *load* but not yet start!
	// timing is important here because we want to avoid any lockups, crashes or other weird behavior!
	while(sceKernelFindModuleByName(MODULE_NAME_INTERNAL) == NULL) sceKernelDelayThread(1);
#ifdef MODULE_INTERNAL_THREAD_NAME
	Kprintf(MODULE_NAME ": Searching thread: " MODULE_INTERNAL_THREAD_NAME);
	SceUID thid = 0;
	do
	{
		thid = FindThreadByName(MODULE_INTERNAL_THREAD_NAME);
		if (thid != 0)
			break;
		sceKernelDelayThread(1);
	} while (thid == 0);
	Kprintf(MODULE_NAME ": Suspending thread: " MODULE_INTERNAL_THREAD_NAME);
	sceKernelSuspendThread(thid);
#endif
	// take the path from the argp
	strcpy(path, (const char*)argp);
	slash = strrchr(path, '/');
	if (slash != NULL)
	{
		slash++;
		*slash = 0;
		strcat(path, MODULE_BOOT_TARGET);
	}
	else
		// fallback
		strcpy(path, "ms0:/seplugins/" MODULE_BOOT_TARGET);

	modid = load_start_module2(path, args, argp, 1);
	if(modid < 0)
		Kprintf("Failed to Load/Start module '%s' Error: 0x%08X\n", path, modid);

#ifdef MODULE_INTERNAL_THREAD_NAME
	Kprintf(MODULE_NAME ": Resuming thread " MODULE_INTERNAL_THREAD_NAME);
	sceKernelResumeThread(thid);
#endif
	sceKernelSelfStopUnloadModule(1, 0, NULL);

	return 0;
}

int module_start(SceSize args, void* argp)
{
	SceUID uid;

	// spin up a thread to avoid locking up the game bootup
	uid = sceKernelCreateThread(BOOTER_THREAD_NAME, _main, 32, 0x10000, 0, 0);
	if(uid < 0)
	{
		return 1;
	}
	sceKernelStartThread(uid, args, argp);
	

    return 0;
}
