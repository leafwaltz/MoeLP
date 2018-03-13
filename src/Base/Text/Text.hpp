#ifndef MoeLP_Base_Text
#define MoeLP_Base_Text

#include "../Base.hpp"
#include "../Memory.hpp"

#include <type_traits>
#include <vector>
#include <tuple>

namespace MoeLP
{
	namespace Text_Internal
	{
		struct CodeConverterHelper
		{
			template<typename T1, typename T2>
			static inline T2* UTF32ToUTF8(T1 ucs4, size_t& length)
			{
				const muint8 prefix[] = { 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
				const muint32 codeUp[] =
				{
					0x80,           // U+00000000 ~ U+0000007F  
					0x800,          // U+00000080 ~ U+000007FF  
					0x10000,        // U+00000800 ~ U+0000FFFF  
					0x200000,       // U+00010000 ~ U+001FFFFF  
					0x4000000,      // U+00200000 ~ U+03FFFFFF  
					0x80000000      // U+04000000 ~ U+7FFFFFFF 
				};

				mint i, len;

				len = sizeof(codeUp) / sizeof(muint32);
				for (i = 0; i < len; i++)
				{
					if (ucs4 < codeUp[i])
						break;
				}

				if (i == len) return 0; //invalid

				len = i + 1;
				T2* temp = (T2*)cpuAllocate(sizeof(T2)*len);
				for (; i > 0; i--)
				{
					temp[i] = static_cast<T2>((ucs4 & 0x3F) | 0x80);
					ucs4 >>= 6;
				}

				temp[0] = static_cast<T2>(ucs4 | prefix[len - 1]);

				length = len;
				return temp;
			}

			template<typename T1, typename T2>
			static inline T2 UTF8ToUTF32(const T1* utf8, size_t& length)
			{
				mint     i, len;
				muint8   b;
				T2		 ucs4;

				if (utf8 == nullptr)
				{
					length = 0;
					return 0;
				}

				b = *utf8++;
				if (b < 0x80)
				{
					ucs4 = b;
					return 1;
				}

				if (b < 0xC0 || b > 0xFD)
				{
					length = 0;
					return 0;
				}

				if (b < 0xE0)
				{
					ucs4 = b & 0x1F;
					len = 2;
				}
				else if (b < 0xF0)
				{
					ucs4 = b & 0x0F;
					len = 3;
				}
				else if (b < 0xF8)
				{
					ucs4 = b & 7;
					len = 4;
				}
				else if (b < 0xFC)
				{
					ucs4 = b & 3;
					len = 5;
				}
				else
				{
					ucs4 = b & 1;
					len = 6;
				}

				for (i = 1; i < len; i++)
				{
					b = *utf8++;
					if (b < 0x80 || b > 0xBF)
					{
						break;
					}

					ucs4 = (ucs4 << 6) + (b & 0x3F);
				}

				if (i < len)
				{
					length = 0;
					return 0;
				}
				else
				{
					length = len;
					return ucs4;
				}
			}

			template<typename T1, typename T2>
			static inline T2* UTF32ToUTF16(T1 ucs4, size_t& length)
			{
				T2* temp;

				if (ucs4 <= 0xFFFF)
				{
					temp = (T2*)cpuAllocate(sizeof(T2));
					temp[0] = static_cast<T2>(ucs4);
					length = 1;
					return temp;
				}
				else if (ucs4 <= 0xEFFFF)
				{
					temp = (T2*)cpuAllocate(sizeof(T2)*2);
					temp[0] = static_cast<T2>(0xD800 + (ucs4 >> 10) - 0x40);   // high 10 bits  
					temp[1] = static_cast<T2>(0xDC00 + (ucs4 & 0x03FF));       // low 10 bits  
					length = 2;
					return temp;
				}
				else
				{
					length = 0;
					return 0;
				}
			}

			template<typename T1, typename T2>
			static inline T2 UTF16ToUTF32(const T1* ucs2, size_t& length)
			{
				muint16 w1, w2;
				T2 temp;
				if (ucs2 == nullptr)
				{
					length = 0;
					return 0;
				}

				w1 = ucs2[0];
				if (w1 >= 0xD800 && w1 <= 0xDFFF)
				{
					if (w1 < 0xDC00)
					{
						w2 = ucs2[1];
						if (w2 >= 0xDC00 && w2 <= 0xDFFF)
						{
							temp = (w2 & 0x03FF) + (((w1 & 0x03FF) + 0x40) << 10);
							length = 2;
							return temp;
						}
					}
					length = 0;
					return 0;
				}
				else
				{
					temp = w1;
					length = 1;
					return temp;
				}
			}
		};

