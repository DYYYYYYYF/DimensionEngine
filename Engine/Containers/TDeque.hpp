#pragma once

#include "Defines.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"
#include <cstddef>
#include <utility>
#include <initializer_list>

/**
 * @brief 双端队列容器，替代 std::deque。
 *
 * 提供 std::deque 风格的接口：push_back / push_front / pop_back / pop_front、
 * 迭代器随机访问、insert / erase 等。
 * 内部以动态数组 + 头部偏移实现，队头/队尾操作均为 O(1) 摊还。
 */
template<typename ElementType>
class TDeque {
public:
	using value_type = ElementType;
	using size_type = size_t;
	using reference = ElementType&;
	using const_reference = const ElementType&;
	using iterator = ElementType*;
	using const_iterator = const ElementType*;

	TDeque()
		: Data_(nullptr), Capacity_(0), Length_(0), Head_(0) {
	}

	explicit TDeque(size_t size)
		: Data_(nullptr), Capacity_(0), Length_(0), Head_(0) {
		reserve(size);
		for (size_t i = 0; i < size; ++i)
			PushBack(ElementType());
	}

	TDeque(const TDeque& other)
		: Data_(nullptr), Capacity_(0), Length_(0), Head_(0) {
		for (size_t i = 0; i < other.Length_; ++i)
			PushBack(other.AtLinear(i));
	}

	TDeque(TDeque&& other) noexcept
		: Data_(other.Data_), Capacity_(other.Capacity_),
		Length_(other.Length_), Head_(other.Head_) {
		other.Data_ = nullptr;
		other.Capacity_ = 0;
		other.Length_ = 0;
		other.Head_ = 0;
	}

	~TDeque() {
		clear();
		Memory::Free(Data_, MemoryType::eMemory_Type_Array);
		Data_ = nullptr;
		Capacity_ = 0;
	}

	TDeque& operator=(const TDeque& other) {
		if (this == &other) return *this;
		clear();
		for (size_t i = 0; i < other.Length_; ++i)
			PushBack(other.AtLinear(i));
		return *this;
	}

	TDeque& operator=(TDeque&& other) noexcept {
		if (this != &other) {
			clear();
			Memory::Free(Data_, MemoryType::eMemory_Type_Array);
			Data_ = other.Data_;
			Capacity_ = other.Capacity_;
			Length_ = other.Length_;
			Head_ = other.Head_;
			other.Data_ = nullptr;
			other.Capacity_ = 0;
			other.Length_ = 0;
			other.Head_ = 0;
		}
		return *this;
	}

	TDeque(std::initializer_list<ElementType> list)
		: Data_(nullptr), Capacity_(0), Length_(0), Head_(0) {
		reserve(list.size());
		for (const ElementType& v : list)
			PushBack(v);
	}

	// ── 容量 ─────────────────────────────────────────────

	size_t size() const noexcept { return Length_; }
	bool   empty() const noexcept { return Length_ == 0; }
	size_t capacity() const noexcept { return Capacity_; }

	void clear() {
		for (size_t i = 0; i < Length_; ++i)
			AtLinear(i).~ElementType();
		Length_ = 0;
		Head_ = 0;
	}

	// 预留容量（不改变元素数量）
	void reserve(size_t newCapacity) {
		if (newCapacity <= Capacity_) return;
		Grow(newCapacity);
	}

	// ── 元素访问 ─────────────────────────────────────────

	reference       operator[](size_t i)       { return AtLinear(i); }
	const_reference operator[](size_t i) const { return AtLinear(i); }

	reference       front()       { return AtLinear(0); }
	const_reference front() const { return AtLinear(0); }
	reference       back()        { return AtLinear(Length_ - 1); }
	const_reference back()  const { return AtLinear(Length_ - 1); }

	// ── 迭代器 ───────────────────────────────────────────

	iterator       begin()       { return Data_ + Head_; }
	const_iterator begin() const { return Data_ + Head_; }
	iterator       end()         { return Data_ + Head_ + Length_; }
	const_iterator end()   const { return Data_ + Head_ + Length_; }

