#include <monapi.h>
#include <monapi/messages.h>
#include <monapi/CString.h>
#include "FileServer.h"
#include "file.h"
#include "CFile.h"
#include "ISO9660.h"

using namespace MonAPI;

extern bool hasCD;
extern ISO9660* iso;

monapi_cmemoryinfo* ReadFile(const char* file, bool prompt /*= false*/)
{
    CString path = file;
    if (prompt) printf("%s: Reading %s....", SVR, file);

    if (path.startsWith("/CD0"))
    {
        if (!hasCD) return NULL;

        CString cdpath = path.substring(5, path.getLength());
        printf("cdpath %s\n", (const char*) cdpath);
        File* cdfile = iso->GetFile(cdpath);

        if (cdfile == NULL) return NULL;

        monapi_cmemoryinfo* ret = monapi_cmemoryinfo_new();
        if (!monapi_cmemoryinfo_create(ret, cdfile->GetSize() + 1, prompt))
        {
            monapi_cmemoryinfo_delete(ret);
            return NULL;
        }

        cdfile->Seek(0, SEEK_SET);
        ret->Size--;
        cdfile->Read(ret->Data, ret->Size);
        ret->Data[ret->Size] = 0;
        if (prompt) printf("OK\n");
        return ret;
    }
    else
    {
        FileInputStream fis(file);
        if (fis.open() != 0)
        {
            if (prompt) printf("ERROR\n");
            return NULL;
        }

        monapi_cmemoryinfo* ret = monapi_cmemoryinfo_new();
        if (!monapi_cmemoryinfo_create(ret, fis.getFileSize() + 1, prompt))
        {
            monapi_cmemoryinfo_delete(ret);
            return NULL;
        }

        ret->Size--;
        fis.read(ret->Data, ret->Size);
        fis.close();
        ret->Data[ret->Size] = 0;
        if (prompt) printf("OK\n");
        return ret;
    }
}

monapi_cmemoryinfo* ReadDirectory(const char* path, bool prompt /*= false*/)
{
    if (syscall_cd(path))
    {
        if (prompt) printf("%s: ERROR: directory not found: %s\n", SVR, path);
        return NULL;
    }
    if (syscall_dir_open())
    {
        if (prompt) printf("%s: ERROR: can not open directory: %s\n", SVR, path);
        return NULL;
    }

    HList<monapi_directoryinfo*> files;
    monapi_directoryinfo di;
    while (syscall_dir_read(di.name, &di.size, &di.attr) == 0)
    {
        files.add(new monapi_directoryinfo(di));
    }
    syscall_dir_close();
    syscall_cd("/");

    monapi_cmemoryinfo* ret = monapi_cmemoryinfo_new();
    int size = files.size();
    if (!monapi_cmemoryinfo_create(ret, sizeof(int) + size * sizeof(monapi_directoryinfo), prompt))
    {
        monapi_cmemoryinfo_delete(ret);
        while (files.size() > 0) delete files.removeAt(0);
        return NULL;
    }

    memcpy(ret->Data, &size, sizeof(int));
    monapi_directoryinfo* p = (monapi_directoryinfo*)&ret->Data[sizeof(int)];
    while (files.size() > 0)
    {
        monapi_directoryinfo* di2 = files.removeAt(0);
        memcpy(p, di2, sizeof(monapi_directoryinfo));
        delete di2;
        p++;
    }
    return ret;
}
