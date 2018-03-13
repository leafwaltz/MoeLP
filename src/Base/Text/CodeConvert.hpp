#ifndef MoeLP_Base_CodeConvert
#define MoeLP_Base_CodeConvert

#include "../Memory.hpp"
#include "../Base.hpp"

#include <type_traits>

namespace MoeLP
{
	namespace CodeConvert_Internal
	{
		template<typename T1, typename T2>
		static inline size_t UTF32ToUTF8(T1 ucs4, T2*& utf8)
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

			for (; i > 0; i--)
			{
				utf8[i] = static_cast<T2>((ucs4 & 0x3F) | 0x80);
				ucs4 >>= 6;
			}

			utf8[0] = static_cast<T2>(ucs4 | prefix[len - 1]);

			return len;
		}

		template<typename T1, typename T2>
		static inline size_t UTF8ToUTF32(const T1* utf8, T2& ucs4)
		{
			mint     i, len;
			muint8   b;

			if (utf8 == nullptr)
			{
				len = 0;
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
				len = 0;
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
				len = 0;
				return len;
			}
			else
			{
				return len;
			}
		}

		template<typename T1, typename T2>
		static inline size_t UTF32ToUTF16(T1 ucs4, T2*& ucs2)
		{
			mint length = 0;
			if (ucs4 <= 0xFFFF)
			{
				ucs2[0] = static_cast<T2>(ucs4);
				length = 1;
				return length;
			}
			else if (ucs4 <= 0xEFFFF)
			{
				ucs2[0] = static_cast<T2>(0xD800 + (ucs4 >> 10) - 0x40);   // high 10 bits  
				ucs2[1] = static_cast<T2>(0xDC00 + (ucs4 & 0x03FF));       // low 10 bits  
				length = 2;
				return length;
			}
			else
			{
				length = 0;
				return 0;
			}
		}

		template<typename T1, typename T2>
		static inline size_t UTF16ToUTF32(const T1* ucs2, T2& ucs4)
		{
			muint16 w1, w2;
			mint length = 0;

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
						ucs4 = (w2 & 0x03FF) + (((w1 & 0x03FF) + 0x40) << 10);
						length = 2;
						return length;
					}
				}
				length = 0;
				return 0;
			}
			else
			{
				ucs4 = w1;
				length = 1;
				return length;
			}
		}

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
	typename std::enable_if<CodeConvert_Internal::utfMatch<T1, 32>::value &&
		CodeConvert_Internal::utfMatch<T2, 8>::value,
		size_t>::type codeConvert(T1 ucs4, T2*& utf8)
	{
		return CodeConvert_Internal::UTF32ToUTF8<T1, T2>(ucs4, utf8);
	}

	/**
	 * @brief UTF-8 to UTF-32
	 */
	template<typename T1, typename T2>
	typename std::enable_if<CodeConvert_Internal::utfMatch<T1, 8>::value &&
		CodeConvert_Internal::utfMatch<T2, 32>::value,
		size_t>::type codeConvert(const T1* utf8, T2& ucs4)
	{
		return CodeConvert_Internal::UTF8ToUTF32<T1, T2>(utf8, ucs4);
	}

	/**
	 * @brief UTF-32 to UTF-16
	 */
	template<typename T1, typename T2>
	typename std::enable_if<CodeConvert_Internal::utfMatch<T1, 32>::value &&
		CodeConvert_Internal::utfMatch<T2, 16>::value,
		size_t>::type codeConvert(T1 ucs4, T2*& ucs2)
	{
		return CodeConvert_Internal::UTF32ToUTF16<T1, T2>(ucs4, ucs2);
	}

	/**
	 * @brief UTF-16 to UTF-32
	 */
	template<typename T1, typename T2>
	typename std::enable_if<CodeConvert_Internal::utfMatch<T1, 16>::value &&
		CodeConvert_Internal::utfMatch<T2, 32>::value,
		size_t>::type codeConvert(const T1* ucs2, T2& ucs4)
	{
		return CodeConvert_Internal::UTF16ToUTF32<T1, T2>(ucs2, ucs4);
	}

	/**
	 * @brief UTF-16 to UTF-8
	 */
	template<typename T1, typename T2>
	typename std::enable_if<CodeConvert_Internal::utfMatch<T1, 16>::value &&
		CodeConvert_Internal::utfMatch<T2, 8>::value,
		size_t>::type codeConvert(T1 ucs2, T2*& utf8)
	{
		muint32 temp = 0;
		size_t length = CodeConvert_Internal::UTF16ToUTF32<T1, muint32>(&ucs2, temp);
		if (length != 1) return 0;
		length = CodeConvert_Internal::UTF32ToUTF8<muint32, T2>(temp, utf8);
		return length;
	}

	/**
	 * @brief UTF-8 to UTF-16
	 */
	template<typename T1, typename T2>
	typename std::enable_if<CodeConvert_Internal::utfMatch<T1, 8>::value &&
		CodeConvert_Internal::utfMatch<T2, 16>::value,
		size_t>::type codeConvert(const T1* utf8, T2& ucs2)
	{
		muint32 temp = 0;
		size_t length = CodeConvert_Internal::UTF8ToUTF32<T1, muint32>(utf8, temp);
		if (length == 0) return 0;

		T2* ucs2temp = (T2*)cpuAllocate(sizeof(T2));
		length = CodeConvert_Internal::UTF32ToUTF16<muint32, T2>(temp, ucs2temp);
		if (length != 1) return 0;
		ucs2 = ucs2temp[0];
		cpuDeallocate(ucs2temp, sizeof(T2));
		return length;
	}

	/**
	 * @brief UTF-8 string to UTF-16 string
	 */
	template<typename T1, typename T2>
	typename std::enable_if<CodeConvert_Internal::utfMatch<T1, 8>::value &&
		CodeConvert_Internal::utfMatch<T2, 16>::value,
		size_t>::type codeConvert(T1* utf8str, T2*& ucs2str)
	{
		mint num, len;
		muint32 ucs4;

		if (utf8str == nullptr)
			return 0;

		num = 0;
		while (*utf8str)
		{
			len = codeConvert(utf8str, ucs4);
			if (len == 0) return 0;

			utf8str += len;

			T2* temp = (T2*)cpuAllocate(sizeof(T2) * 10);
			len = codeConvert(ucs4, temp);
			if (len == 0) return 0;

			memcpy(ucs2str + num, temp, len * sizeof(T2));
			num += len;

			cpuDeallocate(temp, sizeof(T2) * 10);
		}
		*(ucs2str + num) = 0;
		return num;
	}

	/**
	 * @brief UTF-16 string to UTF-8 string
	 */
	template<typename T1, typename T2>
	typename std::enable_if<CodeConvert_Internal::utfMatch<T1, 16>::value &&
		CodeConvert_Internal::utfMatch<T2, 8>::value,
		size_t>::type codeConvert(T1* ucs2str, T2*& utf8str)
	{
		mint num, len;
		muint32 ucs4;

		if (ucs2str == nullptr)
			return 0;

		num = 0;
		while (*ucs2str)
		{
			len = codeConvert(ucs2str, ucs4);
			if (len == 0) return 0;

			ucs2str += len;

			T2* temp = (T2*)cpuAllocate(sizeof(T2) * 10);
			len = codeConvert(ucs4, temp);
			if (len == 0) return 0;

			memcpy(utf8str + num, temp, len * sizeof(T2));
			num += len;
			cpuDeallocate(temp, sizeof(T2) * 10);
		}
		*(utf8str + num) = 0;
		return num;
	}
}

#endif
