#include<tester.h>
#include<FAT12.h>
#include<FDCDriver.h>
#include<global.h>
#include<io.h>
#include<GraphicalConsole.h>
#include<operator.h>
#include<z.h>
#include<MemoryManager.h>
#include<KeyBoardManager.h>
#include<elf.h>
#include<userlib.h>
#include<SystemInfo.h>

/*!

  \brief  test code for higepon

  Copyright (c) 2002,2003 Higepon
  All rights reserved.
  License=MIT/X Licnese

  \author  HigePon
  \version $Revision$
  \date   create:2003/05/18 update:$Date$
*/


extern "C" void put_pixel(int x, int y, char color);

typedef struct {
    dword a;
    dword b;
    dword width;
    dword height;
} ARGBHeader;

static bool drawARGB(byte* rgba, int x, int y, int size);
static int getColorNumber(byte* rgba);

struct read_info {
    FAT12 *fat;
    int sz;
} read_info;

#define SHARED_MM_ERROR -1
#define FAT_INIT_ERROR  -2
#define FAT_OPEN_ERROR  -3

int send(const char* name, Message* message) {

    ProcessInfo* pinfo;
    Message* kmessage;

    if (message == (Message*)NULL) {
        return -1;
    }

    if ((pinfo = g_process_manager->findProcess(name)) == (ProcessInfo*)NULL) {
        return -1;
    }

    if ((kmessage = (Message*)malloc(sizeof(Message))) == NULL) {
        return -1;
    }

    memcpy(kmessage, message, sizeof(Message));
    pinfo->vec->add(kmessage);
    return 0;
}

int receive(Message* message) {

    Message* from = g_current_process->vec->get(0);

    if (from == (Message*)NULL) {
        return -1;
    }

    memcpy(message, from, sizeof(Message));
    free(from);
    g_current_process->vec->remove(0);
    return 0;
}

int loadProcess(const char* path, const char* file, bool isUser) {

    static dword sharedId = 0x1000;
    sharedId++;

    int    fileSize;
    int    readTimes;
    byte*  buf;
    bool   isOpen;
    bool   isAttaced;
    FAT12* fat;

    //    g_console->printf("load prcess 1");
    while (Semaphore::down(&g_semaphore_shared));
    isOpen = SharedMemoryObject::open(sharedId, 4096 * 5);
    //    g_console->printf("load prcess 2");
    isAttaced = SharedMemoryObject::attach(sharedId, g_current_process, 0x80000000);
    //    g_console->printf("load prcess 3");
    Semaphore::up(&g_semaphore_shared);

    if (!isOpen || !isAttaced) return SHARED_MM_ERROR;

    g_fdcdriver = new FDCDriver();
    g_fdcdriver->motor(ON);
    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();

    g_console->printf("load prcess 4");

    fat = new FAT12((DiskDriver*)g_fdcdriver);
    g_console->printf("load prcess 4.5");
    if (!fat->initilize()) return FAT_INIT_ERROR;
    g_console->printf("load prcess 5");
    if (!fat->open(path, file, FAT12::READ_MODE)) return FAT_OPEN_ERROR;

    g_console->printf("load prcess 6");

    fileSize  = fat->getFileSize();
    readTimes = fileSize / 512 + (fileSize % 512 ? 1 : 0);
    buf = (byte*)malloc(512 * readTimes);
    if (buf == NULL) return -1;

    g_console->printf("load prcess 7[%d]", readTimes);
    for (int i = 0; i < readTimes; i++) {
	//    SystemInfo::rdtsc();//
        if (!fat->read(buf + 512 * i)) {
            g_console->printf("read failed %d", i);
            while (true);
        }
	//    SystemInfo::rdtscsub();
	//    if (!(SystemInfo::timeH)) g_console->printf("time=[%x]", SystemInfo::timeL);
    }
    g_console->printf("load prcess 8");



    if (!fat->close()) {
        info(ERROR, "close failed");
    }

    ELFLoader* loader = new ELFLoader();
    g_console->printf("elf size = %d", loader->prepare((dword)buf));

    //    g_console->printf("load prcess 9");

    dword entrypoint = loader->load((byte*)0x80000000);

    //    g_console->printf("load prcess 10");

    delete(loader);
    free(buf);

    //    g_console->printf("load prcess 11");

    Process* process1 = isUser ? new UserProcess(file) : new Process(file);

    while (Semaphore::down(&g_semaphore_shared));
    isOpen = SharedMemoryObject::open(sharedId, 4096 * 5);
    isAttaced = SharedMemoryObject::attach(sharedId, &(process1->pinfo_), 0xA0000000);
    Semaphore::up(&g_semaphore_shared);
    if (!isOpen || !isAttaced) panic("loadProcess: not open");

    g_process_manager->addProcess(process1, entrypoint);

    //    while (Semaphore::down(&g_semaphore_shared));
    //    SharedMemoryObject::detach(sharedId, g_current_process);
    //    Semaphore::up(&g_semaphore_shared);

    //    g_console->printf("load prcess 12");
    return 0;
}