	// ── 修改 ─────────────────────────────────────────────
	// [FIX] 容量判定缺陷：push/pop 混合使用时，pop_front 会累积队头空位（Head_>0），
	// 此时队尾可用空间为 Capacity_-Head_-Length_，可能为 0。原实现只检查 Length_ >=
	// Capacity_，当 Head_+Length_ >= Capacity_（队尾已满）时仍按 Data_[Head_+Length_]
	// 写入，越界覆盖本分配块之后的内存（JobSystem 场景下覆盖自身 AllocHeader，
	// 表现为 FreeAligned Free failed + Header 被 std::function 指针覆盖）。
	// 修复：所有写入路径统一检查队尾空间 Head_+Length_，Grow 目标容量同时容纳
	// Head_+Length_+1，避免扩容后仍越界。

	void push_back(const ElementType& v) {
		if (Head_ + Length_ >= Capacity_ && !Grow(std::max<size_t>(Capacity_ ? Capacity_ * 2 : 4, Head_ + Length_ + 1))) return;
		new (Data_ + Head_ + Length_) ElementType(v);
		++Length_;
	}

	void push_back(ElementType&& v) {
		if (Head_ + Length_ >= Capacity_ && !Grow(std::max<size_t>(Capacity_ ? Capacity_ * 2 : 4, Head_ + Length_ + 1))) return;
		new (Data_ + Head_ + Length_) ElementType(std::move(v));
		++Length_;
	}

	template<typename... Args>
	reference emplace_back(Args&&... args) {
		if (Head_ + Length_ >= Capacity_ && !Grow(std::max<size_t>(Capacity_ ? Capacity_ * 2 : 4, Head_ + Length_ + 1))) return back();
		new (Data_ + Head_ + Length_) ElementType(std::forward<Args>(args)...);
		++Length_;
		return back();
	}

	void pop_back() {
		if (Length_ == 0) return;
		AtLinear(Length_ - 1).~ElementType();
		--Length_;
	}

	void push_front(const ElementType& v) {
		if (Head_ + Length_ >= Capacity_ && !Grow(std::max<size_t>(Capacity_ ? Capacity_ * 2 : 4, Head_ + Length_ + 1))) return;
		MakeHeadRoom();
		--Head_;
		new (Data_ + Head_) ElementType(v);
		++Length_;
	}

	void push_front(ElementType&& v) {
		if (Head_ + Length_ >= Capacity_ && !Grow(std::max<size_t>(Capacity_ ? Capacity_ * 2 : 4, Head_ + Length_ + 1))) return;
		MakeHeadRoom();
		--Head_;
		new (Data_ + Head_) ElementType(std::move(v));
		++Length_;
	}

	void pop_front() {
		if (Length_ == 0) return;
		AtLinear(0).~ElementType();
		++Head_;
		--Length_;
		if (Length_ == 0) Head_ = 0;
	}

	// 在迭代器位置之前插入，返回指向新元素的迭代器（与 std::deque 语义一致）
	iterator insert(iterator pos, const ElementType& v) {
		size_t idx = static_cast<size_t>(pos - begin());
		if (idx > Length_) idx = Length_;
		if (Head_ + Length_ >= Capacity_ && !Grow(std::max<size_t>(Capacity_ ? Capacity_ * 2 : 4, Head_ + Length_ + 1))) return end();
		pos = begin() + idx;
		if (idx == Length_) {
			new (Data_ + Head_ + Length_) ElementType(v);
			++Length_;
			return begin() + idx;
		}
		if (idx == 0) {
			push_front(v);
			return begin();
		}
		new (Data_ + Head_ + Length_) ElementType(std::move(back()));
		for (size_t i = Length_ - 1; i > idx; --i)
			AtLinear(i) = std::move(AtLinear(i - 1));
		AtLinear(idx) = ElementType(v);
		++Length_;
		return begin() + idx;
	}

