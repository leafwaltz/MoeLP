#ifndef MoeLP_Base_Memory
#define MoeLP_Base_Memory

#include "Base.hpp"

#include <new>
#include <memory>
#include <mutex>
#include <type_traits>

namespace MoeLP
{
	namespace MoeLP_Memory_Internal
	{
		template<typename T0, typename T1>
		inline T0* byteShift(T1* ptr, mint bias)
		{
			return reinterpret_cast<T0*>(reinterpret_cast<char*>(ptr) + bias);
		}

		/**
		* @brief the interface of allocator
		*/
		struct Allocator
		{
			virtual ~Allocator() {}
			virtual void* allocate(size_t size) = 0;
			virtual void deallocate(void* ptr) = 0;
		};

		struct CpuAllocator : public Allocator
		{
			~CpuAllocator() {}

			virtual void* allocate(size_t size)
			{
				return malloc(size);
			}

			virtual void deallocate(void* ptr)
			{
 				free(ptr);
			}
		};

		class ObjectPool
		{
			struct FreeNode
			{
				muint32		bias;
				FreeNode*	next;
				FreeNode*	prior;
			};

			struct Block
			{
				muint32		freeNodeCount;
				Block*		next;
				Block*		prior;
			};

			static const muint  nodesPerBlock = 4096;
			static const mint	blockDataSize = sizeof(Block);
			static const mint	freeNodeOffset = sizeof(FreeNode);

		public:
			ObjectPool(mint size)
				: nodeSize(size),
				freeNodeSize(freeNodeOffset + nodeSize),
				blockSize(blockDataSize + freeNodeSize * nodesPerBlock),
				allocator(new CpuAllocator()),
				headBlock(nullptr),
				headFreeNode(nullptr),
				shiftTable((mint*)allocator->allocate(nodesPerBlock * sizeof(mint))),
				recycledBytes(0)
			{
				shiftTable[0] = -1 * blockDataSize;
				for (size_t i = 1; i < nodesPerBlock; i++)
				{
					shiftTable[i] = shiftTable[i - 1] - freeNodeSize;
				}
			}

			~ObjectPool()
			{
			}

			void* allocate() throw (std::bad_alloc)
			{
				if (!headFreeNode)
				{
					Block* newBlock = reinterpret_cast<Block*>(allocator->allocate(blockSize));
					if (!headBlock)
					{
						newBlock->freeNodeCount = nodesPerBlock;
						newBlock->next = nullptr;
						newBlock->prior = nullptr;
						headBlock = newBlock;
						tailBlock = headBlock;
					}
					else
					{
						newBlock->next = nullptr;
						newBlock->prior = tailBlock;
						tailBlock->next = newBlock;
						tailBlock = newBlock;
					}

					FreeNode* head = byteShift<FreeNode>(newBlock, blockDataSize);
					head->bias = 0;
					head->next = nullptr;
					head->prior = nullptr;
					headFreeNode = head;
					tailFreeNode = headFreeNode;

					for (size_t i = 1; i < nodesPerBlock; i++)
					{
						FreeNode* node = byteShift<FreeNode>(tailFreeNode, freeNodeSize);
						node->bias = tailFreeNode->bias + 1;
						node->prior = tailFreeNode;
						node->next = nullptr;
						tailFreeNode->next = node;
						tailFreeNode = node;
					}
				}

				FreeNode* returnNode = tailFreeNode;
				if (returnNode == headFreeNode)
				{
					headFreeNode = nullptr;
					tailFreeNode = headFreeNode;
				}
				else
				{
					tailFreeNode = tailFreeNode->prior;
					returnNode->prior = nullptr;
					tailFreeNode->next = nullptr;
				}

				byteShift<Block>(returnNode, shiftTable[returnNode->bias])->freeNodeCount--;
				return byteShift<void*>(returnNode, freeNodeOffset);
			}

