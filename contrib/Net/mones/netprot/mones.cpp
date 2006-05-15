/*!
  \file  mones.cpp
  \brief Code:Mones���C��

  Copyright (c) 2004 Yamami
  All rights reserved.
  License=MIT/X License

  \author  Yamami
  \version $Revision$
  \date   create:2004/08/08 update:$Date$
*/


//�����ŁAMones�̃O���[�o���I�u�W�F�N�g���C���X�^���X������
#define MONES_GLOBAL_VALUE_DEFINED
#include "MonesGlobal.h"

#include <monapi.h>
#include <nicdrv/AbstractMonic.h>
#include <nicdrv/MonesFactory.h>
#include "MoEther.h"
#include "MoIcmp.h"
#include "MonesConfig.h"


using namespace MonAPI;


/*!
    \brief MonaMain
        Code:Mones���C��

    \param List<char*>* pekoe

    \author  Yamami
    \date    create:2004/08/08 update:2004/10/31
*/
int MonaMain(List<char*>* pekoe)
{
    
    int ret;
    
    //MonesRList = new HList<MONES_IP_REGIST*>();
    //MONES_IP_REGIST *regist;
    
    //���̃v���Z�X���쒆�́AIO��������x��3�܂ŋ�����
    syscall_get_io();
    
    // NIC�̃C���X�^���X��
    // MonesFactory�N���X�o�R�ŃC���X�^���X������B
    AbstractMonic* insAbstractNic;
    
    MonesFactory* insNicFactory = new MonesFactory();
    insNicFactory->setup();
    insAbstractNic = insNicFactory->getNicInstance();
    //NIC�̃��[�h�Ɏ��s�����ꍇ�́AMones�I��
    if(insAbstractNic == 0){
        printf("NIC Error Mones Quit \n");
        exit(1);
    }
    
    //Ether�N���X�̃C���X�^���X��
    g_MoEther = new MoEther();
    g_MoEther->etherInit(insAbstractNic);

    //ARP�N���X�̃C���X�^���X��
    g_MoArp = new MoArp();
    g_MoArp->initArp(insAbstractNic);

    //IP�N���X�̃C���X�^���X��
    g_MoIp = new MoIp();
    g_MoIp->initIp(insAbstractNic);

    //UDP�N���X�̃C���X�^���X��
    g_MoUdp = new MoUdp();
    g_MoUdp->initUdp(insAbstractNic);

    //TCP�N���X�̃C���X�^���X��
    g_MoTcp = new MoTcp();
    g_MoTcp->initTcp(insAbstractNic);

    //SocketsManager�N���X�̃C���X�^���X��
    g_SocketsManager = new SocketsManager();
    
    // Server start ok
    MessageInfo info;
    dword targetID = Message::lookupMainThread("MONITOR.BIN");
    if (targetID == 0xFFFFFFFF)
    {
        printf("Mones:INIT not found\n");
        exit(1);
    }

    // create message
    Message::create(&info, MSG_SERVER_START_OK, 0, 0, 0, NULL);

    // send
    if (Message::send(targetID, &info)) {
        printf("Mones:INIT error\n");
    }

    //�l�b�g���[�N��IRQ�}�X�NEnable
    insAbstractNic->enableNetWork();

    //IRQ���V�[�o�Ƃ��ēo�^ (IRQ�́ANIC�h���C�o�N���X��蓾��)
    syscall_set_irq_receiver(insAbstractNic->getNicIRQ());

    //Mones�̃v���C�I���e�B������
    syscall_change_base_priority(1);

    /* Message loop */
    //�����Ń��b�Z�[�W���[�v
    for (;;)
    {
        /* receive */
        if (!Message::receive(&info))
        {
            
            switch(info.header)
            {
            
            //NIC�n�[�h�E�F�A����̊��荞�ݗv��
            case MSG_INTERRUPTED:
                
                //logprintf("MSG_INTERRUPTED\n");
                //�p�P�b�g��M���҂����X�g�֓o�^
                insAbstractNic->frame_input_public();
                
                break;

            //�h���C�o�w����̃p�P�b�g�����v��
            case MSG_MONES_FRAME_REQ:
                
                //logprintf("MSG_MONES_FRAME_REQ\n");
                
                //�҂��͂��邩�H
                if(insAbstractNic->waitFrameBufList->isEmpty()){
                    break;
                }
                
                //���L����������҂��p�P�b�g�擾                
                WAIT_FRAME_BUF* bgetWork = insAbstractNic->waitFrameBufList->removeAt(0);
                
                monapi_cmemoryinfo* cmPac;

                cmPac = monapi_cmemoryinfo_new();
                cmPac->Handle = bgetWork->cmHandle;
                //cmPac->Owner  = tid;
                cmPac->Size   = bgetWork->cmSize;
                monapi_cmemoryinfo_map(cmPac);

                dword    i;
                //Ether�N���X�ɓo�^
                i = g_MoEther->setEtherFrame(cmPac->Data ,cmPac->Size);
                //�C�[�T�l�b�g�t���[����M����
                i = g_MoEther->receiveEther();                
                
                //���L���������
                monapi_cmemoryinfo_dispose(cmPac);
                monapi_cmemoryinfo_delete(cmPac);
                
                //���X�g���������
                delete bgetWork;

                break;


            //�A�v�������Mones�֓o�^
            case MSG_MONES_REGIST:
                
                //printf("MSG_MONES_REGIST\n");
                
                //���X�����X�g�ɓo�^
                g_SocketsManager->registLisSocket(info.from , (word)info.arg2);
                
                break;

            //�A�v������̃p�P�b�g���M�v��
            case MSG_MONES_IP_SEND:
                
                //2005/08/09 ������ �ǂ�����H�R�����g UDP�p�P�b�g�g�ݗ��Ă͂ǂ��̎d�����H
                //info.arg1 �� IP�w�b�_�v���g�R��No
                //info.arg2 �� ���L�������n���h��
                //info.arg3 �� ���L�������T�C�Y
                
                //printf("MSG_MONES_IP_SEND\n");
                
                //�p�P�b�g���M
                //�Ƃ肠�����́AMessageInfo ��str 128�o�C�g�܂ŃT�|�[�g
                //����ȏ�̃T�C�Y�́A���L��������p����
                if(info.arg1 == IPPROTO_ICMP){

                    int icmp_size;
                    icmp_size=info.length;

                    dword ip;
                    TRANS_BUF_INFO *tbi;
                    tbi = new TRANS_BUF_INFO();
                    
                    ICMP_HEADER *volatile icmpHead;
                    //icmpHead = new ICMP_HEADER();
                    icmpHead=(ICMP_HEADER*)info.str;
                    
                    
                    //���M�� ����2�̒l
                    ip = info.arg2;
                    
                    //ICMP �`�F�b�N�T���v�Z
                    icmpHead->chksum=0;
                    icmpHead->chksum=MoPacUtl::calcCheckSum((dword*)icmpHead,icmp_size);

                    //���M�o�b�t�@�e�[�u���̐ݒ�
                    tbi->data[2]=NULL;
                    tbi->size[2]=0;
                    tbi->data[1]=(char*)icmpHead;
                    tbi->size[1]=icmp_size;
                    tbi->ipType=IPPROTO_ICMP;
        
                    ret = g_MoIp->transIp(tbi , MoPacUtl::swapLong(ip) ,0, 0);
                    
                    //printf("MoPacUtl::swapLong(ip)=%x\n",MoPacUtl::swapLong(ip));
                }

                //UDP
                if(info.arg1 == IPPROTO_UDP){
                    logprintf("Mones MSG_MONES_IP_SEND In!!! info.arg1 == IPPROTO_UDP\n");
                    
                    SocketContainer *soc;
                    
                    //soc = new SocketContainer();

                    //���L������
                    monapi_cmemoryinfo* cmSoc;

                    cmSoc = monapi_cmemoryinfo_new();
                    cmSoc->Handle = info.arg2;
                    cmSoc->Size   = info.arg3;
                    
                    logprintf("Mones MSG_MONES_IP_SEND cmSoc->Handle=%d  cmSoc->Size=%d\n",cmSoc->Handle  ,cmSoc->Size);
                    
                    int reti;
                    
                    reti = monapi_cmemoryinfo_map(cmSoc);
                    
                    logprintf("Mones MSG_MONES_IP_SEND In!!! monapi_cmemoryinfo_map(cmSoc)\n");
                    logprintf("reti = %d\n",reti);
                    
                    if(reti == 0){
                        break;
                    }
                    
                    //���L����������SocketContainer�����o���B
                    soc = (SocketContainer*)cmSoc->Data;
                    //memcpy(soc,cmSoc->Data,sizeof(SocketContainer));
                    
                    logprintf("Mones MSG_MONES_IP_SEND In!!! soc = (SocketContainer*)cmSoc->Data\n");
                    
                    g_MoUdp->transUdp(soc);
                    
                    //SocketContainer�A���L���������!!
                    monapi_cmemoryinfo_dispose(cmSoc);
                    monapi_cmemoryinfo_delete(cmSoc);

                    
                }

                
                break;

            //ARP�҂������Wake Up
            case MSG_MONES_WAKEUP_ARP_WAIT:
                
                //printf("MSG_MONES_WAKEUP_ARP_WAIT\n");
                
                MAC_REPLY_WAIT* nowWait;
                
                //ARP�v���҂����X�g�̌���
                for (int i = 0; i < g_MoArp->macWaitList->size() ; i++) {
                    nowWait = g_MoArp->macWaitList->get(i);
                    
                    if(nowWait->repFlg == 1){
                        //ARP�����ς݂Ȃ�A�҂��p�P�b�g�𑗐M����B
                        TRANS_BUF_INFO *tbi;
                        tbi = new TRANS_BUF_INFO();
                        
                        //���M�o�b�t�@�e�[�u���̐ݒ�
                        tbi->data[2]=NULL;
                        tbi->size[2]=0;
                        tbi->data[1]=(char*)MonAPI::MemoryMap::map(nowWait->ipPacketBuf01->Handle);
                        tbi->size[1]=nowWait->ipPacketBuf01->Size;
                        tbi->ipType=nowWait->ipType;
                        
                        ret = g_MoIp->transIp(tbi , nowWait->ip ,0, 0);
                        
                        
                        //�Ҕ����Ă���IP�p�P�b�g�o�b�t�@�̉��
                        //free(nowWait->ipPacketBuf);
                        monapi_cmemoryinfo_dispose(nowWait->ipPacketBuf01);
                        monapi_cmemoryinfo_delete(nowWait->ipPacketBuf01);
                        
                        
                        //�҂����X�g����폜
                        g_MoArp->macWaitList->removeAt(i);
                        //�J�E���^�f�N�������g������B
                        i--;
                    }
                }
                
                
                break;

            default:
                /* igonore this message */
                break;
            }

        }
    }
    return 0;
}
