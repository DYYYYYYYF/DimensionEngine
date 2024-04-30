#include <iostream>
#include "Containers/THashTable.hpp"

struct TestHash {
	TestHash() { a = 1, b = 2, c = false; }
	int a;
	int b;
	bool c;
};

int main() {

	HashTable Hashtable;

#define NUM_MAX 100

	uint32_t ElementCount = NUM_MAX;
	size_t TestHashSize = sizeof(uint32_t);

	uint32_t Test[NUM_MAX] = { 0 };

	TestHash Ref;
	Ref.a = 92;
	Ref.b = 36;
	Ref.c = true;

	//uint32_t x = 17;
	//for (uint32_t i = 0; i < NUM_MAX; ++i) {
	//	memcpy((char*)(&Test) + sizeof(uint32_t) * i, &x, sizeof(uint32_t));
	//}

	//printf("%i", *((char*)(&Test) + sizeof(uint32_t) * 99));

	uint32_t t = 19;

	Hashtable.Create(TestHashSize, ElementCount, &Test, false);
	Hashtable.Fill(&t);
	//Hashtable.Set("test1", &Ref);

	TestHash GetTestHash;

	uint32_t n;
	Hashtable.Get("test1", &n);

	std::cout << "Size : " << sizeof(Test) << "Size : " << TestHashSize * ElementCount << std::endl;

	std::cout << "Num1 : " << GetTestHash.a << "  Num2: " << GetTestHash.b << std::endl;
	std::cout << "n : " << n << std::endl;

	if (GetTestHash.c) {
		std::cout << "c : \n";
	}
	else {
		std::cout << "not c : \n";
	}

	char a;
	std::cin >> a;

	return 0;
}