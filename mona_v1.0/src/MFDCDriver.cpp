/*!
    \file  MFDCDriver.cpp
    \brief class Floppy Disk Controller for MultiTask

    Copyright (c) 2003 HigePon
    WITHOUT ANY WARRANTY

    \author  HigePon
    \version $Revision$
    \date   create:2003/02/07 update:$Date$
*/

#include<MFDCDriver.h>
#include<monaIo.h>
#include<monaVga.h>
#include<monaKernel.h>
#include<monaOperator.h>

/* definition DOR */
#define FDC_MOTA_START  0x10
#define FDC_DMA_ENABLE  0x08
#define FDC_REST_RESET  0x00
#define FDC_REST_ENABLE 0x04
#define FDC_DR_SELECT_A 0x00

/* definition MSR */
#define FDC_MRQ_READY    0x80
#define FDC_DIO_TO_CPU   0x40
#define FDC_NDMA_NOT_DMA 0x20
#define FDC_BUSY_ACTIVE  0x10
#define FDC_ACTD_ACTIVE  0x08
#define FDC_ACTC_ACTIVE  0x04
#define FDC_ACTB_ACTIVE  0x02
#define FDC_ACTA_ACTIVE  0x01

/* definition on/off */
#define ON  true
#define OFF false

/* port address */
#define FDC_DOR_PRIMARY   0x3f2
#define FDC_DOR_SECONDARY 0x372
#define FDC_MSR_PRIMARY   0x3f4
#define FDC_MSR_SECONDARY 0x374
#define FDC_DR_PRIMARY    0x3f5
#define FDC_DR_SECONDARY  0x375
#define FDC_CCR_PRIMARY   0x3f7
#define FDC_CCR_SECONDARY 0x377

#define FDC_DMA_S_SMR     0x04
#define FDC_DMA_S_MR      0x0b
#define FDC_DMA_S_CBP     0x0c
#define FDC_DMA_S_BASE    0x04
#define FDC_DMA_S_COUNT   0x05
#define FDC_DMA_PAGE2     0x81

/* summary */
#define FDC_DOR_RESET   0
#define FDC_START_MOTOR (FDC_DMA_ENABLE | FDC_MOTA_START | FDC_REST_ENABLE | FDC_DR_SELECT_A)
#define FDC_STOP_MOTOR  (FDC_DMA_ENABLE | FDC_REST_RESET | FDC_DR_SELECT_A)

/* FDC Commands */
#define FDC_COMMAND_SEEK            0x0f
#define FDC_COMMAND_SENSE_INTERRUPT 0x08
#define FDC_COMMAND_SPECIFY         0x03
#define FDC_COMMAND_READ            0x46

/* time out */
#define FDC_RETRY_MAX 6000000

/* delay */
#define delay() inportb(0x80);

#define FDC_DMA_BUFF_SIZE 512

MFDCDriver* gMFDCDriver = 0;
bool MFDCDriver::interrupt_ = false;
VirtualConsole* MFDCDriver::console_;
byte* MFDCDriver::dmabuff_;

/*!
    \brief Constructer

    \param  console message out console
    \author HigePon
    \date   create:2003/02/03 update:
*/
MFDCDriver::MFDCDriver(VirtualConsole* console) {

    /* set console */
    console_ = console;

    initilize();
    while (true);
}

/*!
    \brief Destructer

    \author HigePon
    \date   create:2003/02/03 update:
*/
MFDCDriver::~MFDCDriver() {

    motor(OFF);
    return;
}

/*!
    \brief initilize controller

    \author HigePon
    \date   create:2003/02/03 update:
*/
void MFDCDriver::initilize() {

    byte specifyCommand[] = {FDC_COMMAND_SPECIFY
                           , 0xC1 /* SRT = 4ms HUT = 16ms */
                           , 0x10 /* HLT = 16ms DMA       */
                            };

    /* allocate dma buffer */
    dmabuff_ = (byte*)malloc(FDC_DMA_BUFF_SIZE);
    console_->printf("dma buff_=[%dkb]\n", ((dword)dmabuff_/1024));

    /* dma buff should be 64kb < dma buff < 16Mb
    if (!dmabuff_ || (dword)dmabuff_ < 64 * 1024 || (dword)dmabuff_  + FDC_DMA_BUFF_SIZE > 16 * 1024 * 1024) {
        panic("dma buff allocate error");
    }

    /* setup DMAC */
    outportb(0xda, 0);
    outportb(0x08, 0);
    outportb(0xd6, 0xc0);
    outportb(0xd4, 0);

    /* specify */
    sendCommand(specifyCommand, sizeof(specifyCommand));

    /* reset drive */
    outportb(FDC_DOR_PRIMARY, FDC_DOR_RESET);
    delay();
    outportb(FDC_CCR_PRIMARY, 0);
    delay();

    /* start driveA */
    interrupt_ = false;
    motor(ON);
    while (!waitInterrupt());

    /* recalibrate */
    recalibrate();
    motor(OFF);

    /* test */
    interrupt_ = false;
    motor(ON);
    while (!waitInterrupt());

    /* seek test */
    //    seek(3);
    printStatus("after seek");


    read(0, 0, 1);

    motor(OFF);

    return;
}