	iterator insert(iterator pos, ElementType&& v) {
		size_t idx = static_cast<size_t>(pos - begin());
		if (idx > Length_) idx = Length_;
		if (Head_ + Length_ >= Capacity_ && !Grow(std::max<size_t>(Capacity_ ? Capacity_ * 2 : 4, Head_ + Length_ + 1))) return end();
		pos = begin() + idx;
		if (idx == Length_) {
			new (Data_ + Head_ + Length_) ElementType(std::move(v));
			++Length_;
			return begin() + idx;
		}
		if (idx == 0) {
			push_front(std::move(v));
			return begin();
		}
		new (Data_ + Head_ + Length_) ElementType(std::move(back()));
		for (size_t i = Length_ - 1; i > idx; --i)
			AtLinear(i) = std::move(AtLinear(i - 1));
		AtLinear(idx) = ElementType(std::move(v));
		++Length_;
		return begin() + idx;
	}

	// 删除迭代器位置的元素，返回下一元素的迭代器（std::deque 语义）
	iterator erase(iterator pos) {
		size_t idx = static_cast<size_t>(pos - begin());
		if (idx >= Length_) return end();
		if (idx == 0) {
			pop_front();
			return begin();
		}
		if (idx == Length_ - 1) {
			pop_back();
			return end();
		}
		AtLinear(idx).~ElementType();
		for (size_t i = idx; i + 1 < Length_; ++i)
			AtLinear(i) = std::move(AtLinear(i + 1));
		AtLinear(Length_ - 1).~ElementType();
		--Length_;
		return begin() + idx;
	}

private:
	ElementType* Data_;
	size_t       Capacity_;
	size_t       Length_;
	size_t       Head_;

	// 逻辑下标 -> 物理下标
	ElementType& AtLinear(size_t i) { return Data_[Head_ + i]; }
	const ElementType& AtLinear(size_t i) const { return Data_[Head_ + i]; }

	// 当队头没有空位时，把元素整体搬到数组尾部，腾出队头空间供 push_front 使用。
	// 目标区 [newHead, newHead+Length_) 位于源区 [Head_, Head_+Length_) 之后，
	// 必须从后往前搬，避免覆盖尚未读取的源元素；空桶则直接定位到桶尾之后。
	void MakeHeadRoom() {
		if (Length_ == 0) {
			Head_ = Capacity_;   // push_front 先 --Head_，将写入最后一个槽
			return;
		}
		size_t newHead = Capacity_ - Length_;
		if (newHead == Head_) return;
		if (newHead < Head_) {
			// 目标区在源区左侧（newHead < Head_）：从前往后搬，避免覆盖尚未读取的源元素
			for (size_t i = 0; i < Length_; ++i)
				Data_[newHead + i] = std::move(Data_[Head_ + i]);
		} else {
			// 目标区在源区右侧：从后往前搬
			for (size_t i = Length_; i > 0; --i)
				Data_[newHead + i - 1] = std::move(Data_[Head_ + i - 1]);
		}
		Head_ = newHead;
	}

	// 扩容。返回 true 表示成功或无需扩容；false 表示分配失败，调用方必须放弃本次写入。
	bool Grow(size_t newCapacity) {
		if (newCapacity <= Capacity_) return true;
		ElementType* newData = (ElementType*)Memory::Allocate(
			newCapacity * sizeof(ElementType), MemoryType::eMemory_Type_Array);
		if (!newData) {
			GLOG(Log::eFatal, "TDeque::Grow: allocation failed (cap=%zu -> %zu), write aborted.", Capacity_, newCapacity);
			return false;
		}
		for (size_t i = 0; i < Length_; ++i)
			new (newData + i) ElementType(std::move(AtLinear(i)));
		for (size_t i = 0; i < Length_; ++i)
			AtLinear(i).~ElementType();
		Memory::Free(Data_, MemoryType::eMemory_Type_Array);
		Data_ = newData;
		Capacity_ = newCapacity;
		Head_ = 0;
		return true;
	}
};
