#include "HashTable/TestHashtable.cpp"
#include "Freelist/TestFreelist.cpp"
#include "String/TestString.cpp"
#include "Audio/TestAudio.cpp"
#include "Array/UnitTestArray.cpp"
#include "MathLibrary/TestMatrix.cpp"
#include "SIMD/TestSIMD.cpp"

#include<functional>

void CHECK_FUNC_CONTINUE(std::function<void()> callback, const std::string& err_msg = "") {
	try { callback(); }
	catch (const std::exception&) { std::cout << err_msg << std::endl; }
}

int main() {

	Memory::Initialize(GIBIBYTES(1));
	
	CHECK_FUNC_CONTINUE(&TestArray, "TestArray Failed.");
	CHECK_FUNC_CONTINUE(&TestHashTable, "TestHashTable Failed.");
	CHECK_FUNC_CONTINUE(&TestString, "TestString Failed.");
	CHECK_FUNC_CONTINUE(&UnitTestAudio, "UnitTestAudio Failed.");
	CHECK_FUNC_CONTINUE(&TestSIMD, "TestSIMD Failed.");
	CHECK_FUNC_CONTINUE(&TestMathLibrary, "TestMathLibrary Failed.");
	// 放在最后，有延时测试
	CHECK_FUNC_CONTINUE(&TestFreelist, "TestFreelist Failed.");

	return 0;
}