			void deallocate(void* ptr)
			{
				FreeNode* node = byteShift<FreeNode>(ptr, -1 * freeNodeOffset);

				if (!headFreeNode)
				{
					node->next = nullptr;
					node->prior = nullptr;
					headFreeNode = node;
					tailFreeNode = headFreeNode;
				}
				else
				{
					node->prior = tailFreeNode;
					node->next = nullptr;
					tailFreeNode->next = node;
					tailFreeNode = node;
				}

				Block* belongBlock = byteShift<Block>(node, shiftTable[node->bias]);
				if (++belongBlock->freeNodeCount == nodesPerBlock)
				{
					if (belongBlock == headBlock)
					{
						if (belongBlock->next == nullptr)
						{
							headBlock = nullptr;
							tailBlock = headBlock;
						}
						else
						{
							headBlock = belongBlock->next;
							headBlock->prior = nullptr;
							belongBlock->next = nullptr;
						}
					}
					else if (belongBlock == tailBlock && tailBlock != headBlock)
					{
						tailBlock = tailBlock->prior;
						tailBlock->next = nullptr;
						belongBlock->prior = nullptr;
					}
					else
					{
						belongBlock->next->prior = belongBlock->prior;
						belongBlock->prior->next = belongBlock->next;
					}

					for (size_t i = 0; i < nodesPerBlock; i++)
					{
						FreeNode* n = byteShift<FreeNode>(belongBlock, blockDataSize + i * freeNodeSize);

						if (n == headFreeNode)
						{
							if (n->next == nullptr)
							{
								headFreeNode = nullptr;
								tailFreeNode = headFreeNode;
							}
							else
							{
								headFreeNode = n->next;
								headFreeNode->prior = nullptr;
								n->next = nullptr;
							}
						}
						else if (n == tailFreeNode && tailFreeNode != headFreeNode)
						{
							tailFreeNode = tailFreeNode->prior;
							tailFreeNode->next = nullptr;
							n->prior = nullptr;
						}
						else
						{
							n->next->prior = n->prior;
							n->prior->next = n->next;
						}
					}

					recycledBytes += blockSize;
					allocator->deallocate(belongBlock);
				}
			}

			size_t getRecycledBytes() const
			{
				return recycledBytes;
			}

			inline ObjectPool* instance()
			{
				std::lock_guard<std::mutex> locker(mutex_);
				return this;
			}

		private:
			const mint		nodeSize;
			const mint		freeNodeSize;
			const mint		blockSize;
			Allocator*		allocator;
			Block*			headBlock;
			Block*			tailBlock;
			FreeNode*		headFreeNode;
			FreeNode*		tailFreeNode;
			mint*			shiftTable;
			std::mutex		mutex_;
			size_t			recycledBytes;
		};
		
		class ObjectPoolArray
		{
		public:
			ObjectPoolArray(size_t size, size_t step)
				: size_(size),
				array(static_cast<ObjectPool*>(operator new (sizeof(ObjectPool)*size_)))
			{
				for (size_t i = 0; i < size; i++)
					new (array + i) ObjectPool(i*step + step);
			}

			~ObjectPoolArray()
			{
				for (size_t i = 0; i < size_; i++)
					(array + i)->~ObjectPool();
				operator delete(array);
			}

			size_t size() const { return size_; }

			ObjectPool& operator [] (size_t index) { return array[index]; }

		private:
			size_t			size_;
			ObjectPool*		array;
		};
	};

	class CpuMemoryHandler
	{
	public:
		static const size_t sizeStep = 8;
		static const size_t poolSize = 128;
		static const size_t maxSize = sizeStep * poolSize;

		CpuMemoryHandler()
			: allocator(new MoeLP_Memory_Internal::CpuAllocator())
		{}

		void* allocate(size_t size) throw (std::bad_alloc)
		{
			if (size == 0) throw std::bad_alloc();
			else if (size > maxSize)
				return allocator->allocate(size);
			else
				return pool[(size + sizeStep - 1) / sizeStep - 1].instance()->allocate();
		}

		void deallocate(void* ptr, size_t size)
		{
			if (size > maxSize)
				allocator->deallocate(ptr);
			else
				pool[(size + sizeStep - 1) / sizeStep - 1].instance()->deallocate(ptr);
		}

		size_t getRecycledBytes(size_t size)
		{
			return pool[(size + sizeStep - 1) / sizeStep - 1].instance()->getRecycledBytes();
		}

	private:
		MoeLP_Memory_Internal::Allocator* allocator;
		static MoeLP_Memory_Internal::ObjectPoolArray pool;
	};

	MoeLP_Memory_Internal::ObjectPoolArray
		CpuMemoryHandler::pool(CpuMemoryHandler::poolSize, CpuMemoryHandler::sizeStep);

