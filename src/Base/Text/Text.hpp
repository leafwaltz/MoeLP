#ifndef MoeLP_Base_Text
#define MoeLP_Base_Text

#include "../Base.hpp"
#include "../Memory.hpp"
#include "CodeConvert.hpp"

#include <vector>
#include <tuple>

namespace MoeLP
{
	class Character
	{
	public:
		Character()
		{
			data.word = 0;
		}

		/**
		 * @brief from an ascii character
		 * @param c: an ascii character
		 */
		Character(char c)
		{
			data.bytes[0] = c;
			data.bytes[1] = 0;
		}
		
		/**
		 * @brief from an unicode code point
		 * @param code: an unicode code point
		 */
		Character(char16_t code)
		{
			data.word = code;
		}

		/**
		 * @brief from a wide character
		 * @param c: a wide character
		 */
		Character(wchar_t c)
		{
			if(sizeof(wchar_t)==2)
				data.word = c;
			else if (sizeof(wchar_t) == 4)
			{
				muint16* temp = (muint16*)cpuAllocate(sizeof(muint16)*2);
				size_t len = codeConvert((muint32)c, temp);
				if (len == 0)data.word = 0;
				data.word = temp[0];
				cpuDeallocate(temp, sizeof(muint16) * 2);
			}
			else
			{
				data.word = 0;
			}
		}

		/**
		 * @brief from an unicode code point
		 * @param code: an unicode code point
		 */
		Character(muint16 code)
		{
			data.word = code;
		}

		/**
		 * @brief from an unicode code point
		 * @param ucs4: an ucs4 code point
		 */
		Character(muint32 ucs4)
		{
			muint16* temp;
			size_t len = codeConvert(ucs4, temp);
			if (len != 1) data.word = 0;
			data.word = temp[0];
		}

		/**
		 * @brief from an unicode code point
		 * @detail only enable while sizeof(wchar_t)==2
		 * @param code: an unicode code point
		 */
		/*template<typename T = std::enable_if<sizeof(wchar_t)==2, void>::type>
		Character(wchar_t code)
		{
			data.word = code;
		}*/

		muint16 code() const
		{
			return data.word;
		}

		/**
		 * @brief return true if the character is 0x0000
		 */
		bool isNull() const
		{
			return data.word == 0 ? true : false;
		}

		/**
		 * @brief return true if the character is Chinese character
		 */
		bool isChineseCharacter() const
		{
			return (data.word >= 0x4E00 && data.word <= 0x9FCB) ?
				true : false;
		}

		/**
		 * @brief return true if the character is digit 
		 */
		bool isDigit() const
		{
			return (data.word >= 0x0030 && data.word <= 0x0039) ?
				true : false;
		}

		/**
		 * @brief return true if the character is letter
		 */
		bool isLetter() const
		{
			return (data.word >= 0x0061 && data.word <= 0x007A)
				|| (data.word >= 0x0041 && data.word <= 0x005A) ?
				true : false;
		}

		/**
		 * @brief return true if the character is high surrogate
		 */
		bool isHighSurrogate() const
		{
			return (data.word >= 0xD800 && data.word <= 0xDBFF) ?
				true : false;
		}

		/**
		 * @brief return true if the character is low surrogate
		 */
		bool isLowSurrogate() const
		{
			return (data.word >= 0xDC00 && data.word <= 0xDFFF) ?
				true : false;
		}

		/**
		 * @brief return true if the character is lower
		 */
		bool isLower() const
		{
			return (data.word >= 0x0061 && data.word <= 0x007A) ?
				true : false;
		}

		/**
		 * @brief return true if the character is upper
		 */
		bool isUpper() const
		{
			return (data.word >= 0x0041 && data.word <= 0x005A) ?
				true : false;
		}

		/**
		 * @brief get lower letter
		 */
		muint16 toLower()
		{
			if (isUpper())
			{
				return data.word + 0x0020;
			}
			else
			{
				return data.word;
			}
		}

		/**
		 * @brief get upper letter
		 */
		muint16 toUpper()
		{
			if (isLower())
			{
				return data.word - 0x0020;
			}
			else
			{
				return data.word;
			}
		}

		void operator =(const muint16& code)
		{
			data.word = code;
		}

		bool operator !=(const Character& c)
		{
			return (data.word != c.data.word);
		}

		bool operator ==(const Character& c)
		{
			return (data.word == c.data.word);
		}

		bool operator <(const Character& c)
		{
			return (data.word < c.data.word);
		}

		bool operator <=(const Character& c)
		{
			return (data.word <= c.data.word);
		}

		bool operator >(const Character& c)
		{
			return (data.word > c.data.word);
		}

