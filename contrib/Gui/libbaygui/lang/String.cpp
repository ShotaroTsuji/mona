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

#include "baygui.h"

namespace baygui
{
	String::String()
	{
		this->buffer = NULL;
		this->_len = 0;
	}

	String::String(const char* text, int length /*= -1*/)
	{
		if (text == NULL) {
			this->buffer = NULL;
			this->_len = 0;
		} else {
			if (length == -1) length = strlen(text);
			this->buffer = new char[length + 1];
			//ASSERT(this->buffer)
			memcpy(this->buffer, text, length);
			this->buffer[length] = '\0';
			this->_len = length;
		}
	}

	String::String(const String& text)
	{
		this->buffer = NULL;
		this->_len = 0;
		*this = text;
	}

	String::~String()
	{
		if (this->buffer != NULL) delete [] this->buffer;
	}

	String& String::operator =(const char* text)
	{
		if (this->buffer != NULL) delete [] this->buffer;
		if (text == NULL) {
			this->buffer = NULL;
			this->_len = 0;
		} else {
			this->_len = strlen(text);
			this->buffer = new char[this->_len + 1];
			//ASSERT(this->buffer)
			memcpy(this->buffer, text, this->_len);
			this->buffer[this->_len] = '\0';
		}
		return *this;
	}

	String& String::operator =(const String& text)
	{
		if (this->buffer != NULL) delete [] this->buffer;
		if (text.buffer == NULL) {
			this->buffer = NULL;
			this->_len = 0;
		} else {
			this->_len = text._len;
			this->buffer = new char[this->_len + 1];
			//ASSERT(this->buffer)
			memcpy(this->buffer, text.buffer, this->_len);
			this->buffer[this->_len] = '\0';
		}
		return *this;
	}

	void String::operator +=(const char* text)
	{
		int len1 = this->_len, len2 = text != NULL ? strlen(text) : 0;
		this->_len = len1 + len2;
		char* buf;
		if (this->_len == 0) {
			buf = NULL;
		} else {
			buf = new char[this->_len + 1];
			//ASSERT(buf)
			if (this->buffer != NULL) memcpy(buf, this->buffer, len1);
			if (text != NULL) memcpy(&buf[len1], text, len2);
			buf[this->_len] = '\0';
		}
		if (this->buffer != NULL) delete [] this->buffer;
		this->buffer = buf;
	}

	void String::operator +=(const String& text)
	{
		int len1 = this->_len, len2 = text._len;
		this->_len = len1 + len2;
		char* buf;
		if (this->_len == 0) {
			buf = NULL;
		} else {
			buf = new char[this->_len + 1];
			//ASSERT(buf)
			if (this->buffer != NULL) memcpy(buf, this->buffer, len1);
			if (text .buffer != NULL) memcpy(&buf[len1], text.buffer, len2);
			buf[this->_len] = '\0';
		}
		if (this->buffer != NULL) delete [] this->buffer;
		this->buffer = buf;
	}

	void String::operator +=(char ch)
	{
		char* buf = new char[this->_len + 2];
		//ASSERT(buf)
		memcpy(buf, this->buffer, this->_len);
		buf[this->_len++] = ch;
		buf[this->_len] = '\0';
		if (this->buffer != NULL) delete [] this->buffer;
		this->buffer = buf;
	}

	String String::operator +(const char* text) const
	{
		String ret = *this;
		ret += text;
		return ret;
	}

	String String::operator +(const String& text) const
	{
		String ret = *this;
		ret += text;
		return ret;
	}

	int String::length() const
	{
		unsigned char c1, c2, c3;
		int len = 0;
		
		// UTF-8 -> UCS-4
		for (int i = 0; i < this->_len; i++) {
			// 1st unsigned char
			if (this->buffer[i] == 0) {
				break;
			} else {
				c1 = (unsigned char)this->buffer[i];
			}
			// 0aaa bbbb - > 0aaa bbbb (0x20-0x7F)
			if (c1 <= 0x7F) {
				len++; // c1
			// 110a aabb 10bb cccc -> 0000 0aaa bbbb cccc (0xC280-0xDFBF)
			} else if (0xC2 <= c1 && c1 <= 0xDF) {
				// 2nd unsigned char
				if (this->buffer[i] == this->_len - 1) {
					break;
				} else {
					c2 = (unsigned char)this->buffer[++i];
				}
				len++; // ((c1 & 0x1F) << 6) | (c2 & 0x3F);
			// 1110 aaaa 10bb bbcc 10cc dddd -> aaaa bbbb cccc dddd (0xE0A080-0xEFBFBF)
			} else if (0xE0 <= c1 && c1 <= 0xEF) {
				// 2nd unsigned char
				if (this->buffer[i] == this->_len - 1) {
					break;
				} else {
					c2 = (unsigned char)this->buffer[++i];
				}
				// 3rd unsigned char
				if (this->buffer[i] == this->_len - 1) {
					break;
				} else {
					c3 = (unsigned char)this->buffer[++i];
				}
				len++; // ((c1 & 0xF) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
			}
		}
		return len;
	}
	