	template<class T>
	class PoolAllocatorBase
	{
	public:
		~PoolAllocatorBase()
		{
			delete memoryHandler;
		}

		/**
		 * @brief allocate memory
		 */
		static void* allocate(size_t size)
		{
			return memoryHandler->allocate(size);
		}

		/**
		 * @brief deallocate memory
		 */
		static void deallocate(void* ptr, size_t size)
		{
			memoryHandler->deallocate(ptr, size);
		}

		/**
		 * @brief construct an object
		 * @param args: the args of the object's constructor
		 */
		template<class C, typename ...Args>
		static C* construct(Args&& ... args)
		{
			void* ptr = allocate(sizeof(C));
			new (ptr) C(std::forward<Args>(args)...);
			return static_cast<C*>(ptr);
		}

		/**
		 * @brief destroy an object and recycle the memory
		 */
		template<class C>
		static void destroy(C* ptr)
		{
			ptr->~C();
			deallocate(ptr, sizeof(C));
		}

		static size_t getRecycledBytes(size_t size)
		{
			return memoryHandler->getRecycledBytes(size);
		}

	private:
		static T* memoryHandler;
	};

	template<class T>
	T* PoolAllocatorBase<T>::memoryHandler = new T();

	using CpuPoolAllocator = PoolAllocatorBase<CpuMemoryHandler>;

	void* cpuAllocate(size_t size)
	{
		return CpuPoolAllocator::allocate(size);
	}

	void cpuDeallocate(void* ptr, size_t size)
	{
		CpuPoolAllocator::deallocate(ptr, size);
	}

	size_t cpuGetRecycledBytes(size_t size)
	{
		return CpuPoolAllocator::getRecycledBytes(size);
	}

	/**
	 * @brief a standard memory allocator
	 */
	template<typename T>
	class PoolAllocator
	{
	public:
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef T* pointer;
		typedef const T* const_pointer;
		typedef T& reference;
		typedef const T& const_reference;
		typedef T value_type;

		template<typename T1>
		struct rebind { typedef PoolAllocator<T1> other; };

		PoolAllocator() throw() {}
		PoolAllocator(PoolAllocator const&) throw() {}
		template<typename T1>
		PoolAllocator(PoolAllocator<T1> const&) throw() {}
		~PoolAllocator() throw() {}

		pointer address(reference r) const { return &r; }
		const_pointer address(const_reference r) const
		{
			return &r;
		}

		size_type max_size() const throw() { return size_t(-1) / sizeof(T); }

		void construct(pointer ptr, const_reference v) 
		{
			new ((void*)ptr) T(v);
		}

		void destroy(pointer ptr) { ptr->~T(); }

		pointer allocate(size_type n, const void* = 0) throw (std::bad_alloc) 
		{
			return static_cast<pointer>(CpuPoolAllocator::allocate(n * sizeof(T)));
		}

		void deallocate(pointer ptr, size_type n) 
		{
			CpuPoolAllocator::deallocate(ptr, n * sizeof(T));
		}
	};

	template<typename T>
	inline bool operator==(const PoolAllocator<T>&, const PoolAllocator<T>&)
	{
		return true;
	}

	template<typename T>
	inline bool operator!=(const PoolAllocator<T>&, const PoolAllocator<T>&)
	{
		return false;
	}

	/**
	 * @brief a shared pointer
	 * @detail the pointer is only used to manage memory allocated by CpuPoolAllocator
	 * if you want to manage other memory blocks please use std::shared_ptr etc.
	 * @example Ptr<type> p = Ptr<type>::create(sizeof(type), constructor args...)
	 */
	template<typename T>
	class Ptr
	{
		template<typename Other>
		friend class Ptr;

	public:

		Ptr()
			: refCounter(0),
			reference(0),
			size(0)
		{}

		Ptr(const Ptr<T>& pointer)
			: refCounter(pointer.refCounter),
			reference(pointer.reference),
			size(pointer.size)
		{
			incrementRefCounter();
		}

		Ptr(Ptr<T>&& pointer)
			: refCounter(pointer.refCounter),
			reference(pointer.reference),
			size(pointer.size)
		{
			pointer.refCounter = 0;
			pointer.reference = 0;
			pointer.size = 0;
		}

