//
// PSP userspace module example for modding games
// A module that can be used as a basis to build mods for PSP games and apps
// 
// by xan1242 / Tenjoin 
// 
// This is based on PSP modules from Widescreen Fixes Pack by ThirteenAG (and others!)
// https://github.com/ThirteenAG/WidescreenFixesPack
// 
// NOTE: in order to load this on a CFW PSP, you will need to bootstrap it with a kernel module!
// It will fail to load if the module size is bigger than certain size.
// PPSSPP works fine otherwise.
// 
// NOTE 2: Since this is a userspace plugin, please be aware that for any kernel function,
// you MUST use bridging functions (such as kubridge or sctrl functions)
// You may NOT even *link* against or include any protected kernel functions, or else the module will NOT start!
// 


#include <pspsdk.h>
#include <pspuser.h>
#include <pspctrl.h>
#include <systemctrl.h>
#include <kubridge.h>
#include <stdio.h>
#include <string.h>

// We have to use paths relative to the source file (unless you edit the makefile)
#include "../../includes/psp/injector.h"
#include "../../includes/psp/inireader.h"

// Define the name of the game's main module here
// The easiest way this can be found is by using PPSSPP's debugger
// Example: GTA Vice City Stories and Liberty City Stories both have the module name of "GTA3"
#define MODULE_NAME_INTERNAL "TargetModule"

// This is the name of this module that will be presented to the PSP OS.
// We also use it here as the base name for the ini and log files.
// Best practice is to rename this to match your project name
#define MODULE_NAME "PSPModBase"

#define MODULE_VERSION_MAJOR 1
#define MODULE_VERSION_MINOR 0

// Uncomment for logging
// We use a global definition like so to reduce final binary size (which is very important because PSP is memory constrained!)
//#define LOG

// We ignore Intellisense here to reduce squiggles in VS
#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, 0, MODULE_VERSION_MAJOR, MODULE_VERSION_MINOR);
#endif

// Forward-declare initialization function
int MainInit();

int bPPSSPP = 0;
static STMOD_HANDLER previous;

#define INI_NAME MODULE_NAME ".ini"
char inipath[128] = "ms0:/seplugins/" INI_NAME;

#ifdef LOG
#define LOG_NAME MODULE_NAME ".log"
// Default initialized path
char logpath[128] = "ms0:/seplugins/" LOG_NAME;

//
// A basic printf logger that writes to a file.
//
int logPrintf(const char* text, ...) {
    va_list list;
    char string[512];

    va_start(list, text);
    vsprintf(string, text, list);
    va_end(list);

    SceUID fd = sceIoOpen(logpath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, string, strlen(string));
        sceIoWrite(fd, "\n", 1);
        sceIoClose(fd);
    }

    return 0;
}
#endif

//
// CheckModules
// Executes only once on startup
// This works both on a real PSP and PPSSPP, but we limit this to PPSSPP because OnModuleStart is better
//
static void CheckModules()
{
    SceUID modules[10];
    int count = 0;
    int bFoundMainModule = 0;
    int bFoundInternalModule = 0;
    if (sceKernelGetModuleIdList(modules, sizeof(modules), &count) >= 0)
    {
        int i;
        SceKernelModuleInfo info;
        for (i = 0; i < count; ++i)
        {
            info.size = sizeof(SceKernelModuleInfo);
            if (sceKernelQueryModuleInfo(modules[i], &info) < 0)
            {
                continue;
            }
            if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
            {
#ifdef LOG
                logPrintf("Found module " MODULE_NAME_INTERNAL);
                logPrintf("text_addr: 0x%X\ntext_size: 0x%X", info.text_addr, info.text_size);
#endif
                injector.SetGameBaseAddress(info.text_addr, info.text_size);

                bFoundMainModule = 1;
            }
            else if (strcmp(info.name, MODULE_NAME) == 0)
            {
#ifdef LOG
                logPrintf("PRX module " MODULE_NAME);
                logPrintf("text_addr: 0x%X\ntext_size: 0x%X", info.text_addr, info.text_addr);
#endif
                injector.SetModuleBaseAddress(info.text_addr, info.text_size);

                bFoundInternalModule = 1;
            }
    }
}

    if (bFoundInternalModule)
    {
        if (bFoundMainModule)
        {
            MainInit();
        }
    }

    // Since we can't use OnModuleStart like on a PSP CFW, we have to scan for modules again
    // if we want to intercept another one. Read the note at the bottom of OnModuleStart for more info.

    return;
}


