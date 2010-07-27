/*
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

#ifndef _STRING_H_INCLUDED_
#define _STRING_H_INCLUDED_

#include "Object.h"

namespace baygui {
	/** ������N���X */
	class String : public Object {
	protected:
		/** ���������� */
		char* buffer;
		/** ���������� */
		int _len;

	public:
		/** �f�t�H���g�R���X�g���N�^ */
		String();
		
		/**
		 �R�s�[�R���X�g���N�^
		 @param text ������
		 @param length �����񒷁i�����l���^����ꂽ������̒����j
		*/
		String(const char* text, int length = -1);
		
		/**
		 �R�s�[�R���X�g���N�^
		 @param text ������
		*/
		String(const String& text);
		
		/** �f�X�g���N�^ */
		virtual ~String();
		
		/** 'const char*'���Z�q */
		operator const char*() const { return (buffer == NULL ? "" : buffer); }
		
		/** '='���Z�q */
		String& operator =(const char* text);
		
		/** '='���Z�q */
		String& operator =(const String& text);
		
		/** '+='���Z�q */
		void operator +=(const char* text);
		
		/** '+='���Z�q */
		void operator +=(const String& text);
		
		/** '+='���Z�q */
		void operator +=(char ch);
		
		/** '+'���Z�q */
		String operator +(const char* text) const;
		
		/** '+'���Z�q */
		String operator +(const String& text) const;
		
		/** ����������𓾂� */
		char* getBytes() const { return this->buffer; }
		
		/** ���C�h������̒����𓾂� */
		int length() const;
		
		/** �w�肳�ꂽ���Ԃ̃��C�h�����𓾂� */
		unsigned int charAt(int index) const;
		
		/** ���C�h������𓾂� */
		const unsigned int* toCharArray() const;
		
		/**
		 �w�肳�ꂽ�I�u�W�F�N�g�Ɠ��������ǂ����𓾂�
		 @param obj ��r�Ώۂ̃I�u�W�F�N�g
		*/
		virtual bool equals(Object* obj) const { return equals((String *)obj); }
		
		/**
		 �w�肳�ꂽ������Ɠ��������ǂ����𓾂�
		 @param value ������
		*/
		bool equals(const String& value) const;
		
		/**
		 �w�肳�ꂽ������Ŏn�܂��Ă��邩�ǂ����𓾂�
		 @param value ������
		*/
		bool startsWith(const String& value) const;
		
		/**
		 �w�肳�ꂽ������ŏI����Ă��邩�ǂ����𓾂�
		 @param value ������
		*/
		bool endsWith(const String& value) const;
		
		/**
		 �w�肳�ꂽ�������o������擪����̈ʒu�𓾂�
		 @param ch ����
		 @param from �J�n�ʒu
		*/
		int indexOf(char ch, int from = 0) const;
		
		/**
		 �w�肳�ꂽ�����񂪏o������擪����̈ʒu�𓾂�
		 @param value ������
		 @param from �J�n�ʒu
		*/
		int indexOf(const String& value, int from = 0) const;
		
		/**
		 �w�肳�ꂽ�������o������I�[����̈ʒu�𓾂�
		 @param ch ����
		 @param from �J�n�ʒu
		*/
		int lastIndexOf(char ch, int from = -1) const;
		
		/**
		 �w�肳�ꂽ�����񂪏o������I�[����̈ʒu�𓾂�
		 @param value ������
		 @param from �J�n�ʒu
		*/
		int lastIndexOf(const String& value, int from = -1) const;
		
		/**
		 ����������𓾂�
		 @param start �J�n�ʒu
		 @param length ����
		*/
		String substring(int start, int length) const;
		
		/** ���ׂĂ̕������������ɕϊ����� */
		String toLowerCase() const;
		
		/** ���ׂĂ̕�����啶���ɕϊ����� */
		String toUpperCase() const;
	};
}

extern baygui::String operator +(const char* text1, const baygui::String& text2);

#endif  // _STRING_H_INCLUDED_
