/*!
  \file  syscalls.cpp
  \brief syscalls

  Copyright (c) 2002,2003 Higepon and the individuals listed on the ChangeLog entries.
  All rights reserved.
  License=MIT/X License

  \author  Higepon
  \version $Revision$
  \date   create:2003/03/22 update:$Date$
*/

#include "syscalls.h"
#include "global.h"
#include "io.h"
#include "Loader.h"
#include "RTC.h"
#include "apm.h"
#include "shutdown.h"
#include "sys/error.h"
#include "Condition.h"
#include "KObjectService.h"
#include "sys/BinaryTree.h"

extern const char* version;
extern uint32_t version_number;

static BinaryTree<PsInfo*> psInfoMap;

inline static Process* getCurrentProcess()
{
    return g_currentThread->process;
}

inline intptr_t syscall1(intptr_t syscall_number, intptr_t arg1)
{
    intptr_t ret = 0;
    asm volatile("movl $%c1, %%ebx \n"
                 "movl %2  , %%esi \n"
                 "int  $0x80       \n"
                 "movl %%eax, %0   \n"
                 :"=m"(ret)
                 :"g"(syscall_number), "m"(arg1)
                 :"ebx", "esi"
                 );
    return ret;
}

inline intptr_t syscall2(intptr_t syscall_number, intptr_t arg1, intptr_t arg2)
{
    intptr_t ret = 0;
    asm volatile("movl $%c1, %%ebx \n"
                 "movl %2  , %%esi \n"
                 "movl %3  , %%ecx \n"
                 "int  $0x80       \n"
                 "movl %%eax, %0   \n"
                 :"=m"(ret)
                 :"g"(syscall_number), "m"(arg1), "m"(arg2)
                 :"ebx", "esi", "ecx"
                 );
    return ret;
}



static inline void setReturnValue(ArchThreadInfo* info, intptr_t value)
{
    info->eax = value;
}



uint32_t systemcall_mutex_lock(uint32_t id)
{
    int noTimeout = 0;
    return syscall2(SYSTEM_CALL_MUTEX_LOCK, id, noTimeout);
}

uint32_t systemcall_mutex_unlock(uint32_t id)
{
    return syscall1(SYSTEM_CALL_MUTEX_UNLOCK, id);
}

// don't call without systemcall
// this has context change
// use systemcall_mutex_lock()
static intptr_t systemcall_mutex_lock2(intptr_t id, intptr_t timeoutTick)
{
    KObject* object = g_id->get(id, KObject::KMUTEX);
    if (NULL == object) {
        return M_BAD_MUTEX_ID;
    }
    KMutex* mutex = (KMutex*)object;
    return mutex->lock(g_currentThread->thread, timeoutTick);
}

// don't call without systemcall
// this has context change
// use systemcall_mutex_unlock()
static intptr_t systemcall_mutex_unlock2(intptr_t id)
{
    KObject* object = g_id->get(id, KObject::KMUTEX);
    if (object == NULL) {
        return M_BAD_MUTEX_ID;
    }
    KMutex* mutex = (KMutex*)object;
    return mutex->unlock();
}

