#ifndef MoeLP_Base
#define MoeLP_Base

#define MOE_FORCE_INLINE __forceinline

#define MOE_DISALLOW_COPY_AND_ASSIGN(TYPE)			\
		TYPE(const TYPE&) = delete;					\
		void operator=(const TYPE&) = delete;

#include <assert.h>
#define MOE_ASSERT(expression) assert(expression)
#define MOE_STATIC_ASSERT(expression, error) static_assert(expression, error)

/**
 * @brief platforms
 */

#if defined _WIN32
#define MOE_PLATFORM_WINDOWS
#include <Windows.h>
#elif defined __linux__
#define MOE_PLATFORM_LINUX
#endif

#if defined _WIN64 || __x86_64 || __LP64__
#define MOE_x64
#endif

#if defined _MSC_VER
#define MOE_MSVC
MOE_STATIC_ASSERT(_MSC_VER >= 1700, "Old version of msvc is not supported.");
#elif defined __GNUC__
#define MOE_GCC
#else
MOE_STATIC_ASSERT(false, "Only support msvc and gcc.");
#endif


#if defined MOE_MSVC
#include <intrin.h>
#elif defined MOE_GCC
#include <x86intrin.h>
#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <stdio.h>
#include <cstring>
#endif

/**
 * @brief types
 */
namespace MoeLP
{
#if defined MOE_MSVC
	typedef signed __int8			mint8;
	typedef unsigned __int8			muint8;
	typedef signed __int16			mint16;
	typedef unsigned __int16		muint16;
	typedef signed __int32			mint32;
	typedef unsigned __int32		muint32;
	typedef signed __int64			mint64;
	typedef unsigned __int64		muint64;
#elif defined MOE_GCC
	typedef int8_t					mint8;
	typedef uint8_t					muint8;
	typedef int16_t					mint16;
	typedef uint16_t				muint16;
	typedef int32_t					mint32;
	typedef uint32_t				muint32;
	typedef int64_t					mint64;
	typedef uint64_t				muint64;
#endif

#if defined MOE_x64
	typedef mint64					mint;
	typedef muint64					muint;
#else
	typedef mint32					mint;
	typedef muint32					muint;
#endif
}

/**
 * @breif atomic operations
 */
#if defined MOE_x64
	#if defined MOE_MSVC
	#define ATOMIC_INCREMENT(x) _InterlockedIncrement64(x)
	#define ATOMIC_DECREMENT(x) _InterlockedDecrement64(x)
	#elif defined MOE_GCC
	#define ATOMIC_INCREMENT(x) __sync_add_and_fetch(x, 1)
	#define ATOMIC_DECREMENT(x) __sync_sub_and_fetch(x, 1)
	#endif
#else
	#if defined MOE_MSVC
	#define ATOMIC_INCREMENT(x) _InterlockedIncrement((volatile long*)x)
	#define ATOMIC_DECREMENT(x) _InterlockedDecrement((volatile long*)x)
	#elif defined MOE_GCC
	#define ATOMIC_INCREMENT(x) __sync_add_and_fetch(x, 1)
	#define ATOMIC_DECREMENT(x) __sync_sub_and_fetch(x, 1)
	#endif
#endif

#include <vector>  
#include <bitset>  
#include <array>  
#include <string>
#if defined(MOE_GCC)
#include <cpuid.h>
#endif

namespace MoeLP
{
	class InstructionSet
	{
		static void cpuidex(unsigned int CpuInfo[4], unsigned int InfoType, unsigned int ECXValue)
		{
			#if defined(MOE_GCC)
			__cpuid_count(InfoType, ECXValue, CpuInfo[0], CpuInfo[1], CpuInfo[2], CpuInfo[3]);
			#elif defined(MOE_MSVC)
			__cpuidex((int*)(void*)CpuInfo, (int)InfoType, (int)ECXValue);
			#endif	
		}

		static void cpuid(unsigned int CpuInfo[4], unsigned int InfoType)
		{
			#if defined(MOE_GCC)
			__cpuid(InfoType, CpuInfo[0], CpuInfo[1], CpuInfo[2], CpuInfo[3]);
			#elif defined(MOE_MSVC)
			__cpuid((int*)(void*)CpuInfo, (int)InfoType);
			#endif
		}

