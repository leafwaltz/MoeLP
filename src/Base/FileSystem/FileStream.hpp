#ifndef MoeLP_Base_FileStream
#define MoeLP_Base_FileStream

#include "../Base.hpp"
#include "../Text/Text.hpp"

namespace MoeLP
{
	class FileStream
	{
	public:
		enum AccessRight
		{
			ReadOnly,
			WriteOnly,
			ReadWrite,
			Append
		};

		FileStream(const Text& fileName, AccessRight accessRight)
			: accessRight(accessRight)
		{
			const char* mode = "wb+";
			switch (accessRight)
			{
			case ReadOnly:
				mode = "rb";
				break;
			case WriteOnly:
				mode = "wb";
				break;
			case ReadWrite:
				mode = "wb+";
				break;
			case Append:
				mode = "ab";
				break;
			}

			mint len = wtoa(fileName.c_str(), nullptr, 0);
			char* fileName_ = (char*)cpuAllocate(sizeof(char)*len);
			memset(fileName_, 0, len * sizeof(char));
			wtoa(fileName.c_str(), fileName_, len);

			#if defined MOE_MSVC
			if (fopen_s(&file, fileName_, mode) != 0)
				file = 0;
			#elif defined MOE_GCC
			file = fopen(fileName_, mode);
			#endif

			cpuDeallocate(fileName_, sizeof(char)*len);
		}

		~FileStream()
		{
			close();
		}

		bool readable() const
		{
			return file != 0 && (accessRight == ReadOnly || accessRight == ReadWrite);
		}

		bool writable() const
		{
			return file != 0 && (accessRight == WriteOnly || accessRight == ReadWrite);
		}

		bool appendable() const
		{
			return file != 0 && accessRight == Append;
		}

		bool available() const
		{
			return file != 0;
		}

		void close()
		{
			if (file != 0)
			{
				fclose(file);
				file = 0;
			}
		}

		mint64 position() const
		{
			if (file != 0)
			{
				#if defined MOE_MSVC
				mint64 pos = 0;
				if (fgetpos(file, &pos) == 0)
				{
					return pos;
				}
				#elif defined MOE_GCC
				return ftell(file);
				#endif
			}
			return -1;
		}

		mint64 size() const
		{
			if (file != 0)
			{
				mint32 pos = ftell(file);
				fseek(file, 0, SEEK_END);
				mint32 size = ftell(file);
				fseek(file, pos, SEEK_SET);
				return (mint64)size;
			}
			return -1;
		}

		void seekFromCur(mint64 index)
		{
			if (position() + index > size())
			{
				fseek(file, 0, SEEK_END);
			}
			else if (position() + index < 0)
			{
				fseek(file, 0, SEEK_SET);
			}
			else
			{
				fseek(file, index, SEEK_CUR);
			}
		}

		void seekFromBegin(mint64 index)
		{
			if (index > size())
			{
				fseek(file, 0, SEEK_END);
			}
			else if (index < 0)
			{
				fseek(file, 0, SEEK_SET);
			}
			else
			{
				fseek(file, index, SEEK_SET);
			}
		}

		void seekFromEnd(mint64 index)
		{
			if (index < 0)
			{
				fseek(file, 0, SEEK_END);
			}
			else if (index > size())
			{
				fseek(file, 0, SEEK_SET);
			}
			else
			{
				fseek(file, -index, SEEK_END);
			}
		}

		mint read(void* buffer, mint size)
		{
			MOE_ERROR(file != 0, "FileStream::read(void* buffer, mint size): The stream is not available, may be it has been closed.");
			MOE_ERROR(size != 0, "FileStream::read(void* buffer, mint size): Argument size is unlawful.");
			return fread(buffer, 1, size, file);
		}

		mint write(void* buffer, mint size)
		{
			MOE_ERROR(file != 0, "FileStream::write(void* buffer, mint size): The stream is not available, may be it has been closed.");
			MOE_ERROR(size != 0, "FileStream::write(void* buffer, mint size): Argument size is unlawful.");
			return fwrite(buffer, 1, size, file);
		}

		mint peek(void* buffer, mint size)
		{
			MOE_ERROR(file != 0, "FileStream::peek(void* buffer, mint size): The stream is not available, may be it has been closed.");
			MOE_ERROR(size != 0, "FileStream::peek(void* buffer, mint size): Argument size is unlawful.");
			mint32 pos = ftell(file);
			size_t count = fread(buffer, 1, size, file);
			fseek(file, pos, SEEK_SET);
			return count;
		}

	private:
		AccessRight accessRight;
		FILE* file;
	};
}

#endif