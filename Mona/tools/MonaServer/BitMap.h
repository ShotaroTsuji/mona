/*!
    \file  BitMap.h
    \brief class BitMap

    Copyright (c) 2003 HigePon
    WITHOUT ANY WARRANTY

    \author  HigePon
    \version $Revision$
    \date   create:2003/03/30 update:$Date$
*/

#ifndef __BITMAP_H__
#define __BITMAP_H__

/*!
    BitMap claass
*/
class BitMap {

 public:
    BitMap(int number);
    ~BitMap();

 public:
    void mark(int index);
    void clear(int index);
    int find();
    int countClear();
    bool marked(int index);
    int getBitsNumber() const;

  public:
    enum
    {
        NOT_FOUND  = -1,
        DWORD_BITS = sizeof(dword) * 8
    };

  private:
    int* map_;
    int bitsNumber_;
    int dwordNumber_;
};

#endif