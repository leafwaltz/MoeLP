#ifndef MoeLP_Base_FileSystem
#define MoeLP_Base_FileSystem

#include "../Base.hpp"
#include "../Text/Text.hpp"
#include "../Text/ITextWriter.hpp"
#include "FileStream.hpp"

#if defined MOE_MSVC
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#elif defined MOE_GCC
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include <vector>
#include <iostream>
namespace MoeLP
{
	class FilePath
	{
	public:
		#if defined MOE_MSVC
		static const wchar_t delimiter = L'\\';
		#elif defined MOE_GCC
		static const wchar_t delimiter = L'/';
		#endif

		static const size_t maxLength = 512;

		FilePath()
		{
		}

		FilePath(const Text& path)
			: fullPath(path)
		{
			init();
		}

		FilePath(const wchar_t* path)
			: fullPath(path)
		{
			init();
		}
		
		FilePath(const FilePath& other)
			: fullPath(other.fullPath)
		{
			init();
		}

		~FilePath()
		{

		}

		bool isFile() const
		{
			#if defined MOE_MSVC
			WIN32_FILE_ATTRIBUTE_DATA info;
			BOOL result = GetFileAttributesExW(fullPath.c_str(), GetFileExInfoStandard, &info);
			if (!result) return false;
			return (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
			#elif defined MOE_GCC
			struct stat info;
			mint len = wtoa(fullPath.c_str(), 0, 0);
			char* buffer = new char[len];
			memset(buffer, 0, len * sizeof(*buffer));
			wtoa(fullPath.c_str(), buffer, (int)len);
			int result = stat(buffer, &info);
			if (result != 0) return false;
			delete[] buffer;
			else return S_ISREG(info.st_mode);
			#endif
		}

		bool isFolder() const
		{
			#if defined MOE_MSVC
			WIN32_FILE_ATTRIBUTE_DATA info;
			BOOL result = GetFileAttributesExW(fullPath.c_str(), GetFileExInfoStandard, &info);
			if (!result) return false;
			return (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			#elif defined MOE_GCC
			struct stat info;
			mint len = wtoa(fullPath.c_str(), 0, 0);
			char* buffer = new char[len];
			memset(buffer, 0, len * sizeof(*buffer));
			wtoa(fullPath.c_str(), buffer, (int)len);
			int result = stat(buffer, &info);
			if (result != 0) return false;
			delete[] buffer;
			else return S_ISREG(info.st_mode);
			#endif
		}

		bool isRoot() const
		{
			#if defined MOE_MSVC
			return fullPath == L"";
			#elif defined  MOE_GCC
			return fullPath == L"/";
			#endif
		}

		Text name() const
		{
			auto index = fullPath.findLast(delimiter);
			if (index.first == -1) return fullPath;
			return fullPath.right(fullPath.length() - index.first - 1);
		}

		FilePath folder() const
		{
			auto index = fullPath.findLast(delimiter);
			if (index.first == -1) return fullPath;
			return fullPath.left(index.first);
		}

		Text toText() const
		{
			return fullPath;
		}

		/**
		 * @brief get relative path from a folder
		 * @param folder:the referencing folder
		 */
		Text relativePath(const FilePath& dir)
		{
			if (fullPath.length() == 0 || dir.fullPath.length() == 0 || fullPath[0] != dir.fullPath[0])
			{
				return dir.fullPath;
			}
			#if defined MOE_MSVC
			wchar_t buf[maxLength + 1] = { 0 };
			PathRelativePathToW(
				buf,
				fullPath.c_str(),
				(isFolder() ? FILE_ATTRIBUTE_DIRECTORY : 0),
				dir.fullPath.c_str(),
				(dir.isFolder() ? FILE_ATTRIBUTE_DIRECTORY : 0)
			);
			return buf;
			#elif defined MOE_GCC
			std::vector<Text> srcSections, targetSections, resultSections;
			getPathSections(isFolder() ? fullPath : folder().path(), srcSections);
			getPathSections(dir.fullPath, targetSections);
			size_t minLength = srcSections.size();
			minLength <= targetSections.size() ? srcSections.size() : targetSections.size();
			mint lastCommonSection = 0;
			for (size_t i = 0; i < minLength; i++)
			{
				if (srcSections[i] == targetSections[i])
					lastCommonSection = i;
				else
					break;
			}

			for (size_t i = lastCommonSection + 1; i < srcSections.size(); i++)
			{
				resultSections.push_back(L"..");
			}

			for (size_t i = lastCommonSection + 1; i < targetSections.size(); i++)
			{
				resultSections.push_back(targetSections[i]);
			}

			return sectionsToPath(resultSections);
			#endif
		}

	private:
		mutable Text fullPath;

		void init()
		{
			wchar_t* temp = new wchar_t[fullPath.length() + 1];
			memcpy(temp, fullPath.c_str(), sizeof(wchar_t) * (fullPath.length() + 1));
			for (size_t i = 0; i < fullPath.length() + 1; i++)
			{
				if (temp[i] == L'\\' || temp[i] == L'/')
				{
					temp[i] = delimiter;
				}
			}
			fullPath = Text(temp);
			delete[] temp;

			#if defined MOE_MSVC
			if (fullPath != L"")
			{
				if (fullPath.length() < 2 || fullPath[1] != L':')
				{
					wchar_t buf[maxLength + 1] = { 0 };
					muint16 result = GetCurrentDirectoryW(sizeof(buf) / sizeof(wchar_t), buf);
					MOE_ERROR(!(result > maxLength + 1 || result == 0), "Get current directory failed.");
					fullPath = Text(buf) + L"\\" + fullPath;
				}

				wchar_t buf[maxLength + 1] = { 0 };
				if (fullPath.length() == 2 && fullPath[1] == L':')
				{
					fullPath += L"\\";
				}
				muint16 result = GetFullPathNameW(fullPath.c_str(), sizeof(buf) / sizeof(wchar_t), buf, NULL);
				MOE_ERROR(!(result > maxLength + 1 || result == 0), "The path is illegal.");
				fullPath = buf;
			}
			#elif defined MOE_GCC
			if (fullPath.length() == 0)
				fullPath = L"/";

			if (fullPath[0] != delimiter)
			{
				char buf[maxLength] = { 0 };
				getcwd(buf, maxLength);
				fullPath = Text::fromLocal(buf) + delimiter + fullPath;
			}

			std::vector<Text> sections;
			getPathSections(fullPath, sections);
			for (size_t i = 0; i < sections.size(); i++)
			{
				if (sections[i] == L".")
				{
					sections.erase(sections.begin() + i);
					i--;
				}
				else if (sections[i] == L"..")
				{
					if (i > 0)
					{
						sections.erase(sections.begin() + i);
						sections.erase(sections.begin() + i - 1);
						i -= 2;
					}
					else
					{
						MOE_ERROR(false, "The path is illegal.");
					}
				}

			}

			fullPath = sectionsToPath(sections);
			#endif

			if (fullPath != L"/" && fullPath.length() > 0 && fullPath[fullPath.length() - 1] == delimiter)
			{
				fullPath = fullPath.left(fullPath.length() - 1);
			}
		}

		void getPathSections(Text path, std::vector<Text>& sections)
		{
			sections.clear();
			
			Text remain = path;

			while (true)
			{
				auto index = remain.findFirst(delimiter);
				if (index.first == -1)
					break;

				if (index.first != 0)
					sections.push_back(remain.left(index.first));
				else
				{
					#if defined MOE_MSVC
					if (remain.length() >= 2 && remain[1] == delimiter)
					{
						sections.push_back(delimiter);
						index.second++;
					}
					#elif defined MOE_GCC
					sections.push_back(delimiter);
					#endif
				}
				remain = remain.right(remain.length() - (index.first + index.second));
			}

			if (remain.length() != 0)
			{
				sections.push_back(remain);
			}
		}

		Text sectionsToPath(const std::vector<Text>& sections)
		{
			Text result;

			size_t i = 0;

			#if defined MOE_MSVC
			if (sections.size() > 0 && sections[0] == L"\\")
			{
				result += delimiter;
				i++;
			}
			#elif defined MOE_GCC
			if (sections.size() > 0 && sections[0] == delimiter)
			{
				result += delimiter;
				i++;
			}
			#endif

			while (i < sections.size())
			{
				result += sections[i];
				if (i + 1 < sections.size())
					result += delimiter;
				i++;
			}

			return result;
		}
	};

	/**
	 * @brief File class
	 * @example: 
	 * File f(...);
	 * f.writeLine(...).writeLine(...).write(...)...
	 * @detail operator write or writeLine will not clear the contents of the file, if
	 * you want to rewrite the file, please use the operator rewrite.
	 */
	class File : public ITextWriter
	{
	public:

		enum Charset
		{
			withBom,
			ansi,
			utf8,
			utf16,
			utf16be
		};

		File()
		{
		}

		File(const FilePath& filePath, Charset charset = File::withBom)
			: filePath(filePath),
			charset(charset),
			bomSize(0),
			rewriteFlag(true),
			rereadFlag(true),
			readCursor(0)
		{
			memset(bom, 0, 3);

			FileStream fs(filePath.toText(), FileStream::ReadOnly);
			MOE_ERROR(fs.available(), "The file is not avaliable.");

			fs.read(bom, 3);

			if (this->charset == File::withBom)
			{
				if (bom[0] == 0xFF && bom[1] == 0xFE)
				{
					this->charset = File::utf16;
					bomSize = 2;
				}
				else if (bom[0] == 0xFE && bom[1] == 0xFF)
				{
					this->charset = File::utf16be;
					bomSize = 2;
				}
				else if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
				{
					this->charset = File::utf8;
					bomSize = 3;
				}
				else
				{
					#if defined MOE_PLATFORM_WINDOWS
					this->charset = File::ansi;
					#elif defined MOE_PLATFORM_LINUX
					this->charset = File::utf8;
					#endif
					bomSize = 0 ;
				}
			}
		}

		~File()
		{
		}

		const FilePath& path() const
		{
			return filePath;
		}

		/**
		 * @breif clear the contents of the file.
		 */
		void rewrite()
		{
			FileStream fs(filePath.toText(), FileStream::WriteOnly);
			MOE_ERROR(fs.available(), "The file is not avaliable.");
			rewriteFlag = true;
		}

		/**
		 * @breif read a line from the file.
		 * @param bufferSize: the size of the cache buffer, if the number of bytes in a line exceeds it
		 * you will get a line that is incomplete.
		 * @detail the operation will move the cursor of file, if you want to reread from the begin of
		 * the file please use the operation reread() to reset the read cursor.
		 */
		Text readLine(size_t bufferSize = 512)
		{
			FileStream fs(filePath.toText(), FileStream::ReadOnly);
			MOE_ERROR(fs.available(), "The file is not avaliable.");

			fs.seekFromBegin(readCursor);

			Text result;

			char* buffer = (char*)cpuAllocate(bufferSize * sizeof(char));
			mint i = 0;
			mint len = 0;

			while (true)
			{
				char c;
				fs.read(&c, sizeof(char));

				if (c == '\0' || c == '\n')
				{
					buffer[i] = '\0';
					len = i;
					i = 0;
					break;
				}
				else
				{
					if (i == 65535)
					{
						buffer[i] = '\0';
						len = i;
						i = 0;
					}
					buffer[i++] = c;
				}
			}

			readCursor += len;

			switch (charset)
			{
			case ansi:
			{
				result = Text::fromLocal(buffer);
			}
			break;
			case utf8:
			{
				if (rereadFlag && bomSize != 0)
				{
					result = Text::fromUTF8(buffer + 3);
					rereadFlag = false;
				}
				else
					result = Text::fromUTF8(buffer);
			}
			break;
			case utf16:
			{
				if (rereadFlag)
					result = Text((muint16*)(buffer + 2));
				else
					result = Text((muint16*)(buffer));
			}
			break;
			case utf16be:
			{
				if (rereadFlag)
					result = result.right(result.length() - 2);
			}
			break;
			}

			cpuDeallocate(buffer, bufferSize * sizeof(char));

			if (result[result.length() - 1] == L'\r')
				result = result.left(result.length() - 1);

			return result;
		}

		/**
		 * @breif move the read cursor to the begin of the file
		 */
		void reread()
		{
			FileStream fs(filePath.toText(), FileStream::WriteOnly);
			MOE_ERROR(fs.available(), "The file is not avaliable.");
			fs.seekFromBegin(0);
			rewriteFlag = true;
			readCursor = 0;
		}
		
	private:
		FilePath	filePath;
		Charset		charset;
		muint8		bom[3];
		mint		bomSize;
		bool		rewriteFlag;
		bool		rereadFlag;
		mint		readCursor;

		virtual void writeText(const Text& text)
		{
			FileStream fs(filePath.toText(), FileStream::Append);
			MOE_ERROR(fs.available(), "The file is not avaliable.");

			switch (charset)
			{
			case ansi:
			{
				mint len = wtoa(text.c_str(), nullptr, 0);
				char* buf = (char*)cpuAllocate(sizeof(char)*len);
				memset(buf, 0, len * sizeof(char));
				wtoa(text.c_str(), buf, len);
				fs.write(buf, len - 1);
				cpuDeallocate(buf, sizeof(char)*len);
			}
			break;
			case utf8:
			{
				if (bomSize == 3)
				{
					if (rewriteFlag)
					{
						char* buf = (char*)cpuAllocate(sizeof(char) * text.length() * 6 + 3);
						char* start = buf + 3;
						size_t len = codeConvert(text.data(), start);
						memcpy(buf, bom, sizeof(char) * 3);
						fs.write(buf, len + 3);
						cpuDeallocate(buf, sizeof(char) * text.length() * 6 + 3);
						rewriteFlag = false;
					}
					else
					{
						char* buf = (char*)cpuAllocate(sizeof(char) * text.length() * 6);
						size_t len = codeConvert(text.data(), buf);
						fs.write(buf, len);
						cpuDeallocate(buf, sizeof(char) * text.length() * 6);
					}
				}
				else
				{
					char* buf = (char*)cpuAllocate(sizeof(char) * text.length() * 6);
					size_t len = codeConvert(text.data(), buf);
					fs.write(buf, len);
					cpuDeallocate(buf, sizeof(char) * text.length() * 6);
				}
			}
			break;
			case utf16:
			{
				if (rewriteFlag)
				{
					char* buf = (char*)cpuAllocate(sizeof(char) * (text.length() * 2 + 2));
					char* start = buf + 2;
					memcpy(buf, bom, sizeof(char) * 2);
					memcpy(start, text.data(), sizeof(muint16)*text.length());
					fs.write(buf, sizeof(char) * (text.length() * 2 + 2));
					cpuDeallocate(buf, sizeof(char) * (text.length() * 2 + 2));
					rewriteFlag = false;
				}
				else
				{
					char* buf = (char*)cpuAllocate(sizeof(char) * text.length() * 2);
					memcpy(buf, text.data(), sizeof(muint16)*text.length());
					fs.write(buf, sizeof(char) * text.length() * 2);
					cpuDeallocate(buf, sizeof(char) * text.length() * 2);
				}
			}
			break;
			case utf16be:
			{
				if (rewriteFlag)
				{
					char* buf = (char*)cpuAllocate(sizeof(char) * (text.length() * 2 + 2));
					char* start = buf + 2;
					memcpy(start, text.data(), sizeof(muint16)*text.length());
					for (size_t i = 2; i < text.length() * 2 + 2; i += 2)
					{
						char temp = buf[i];
						buf[i] = buf[i + 1];
						buf[i + 1] = temp;
					}
					memcpy(buf, bom, sizeof(char) * 2);
					fs.write(buf, sizeof(char) * (text.length() * 2 + 2));
					cpuDeallocate(buf, sizeof(char) * (text.length() * 2 + 2));
					rewriteFlag = false;
				}
				else
				{
					char* buf = (char*)cpuAllocate(sizeof(char) * text.length() * 2);
					memcpy(buf, text.data(), sizeof(muint16)*text.length());
					for (size_t i = 0; i < text.length() * 2; i += 2)
					{
						char temp = buf[i];
						buf[i] = buf[i + 1];
						buf[i + 1] = temp;
					}
					fs.write(buf, sizeof(char) * text.length() * 2);
					cpuDeallocate(buf, sizeof(char) * text.length() * 2);
				}
			}
			break;
			}
		}
	};
}

#endif