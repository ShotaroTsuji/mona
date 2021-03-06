#include <monapi/messages.h>
#include <monapi.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <vector>

using namespace MonAPI;
using namespace std;

#define LISTEN_PORT 8181

static SharedMemory* clip;
static Mutex mutex;

static void __fastcall listenThread(void* arg)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        monapi_fatal("socket failed %d", sock);
    }

    struct sockaddr_in addr;
    memset(&addr, sizeof(addr), 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(LISTEN_PORT);

    int ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        monapi_fatal("bind failed %d", sock);
    }
    ret = listen(sock, 5);
    if (ret < 0) {
        monapi_fatal("listen failed %d", sock);
    }

    for (;;) {
        struct sockaddr_in writer_addr;
        uint32_t writer_len;
        int sockfd =  accept(sock, (struct sockaddr *)&writer_addr, &writer_len);
        if (sockfd < 0) {
            monapi_fatal("accept failed %d", sockfd);
        }

#define MAXDATA 1024
        std::vector<uint8_t> clip_data;
        uint8_t buf[MAXDATA];
        int readSize = recv(sockfd, buf, MAXDATA, 0);
        do {
            for (int i = 0; i < readSize; i++) {
                clip_data.push_back(buf[i]);
            }
        } while ((readSize = recv(sockfd, buf, MAXDATA, 0)) > 0);
        closesocket(sockfd);
        mutex.lock();
        if (clip != NULL) {
            delete clip;
        }
        clip = new SharedMemory(clip_data.size());
        if (clip->map() != M_OK ) {
            monapi_fatal("map error");
        }
        copy(clip_data.begin(), clip_data.end(), clip->data());
        mutex.unlock();
    }
    closesocket(sock);
    exit(0);
}

int main(int argc, char* argv[])
{
    if (monapi_notify_server_start("MONITOR.BIN") != M_OK) {
        monapi_warn("MONITOR not found");
        exit(-1);
    }

    syscall_mthread_create_with_arg(listenThread, NULL);

    if (monapi_name_add("/servers/clipboard") != M_OK) {
        monapi_fatal("monapi_name_add failed");
    }

    for (MessageInfo msg;;)
    {
        if (Message::receive(&msg) != 0) continue;

        switch (msg.header)
        {
        case MSG_CLIPBOARD_SET:
        {
            mutex.lock();
            if (clip != NULL) {
                delete clip;
            }
            clip = new SharedMemory(msg.arg1, msg.arg2);
            ASSERT(clip);

            // memory map referce should be greater than 0, so we map it.
            intptr_t result = clip->map();
            mutex.unlock();
            if (result != M_OK) {
                Message::reply(&msg, result);
            } else {
                Message::reply(&msg, M_OK);
            }
            break;
        }
        case MSG_CLIPBOARD_GET:
            if (clip == NULL) {
                Message::reply(&msg, M_OBJECT_NOT_FOUND);
            } else {
                Message::reply(&msg, clip->handle(), clip->size());
            }
            break;
        case MSG_CLIPBOARD_CLEAR:
            mutex.lock();
            if (clip!= NULL) {
                delete clip;
            }
            clip = NULL;
            mutex.unlock();
            Message::reply(&msg, M_OK);
            break;
        default:
            break;
        }
    }

    return 0;
}