void userKeyStrokeHandler() {

    KeyBoardManager& km = KeyBoardManager::instance();
    KeyInfo* info = km.getKeyInfo();

    g_console->printf("keycode=[%d], modifiers=[%d]", info->keycode, info->modifiers);
}

void keyStrokeTest() {

    KeyBoardManager& km = KeyBoardManager::instance();
    km.setHandler(userKeyStrokeHandler);
}

int read(input_stream *p, int sz)
{
    if (0 >= read_info.sz)
        return 0;

    if (!read_info.fat->read(p->bf)) {
        g_console->printf("error read\n");
        return 0;
    }

    if (512 > read_info.sz)
        return read_info.sz;

    read_info.sz -= 512;

    return 512;
}

int write(output_stream *p, int sz)
{
    *((int*)(p)->data) = sz;

    return sz;
}


bool drawARGB(byte* rgba, int x, int y, int size) {

    int startx = x;
    int starty = y;
    ARGBHeader* header = (ARGBHeader*)rgba;

    /* check identifier */
    if (header->a != 0x41524742) return false;

    /* draw loop */
    for (int i = sizeof(ARGBHeader) / 4; i < size / 4; i++) {

        if (x >= startx + (int)(header->width)) {

            y++;
            x = startx;
        }

        if (y >= starty + (int)(header->height)) break;

        put_pixel(x, y, getColorNumber(&(rgba[i * 4])));
        x++;
    }

    return true;
}

int getColorNumber(byte* rgba) {

    byte r = rgba[2];
    byte g = rgba[1];
    byte b = rgba[0];
    int result;

    if (r == 0x00 && g == 0x00 && b == 0x00) result = 0;
    else if (r == 0x00 && g == 0x00 && b == 0x80) result = 1;
    else if (r == 0x00 && g == 0x80 && b == 0x00) result = 2;
    else if (r == 0x00 && g == 0x80 && b == 0x80) result = 3;
    else if (r == 0x80 && g == 0x00 && b == 0x00) result = 4;
    else if (r == 0x80 && g == 0x00 && b == 0x80) result = 5;
    else if (r == 0x80 && g == 0x80 && b == 0x00) result = 6;
    else if (r == 0xc0 && g == 0xc0 && b == 0xc0) result = 7;
    else if (r == 0x80 && g == 0x80 && b == 0x80) result = 8;
    else if (r == 0x00 && g == 0x00 && b == 0xff) result = 9;
    else if (r == 0x00 && g == 0xff && b == 0x00) result = 10;
    else if (r == 0x00 && g == 0xff && b == 0xff) result = 11;
    else if (r == 0x00 && g == 0xff && b == 0xff) result = 12;
    else if (r == 0xff && g == 0x00 && b == 0x00) result = 13;
    else if (r == 0xff && g == 0x00 && b == 0xff) result = 14;
    else result = 15;

    return result;
}

