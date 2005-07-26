#include <monapi/messages.h>
#include <monapi/Keys.h>
#ifdef USE_MONAFORMS
#include <gui/messages.h>
#endif

#include "Shell.h"

using namespace MonAPI;

/*----------------------------------------------------------------------
    Shell
----------------------------------------------------------------------*/
Shell::Shell(bool callAutoExec)
    : position(0), hasExited(false), callAutoExec(callAutoExec), doExec(false),
     waiting(THREAD_UNKNOWN), prevX(0), prevY(0), firstTimeOfCD0(true)
{
    this->driveLetter[DRIVE_FD0]    = "fd0:";
    this->driveLetter[DRIVE_CD0]    = "cd0:";
    this->startDirectory[DRIVE_FD0] = "/APPS";
    this->startDirectory[DRIVE_CD0] = "/APPS";

    this->currentDrive = monapi_call_get_current_drive();
    if (this->currentDrive == DRIVE_NONE)
    {
        printf("Server can't get current drive!!\n");
    }

    /* current drive */
    this->currentDirectory.Alloc(2);

    changeDirecotory(startDirectory[currentDrive]);

    if (this->callAutoExec)
    {
        this->executeMSH("/AUTOEXEC.MSH");
        this->callAutoExec = false;
    }

    this->self = syscall_get_tid();

    this->printPrompt("\n");
    this->drawCaret();
}

Shell::~Shell()
{
}

void Shell::run()
{
    MessageInfo msg;
    while (!this->hasExited)
    {
        if (Message::receive(&msg) != 0) continue;

#if 0  /// DEBUG for message
        if ((msg.header == MSG_RESULT_OK && msg.arg1 == MSG_PROCESS_STDOUT_DATA) || msg.header == MSG_PROCESS_STDOUT_DATA)
        {
            char buf[128];
            sprintf(buf, "**** INVALID MESSAGE!! ****[%d: %d, %d]\n", syscall_get_tid(), msg.header, msg.arg1);
            syscall_print(buf);
            for (;;);
        }
#endif
        switch (msg.header)
        {
            case MSG_KEY_VIRTUAL_CODE:
                if (!this->doExec && (msg.arg2 & KEY_MODIFIER_DOWN) != 0)

                {
                    this->onKeyDown(msg.arg1, msg.arg2);
                }
                break;
#ifdef USE_MONAFORMS
            case MSG_GUISERVER_KEYDOWN:
                this->onKeyDown(msg.arg2, msg.arg3);
                break;
#endif

            case MSG_PROCESS_TERMINATED:
                if (this->waiting == msg.arg1)
                {
                    this->printPrompt("\n");
                    this->drawCaret();
                    this->waiting = THREAD_UNKNOWN;
                    this->doExec = false;
                }

                break;
        }
    }
}

void Shell::backspace()
{
    if (this->position == 0)
    {
        /* donothing */
        return;
    }

    this->checkCaretPosition();

    int x, y;
    syscall_get_cursor(&x, &y);
    syscall_set_cursor(x - 1, y);
    printf("  ");
    syscall_set_cursor(x - 1, y);
    this->drawCaret();

    /* backspace */
    this->position--;
}

void Shell::commandChar(char c)
{
    if (c != '\0')
    {
        if (!this->doExec)
        {
            this->checkCaretPosition();
            int x, y;
            syscall_get_cursor(&x, &y);
            x++;
            if ((x + 1) * FONT_WIDTH >= this->screen.getWidth()) return;
        }
        printf("%c", c);
        this->drawCaret();
    }
    this->commandLine[this->position] = c;
    this->position++;
}

void Shell::commandExecute(bool prompt)
{
    if (prompt) printf("\n");

    _A<CString> args = this->parseCommandLine();
    this->position = 0;
    if (args.get_Length() == 0)
    {
        /* command is empty */
        this->printPrompt();
        return;
    }

    /* internal command */
    int isInternal = isInternalCommand(args[0]);
    if (isInternal != 0)
    {
        bool newline = internalCommandExecute(isInternal, args);
        if (!this->hasExited && prompt)
        {
            this->printPrompt(newline ? "\n" : NULL);
        }
        return;
    }

    this->commandExecute(args);
}

bool Shell::pathHasDriveLetter(const CString& path)
{
    return path.startsWith("CD0:/") || path.startsWith("FD0:/");
}

