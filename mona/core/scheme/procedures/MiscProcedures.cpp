/*!
    \file   MiscProcedures.cpp
    \brief

    Copyright (c) 2002-2007 Higepon.
    All rights reserved.
    License=MIT/X License

    \author  Higepon
    \version $Revision$
    \date   create:2007/07/14 update:$Date$
*/
#include "procedures/Procedure.h"
#include "primitive_procedures.h"
using namespace std;
using namespace monash;

Objects* pairToObjects(Cons* pair)
{
    Objects* objects = new Objects;
#if 1
    Cons* p = pair;
    for (;;)
    {
        Object* car = p->getCar();
        Object* cdr = p->getCdr();
        if (car != NULL)
        {
            objects->add(car);
        }
        if (cdr == NULL) break;
        if (!cdr->isCons()) break;
        p = (Cons*)cdr;
    }
#else
    objects->add(pair);
#endif
    return objects;
}

PROCEDURE(DefineMacro, "define-macro")
{
    CAST(arguments->get(0), Identifier, r);
    Variable* v = new Variable(r->text());
    TraditionalMacro* macro = new TraditionalMacro(Kernel::eval(arguments->get(1), env));
    SCM_ASSERT(macro);
    env->defineVariable(v, macro);
    return SCM_UNDEF;
}


PROCEDURE(DynamicWindProc, "dynamic-wind")
{
    ARGC_SHOULD_BE(3);
    DynamicWind* dynamicWind = new DynamicWind(ARGV(0), ARGV(1), ARGV(2));
    g_dynamic_winds->add(dynamicWind);
    Kernel::applyFullEvaled(ARGV(0), SCM_NO_ARG, env);
    Object* ret = Kernel::applyFullEvaled(ARGV(1), SCM_NO_ARG, env);
    Kernel::applyFullEvaled(ARGV(2), SCM_NO_ARG, env);
    g_dynamic_winds->removeAt(0);
    return ret;
}

// if this fail, see cont.c adjust next_stack= prev_stack - 0x1000;
PROCEDURE(CallWithValues, "call-with-values")
{
    ARGC_SHOULD_BE(2);
    CAST(ARGV(0), Procedure, producer);
    CAST(ARGV(1), Procedure, consumer);
    Object* applyed = Kernel::apply(producer, SCM_NO_ARG, env);
    if (applyed->needEval)
    {
        applyed = Kernel::evalTailOpt(applyed, applyed->env);
    }
    Cons* applyeds = new Cons;
    applyeds->append(applyed);
    return Kernel::apply(consumer, applyeds, env);
}

// if this fail, see cont.c adjust next_stack= prev_stack - 0x1000;
PROCEDURE(CallWithCurrentContinuation, "call-with-current-continuation")
{
    ARGC_SHOULD_BE(1);
    CAST(ARGV(0), Procedure, procedure);
    Continuation* continuation = new Continuation;
    if (0 == cont_save(&(continuation->cont)))
    {
        int wind_size = g_dynamic_winds->size();
        if (wind_size != 0)
        {
           continuation->dynamicWind = g_dynamic_winds->get(0);
           g_dynamic_winds->removeAt(0);
        }
        Cons* arguments = new Cons;
        arguments->append(continuation);
        return Kernel::apply(procedure, arguments, env, false /* don't eval arguments */);
    }
    else
    {
        int wind_size = g_dynamic_winds->size();
        if (wind_size != 0)
        {

            for (int i = wind_size - 1; i >= 0; i--)
            {
                DynamicWind* d = g_dynamic_winds->get(i);
                Kernel::applyFullEvaled(d->after, SCM_NO_ARG, env);
                g_dynamic_winds->removeAt(i);
            }
        }
        else if (continuation->dynamicWind != NULL)
        {
            Kernel::applyFullEvaled(continuation->dynamicWind->before, SCM_NO_ARG, env);
        }

        if (continuation->callAruguments->size() == 1)
        {
            Object* result = Kernel::evalTailOpt(continuation->callAruguments->get(0), env);
            return result;
        }
        else
        {
            return new Values(continuation->callAruguments, lineno());
        }
    }
    RAISE_ERROR(lineno(), "unknown call/cc");
}