		template<typename C>
		Ptr(const Ptr<C>& pointer)
			: refCounter(0),
			reference(0),
			size(0)
		{
			T* convert = pointer.object();
			if (convert)
			{
				refCounter = pointer.getRefCounter();
				reference = convert;
				size = pointer.size;
				incrementRefCounter();
			}
		}

		~Ptr()
		{
			decrementRefCounter();
		}

		template<typename ...Args>
		static Ptr<T> create(size_t size, Args ...args)
		{
			void* ptr = CpuPoolAllocator::allocate(size);
			new (ptr) T(args...);
			return Ptr<T>(static_cast<T*>(ptr), size);
		}

		template<typename C>
		Ptr<C> cast() const
		{
			C* convert = dynamic_cast<C*>(reference);
			return Ptr<C>((convert ? refCounter : 0), convert, size);
		}

		Ptr<T>& operator=(const Ptr<T>& pointer)
		{
			if (this != &pointer)
			{
				decrementRefCounter();
				refCounter = pointer.refCounter;
				reference = pointer.reference;
				size = pointer.reference;
				incrementRefCounter();
			}
			return *this;
		}

		Ptr<T>& operator=(Ptr<T>&& pointer)
		{
			if (this != &pointer)
			{
				decrementRefCounter();
				refCounter = pointer.refCounter;
				reference = pointer.reference;
				size = pointer.size;

				pointer.refCounter = 0;
				pointer.reference = 0;
				pointer.size = 0;
			}
			return *this;
		}

		template<typename C>
		Ptr<T>& operator=(const Ptr<C>& pointer)
		{
			T* convert = pointer.object();
			decrementRefCounter();
			if (convert)
			{
				refCounter = pointer.refCounter;
				reference = convert;
				size = pointer.size;
				incrementRefCounter();
			}
			else
			{
				refCounter = 0;
				reference = 0;
				size = 0;
			}
			return *this;
		}

		bool operator==(const T* pointer)const
		{
			return reference == pointer;
		}

		bool operator!=(const T* pointer)const
		{
			return reference != pointer;
		}

		bool operator>(const T* pointer)const
		{
			return reference>pointer;
		}

		bool operator>=(const T* pointer)const
		{
			return reference >= pointer;
		}

		bool operator<(const T* pointer)const
		{
			return reference<pointer;
		}

		bool operator<=(const T* pointer)const
		{
			return reference <= pointer;
		}

		bool operator==(const Ptr<T>& pointer)const
		{
			return reference == pointer.reference;
		}

		bool operator!=(const Ptr<T>& pointer)const
		{
			return reference != pointer.reference;
		}

		bool operator>(const Ptr<T>& pointer)const
		{
			return reference>pointer.reference;
		}

		bool operator>=(const Ptr<T>& pointer)const
		{
			return reference >= pointer.reference;
		}

		bool operator<(const Ptr<T>& pointer)const
		{
			return reference<pointer.reference;
		}

		bool operator<=(const Ptr<T>& pointer)const
		{
			return reference <= pointer.reference;
		}

		operator bool() const
		{
			return reference != 0;
		}

		T* object() const
		{
			return reference;
		}

		T* operator->() const
		{
			return reference;
		}

		T& operator*() const
		{
			return *reference;
		}

		T& operator[](size_t index) const
		{
			MOE_ERROR(index >= 0 && index < size / sizeof(T), "template<typename T> Ptr<T>::operator[](size_t index): Argument index out of range.");
			return reference[index];
		}

	private:
		mutable volatile mint* refCounter;
		mutable T* reference;
		mutable size_t size;

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
				cpuDeallocate((void*)refCounter, sizeof(muint16));
				cpuDeallocate((void*)reference, size);

				refCounter = nullptr;
				reference = nullptr;
				size = 0;
			}
		}

		volatile mint* getRefCounter() const
		{
			return refCounter;
		}

		Ptr(T* pointer, size_t size)
			: refCounter(0),
			reference(0),
			size(size)
		{
			if (pointer)
			{
				refCounter = (mint*)cpuAllocate(sizeof(mint));
				reference = pointer;
				incrementRefCounter();
			}
			else
				size = 0;
		}

		Ptr(volatile mint* refCounter, T* reference, size_t size)
			: refCounter(refCounter),
			reference(reference),
			size(size)
		{
			incrementRefCounter();
		}
	};
}
#endif