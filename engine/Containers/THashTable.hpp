#pragma once

#include "Defines.hpp"

/*
* @brief Represents a simple hash table. Member of this structure
* should not be modified outside the functions associated with it.
* 
* For non-pointer types, table retains a copy of the value. For pointer 
* types, make ownership of pointers or associated memory allocations,
* and should be managed externally.
*/
struct DAPI HashTable {
public:
	size_t ElementSize;
	unsigned int ElementCount;
	bool IsPointerType;
	void* Memory;

#if defined(LEVEL_DEBUG)
	size_t Count;
#endif

public:
	HashTable() {}

	/*
	* @breif Creates a hashtable and stores it.
	* 
	* @param element_size The size of each element in bytes.
	* @param element_count The maximum number of elements. Cannot be resized.
	* @param memory A block of memory to be used. Should be equal in size to element_size * element_count.
	* @param is_pointer Indicates if this hashtable will hold pointer types.
	*/
	void Create(size_t element_size, unsigned int element_count, void* memory, bool is_pointer);

	/*
	* @brief Destroys the hashtable. Does not release memory  for pointer types.
	*/
	void Destroy();

	/*
	* @breif Stores a copy of the data in value.
	*
	* @param name The name of the entry to set. Required.
	* @param value The value of be set. Required.
	* @return True or false if a null pointer is passed.
	*/
	bool Set(const char* name, void* value);

	/*
	* @breif Stores a pointer.
	* Only use for tables which were created with is_pointer = true;
	* 
	* @param name The name of the entry to set. Required.
	* @param value The value of be set. Can pass nullptr to 'unset' an entry.
	* @return True or false if a null pointer is passed or if then entry is null.
	*/
	bool Set(const char* name, void** value);

	/*
	* @brief Obtains a pointer to data present.
	*
	* @param name The name of the retrieved to set. Required.
	* @param value A pointer to store the retrieved value. Required.
	* @return True or false if a null pointer is passed.
	*/
	bool Get(const char* name, void* out_value);

	/*
	* @brief Obtains a pointer to data present.
	*
	* @param name The name of the retrieved to set. Required.
	* @param value A pointer to store the retrieved value. Required.
	* @return True or false if a null pointer is passed or if then entry is null.
	*/
	bool Get(const char* name, void** out_value);

	/*
	* @brief Fill all entries with the given value.
	* Useful when non-existent names should return some default value.
	* Should not be used with pointer table.
	* 
	* @param value The default value.
	* @return True or false if successful.
	*/
	bool Fill(void* value);

private:
	/*
	* @brief Generate hash code by name and element_count.
	*
	* @param name The entry name.
	* @param element_count The element count.
	* @return Hash code.
	*/
	size_t HashName(const char* name, unsigned int element_count) {
		// A multipler to use when generating a hash. Prime to hopefully avoid collisions.
		static const size_t Multipler = 97;

		unsigned const char* us;
		size_t Hash = 0;
		for (us = (unsigned const char*)name; *us; us++) {
			Hash = Hash * Multipler + *us;
		}

		// Mod it against the size of the table
		Hash %= element_count;

		return Hash;
	}

};