#include "DynamicAllocator.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

#ifdef _MSC_VER
#include <intrin.h>
#endif

// [DIAG] 手动声明，避免引入 windows.h 造成宏污染
extern "C" __declspec(dllimport) void* __stdcall LoadLibraryA(const char*);
extern "C" __declspec(dllimport) void* __stdcall GetProcAddress(void*, const char*);

// The storage size in bytes of a node's user memory block size.
#define DSIZE_STORAGE sizeof(uint32_t)

struct AllocHeader {
	void* start;
	size_t alignment;
};

// ============================================================
// [DIAG-ALLOC-REGISTRY] 分配登记环形缓冲
// 目的：FreeAligned 失败（FreeBlock out of range / Free failed）时，
// 回查"坏块"与"坏块之前最近分配块"的分配调用栈，定位越界写的加害者。
// 堆损坏根因在崩溃点之前：坏块自身往往是受害者，加害者通常是其
// 前一块 user data 写超（覆盖本块 BlockSize/Header），或本块被重复释放。
// 登记表只保留最近 kAllocRegCapacity 次分配（覆盖最旧），线程安全由
// 调用方持有的 Mtx 保证（AllocateAligned/FreeAligned 均持锁）。
// ============================================================
struct AllocRegEntry {
	void* block;            // user data 起始（AllocateAligned 返回值）
	size_t size;            // 请求大小
	size_t alignment;
	uint32_t stack_count;
	void* stack[12];        // 分配调用栈
};
static constexpr uint32_t kAllocRegCapacity = 8192;
static AllocRegEntry s_AllocReg[kAllocRegCapacity];
static uint32_t s_AllocRegIndex = 0;

// 捕获调用栈（返回帧数）
static unsigned short CaptureStack(void** frames, unsigned long max) {
	typedef unsigned short(__stdcall* RtlCaptureFn)(unsigned long, unsigned long, void**, unsigned long*);
	static RtlCaptureFn s_capture = nullptr;
	if (!s_capture) {
		void* ntdll = LoadLibraryA("ntdll.dll");
		if (ntdll)
			s_capture = (RtlCaptureFn)GetProcAddress(ntdll, "RtlCaptureStackBackTrace");
	}
	if (s_capture)
		return s_capture(0, max, frames, nullptr);
	return 0;
}

// 登记一次分配（在 AllocateAligned 成功分支、持锁状态下调用）
static void RegisterAllocation(void* block, size_t size, size_t alignment) {
	AllocRegEntry& e = s_AllocReg[s_AllocRegIndex % kAllocRegCapacity];
	e.block = block;
	e.size = size;
	e.alignment = alignment;
	e.stack_count = CaptureStack(e.stack, 12);
	++s_AllocRegIndex;
}

// 在 FreeAligned 失败时回查登记表：优先精确匹配坏块；否则取地址低于坏块的最近分配块
static const AllocRegEntry* FindAllocRegistryEntry(void* block) {
	const AllocRegEntry* best = nullptr;
	size_t best_dist = (size_t)-1;
	uint32_t latest = s_AllocRegIndex > kAllocRegCapacity ? kAllocRegCapacity : s_AllocRegIndex;
	for (uint32_t i = 0; i < latest; ++i) {
		const AllocRegEntry& e = s_AllocReg[i];
		if (!e.block) continue;
		if (e.block == block) return &e;
		if ((size_t)e.block < (size_t)block) {
			size_t d = (size_t)block - (size_t)e.block;
			if (d < best_dist) { best_dist = d; best = &e; }
		}
	}
	return best;
}

static void LogRegistryEntry(const char* tag, const AllocRegEntry* e, void* block) {
	if (!e) {
		GLOG(Log::eError, "  [allocreg] %s: no registry entry for block 0x%p", tag, block);
		return;
	}
	GLOG(Log::eError, "  [allocreg] %s: block=0x%p size=%zu alignment=%zu frames=%u", tag, e->block, e->size, e->alignment, (unsigned)e->stack_count);
	for (uint32_t i = 0; i < e->stack_count; ++i)
		GLOG(Log::eError, "  [allocreg] %s stack#%u 0x%p", tag, (unsigned)i, e->stack[i]);
}