PROCEDURE(NotSupported, "not-supported")
{
    ARGC_SHOULD_BE(1);
    CAST(ARGV(0), SString, s);
    RAISE_ERROR(0, "%s not supported\n", s->toStringValue().data());
    RETURN_BOOLEAN(false);
}

PROCEDURE(ProcedureP, "procedure?")
{
    ARGC_SHOULD_BE(1);
    RETURN_BOOLEAN(ARGV(0)->isProcedure());
}

PROCEDURE(BooleanP, "boolean?")
{
    ARGC_SHOULD_BE(1);
    RETURN_BOOLEAN(ARGV(0)->isBoolean());
}

#ifdef USE_MONA_GC
extern "C" {
#include "gc/gc.h"
};
#endif
PROCEDURE(Exit, "exit")
{
#ifdef USE_MONA_GC
    gc();
    gc_fini();
#endif
    exit(-1);
    RETURN_BOOLEAN(false);
    /* NOTREACHED */
}


PROCEDURE(Apply, "apply")
{
    ARGC_SHOULD_BE_GT(1);
    Cons* tmp = new Cons;
    if (ARGC == 2 && ARGV(1)->isNil())
    {
        tmp->append(ARGV(1));
        return Kernel::apply(ARGV(0), tmp, env);
    }
    CAST(ARGV(ARGC - 1), Cons, p);
    Objects* os = pairToObjects(p);

    for(int i = 1; i < ARGC -1; i++)
    {
        tmp->append(ARGV(i));
    }
    for (int i = 0; i < os->size(); i++)
    {
        tmp->append(os->get(i));
    }
    return Kernel::apply(ARGV(0), tmp, env, false);
}

PROCEDURE(Eval, "eval")
{
     Object* sexp = Kernel::eval(arguments->get(0), env);
     CAST(Kernel::eval(arguments->get(1), env), Environment, e);
     return Kernel::eval(sexp, e);
}

PROCEDURE(NullEnvironment, "null-environment")
{
    ARGC_SHOULD_BE(1);
    CAST(ARGV(0), Number, n);
    if (n->value() != 5)
    {
        RAISE_ERROR(lineno(), "%s got wrong version" , toString().data());
    }
    Translator translator;
    return new Environment(lineno());
}

PROCEDURE(SchemeReportEnvironment, "scheme-report-environment")
{
    ARGC_SHOULD_BE(1);
    CAST(ARGV(0), Number, n);
    if (n->value() != 5)
    {
        RAISE_ERROR(lineno(), "%s got wrong version" , toString().data());
    }
    return env;
}

PROCEDURE(InteractionEnvironment, "interaction-environment")
{
    return env;
}

PROCEDURE(CallPipe, "|")
{
#ifdef MONA
    ARGC_SHOULD_BE_GT(1);
    MonAPI::Stream* resultStream = new MonAPI::Stream();
    uint32_t* streamHandles = new uint32_t[ARGC + 1];
    streamHandles[0]    = g_terminal->getScreenHandle();
    streamHandles[ARGC] = resultStream->handle();
    for (int i = 1; i < ARGC; i++)
    {
        streamHandles[i] = (new MonAPI::Stream())->handle();
    }
    uint32_t tid;
    for (int i = 0; i < ARGC; i++)
    {
        CAST(ARGV(i), SString, s);
        int result = monapi_call_process_execute_file_get_tid(s->value().data(), MONAPI_TRUE, &tid, streamHandles[i], streamHandles[i + 1]);
        if (result != 0)
        {
            RAISE_ERROR(lineno(), "system can't execute %s" , s->value().data());
        }
    }

    const uint32_t bufsize = 256;
    uint8_t buf[bufsize];
    ::util::String text;
    for (;;)
    {
        resultStream->waitForRead();
        uint32_t size = resultStream->read(buf, bufsize);
        if (size == 0)
        {
            continue;
        }

        text += ::util::String((char*)buf, size);
        if (size >= 3)
        {
            // bad!
            if ((buf[size - 5] == '^') &&
                (buf[size - 4] == 'E') &&
                (buf[size - 3] == 'O') &&
                (buf[size - 2] == 'P'))
            {
                break;
            }
        }

    }
    env->setVaribale(new Variable("status", lineno()), new Number(monapi_process_wait_terminated(tid), lineno()));
//    monapi_process_wait_terminated(tid);
    return new SString(text, lineno());

#endif
    RETURN_BOOLEAN(false);
}

