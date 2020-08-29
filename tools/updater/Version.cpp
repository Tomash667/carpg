#include <CarpgLibCore.h>
#include <WindowsIncludes.h>
#include "Updater.h"

int GetCurrentVersion(cstring exe)
{
    DWORD dwHandle;
    uint size = GetFileVersionInfoSize(exe, &dwHandle);
    if(size == 0u)
        return -1;

    Ptr<byte> data(new byte[size]);
    uint result = GetFileVersionInfo(exe, 0, size, data);
    if(!result)
        return -1;

    uint varSize;
    void* var;
    result = VerQueryValue(data, "\\", &var, &varSize);
    if(!result || !varSize)
        return -1;

    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)var;
    if(verInfo->dwSignature == 0xfeef04bd)
    {
        // windows store version part as 16 bits, but we only use 8 bits...
        int major = (verInfo->dwFileVersionMS >> 16) & 0xFF;
        int minor = verInfo->dwFileVersionMS & 0xFF;
        int patch = (verInfo->dwFileVersionLS >> 16) & 0xFF;
        return (major << 16) | (minor << 8) | patch;
    }
    else
        return -1;
}

cstring VersionToString(int version)
{
#define GET_MAJOR(x) (((x)&(0xFF<<16))>>16)
#define GET_MINOR(x) (((x)&(0xFF<<8))>>8)
#define GET_PATCH(x) ((x)&0xFF)

    uint major = GET_MAJOR(version),
        minor = GET_MINOR(version),
        patch = GET_PATCH(version);

    if(patch == 0)
        return Format("%u.%u", major, minor);
    else
        return Format("%u.%u.%u", major, minor, patch);
}
