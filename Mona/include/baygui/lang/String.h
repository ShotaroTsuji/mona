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

#if !defined(_STRING_H_INCLUDED_)
#define _STRING_H_INCLUDED_

#if SUPPORT_SJIS
#include "../../../tools/code_table/cp932.h"
#endif

namespace baygui {
	/** ������N���X */
	class String : public Object {
#if SUPPORT_SJIS
	public:
		static const int UTF_8 = 0;
		static const int CP932 = 1;
#endif
		
	private:
#if SUPPORT_SJIS
		/** �G���R�[�f�B���O */
		int encoding;
#endif
		/** ���������� */
		char* bytes;
		/** ���C�h������ (UCS-4) */
		wchar* charArray;
		/** ���C�h������̒��� */
		int len;
	
	private:
		/**
		 �������ݒ肷��
		 @param s ������
		*/
		String& set(const char* str);
		
	public:
		/** �f�t�H���g�R���X�g���N�^ */
		String();
		
		/**
		 �R�s�[�R���X�g���N�^
		 @param str ������
		*/
		String(const char *str);
		
#if SUPPORT_SJIS
		/**
		 �R�s�[�R���X�g���N�^
		 @param str ������
		 @param encoding �G���R�[�f�B���O
		*/
		String(const char *str, int encoding);
#endif
		
		/** �f�X�g���N�^ */
		~String();
		
		/** ���C�h��������Ԃ� (wstrlen����) */
		int length();
		
		/** ���������� (byte�z��) ��Ԃ� */
		char* getBytes();
		
		/** ���C�h�������Ԃ� */
		wchar* toCharArray();
		
		/** i�Ԗڂ̃��C�h�����𓾂� */
		wchar charAt(int i);
		
		/**
		 "="���Z�q�̑��d��`�B<br>
		 String str = "hoge"; �̂悤�Ɏg�����Ƃ��ł���B
		*/
		const String& operator=(const char* str)
		{
			set(str);
			return *this;
		}
		
		/**
		 �w�肳�ꂽ�I�u�W�F�N�g�Ɠ��������ǂ����𓾂�
		 @param obj ��r�Ώۂ̃I�u�W�F�N�g
		 */
		bool equals(Object* obj);
		
		/**
		 �w�肳�ꂽ������Ɠ��������ǂ����`�F�b�N����
		 @param str ������
		 */
		bool equals(const char* str);
		
		/**
		 �w�肳�ꂽ������Ŏn�܂邩�ǂ����`�F�b�N����
		 @param str ������
		 @return �n�܂��Ă����true�A�����łȂ����false
		*/
		bool startsWith(const char* str);
		
		/**
		 �w�肳�ꂽ������ŏI���Ă��邩�ǂ����`�F�b�N����
		 @param str ������
		 @return �I���Ă����true�A�����łȂ����false
		*/
		bool endsWith(const char* str);
	};
}

#endif // _STRING_H_INCLUDED_