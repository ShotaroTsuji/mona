/*
Copyright (c) 2004 Tino
Copyright (c) 2005 bayside

Permission is hereby granted, free of charge, to any person 
obtaining a copy of this software and associated documentation files 
(the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, 
publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be 
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#if !defined(_POINTER_H_INCLUDED_)
#define _POINTER_H_INCLUDED_

#define _P Pointer

/** �Q�ƃJ�E���g�^�X�}�[�g�|�C���^�[ */
template <class T> struct Pointer {
private:
	/** ���|�C���^�[ */
	T* pointer;
	/** �Q�ƃJ�E���g */
	int* refCount;

private:
	/** ���������� */
	inline void Initialize()
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		this->pointer = 0 /* NULL */;
		this->refCount = 0 /* NULL */;
	}

public:
	/** �f�t�H���g�R���X�g���N�^ */
	Pointer()
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		this->Initialize();
	}
	
	/** �R�s�[�R���X�g���N�^ */
	Pointer(Object* pointer)
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		this->Initialize();
		this->set(pointer);
	}
	
	/** �f�X�g���N�^ */
	~Pointer()
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		this->unset();
	}
	
	/** �I�u�W�F�N�g��ݒ肷�� */
	void set(Object* pointer)
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		this->unset();
		this->pointer = (T*)pointer;
		if (this->pointer == 0 /* NULL */) return;
		this->refCount = pointer->getPointer();
		(*this->refCount)++;
	}
	
	/** �I�u�W�F�N�g��j������ */
	void unset()
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		if (this->refCount != 0 /* NULL */) {
			(*this->refCount)--;
			if (*this->refCount < 1) {
				this->refCount = 0 /*NULL*/;
				delete this->pointer;
			} else {
				this->refCount = 0 /*NULL*/;
			}
			this->pointer = 0 /* NULL */;
		}
	}
	
	/** �A���[���Z�q�̑��d��` */
	inline T* operator ->()
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return this->pointer;
	}
	
	/** ���|�C���^�[�𓾂� */
	inline T* get()
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return this->pointer;
	}
	
	/** �Q�ƃJ�E���g�𓾂� */
	inline int getRefCount() const
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return this->refCount != 0 ? *this->refCount : -1;
	}
	
	/** ==���Z�q�̑��d��` */
	inline bool operator ==(T* arg) const
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return this->pointer == arg;
	}
	
	/** !=���Z�q�̑��d��` */
	inline bool operator !=(T* arg) const
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return this->pointer != arg;
	}
	
	/** ==���Z�q�̑��d��` */
	inline bool operator ==(const Pointer<T>& arg) const
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return this->pointer == arg.pointer;
	}
	
	/** !=���Z�q�̑��d��` */
	inline bool operator !=(const Pointer<T>& arg) const
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return this->pointer != arg.pointer;
	}
	
	/** ������Z�q�̑��d��` */
	inline Pointer<T>& operator =(Object* pointer)
	{
		//logprintf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		this->set(pointer);
		return *this;
	}
};

#endif // _POINTER_H_INCLUDED_