DynamicAllocator::DynamicAllocator() : DynamicAllocator(MEBIBYTES(10)) {}

DynamicAllocator::DynamicAllocator(size_t total_size) {
	if (total_size < 1) {
		GLOG(Log::eError, "Dynamic allocator create can not have a total_size of 0. Failed.");
		return;
	}

	TotalSize = total_size;
	if (!List.Create(TotalSize)) {
		return;
	}
	
	MemoryBlock = Platform::PlatformAllocate(total_size, false);
	if (MemoryBlock == nullptr) {
		GLOG(Log::eFatal, "DynamicAllocator::Create() Cannot allocate enough memory for dynamic allocator.");
	}

	Platform::PlatformSetMemory(MemoryBlock, 0, total_size);
}

DynamicAllocator& DynamicAllocator::Get() {
	// [FIX] 退出崩溃根因（ntdll 0xC0000005）：
	// 原实现为函数内 static 对象，在 main() 返回后 CRT 静态析构阶段按「构造逆序」
	// 最先被析构（~DynamicAllocator -> DestroyInternal）。但 EngineEvent::registered、
	// Console::RegisteredConsumers/Commands、Controller::KeymapStack、Identifier::Owners
	// 等 inline static TArray 在 main() 前静态初始化期构造、main() 返回后才析构：
	// 运行期若被填充非空，析构时 Empty()->Memory::Free() 会经 DynamicAllocator::Get()
	// 访问已销毁的分配器（MemoryBlock 已置空、Freelist 已 Destroy），触发堆访问违规；
	// 且 DestroyInternal 内部 GLOG("Shutdown memory system, left memory...") 在分配器
	// 自身销毁过程中再次访问分配器与 Console 静态容器，构成二次释放/悬垂访问。
	// 改为堆分配、永不析构的单例：分配器生命周期覆盖全部静态容器析构，进程退出时
	// 由操作系统回收内存池，彻底消除退出阶段析构顺序问题。
	static DynamicAllocator* AllocatorInstance = new DynamicAllocator();
	return *AllocatorInstance;
}

bool DynamicAllocator::Resize(size_t total_size) {
	if (total_size < 1) {
		GLOG(Log::eError, "Dynamic allocator create can not have a total_size of 0. Failed.");
		return false;
	}

	// [FIX] 持锁：Resize 会 Destroy 并重建整个 MemoryBlock，必须阻塞其它线程分配。
	std::lock_guard<std::recursive_mutex> lock(*Mtx);

	if (TotalSize > 0)  GLOG(Log::eWarn, "Dynamic allocator resize will clear all memory!");
	if (total_size < TotalSize) GLOG(Log::eWarn, "Dynamic allocator resize from %llu to %llu. Memory will be cut.", TotalSize, total_size);

	// 先清空（内部实现不加锁，锁由调用方持有，避免同一线程重复锁定 std::mutex）
	DestroyInternal();

	TotalSize = total_size;
	if (!List.Create(TotalSize)) {
		return false;
	}

	MemoryBlock = Platform::PlatformAllocate(total_size, false);
	if (MemoryBlock == nullptr) {
		GLOG(Log::eFatal, "DynamicAllocator::Create() Cannot allocate enough memory for dynamic allocator.");
	}

	Platform::PlatformSetMemory(MemoryBlock, 0, total_size);
	return true;
}

bool DynamicAllocator::Destroy() {
	std::lock_guard<std::recursive_mutex> lock(*Mtx);
	return DestroyInternal();
}

bool DynamicAllocator::DestroyInternal() {
	if (MemoryBlock != nullptr) {
		List.Destroy();
		Platform::PlatformFree(MemoryBlock, false);
		MemoryBlock = nullptr;
		TotalSize = 0;

		GLOG(Log::eInfo, "Shutdown memory system, left memory: %llu.", GetFreeSpace());
		return true;
	}

	return false;
}