		struct InstructionSet_Internal
		{
			InstructionSet_Internal()
				: nIds_{ 0 },
				nExIds_{ 0 },
				isIntel_{ false },
				isAMD_{ false },
				f_1_ECX_{ 0 },
				data_{},
				extdata_{}
			{
				//int cpuInfo[4] = {-1};  
				std::array<int, 4> cpui;

				// Calling __cpuid with 0x0 as the function_id argument  
				// gets the number of the highest valid function ID.  
				cpuid((unsigned int*)cpui.data(), 0);
				nIds_ = cpui[0];

				for (int i = 0; i <= nIds_; ++i)
				{
					cpuidex((unsigned int*)cpui.data(), i, 0);
					data_.push_back(cpui);
				}

				// Capture vendor string  
				char vendor[0x20];
				memset(vendor, 0, sizeof(vendor));
				*reinterpret_cast<int*>(vendor) = data_[0][1];
				*reinterpret_cast<int*>(vendor + 4) = data_[0][3];
				*reinterpret_cast<int*>(vendor + 8) = data_[0][2];
				vendor_ = vendor;
				if (vendor_ == "GenuineIntel")
				{
					isIntel_ = true;
				}
				else if (vendor_ == "AuthenticAMD")
				{
					isAMD_ = true;
				}

				// load bitset with flags for function 0x00000001  
				if (nIds_ >= 1)
				{
					f_1_ECX_ = data_[1][2];
				}

				// Calling __cpuid with 0x80000000 as the function_id argument  
				// gets the number of the highest valid extended ID.  
				cpuid((unsigned int*)cpui.data(), 0x80000000);
				nExIds_ = cpui[0];

				char brand[0x40];
				memset(brand, 0, sizeof(brand));

				for (int i = 0x80000000; i <= nExIds_; ++i)
				{
					cpuidex((unsigned int*)cpui.data(), i, 0);
					extdata_.push_back(cpui);
				}

				// Interpret CPU brand string if reported  
				if (nExIds_ >= 0x80000004)
				{
					memcpy(brand, extdata_[2].data(), sizeof(cpui));
					memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
					memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
					brand_ = brand;
				}
			};

			int nIds_;
			int nExIds_;
			std::string vendor_;
			std::string brand_;
			bool isIntel_;
			bool isAMD_;
			std::bitset<32> f_1_ECX_;
			std::vector<std::array<int, 4>> data_;
			std::vector<std::array<int, 4>> extdata_;
		};

	public:
		// getters  
		static std::string Vendor() { return CPU_Rep.vendor_; }
		static std::string Brand() { return CPU_Rep.brand_; }

		static bool SSE3()	{ return CPU_Rep.f_1_ECX_[0]; }
		static bool SSE41() { return CPU_Rep.f_1_ECX_[19]; }
		static bool SSE42() { return CPU_Rep.f_1_ECX_[20]; }
		static bool AVX()	{ return CPU_Rep.f_1_ECX_[28]; }

	private:
		static const InstructionSet_Internal CPU_Rep;
	};

	const InstructionSet::InstructionSet_Internal InstructionSet::CPU_Rep;

	class Exception
	{
	private:
		const char* description_;
		const char* file_;
		muint32 line_;

	public:
		Exception(const char* description,
			const char* file,
			muint32 line)
			: description_(description),
			file_(file), line_(line)
		{}

		const char* description() const { return description_; }
		const char* file() const { return file_; }
		muint32		line() const { return line_; }
	};

	#define MOE_ERROR(CONDITION, DESCRIPTION) do{if(!(CONDITION))throw Exception(DESCRIPTION, __FILE__, __LINE__);}while(0)

	#if defined max
	#undef max
	#endif
	#if defined min
	#undef min
	#endif

	template<typename T1, typename T2>
	inline T1 max(const T1& a, const T2& b)
	{
		return a > b ? a : b;
	}

	template<typename T1, typename T2>
	inline T1 min(const T1& a, const T2& b)
	{
		return a < b ? a : b;
	}

	mint atow(const char* ansi, wchar_t* buffer, mint chars)
	{
		#if defined MOE_MSVC
		return MultiByteToWideChar(CP_ACP, 0, ansi, -1, buffer, (int)(buffer ? chars : 0));
		#elif defined MOE_GCC
		return mbstowcs(buffer, ansi, chars - 1) + 1;
		#endif
	}

	mint wtoa(const wchar_t* wide, char* buffer, mint chars)
	{
		#if defined MOE_MSVC
		return WideCharToMultiByte(CP_ACP, 0, wide, -1, buffer, (int)(buffer ? chars : 0), 0, 0);
		#elif defined MOE_GCC
		return wcstombs(buffer, wide, chars - 1) + 1;
		#endif
	}
}

#endif