		bool operator >=(const Character& c)
		{
			return (data.word >= c.data.word);
		}

		muint8& operator[](size_t index)
		{
			MOE_ERROR(index >= 0 && index < 2, "Character::operator[](size_t index): Argument index out of range.");
			return data.bytes[index];
		}

	private:
		union
		{
			muint16 word;
			muint8 bytes[2];
		}data;
	};

	class Text
	{
		static const mint localSize = sizeof(void*) == 8 ? 19 : 15;

	public:
		/**
		 * @brief create an empty string
		 */
		Text()
		{
			size = 0;
			realSize = size;
			refCounter = 0;
			start = 0;
			temp[0] = 0;
			buffer = temp;
		}

		/**
		 * @brief create a string containing one character
		 * @param ucs2: a unicode character
		 */
		Text(const muint16& ucs2)
		{
			refCounter = (mint*)cpuAllocate(sizeof(mint));
			*refCounter = 1;
			start = 0;
			temp[0] = ucs2;
			temp[1] = 0;
			size = 1;
			realSize = size;
			buffer = temp;
		}

		/**
		 * @brief copy a string
		 * @param str: string to copy
		 * @param length: length of the content string
		 */
		Text(const muint16* str, size_t length)
		{
			start = 0;
			if (length <= 0)
			{
				size = 0;
				refCounter = 0;
				temp[0] = 0;
				buffer = temp;
			}
			else
			{
				if (length <= localSize)
				{
					size = length;
					refCounter = (mint*)cpuAllocate(sizeof(mint));
					*refCounter = 1;
					memcpy(temp, str, length * sizeof(muint16));
					temp[length] = 0;
					buffer = temp;
				}
				else
				{
					size = length;
					refCounter = (mint*)cpuAllocate(sizeof(mint));
					*refCounter = 1;
					buffer = (muint16*)cpuAllocate(sizeof(muint16)*(length + 1));
					memcpy(buffer, str, length * sizeof(muint16));
					buffer[length] = 0;
				}
			}
			realSize = size;
		}

		/**
		 * @brief copy a string
		 * @param str: string to copy, it will contain the zero terminator
		 */
		Text(const muint16* str)
		{
			size = getBufferLength(str);
			start = 0;
			if (size <= localSize)
			{
				refCounter = (mint*)cpuAllocate(sizeof(mint));
				*refCounter = 1;
				memcpy(temp, str, size * sizeof(muint16));
				temp[size] = 0;
				buffer = temp;
			}
			else
			{
				refCounter = (mint*)cpuAllocate(sizeof(mint));
				*refCounter = 1;
				buffer = (muint16*)cpuAllocate(sizeof(muint16)*(size + 1));
				memcpy(buffer, str, size * sizeof(muint16));
				buffer[size] = 0;
			}
			realSize = size;
		}

		/**
		 * @brief copy a string
		 * @param str: string to copy
		 * @param length: length of the content string
		 */
		Text(const wchar_t* str, size_t length)
		{
			start = 0;
			if (length <= 0)
			{
				size = 0;
				refCounter = 0;
				temp[0] = 0;
				buffer = temp;
			}
			else
			{
				size = length;
				if (sizeof(wchar_t) == 2)
				{
					if (size <= localSize)
					{
						refCounter = (mint*)cpuAllocate(sizeof(mint));
						*refCounter = 1;
						memcpy(temp, str, size * sizeof(muint16));
						temp[size] = 0;
						buffer = temp;
					}
					else
					{
						refCounter = (mint*)cpuAllocate(sizeof(mint));
						*refCounter = 1;
						buffer = (muint16*)cpuAllocate(sizeof(muint16)*(size + 1));
						memcpy(buffer, str, size * sizeof(muint16));
						buffer[size] = 0;
					}
				}
				else if (sizeof(wchar_t) == 4)
				{
					if (size <= localSize)
					{
						refCounter = (mint*)cpuAllocate(sizeof(mint));
						*refCounter = 1;
						for (size_t i = 0; *str; str++, i++)
						{
							muint16* ucs2temp = (muint16*)cpuAllocate(sizeof(muint16) * 2);
							muint32 ucs4temp = *str;
							size_t len = codeConvert(ucs4temp, ucs2temp);
							temp[i] = ucs2temp[0];
							if (len != 1) temp[i] = 0;
							cpuDeallocate(ucs2temp, sizeof(muint16) * 2);
						}
						temp[size] = 0;
						buffer = temp;
					}
					else
					{
						refCounter = (mint*)cpuAllocate(sizeof(mint));
						*refCounter = 1;
						buffer = (muint16*)cpuAllocate(sizeof(muint16)*(size + 1));
						for (size_t i = 0; *str; str++, i++)
						{
							muint16* ucs2temp = (muint16*)cpuAllocate(sizeof(muint16) * 2);
							muint32 ucs4temp = *str;
							size_t len = codeConvert(ucs4temp, ucs2temp);
							buffer[i] = ucs2temp[0];
							if (len != 1) buffer[i] = 0;
							cpuDeallocate(ucs2temp, sizeof(muint16) * 2);
						}
						buffer[size] = 0;
					}
				}
			}
			realSize = size;
		}

