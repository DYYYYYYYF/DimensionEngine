#pragma once

#include "Defines.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"
#include <type_traits>

#define ARRAY_DEFAULT_CAPACITY 1
#define ARRAY_DEFAULT_RESIZE_FACTOR 2

/*
Memory layout
size_t(unsigned long long) capacity = number elements that an be held
size_t(unsigned long long) length = number of elements currently contained
size_t(unsigned long long) stride = size of each element in bytes
void* elements
*/
template<typename ElementType>
class DAPI TArray {
public:
	TArray() {
		size_t ArrayMemSize = ARRAY_DEFAULT_CAPACITY * sizeof(ElementType);
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		Platform::PlatformSetMemory(ArrayMemory, 0, ArrayMemSize);

		Capacity = ARRAY_DEFAULT_CAPACITY;
		Stride = sizeof(ElementType);
		Length = 0;
	}

	TArray(const TArray& arr) {
		Capacity = arr.Capacity;
		Stride = arr.Stride;
		Length = arr.Length;

		size_t ArrayMemSize = Capacity * Stride;
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		if (ArrayMemory) {
			for (size_t i = 0; i < Length; ++i) {
				new(reinterpret_cast<ElementType*>(ArrayMemory) + i) ElementType(arr[i]); // 使用拷贝构造函数
			}
		}
	}

	TArray(size_t size) {
		size_t ArrayMemSize = size * sizeof(ElementType);
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		Capacity = size;
		Stride = sizeof(ElementType);
		Length = size;

		// If pointer then new object
		if constexpr (std::is_pointer<ElementType>::value) {
			Platform::PlatformSetMemory(ArrayMemory, 0, ArrayMemSize);
		}
		else {
			for (size_t i = 0; i < size; ++i) {
				new(reinterpret_cast<char*>(ArrayMemory) + i * sizeof(ElementType)) ElementType();
			}
		}
	}

	virtual ~TArray() {
		Destroy();
	}

	ElementType* begin() {
		return reinterpret_cast<ElementType*>(ArrayMemory);
	}

	const ElementType* end() {
		return reinterpret_cast<const ElementType*>(ArrayMemory) + Length;
	}


public:
	size_t GetField(size_t field) {}
	void SetField(size_t field, size_t val){}

	void Resize(size_t size = 0) {
		size_t NewCapacity = size > 0 ? size : Capacity * ARRAY_DEFAULT_RESIZE_FACTOR;
		ElementType* TempMemory = (ElementType*)Memory::Allocate(NewCapacity * Stride, MemoryType::eMemory_Type_Array);

		if (ArrayMemory) {
			for (size_t i = 0; i < Length; ++i) {
				new(reinterpret_cast<ElementType*>(TempMemory) + i) ElementType(ArrayMemory[i]); 
			}
		}
		Memory::Free(ArrayMemory, Capacity, MemoryType::eMemory_Type_Array);

		if (size > 0) {
			Length = size;
		}
		Capacity = NewCapacity;
		ArrayMemory = TempMemory;
	}

	void Push(const ElementType& value) {
		if (Length >= Capacity) {
			Resize();
		}

		char* addr = (char*)ArrayMemory + (Length * Stride);
		new(reinterpret_cast<char*>(addr)) ElementType(value);

		Length++;
	}

	void InsertAt(size_t index, ElementType val) {
		if (index > Length) {
			LOG_ERROR("Index Out of length! Length: %i, Index: %i", Length, index);
		}

		if (Length >= Capacity) {
			Resize();
		}

		if (index != Length - 1) {
			for (size_t i = index; i < Length - 1; ++i) {
				if constexpr (std::is_pointer<ElementType>::value) {
					ArrayMemory[i] = ArrayMemory[i + 1];
				}
				else {
					reinterpret_cast<ElementType*>(ArrayMemory)[i + 1].~ElementType();
					new(reinterpret_cast<char*>(ArrayMemory) + (i + 1) * sizeof(ElementType)) ElementType(ArrayMemory[i]);
				}
			}
		}

		// Handle the element at index.
		if constexpr (std::is_pointer<ElementType>::value) {
			ArrayMemory[index] = val;
		}
		else {
			reinterpret_cast<ElementType*>(ArrayMemory)[index].~ElementType();
			new(reinterpret_cast<char*>(ArrayMemory) + (index) * sizeof(ElementType)) ElementType(val);
		}

		Length++;
	}

