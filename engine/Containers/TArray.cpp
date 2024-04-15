#include "TArray.hpp"
#include "../core/DMemory.hpp"

template <class T>
TArray<T>::TArray() {
	Capacity = ARRAY_DEFAULT_CAPACITY;

	size_t ArrayMemSize = Capacity * sizeof(T);
	ArrayMemory = ArrayMemory::Allocate(ArrayMemSize, MemoryType::eMemory_Type_Array);
	ArrayMemory::Set(ArrayMemory, 0, ArrayMemSize);

	Capacity = length;
	Stride = sizeof(T);
	Length = len;
}

template<class T>
TArray<T>::~TArray() {
	if (ArrayMemory) {
		size_t MemorySize = Capacity * Stride;
		ArrayMemory::Free(ArrayMemory, MemorySize, MemoryType::eMemory_Type_Array);
		ArrayMemory = nullptr;
		Length = 0;
	}
}

template<class T>
size_t TArray<T>::GetField(size_t field){

}

template<class T>
void TArray<T>::SetField(size_t field, size_t val) {

}

template<class T>
void TArray<T>::Resize() {
	Capacity *= ARRAY_DEFAULT_RESIZE_FACTOR;
	void* TempMemory = ArrayMemory::Allocate(Capacity, MemoryType::eMemory_Type_Array);

	ArrayMemory::Copy(TempMemory, ArrayMemory, Length * Stride);
	ArrayMemory::Free(ArrayMemory, Capacity * Stride, MemoryType::eMemory_Type_Array);

	ArrayMemory = TempMemory;
}

template<class T>
void TArray<T>::Push(const T& value) {
	if (Length > Capacity) {
		Resize();
	}

	size_t addr = (size_t)ArrayMemory;
	addr += (Length * Stride);
	Memory::Copy((void*)addr, val, Stride);

	Length++;
}

template<class T>
T TArray<T>::Pop() {
	if (index > Length - 1) {
		UL_ERROR("Index Out of length! Length: %i, Index: %i", Length, index);
		return nullptr;
	}

	size_t addr = (size_t)ArrayMemory;
	addr += ((Length - 1) * Stride);

	T result;
	Memory::Copy(&result, (void*)addr, Stride);
	Length--;

	return result;
}

template<class T>
T TArray<T>::PopAt(size_t index) {
	if (index > Length - 1) {
		UL_ERROR("Index Out of length! Length: %i, Index: %i", Length, index);
		return nullptr;
	}

	size_t addr = (size_t)ArrayMemory;
	T result;
	Memory::Copy(&result, (void*)addr, Stride);

	if (index != Length - 1) {
		Memory::Copy(
			(void*)(addr + (index * Stride)),
			(void*)(addr + (index + 1) * Stride),
			Stride * (Length - index);
		)
	}

	Length--;
	return result;
}


template<class T>
const T& TArray<T>::Pop() const {
	return Pop();
}

template<class T>
const T& TArray<T>::PopAt(size_t index) const {
	return PopAt(index);
}


template<class T>
void TArray<T>::InsertAt(size_t index, T val) {
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
			Stride * (Length - index);
		)
	}

	Memory::Copy((void*)(addr + (index * Stride)), val, Stride);
	Length++;
}

template<class T>
void TArray<T>::Clear() {
	if (ArrayMemory != nullptr) {
		size_t MemorySize = Capacity * Stride;
		ArrayMemory::Free(ArrayMemory, MemorySize, MemoryType::eMemory_Type_Array);

		ArrayMemory = nullptr;
		Length = 0;
	}
}

template<class T>
bool TArray<T>::IsEmpty() const {
	return (ArrayMemory == nullptr) && (Length == 0);
}

template<class T>
T* TArray<T>::Data() {
	return (T*)ArrayMemory;
}

template<class T>
const T* TArray<T>::Data() const {
	return (T*)ArrayMemory;
}

template<class T>
T  TArray<T>::operator[](size_t i) {
	return ArrayMemory[i];
}

template<class T>
const T& TArray<T>::operator[](size_t i) const {
	return ArrayMemory[i];
}