		/**
		 * @brief copy a string
		 * @param str: ucs2/4 string to copy, it will contain the zero terminator
		 */
		Text(const wchar_t* str)
		{
			size = getBufferLength(str);
			start = 0;
			if (sizeof(wchar_t) == 2)
			{
				if (size <= localSize)
				{
					refCounter = (mint*)cpuAllocate(sizeof(mint));
					*refCounter = 1;
					memcpy(temp, str, size * sizeof(muint16));
					temp[size] = 0;
					buffer = temp;
				}
				else
				{
					refCounter = (mint*)cpuAllocate(sizeof(mint));
					*refCounter = 1;
					buffer = (muint16*)cpuAllocate(sizeof(muint16)*(size + 1));
					memcpy(buffer, str, size * sizeof(muint16));
					buffer[size] = 0;
				}
			}
			else if (sizeof(wchar_t) == 4)
			{
				if (size <= localSize)
				{
					refCounter = (mint*)cpuAllocate(sizeof(mint));
					*refCounter = 1;
					for (size_t i = 0; *str; str++, i++)
					{
						muint16* ucs2temp = (muint16*)cpuAllocate(sizeof(muint16) * 2);
						muint32 ucs4temp = *str;
						size_t len = codeConvert(ucs4temp, ucs2temp);
						temp[i] = ucs2temp[0];
						if (len != 1) temp[i] = 0;
						cpuDeallocate(ucs2temp, sizeof(muint16) * 2);
					}
					temp[size] = 0;
					buffer = temp;
				}
				else
				{
					refCounter = (mint*)cpuAllocate(sizeof(mint));
					*refCounter = 1;
					buffer = (muint16*)cpuAllocate(sizeof(muint16)*(size + 1));
					for (size_t i = 0; *str; str++, i++)
					{
						muint16* ucs2temp = (muint16*)cpuAllocate(sizeof(muint16) * 2);
						muint32 ucs4temp = *str;
						size_t len = codeConvert(ucs4temp, ucs2temp);
						buffer[i] = ucs2temp[0];
						if (len != 1) buffer[i] = 0;
						cpuDeallocate(ucs2temp, sizeof(muint16) * 2);
					}
					buffer[size] = 0;
				}
			}
			realSize = size;
		}

		/**
		 * @brief copy a text
		 * @param text: the text to copy
		 */
		Text(const Text& text)
		{
			refCounter = text.refCounter;
			buffer = text.buffer;
			size = text.size;
			start = text.start;
			realSize = text.realSize;
			if (text.size <= localSize)
			{
				memcpy(temp, text.temp, sizeof(muint16)*realSize);
				buffer = temp;
			}
			incrementRefCounter();
		}

		/**
		 * @brief copy a part of a text
		 * @param text: text to copy
		 * @param startpos: the start position
		 * @param length: the length of the part to be copyed
		 */
		Text(const Text& text, mint startpos, size_t length)
		{
			if (length <= 0)
			{
				size = 0;
				realSize = size;
				refCounter = 0;
				temp[0] = 0;
				start = 0;
				buffer = temp;
			}
			else
			{
				refCounter = text.refCounter;
				buffer = text.buffer;
				size = length;
				realSize = text.realSize;
				start = text.start + startpos;
				if (text.size <= localSize)
				{
					memcpy(temp, text.temp, sizeof(muint16)*realSize);
					buffer = temp;
				}
				incrementRefCounter();
			}
		}

		/**
		 * @brief combine two texts
		 */
		Text(const Text& src1, const Text& src2)
		{
			refCounter = (mint*)cpuAllocate(sizeof(muint));
			*refCounter = 1;
			size = src1.size + src2.size;
			realSize = size;
			start = 0;
			buffer = (muint16*)cpuAllocate(sizeof(muint16)*(size + 1));
			if (size <= localSize)
			{
				memcpy(temp, src1.buffer + src1.start, sizeof(muint16)*src1.size);
				memcpy(temp + src1.size, src2.buffer + src2.start, sizeof(muint16)*src2.size);
				temp[size] = 0;
				buffer = temp;
			}
			else
			{
				memcpy(buffer, src1.buffer + src1.start, sizeof(muint16)*src1.size);
				memcpy(buffer + src1.size, src2.buffer + src2.start, sizeof(muint16)*src2.size);
				buffer[size] = 0;
			}
		}