PROCEDURE(CallProcess, "call-process")
{
#ifdef MONA
    ARGC_SHOULD_BE(1);
    CAST(ARGV(0), SString, s);
    uint32_t tid;
// don't use outStream directory! auto-import trap!
//    int result = monapi_call_process_execute_file_get_tid(s->value().data(), MONAPI_TRUE, &tid, outStream->handle(), outStream->handle());
//    ::MonAPI::Stream* out = ::MonAPI::System::getStdoutStream();
    int result = monapi_call_process_execute_file_get_tid(s->value().data(), MONAPI_TRUE, &tid, g_terminal->getScreenHandle(), g_terminal->getScreenHandle());
    if (result != 0)
    {
        RAISE_ERROR(lineno(), "system can't execute %s" , s->value().data());
    }
    Number* status = new Number(monapi_process_wait_terminated(tid), lineno());
    env->setVaribale(new Variable("status", lineno()), status);
    return status;
#endif
    RETURN_BOOLEAN(false);
}

PROCEDURE(StartProcess, "start-process")
{
#ifdef MONA
    ARGC_SHOULD_BE_BETWEEN(1, 2);
    CAST(ARGV(0), SString, s);
    if (ARGC == 2)
    {
        if (ARGV(1))
        {
            g_terminal->setKeySuppresed();
        }
    }
    uint32_t tid;
// don't use outStream directory! auto-import trap!
//    int result = monapi_call_process_execute_file_get_tid(s->value().data(), MONAPI_TRUE, &tid, outStream->handle(), outStream->handle());
    ::MonAPI::System::getStdoutStream();
    int result = monapi_call_process_execute_file_get_tid(s->value().data(), MONAPI_TRUE, &tid, g_terminal->getScreenHandle(), g_terminal->getScreenHandle());
    if (result != 0)
    {
        RAISE_ERROR(lineno(), "system can't execute %s" , s->value().data());
    }
    RETURN_BOOLEAN(true);
#endif
    RETURN_BOOLEAN(false);
}


PROCEDURE(CallProcessOutString, "call-process-out-string")
{
#ifdef MONA
    ARGC_SHOULD_BE(1);
    CAST(ARGV(0), SString, s);

    uint32_t tid;
    // fix me who release?
    MonAPI::Stream* in = new MonAPI::Stream();
    int result = monapi_call_process_execute_file_get_tid(s->value().data(), MONAPI_TRUE, &tid, in->handle(), in->handle());
    if (result != 0)
    {
        RAISE_ERROR(lineno(), "system can't execute %s" , s->value().data());
    }


    const uint32_t bufsize = 256;
    uint8_t buf[bufsize];
    ::util::String text;
    for (;;)
    {
        in->waitForRead();
        uint32_t size = in->read(buf, bufsize);
        if (size == 0)
        {
            continue;
        }

        text += ::util::String((char*)buf, size);
        if (size >= 3)
        {
            // bad!
            if (buf[size - 5] == '^' && buf[size - 4] == 'E' && buf[size - 3] == 'O' && buf[size - 2] == 'P')
            {
                break;
            }
        }

    }
    env->setVaribale(new Variable("status", lineno()), new Number(monapi_process_wait_terminated(tid), lineno()));
//    monapi_process_wait_terminated(tid);
    return new SString(text, lineno());
#endif
    RETURN_BOOLEAN(false);
}

#ifdef MONA
#include <servers/gui.h>
#endif

