#pragma once

#include <chrono>
#include <random>

using namespace std;

class Random
{
public:
	
	static void AutoSeed()
	{
		auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
		generator.seed(static_cast<uint32_t>(seed));
	}
	
	static int Integer(int minInclusive, int maxInclusive)
	{
		return uniform_int_distribution<int>{minInclusive, maxInclusive}(generator);
	}
	
	static int Unsigned(unsigned minInclusive, unsigned maxInclusive)
	{
		return uniform_int_distribution<unsigned>{minInclusive, maxInclusive}(generator);
	}
	
	template<typename T>
	static T Real(T minInclusive, T maxExclusive)
	{
		return uniform_real_distribution<T>{minInclusive, maxExclusive}(generator);
	}
	
	static uint64_t Int64()
	{
		return (((uint64_t)generator()) << 32) | ((uint64_t)generator());
	}

private:
	static mt19937 generator;
	
	Random() = delete;
	
};
