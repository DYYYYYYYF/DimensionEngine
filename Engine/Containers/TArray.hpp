#pragma once

#include "Defines.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"
#include <type_traits>
#include <utility>
#include <algorithm>

#define ARRAY_DEFAULT_CAPACITY 1
#define ARRAY_DEFAULT_RESIZE_FACTOR 2

/*
Memory layout
size_t(unsigned long long) capacity = number elements that can be held
size_t(unsigned long long) length = number of elements currently contained
size_t(unsigned long long) stride = size of each element in bytes
void* elements
*/
// 判断类型是否完整：元素类型可能仅为前向声明（不完整类型），
// 此时跳过元素的显式析构，避免 error C2027（std::vector 对不完整类型不实例化析构，
// TArray 需显式做同样的保护）。
template<typename T, typename = void>
struct TIsCompleteType : std::false_type {};

template<typename T>
struct TIsCompleteType<T, std::void_t<decltype(sizeof(T))>> : std::true_type {};

template<typename ElementType>
class DAPI TArray {
public:
	// [FIX] 默认构造不再分配内存（对齐 std::vector 语义）。
	// 原实现默认即分配 1 个元素，导致全局/静态 TArray（如 EngineEvent::registered、
	// Console::RegisteredConsumers）在 main() 之前的静态初始化期就从 DynamicAllocator
	// 的 10MB 初始块分配内存；随后 Memory::Initialize(500MB) 的 Resize 会销毁该旧块，
	// 使这些 TArray 持有的指针悬垂，首次 push/resize/reserve 释放时触发
	// FreeAligned 失败/越界释放，进而 0xC0000005 崩溃。
	TArray() : ArrayMemory(nullptr), Capacity(0), Stride(sizeof(ElementType)), Length(0) {}

	// 拷贝构造函数 - 修正了内存分配失败处理
	TArray(const TArray& arr) : ArrayMemory(nullptr), Capacity(0), Stride(sizeof(ElementType)), Length(0) {
		if (arr.ArrayMemory == nullptr || arr.Length == 0) {
			return; // [FIX] 源为空时保持空数组，不再 *this = TArray()（会无限递归）
		}

		Capacity = arr.Capacity;
		Length = arr.Length;

		size_t ArrayMemSize = Capacity * Stride;
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		if (!ArrayMemory) {
			GLOG(Log::eError, "Failed to allocate memory in copy constructor");
			Capacity = 0;
			Length = 0;
			return;
		}

		// 使用placement new和拷贝构造函数
		for (size_t i = 0; i < Length; ++i) {
			try {
				new(ArrayMemory + i) ElementType(arr[i]);
			}
			catch (...) {
				// 异常安全：销毁已构造的元素
				for (size_t j = 0; j < i; ++j) {
					ArrayMemory[j].~ElementType();
				}
				Memory::Free(ArrayMemory, MemoryType::eMemory_Type_Array);
				ArrayMemory = nullptr;
				Capacity = 0;
				Length = 0;
				throw;
			}
		}
	}

	// 构造函数 - 修正了非指针类型的初始化
	TArray(size_t size) : ArrayMemory(nullptr), Capacity(0), Stride(sizeof(ElementType)), Length(0) {
		if (size == 0) {
			return; // [FIX] 保持空数组
		}

		size_t ArrayMemSize = size * sizeof(ElementType);
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		if (!ArrayMemory) {
			GLOG(Log::eError, "Failed to allocate memory for TArray with size %zu", size);
			return;
		}

		Capacity = size;
		Length = size;

		if constexpr (std::is_pointer<ElementType>::value) {
			Platform::PlatformSetMemory(ArrayMemory, 0, ArrayMemSize);
		}
		else {
			for (size_t i = 0; i < size; ++i) {
				try {
					new(ArrayMemory + i) ElementType();
				}
				catch (...) {
					// 异常安全：销毁已构造的元素
					for (size_t j = 0; j < i; ++j) {
						ArrayMemory[j].~ElementType();
					}
					Memory::Free(ArrayMemory, MemoryType::eMemory_Type_Array);
					ArrayMemory = nullptr;
					Capacity = 0;
					Length = 0;
					throw;
				}
			}
		}
	}