PROCEDURE(MonaGuiMoveWindow, "mona-gui-move-window")
{
#ifdef MONA
    ARGC_SHOULD_BE(3);
    CAST(ARGV(0), Number, handle);
    CAST(ARGV(1), Number, x);
    CAST(ARGV(2), Number, y);
    MessageInfo msg;
    uint32_t tid = monapi_get_server_thread_id(ID_GUI_SERVER);
    if (MonAPI::Message::sendReceive(&msg, tid, MSG_GUISERVER_MOVEWINDOW, handle->value(), x->value(), y->value()) != M_OK)
    {
        RETURN_BOOLEAN(false);
    }
    RETURN_BOOLEAN(true);

//     System::Mona::gui_move_window((uint32_t)handle->value(), x->value(), y->value());
//     RETURN_BOOLEAN(true);
#endif
    RETURN_BOOLEAN(false);
}

PROCEDURE(MonaGuiGetWindowTitle, "mona-gui-get-window-title")
{
#ifdef MONA
    ARGC_SHOULD_BE(1);
    CAST(ARGV(0), Number, handle);
    char buffer[WINDOW_TITLE_MAX_LENGTH];
    MessageInfo msg;
    uint32_t tid = monapi_get_server_thread_id(ID_GUI_SERVER);
    if (MonAPI::Message::sendReceive(&msg, tid, MSG_GUISERVER_GETTITLE, handle->value()) != M_OK)
    {
        RETURN_BOOLEAN(false);
    }
    memcpy(buffer, msg.str, WINDOW_TITLE_MAX_LENGTH);
    return new SString(buffer, lineno());
#endif
    RETURN_BOOLEAN(false);
}

PROCEDURE(MonaGuiEnumWindows, "mona-gui-enum-windows")
{
#ifdef MONA
    MessageInfo msg;
    uint32_t tid = monapi_get_server_thread_id(ID_GUI_SERVER);
    if (MonAPI::Message::sendReceive(&msg, tid, MSG_GUISERVER_ENUMWINDOWS) != M_OK)
    {
        RETURN_BOOLEAN(false);
    }

    int num = msg.arg2;
    Objects* objects = new Objects;
    for (int i = 0; i < num; i++)
    {
        objects->add(new Number(*((uint32_t*)&msg.str[i * 4])));
    }
    Cons* ret;
    SCM_LIST(objects, ret, lineno());
    return ret;
#endif
    RETURN_BOOLEAN(false);
}

PROCEDURE(MonaHalt, "mona-halt")
{
#ifdef MONA
    syscall_shutdown(SHUTDOWN_HALT, SHUTDOWN_DEVICE_ALL);
#endif
    RETURN_BOOLEAN(false);
}

PROCEDURE(MonaReboot, "mona-reboot")
{
#ifdef MONA
    syscall_shutdown(SHUTDOWN_REBOOT, SHUTDOWN_DEVICE_ALL);
#endif
    RETURN_BOOLEAN(false);
}

PROCEDURE(MonaKill, "mona-kill")
{
    ARGC_SHOULD_BE(1);
#ifdef MONA
    CAST(ARGV(0), Number, n);
    RETURN_BOOLEAN(syscall_kill_thread(n->value()) == 0);
#endif
    RETURN_BOOLEAN(false);
}

PROCEDURE(MonaSleep, "mona-sleep")
{
    ARGC_SHOULD_BE(1);
#ifdef MONA
    CAST(ARGV(0), Number, ms);
    sleep(ms->value());
    return SCM_TRUE;
#endif
    RETURN_BOOLEAN(false);
}

extern Scanner* g_scanner;

PROCEDURE(MonaPs, "mona-ps")
{
    ARGC_SHOULD_BE(0);
#ifdef MONA
    ::util::String result = "[tid] [state]  [eip]    [esp]    [cr3]    [name]\n";
    char buf[256];
    syscall_set_ps_dump();
    PsInfo info;
    while (syscall_read_ps_dump(&info) == 0)
    {
        sprintf(buf, "%5d %s %08x %08x %08x %s\n",
                info.tid, info.state ? "running" : "waiting",
                info.eip, info.esp, info.cr3, info.name);
        result += buf;
    }
    return new SString(result, lineno());
#endif
    RETURN_BOOLEAN(false);
}