void* DynamicAllocator::Allocate(size_t size) {
	return AllocateAligned(size, 1);
}

void* DynamicAllocator::AllocateAligned(size_t size, size_t alignment) {
	// [FIX] 持锁：防止 job 线程与主线程并发修改 FreeList 空闲块链表。
	std::lock_guard<std::recursive_mutex> lock(*Mtx);

	if (size > 0 && alignment > 0) {
		// The size required is based on the requested size, plus the alignment, header and a u32 to hold
		// the size for quick/easy lookups.
		size_t HeaderSize = sizeof(AllocHeader);
		size_t StorageSize = DSIZE_STORAGE;
		size_t RequiredSize = alignment + HeaderSize + StorageSize + size;

		// NOTE: This cast will really only be an issue on allocations over ~4GiB, so... don't do that.
		ASSERT(RequiredSize < 4294967295U);

		size_t BaseOffset = 0;
		if (List.AllocateBlock(RequiredSize, &BaseOffset)) {
			void* ptr = (void*)((size_t)MemoryBlock + BaseOffset);
			// Start the alignment after enough space to hold a u32. This allows for the u32 to be stored
			// immediately before the user block, while maintaining alignment on said user block.
			size_t AlignedBlockOffset = PaddingAligned((size_t)ptr + DSIZE_STORAGE, alignment);
			// Store the size just before the user data block
			uint32_t* BlockSize = (uint32_t*)(AlignedBlockOffset - DSIZE_STORAGE);
			*BlockSize = (uint32_t)size;

			// Store the header immediately after the user block.
			AllocHeader* Header = (AllocHeader*)(AlignedBlockOffset + size);
			Header->start = ptr;
			Header->alignment = alignment;

			// [DIAG-ALLOC-REGISTRY] 登记本次分配（持锁状态下）
			RegisterAllocation((void*)AlignedBlockOffset, size, alignment);

			return (void*)AlignedBlockOffset;
		}
		else {
			GLOG(Log::eWarn, "DynamicAllocator::AllocateAligned() allocate no blocks of memory large enough to allocate from.");
			size_t available = List.GetFreeSpace();
			GLOG(Log::eWarn, "Requested size: %llu, Total space available: %llu.", size, available);
			// TODO: Report fragmentation?
			return nullptr;
		}
	}

	GLOG(Log::eError, "Dynamic allocator allocate requires a valid size and alignment. size=%llu alignment=%llu", size, alignment);
	// [DIAG] 打印调用栈定位 size==0 / alignment==0 分配来源
	{
		typedef unsigned short(__stdcall* RtlCaptureFn)(unsigned long, unsigned long, void**, unsigned long*);
		static RtlCaptureFn s_capture = nullptr;
		if (!s_capture) {
			void* ntdll = LoadLibraryA("ntdll.dll");
			if (ntdll)
				s_capture = (RtlCaptureFn)GetProcAddress(ntdll, "RtlCaptureStackBackTrace");
		}
		if (s_capture) {
			void* frames[20] = {};
			unsigned short n = s_capture(0, 20, frames, nullptr);
			GLOG(Log::eError, "  [alloc0-stack] %u frames:", (unsigned)n);
			for (unsigned short i = 0; i < n; ++i) {
				GLOG(Log::eError, "  [alloc0-stack] #%u 0x%p", (unsigned)i, frames[i]);
			}
		}
	}
	return nullptr;
}

bool DynamicAllocator::Free(void* block, size_t size) {
	if (block != nullptr) {
		size_t stored_size;
		size_t alignment;
		if (GetAlignmentSize(block, &stored_size, &alignment)) {
			if (stored_size != size) {
				GLOG(Log::eWarn, "Size mismatch in Free: expected %zu, got %zu", stored_size, size);
			}
		}
	}
	return FreeAligned(block);
}

