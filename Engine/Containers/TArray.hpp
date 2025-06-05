#pragma once

#include "Defines.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"
#include <type_traits>
#include <utility>

#define ARRAY_DEFAULT_CAPACITY 1
#define ARRAY_DEFAULT_RESIZE_FACTOR 2

/*
Memory layout
size_t(unsigned long long) capacity = number elements that can be held
size_t(unsigned long long) length = number of elements currently contained
size_t(unsigned long long) stride = size of each element in bytes
void* elements
*/
template<typename ElementType>
class DAPI TArray {
public:
	TArray() : ArrayMemory(nullptr), Capacity(0), Stride(sizeof(ElementType)), Length(0) {
		size_t ArrayMemSize = ARRAY_DEFAULT_CAPACITY * sizeof(ElementType);
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		if (!ArrayMemory) {
			GLOG(Log::eError, "Failed to allocate memory for TArray");
			return;
		}
		Platform::PlatformSetMemory(ArrayMemory, 0, ArrayMemSize);
		Capacity = ARRAY_DEFAULT_CAPACITY;
	}

	// 拷贝构造函数 - 修正了内存分配失败处理
	TArray(const TArray& arr) : ArrayMemory(nullptr), Capacity(0), Stride(sizeof(ElementType)), Length(0) {
		if (arr.ArrayMemory == nullptr || arr.Length == 0) {
			*this = TArray(); // 调用默认构造函数
			return;
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
				Memory::Free(ArrayMemory, ArrayMemSize, MemoryType::eMemory_Type_Array);
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
			*this = TArray(); // 调用默认构造函数
			return;
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
					Memory::Free(ArrayMemory, ArrayMemSize, MemoryType::eMemory_Type_Array);
					ArrayMemory = nullptr;
					Capacity = 0;
					Length = 0;
					throw;
				}
			}
		}
	}

	virtual ~TArray() {
		Destroy();
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
						if constexpr (!std::is_pointer<ElementType>::value) {
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
					if constexpr (std::is_move_constructible<ElementType>::value && !std::is_pointer<ElementType>::value) {
						new(TempMemory + i) ElementType(std::move(ArrayMemory[i]));
					}
					else {
						new(TempMemory + i) ElementType(ArrayMemory[i]);
					}
				}
				catch (...) {
					// 异常安全：清理已构造的元素
					for (size_t j = 0; j < i; ++j) {
						TempMemory[j].~ElementType();
					}
					Memory::Free(TempMemory, NewCapacity * Stride, MemoryType::eMemory_Type_Array);
					throw;
				}
			}

			// 销毁原数组中的元素
			if constexpr (!std::is_pointer<ElementType>::value) {
				for (size_t i = 0; i < Length; ++i) {
					ArrayMemory[i].~ElementType();
				}
			}
			Memory::Free(ArrayMemory, Capacity * Stride, MemoryType::eMemory_Type_Array);
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

	// 修正了InsertAt函数的逻辑错误
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

		if constexpr (!std::is_pointer<ElementType>::value) {
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

		if constexpr (!std::is_pointer<ElementType>::value) {
			ArrayMemory[Length - 1].~ElementType();
		}

		Length--;
		return result;
	}

	void Clear() {
		if (ArrayMemory != nullptr) {
			if constexpr (!std::is_pointer<ElementType>::value) {
				for (size_t i = 0; i < Length; ++i) {
					ArrayMemory[i].~ElementType();
				}
			}
			Length = 0;
		}
	}

	void Destroy() {
		if (ArrayMemory != nullptr) {
			if constexpr (!std::is_pointer<ElementType>::value) {
				for (size_t i = 0; i < Length; ++i) {
					ArrayMemory[i].~ElementType();
				}
			}
			Memory::Free(ArrayMemory, Capacity * Stride, MemoryType::eMemory_Type_Array);
			ArrayMemory = nullptr;
		}

		Capacity = 0;
		Stride = 0;
		Length = 0;
	}

	bool IsEmpty() const { return Length == 0; }
	size_t Size() const { return Length; }
	size_t GetCapacity() const { return Capacity; }

	ElementType* Data() { return ArrayMemory; }
	const ElementType* Data() const { return ArrayMemory; }

	// 修正了赋值操作符 - 添加了自赋值检查和异常安全
	TArray<ElementType>& operator=(const TArray<ElementType>& other) {
		if (this == &other) {
			return *this; // 自赋值检查
		}

		// 先销毁当前数据
		Destroy();

		if (other.ArrayMemory == nullptr || other.Length == 0) {
			*this = TArray(); // 重新初始化为空数组
			return *this;
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
				Memory::Free(ArrayMemory, ArrayMemSize, MemoryType::eMemory_Type_Array);
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
		other.Stride = 0;
		other.Length = 0;
	}

	TArray& operator=(TArray&& other) noexcept {
		if (this != &other) {
			Destroy();

			ArrayMemory = other.ArrayMemory;
			Capacity = other.Capacity;
			Stride = other.Stride;
			Length = other.Length;

			other.ArrayMemory = nullptr;
			other.Capacity = 0;
			other.Stride = 0;
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

private:
	ElementType* ArrayMemory;
	size_t Capacity;
	size_t Stride;
	size_t Length;
};