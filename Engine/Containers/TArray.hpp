#pragma once

#include "Defines.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

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
		ArrayMemory = Platform::PlatformAllocate(ArrayMemSize, false);
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
		ArrayMemory = Platform::PlatformAllocate(ArrayMemSize, false);
		Platform::PlatformSetMemory(ArrayMemory, 0, ArrayMemSize);
	}

	TArray(size_t size) {
		size_t ArrayMemSize = size * sizeof(ElementType);
		ArrayMemory = Platform::PlatformAllocate(ArrayMemSize, false);
		Platform::PlatformSetMemory(ArrayMemory, 0, ArrayMemSize);

		Capacity = size;
		Stride = sizeof(ElementType);
		Length = size;
	}

	virtual ~TArray() {
		//Platform::PlatformFree(ArrayMemory, false);
		//ArrayMemory = nullptr;
		//Capacity = 0;
		//Stride = 0;
		//Length = 0;
	}

public:
	size_t GetField(size_t field) {}
	void SetField(size_t field, size_t val){}

	void Resize(size_t size = 0) {
		size_t NewCapacity = size > 0 ? size : Capacity * ARRAY_DEFAULT_RESIZE_FACTOR;
		void* TempMemory = Platform::PlatformAllocate(NewCapacity * Stride, false);

		Platform::PlatformCopyMemory(TempMemory, ArrayMemory, Length * Stride);
		Platform::PlatformFree(ArrayMemory, false);

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
		Platform::PlatformCopyMemory((void*)addr, &value, Stride);

		Length++;
	}

	void InsertAt(size_t index, ElementType val) {
		if (index > Length - 1) {
			LOG_ERROR("Index Out of length! Length: %i, Index: %i", Length, index);
		}

		if (Length >= Capacity) {
			Resize();
		}

		char* addr = (char*)ArrayMemory + (Length * Stride);
		if (index != Length - 1) {
			Platform::PlatformCopyMemory(
				(void*)(addr + (index + 1) * Stride),
				(void*)(addr + (index * Stride)),
				Stride * (Length - index)
			);
		}

		Platform::PlatformCopyMemory((void*)(addr + (index * Stride)), val, Stride);
		Length++;
	}

	ElementType Pop() {
		char* addr = (char*)ArrayMemory + (Length * Stride);
		addr += ((Length - 1) * Stride);

		ElementType result;
		Platform::PlatformCopyMemory(&result, (void*)addr, Stride);
		Length--;

		return result;
	}

	ElementType PopAt(size_t index) {
		if (index > Length - 1) {
			LOG_ERROR("Index Out of length! Length: %i, Index: %i", Length, index);
			return ElementType();
		}

		char* addr = (char*)ArrayMemory + (index * Stride);
		ElementType result;
		Platform::PlatformCopyMemory(&result, (void*)addr, Stride);

		if (index != Length - 1) {
			Platform::PlatformCopyMemory(
				(void*)(addr + (index * Stride)),
				(void*)(addr + (index + 1) * Stride),
				Stride * (Length - index)
			);
		}

		Length--;
		return result;
	}

	void Clear() {
		if (ArrayMemory != nullptr) {
			size_t MemorySize = Capacity * Stride;
			Platform::PlatformFree(ArrayMemory, false);

			ArrayMemory = nullptr;
			Length = 0;
		}
	}


	const ElementType& Pop() const { return Pop(); }
	const ElementType& PopAt(size_t index) const { return PopAt(index); }
	
	bool IsEmpty() const { return (ArrayMemory == nullptr) && (Length == 0); }
	size_t Size() const { return Length; }


	ElementType* Data() { return (ElementType*)ArrayMemory; }
	const ElementType* Data() const { return (ElementType*)ArrayMemory; }

	template<typename IntegerType>
	ElementType& operator[](const IntegerType& i) {
		return *((ElementType*)((char*)ArrayMemory + i * Stride));
	}


private:
	void* ArrayMemory;

	size_t Capacity;		
	size_t Stride;
	size_t Length;
};