		/**
		 * @brief move a text
		 * @param text: the text to copy
		 */
		Text(Text&& text)
		{
			refCounter = text.refCounter;
			buffer = text.buffer;
			size = text.size;
			realSize = text.realSize;
			start = text.start;
			if (text.size <= localSize)
			{
				memcpy(temp, text.temp, sizeof(muint16)*realSize);
				buffer = temp;
			}

			text.refCounter = 0;
			text.buffer = 0;
			text.size = 0;
			text.realSize = 0;
			text.start = 0;
		}

		~Text()
		{
			decrementRefCounter();
		}

		/**
		 * @brief compare two texts
		 */
		static mint compare(const Text& text1, const Text& text2)
		{
			const muint16* buffer1 = text1.buffer + text1.start;
			const muint16* buffer2 = text2.buffer + text2.start;

			mint len = text1.size < text2.size ? text1.size : text2.size;

			while (len--)
			{
				mint difference = *buffer1++ - *buffer2++;
				if (difference != 0)
					return difference;
			}

			return text1.size - text2.size;
		}

		/**
		 * @brief create a text from an utf8 string
		 * @param utf8str: utf8 string
		 */
		static Text fromUTF8(const char* utf8str)
		{
			mint length = getBufferLength(utf8str);
			muint16* temp = (muint16*)cpuAllocate(sizeof(muint16)*length);
			size_t size = codeConvert(utf8str, temp);
			Text t = Text(temp);
			cpuDeallocate(temp, sizeof(muint16)*length);
			return t;
		}

		/**
		 * @brief create a text from an utf8 string
		 * @param local: local string
		 */
		static Text fromLocal(const char* local)
		{
			mint length = atow(local, nullptr, 0);
			wchar_t* buf = (wchar_t*)cpuAllocate(sizeof(wchar_t)*length);
			memset(buf, 0, length * sizeof(wchar_t));
			atow(local, buf, length);
			Text t = buf;
			cpuDeallocate(buf, sizeof(wchar_t)*length);
			return t;
		}

		/**
		 * @brief return the buffer of the text
		 * @detail the operation will create a new text buffer when start position 
		 * plus size is not equal with the real size of the buffer in memory
		 */
		const muint16* data() const
		{
			if (start + size != realSize)
			{
				muint16* newBuffer = (muint16*)cpuAllocate(sizeof(muint16)*(size + 1));
				memcpy(newBuffer, buffer + start, size * sizeof(muint16));
				newBuffer[size] = 0;
				decrementRefCounter();
				buffer = newBuffer;
				refCounter = new mint(1);
				start = 0;
				realSize = size;
			}
			return buffer + start;
		}

		const wchar_t* c_str() const
		{
			if (sizeof(wchar_t) == 2)
				return (const wchar_t*)data();
			else if (sizeof(wchar_t) == 4)
			{
				const muint16* temp = data();
				if (cstr == 0)
				{
					cstr = new muint32[size];
					for (size_t i = 0; i < size; i++)
					{
						size_t len = codeConvert(&temp[i], cstr[i]);
					}
				}
				return (wchar_t*)cstr;
			}
			else
			{
				return 0;
			}
		}

		/**
		 * @brief return a sub text
		 * @param index: the begin of the sub text
		 * @param count: the count of character from index
		 */
		Text subText(mint index, mint count) const
		{
			MOE_ERROR(index >= 0 && index <= size, "Text::subText(mint index, mint count): Argument index out of range.");
			MOE_ERROR(index + count >= 0 && index + count <= size, "Text::subText(mint index, mint count): Argument count out of range.");
			return Text(*this, index, count);
		}

		/**
		 * @brief return a sub text from left end of the text
		 * @param count: the count of character from index
		 */
		Text left(mint count) const
		{
			MOE_ERROR(count >= 0 && count <= size, "Text::left(mint count): Argument count out of range.");
			return Text(*this, 0, count);
		}

		/**
		 * @brief return a sub text from right end of the text
		 * @param count: the count of character from index
		 */
		Text right(mint count) const
		{
			MOE_ERROR(count >= 0 && count <= size, "Text::right(mint count): Argument count out of range.");
			return Text(*this, size - count, count);
		}

		/**
		 * @brief return a new text by inserting a text in this text
		 * @param index: the begin of the sub text
		 * @param text: the text to be inserted
		 */
		Text insert(mint index, const Text& text)
		{
			MOE_ERROR(index >= 0 && index <= size, "Text::insert(mint index, const Text& text): Argument index out of range.");
			return Text(*this, text, index, 0);
		}