void mmChangeTester() {

    g_info_level = MSG;

    /* test1 FD read */
    g_fdcdriver = new FDCDriver();

    g_fdcdriver->motor(ON);

    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();

    FAT12* fat = new FAT12((DiskDriver*)g_fdcdriver);
    if (!fat->initilize()) {

        int errorNo = fat->getErrorNo();

        if (errorNo == FAT12::BPB_ERROR) info(ERROR, "BPB read  error \n");
        else if (errorNo == FAT12::NOT_FAT12_ERROR) info(ERROR, "NOT FAT12 error \n");
        else if (errorNo == FAT12::FAT_READ_ERROR) info(ERROR, "NOT FAT12 error \n");
        else info(ERROR, "unknown error \n");

        info(ERROR, "fat initilize faild\n");
        while (true);
    }

    if (!fat->open(".", "OKU.LGO", FAT12::READ_MODE)) {

        info(ERROR, "open failed");
    }

    int fileSize  = fat->getFileSize();
    int readTimes = fileSize / 512 + (fileSize % 512 ? 1 : 0);

    byte* buf = (byte*)malloc(512 * readTimes);

    for (int i = 0; i < readTimes; i++) {

        if (!fat->read(buf + 512 * i)) {

            info(ERROR, "read failed %d", i);
        }
    }


    drawARGB(buf, 0, 0, fileSize);

    if (!fat->close()) {
        info(ERROR, "close failed");
    }

    g_fdcdriver->motor(false);

    delete(fat);
    free(buf);

}

void FDCDriverTester() {

    g_info_level = MSG;

    g_fdcdriver = new FDCDriver();
    g_fdcdriver->motor(ON);

    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();

    FAT12* fat = new FAT12((DiskDriver*)g_fdcdriver);

    if (!fat->initilize()) {
        g_console->printf("error fat initialize\n");
        g_fdcdriver->motor(false);
        return;
    }

    if (!fat->open(".", "LOGO.Z", FAT12::READ_MODE)) {
        g_console->printf("error open mona.z\n");
        g_fdcdriver->motor(false);
        return;
    }

    read_info.fat = fat;
    read_info.sz = fat->getFileSize();

    unsigned char *bf = (unsigned char*)malloc(512);
    if (NULL == bf) {
        g_console->printf("not enough memory\n");
        g_fdcdriver->motor(false);
        return;
    }

    input_stream is;
    is.bf = bf;
    is.sz = 512;
    is.read = read;

    int bf_size = 1300000;
    bf = (unsigned char*)malloc(bf_size);
    if (NULL == bf) {
        g_console->printf("not enough memory\n");
        g_fdcdriver->motor(false);
        return;
    }

    int image_size;
    output_stream os;
    os.bf = bf;
    os.sz = bf_size;
    os.write = write;
    os.data = &image_size;

    decode(&is, &os);
    drawARGB(bf, 0, 0, image_size);

    if (!fat->close()) {
        g_console->printf("error close\n");
    }

    g_fdcdriver->motor(false);
    delete(fat);
}

void ELFTester(byte* out) {

    g_fdcdriver = new FDCDriver();

    byte tbuf[512];
    for (int i = 0; i < 0xff; i++) {tbuf[i] = i;}
    for (int i = 0xff; i < 512; i++){ tbuf[i] = 512 - i;}

    g_fdcdriver->motor(false);

    for (int i = 0; i< 20; i++) {
        delay(1);
        delay(1);
    }

    g_fdcdriver->motor(true);
    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();

    for (int i = 1; i < 25; i++) {
        memset(tbuf, 0x99, 512);
        g_fdcdriver->read(i, tbuf);

    }

    FAT12* fat = new FAT12((DiskDriver*)g_fdcdriver);
    if (!fat->initilize()) {

        int errorNo = fat->getErrorNo();

        if (errorNo == FAT12::BPB_ERROR) info(ERROR, "BPB read  error \n");
        else if (errorNo == FAT12::NOT_FAT12_ERROR) info(ERROR, "NOT FAT12 error \n");
        else if (errorNo == FAT12::FAT_READ_ERROR) info(ERROR, "NOT FAT12 error \n");
        else info(ERROR, "unknown error \n");

        info(ERROR, "fat initilize faild\n");
        while (true);
    }

    if (!fat->open(".", "USER.ELF", FAT12::READ_MODE)) {

        info(ERROR, "open failed");
    }

    for (int i = 0; i < 10; i++) {

        if (!fat->read(out + 512 * i)) {

            info(ERROR, "read failed %d", i);
        }

    }

    if (!fat->close()) {
        info(ERROR, "close failed");
    }

    g_console->printf("load done...USER.ELF\n");
    g_fdcdriver->motor(false);

}




