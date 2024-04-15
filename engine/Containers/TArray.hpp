#pragma once

#include "../Defines.hpp"

#define ARRAY_DEFAULT_CAPACITY 1
#define ARRAY_DEFAULT_RESIZE_FACTOR 2

/*
Memory layout
size_t(unsigned long long) capacity = number elements that an be held
size_t(unsigned long long) length = number of elements currently contained
size_t(unsigned long long) stride = size of each element in bytes
void* elements
*/
template<class T>
class TArray {
public:
	TArray();
	virtual ~TArray();

public:
	size_t GetField(size_t field);
	void SetField(size_t field, size_t val);

	void Resize();
	void Push(const T& value);
	void InsertAt(size_t index, T val);

	T Pop();
	T PopAt(size_t index);
	const T& Pop() const;
	const T& PopAt(size_t index) const;

	void Clear();
	bool IsEmpty() const;

	size_t Size() const { return Length; }

	T* Data();
	const T* Data() const;

	T operator[](size_t i);
	const T& operator[](size_t i) const;

private:
	void* ArrayMemory;

	size_t Capacity;		
	size_t Stride;
	size_t Length;
};
