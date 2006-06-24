/*!
    \file  vnode.h
    \brief vnode

    Copyright (c) 2006 Higepon
    WITHOUT ANY WARRANTY

    \author  HigePon
    \version $Revision$
    \date   create:2006/05/24 update:$Date$
*/

#ifndef _VNODE_
#define _VNODE_

#include <map>
#include "monapi/CString.h"
#include "monapi/messages.h"
#include "FileSystem.h"
#include "types.h"


// enum vtype
// {
//     VNON,
//     VREG,
//     VDIR,
//     VBLK,
//     VCHR,
//     VLNK,
//     VSOCK,
//     VFIFO,
//     VBAD
// };

// typedef struct vnode
// {
//     enum vtype v_type;
//     FileSystem* fs;
//     void* fnode;
// };

namespace io
{

class Context
{
public:
    Context() : memory(NULL), offset(0), size(0) {}
    virtual ~Context()
    {
        if (NULL == memory) return;
        monapi_cmemoryinfo_dispose(memory);
        monapi_cmemoryinfo_delete(memory);
    }

    dword tid;
    monapi_cmemoryinfo* memory;
    dword offset;
    dword origin;
    dword size;
    dword resultSize;
};

typedef struct
{
    Vnode* vnode;
    io::Context context;
} FileInfo;

};
class Vnode
{
public:
    int type;
    FileSystem* fs;
    void* fnode;
    enum
    {
        NONE,
        REGULAR,
        DIRECTORY,
        LINK,
        BAD
    };
    Vnode* mountedVnode;
};




#endif // _VNODE_