bool DynamicAllocator::FreeAligned(void* block) {
	// [FIX] 持锁：与 AllocateAligned 互斥，防止并发 FreeBlock 破坏空闲链表。
	std::lock_guard<std::recursive_mutex> lock(*Mtx);

	if (block == nullptr || MemoryBlock == nullptr) {
		GLOG(Log::eError, "DynamicAllocator::FreeAligned(): Free requires a valid block (0x%p).", block);
		return false;
	}

	void* EndOfBlock = (void*)((size_t)MemoryBlock + TotalSize);
	if (block < MemoryBlock || block >= EndOfBlock) {
		GLOG(Log::eError, "DynamicAllocator::FreeAligned(): Trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p). Sub size: %uul, Total size: %uul.",
			block, MemoryBlock, EndOfBlock, (size_t)block - (size_t)MemoryBlock, TotalSize);
		// [DIAG] 通过 ntdll RtlCaptureStackBackTrace 打印调用栈定位越界释放来源
		{
			typedef unsigned short(__stdcall* RtlCaptureFn)(unsigned long, unsigned long, void**, unsigned long*);
			static RtlCaptureFn s_capture = nullptr;
			if (!s_capture) {
				void* ntdll = LoadLibraryA("ntdll.dll");
				if (ntdll)
					s_capture = (RtlCaptureFn)GetProcAddress(ntdll, "RtlCaptureStackBackTrace");
			}
			if (s_capture) {
				void* frames[20] = {};
				unsigned short n = s_capture(0, 20, frames, nullptr);
				GLOG(Log::eError, "  [stack] %u frames:", (unsigned)n);
				for (unsigned short i = 0; i < n; ++i) {
					GLOG(Log::eError, "  [stack] #%u 0x%p", (unsigned)i, frames[i]);
				}
			} else {
				GLOG(Log::eError, "  [stack] RtlCaptureStackBackTrace unavailable");
			}
		}
		return false;
	}

	uint32_t* BlockSize = (uint32_t*)((size_t)block - DSIZE_STORAGE);
	AllocHeader* Header = (AllocHeader*)((size_t)block + *BlockSize);
	size_t RequiredSize = Header->alignment + sizeof(AllocHeader) + DSIZE_STORAGE + *BlockSize;
	size_t Offset = (size_t)Header->start - (size_t)MemoryBlock;

	if (!List.FreeBlock(RequiredSize, Offset)) {
		GLOG(Log::eError, "DynamicAllocator::FreeAligned(): Free failed.");
		// [DIAG2] 打印被释放块的元数据快照：若 BlockSize/Header->start 已非原值，
		// 说明该块被越界写破坏（常见于前一块 user data 写超、或双释放后元数据被改写）。
		GLOG(Log::eError, "  [freefail-detail] block=0x%p BlockSize=%u Header.start=0x%p Header.alignment=%zu RequiredSize=%zu Offset=%zu TotalSize=%zu",
			block, (unsigned)(*BlockSize), Header->start, Header->alignment, RequiredSize, Offset, TotalSize);
		// [DIAG-ALLOC-REGISTRY] 回查坏块及其前一块的分配登记
		{
			const AllocRegEntry* e = FindAllocRegistryEntry(block);
			if (e && e->block == block) {
				LogRegistryEntry("badblock", e, block);
			} else {
				LogRegistryEntry("nearest-prev", e, block);
			}
		}
		// [DIAG3] dump 崩溃块 user data 首尾字节与相邻块信息，判断该块保存的对象类型
		// 及越界写来源（前一块 user data 写超 vs 本块内容被篡改）。
		{
			const unsigned char* ud = (const unsigned char*)block;
			GLOG(Log::eError, "  [freefail-mem] MemoryBlock=0x%p block_offset=%zu", MemoryBlock, (size_t)block - (size_t)MemoryBlock);
			GLOG(Log::eError, "  [freefail-mem] userdata[0..31]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
				ud[0], ud[1], ud[2], ud[3], ud[4], ud[5], ud[6], ud[7],
				ud[8], ud[9], ud[10], ud[11], ud[12], ud[13], ud[14], ud[15],
				ud[16], ud[17], ud[18], ud[19], ud[20], ud[21], ud[22], ud[23],
				ud[24], ud[25], ud[26], ud[27], ud[28], ud[29], ud[30], ud[31]);
			if (*BlockSize >= 32) {
				const unsigned char* tail = ud + (*BlockSize) - 32;
				GLOG(Log::eError, "  [freefail-mem] userdata[sz-32..sz-1]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
					tail[0], tail[1], tail[2], tail[3], tail[4], tail[5], tail[6], tail[7],
					tail[8], tail[9], tail[10], tail[11], tail[12], tail[13], tail[14], tail[15],
					tail[16], tail[17], tail[18], tail[19], tail[20], tail[21], tail[22], tail[23],
					tail[24], tail[25], tail[26], tail[27], tail[28], tail[29], tail[30], tail[31]);
			}
			// 块末尾（Header 区）附近 32 字节
			const unsigned char* hd = (const unsigned char*)((size_t)block + *BlockSize);
			GLOG(Log::eError, "  [freefail-mem] header_area[0..31]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
				hd[0], hd[1], hd[2], hd[3], hd[4], hd[5], hd[6], hd[7],
				hd[8], hd[9], hd[10], hd[11], hd[12], hd[13], hd[14], hd[15],
				hd[16], hd[17], hd[18], hd[19], hd[20], hd[21], hd[22], hd[23],
				hd[24], hd[25], hd[26], hd[27], hd[28], hd[29], hd[30], hd[31]);
			// [DIAG4] dump 完整块内容（hex+ascii）识别对象布局与越界写来源
			{
				const size_t dump_front = (*BlockSize > 256) ? 256 : *BlockSize;
				const unsigned char* dd = (const unsigned char*)block;
				for (size_t off = 0; off < dump_front; off += 16) {
					char hx[96]; char asc[24]; int hp = 0; int ap = 0;
					for (size_t k = 0; k < 16 && off + k < dump_front; ++k) {
						unsigned char b = dd[off + k];
						hp += snprintf(hx + hp, sizeof(hx) - (size_t)hp, "%02X ", b);
						asc[ap++] = (b >= 32 && b < 127) ? (char)b : '.';
					}
					asc[ap] = 0;
					GLOG(Log::eError, "  [front] +%04zu: %s | %s", off, hx, asc);
				}
				if (*BlockSize > 512) {
					size_t tail_start = (*BlockSize > 1024) ? (*BlockSize - 512) : 512;
					const unsigned char* tt = dd + tail_start;
					size_t tt_len = *BlockSize - tail_start + 32;
					for (size_t off = 0; off < tt_len; off += 16) {
						char hx[96]; char asc[24]; int hp = 0; int ap = 0;
						for (size_t k = 0; k < 16 && off + k < tt_len; ++k) {
							unsigned char b = tt[off + k];
							hp += snprintf(hx + hp, sizeof(hx) - (size_t)hp, "%02X ", b);
							asc[ap++] = (b >= 32 && b < 127) ? (char)b : '.';
						}
						asc[ap] = 0;
						GLOG(Log::eError, "  [tail] +%04zu: %s | %s", off, hx, asc);
					}
				}
			}
			// [DIAG5] 前一块尾部 64 字节（含前一块 Header 与本块 BlockSize），判断越界写来源
			{
				const unsigned char* pv = ud;
				if ((size_t)block >= (size_t)MemoryBlock + 64) {
					pv = (const unsigned char*)((size_t)block - 64);
				}
				for (size_t off = 0; off < 64; off += 16) {
					char hx[96]; char asc[24]; int hp = 0; int ap = 0;
					for (size_t k = 0; k < 16; ++k) {
						unsigned char b = pv[off + k];
						hp += snprintf(hx + hp, sizeof(hx) - (size_t)hp, "%02X ", b);
						asc[ap++] = (b >= 32 && b < 127) ? (char)b : '.';
					}
					asc[ap] = 0;
					GLOG(Log::eError, "  [prev] +%04zu: %s | %s", off, hx, asc);
				}
			}
			// 下一块（若有）的 BlockSize 与 Header，判断下一块是否也被破坏
			void* next_header_start = (void*)((size_t)block + *BlockSize + sizeof(AllocHeader));
			uint32_t* next_bs = (uint32_t*)((size_t)next_header_start + DSIZE_STORAGE);
			void* next_block = (void*)((size_t)next_header_start + DSIZE_STORAGE + 4);
			if ((size_t)next_block >= (size_t)MemoryBlock && (size_t)next_block + 4 <= (size_t)MemoryBlock + TotalSize) {
				GLOG(Log::eError, "  [freefail-mem] next_block_candidate=0x%p next_BlockSize=%u", next_block, (unsigned)(*next_bs));
			}
		}
		// [DIAG] FreeBlock 失败：通常意味着该块元数据被越界写破坏，或重复释放。
		// 打印调用栈定位"谁在释放坏块"，结合引擎/编辑器帧判断越界写的来源。
		{
			typedef unsigned short(__stdcall* RtlCaptureFn)(unsigned long, unsigned long, void**, unsigned long*);
			static RtlCaptureFn s_capture = nullptr;
			if (!s_capture) {
				void* ntdll = LoadLibraryA("ntdll.dll");
				if (ntdll)
					s_capture = (RtlCaptureFn)GetProcAddress(ntdll, "RtlCaptureStackBackTrace");
			}
			if (s_capture) {
				void* frames[24] = {};
				unsigned short n = s_capture(0, 24, frames, nullptr);
				GLOG(Log::eError, "  [freefail-stack] %u frames:", (unsigned)n);
				for (unsigned short i = 0; i < n; ++i) {
					GLOG(Log::eError, "  [freefail-stack] #%u 0x%p", (unsigned)i, frames[i]);
				}
			} else {
				GLOG(Log::eError, "  [freefail-stack] RtlCaptureStackBackTrace unavailable");
			}
		}
		return false;
	}

	return true;
}