		/**
		 * @brief return a new text by removing a subtext
		 * @param index: the begin of the sub text
		 * @param count: the count of character from index
		 */
		Text remove(mint index, mint count)
		{
			MOE_ERROR(index >= 0 && index <= size, "Text::remove(mint index, mint count): Argument index out of range.");
			MOE_ERROR(index + count >= 0 && index + count <= size, "Text::remove(mint index, mint count): Argument count out of range.");
			return Text(*this, Text(), index, count);
		}

		/**
		 * @brief return a new text by replacing subtext from index1 to index2 by text
		 */
		Text replace(const Text& text, mint index1, mint index2)
		{
			MOE_ERROR(index1 >= 0 && index1 <= index2, "Text::replace(const Text& text, mint index1, mint index2): Argument index1 out of range.");
			MOE_ERROR(index2 >= 0 && index2 >= index1, "Text::replace(const Text& text, mint index1, mint index2): Argument index2 out of range.");
			size_t sz = text.size + size - (index2 - index1) - 1;
			muint16* temp = (muint16*)cpuAllocate(sizeof(muint16)*(sz + 1));
			memcpy(temp, buffer, sizeof(muint16)*index1);
			memcpy(temp + index1, text.buffer, sizeof(muint16)*text.size);
			memcpy(temp + index1 + text.size, buffer + index2 + 1, sizeof(muint16)*(size - index2 - 1));
			temp[sz] = 0;
			Text t = Text(temp);
			cpuDeallocate(temp, sizeof(muint16)*(sz + 1));
			return t;
		}

		/**
		 * @brief return a reversed text.
		 */
		Text reverse()
		{
			muint16* temp = (muint16*)cpuAllocate(sizeof(muint16)*size + 1);
			for (size_t i = 0; i < size; i++)
			{
				temp[i] = buffer[size - i - 1];
			}
			temp[size] = 0;
			Text t = Text(temp);
			cpuDeallocate(temp, sizeof(muint16)*size + 1);
			return t;
		}

		/**
		 * @brief return the first position the text to be found appears in the text and it's length.
		 * @param text: the text to be found.
		 */
		std::pair<mint, size_t> findFirst(const Text& text)
		{
			return std::make_pair(KMP(c_str(), text.c_str()), text.length());
		}

		/**
		 * @brief return the last position the text to be found appears in the text and it's length.
		 * @param text: the text to be found.
		 */
		std::pair<mint, size_t> findLast(const Text& text)
		{
			mint index = KMP(reverse().c_str(), text.c_str());
			if (index == -1)return std::make_pair(index, text.length());
			return std::make_pair(size - index, text.length());
		}

		/**
		 * @brief convert a text into a double precision number
		 */
		double toDouble()
		{
			return wcstod(c_str(), nullptr);
		}

		/**
		 * @brief convert a text into a long double precision number
		 */
		long double toLongDouble()
		{
			return wcstold(c_str(), nullptr);
		}

		/**
		 * @brief convert a text into a 32 bits integer
		 * @param radix: the radix of the text number
		 * @detail for example when the text is "0xff" the parameter radix is 16
		 */
		mint32 toInt32(mint radix = 10)
		{
			return wcstol(c_str(), nullptr, radix);
		}

		/**
		 * @brief convert a text into a 64 bits integer
		 * @param radix: the radix of the text number
		 * @detail for example when the text is "0xff" the parameter radix is 16
		 */
		mint64 toInt64(mint radix = 10)
		{
			return wcstoll(c_str(), nullptr, radix);
		}

		/**
		 * @brief convert a text into a 32 bits unsigned integer
		 * @param radix: the radix of the text number
		 * @detail for example when the text is "0xff" the parameter radix is 16
		 */
		muint32 toUInt32(mint radix = 10)
		{
			return wcstoul(c_str(), nullptr, radix);
		}

		/**
		 * @brief convert a text into a 64 bits unsigned integer
		 * @param radix: the radix of the text number
		 * @detail for example when the text is "0xff" the parameter radix is 16
		 */
		muint64 toUInt64(mint radix = 10)
		{
			return wcstoull(c_str(), nullptr, radix);
		}

		static Text number(mint32 n, mint radix = 10)
		{
			wchar_t buf[100];
			i32tow(n, buf, sizeof(buf) / sizeof(wchar_t), radix);
			return Text(buf);
		}

		static Text number(mint64 n, mint radix = 10)
		{
			wchar_t buf[100];
			i64tow(n, buf, sizeof(buf) / sizeof(wchar_t), radix);
			return Text(buf);
		}

