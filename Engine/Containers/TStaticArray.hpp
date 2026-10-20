#pragma once

#include "Defines.hpp"
#include <cstddef>

/**
 * @brief 定长数组容器，替代 std::array。
 *
 * 与 std::array 同语义：编译期定长、栈上存储、无动态分配。
 * 使用示例：
 *   TStaticArray<Keymap::Entry, (size_t)eKeys::Max> Entries;
 *   Entries[i]; Entries.size(); Entries.data();
 */
template<typename T, size_t N>
class TStaticArray {
public:
	using value_type = T;
	using size_type = size_t;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = T*;
	using const_iterator = const T*;

	TStaticArray() = default;

	// std::array 风格聚合初始化
	TStaticArray(std::initializer_list<T> list) {
		size_t n = list.size() < N ? list.size() : N;
		size_t i = 0;
		for (const T& v : list) {
			if (i >= n) break;
			Data_[i++] = v;
		}
	}

	reference       operator[](size_t i)       { return Data_[i]; }
	const_reference operator[](size_t i) const { return Data_[i]; }

	reference       At(size_t i)       { return Data_[i]; }
	const_reference At(size_t i) const { return Data_[i]; }

	reference       Front()       { return Data_[0]; }
	const_reference Front() const { return Data_[0]; }
	reference       Back()        { return Data_[N - 1]; }
	const_reference Back()  const { return Data_[N - 1]; }

	pointer       Data()       { return Data_; }
	const_pointer Data() const { return Data_; }

	iterator       begin()       { return Data_; }
	const_iterator begin() const { return Data_; }
	iterator       end()         { return Data_ + N; }
	const_iterator end()   const { return Data_ + N; }

	static constexpr size_t Size() noexcept { return N; }
	static constexpr bool   IsEmpty() noexcept { return N == 0; }
	static constexpr bool   Empty() noexcept { return N == 0; }

	void Fill(const T& value) {
		for (size_t i = 0; i < N; ++i)
			Data_[i] = value;
	}

	// std 兼容别名
	size_t       size() const noexcept { return N; }
	bool         empty() const noexcept { return N == 0; }
	T*           data() noexcept { return Data_; }
	const T*     data() const noexcept { return Data_; }
	void         fill(const T& value) { Fill(value); }

private:
	T Data_[N];
};
