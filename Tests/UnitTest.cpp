#include "HashTable/TestHashtable.cpp"
#include "Freelist/TestFreelist.cpp"
#include "String/TestString.cpp"

int main() {

	Memory::Initialize(GIBIBYTES(1));
	
	TestHashTable();
	TestFreelist();
	TestString();

	return 0;
}