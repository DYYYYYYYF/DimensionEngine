#pragma once

#include "Defines.hpp"
#include "core/EngineLogger.hpp"
#include "core/DMemory.hpp"

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
class TArray {
public:
	TArray(MemoryType memory_type = MemoryType::eMemory_Type_Array) {
		size_t ArrayMemSize = ARRAY_DEFAULT_CAPACITY * sizeof(ElementType);
		ArrayMemory = Memory::Allocate(ArrayMemSize, memory_type);
		Memory::Set(ArrayMemory, 0, ArrayMemSize);

		UsedMemoryType = memory_type;
		Capacity = ARRAY_DEFAULT_CAPACITY;
		Stride = sizeof(ElementType);
		Length = 0;
	}

	TArray(size_t size, MemoryType memory_type = MemoryType::eMemory_Type_Array) {
		size_t ArrayMemSize = size * sizeof(ElementType);
		ArrayMemory = Memory::Allocate(ArrayMemSize, memory_type);
		Memory::Set(ArrayMemory, 0, ArrayMemSize);

		UsedMemoryType = memory_type;
		Capacity = size;
		Stride = sizeof(ElementType);
		Length = size;
	}

	virtual ~TArray() {}

public:
	size_t GetField(size_t field) {}
	void SetField(size_t field, size_t val){}

	void Resize(size_t size = 0) {
		size_t NewCapacity = size > 0 ? size : Capacity * ARRAY_DEFAULT_RESIZE_FACTOR;
		void* TempMemory = Memory::Allocate(NewCapacity * Stride, UsedMemoryType);

		Memory::Copy(TempMemory, ArrayMemory, Length * Stride);
		Memory::Free(ArrayMemory, Capacity * Stride, UsedMemoryType);

		Capacity = NewCapacity;
		ArrayMemory = TempMemory;
	}

	void Push(const ElementType& value) {
		if (Length >= Capacity) {
			Resize();
		}

		size_t addr = (size_t)ArrayMemory + (Length * Stride);
		Memory::Copy((void*)addr, &value, Stride);

		Length++;
	}

	void InsertAt(size_t index, ElementType val) {
		if (index > Length - 1) {
			UL_ERROR("Index Out of length! Length: %i, Index: %i", Length, index);
			return nullptr;
		}

		if (Length >= Capacity) {
			Resize();
		}

		size_t addr = (size_t)ArrayMemory;
		if (index != Length - 1) {
			Memory::Copy(
				(void*)(addr + (index + 1) * Stride),
				(void*)(addr + (index * Stride)),
				Stride * (Length - index)
			);
		}

		Memory::Copy((void*)(addr + (index * Stride)), val, Stride);
		Length++;
	}

	ElementType Pop() {
		size_t addr = (size_t)ArrayMemory;
		addr += ((Length - 1) * Stride);

		ElementType result;
		Memory::Copy(&result, (void*)addr, Stride);
		Length--;

		return result;
	}

	ElementType PopAt(size_t index) {
		if (index > Length - 1) {
			UL_ERROR("Index Out of length! Length: %i, Index: %i", Length, index);
			return ElementType();
		}

		size_t addr = (size_t)ArrayMemory;
		ElementType result;
		Memory::Copy(&result, (void*)addr, Stride);

		if (index != Length - 1) {
			Memory::Copy(
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
			Memory::Free(ArrayMemory, MemorySize, UsedMemoryType);

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

	ElementType& operator[](size_t i) { 
		//if (i > Length || ArrayMemory == nullptr) return ElementType();
		return *((ElementType*)((size_t)ArrayMemory + i * Stride));
	}


private:
	void* ArrayMemory;

	size_t Capacity;		
	size_t Stride;
	size_t Length;

	MemoryType UsedMemoryType;
};