	ElementType Pop() {
		if (Length < 1) {
			LOG_ERROR("Tring to pop a 0 length array.");
			return ElementType();
		}

		ElementType result = ArrayMemory[Length - 1];

		char* addr = (char*)ArrayMemory + (Length * Stride);
		addr += ((Length - 1) * Stride);
		if constexpr (std::is_pointer<ElementType>::value) {
			ArrayMemory[Length - 1] = nullptr;
		}
		else{
			reinterpret_cast<ElementType*>(ArrayMemory)[Length - 1].~ElementType();
		}

		Length--;
		return result;
	}

	ElementType PopAt(size_t index) {
		if (index > Length - 1 ) {
			LOG_ERROR("Index Out of length! Length: %i, Index: %i", Length, index);
			return ElementType();
		}

		ElementType result = ArrayMemory[index];
		if (index != Length - 1) {
			for (size_t i = index; i < Length - 1; ++i) {
				if constexpr (std::is_pointer<ElementType>::value) {
					ArrayMemory[i] = ArrayMemory[i + 1];
				}
				else {
					reinterpret_cast<ElementType*>(ArrayMemory)[i].~ElementType();
					new(reinterpret_cast<char*>(ArrayMemory) + i * sizeof(ElementType)) ElementType(ArrayMemory[i + 1]);
				}
			}
		}

		Length--;
		return result;
	}

	void Clear() {
		if (ArrayMemory != nullptr) {
			if constexpr (std::is_pointer<ElementType>::value) {
				size_t MemorySize = Capacity * Stride;
				Memory::Zero(ArrayMemory, MemorySize);
			}
			else {
				for (size_t i = 0; i < Length; ++i) {
					reinterpret_cast<ElementType*>(ArrayMemory)[i].~ElementType();
				}
			}

			Length = 0;
		}
	}

	void Destroy() {
		if (ArrayMemory != nullptr) {
			if constexpr (std::is_pointer<ElementType>::value) {
				Memory::Free(ArrayMemory, Capacity * Stride, MemoryType::eMemory_Type_Array);
			}
			else {
				for (size_t i = 0; i < Length; ++i) {
					reinterpret_cast<ElementType*>(ArrayMemory)[i].~ElementType();
				}
				Memory::Free(ArrayMemory, Capacity * Stride, MemoryType::eMemory_Type_Array);
			}
			ArrayMemory = nullptr;
		}

		Capacity = 0;
		Stride = 0;
		Length = 0;
	}

	const ElementType& Pop() const { return Pop(); }
	const ElementType& PopAt(size_t index) const { return PopAt(index); }
	
	bool IsEmpty() const { return (ArrayMemory == nullptr) && (Length == 0); }
	size_t Size() const { return Length; }


	ElementType* Data() { return (ElementType*)ArrayMemory; }
	const ElementType* Data() const { return (ElementType*)ArrayMemory; }

	TArray<ElementType>& operator=(const TArray<ElementType>& other) {
		Capacity = other.Capacity;
		Stride = other.Stride;
		Length = other.Length;

		size_t ArrayMemSize = Capacity * Stride;
		ArrayMemory = (ElementType*)Memory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
		if (ArrayMemory) {
			for (size_t i = 0; i < Length; ++i) {
				new(reinterpret_cast<ElementType*>(ArrayMemory) + i) ElementType(other[i]); // 使用拷贝构造函数
			}
		}

		return *this;
	}

	template<typename IntegerType>
	ElementType& operator[](const IntegerType& i) {
		return ArrayMemory[i];
	}

	template<typename IntegerType>
	const ElementType& operator[](const IntegerType& i) const {
		return ArrayMemory[i];
	}

private:
	ElementType* ArrayMemory;

	size_t Capacity;		
	size_t Stride;
	size_t Length;
};

