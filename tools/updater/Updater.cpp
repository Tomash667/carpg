#include <CarpgLibCore.h>
#include <WindowsIncludes.h>
#include <conio.h>
#include <File.h>
#include <Crc.h>
#include "Updater.h"
#include "Pak.h"

bool DoWork(cstring exe)
{
    if(!exe)
        exe = "carpg.exe";

    // get current version
    int currentVer = GetCurrentVersion(exe);
    if(currentVer == -1)
    {
        printf("ERROR: Failed to read current version (carpg.exe missing?).\n");
        return false;
    }
    printf("Current version: %s\n", VersionToString(currentVer));

    // get updates
    printf("Checking for updates...\n");
    vector<Update> updates;
    if(!GetUpdate(currentVer, updates))
    {
        printf("ERROR: Failed to get update information.\n");
        return false;
    }
    if(updates.empty())
    {
        printf("Version up to date.\n");
        return false;
    }
    int newVer = updates[updates.size() - 1].ver;
    printf("Available version: %s\n", VersionToString(newVer));

    // for each update
    for(Update& update : updates)
    {
        // download file
        string filename = io::FilenameFromPath(update.path);
        if(!DownloadFile(update.path, filename, update.crc))
            return false;

        // extract files
        printf("Extracting files from %s.\n", filename.c_str());
        Pak* pak = new Pak;
        if(!pak->Open(filename))
        {
            printf("ERROR: Invalid PAK or corrupted file.\n");
            delete pak;
            return false;
        }
        pak->Extract();
        delete pak;

        // install scripts
        printf("Checking for install script.\n");
        RunInstallScripts();
        DeleteFileA(filename.c_str());
        printf(Format("Update to %s done.\n", VersionToString(update.ver)));
    }

    printf("Updated carpg succesfuly!\n");
    return true;
}

int main(int argc, char** argv)
{
    printf("CaRpg auto-updater v0\n");
    printf("==================================================\n");
    cstring exe = (argc == 2 ? argv[1] : nullptr);
    bool ok = DoWork(exe);
    if(ok && exe)
    {
        // start game
        STARTUPINFO si = { 0 };
        si.cb = sizeof(STARTUPINFO);
        PROCESS_INFORMATION pi = { 0 };
        CreateProcess(nullptr, (char*)exe, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
    }
    else
    {
        printf("\n\n(ok)");
        _getch();
    }
    return 0;
}
