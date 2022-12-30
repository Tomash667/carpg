#include <CarpgLibCore.h>
#include <WindowsIncludes.h>
#include <conio.h>
#include <File.h>
#include <Crc.h>
#include "Updater.h"
#include "Pak.h"

bool DoWork(cstring exeInput, int resumeVer)
{
    cstring exe = exeInput;
    if(!exe)
        exe = "carpg.exe";

    if(resumeVer != 0)
    {
        printf("Updater update finished.\n");
        io::DeleteFile("updater.exe~");
        RunInstallScripts();
        printf(Format("Update to %s done.\n", VersionToString(resumeVer)));
    }

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
        if(resumeVer != 0)
        {
            printf("Updated carpg succesfuly!\n");
            return true;
        }
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

        bool restart = false;
        if(pak->HaveFile("updater.exe"))
        {
            // can't replace itself, need to restart new updater
            io::MoveFile("updater.exe", "updater.exe~");
            restart = true;
        }

        // extract files
        pak->Extract();
        delete pak;
        io::DeleteFile(filename.c_str());

        // restart if needed
        if(restart)
        {
            printf("Restarting updater.\n");
            STARTUPINFO si{};
            si.cb = sizeof(STARTUPINFO);
            PROCESS_INFORMATION pi{};
            string cmdLine = Format("updater.exe -resume %d", update.ver);
            if(exeInput)
            {
                cmdLine += " ";
                cmdLine += exeInput;
            }
            CreateProcess(nullptr, (char*)cmdLine.c_str(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
            exit(0);
        }

        // install scripts
        printf("Checking for install script.\n");
        RunInstallScripts();
        printf(Format("Update to %s done.\n", VersionToString(update.ver)));
    }

    printf("Updated carpg succesfuly!\n");
    return true;
}

int main(int argc, char** argv)
{
    printf("CaRpg auto-updater v1\n");
    printf("==================================================\n");

    cstring exe = nullptr;
    int resumeVer = 0;
    for(int i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], "-resume") == 0)
        {
            ++i;
            if(i < argc)
                resumeVer = atoi(argv[i]);
        }
        else
            exe = argv[i];
    }

    bool ok = DoWork(exe, resumeVer);
    if(ok && exe)
    {
        // start game
        STARTUPINFO si{};
        si.cb = sizeof(STARTUPINFO);
        PROCESS_INFORMATION pi{};
        CreateProcess(nullptr, (char*)exe, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
    }
    else
    {
        printf("\n\n(ok)");
        _getch();
    }
    return 0;
}