	// 构造：填充 size 个 value（std::vector(size, value) 兼容）
	TArray(size_t size, const ElementType& value) : ArrayMemory(nullptr), Capacity(0), Stride(sizeof(ElementType)), Length(0) {
		if (size == 0) {
			return; // [FIX] 保持空数组
		}

		size_t ArrayMemSize = size * sizeof(ElementType);
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		if (!ArrayMemory) {
			GLOG(Log::eError, "Failed to allocate memory for TArray with size %zu and fill value", size);
			return;
		}

		Capacity = size;
		Length = size;

		if constexpr (std::is_pointer<ElementType>::value) {
			Platform::PlatformSetMemory(ArrayMemory, 0, ArrayMemSize);
			for (size_t i = 0; i < size; ++i) {
				new(ArrayMemory + i) ElementType(value);
			}
		}
		else {
			for (size_t i = 0; i < size; ++i) {
				try {
					new(ArrayMemory + i) ElementType(value);
				}
				catch (...) {
					for (size_t j = 0; j < i; ++j) {
						ArrayMemory[j].~ElementType();
					}
					Memory::Free(ArrayMemory, MemoryType::eMemory_Type_Array);
					ArrayMemory = nullptr;
					Capacity = 0;
					Length = 0;
					throw;
				}
			}
		}
	}

	TArray(std::initializer_list<ElementType> list)
		: ArrayMemory(nullptr), Capacity(0), Stride(sizeof(ElementType)), Length(0)
	{
		if (list.size() == 0) {
			return; // [FIX] 保持空数组
		}

		size_t ArrayMemSize = list.size() * sizeof(ElementType);
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		if (!ArrayMemory) {
			GLOG(Log::eError, "Failed to allocate memory for TArray initializer_list");
			return;
		}

		Capacity = list.size();
		Length = list.size();

		size_t i = 0;
		for (const ElementType& elem : list) {
			try {
				new(ArrayMemory + i) ElementType(elem);
				++i;
			}
			catch (...) {
				for (size_t j = 0; j < i; ++j) {
					ArrayMemory[j].~ElementType();
				}
				Memory::Free(ArrayMemory, MemoryType::eMemory_Type_Array);
				ArrayMemory = nullptr;
				Capacity = 0;
				Length = 0;
				throw;
			}
		}
	}

	virtual ~TArray() {
		Empty();
	}

	ElementType* begin() {
		return ArrayMemory;
	}

	const ElementType* begin() const {
		return ArrayMemory;
	}

	ElementType* end() {
		return ArrayMemory + Length;
	}

	const ElementType* end() const {
		return ArrayMemory + Length;
	}

public:
	// 修正了Resize函数的异常安全性
	void Resize(size_t newSize = 0) {
		size_t NewCapacity = newSize > 0 ? newSize : Capacity * ARRAY_DEFAULT_RESIZE_FACTOR;
		// [FIX] 空数组（默认构造后 Capacity==0）首次扩容至少分配 1 个元素，
		// 否则 NewCapacity 恒为 0，会误入下方 newSize==0 分支直接 return，
		// 导致 Push/Emplace 对 nullptr 的 ArrayMemory 执行 placement new。
		if (NewCapacity == 0) {
			NewCapacity = ARRAY_DEFAULT_CAPACITY;
		}

		if (NewCapacity == Capacity) {
			if (newSize > 0 && newSize != Length) {
				// 只改变Length，不重新分配内存
				if (newSize > Length) {
					// 扩展：构造新元素
					for (size_t i = Length; i < newSize; ++i) {
						new(ArrayMemory + i) ElementType();
					}
				}
				else {
					// 收缩：销毁多余元素
					for (size_t i = newSize; i < Length; ++i) {
						if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
							ArrayMemory[i].~ElementType();
						}
					}
				}
				Length = newSize;
			}
			return;
		}

		ElementType* TempMemory = (ElementType*)Memory::Allocate(NewCapacity * Stride, MemoryType::eMemory_Type_Array);
		if (!TempMemory) {
			GLOG(Log::eError, "Failed to allocate memory during resize");
			return;
		}

