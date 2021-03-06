#include <monapi.h>
#include <monapi/CString.h>
#include <monapi/messages.h>
#include <monapi/Array.h>
#include <monalibc.h>
#include "ProcessServer.h"
#include "ProcessManager.h"

using namespace MonAPI;

static int ExecuteProcess(uint32_t parent, SharedMemory& shm, uint32_t entryPoint, const CString& path, const CString& name, CommandOption* option, bool prompt, uint32_t stdin_id, uint32_t stdout_id, uint32_t* tid)
{
    LoadProcessInfo info;
    info.image = shm.data();
    info.size  = shm.size();
    info.entrypoint = entryPoint;
    info.name = name;
    info.list = option;

    addProcessInfo(name);
    intptr_t ret = syscall_load_process_image(&info);
    *tid = addProcessInfo(parent, name, path, stdin_id, stdout_id);
    if (prompt)
    {
        switch(ret)
        {
            case 4:
                  monapi_warn("%s: Shared Memory error1", SVR);
                  break;
            case 5:
                  monapi_warn("%s: Shared Memory error2", SVR);
                  break;
        }
    }
    return ret;
}

static CString GetFileName(const CString& path)
{
    int p = path.lastIndexOf('/');
    if (p < 0) return path;

    p++;
    return path.substring(p, path.getLength() - p);
}

static int ExecuteFile(uint32_t parent, const CString& commandLine, bool prompt, uint32_t stdin_id, uint32_t stdout_id, uint32_t* tid)
{
    /* list initilize */
    CommandOption list;
    list.next = NULL;
    CommandOption* option = NULL;
    CString path;
    _A<CString> args = commandLine.split(' ');
    FOREACH (CString, arg, args)
    {
        if (arg == NULL) continue;

        if (path == NULL)
        {
            path = arg.toUpper();
            continue;
        }
        option = new CommandOption;
        strncpy(option->str, arg, sizeof(option->str));
        option->next = list.next;
        list.next = option;
    }
    END_FOREACH
    uint64_t s2 = MonAPI::Date::nowInMsec();
    SharedMemory* shm = NULL;
    uint32_t entryPoint = 0xa0000000;
    int result = 1;
    const char* svr_id = NULL;
    uint64_t s3, s4;
    if (path.endsWith(".ELF") || path.endsWith(".EL2") || path.endsWith(".EL5"))
    {
        svr_id = "/servers/elf";
    }
    else if (path.endsWith(".EXE") || path.endsWith(".EX2") || path.endsWith(".EX5"))
    {
        svr_id = "/servers/pe";
    }
    if (svr_id != NULL)
    {
        MessageInfo msg;
        uint32_t tid;
        if (monapi_name_whereis(svr_id, tid) != M_OK) {
            monapi_fatal("server not found");
        }
        if (Message::sendReceive(&msg, tid, MSG_PROCESS_CREATE_IMAGE, prompt ? MONAPI_TRUE : MONAPI_FALSE, 0, 0, path) != M_OK) {
            _printf("Error %s:%d\n", __FILE__, __LINE__);
            exit(-1);
        }
        if (msg.arg2 != 0) {
            result = 0;
            entryPoint = msg.arg3;
            shm = new SharedMemory(msg.arg2, atoi(msg.str));
            if (shm->map(true) != M_OK) {
                monapi_fatal("Error %s:%d\n", __FILE__, __LINE__);
                exit(-1);
            }
            // notify map is done.
            int status = Message::reply(&msg);
            if (status != M_OK) {
                monapi_warn("%s reply failed : %s\n", __func__, monapi_error_string(status));
            }

        }
        else
        {
            result = msg.arg3;
        }
    }
    else if (path.endsWith(".BN2"))
    {
        shm = monapi_call_file_decompress_bz2_file(path, prompt ? MONAPI_TRUE : MONAPI_FALSE);
    }
    else if (path.endsWith(".BN5"))
    {
        shm = monapi_call_file_decompress_st5_file(path, prompt ? MONAPI_TRUE : MONAPI_FALSE);
    }
    else
    {
        shm = monapi_file_read_all(path);
    }

    if (shm == NULL)
    {
        return result;
    }
    else
    {
        result = ExecuteProcess(parent, *shm, entryPoint, path, GetFileName(path), &list, prompt, stdin_id, stdout_id, tid);
        delete shm;
    }
    CommandOption* next;
    for (option = list.next; option; option = next)
    {
        next = option->next;
        delete option;
    }
    return result;
}

static void MessageLoop()
{
    for (MessageInfo msg;;)
    {
        if (Message::receive(&msg) != 0) continue;

        switch (msg.header)
        {
            case MSG_PROCESS_EXECUTE_FILE:
            {
                uint32_t tid = 0;
                int result = ExecuteFile(msg.from, msg.str, msg.arg1 != 0, msg.arg2, msg.arg3, &tid);
                Message::reply(&msg, result, tid);
                break;
            }
            default:
                if (processHandler(&msg)) break;
                break;
        }
    }
}

int main(int argc, char* argv[])
{
    initCommonParameters();

    if (monapi_notify_server_start("INIT") != M_OK) {
        exit(-1);
    }

    if (monapi_name_add("/servers/process") != M_OK) {
        monapi_fatal("monapi_name_add failed");
    }
    MessageLoop();

    return 0;
}
