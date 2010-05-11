#include "Scheduler.h"
#include "Messenger.h"
#include "global.h"

/*----------------------------------------------------------------------
    Messenger
----------------------------------------------------------------------*/
Messenger::Messenger(Scheduler* scheduler) : scheduler_(scheduler)
{
}

Messenger::~Messenger()
{
}


intptr_t Messenger::send(Thread* thread, MessageInfo* message)
{
    ASSERT(message != NULL);

    if (thread->messageList->size() == MAX_MESSAGES) {
        return M_MESSAGE_OVERFLOW;
    }

    if (message->header == 0x417) {
        logprintf("MSG_FILE_WRITE!\n");
    }

    if (message->header == 3 && message->arg1 == 0x417) {
        logprintf("MSG_RESULT_OK(%x) from=%x! uid=%x\n", message->arg1, message->from, message->uid);
    }

    if (message == (MessageInfo*)NULL)
    {
        if (message->header == 3 && message->arg1 == 0x417) {
            logprintf("drop MSG_RESULT_OK\n");
        }
        return -1;
    }

    MessageInfo* info = new MessageInfo;

    *info = *message;

    info->from = g_currentThread->thread->id;

    thread->flags |= MEvent::MESSAGE;
    thread->messageList->add(info);

    if (message->header == 3 && message->arg1 == 0x417) {
        logprintf("MSG_RESULT_OK(%x) %s from=%x! uid=%x list=%x listSize=%d\n", message->arg1, thread->tinfo->process->getName(), message->from, message->uid, thread->messageList, thread->messageList->size());
//         for (int i = 0; i < thread->messageList->size(); i++) {
//             logprintf("header=%x %d\n", thread->messageList->get(i)->header, i);
//         }
   }


    scheduler_->EventComes(thread, MEvent::MESSAGE);
    return M_OK;
}

int Messenger::receive(Thread* thread, MessageInfo* message)
{
    MessageInfo* from = thread->messageList->removeAt(0);
    if (from == NULL) {
        return -1;
    }

    thread->flags &= ~MEvent::MESSAGE;
    *message = *from;
    delete from;
    return M_OK;
}

int Messenger::peek(Thread* thread, MessageInfo* message, int index, int flags)
{
    List<MessageInfo*>* list = thread->messageList;

    if (index > list->size())
    {
        logprintf("%s invalid peek list=%x %d:%d\n", thread->tinfo->process->getName(), list, index, list->size());
        return 1;
    }

    logprintf("peek remove? %d : index=%d %s size=%d\n", flags & PEEK_REMOVE, index, thread->tinfo->process->getName(), list->size());
        for (int i = 0; i < thread->messageList->size(); i++) {
            logprintf("header=%x %d, arg1=%x\n", thread->messageList->get(i)->header, i, thread->messageList->get(i)->arg1);
        }

    MessageInfo* from = NULL;
    if (flags & PEEK_REMOVE) {
        from = list->removeAt(index);
        logprintf("peek REMOVED %d\n", list->size());
        for (int i = 0; i < thread->messageList->size(); i++) {
            logprintf("header=%x %d, arg1=%x\n", thread->messageList->get(i)->header, i, thread->messageList->get(i)->arg1);
        }

    } else {
        logprintf("peek NOT REMOVED %d\n", list->size());
        from = list->get(index);
    }

    if (from == (MessageInfo*)NULL)
    {
        return -1;
    }

    logprintf("peek header=%x\n", from->header);

    thread->flags &= ~MEvent::MESSAGE;
    *message = *from;
    if (flags & PEEK_REMOVE) {
        delete from;
    }
    return M_OK;
}