		size_t copyLength = (newSize > 0 && newSize < Length) ? newSize : Length;

		// 移动或拷贝现有元素
		if (ArrayMemory) {
			for (size_t i = 0; i < copyLength; ++i) {
				try {
					// 如果ElementType为unique_ptr会失败
					/*if constexpr (std::is_move_constructible<ElementType>::value && !std::is_pointer<ElementType>::value) {
						new(TempMemory + i) ElementType(std::move(ArrayMemory[i]));
					}
					else {
						new(TempMemory + i) ElementType(ArrayMemory[i]);
					}*/

					new(TempMemory + i) ElementType(std::move(ArrayMemory[i]));
				}
				catch (...) {
					// 异常安全：清理已构造的元素
					for (size_t j = 0; j < i; ++j) {
						TempMemory[j].~ElementType();
					}
					Memory::Free(TempMemory, MemoryType::eMemory_Type_Array);
					throw;
				}
			}

			// 销毁原数组中的元素
			if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
				for (size_t i = 0; i < Length; ++i) {
					ArrayMemory[i].~ElementType();
				}
			}
			Memory::Free(ArrayMemory, MemoryType::eMemory_Type_Array);
		}

		// 如果新大小大于原长度，构造新元素
		if (newSize > copyLength) {
			for (size_t i = copyLength; i < newSize; ++i) {
				new(TempMemory + i) ElementType();
			}
		}

		ArrayMemory = TempMemory;
		Capacity = NewCapacity;
		Length = newSize > 0 ? newSize : copyLength;
	}

	void Push(const ElementType& value) {
		if (Length >= Capacity) {
			Resize();
		}

		try {
			new(ArrayMemory + Length) ElementType(value);
			Length++;
		}
		catch (...) {
			GLOG(Log::eError, "Failed to push element");
			throw;
		}
	}

	void Push(ElementType&& value)
	{
		if (Length >= Capacity)
			Resize();

		new(ArrayMemory + Length) ElementType(std::move(value));
		Length++;
	}

	// 直接构造对象
	/**
	 * XXX.Emplace(std::make_unique<AActor>());
	 * XXX.Emplace(std::move(child));
	 */
	template<typename... Args>
	ElementType& Emplace(Args&&... args)
	{
		if (Length >= Capacity)
			Resize();

		new(ArrayMemory + Length) ElementType(std::forward<Args>(args)...);
		return ArrayMemory[Length++];
	}

	void InsertAt(size_t index, const ElementType& val) {
		if (index > Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %zu", Length, index);
			return;
		}

		if (Length >= Capacity) {
			Resize();
		}

		// 从后往前移动元素，为新元素腾出空间
		for (size_t i = Length; i > index; --i) {
			try {
				new(ArrayMemory + i) ElementType(std::move(ArrayMemory[i - 1]));
				ArrayMemory[i - 1].~ElementType();
			}
			catch (...) {
				GLOG(Log::eError, "Failed to move element during insertion");
				throw;
			}
		}

		// 在指定位置构造新元素
		try {
			new(ArrayMemory + index) ElementType(val);
			Length++;
		}
		catch (...) {
			GLOG(Log::eError, "Failed to insert element at index %zu", index);
			throw;
		}
	}

	ElementType Pop() {
		if (Length < 1) {
			GLOG(Log::eError, "Trying to pop from empty array");
			return ElementType();
		}

		ElementType result = std::move(ArrayMemory[Length - 1]);

		if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
			ArrayMemory[Length - 1].~ElementType();
		}

		Length--;
		return result;
	}

	ElementType PopAt(size_t index) {
		if (index >= Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %zu", Length, index);
			return ElementType();
		}

		ElementType result = std::move(ArrayMemory[index]);

		// 向前移动后续元素
		for (size_t i = index; i < Length - 1; ++i) {
			ArrayMemory[i].~ElementType();
			new(ArrayMemory + i) ElementType(std::move(ArrayMemory[i + 1]));
		}

		if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
			ArrayMemory[Length - 1].~ElementType();
		}

		Length--;
		return result;
	}

	// 移除指定下标的元素（后续元素前移），下标越界时不执行任何操作
	void RemoveAt(size_t index) {
		if (index >= Length) {
			GLOG(Log::eError, "RemoveAt index out of bounds! Length: %zu, Index: %zu", Length, index);
			return;
		}

		// 向前移动后续元素
		for (size_t i = index; i < Length - 1; ++i) {
			ArrayMemory[i].~ElementType();
			new(ArrayMemory + i) ElementType(std::move(ArrayMemory[i + 1]));
		}

		if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
			ArrayMemory[Length - 1].~ElementType();
		}

		Length--;
	}

	// 仅预留容量（不改变已有元素数量），容量不足时重新分配
	void Reserve(size_t newCapacity) {
		if (newCapacity <= Capacity) {
			return;
		}

		ElementType* TempMemory = (ElementType*)Memory::Allocate(newCapacity * sizeof(ElementType), MemoryType::eMemory_Type_Array);
		if (!TempMemory) {
			GLOG(Log::eError, "Failed to allocate memory during reserve");
			return;
		}

		for (size_t i = 0; i < Length; ++i) {
			new(TempMemory + i) ElementType(std::move(ArrayMemory[i]));
		}

		if (ArrayMemory != nullptr) {
			if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
				for (size_t i = 0; i < Length; ++i) {
					ArrayMemory[i].~ElementType();
				}
			}
			Memory::Free(ArrayMemory, MemoryType::eMemory_Type_Array);
		}

		ArrayMemory = TempMemory;
		Capacity = newCapacity;
	}

	void Clear() {
		if (ArrayMemory != nullptr) {
			if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
				for (size_t i = 0; i < Length; ++i) {
					ArrayMemory[i].~ElementType();
				}
			}
			Length = 0;
		}
	}

	void Empty() {
		if (ArrayMemory != nullptr) {
			if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
				for (size_t i = 0; i < Length; ++i) {
					ArrayMemory[i].~ElementType();
				}
			}
			Memory::Free(ArrayMemory, MemoryType::eMemory_Type_Array);
			ArrayMemory = nullptr;
		}

		Capacity = 0;
		Length = 0;
		// [FIX] Empty() 后保留 Stride：数组为空但元素类型未变，
		// 之后 push_back 时 Allocate(NewCapacity * Stride) 才能按真实元素大小分配。
	}

	bool IsEmpty() const { return Length == 0; }
	size_t Size() const { return Length; }
	size_t GetCapacity() const { return Capacity; }

	ElementType* Data() { return ArrayMemory; }

	// 排序：替代标准库写法 std::sort(Array.begin(), Array.end(), cmp)
	// 元素类型需为完整类型；内部以原生指针作为随机访问迭代器
	template<typename Compare>
	void Sort(Compare compare) {
		if (Length > 1) {
			std::sort(ArrayMemory, ArrayMemory + Length, compare);
		}
	}

	void Sort() {
		if (Length > 1) {
			std::sort(ArrayMemory, ArrayMemory + Length);
		}
	}

	const ElementType* Data() const { return ArrayMemory; }

	// 修正了赋值操作符 - 添加了自赋值检查和异常安全
	TArray<ElementType>& operator=(const TArray<ElementType>& other) {
		if (this == &other) {
			return *this; // 自赋值检查
		}

		// 先销毁当前数据
		Empty();

		if (other.ArrayMemory == nullptr || other.Length == 0) {
			return *this; // [FIX] 源为空时保持空数组，不再 *this = TArray()（会无限递归）
		}

		Capacity = other.Capacity;
		Stride = other.Stride;
		Length = other.Length;

		size_t ArrayMemSize = Capacity * Stride;
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		if (!ArrayMemory) {
			GLOG(Log::eError, "Failed to allocate memory in assignment operator");
			Capacity = 0;
			Length = 0;
			return *this;
		}

		for (size_t i = 0; i < Length; ++i) {
			try {
				new(ArrayMemory + i) ElementType(other[i]);
			}
			catch (...) {
				// 异常安全：清理已构造的元素
				for (size_t j = 0; j < i; ++j) {
					ArrayMemory[j].~ElementType();
				}
				Memory::Free(ArrayMemory, MemoryType::eMemory_Type_Array);
				ArrayMemory = nullptr;
				Capacity = 0;
				Length = 0;
				throw;
			}
		}

		return *this;
	}

	// 添加移动语义支持
	TArray(TArray&& other) noexcept
		: ArrayMemory(other.ArrayMemory), Capacity(other.Capacity),
		Stride(other.Stride), Length(other.Length) {
		other.ArrayMemory = nullptr;
		other.Capacity = 0;
		// [FIX] 移动后保留 Stride：元素类型不变，Stride 不应归零。
		// 归零会使源对象后续 push_back/Resize 走 Allocate(NewCapacity*0)=Allocate(0)
		// 拿到 0 字节块/空指针，placement new 或元素访问触发 0xC0000005。
		other.Length = 0;
	}

	TArray& operator=(TArray&& other) noexcept {
		if (this != &other) {
			Empty();

			ArrayMemory = other.ArrayMemory;
			Capacity = other.Capacity;
			Stride = other.Stride;
			Length = other.Length;

			other.ArrayMemory = nullptr;
			other.Capacity = 0;
			// [FIX] 移动后保留 Stride（理由同上：源对象复用需按真实元素大小分配）。
			other.Length = 0;
		}
		return *this;
	}

	template<typename IntegerType>
	ElementType& operator[](const IntegerType& i) {
		if (static_cast<size_t>(i) >= Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %lld", Length, static_cast<long long>(i));
		}
		return ArrayMemory[i];
	}

	template<typename IntegerType>
	const ElementType& operator[](const IntegerType& i) const {
		if (static_cast<size_t>(i) >= Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %lld", Length, static_cast<long long>(i));
		}
		return ArrayMemory[i];
	}

	// ══════════════════════════════════════════════════════════════
	//  std::vector 兼容别名（无侵入替换 std::vector 时使用）
	// ══════════════════════════════════════════════════════════════
	using value_type = ElementType;
	using size_type = size_t;
	using reference = ElementType&;
	using const_reference = const ElementType&;
	using pointer = ElementType*;
	using const_pointer = const ElementType*;
	// 迭代器即裸指针（与 begin()/end() 返回类型一致）
	using iterator = ElementType*;
	using const_iterator = const ElementType*;

	void push_back(const ElementType& value) { Push(value); }
	void push_back(ElementType&& value) { Push(std::move(value)); }

	template<typename... Args>
	reference emplace_back(Args&&... args) { return Emplace(std::forward<Args>(args)...); }

	reference at(size_t i) {
		if (i >= Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %zu", Length, i);
			// std::vector::at 语义：越界抛出异常；这里在越界时返回末尾元素兜底，
			// 避免无界 UB（调用方本应保证索引有效）。
			i = Length > 0 ? Length - 1 : 0;
		}
		return ArrayMemory[i];
	}

	const_reference at(size_t i) const {
		if (i >= Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %zu", Length, i);
			i = Length > 0 ? Length - 1 : 0;
		}
		return ArrayMemory[i];
	}

	// 删除指定迭代器位置的元素，返回下一个元素的迭代器（std::vector::erase 语义）
	iterator erase(iterator pos) {
		size_t idx = static_cast<size_t>(pos - begin());
		if (idx >= Length) return end();
		RemoveAt(idx);
		return begin() + idx;
	}

	iterator erase(const_iterator pos) {
		size_t idx = static_cast<size_t>(pos - cbegin());
		if (idx >= Length) return end();
		RemoveAt(idx);
		return begin() + idx;
	}

	// 删除 [first, last) 范围内的元素，返回 last 之后的下一个元素迭代器
	iterator erase(iterator first, iterator last) {
		size_t first_idx = static_cast<size_t>(first - begin());
		size_t last_idx = static_cast<size_t>(last - begin());
		if (first_idx >= Length) return end();
		if (last_idx > Length) last_idx = Length;
		if (last_idx <= first_idx) return begin() + last_idx;

		size_t count = last_idx - first_idx;
		// 把 last 之后的元素逐个移动回 first 处
		for (size_t i = first_idx; i + count < Length; ++i) {
			ArrayMemory[i].~ElementType();
			new (ArrayMemory + i) ElementType(std::move(ArrayMemory[i + count]));
		}
		// 销毁尾部多余元素
		if constexpr (TIsCompleteType<ElementType>::value && !std::is_pointer<ElementType>::value) {
			for (size_t i = Length - count; i < Length; ++i) {
				ArrayMemory[i].~ElementType();
			}
		}
		Length -= count;
		return begin() + first_idx;
	}

	// 在迭代器位置之前插入元素，返回指向新元素的迭代器（std::vector::insert 语义）
	iterator insert(iterator pos, const ElementType& value) {
		size_t idx = static_cast<size_t>(pos - begin());
		if (idx > Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %zu", Length, idx);
			return end();
		}
		if (Length >= Capacity) Resize();
		// 从后往前移动元素，为新元素腾出空间
		for (size_t i = Length; i > idx; --i) {
			new (ArrayMemory + i) ElementType(std::move(ArrayMemory[i - 1]));
			ArrayMemory[i - 1].~ElementType();
		}
		new (ArrayMemory + idx) ElementType(value);
		++Length;
		return begin() + idx;
	}

	iterator insert(const_iterator pos, const ElementType& value) {
		size_t idx = static_cast<size_t>(pos - ArrayMemory);
		if (idx > Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %zu", Length, idx);
			return end();
		}
		if (Length >= Capacity) Resize();
		for (size_t i = Length; i > idx; --i) {
			new (ArrayMemory + i) ElementType(std::move(ArrayMemory[i - 1]));
			ArrayMemory[i - 1].~ElementType();
		}
		new (ArrayMemory + idx) ElementType(value);
		++Length;
		return begin() + idx;
	}

	iterator insert(iterator pos, ElementType&& value) {
		size_t idx = static_cast<size_t>(pos - begin());
		if (idx > Length) {
			GLOG(Log::eError, "Index out of bounds! Length: %zu, Index: %zu", Length, idx);
			return end();
		}
		if (Length >= Capacity) Resize();
		// 从后往前移动元素，为新元素腾出空间
		for (size_t i = Length; i > idx; --i) {
			new (ArrayMemory + i) ElementType(std::move(ArrayMemory[i - 1]));
			ArrayMemory[i - 1].~ElementType();
		}
		new (ArrayMemory + idx) ElementType(std::move(value));
		++Length;
		return begin() + idx;
	}

	void pop_back() { if (Length > 0) RemoveAt(Length - 1); }
	void pop_front() { if (Length > 0) RemoveAt(0); }

	reference front() { return ArrayMemory[0]; }
	const_reference front() const { return ArrayMemory[0]; }
	reference back() { return ArrayMemory[Length > 0 ? Length - 1 : 0]; }
	const_reference back() const { return ArrayMemory[Length > 0 ? Length - 1 : 0]; }

	bool empty() const { return Length == 0; }
	size_t size() const { return Length; }
	size_t capacity() const { return Capacity; }
	pointer data() { return ArrayMemory; }
	const_pointer data() const { return ArrayMemory; }
	pointer cbegin() { return ArrayMemory; }
	pointer cend() { return ArrayMemory + Length; }

	void resize(size_t newSize) { Resize(newSize); }
	void resize(size_t newSize, const ElementType& value) {
		size_t oldSize = Length;
		Resize(newSize);
		for (size_t i = oldSize; i < Length; ++i) {
			ArrayMemory[i] = value;
		}
	}
	void reserve(size_t newCapacity) { Reserve(newCapacity); }
	void clear() { Clear(); }

	void swap(TArray& other) noexcept {
		std::swap(ArrayMemory, other.ArrayMemory);
		std::swap(Capacity, other.Capacity);
		std::swap(Stride, other.Stride);
		std::swap(Length, other.Length);
	}

private:
	ElementType* ArrayMemory;
	size_t Capacity;
	size_t Stride;
	size_t Length;
};