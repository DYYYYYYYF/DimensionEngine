#include "Containers/TString.hpp"

using namespace std;

String Add(const String& s1, const String& s2) {
	String s = s1 + s2;
	return s;
}

bool TestString() {
	String str;
	str = "AAA";
	if (!str.Equal("AAA")) {
		return false;
	}
	cout << str << endl;

	String str1("BBB");
	if (!str1.Equal("BBB")) {
		return false;
	}
	cout << str1 << endl;


	str = str1;
	if (!str.Equal("BBB")) {
		return false;
	}
	cout << str << endl;;

	String str2 = "CCC";
	str += str2;
	if (!str.Equal("BBBCCC")) {
		return false;
	}
	cout << str << endl;

	String str3 = Add(str1, str2);
	cout << str3 << endl;

	str = str3.SubStr(2, 4);
	cout << str3 << " Sub(2, 4): " << str << endl;

	String str4 = "AAA,BBB,CCC,DDD";
	auto index = str4.IndexOf(',');
	cout << "The first ',' in " << str4 << " is at: " << index << endl;

	auto arr = str4.Split(',');
	for (auto s : arr) {
		cout << s << endl;
	}
	cout << endl;

	return true;
}