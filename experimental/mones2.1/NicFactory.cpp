#include <pci/Pci.h>
#include "NicFactory.h"
#include "NE2000.h"
#include "MonAMDpcn.h"

// based on MonesLoader by Yamami

using namespace mones;

Nic* NicFactory::create()
{
    PciInf pciinfo;
    Pci* pcilib = new Pci();
    Nic* nic;
    printf("\nIP address is defined in NicFactory::create(). Rewrite it as your environment.\n");
    pcilib->CheckPciExist(MonAMDpcn::VENDORID,MonAMDpcn::DEVICEID,&pciinfo);
    if( pciinfo.Exist== 0){
        // 0x04 is PCI_COMMAND
        dword val=pcilib->ReadConfig(0, pciinfo.DeviceNo, 0, 0x04,2);
        pcilib->WriteConfig(0,pciinfo.DeviceNo,0,0x04,2,val|0x4);
        nic=new MonAMDpcn();
        nic->setIP(172,16,177,4);  //Vmware. 
        nic->setIRQ(pciinfo.IrqLine);
        nic->setIOBase(pciinfo.BaseAd);
        nic->init();
        delete pcilib;
        return nic;
    }

    //TODO delete magic number.
    pcilib->CheckPciExist(0x10EC,0x8029,&pciinfo);
    if( pciinfo.Exist == 0){
        nic = new NE2000();
        nic->setIP(172,16,150,4); //QEMU.
        nic->setIRQ(pciinfo.IrqLine);
        nic->setIOBase(pciinfo.BaseAd);
        nic->init();
        delete pcilib;
        return nic;
    }

    delete pcilib;
    //default
    nic = new NE2000();
    //Bochs
    nic->setIP(172,16,110,4);
    nic->setIRQ(3);
    nic->setIOBase(0x240);
    nic->init();
    return nic;
}