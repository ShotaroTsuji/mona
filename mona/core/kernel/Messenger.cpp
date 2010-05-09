#include "Messenger.h"
#include "global.h"

/*----------------------------------------------------------------------
    Messenger
----------------------------------------------------------------------*/
Messenger::Messenger(int size) : size_(size), allocated_(0) {

    info_ = new MessageInfo[size];
}

Messenger::~Messenger() {
}

MessageInfo* Messenger::allocateMessageInfo() {

    MessageInfo* result = &(info_[allocated_]);
    allocated_++;
    if (allocated_ > size_ - 1) {

#if 1  // DEBUG for message
        logprintf("***** msg buf index set to zero again ****");
#endif
        allocated_ = 0;
    }
    return result;
}

int Messenger::send(uint32_t id, MessageInfo* message)
{
    Thread* thread;
    MessageInfo* info;
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

    if ((thread = g_scheduler->Find(id)) == (Thread*)NULL)
    {
    if (message->header == 3 && message->arg1 == 0x417) {
        logprintf("drop MSG_RESULT_OK2\n");
    }

        return -1;
    }

    info = allocateMessageInfo();

#if 0
    logprintf("send:to=%x head=%x a1=%x a2=%x a3=%x from=%x\n"
              , id
              , message->header
              , message->arg1
              , message->arg2
              , message->arg3
              , message->from
        );
#endif

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


    g_scheduler->EventComes(thread, MEvent::MESSAGE);

    return 0;
}

int Messenger::receive(Thread* thread, MessageInfo* message)
{
    MessageInfo* from = thread->messageList->removeAt(0);

    if (from == (MessageInfo*)NULL)
    {
        return -1;
    }

#if 0
    logprintf("recv:to=%x head=%x a1=%x a2=%x a3=%x from=%x\n"
              , thread->id
              , message->header
              , message->arg1
              , message->arg2
              , message->arg3
              , message->from
        );
#endif

    thread->flags &= ~MEvent::MESSAGE;
    *message = *from;
//     if (message->arg2 > 0x9050)
//     {
//         logprintf("message->arg2 = %x\n", message->arg2);
//     }
    return 0;
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
    return 0;
}