		static Text number(muint32 n, mint radix = 10)
		{
			wchar_t buf[100];
			ui32tow(n, buf, sizeof(buf) / sizeof(wchar_t), radix);
			return Text(buf);
		}

		static Text number(muint64 n, mint radix = 10)
		{
			wchar_t buf[100];
			ui64tow(n, buf, sizeof(buf) / sizeof(wchar_t), radix);
			return Text(buf);
		}

		static Text number(double n, mint precision = 6)
		{
			char buf[100];
			#if defined MOE_MSVC
			_gcvt_s(buf, 100, n, precision);
			#elif defined MOE_GCC
			gcvt(n, precision, buf);
			#endif
			return fromLocal(buf);
		}

		static Text number(long double n, mint precision = 6)
		{
			char buf[100];
			#if defined MOE_MSVC
			_gcvt_s(buf, 100, n, precision);
			#elif defined MOE_GCC
			gcvt(n, precision, buf);
			#endif
			return fromLocal(buf);
		}

		Text toUpper()
		{
			muint16* temp = (muint16*)data();
			muint16* p = temp;
			while (*p)
			{
				if (*p >= 0x0061 && *p <= 0x007A)
					*p -= 0x0020;
				p++;
			}
			return Text(temp);
		}

		Text toLower()
		{
			muint16* temp = (muint16*)data();
			muint16* p = temp;
			while (*p)
			{
				if (*p >= 0x0041 && *p <= 0x005A)
					*p += 0x0020;
				p++;
			}
			return Text(temp);
		}

		Text arg(mint32 n, mint radix = 10) const
		{
			Text t(*this);
			Text nt = number(n, radix);
			mint offset = 0;

			for (auto indices : findArgEscapes())
			{
				t = t.replace(nt, indices.first + offset, indices.second + offset);
				offset += nt.size - (indices.second - indices.first + 1);
			}

			return t;
		}

		Text arg(mint64 n, mint radix = 10) const
		{
			Text t(*this);
			Text nt = number(n, radix);
			mint offset = 0;

			for (auto indices : findArgEscapes())
			{
				t = t.replace(nt, indices.first + offset, indices.second + offset);
				offset += nt.size - (indices.second - indices.first + 1);
			}

			return t;
		}

		Text arg(muint32 n, mint radix = 10) const
		{
			Text t(*this);
			Text nt = number(n, radix);
			mint offset = 0;

			for (auto indices : findArgEscapes())
			{
				t = t.replace(nt, indices.first + offset, indices.second + offset);
				offset += nt.size - (indices.second - indices.first + 1);
			}

			return t;
		}

		Text arg(muint64 n, mint radix = 10) const
		{
			Text t(*this);
			Text nt = number(n, radix);
			mint offset = 0;

			for (auto indices : findArgEscapes())
			{
				t = t.replace(nt, indices.first + offset, indices.second + offset);
				offset += nt.size - (indices.second - indices.first + 1);
			}

			return t;
		}

		Text arg(double n, mint precision = 6) const
		{
			Text t(*this);
			Text nt = number(n, precision);
			mint offset = 0;

			for (auto indices : findArgEscapes())
			{
				t = t.replace(nt, indices.first + offset, indices.second + offset);
				offset += nt.size - (indices.second - indices.first + 1);
			}

			return t;
		}

		Text arg(long double n, mint precision = 6) const
		{
			Text t(*this);
			Text nt = number(n, precision);
			mint offset = 0;

			for (auto indices : findArgEscapes())
			{
				t = t.replace(nt, indices.first + offset, indices.second + offset);
				offset += nt.size - (indices.second - indices.first + 1);
			}

			return t;
		}

		Text arg(const Text& text) const
		{
			Text t(*this);
			mint offset = 0;

			for (auto indices : findArgEscapes())
			{
				t = t.replace(text, indices.first + offset, indices.second + offset);
				offset += text.size - (indices.second - indices.first + 1);
			}

			return t;
		}

		size_t length() const
		{
			return size;
		}

		Text& operator=(const Text& text)
		{
			if (this != &text)
			{
				decrementRefCounter();
				size = text.size;
				start = text.start;
				realSize = text.realSize;
				buffer = text.buffer;
				refCounter = text.refCounter;
				if (text.size <= localSize)
				{
					memcpy(temp, text.temp, sizeof(muint16)*realSize);
					buffer = temp;
				}
				incrementRefCounter();
			}
			return *this;
		}