		template<size_t size>	struct utf;
		template<>				struct utf<8>  { typedef muint8  type; };
		template<>				struct utf<16> { typedef muint16 type; };
		template<>				struct utf<32> { typedef muint32 type; };

		template <typename T, size_t size>
		struct utfMatch
		{
			static const bool value =
				(sizeof(T) == sizeof(typename utf<size>::type)) && (!std::is_pointer<T>::value);
		};
	}
	
	/**
	 * @brief UTF-32 to UTF-8
	 */
	template<typename T1, typename T2>
	typename std::enable_if<Text_Internal::utfMatch<T1, 32>::value &&
		Text_Internal::utfMatch<T2, 8>::value, 
		size_t>::type codeConvert(T1 ucs4, T2*& utf8)
	{
		size_t length = 0;
		utf8 = Text_Internal::CodeConverterHelper::UTF32ToUTF8<T1, T2>(ucs4, length);
		return length;
	}

	/**
	 * @brief UTF-8 to UTF-32
	 */
	template<typename T1, typename T2>
	typename std::enable_if<Text_Internal::utfMatch<T1, 8>::value &&
		Text_Internal::utfMatch<T2, 32>::value,
		size_t>::type codeConvert(const T1* utf8, T2& ucs4)
	{
		size_t length = 0;
		ucs4 = Text_Internal::CodeConverterHelper::UTF8ToUTF32<T1, T2>(utf8, length);
		return length;
	}

	/**
	 * @brief UTF-32 to UTF-16
	 */
	template<typename T1, typename T2>
	typename std::enable_if<Text_Internal::utfMatch<T1, 32>::value &&
		Text_Internal::utfMatch<T2, 16>::value,
		size_t>::type codeConvert(T1 ucs4, T2*& ucs2)
	{
		size_t length = 0;
		ucs2 = Text_Internal::CodeConverterHelper::UTF32ToUTF16<T1, T2>(ucs4, length);
		return length;
	}

	/**
	 * @brief UTF-16 to UTF-32
	 */
	template<typename T1, typename T2>
	typename std::enable_if<Text_Internal::utfMatch<T1, 16>::value &&
		Text_Internal::utfMatch<T2, 32>::value,
		size_t>::type codeConvert(const T1* ucs2, T2& ucs4)
	{
		size_t length = 0;
		ucs4 = Text_Internal::CodeConverterHelper::UTF16ToUTF32<T1, T2>(ucs2, length);
		return length;
	}

	/**
	 * @brief UTF-16 to UTF-8
	 */
	template<typename T1, typename T2>
	typename std::enable_if<Text_Internal::utfMatch<T1, 16>::value &&
		Text_Internal::utfMatch<T2, 8>::value,
		size_t>::type codeConvert(T1 ucs2, T2*& utf8)
	{
		size_t length = 0;
		muint32 temp = Text_Internal::CodeConverterHelper::UTF16ToUTF32<T1, muint32>(&ucs2, length);
		if (length != 1) return 0;
		utf8 = Text_Internal::CodeConverterHelper::UTF32ToUTF8<muint32, T2>(temp, length);
		return length;
	}

	/**
	 * @brief UTF-8 to UTF-16
	 */
	template<typename T1, typename T2>
	typename std::enable_if<Text_Internal::utfMatch<T1, 8>::value &&
		Text_Internal::utfMatch<T2, 16>::value,
		size_t>::type codeConvert(const T1* utf8, T2& ucs2)
	{
		size_t length = 0;
		muint32 temp = Text_Internal::CodeConverterHelper::UTF8ToUTF32<T1, muint32>(utf8, length);
		if (length == 0) return 0;
		ucs2 = Text_Internal::CodeConverterHelper::UTF32ToUTF16<muint32, T2>(temp, length)[0];
		if (length != 1) return 0;
		return length;
	}

	/**
	 * @brief UTF-8 string to UTF-16 string
	 */
	template<typename T1, typename T2>
	typename std::enable_if<Text_Internal::utfMatch<T1, 8>::value &&
		Text_Internal::utfMatch<T2, 16>::value,
		size_t>::type codeConvert(T1* utf8str, T2*& ucs2str)
	{
		mint num, len;
		muint32 ucs4;
		size_t bufferSize = 20;
		ucs2str = (T2*)cpuAllocate(sizeof(T2)*bufferSize);

		if (utf8str == nullptr)
			return 0;

		num = 0;
		while (*utf8str)
		{
			len = codeConvert(utf8str, ucs4);
			if (len == 0) return 0;

			utf8str += len;

			T2* temp;
			len = codeConvert(ucs4, temp);
			if (len == 0) return 0;

			if (num + len >= bufferSize)
			{
				bufferSize += 20;
				T2* buf = (T2*)cpuAllocate(sizeof(T2)*bufferSize);
				memcpy(buf, ucs2str, num * sizeof(T2));
				cpuDeallocate(ucs2str, num * sizeof(T2));
				ucs2str = buf;
			}

			memcpy(ucs2str + num, temp, len * sizeof(T2));
			num += len;
		}
		*(ucs2str + num) = 0;
		return num;
	}