bool Shell::commandExecute(_A<CString> args)
{
    CString cmdLine;
    CString command = args[0].toUpper();
    if (command[0] == '/' || pathHasDriveLetter(command))
    {
        cmdLine = command;
    }
    else if (command.endsWith(".BIN") || command.endsWith(".BN2") || command.endsWith(".BN5")
        || command.endsWith(".ELF") || command.endsWith(".EL2") || command.endsWith(".EL5")
        || command.endsWith(".EXE") || command.endsWith(".EX2") || command.endsWith(".EX5"))
    {
        cmdLine = APPSDIR"/" + command;
    }
    else if (command.endsWith(".APP"))
    {
        CString name = command.substring(0, command.getLength() - 4);
        cmdLine = APPSDIR"/" + name + ".APP/" + name + ".EX2";
    }
    else
    {
        CString cmd2 = command + ".";
        for (int i = 0; i < this->apps[currentDrive].size(); i++)
        {
            CString file = apps[currentDrive].get(i);
            if (file == command + ".APP")
            {
                cmdLine = APPSDIR"/" + file + "/" + command + ".EX2";
                break;
            }
            else if (file.startsWith(cmd2))
            {
                cmdLine = APPSDIR"/" + file;
                break;
            }
        }
        if (cmdLine == NULL)
        {
            printf("Can not find command.\n");
            if (this->waiting == THREAD_UNKNOWN) this->printPrompt("\n");
            return false;;
        }
    }

    if (cmdLine.endsWith(".MSH"))
    {
        this->executeMSH(cmdLine);
        if (this->waiting == THREAD_UNKNOWN) this->printPrompt("\n");
        return true;
    }

    for (int i = 1; i < args.get_Length(); i++)
    {
        cmdLine += ' ';
        cmdLine += args[i];
    }

    dword tid;
    int result = monapi_call_process_execute_file_get_tid(cmdLine, MONAPI_TRUE, &tid, this->self);

    if (!this->callAutoExec && result == 0 && !this->doExec)
    {
        this->waiting = tid;
    }
    else
    {
        this->printPrompt("\n");
    }

    return result == 0;
}

void Shell::commandTerminate()
{
    commandChar('\0');
}

void Shell::onKeyDown(int keycode, int modifiers)
{
    if (!this->doExec && this->waiting != THREAD_UNKNOWN)
    {
        this->printPrompt("\n");
        this->waiting = THREAD_UNKNOWN;
        if (keycode == Keys::Enter) return;
    }

    switch(keycode) {
    case(Keys::A):
    case(Keys::B):
    case(Keys::C):
    case(Keys::D):
    case(Keys::E):
    case(Keys::F):
    case(Keys::G):
    case(Keys::H):
    case(Keys::I):
    case(Keys::J):
    case(Keys::K):
    case(Keys::L):
    case(Keys::M):
    case(Keys::N):
    case(Keys::O):
    case(Keys::P):
    case(Keys::Q):
    case(Keys::R):
    case(Keys::S):
    case(Keys::T):
    case(Keys::U):
    case(Keys::V):
    case(Keys::W):
    case(Keys::X):
    case(Keys::Y):
    case(Keys::Z):
    case(Keys::Decimal):
    case(Keys::D0):
    case(Keys::D1):
    case(Keys::D2):
    case(Keys::D3):
    case(Keys::D4):
    case(Keys::D5):
    case(Keys::D6):
    case(Keys::D7):
    case(Keys::D8):
    case(Keys::D9):
    case(Keys::NumPad1):
    case(Keys::NumPad2):
    case(Keys::NumPad3):
    case(Keys::NumPad4):
    case(Keys::NumPad5):
    case(Keys::NumPad6):
    case(Keys::NumPad7):
    case(Keys::NumPad8):
    case(Keys::NumPad9):
    case(Keys::NumPad0):
    case(Keys::Subtract):
    case(Keys::Add):
    case(Keys::Space):
    case(Keys::Divide):
    case(Keys::OemPeriod):
    case(Keys::OemQuestion):
    case(Keys::OemMinus):
    case(Keys::OemBackslash):
    case(Keys::OemSemicolon):
    case(Keys::Oemplus):
        KeyInfo key;
        key.keycode = keycode;
        key.modifiers = modifiers;
        this->commandChar(Keys::ToChar(key));
        break;
    case(Keys::Enter):
        this->drawCaret(true);
        this->commandTerminate();
        this->commandExecute(true);
        if (this->waiting == THREAD_UNKNOWN && !this->hasExited) this->drawCaret();
        break;

    case(Keys::Up):

        break;
    case(Keys::Down):

        break;
    case(Keys::Left):

        break;
    case(Keys::Right):

        break;
    case(Keys::Back):
        backspace();
        break;
    default:
        break;
    }
}

_A<CString> Shell::parseCommandLine()
{
    _A<CString> args = CString(this->commandLine).split(' ');
    int size = 0;
    FOREACH (CString, arg, args)
    {
        if (arg != NULL) size++;
    }
    END_FOREACH

    _A<CString> ret(size);
    int i = 0;
    FOREACH (CString, arg, args)
    {
        if (arg == NULL) continue;

        ret[i++] = arg;
    }
    END_FOREACH

    return ret;
}

int Shell::makeApplicationList()
{
    if (apps[currentDrive].size() > 0) return 0;

    monapi_cmemoryinfo* mi = monapi_call_file_read_directory(APPSDIR, MONAPI_TRUE);

    int size = *(int*)mi->Data;
    if (mi == NULL || size == 0)
    {
        return 1;
    }

    monapi_directoryinfo* p = (monapi_directoryinfo*)&mi->Data[sizeof(int)];
    for (int i = 0; i < size; i++, p++)
    {
        CString file = p->name;

        if (file.endsWith(".BIN") || file.endsWith(".BN2")
            || file.endsWith(".ELF") || file.endsWith(".EL2")
            || file.endsWith(".EXE") || file.endsWith(".EX2")
            || file.endsWith(".APP") || file.endsWith(".MSH"))
        {
            apps[currentDrive].add(file);
        }
    }

    monapi_cmemoryinfo_dispose(mi);
    monapi_cmemoryinfo_delete(mi);
    return 0;
}