void syscall_entrance()
{
    ScreenInfo* screenInfo;
    ArchThreadInfo* info = g_currentThread->archinfo;

    /* result normal */
    setReturnValue(info, M_OK);

#define SYSTEM_CALL_ARG_1 (info->esi)
#define SYSTEM_CALL_ARG_2 (info->ecx)
#define SYSTEM_CALL_ARG_3 (info->edi)
#define SYSTEM_CALL_ARG_4 (info->edx)

// To check resource leak on each syscall.
#if 0
    static int prevSyscall = 0;
    static uintptr_t prevSize = 0;
    static const char* prevName = "";
    if (prevSize != km.getFreeSize()) {
        logprintf("syscall[%x], %s %d %d %s\n", prevSyscall, km.getFreeSize() > prevSize ? "free-ed" : "alloc-ed", km.getFreeSize() - prevSize, km.getFreeSize(), prevName);
    }
    prevSize = km.getFreeSize();
    prevSyscall = info->ebx;
    prevName =  getCurrentProcess()->getName();
#endif
    switch(info->ebx)
    {
    case SYSTEM_CALL_PRINT:

        //enableInterrupt();
        g_console->printf("%s", (char*)(SYSTEM_CALL_ARG_1));
        break;

    case SYSTEM_CALL_ALLOCATE_CONTIGUOUS:
    {

        LinearAddress laddress = SYSTEM_CALL_ARG_1;
        int pageNum = SYSTEM_CALL_ARG_2;
        setReturnValue(info, g_page_manager->allocateContiguous(getCurrentProcess()->getPageDirectory(),
                                                       laddress,
                                                       pageNum));
        break;
    }
    case SYSTEM_CALL_DEALLOCATE_CONTIGUOUS:
    {
        LinearAddress laddress = SYSTEM_CALL_ARG_1;
        int pageNum = SYSTEM_CALL_ARG_2;
        g_page_manager->deallocateContiguous(getCurrentProcess()->getPageDirectory(),
                                             laddress,
                                             pageNum);
        break;
    }
    case SYSTEM_CALL_SET_TIMER:

        setReturnValue(info, g_scheduler->SetTimer(g_currentThread->thread, SYSTEM_CALL_ARG_1));
        break;

    case SYSTEM_CALL_KILL_TIMER:

        setReturnValue(info, g_scheduler->KillTimer(SYSTEM_CALL_ARG_1, g_currentThread->thread));
        break;

    case SYSTEM_CALL_MTHREAD_SLEEP:
        g_scheduler->Sleep(g_currentThread->thread, SYSTEM_CALL_ARG_1);
        g_scheduler->SwitchToNext();
        break;
    case SYSTEM_CALL_GET_KSTAT:
    {
        KStat* dest = (KStat*)(SYSTEM_CALL_ARG_1);
        *dest = gKStat;
        break;
    }
    case SYSTEM_CALL_MTHREAD_SELF:
    {
        setReturnValue(info, g_currentThread->thread->id);
        break;
    }
    case SYSTEM_CALL_KILL:
    {
        ThreadOperation::kill(getCurrentProcess(), g_currentThread->thread);
        g_scheduler->SwitchToNext();
        break;
    }
    case SYSTEM_CALL_KILL_THREAD:
    {
        uint32_t tid = SYSTEM_CALL_ARG_1;
        intptr_t ret = ThreadOperation::kill(tid);
        if (ret == Scheduler::YIELD) {
            g_scheduler->SwitchToNext();
        } else {
            setReturnValue(info, ret);
        }
        break;
    }

    case SYSTEM_CALL_SEND:
    {
        uintptr_t id = (uintptr_t)(SYSTEM_CALL_ARG_1);
        Thread* thread = g_scheduler->Find(id);
        if (NULL == thread) {
            setReturnValue(info, -1);
            break;
        } else {
            intptr_t ret = g_messenger->send(thread, (MessageInfo*)(SYSTEM_CALL_ARG_2));
            setReturnValue(info, ret);
            if (ret == M_OK) {
                g_scheduler->SwitchToNext();
                /* not reached */
            } else {
                break;
            }
        }
    }
    case SYSTEM_CALL_RECEIVE:

        setReturnValue(info, g_messenger->receive(g_currentThread->thread, (MessageInfo*)(SYSTEM_CALL_ARG_1)));
        break;

    case SYSTEM_CALL_EXIST_MESSAGE:
    {
        bool existMessage = !(g_currentThread->thread->messageList->isEmpty());
        setReturnValue(info, existMessage ? 1 : 0);
        break;
    }
    case SYSTEM_CALL_GET_PHYSICAL_ADDRESS:
    {
        PhysicalAddress ret = 0;
        g_page_manager->getPhysicalAddress((PageEntry*)g_currentThread->archinfo->cr3, (uint32_t)(SYSTEM_CALL_ARG_1), &ret);
        setReturnValue(info, ret);
        break;
    }


    case SYSTEM_CALL_MTHREAD_CREATE:
    {
        uint32_t arg = SYSTEM_CALL_ARG_2;
        Thread* thread = ThreadOperation::create(getCurrentProcess(), SYSTEM_CALL_ARG_1);
        thread->tinfo->archinfo->ecx = arg;
        g_scheduler->Join(thread);
        setReturnValue(info, thread->id);
        break;
    }
    case SYSTEM_CALL_MTHREAD_KILL:
    {
//         KObject* object = g_id->get(SYSTEM_CALL_ARG_1, g_currentThread->thread, KObject::THREAD);

//         if (object == NULL) {
//             setReturnValue(info, M_BAD_THREAD_ID);
//         } else {
//             Thread* t = (Thread*)object;
//             intptr_t ret = ThreadOperation::kill(t->id);
//             if (ret == Scheduler::YIELD) {
//                 g_scheduler->SwitchToNext();
//             } else {
//                 setReturnValue(info, ret);
//             }
//         }
        break;
    }
    case SYSTEM_CALL_CONDITION_CREATE:
    {
        Process* owner = g_currentThread->thread->tinfo->process;
        intptr_t condition_id = KObjectService::create<Condition>(owner);
        setReturnValue(info, condition_id);
        break;
    }
    case SYSTEM_CALL_CONDITION_DESTROY:
    {
        const intptr_t condition_id = SYSTEM_CALL_ARG_1;
        KObject* object = g_id->get(condition_id, KObject::CONDITION);
        if (object == NULL) {
            setReturnValue(info, M_BAD_CONDITION_ID);
        } else {
            KObjectService::destroy(getCurrentProcess(), condition_id, object);
            setReturnValue(info, M_OK);
        }
        break;
    }
    case SYSTEM_CALL_CONDITION_NOTIFY_ALL:
    {
        const intptr_t condition_id = SYSTEM_CALL_ARG_1;
        KObject* object = g_id->get(condition_id, KObject::CONDITION);
        if (object == NULL) {
            setReturnValue(info, M_BAD_CONDITION_ID);
        } else {
            Condition* condition = (Condition*)object;
            intptr_t ret = condition->notifyAll();
            setReturnValue(info, M_OK);
            ASSERT(ret == Scheduler::YIELD);
            g_scheduler->SwitchToNext();

            /* Not reached */
        }
        break;
    }
    case SYSTEM_CALL_CONDITION_WAIT:
    {
        const intptr_t condition_id = SYSTEM_CALL_ARG_1;
        KObject* condObject = g_id->get(condition_id, KObject::CONDITION);

        const intptr_t mutex_id = SYSTEM_CALL_ARG_2;
        KObject* mutexObject = g_id->get(mutex_id, KObject::KMUTEX);

        if (condObject == NULL) {
            setReturnValue(info, M_BAD_CONDITION_ID);
        } else if (mutexObject == NULL) {
            setReturnValue(info, M_BAD_MUTEX_ID);
        } else {
            Condition* condition = (Condition*)condObject;
            KMutex* mutex = (KMutex*)mutexObject;

            // unlock and wait should be atomic.
            mutex->unlock();
            intptr_t ret = condition->wait(g_currentThread->thread);
            ASSERT(ret == Scheduler::YIELD);
            g_scheduler->SwitchToNext();
            setReturnValue(info, ret);
        }
        break;
    }
    case SYSTEM_CALL_CONDITION_WAIT_TIMEOUT:
    {
        const intptr_t condition_id = SYSTEM_CALL_ARG_1;
        KObject* condObject = g_id->get(condition_id, KObject::CONDITION);

        const intptr_t mutex_id = SYSTEM_CALL_ARG_2;
        KObject* mutexObject = g_id->get(mutex_id, KObject::KMUTEX);

        const intptr_t timeoutTick = SYSTEM_CALL_ARG_3;

        if (condObject == NULL) {
            setReturnValue(info, M_BAD_CONDITION_ID);
        } else if (mutexObject == NULL) {
            setReturnValue(info, M_BAD_MUTEX_ID);
        } else {
            Condition* condition = (Condition*)condObject;
            KMutex* mutex = (KMutex*)mutexObject;

            // unlock and wait should be atomic.
            mutex->unlock();
            intptr_t ret = condition->waitTimeout(g_currentThread->thread, timeoutTick);
            ASSERT(ret == Scheduler::YIELD);
            g_scheduler->SwitchToNext();
            setReturnValue(info, ret);
        }
        break;
    }
    case SYSTEM_CALL_MUTEX_CREATE:
        if (SYSTEM_CALL_ARG_1 == MUTEX_CREATE_NEW) {
            Process* owner = g_currentThread->thread->tinfo->process;
            intptr_t mutexid = KObjectService::create<KMutex>(owner);
            ASSERT(mutexid > 0);
            setReturnValue(info, mutexid);
        } else {
            KObject* object = g_id->get(SYSTEM_CALL_ARG_1, KObject::KMUTEX);
            if (object == NULL) {
                setReturnValue(info, M_BAD_MUTEX_ID);
            } else {
                KMutex* mutex = (KMutex*)object;
                Process* owner = g_currentThread->thread->tinfo->process;
                intptr_t id = KObjectService::markAsShared(owner, mutex);
                setReturnValue(info, id);
            }
        }
        break;
    case SYSTEM_CALL_SEMAPHORE_CREATE:
    {
        int num = SYSTEM_CALL_ARG_1;
        Process* owner = g_currentThread->thread->tinfo->process;
        intptr_t id = KObjectService::createUserSemaphore(owner, num);
        setReturnValue(info, id);
        break;
    }
    case SYSTEM_CALL_MUTEX_LOCK:
    {
        intptr_t ret = systemcall_mutex_lock2(SYSTEM_CALL_ARG_1, SYSTEM_CALL_ARG_2);
        if (ret == Scheduler::YIELD) {
            g_scheduler->SwitchToNext();
        } else {
            setReturnValue(info, ret);
        }
        break;
    }
    case SYSTEM_CALL_SEMAPHORE_DOWN:
    {
        KObject* object = g_id->get(SYSTEM_CALL_ARG_1, KObject::USER_SEMAPHORE);
        if (object == NULL) {
            setReturnValue(info, M_BAD_SEMAPHORE_ID);
        } else {
            UserSemaphore* semaphore = (UserSemaphore*)object;
            intptr_t ret = semaphore->down(g_currentThread->thread);
            if (ret == Scheduler::YIELD) {
                g_scheduler->SwitchToNext();
            } else {
                setReturnValue(info, ret);
            }
        }
        break;
    }
    case SYSTEM_CALL_MUTEX_TRY_LOCK:
    {
        KObject* object = g_id->get(SYSTEM_CALL_ARG_1, KObject::KMUTEX);
        if (object == NULL) {
            setReturnValue(info, M_BAD_MUTEX_ID);
        } else {
            setReturnValue(info, ((KMutex*)object)->tryLock(g_currentThread->thread));
        }
        break;
    }
    case SYSTEM_CALL_SEMAPHORE_TRYDOWN:
    {
        KObject* object = g_id->get(SYSTEM_CALL_ARG_1, KObject::USER_SEMAPHORE);

        if (object == NULL) {
            setReturnValue(info, M_BAD_SEMAPHORE_ID);
        } else {
            setReturnValue(info, ((UserSemaphore*)object)->tryDown(g_currentThread->thread));
        }
        break;
    }

    case SYSTEM_CALL_MUTEX_UNLOCK:
    {
        intptr_t ret = systemcall_mutex_unlock2(SYSTEM_CALL_ARG_1);
        if (ret == Scheduler::YIELD) {
            g_scheduler->SwitchToNext();
        } else {
            setReturnValue(info, ret);
        }
        break;
    }
    case SYSTEM_CALL_SEMAPHORE_UP:
    {
        KObject* object = g_id->get(SYSTEM_CALL_ARG_1, KObject::USER_SEMAPHORE);

        if (object == NULL) {
            setReturnValue(info, M_BAD_SEMAPHORE_ID);
        } else {
            UserSemaphore* semaphore = (UserSemaphore*)object;
            intptr_t ret = semaphore->up();
            if (ret == Scheduler::YIELD) {
                g_scheduler->SwitchToNext();
            } else {
                setReturnValue(info, ret);
            }
        }
        break;
    }
    case SYSTEM_CALL_MUTEX_DESTROY:
    {
        intptr_t id = SYSTEM_CALL_ARG_1;
        KObject* object = g_id->get(id, KObject::KMUTEX);
        if (object == NULL) {
            setReturnValue(info, M_BAD_MUTEX_ID);
        } else {
            if (KObjectService::destroy(getCurrentProcess(), id, object)) {
                // mutex is not no more referenced, so deleted.
                setReturnValue(info, M_OK);
            } else {
                // mutex is referenced by other.
                setReturnValue(info, M_RELEASED);
            }
        }
        break;
    }
    case SYSTEM_CALL_MUTEX_COUNT:
    {
        setReturnValue(info, g_id->getCount(KObject::KMUTEX));
        break;
    }
    case SYSTEM_CALL_CONDITION_COUNT:
    {
        setReturnValue(info, g_id->getCount(KObject::CONDITION));
        break;
    }
    case SYSTEM_CALL_SEMAPHORE_DESTROY:
    {
        intptr_t id = SYSTEM_CALL_ARG_1;
        KObject* object = g_id->get(id, KObject::USER_SEMAPHORE);
        if (object == NULL) {
            setReturnValue(info, M_BAD_SEMAPHORE_ID);
        } else {
            KObjectService::destroy(getCurrentProcess(), id, object);
            setReturnValue(info, M_OK);
        }
        break;
    }
    case SYSTEM_CALL_LOOKUP:
        setReturnValue(info, g_scheduler->Lookup((char*)(SYSTEM_CALL_ARG_1)));
        break;

    case SYSTEM_CALL_GET_VRAM_INFO:
        screenInfo = (ScreenInfo*)(SYSTEM_CALL_ARG_1);
        screenInfo->vram = (uint32_t)(g_vesaDetail->physBasePtr);
        screenInfo->bpp  = (uint32_t)(g_vesaDetail->bitsPerPixel);
        screenInfo->x    = (uint32_t)(g_vesaDetail->xResolution);
        screenInfo->y    = (uint32_t)(g_vesaDetail->yResolution);
        break;

    case SYSTEM_CALL_LOAD_PROCESS:

        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_SET_CURSOR:

        g_console->setCursor((int)(SYSTEM_CALL_ARG_1), (int)(SYSTEM_CALL_ARG_2));
        break;

    case SYSTEM_CALL_GET_CURSOR:

        g_console->getCursor((int*)(SYSTEM_CALL_ARG_1), (int*)(SYSTEM_CALL_ARG_2));
        break;

    case SYSTEM_CALL_FDC_OPEN:

        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FDC_CLOSE:

        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FDC_READ:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FDC_WRITE:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FILE_OPEN:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FILE_READ:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FILE_WRITE:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FILE_CREATE:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FILE_CLOSE:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_GET_PID:

        setReturnValue(info, getCurrentProcess()->getPid());
        break;

    case SYSTEM_CALL_GET_TID:

        setReturnValue(info, g_currentThread->thread->id);
        break;

    case SYSTEM_CALL_ARGUMENTS_NUM:

        setReturnValue(info, getCurrentProcess()->getArguments()->size());
        break;

    case SYSTEM_CALL_GET_ARGUMENTS:
    {
        List<char*>* list = getCurrentProcess()->getArguments();
        char* buf = (char*)(SYSTEM_CALL_ARG_1);
        int index = (int)(SYSTEM_CALL_ARG_2);

        if (index - 1 > list->size())
        {
            setReturnValue(info, 1);
            break;
        }

        strncpy(buf, list->get(index), MAX_PROCESS_ARGUMENT_LENGTH);
        setReturnValue(info, 0);
        break;
    }

    case SYSTEM_CALL_MTHREAD_YIELD_MESSAGE:

        /* message has come. after your last peek or receive */
        if (g_currentThread->thread->flags & MEvent::MESSAGE)
        {
            break;
        }

        g_scheduler->WaitEvent(g_currentThread->thread, MEvent::MESSAGE);
        g_scheduler->SwitchToNext();

        /* not reached */
        break;

    case SYSTEM_CALL_DATE:
    {
        KDate* date = (KDate*)(SYSTEM_CALL_ARG_1);
        RTC::getDate(date);
        setReturnValue(info, 0);
        break;
    }


    case SYSTEM_CALL_GET_IO:

        info->eflags = info->eflags |  0x3000;
        setReturnValue(info, 0);
        {
            bool isProcessChange = g_scheduler->SetNextThread();
            ThreadOperation::switchThread(isProcessChange, 17);
        }
        break;

    case SYSTEM_CALL_FDC_DISK_CHANGED:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_LOOKUP_MAIN_THREAD:

    if (SYSTEM_CALL_ARG_1 == NULL)
    {
        setReturnValue(info, g_scheduler->LookupMainThread(getCurrentProcess()));
    }
    else
    {
        setReturnValue(info, g_scheduler->LookupMainThread((char*)(SYSTEM_CALL_ARG_1)));
    }
        break;

    case SYSTEM_CALL_MEMORY_MAP_CREATE:
    {
        // static uint32_t sharedId = 0x9000;
        // sharedId++;
        uint32_t size = SYSTEM_CALL_ARG_1;

        while (Semaphore::down(&g_semaphore_shared));
        SharedMemoryObject* shm = SharedMemoryObject::create(size);
        Semaphore::up(&g_semaphore_shared);

        if (shm == NULL) {
            setReturnValue(info, 0);
            break;
        } else {
            setReturnValue(info, shm->getId());
        }
        break;
     }


    case SYSTEM_CALL_MEMORY_MAP_GET_SIZE:
    {
        uint32_t id = SYSTEM_CALL_ARG_1;
        SharedMemoryObject* object = SharedMemoryObject::find(id);
        if (object == NULL) {
            setReturnValue(info, 0);
        } else {
            ASSERT(object->getSize() != 0);
            setReturnValue(info, object->getSize());
        }
        break;
    }

    case SYSTEM_CALL_MEMORY_MAP_MAP:
    {
        uint32_t id = SYSTEM_CALL_ARG_1;
        uint32_t address = SYSTEM_CALL_ARG_2;
        bool isImmediateMap = SYSTEM_CALL_ARG_3 == 1;

        while (Semaphore::down(&g_semaphore_shared));
        SharedMemoryObject* shm = SharedMemoryObject::find(id);
        if (shm == NULL) {
            Semaphore::up(&g_semaphore_shared);
            Semaphore::up(&g_semaphore_shared);
            setReturnValue(info, M_BAD_MEMORY_MAP_ID);
        } else {
            intptr_t ret = shm->attach(g_page_manager, getCurrentProcess(), address, isImmediateMap);
            Semaphore::up(&g_semaphore_shared);
            Semaphore::up(&g_semaphore_shared);
            setReturnValue(info, ret);
        }
        break;
    }
    case SYSTEM_CALL_MEMORY_MAP_UNMAP:
    {
        uint32_t id = SYSTEM_CALL_ARG_1;
        while (Semaphore::down(&g_semaphore_shared));
        SharedMemoryObject* shm = SharedMemoryObject::find(id);
        if (shm == NULL) {
            setReturnValue(info, M_BAD_MEMORY_MAP_ID);
        } else {
            intptr_t ret = shm->detach(g_page_manager, getCurrentProcess());
            SharedMemoryObject::destroy(shm);
            setReturnValue(info, ret);
        }
        Semaphore::up(&g_semaphore_shared);
        break;
    }

    case SYSTEM_CALL_CD:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_DIR_OPEN:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_DIR_CLOSE:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_DIR_READ:
        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_PS_DUMP_SET:
    {
        PsInfo* dump = g_scheduler->GetAllDump();
        psInfoMap.add(g_currentThread->thread->id, dump);
        break;
    }
    case SYSTEM_CALL_PS_DUMP_READ:
    {
        PsInfo* dest = (PsInfo*)(SYSTEM_CALL_ARG_1);
        PsInfo* current = psInfoMap.get(g_currentThread->thread->id);

        if (current == NULL) {
            setReturnValue(info, 1);
            break;
        } else {
            *dest = *current;
            psInfoMap.add(g_currentThread->thread->id, current->next);
            delete current;
            setReturnValue(info, M_OK);
            break;
        }
    }

    case SYSTEM_CALL_GET_TICK:

        setReturnValue(info, g_scheduler->GetTick());
        break;

    case SYSTEM_CALL_FILE_POSITION:

        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_FILE_SEEK:

        g_console->printf("this systemcall not supported %s:%d\n", __FILE__, __LINE__);
        break;

    case SYSTEM_CALL_GET_KERNEL_VERSION:

        strncpy((char*)SYSTEM_CALL_ARG_1, version, SYSTEM_CALL_ARG_2);
        setReturnValue(info, version_number);
        break;

    case SYSTEM_CALL_LOAD_PROCESS_IMAGE:
    {
        LoadProcessInfo* p = (LoadProcessInfo*)(SYSTEM_CALL_ARG_1);
        setReturnValue(info, Loader::Load(p->image, p->size, p->entrypoint, p->name, true, p->list));
        break;
    }

    case SYSTEM_CALL_CLEAR_SCREEN:

        g_console->clearScreen();
        setReturnValue(info, 0);
        break;

    case SYSTEM_CALL_PEEK:

        setReturnValue(info, g_messenger->peek(g_currentThread->thread
                                      , (MessageInfo*)(SYSTEM_CALL_ARG_1)
                                      , (int)(SYSTEM_CALL_ARG_2)
                                      , (int)(SYSTEM_CALL_ARG_3)
                                      ));

        break;

    case SYSTEM_CALL_REMOVE_IRQ_RECEIVER:
    {
        int irq = (int)SYSTEM_CALL_ARG_1;

        /* out of range */
        if (irq > 15 || irq < 0)
        {
            setReturnValue(info, 1);
            break;
        }

        g_irqInfo[irq].hasReceiver = false;
        g_irqInfo[irq].maskInterrupt = false;

        break;
    }

    case SYSTEM_CALL_SET_IRQ_RECEIVER:
    {
        int irq  = (int)SYSTEM_CALL_ARG_1;
        bool maskInterrupt = SYSTEM_CALL_ARG_2 != 0;

        /* out of range */
        if (irq > 15 || irq < 0)
        {
            break;
        }

        g_irqInfo[irq].hasReceiver = true;
        g_irqInfo[irq].thread      = g_currentThread;
        g_irqInfo[irq].maskInterrupt = maskInterrupt;

        break;
    }
    case SYSTEM_CALL_HAS_IRQ_RECEIVER:
    {
        int irq  = (int)SYSTEM_CALL_ARG_1;

        /* out of range */
        if (irq > 15 || irq < 0)
        {
            break;
        }
        setReturnValue(info, g_irqInfo[irq].hasReceiver ? 1 : 0);
        break;
    }

    // case SYSTEM_CALL_FREE_PAGES:
    // {
    //     uint32_t address = SYSTEM_CALL_ARG_1;
    //     uint32_t size    = SYSTEM_CALL_ARG_2;

    //     g_page_manager->returnPages(getCurrentProcess()->getPageDirectory(), address, size);
    //     break;
    // }

    case SYSTEM_CALL_GET_MEMORY_INFO:
    {
        MemoryInfo* i = (MemoryInfo*)(SYSTEM_CALL_ARG_1);

        i->totalMemoryL = g_total_system_memory;
        g_page_manager->getPagePoolInfo(&(i->freePageNum), &(i->totalPageNum), &(i->pageSize));
        break;
    }

    case SYSTEM_CALL_LOG_PRINT:
    {
        logprintf("%s", (const char*)(SYSTEM_CALL_ARG_1));
        break;
    }

    case SYSTEM_CALL_ALLOCATE_DMA_MEMORY:
    {
        uint32_t size = SYSTEM_CALL_ARG_1;
        setReturnValue(info, (uint32_t)g_page_manager->allocateDMAMemory(getCurrentProcess()->getPageDirectory(), size, true));
        break;
    }
    case SYSTEM_CALL_DEALLOCATE_DMA_MEMORY:

        g_page_manager->deallocateDMAMemory(getCurrentProcess()->getPageDirectory(), SYSTEM_CALL_ARG_1, SYSTEM_CALL_ARG_2);
        break;
    case SYSTEM_CALL_CHANGE_BASE_PRIORITY:
        g_scheduler->ChangeBasePriority(g_currentThread->thread, SYSTEM_CALL_ARG_1);
        break;
    case SYSTEM_CALL_SET_DLL_SEGMENT_WRITABLE:
        panic(__func__);
    break;

    case SYSTEM_CALL_SET_DLL_SEGMENT_NOTSHARED:
        panic(__func__);
    break;

    case SYSTEM_CALL_APM_BIOS:
        setReturnValue(info, (uint32_t)apm_bios((uint16_t)SYSTEM_CALL_ARG_1, (apm_bios_regs*)SYSTEM_CALL_ARG_2));
    break;

    case SYSTEM_CALL_SHUTDOWN:
        setReturnValue(info, shutdown(SYSTEM_CALL_ARG_1, SYSTEM_CALL_ARG_2));
    break;
    case SYSTEM_CALL_SET_WATCH_POINT:
    {
#define B4(a,b,c,d) ((a)*8+(b)*4+(c)*2+(d))
#define B8(a,b,c,d,e,f,g,h) (B4(a,b,c,d)*16+B4(e,f,g,h))

        uint32_t address   = SYSTEM_CALL_ARG_1;
        uint32_t breakflag = SYSTEM_CALL_ARG_2;
        uint32_t flag = B8(0,0,0,0,0,0,0,0) << 24
            | B8(0,0,0,0,1,1, (breakflag & 2) >> 1 ,(breakflag & 1)) << 16
            | B8(0,0,0,0,0,1,0,1) << 8
            | B8(0,0,0,0,0,0,1,1);

        uint32_t dr0, dr7;
        asm volatile("movl %2, %%eax    \n"
                     "movl %%eax, %%dr0 \n"
                     "movl %3, %%eax    \n"
                     "movl %%eax, %%dr7 \n"
                     "movl %%dr0, %%eax  \n"
                     "movl %%eax, %0    \n"
                     "movl %%dr7, %%eax  \n"
                     "movl %%eax, %1    \n"
                     : "=m"(dr0), "=m"(dr7) : "m"(address), "m"(flag): "eax");
#if 0
        g_console->printf("dr0=%x dr7=%x\n", dr0, dr7);
#endif
        break;
    }
    case SYSTEM_CALL_REMOVE_WATCH_POINT:
    {
        uint32_t flag = B8(0,0,0,0,0,0,0,0) << 24
            | B8(0,0,0,0,1,1,1,1) << 16
            | B8(0,0,0,0,0,1,0,1) << 8
            | B8(0,0,0,0,0,0,0,0);

        asm volatile("movl %0, %%eax    \n"
                     "movl %%eax, %%dr0 \n"
                     : : "m"(flag): "eax");
        break;
    }
    case SYSTEM_CALL_STACKTRACE_ENABLE:
    {
        uint32_t pid = SYSTEM_CALL_ARG_1;
        uint8_t* data = (uint8_t*)SYSTEM_CALL_ARG_2;
        uint32_t size = SYSTEM_CALL_ARG_3;
        bool res = g_page_manager->enableStackTrace(pid, data, size);
        setReturnValue(info, res? M_OK : M_UNKNOWN);
        break;
    }
    case SYSTEM_CALL_STACKTRACE_DISABLE:
    {
        uint32_t pid = SYSTEM_CALL_ARG_1;
        g_page_manager->disableStackTrace(pid);
        setReturnValue(info, 0);
        break;
    }
    case SYSTEM_CALL_STACKTRACE_DUMP:
    {
        uint32_t pid = SYSTEM_CALL_ARG_1;
        g_scheduler->ReserveStackDump(pid);
        setReturnValue(info, 0);
        break;
    }
    case SYSTEM_CALL_NOW_IN_NANOSEC:
    {
        union {
            struct {
                uint32_t l;
                uint32_t h;
            } u32;
            uint64_t u64;
        } n;
        n.u64 = RTC::epochNanoseconds();
        *((uint32_t*)SYSTEM_CALL_ARG_1) = n.u32.l;
        *((uint32_t*)SYSTEM_CALL_ARG_2) = n.u32.h;
        break;
    }

    default:
        g_console->printf("syscall:default");
        break;
    }

    return;
}