	/**
	 * @brief UTF-16 string to UTF-8 string
	 */
	template<typename T1, typename T2>
	typename std::enable_if<Text_Internal::utfMatch<T1, 16>::value &&
		Text_Internal::utfMatch<T2, 8>::value,
		size_t>::type codeConvert(T1* ucs2str, T2*& utf8str)
	{
		mint num, len;
		muint32 ucs4;
		size_t bufferSize = 40;
		utf8str = (T2*)cpuAllocate(sizeof(T2)*bufferSize);

		if (ucs2str == nullptr)
			return 0;

		num = 0;
		while (*ucs2str)
		{
			len = codeConvert(ucs2str, ucs4);
			if (len == 0) return 0;

			ucs2str += len;

			T2* temp;
			len = codeConvert(ucs4, temp);
			if (len == 0) return 0;
			
			if (num + len >= bufferSize)
			{
				bufferSize += 40;
				T2* buf = (T2*)cpuAllocate(sizeof(T2)*bufferSize);
				memcpy(buf, utf8str, num * sizeof(T2));
				cpuDeallocate(utf8str, num * sizeof(T2));
				utf8str = buf;
			}

			memcpy(utf8str + num, temp, len * sizeof(T2));
			num += len;
		}
		*(utf8str + num) = 0;
		return num;
	}

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

		muint8& operator [](size_t index)
		{
			MOE_ASSERT(index < 2);
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
							size_t len = 0;
							temp[i] = Text_Internal::CodeConverterHelper::UTF32ToUTF16<wchar_t, muint16>(*str, len)[0];
							if (len != 1) temp[i] = 0;
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
							size_t len = 0;
							buffer[i] = Text_Internal::CodeConverterHelper::UTF32ToUTF16<wchar_t, muint16>(*str, len)[0];
							if (len != 1) buffer[i] = 0;
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
						size_t len = 0;
						temp[i] = Text_Internal::CodeConverterHelper::UTF32ToUTF16<wchar_t, muint16>(*str, len)[0];
						if (len != 1) temp[i] = 0;
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
						size_t len = 0;
						buffer[i] = Text_Internal::CodeConverterHelper::UTF32ToUTF16<wchar_t, muint16>(*str, len)[0];
						if (len != 1) buffer[i] = 0;
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
			memcpy(buffer, src1.buffer + src1.start, sizeof(muint16)*src1.size);
			memcpy(buffer + src1.size, src2.buffer + src2.start, sizeof(muint16)*src2.size);
			buffer[size] = 0;
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
			muint16* temp;
			size_t size = codeConvert(utf8str, temp);
			return Text(temp);
		}

		/**
		 * @brief create a text from an utf8 string
		 * @param utf8str: utf8 string
		 * @param chars: the number of characters
		 */
		static Text fromLocal(const char* local)
		{
			mint length = fromLocalHelper(local, nullptr, 0);
			wchar_t* buffer = (wchar_t*)cpuAllocate(sizeof(wchar_t)*length);
			memset(buffer, 0, length * sizeof(*buffer));
			fromLocalHelper(local, buffer, length);
			Text t = buffer;
			cpuDeallocate(buffer, sizeof(wchar_t)*length);
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
				wchar_t* cstr = (wchar_t*)cpuAllocate(sizeof(wchar_t)*size);
				for (size_t i = 0; i < size; i++)
				{
					size_t len = 0;
					cstr[i] = Text_Internal::CodeConverterHelper::UTF16ToUTF32<muint16, wchar_t>(&temp[i], len);
				}
				return cstr;
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

		static mint fromLocalHelper(const char* local, wchar_t* buffer, mint chars)
		{
			#if defined MOE_MSVC
			return MultiByteToWideChar(CP_ACP, 0, local, -1, buffer, (int)(buffer ? chars : 0));
			#elif defined MOE_GCC
			return mbstowcs(buffer, local, chars - 1) + 1;
			#endif
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
	};
}
#endif