void Shell::printPrompt(const CString& prefix /*= NULL*/)
{
    if (prefix != NULL) printf("%s", (const char*)prefix);
    CString drive = this->driveLetter[this->currentDrive];
    printf("[Mona]%s%s> ", (const char*)drive.toUpper(), (const char*)this->currentDirectory[currentDrive]);
}

CString Shell::getParentDirectory(const CString& dir)
{
    if (dir == NULL || dir == "/") return "/";

    int p = dir.lastIndexOf('/');
    if (p < 1) return "/";

    return dir.substring(0, p);
}

CString Shell::mergeDirectory(const CString& dir1, const CString& dir2)
{
    if (dir2.startsWith("/")) return dir2.toUpper();

    CString ret = dir1;
    _A<CString> dirs = dir2.split('/');
    FOREACH (CString, d, dirs)
    {
        if (d == NULL || d == ".") continue;

        if (d == "..")
        {
            ret = this->getParentDirectory(ret);
        }
        else
        {
            if (ret != "/") ret += '/';
            ret += d.toUpper();
        }
    }
    END_FOREACH
    return ret;
}

void Shell::printFiles(const CString& dir)
{
    monapi_cmemoryinfo* mi = monapi_call_file_read_directory(dir, MONAPI_TRUE);
    int size;
    if (mi == NULL || (size = *(int*)mi->Data) == 0)
    {
        printf("%s: directory not found: %s\n", SVR, (const char*)dir);
        return;
    }

    CString spc = "               ";
    int w = 0, sw = this->screen.getWidth();

    monapi_directoryinfo* p = (monapi_directoryinfo*)&mi->Data[sizeof(int)];
    for (int i = 0; i < size; i++, p++)
    {
        CString file = p->name;

        if ((p->attr & ATTRIBUTE_DIRECTORY) != 0)
        {
            file = "[" + file + "]";
        }

        file = (file + spc).substring(0, 15);
        int fw = FONT_WIDTH * file.getLength();
        if (w + fw >= sw)
        {
            printf("\n");
            w = 0;
        }
        printf("%s", (const char*)file);
        w += fw;
    }
    printf("\n");

    monapi_cmemoryinfo_dispose(mi);
    monapi_cmemoryinfo_delete(mi);
}

void Shell::executeMSH(const CString& msh)
{
    monapi_cmemoryinfo* mi = monapi_call_file_read_data(msh, 1);
    if (mi == NULL) return;

    for (dword pos = 0, start = 0; pos <= mi->Size; pos++)
    {
        char ch = pos < mi->Size ? (char)mi->Data[pos] : '\n';
        if (ch == '\r' || ch == '\n')
        {
            int len = pos - start;
            bool prompt = true;
            if (len > 0)
            {
                if (mi->Data[start] == '@')
                {
                    start++;
                    len--;
                    prompt = false;
                }
            }
            if (len > 0)
            {
                if (len > 127) len = 127;
                strncpy(this->commandLine, (const char*)&mi->Data[start], len);
                this->position = len;
                this->commandLine[len] = '\0';
                if (prompt)
                {
                    this->printPrompt("\n");
                    printf("%s", this->commandLine);
                }

                this->commandExecute(prompt);
            }
            start = pos + 1;
        }
    }

    monapi_cmemoryinfo_dispose(mi);
    monapi_cmemoryinfo_delete(mi);
}

void Shell::drawCaret(bool erase /*= false*/)
{
    if (this->doExec) return;

    int x, y;
    syscall_get_cursor(&x, &y);
    this->screen.fillRect16(x * FONT_WIDTH, y * FONT_HEIGHT + FONT_HEIGHT - 2, 8, 2,
        erase ? BACKGROUND : FOREGROUND);
    this->prevX = x;
    this->prevY = y;
}

void Shell::checkCaretPosition()
{
    int x, y;
    syscall_get_cursor(&x, &y);
    if (this->prevX == x && this->prevY == y) return;

    monapi_call_mouse_set_cursor(0);
    this->printPrompt(this->prevX == 0 ? NULL : "\n");
    this->commandLine[this->position] = '\0';
    printf(this->commandLine);
    monapi_call_mouse_set_cursor(1);
}

void Shell::setCurrentDirectory()
{
    char buff[128];
    monapi_call_get_current_directory(buff);
    logprintf("buf=%s\n", buff);
    this->currentDirectory[this->currentDrive] = buff;
}

bool Shell::changeDirecotory(const MonAPI::CString& path)
{
    logprintf("1\n");
    if (monapi_call_change_directory(path) == MONA_FAILURE)
    {
    logprintf("2\n");
        return false;
    }
    logprintf("3\n");
    setCurrentDirectory();
    logprintf("4\n");
    makeApplicationList();
    return true;
}