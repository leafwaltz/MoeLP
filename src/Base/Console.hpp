#ifndef MoeLP_Base_Console
#define MoeLP_Base_Console

#include "Text/ITextWriter.hpp"
#include <clocale>
#include <string>
#include <iostream>

namespace MoeLP
{
	namespace Console_Internal
	{
		class Console_ : public ITextWriter
		{
			virtual void writeText(const Text& text)
			{
				#ifdef MOE_GCC
				printf("%ls", text.c_str());
				#elif defined MOE_MSVC
				setlocale(LC_ALL, "");
				printf_s("%ls", text.c_str());
				#endif
			}

		public:

			Text readLine()
			{
				std::wstring s;
				std::getline(std::wcin, s, L'\n');
				return s.c_str();
			}
		};
	}

	Console_Internal::Console_ Console;
}

#endif
