#ifndef MoeLP_Base_ITextWriter
#define MoeLP_Base_ITextWriter

#include "Text.hpp"

namespace MoeLP
{
	class ITextWriter
	{
	protected:
		virtual void writeText(const Text& text) = 0;

	public:

		ITextWriter& write(const Text& text)
		{
			writeText(text);
			return *this;
		}

		template<typename T>
		ITextWriter& write(const Text& text, const T& n)
		{
			writeText(text.arg(n));
			return *this;
		}

		template<typename T, typename... Args>
		ITextWriter& write(const Text& text, const T& n, const Args& ... args)
		{
			write(text.arg(n), args...);
			return *this;
		}

		ITextWriter& writeLine(const Text& text)
		{
			write(text);
			writeText(L"\n");
			return *this;
		}

		template<typename T>
		ITextWriter& writeLine(const Text& text, const T& n)
		{
			write(text.arg(n));
			writeText(L"\n");
			return *this;
		}

		template<typename T, typename... Args>
		ITextWriter& writeLine(const Text& text, const T& n, const Args& ... args)
		{
			write(text.arg(n), args...);
			writeText(L"\n");
			return *this;
		}
	};
}
#endif