#include "Shell.h"
#include <monapi/messages.h>

using namespace MonAPI;

enum
{
    COMMAND_NONE,
    COMMAND_HELP,
    COMMAND_LS,
    COMMAND_CD,
    COMMAND_CAT,
    COMMAND_CHSH,
    COMMAND_UNAME,
    COMMAND_ECHO,
    COMMAND_CLEAR,
    COMMAND_PS,
    COMMAND_KILL,
    COMMAND_EXEC,
    COMMAND_CHANGE_DRIVE_CD0,
    COMMAND_CHANGE_DRIVE_FD0,
};

int Shell::isInternalCommand(const CString& command)
{
    CString cmd = command.toLower();
    if (cmd == "help" || cmd == "?")
    {
        return COMMAND_HELP;
    }
    else if (cmd == "ls" || cmd == "dir")
    {
        return COMMAND_LS;
    }
    else if (cmd == "cd")
    {
        return COMMAND_CD;
    }
    else if (cmd == "cat" || cmd == "type")
    {
        return COMMAND_CAT;
    }
    else if (cmd == "chsh")
    {
        return COMMAND_CHSH;
    }
    else if (cmd == "uname" || cmd == "ver")
    {
        return COMMAND_UNAME;
    }
    else if (cmd == "echo")
    {
        return COMMAND_ECHO;
    }
    else if (cmd == "clear" || cmd == "cls")
    {
        return COMMAND_CLEAR;
    }
    else if (cmd == "ps")
    {
        return COMMAND_PS;
    }
    else if (cmd == "kill")
    {
        return COMMAND_KILL;
    }
    else if (cmd == "exec")
    {
        return COMMAND_EXEC;
    }
    else if (cmd == this->driveLetter[DRIVE_CD0])
    {
        return COMMAND_CHANGE_DRIVE_CD0;
    }
    else if (cmd == this->driveLetter[DRIVE_FD0])
    {
        return COMMAND_CHANGE_DRIVE_FD0;
    }

    return COMMAND_NONE;
}