/*!
    \brief print status of FDC(MSR)

    \param  str message
    \author HigePon
    \date   create:2003/02/10 update:
*/
void MFDCDriver::printStatus(const char* str) const {

    byte msr = inportb(FDC_MSR_PRIMARY);
    console_->printf("data reg |data flow|DMA|BUSY| D | C | B | A |int|\n");
    console_->printf("%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n"
              , msr & FDC_MRQ_READY    ? "  ready  ":"not ready"
              , msr & FDC_DIO_TO_CPU   ? " to CPU  ":" to Cont "
              , msr & FDC_NDMA_NOT_DMA ? " Y "      :" N "
              , msr & FDC_BUSY_ACTIVE  ? "  Y "     :"  N "
              , msr & FDC_ACTD_ACTIVE  ? " Y "      :" N "
              , msr & FDC_ACTC_ACTIVE  ? " Y "      :" N "
              , msr & FDC_ACTB_ACTIVE  ? " Y "      :" N "
              , msr & FDC_ACTA_ACTIVE  ? " Y "      :" N "
              , interrupt_ ? " T " : " F "
              , str
    );
    return;
}

/*!
    \brief interrupt handler

    \author HigePon
    \date   create:2003/02/10 update:
*/
void MFDCDriver::interrupt() {

    interrupt_ = true;
}

/*!
    \brief wait interrupt

    \return true:interrupt
    \author HigePon
    \date   create:2003/02/10 update:
*/
bool MFDCDriver::waitInterrupt() {

    return interrupt_;
}

/*!
    \brief print status of FDC

    \param  on ON/OFF
    \author HigePon
    \date   create:2003/02/10 update:
*/
void MFDCDriver::motor(bool on) {

    if (on) outportb(FDC_DOR_PRIMARY, FDC_START_MOTOR);
    else outportb(FDC_DOR_PRIMARY, FDC_STOP_MOTOR);
    return;
}

bool MFDCDriver::sendCommand(const byte* command, const byte length) {

    //    console_->printf("length=[%d]\n", length);
    //    for (int j = 0; j < length; j++) console_->printf("command[%d]=%x\n", j, command[j]);

    /* send command */
    for (int i = 0; i < length; i++) {

        /* expected condition is ready & date I/O to Controller */
        if (!checkMSR(FDC_MRQ_READY, FDC_MRQ_READY)) {

            console_->printf("MFDCDriver#sendCommand: timeout command[%d]\n", i);
            return false;
        }
        console_->printf("command[%x] ", command[i]);
        outportb(FDC_DR_PRIMARY, command[i]);
    }

    return true;
}

/*!
    \brief recalibrate

    \return true OK/false command fail
    \author HigePon
    \date   create:2003/02/10 update:
*/
bool MFDCDriver::recalibrate() {

    byte command[] = {0x07, 0x00}; /* recalibrate */

    interrupt_ = false;
    if(!sendCommand(command, sizeof(command))){

        console_->printf("MFDCDriver#recalibrate:command fail\n");
        return false;
    }

    // comment out for bochs
    //    while(!waitInterrupt());

    senseInterrupt();
    printStatus("after sense");
    return true;
}

/*!
    \brief wait until FDC is ready.

    \param  expectedcondition
    \return true ready/false time out
    \author HigePon
    \date   create:2003/02/10 update:
*/
bool MFDCDriver::checkMSR(byte expectedCondition, byte mask) {

    bool isOK   = false;
    byte status = 0;

    /* check whether FDC is expected condition */
    for (dword i = 0; i < FDC_RETRY_MAX; i++) {

       status = inportb(FDC_MSR_PRIMARY);
       isOK = (status & mask) == expectedCondition;
       if (isOK) return true;
    }

    /* time out */
    console_->printf("MFDCDriver#checkMSR expectedCondition=[%x] result=[%x] masked=[%x]\n"
                   , expectedCondition, inportb(FDC_MSR_PRIMARY), status);
    return false;
}


