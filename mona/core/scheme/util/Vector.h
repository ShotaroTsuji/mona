/*!
    \file  Vector.h
    \brief 

    Copyright (c) 2006-2007 Higepon
    WITHOUT ANY WARRANTY

    \author  Higepon
    \version $Revision$
    \date   create:2007/07/14 update:$Date$
*/

#ifndef __UTIL_VECTOR_H__
#define __UTIL_VECTOR_H__

#ifdef USE_BOEHM_GC
#include "gc_cpp.h"
#include "gc_allocator.h"
extern bool g_gc_initialized;
#elif defined(USE_MONA_GC)
extern bool g_gc_initialized;
#include "../gc/gc.h"
#endif

namespace util {
/*----------------------------------------------------------------------
    Vector Class
----------------------------------------------------------------------*/
#ifdef USE_BOEHM_GC
template <class T> class Vector : public gc_cleanup
#else
template <class T> class Vector
#endif
{
  public:
    Vector();
    Vector(int size);
    Vector(int size, int increase);
    virtual ~Vector();

  public:
    void add(T element);
    T get(int index) const;
    T operator[](int index) const;
    void set(int index, T element);
    void insert(int index, T element);
    void clear();
    T removeAt(int index);
    T remove(T element);
    int size() const;
    bool isEmpty() const;
    bool hasElement(T element) const;
    void reverse();

  private:
    T* data_;         /*! internal array     */
    int size_;        /*! size of liset      */
    int numElements_; /*! number of elements */
    int increase_;    /*! increase           */

    /* initilize */
    void init(int size, int increase);
};

/*!
    \brief constructor

    constructor default size is 5

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> Vector<T>::Vector()
{
    init(5, 5);
    return;
}

/*!
    \brief constructor

    constructor

    \param size size of initial size of list

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> Vector<T>::Vector(int size)
{
    init(size, 5);
    return;
}

/*!
    \brief constructor

    constructor

    \param size size of initial size of list
    \param increase when resize this value used

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> Vector<T>::Vector(int size, int increase)
{
    init(size, increase);
    return;
}

/*!
    \brief destructor

    destructor

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> Vector<T>::~Vector()
{
#ifndef MONASH_DONT_FREE_MEMORY
    /* release memory */
    delete[] data_;
#endif
    return;
}

/*!
    \brief clear

    default size is 5

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> void Vector<T>::clear()
{
    init(5, 5);
    return;
}

/*!
    \brief isEmpty

    return is Empty or not

    \return true/false empty/has elements

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> bool Vector<T>::isEmpty() const
{
    return numElements_ == 0;
}

/*!
    \brief add element

    add element at the end of array

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> void Vector<T>::add(T element)
{
    /* if array is full */
    if (size_ == numElements_)
    {
        /* resize array */
        size_ += increase_;
#ifdef USE_BOEHM_GC
        T* temp = new(GC) T[size_];
#else
        T* temp = new T[size_];
#endif

        /* optimize ? */
        int numElements = numElements_;

        /* copy original to new array */
        for (int i = 0; i < numElements; i++)
        {
            temp[i] = data_[i];
        }

#ifndef MONASH_DONT_FREE_MEMORY
        delete[] data_;
#endif
        data_ = temp;
    }

    /* add element */
    data_[numElements_] = element;
    numElements_++;
    return;
}

/*!
    \brief insert element

    insert element to index

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> void Vector<T>::insert(int index, T element)
{
    /* if array is full */
    if (size_ == numElements_)
    {
        /* resize array */
        size_ += increase_;
#ifdef USE_BOEHM_GC
        T* temp = new(GC) T[size_];
#else
        T* temp = new T[size_];
#endif

        /* optimize ? */
        int numElements = numElements_;

        /* copy original to new array */
        for (int i = 0; i < numElements; i++)
        {
            temp[i] = data_[i];
        }

#ifndef MONASH_DONT_FREE_MEMORY
        delete[] data_;
#endif
        data_ = temp;
    }

    for (int i = numElements_ - 1; i >= index; i--)
    {
        data_[i + 1] = data_[i];
    }

    /* insert element */
    data_[index] = element;
    numElements_++;
    return;
}

/*!
    \brief get

    get element at index

    \param index index of element to get

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> T Vector<T>::get(int index) const
{
    /* check range */
//     if (index < 0 || index >=numElements_)
//     {
//         return (T)NULL;
//     }
    return data_[index];
}

/*!
    \brief set

    set element at index

    \param index index of element to get

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> void Vector<T>::set(int index, T element)
{
    /* check range */
    if (index < 0 || index >=numElements_)
    {
        return;
    }
    data_[index] = element;
}

/*!
    \brief operator[]

    get element at index

    \param index index of element to get

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> T Vector<T>::operator[](int index) const
{
    return (this->get(index));
}

/*!
    \brief size

    return size of list

    \return size of list

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> int Vector<T>::size() const
{
    return numElements_;
}

/*!
    \brief remove element

    remove element at index

    \param index that removed

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> T Vector<T>::removeAt(int index)
{
    /* check range */
    if (index < 0 || index >=numElements_)
    {
        /* do nothing */
        return (T)NULL;
    }

    /* save element to remove */
    T toRemove = data_[index];

    /* fix hole */
    int numElements = numElements_;
    for (int i = index; i < numElements - 1; i++)
    {
        data_[i] = data_[i + 1];
    }
    numElements_--;
    return toRemove;
}

/*!
    \brief remove element

    remove element

    \param element element to remove

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> T Vector<T>::remove(T element)
{
    /* optimize */
    int size = this->size();

    for (int i = 0; i < size; i++)
    {
        /* element to remove found */
        if (data_[i] == element)
        {
            return (removeAt(i));
        }
    }

    return (T)NULL;
}

/*!
    \brief initilize

    set size of list & increase

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> void Vector<T>::init(int size, int increase)
{

    /* number of elements */
    numElements_ = 0;

    /* set size and increase */
    size_     = size     > 0 ? size : 5;
    increase_ = increase > 0 ? increase : 5;

    /* create internal array */
#ifdef USE_BOEHM_GC
    if (!g_gc_initialized)
    {
        GC_INIT();
    }
    data_ = new(GC) T[size_];
#else
    data_ = new T[size_];
#endif
    return;
}

/*!
    \brief check list has the element

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> bool Vector<T>::hasElement(T element) const
{
    /* optimize? */
    int size = this->size();

    /* find element */
    for (int i = 0; i < size; i++)
    {
        if (data_[i] == element)
        {
            return true;
        }
    }

    /* not found */
    return false;
}

/*!
    \brief reverse

    \author Higepon
    \date   create:2007/02/16 update:
*/
template <class T> void Vector<T>::reverse()
{
    int num = numElements_ / 2;

    for (int i = 0; i < num; i++)
    {
        T tmp = data_[i];
        data_[i] = data_[numElements_ - i - 1];
        data_[numElements_ - i - 1] = tmp;
    }
    return;
}

};
#endif // __UTIL_VECTOR_H__