bool Shell::internalCommandExecute(int command, _A<CString> args)
{
    switch (command)
    {
    case COMMAND_HELP:
        printf("* Mona Shell Internal Commands\n");
        printf("HELP/?, LS/DIR, CD, CAT/TYPE, CHSH, UNAME/VER, ECHO, CLEAR/CLS, PS, KILL, EXEC, FD0:, CD0:\n");
        break;

    case COMMAND_CD:
        {
            CString dir;

            if (args.get_Length() < 2)
            {
                dir = this->startDirectory[this->currentDrive];
            }
            else
            {
                dir = this->mergeDirectory(this->currentDirectory[currentDrive], args[1]);
            }

            if (!changeDirecotory(dir))
            {
                printf("%s: directory not found: %s\n", SVR, (const char*)dir);
            }

            break;
        }

    case COMMAND_LS:
        {
            if (args.get_Length() < 2)
            {
                printFiles(this->currentDirectory[currentDrive]);
            }
            else
            {
                for (int i = 1; i < args.get_Length(); i++)
                {
                    if (i > 1) printf("\n");
                    CString dir = this->mergeDirectory(this->currentDirectory[currentDrive], args[i]);
                    printf("%s:\n", (const char*)dir);
                    printFiles(dir);
                }
            }
            break;
        }

    case COMMAND_CAT:
        {
            if (args.get_Length() < 2)
            {
                printf("usage: CAT/TYPE file\n");
                break;
            }

            monapi_cmemoryinfo* mi = monapi_call_file_read_data(args[1], 1);
            if (mi == NULL) break;

            if (mi->Size > 0)
            {
                byte* p = mi->Data;
                bool cr = false;
                for (dword i = 0; i < mi->Size; i++)
                {
                    byte b = mi->Data[i];
                    switch (b)
                    {
                        case '\r':
                            *p++ = '\n';
                            cr = true;
                            break;
                        case '\n':
                            if (!cr) *p++ = '\n';
                            cr = false;
                            break;
                        default:
                            *p++ = b;
                            cr = false;
                            break;
                    }
                }
                *p = 0;
                printf((const char*)mi->Data);
            }
            monapi_cmemoryinfo_dispose(mi);
            monapi_cmemoryinfo_delete(mi);
            break;
        }

    case COMMAND_CHSH:
        if (monapi_call_process_execute_file("/SERVERS/1LINESH.EX5", MONAPI_TRUE) != 0) break;
//        if (syscall_load_process("/SERVERS/SHELL.BIN", "SHELL.BIN", NULL) != 0) break;
#if 1
        for (MessageInfo msg;;)
        {
            if (Message::receive(&msg) != 0) continue;
            if (msg.header == MSG_SERVER_START_OK) break;
        }
#endif
        hasExited = true;
        break;

    case COMMAND_UNAME:
        {
             char ver[128];
             syscall_get_kernel_version(ver, 128);
             ver[127] = '\0';
             printf("%s\n", ver);
            break;
        }

    case COMMAND_ECHO:
        {
            int len = 0;
            for (int i = 1; i < args.get_Length(); i++)
            {
                if (i > 1) len++;
                len += args[i].getLength();
            }
            char* buf = new char[len + 1], * p = buf;
            for (int i = 1; i < args.get_Length(); i++)
            {
                if (i > 1) *p++ = ' ';
                sprintf(p, args[i]);
                p += args[i].getLength();
            }
            *p = '\0';
            printf("%s\n", buf);
            delete [] buf;
            break;
        }

    case COMMAND_CLEAR:
        if (this->doExec)
        {
            printf("can not clear while exec\n");
            break;
        }
        else
        {
            monapi_call_mouse_set_cursor(0);
            syscall_clear_screen();
            monapi_call_mouse_set_cursor(1);
            syscall_set_cursor(0, 0);
        }
        return false;

    case COMMAND_PS:
        {
            syscall_set_ps_dump();
            PsInfo info;

            printf("[tid] [state]  [eip]    [esp]    [cr3]    [name]\n");

            char buf[256];
            while (syscall_read_ps_dump(&info) == 0)
            {
                sprintf(buf, "%5d %s %08x %08x %08x %s\n",
                    info.tid, info.state ? "running" : "waiting",
                    info.eip, info.esp, info.cr3, info.name);
                printf(buf);
            }

            break;
        }

    case COMMAND_KILL:
        {
            if (args.get_Length() < 2)
            {
                printf("usage: KILL tid\n");
                break;
            }

            if (syscall_kill_thread(atoi(args[1])))
            {
                printf("kill failed. Thread not found\n");
            }
            else
            {
                printf("thread %d killed\n", atoi(args[1]));
            }

            break;
        }

    case COMMAND_EXEC:
        if (args.get_Length() < 2)
        {
            printf("usage: EXEC command [arguments ...]\n");
        }
        else if (this->doExec)
        {
            printf("can not exec while exec\n");
        }
        else
        {
            _A<CString> args2(args.get_Length() - 1);
            for (int i = 1; i < args.get_Length(); i++) args2[i - 1] = args[i];
            if (this->commandExecute(args2)) this->doExec = true;
        }
        break;

    case COMMAND_CHANGE_DRIVE_CD0:
    {
        if (monapi_call_change_drive(DRIVE_CD0, MONAPI_FALSE) == MONA_FAILURE)
        {
            printf("change drive error\n");
            break;
        }
        else
        {
            this->currentDrive = DRIVE_CD0;
        }

        if (this->firstTimeOfCD0)
        {
            changeDirecotory(startDirectory[currentDrive]);
            this->firstTimeOfCD0 = false;
        }

        setCurrentDirectory();
        break;
    }

    case COMMAND_CHANGE_DRIVE_FD0:
    {
        if (monapi_call_change_drive(DRIVE_FD0, MONAPI_FALSE) == MONA_FAILURE)
        {
            printf("change drive error\n");
            break;
        }
        else
        {
            this->currentDrive = DRIVE_FD0;
        }

        setCurrentDirectory();
        break;
    }

    default:
        break;
    }

    return true;
}