/*!
    \brief wait until FDC is ready.

    \param  expectedcondition
    \return true ready/false time out
    \author HigePon
    \date   create:2003/02/10 update:
*/
bool MFDCDriver::checkMSR(byte expectedCondition) {

    return (checkMSR(expectedCondition, 0xffff));
}

/*!
    \brief seek

    \param  track
    \return true OK/false time out
    \author HigePon
    \date   create:2003/02/11 update:
*/
bool MFDCDriver::seek(byte track) {

    byte command[] = {FDC_COMMAND_SEEK, 0, track};

    if(!sendCommand(command, sizeof(command))){

        console_->printf("MFDCDriver#seek:command fail\n");
        return false;
    }

    if (!senseInterrupt()) {

        console_->printf("MFDCDriver#seek:command fail\n");
        return false;
    }
    return true;
}

/*!
    \brief Sense Interrrupt Command

    \author HigePon
    \date   create:2003/02/13 update:
*/
bool MFDCDriver::senseInterrupt() {

    byte command[] = {FDC_COMMAND_SENSE_INTERRUPT};

    if(!sendCommand(command, sizeof(command))){

        console_->printf("MFDCDriver#senseInterrrupt:command fail\n");
        return false;
    }

    readResults();
    return true;
}

/*!
    \brief result phase

    \author HigePon
    \date   create:2003/02/13 update:
*/
void MFDCDriver::readResults() {

    int i;
    byte msr = 0;
    resultsLength_ = 0;

    for (i = 0; i < 10; i++) {

        /* expected condition is ready & data I/O to CPU */
        if (!checkMSR(FDC_MRQ_READY, FDC_MRQ_READY)) {

            console_->printf("MFDCDriver#readResults: timeout results_[%d]\n", i);
            break;
        }

        /* no result */
        if (!(msr = inportb(FDC_MSR_PRIMARY) & FDC_DIO_TO_CPU)) break;

        /* get result */
        results_[i] = inportb(FDC_DR_PRIMARY);
    }
    resultsLength_ = i;

    for (int j = 0; j < resultsLength_; j++) {

         _sys_printf("result[%d] = %x ", j, results_[j]);
    }
    return;
}

/*!
    \brief start dma

    \author HigePon
    \date   create:2003/02/15 update:
*/
void MFDCDriver::startDMA() {

    /* mask channel2 */
    outportb(FDC_DMA_S_SMR, 0x06);
    return;
}

/*!
    \brief stop dma

    \author HigePon
    \date   create:2003/02/15 update:
*/
void MFDCDriver::stopDMA() {

    /* unmask channel2 */
    outportb(FDC_DMA_S_SMR, 0x02);
    return;
}

/*!
    \brief setup dmac for read

    \author HigePon
    \date   create:2003/02/15 update:
*/
void MFDCDriver::setupDMARead(dword size) {

    dword p = (dword)dmabuff_;

    stopDMA();

    /* direction write */
    outportb(FDC_DMA_S_MR, 0x46);

    disableInterrupt();

    /* clear byte pointer */
    outportb(FDC_DMA_S_CBP, 0);
    outportb(FDC_DMA_S_BASE,  byte(p & 0xff));
    outportb(FDC_DMA_S_BASE,  byte((p >> 8) & 0xff));
    outportb(FDC_DMA_S_COUNT, byte(size & 0xff));
    outportb(FDC_DMA_S_COUNT, byte(size >>8));
    outportb(FDC_DMA_PAGE2  , byte((p >>16)&0xFF));

    enableInterrupt();

    startDMA();
    return;
}

/*!
    \brief setup dmac for write

    \author HigePon
    \date   create:2003/02/15 update:
*/
void MFDCDriver::setupDMAWrite(dword size) {

    stopDMA();

    startDMA();
    return;
}

bool MFDCDriver::read(byte track, byte head, byte sector) {

    byte command[] = {0x46//FDC_COMMAND_READ
                   , (head & 1) << 2
                   , track
                   , head
                   , sector
                   , 0x02
                   , 3
                   , 0x1b
                   , 0x00
                   };

    seek(track);
    printStatus("seek:track");
    setupDMARead(512);

    memset(dmabuff_, 0xfffe, 512);
    interrupt_ = false;
    sendCommand(command, sizeof(command));
    //while(!waitInterrupt());

    printStatus("after readcommand");

    stopDMA();

    for (int i = 0; i < 10; i++) _sys_printf("dmabuff%d", dmabuff_[i]);

    readResults();
    motor(OFF);
    while (true);

    return true;
}

bool MFDCDriver::writeID(byte track, byte head, byte data) {

    seek(track);


    return false;
}