	unsigned int String::charAt(int index) const
	{
		unsigned char c1, c2, c3;
		int len = 0;
		
		// UTF-8 -> UCS-4
		for (int i = 0; i < this->_len; i++) {
			// 1st unsigned char
			if (this->buffer[i] == 0) {
				break;
			} else {
				c1 = (unsigned char)this->buffer[i];
			}
			// 0aaa bbbb - > 0aaa bbbb (0x20-0x7F)
			if (c1 <= 0x7F) {
				if (len == index) {
					return c1;
				} else {
					len++;
				}
			// 110a aabb 10bb cccc -> 0000 0aaa bbbb cccc (0xC280-0xDFBF)
			} else if (0xC2 <= c1 && c1 <= 0xDF) {
				// 2nd unsigned char
				if (this->buffer[i] == this->_len - 1) {
					break;
				} else {
					c2 = (unsigned char)this->buffer[++i];
				}
				if (len == index) {
					return ((c1 & 0x1F) << 6) | (c2 & 0x3F);
				} else {
					len++;
				}
			// 1110 aaaa 10bb bbcc 10cc dddd -> aaaa bbbb cccc dddd (0xE0A080-0xEFBFBF)
			} else if (0xE0 <= c1 && c1 <= 0xEF) {
				// 2nd unsigned char
				if (this->buffer[i] == this->_len - 1) {
					break;
				} else {
					c2 = (unsigned char)this->buffer[++i];
				}
				// 3rd unsigned char
				if (this->buffer[i] == this->_len - 1) {
					break;
				} else {
					c3 = (unsigned char)this->buffer[++i];
				}
				if (len == index) {
					return ((c1 & 0xF) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
				} else {
					len++;
				}
			}
		}
		return '?';
	}
	
	const unsigned int* String::toCharArray() const
	{
		int len = length();
		if (len == 0) return NULL;
		unsigned int* charArray = new unsigned int[len];
		for (int i = 0; i < len; i++) {
			charArray[i] = charAt(i);
		}
		return charArray;
	}
	
	bool String::equals(const String& text) const
	{
		if (this->buffer == NULL || text.buffer == NULL) return this->buffer == text.buffer;
		if (this->_len != text._len) return false;

		return strcmp(this->buffer, text.buffer) == 0;
	}

	bool String::startsWith(const String& value) const
	{
		int len = value._len;
		if (len > this->_len) return false;
		for (int i = 0; i < len; i++) {
			if (this->buffer[i] != value.buffer[i]) return false;
		}
		return true;
	}

	bool String::endsWith(const String& value) const
	{
		int len = value._len;
		int pos = this->_len - len;
		if (pos < 0) return false;
		for (int i = 0; i < len; i++) {
			if (this->buffer[pos + i] != value.buffer[i]) return false;
		}
		return true;
	}

	int String::indexOf(char ch, int from /*= 0*/) const
	{
		if (this->buffer == NULL || this->_len == 0) return -1;

		if (from < 0) from = 0;
		for (int i = from; i < this->_len; i++) {
			if (this->buffer[i] == ch) return i;
		}
		return -1;
	}

	int String::indexOf(const String& value, int from /*= 0*/) const
	{
		if (this->buffer == NULL) return value.buffer == NULL;
		if (this->_len == 0) return -1;

		if (from < 0) from = 0;
		int last = this->_len - value._len;
		if (value.buffer == NULL || value._len == 0) return from < this->_len ? from : -1;
		for (int i = from; i <= last; i++) {
			bool ok = true;
			for (int j = 0; j < value._len; j++) {
				if (this->buffer[i + j] != value.buffer[j]) {
					ok = false;
					break;
				}
			}
			if (ok) return i;
		}
		return -1;
	}

	int String::lastIndexOf(char ch, int from /*= -1*/) const
	{
		if (this->buffer == NULL || this->_len == 0) return -1;

		if (from == -1) from = this->_len;
		if (from > this->_len) from = this->_len;
		for (int i = from; i > 0; i--) {
			if (this->buffer[i - 1] == ch) return i - 1;
		}
		return -1;
	}

	int String::lastIndexOf(const String& value, int from /*= -1*/) const
	{
		if (this->buffer == NULL) return value.buffer == NULL;
		if (this->_len == 0) return -1;

		if (from == -1) from = this->_len;
		if (from > this->_len) from = this->_len;
		if (value.buffer == NULL || value._len == 0) return from - 1;
		for (int i = from; i >= value._len; i--) {
			bool ok = true;
			for (int j = 0; j < value._len; j++) {
				if (this->buffer[i - j - 1] != value.buffer[value._len - j - 1]) {
					ok = false;
					break;
				}
			}
			if (ok) return i - value._len;
		}
		return -1;
	}

	String String::substring(int start, int length) const
	{
		if (start < 0 || this->_len <= start || length < 1) return NULL;
        String ret;
        for (int i = 0; i < length; i++) {

            uint8_t buf[4];
            int len = MonAPI::Encoding::ucs4ToUtf8(charAt(i + start), buf);
            for (int j = 0; j < len; j++) {
                ret += buf[j];
            }
        }
        return ret;
	}

	String String::toLowerCase() const
	{
		String ret = *this;
		for (int i = 0; i < ret._len; i++) {
			char ch = ret.buffer[i];
			if ('A' <= ch && ch <= 'Z') ret.buffer[i] = ch + ('a' - 'A');
		}
		return ret;
	}

	String String::toUpperCase() const
	{
		String ret = *this;
		for (int i = 0; i < ret._len; i++) {
			char ch = ret.buffer[i];
			if ('a' <= ch && ch <= 'z') ret.buffer[i] = ch - ('a' - 'A');
		}
		return ret;
	}

baygui::String operator +(const char* text1, const baygui::String& text2)
{
	baygui::String ret = text1;
	ret += text2;
	return ret;
}
}
