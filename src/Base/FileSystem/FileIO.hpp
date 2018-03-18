#ifndef MoeLP_Base_FileIO
#define MoeLP_Base_FileIO

#include "FileStream.hpp"
#include "../Text/Text.hpp"
#include "../Text/CodeConvert.hpp"

namespace MoeLP
{
	class TextEncoder
	{
	public:
		TextEncoder(FileStream* fs)
			: stream(fs)
		{
		}

		mint write(void* buffer, mint size)
		{
			const mint chars = size / sizeof(wchar_t);
			wchar_t* unicode = 0;

			if (chars != 0)
			{
				unicode = (wchar_t*)buffer;
			}

			return writeText(unicode, chars) * sizeof(wchar_t);
		}

	protected:
		FileStream * stream;

		virtual mint writeText(Text text, mint chars) = 0;
	};

	class TextDecoder
	{
	public:
		TextDecoder(FileStream* fs)
			: stream(fs)
		{
		}

		mint read(void* buffer, mint size)
		{
			const mint chars = size / sizeof(wchar_t);
			wchar_t* unicode = (wchar_t*)buffer;
			mint bytes = readText(unicode, chars) * sizeof(wchar_t);
		}

	protected:
		FileStream * stream;

		virtual mint readText(Text text, mint chars) = 0;
	};

	/**
	* @brief mbcs
	*/
	class MbcsEncoder : public TextEncoder
	{
	private:
		virtual mint writeText(Text text, mint chars)
		{
			mint len = wtoa(text.c_str(), 0, 0);
			char* buffer = new char[len];
			memset(buffer, 0, len * sizeof(*buffer));
			wtoa(text.c_str(), buffer, (int)len);
			mint result = stream->write(buffer, len);
			delete[] buffer;

			if (result == len)
			{
				return chars;
			}
			else
			{
				return 0;
			}
		}
	};

	class MbcsDecoder : public TextDecoder
	{
	private:
		virtual mint readText(Text text, mint chars)
		{

		}
	};
}

#endif