bool DynamicAllocator::GetAlignmentSize(void* block, size_t* out_size, size_t* out_alignment) {
	// 添加基本的安全检查
	if (block == nullptr || MemoryBlock == nullptr) {
		return false;
	}

	// 边界检查
	void* EndOfMemory = (void*)((size_t)MemoryBlock + TotalSize);
	if (block < MemoryBlock || block >= EndOfMemory) {
		return false;
	}

	// 检查偏移量
	size_t offset = (size_t)block - (size_t)MemoryBlock;
	if (offset < DSIZE_STORAGE) {
		return false;
	}

	// 检查BlockSize指针
	void* BlockSizePtr = (void*)((size_t)block - DSIZE_STORAGE);
	if (BlockSizePtr < MemoryBlock || BlockSizePtr >= EndOfMemory) {
		return false;
	}

	// 安全读取块大小
	uint32_t block_size = *(uint32_t*)BlockSizePtr;
	if (block_size == 0 || block_size > TotalSize) {
		return false;
	}

	// 检查Header指针
	void* HeaderPtr = (void*)((size_t)block + block_size);
	if (HeaderPtr < MemoryBlock ||
		(size_t)HeaderPtr + sizeof(AllocHeader) >(size_t)EndOfMemory) {
		return false;
	}

	// 安全读取头部
	AllocHeader* Header = (AllocHeader*)HeaderPtr;
	if (Header->alignment == 0 || Header->alignment > 1024) {
		return false;
	}

	if (out_size) *out_size = block_size;
	if (out_alignment) *out_alignment = Header->alignment;
	
	return true;
}

size_t DynamicAllocator::GetTotalSpace() {
	return TotalSize;
}

size_t DynamicAllocator::GetFreeSpace() {
	return List.GetFreeSpace();
}

size_t DynamicAllocator::AllocatorHeaderSize() {
	// Enough space for a header and size storage.
	return sizeof(AllocHeader) + DSIZE_STORAGE;
}