//
// OnModuleStart
// Executes any time a module is started
// This currently only works on PSP CFW, not on PPSSPP
// 
// NOTE: Be very careful with the code you put in here!
// Often times, if you do something bad here, this module will NOT start.
// Usually with the error SCE_KERNEL_ERROR_LIBRARY_NOTFOUND (0x8002013C)
// 
// Example: Calling sceKernelFindModuleByName without elevation here will make the module not start.
// It will load and you'll be able to see it with the 'modlist' command, but it will NOT work.
//
int OnModuleStart(SceModule2* mod) 
{
    char* modname = mod->modname;
#ifdef LOG
    logPrintf("OnModuleStart: %s", modname);
#endif

    // First we search for the target module...
    if (strcmp(modname, MODULE_NAME_INTERNAL) == 0)
    {
#ifdef LOG
        logPrintf("Found module " MODULE_NAME_INTERNAL);
        logPrintf("text_addr: 0x%X\ntext_size: 0x%X", mod->text_addr, mod->text_size);
#endif
        // Then we search for this module...
        // IMPORTANT: we use kuBridge to elevate to kernel permissions!
        SceModule this_module = { 0 };
        int kuErrCode = kuKernelFindModuleByName(MODULE_NAME, &this_module);
#ifdef LOG
        if (kuErrCode == 0)
        {
            logPrintf("PRX module " MODULE_NAME);
            logPrintf("text_addr: 0x%X\ntext_size: 0x%X", this_module.text_addr, this_module.text_size);
        }
#endif

        injector.SetGameBaseAddress(mod->text_addr, mod->text_size);
        if (kuErrCode == 0) // this should NOT fail, or else you can crash the game!
            injector.SetModuleBaseAddress(this_module.text_addr, this_module.text_addr);

        MainInit();
    }

    // You can intercept other modules the same way as MODULE_NAME_INTERNAL and hook in the same way
    // with new initializers and new base addresses.
    // There are some games with separate modules so you will cases where you have to switch around.
    // injector only works with one at a time, so keep that in mind and update the base addresses accordingly!

    if (!previous)
        return 0;
    
    // This passes the call to the next hook that may or may not be there
    return previous(mod);
}

void SetDefaultPaths()
{
    if (bPPSSPP) 
    {
        strcpy(inipath, "ms0:/PSP/PLUGINS/" MODULE_NAME "/" INI_NAME);
#ifdef LOG
        strcpy(logpath, "ms0:/PSP/PLUGINS/" MODULE_NAME "/" LOG_NAME);
#endif
    }
    else 
    { 
        strcpy(inipath, "ms0:/seplugins/" INI_NAME);
#ifdef LOG
        strcpy(logpath, "ms0:/seplugins/" LOG_NAME);
#endif
    }
}

int module_start(SceSize argc, void* argp) 
{
    char* ptr_path;
    // If a kemulator interface exists, we know that we're in an emulator
    if (sceIoDevctl("kemulator:", 0x00000003, NULL, 0, NULL, 0) == 0) 
        bPPSSPP = 1;

    if (argc > 0) 
    { 
        // on real hardware we use module_start's argp path
        // location depending on where prx is loaded from
        strcpy(inipath, (char*)argp);
        ptr_path = strrchr(inipath, '/');
        if (ptr_path)
            strcpy(ptr_path + 1, INI_NAME);
        else
            SetDefaultPaths();
#ifdef LOG
        strcpy(logpath, (char*)argp);
        ptr_path = strrchr(logpath, '/');
        if (ptr_path)
            strcpy(ptr_path + 1, LOG_NAME);
        else
            SetDefaultPaths();
#endif
    }
    else 
    { 
        // no arguments found
        SetDefaultPaths();
    }

    if (bPPSSPP)
        CheckModules(); // scan the modules using normal/official syscalls (https://github.com/hrydgard/ppsspp/pull/13335#issuecomment-689026242)
    else // PSP
        previous = sctrlHENSetStartModuleHandler(OnModuleStart);

    return 0;
}

//
// MainInit
// Put your initialization code here
//
int MainInit() {
#ifdef LOG
    logPrintf(MODULE_NAME " MainInit");
#endif

    /*
    * Here you can use the injector to modify instructions or reroute the code to
    * another place (e.g. a function in this module)
    *
    * Keep in mind that the injector recalculates addresses based on the address
    * you set with SetGameBaseAddress and SetModuleBaseAddress!
    *
    *
    * Example: we can make a call to a function "MyFunction" like so:
    * injector.MakeCALL(0x189560, (uintptr_t)&MyFunction);
    *
    * Example 2: we can write a NOP at any instruction like so:
    * injector.MakeNOP(0x1477DC);
    *
    * For more examples of usage, check WidescreenFixesPack:
    * https://github.com/ThirteenAG/WidescreenFixesPack
    */

    // In this example we'll simply set up the ini reader, read the value from the ini and print it
    // The ini in question in the source here is: data/PSPModBase.ini

    // Set up the inireader path
    inireader.SetIniPath(inipath);

    // And then read & do something with the value...
    int iniValue = inireader.ReadInteger("MAIN", "Value", 0);
    sceKernelPrintf("ini value is: %d\n", iniValue);

    // not really necessary but kept to flush the cache to the disk
    sceKernelDcacheWritebackAll();
    return 0;
}
