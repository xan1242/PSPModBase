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

//int build_args(char *args, const char *execfile, int argc, char **argv)
//{
//	int loc = 0;
//	int i;
//
//	strcpy(args, execfile);
//	loc += strlen(execfile) + 1;
//	for(i = 0; i < argc; i++)
//	{
//		strcpy(&args[loc], argv[i]);
//		loc += strlen(argv[i]) + 1;
//	}
//
//	return loc;
//}

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

// we do not need these variants from psplink
// int load_start_module(const char *name, int argc, char **argv)
// {
// 	SceUID modid;
// 	int status;
// 	char args[1024];
// 	int len;
// 
// 	modid = load_module(name, 0, 0);
// 	if(modid >= 0)
// 	{
// 		len = build_args(args, name, argc, argv);
// 		modid = sceKernelStartModule(modid, len, (void *) args, &status, NULL);
// 	}
// 	else
// 	{
// 		Kprintf("lsm: Error loading module %s %08X\n", name, modid);
// 	}
// 
// 	return modid;
// }

int load_start_module2(const char *name, SceSize args, void *argp, int type)
{
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

int _main(SceSize args, void *argp)
{
	SceUID modid;
	char path[1024];
	char* slash;

	// this is crucial -- we MUST wait for the game module to *load* but not yet start!
	// timing is important here because we want to avoid any lockups, crashes or other weird behavior!
	while(sceKernelFindModuleByName(MODULE_NAME_INTERNAL) == NULL) sceKernelDelayThread(1);

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
	
	
	sceKernelSelfStopUnloadModule(1, 0, NULL);
	sceKernelDelayThread(2000000);
	sceKernelExitGame();

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