		Text& operator=(Text&& text)
		{
			if (this != &text)
			{
				decrementRefCounter();
				refCounter = text.refCounter;
				buffer = text.buffer;
				size = text.size;
				realSize = text.realSize;
				start = text.start;
				if (text.size <= localSize)
				{
					memcpy(temp, text.temp, sizeof(muint16)*realSize);
					buffer = temp;
				}

				text.refCounter = 0;
				text.buffer = 0;
				text.size = 0;
				text.realSize = 0;
				text.start = 0;
			}
			return *this;
		}

		Text operator+(const Text& text)
		{
			return Text(*this, text);
		}

		Text& operator+=(const Text& text)
		{
			return *this = *this + text;
		}

		bool operator==(const Text& text) const
		{
			return compare(*this, text) == 0;
		}

		bool operator!=(const Text& text) const
		{
			return compare(*this, text) != 0;
		}

		bool operator>(const Text& text) const
		{
			return compare(*this, text) > 0;
		}

		bool operator>=(const Text& text) const
		{
			return compare(*this, text) >= 0;
		}

		bool operator<(const Text& text) const
		{
			return compare(*this, text) < 0;
		}

		bool operator<=(const Text& text) const
		{
			return compare(*this, text) <= 0;
		}
	
		bool operator==(const wchar_t* str) const
		{
			return compare(*this, str) == 0;
		}

		bool operator!=(const wchar_t* str) const
		{
			return compare(*this, str) != 0;
		}

		bool operator>(const wchar_t* str) const
		{
			return compare(*this, str)> 0;
		}

		bool operator>=(const wchar_t* str) const
		{
			return compare(*this, str) >= 0;
		}

		bool operator<(const wchar_t* str) const
		{
			return compare(*this, str) < 0;
		}

		bool operator<=(const wchar_t* str) const
		{
			return compare(*this, str) <= 0;
		}

		friend Text operator+(const wchar_t* str, const Text& text)
		{
			return Text(str) + text;
		}

		friend bool operator==(const wchar_t* str, const Text& text)
		{
			return compare(str, text) == 0;
		}

		friend bool operator!=(const wchar_t* str, const Text& text)
		{
			return compare(str, text) != 0;
		}

		friend bool operator>(const wchar_t* str, const Text& text)
		{
			return compare(str, text) > 0;
		}

		friend bool operator>=(const wchar_t* str, const Text& text)
		{
			return compare(str, text) >= 0;
		}

		friend bool operator<(const wchar_t* str, const Text& text)
		{
			return compare(str, text) < 0;
		}

		friend bool operator<=(const wchar_t* str, const Text& text)
		{
			return compare(str, text) <= 0;
		}

		muint16 operator[](size_t index) const
		{
			MOE_ERROR(index >= 0 && index <= size, "Text::operator[](size_t index): Argument index out of range.");
			return buffer[start + index];
		}

		mint referenceCount() const
		{
			return *refCounter;
		}

	private:
		mutable size_t size;
		mutable size_t realSize;
		mutable mint start;
		mutable muint16* buffer;
		muint16 temp[localSize + 1];

		mutable volatile mint* refCounter;

		//only used when sizeof(wchar_t)==4 to avoid memory leakage
		mutable muint32* cstr = 0;

		Text(const Text& src1, const Text& src2, mint index, mint count)
		{
			if (index == 0 && count == src1.size && src2.size == 0)
			{
				size = 0;
				realSize = size;
				refCounter = 0;
				start = 0;
				temp[0] = 0;
				buffer = temp;
			}
			else
			{
				refCounter = (mint*)cpuAllocate(sizeof(muint));
				*refCounter = 1;
				size = src1.size + src2.size - count;
				realSize = size;
				start = 0;
				buffer = (muint16*)cpuAllocate(sizeof(muint16)*(size + 1));
				memcpy(buffer, src1.buffer + src1.start, sizeof(muint16)*index);
				memcpy(buffer + index, src2.buffer + src2.start, sizeof(muint16)*src2.size);
				memcpy(buffer + index + src2.size, src1.buffer + src1.start + index + count, sizeof(muint16)*(src1.size - index - count));
				buffer[size] = 0;
			}
		}

		void incrementRefCounter() const
		{
			if (refCounter)
			{
				ATOMIC_INCREMENT(refCounter);
			}
		}

		void decrementRefCounter() const
		{
			if (refCounter)
			{
				if (ATOMIC_DECREMENT(refCounter) == 0)
				{
					if (size <= localSize)
						buffer = 0;
					else
						cpuDeallocate(buffer, size * sizeof(muint16));

					cpuDeallocate((void*)refCounter, sizeof(muint16));

					if (cstr)
						delete[] cstr;
				}
			}
		}

		template<typename T>
		static mint getBufferLength(const T* buffer)
		{
			mint length = 0;
			while (*buffer++)length++;
			return length;
		}

