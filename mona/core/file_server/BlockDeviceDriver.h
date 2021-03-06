/*
 *   Copyright (c) 2010  Higepon(Taro Minowa)  <higepon@users.sourceforge.jp>
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _BLOCK_DEVICE_DRIVER_
#define _BLOCK_DEVICE_DRIVER_

#include <monapi.h>
#include <drivers/virtio/VirtioBlock.h>

class BlockDeviceDriver : public IStorageDevice
{
public:
    BlockDeviceDriver(int deviceIndex) :
        vb_(VirtioBlock::probe(deviceIndex)),
        sectorSize_(512)
    {
        ASSERT(vb_.get() != NULL);
    }

    BlockDeviceDriver(int deviceIndex, size_t sectorSize) :
        vb_(VirtioBlock::probe(deviceIndex)),
        sectorSize_(sectorSize)
    {
        ASSERT(vb_.get() != NULL);
    }

    virtual ~BlockDeviceDriver()
    {
    }

    int open()
    {
        return M_OK;
    }

    int close()
    {
        return M_OK;
     }

    // ISO9660 assume sector size 2048, but our block device driver 512.
    int read(uint32_t lba, void* buf, int size)
    {
        int64_t sizeRead = vb_->read(buf, lba * (sectorSize_ / 512), size);
        if (size == sizeRead) {
            return M_OK;
        } else {
            if (sizeRead < 0) {
                return sizeRead;
            } else {
                return M_READ_ERROR;
            }
        }
    }

    int write(uint32_t lba, const void* buf, int size)
    {
        if (size == vb_->write(buf, lba * (sectorSize_ / 512), size)) {
            return M_OK;
        } else {
            return M_WRITE_ERROR;
        }
    }

    int ioctl(void* p)
    {
        return M_NOT_SUPPORTED;
    }
private:
    MonAPI::scoped_ptr<VirtioBlock> vb_;
    const size_t sectorSize_;
};

#endif