void FDCTester() {

    info(DEV_NOTICE, "start1");

    g_fdcdriver = new FDCDriver();

    info(DEV_NOTICE, "start2");

    byte tbuf[512];
    for (int i = 0; i < 0xff; i++) {tbuf[i] = i;}
    for (int i = 0xff; i < 512; i++){ tbuf[i] = 512 - i;}

    g_fdcdriver->motor(true);

    info(DEV_NOTICE, "before recalibrate");

    g_fdcdriver->recalibrate();

    info(DEV_NOTICE, "before read");
    for (int i = 0; i < 10; i++) {
        memset(tbuf, 0x99, 512);
        g_fdcdriver->read(1, tbuf);

    }

    info(DEV_NOTICE, "after read");

    //      // write
    //      for (int i = 0; i < 73; i++) {

    //          g_console->printf("write");
    //          memset(tbuf, i + 5, 512);
    //          if (!g_fdcdriver->write(i, tbuf)) {

    //              g_console->printf("write failed %d", i);
    //              //            g_fdcdriver->motor(false);
    //              //            while (true);
    //          }
    //      }

    //      memset(tbuf, 0x99, 512);
    //      if (!g_fdcdriver->read(0, tbuf)) {
    //           g_console->printf("read failed %d", 50);
    //           g_fdcdriver->motor(false);
    //           while (true);
    //      }
    //      for (int i = 0; i < 512; i++) g_console->printf("[%d]", tbuf[i]);

    //      while (g_demo_step < 8);

    //      memset(tbuf, 0x99, 512);
    //      if (!g_fdcdriver->read(1, tbuf)) {
    //           g_console->printf("read failed %d", 50);
    //           g_fdcdriver->motor(false);
    //           while (true);
    //      }
    //      for (int i = 0; i < 512; i++) g_console->printf("[%d]", tbuf[i]);
    //      while (true);

    //      g_fdcdriver->motor(false);
    //      g_console->printf("ok");
    //      while (true);

    FAT12* fat = new FAT12((DiskDriver*)g_fdcdriver);
    if (!fat->initilize()) {

        int errorNo = fat->getErrorNo();

        if (errorNo == FAT12::BPB_ERROR) info(ERROR, "BPB read  error \n");
        else if (errorNo == FAT12::NOT_FAT12_ERROR) info(ERROR, "NOT FAT12 error \n");
        else if (errorNo == FAT12::FAT_READ_ERROR) info(ERROR, "NOT FAT12 error \n");
        else info(ERROR, "unknown error \n");

        info(ERROR, "fat initilize faild\n");
        while (true);
    }

    info(MSG, "initilize OK\n");
    info(MSG, "changeDirectory to SOMEDIR\n");

    if (!fat->changeDirectoryRelative("SOMEDIR")) {
        info(ERROR, "some dir not found");
        while (true);
    }

    info(MSG, "changeDirectoryRelative OK\n");
    info(MSG, "try to create file hige.cpp\n");

    if (!fat->createFlie("HIGE", "CPP")) {

        info(ERROR, "can not create file=%d", fat->getErrorNo());
        while (true);
    }

    info(MSG, "try to open file hige.cpp\n");
    if (!fat->open(".", "HIGE.CPP", FAT12::WRITE_MODE)) {

        info(ERROR, "open failed");
    }

    info(MSG, "try to write to hige.cpp 'M' * 512\n");
    byte text[512];
    memset(text, 'M', 512);
    if (!fat->write(text)) {

        info(ERROR, "write failed");
    }

    info(MSG, "try to write to hige.cpp 'o' * 512\n");
    memset(text, 'o', 512);
    if (!fat->write(text)) {

        info(ERROR, "write failed");
    }

    info(MSG, "try to write to hige.cpp 'n' * 512\n");
    memset(text, 'n', 512);
    if (!fat->write(text)) {

        info(ERROR, "write failed");
    }

    info(MSG, "try to write to hige.cpp 'a' * 512\n");
    memset(text, 'a', 512);
    if (!fat->write(text)) {

        info(ERROR, "write failed");
    }

    info(MSG, "try to close  hige.cpp\n");

    if (!fat->close()) {
        info(ERROR, "close failed");
    }

    g_console->printf("\nHit any key to start [kernel thread demo]\n");
    g_fdcdriver->motor(false);

    info(MSG, "test done!! Boot Windows & read floppy disk \\somedir\\hige.cpp\n");
}
