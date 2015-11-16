#pragma once

#include <algorithm>
#include <functional>
#include <set>
#include <vector>

/// EXTENDED STANDARD TEMPLATE LIBRARY ///

#define ALL(x) x.begin(), x.end()

namespace stdx
{
	
	template <typename value_type, typename func_type>
	inline void erase_if(std::vector<value_type>& vec, func_type pred) {
		vec.erase(remove_if(vec.begin(), vec.end(), pred), vec.end());
	}
	
	template <typename value_type>
	inline bool contains(std::set<value_type>& s, value_type search) {
		return (s.find(search) != s.end());
	}
	
	template <typename value_type>
	inline bool contains(std::vector<value_type>& s, value_type search) {
		for (auto it = s.begin(); it != s.end(); ++it) if (*it == search) return true;
		return false;
	}
	
	template <typename value_type>
	class max_heap {
	public:
		max_heap(){};
		max_heap(size_t n):data_(n){};
		max_heap(size_t n, value_type v):data_(n,v){};
		void assign(size_t n, value_type v) {
			data_.resize(0);
			data_.resize(n,v);
		}
		void push(value_type v) {
			data_.push_back(v);
			push_heap(data_.begin(), data_.end());
		}
		bool empty() { return data_.empty(); }
		value_type top() { return data_[0]; }
		void pop() {
			pop_heap(data_.begin(), data_.end());
			data_.resize(data_.size() - 1);
		}
		value_type next() {
			pop_heap(data_.begin(), data_.end());
			value_type tmp = data_.back();
			data_.resize(data_.size() - 1);
			return tmp;
		}
	private:
		std::vector<value_type> data_;
	};
	
}