		static void i32tow(mint32 n, wchar_t* buffer, size_t size, mint radix)
		{
			switch (radix)
			{
			case 8:
				n >= 0 ?
					swprintf(buffer, size - 1, L"%o", n)
					: swprintf(buffer, size - 1, L"-%o", -n);
				break;
			case 10:
				n >= 0 ?
					swprintf(buffer, size - 1, L"%d", n)
					: swprintf(buffer, size - 1, L"-%d", -n);
				break;
			case 16:
				n >= 0 ?
					swprintf(buffer, size - 1, L"%X", n)
					: swprintf(buffer, size - 1, L"-%X", -n);
				break;
			default:
				swprintf(buffer, size - 1, L"%d", 0);
				break;
			}
		}

		static void i64tow(mint64 n, wchar_t* buffer, size_t size, mint radix)
		{
			switch (radix)
			{
			case 8:
				n >= 0 ?
					swprintf(buffer, size - 1, L"%lo", n)
					: swprintf(buffer, size - 1, L"-%lo", -n);
				break;
			case 10:
				n >= 0 ?
					swprintf(buffer, size - 1, L"%ld", n)
					: swprintf(buffer, size - 1, L"-%ld", -n);
				break;
			case 16:
				n >= 0 ?
					swprintf(buffer, size - 1, L"%lX", n)
					: swprintf(buffer, size - 1, L"-%lX", -n);
				break;
			default:
				swprintf(buffer, size - 1, L"%ld", 0);
				break;
			}
		}

		static void ui32tow(muint32 n, wchar_t* buffer, size_t size, mint radix)
		{
			switch (radix)
			{
			case 8:
				swprintf(buffer, size - 1, L"%o", n);
				break;
			case 10:
				swprintf(buffer, size - 1, L"%u", n);
				break;
			case 16:
				swprintf(buffer, size - 1, L"%X", n);
				break;
			default:
				swprintf(buffer, size - 1, L"%u", 0);
				break;
			}
		}

		static void ui64tow(muint64 n, wchar_t* buffer, size_t size, mint radix)
		{
			switch (radix)
			{
			case 8:
				swprintf(buffer, size - 1, L"%lo", n);
				break;
			case 10:
				swprintf(buffer, size - 1, L"%lu", n);
				break;
			case 16:
				swprintf(buffer, size - 1, L"%lX", n);
				break;
			default:
				swprintf(buffer, size - 1, L"%lu", 0);
				break;
			}
		}

		std::vector<std::pair<size_t, size_t>> findArgEscapes() const
		{
			std::vector<std::pair<size_t, size_t>> indexBuffer;
			mint minnum = 2147483647;
			bool begin = false;
			size_t leftIndex = 0;
			size_t rightIndex = 0;

			for (size_t i = 0; i < size; i++)
			{
				muint16* p = buffer + start + i;

				if (*p == 123)
				{
					leftIndex = i;
					begin = true;
					continue;
				}

				if (begin && (!(*p >= 48 && *p <= 57)))
				{
					begin = false;
					if (*p == 125 && *(p - 1) != 123)
					{
						rightIndex = i;
						mint32 temp = this->subText(leftIndex + 1, rightIndex - leftIndex - 1).toInt32();
						
						if (minnum > temp)
						{
							minnum = temp;
							indexBuffer.clear();
							indexBuffer.push_back(std::make_pair(leftIndex, rightIndex));
						}
						else if (minnum == temp)
						{
							indexBuffer.push_back(std::make_pair(leftIndex, rightIndex));
						}
					}
					continue;
				}
			}
			return indexBuffer;
		}

		/**
		 * @breif KMP
		 */
		mint* getNext(const wchar_t* s, mint& len)
		{
			len = wcslen(s);
			mint* next = (mint*)cpuAllocate(sizeof(mint)*len);
			mint i = 0;
			mint j = -1;
			next[0] = -1;

			while (i < len - 1)
			{
				if (j == -1 || s[i] == s[j])
				{
					++i;
					++j;
					next[i] = j;
				}
				else
				{
					j = next[j];
				}
			}
			return next;
		}

		mint KMP(const wchar_t* s, const wchar_t* t)
		{
			mint slen, tlen;
			mint i, j;
			mint* next = getNext(t, tlen);
			slen = wcslen(s);
			i = 0;
			j = 0;
			while (i<slen && j<tlen)
			{
				if (j == -1 || s[i] == t[j])
				{
					++i;
					++j;
				}
				else
				{
					j = next[j];
				}
			}

			cpuDeallocate(next, sizeof(mint)*tlen);

			if (j == tlen)
				return i - tlen;
			return -1;
		}
	};
}
#endif