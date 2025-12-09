#include <iostream>
#include <Containers/TArray.hpp>
#include <Core/DMemory.hpp>

using namespace std;

class CA {
public:
	CA() {}
	CA(const std::string& s) { Str = s; }
	virtual ~CA() {}

public:
	std::string Str;
};

TArray<CA> T() {
	TArray<CA> Arr1;
	CA A = CA("AAAAAA");
	CA B = CA("BBBBBB");
	CA C = CA("CCCCCC");

	Arr1.Push(A);
	Arr1.Push(B);
	Arr1.Push(C);

	A.Str = "QQQQQQ";

	CA rm = Arr1.PopAt(2);
	cout << "Remove: " << rm.Str << endl;

	CA E = CA("EEEEEE");
	Arr1.InsertAt(2, E);

	Arr1.Pop();

	return Arr1;
}

TArray<CA*> TPointer() {
	TArray<CA*> Arr1;
	CA* A = NewObject<CA>("AAAAAA");
	CA* B = NewObject<CA>("BBBBBB");
	CA* C = NewObject<CA>("CCCCCC");
	CA* D = NewObject<CA>("DDDDDD");

	Arr1.Push(A);
	Arr1.Push(B);
	Arr1.Push(C);
	Arr1.Push(D);

	A->Str = "QQQQQQ";

	CA* rm = Arr1.PopAt(2);
	cout << "Remove: " << rm->Str << endl;

	CA* E = NewObject<CA>("EEEEEE");
	Arr1.InsertAt(3, E);

	Arr1.Pop();

	return Arr1;
}
static int aa = 0;

class TCopy {
public:
	TCopy() {}
	TCopy(const TCopy& c) { 
		ss = c.ss; 
	}

	TCopy(TCopy&& c) {
		ss = c.ss;
	}

	virtual ~TCopy() {
		Shutdown();
	}

private:
	void Shutdown() {
		std::cout << "Shutdown" << aa << "\n";
	}

public:
	std::string ss;
};

void TestArray(){
	TArray<TArray<CA>> Arr1;
	Arr1.Push(T());
	for (auto& a : Arr1) {
		for (auto b : a) {
			cout << b.Str << endl;
		}
	}

	TArray<TArray<CA*>> Arr2;
	Arr2.Push(TPointer());
	for (auto& a : Arr2) {
		for (auto b : a) {
			cout << b->Str << endl;
		}
	}
}