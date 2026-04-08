#pragma once



#ifdef _MSC_VER
#include <intrin.h>	// For unsigned __int64 __popcnt64(unsigned __int64 value);
#endif

#include <cstdint>	// For specific int types
#include <chrono>	// For timers and clocks
#include <cassert>	// For assert
#include <iostream>	// For cout
#include <bitset>	// For bit set for counting bits

#include "../Code/MortonNumbers.h"	// Morton Numbers

uint64_t BitCount(uint64_t x)
{
#ifdef _MSC_VER
	return __popcnt64(x);
#elif __clang__
	return __builtin_popcount(x);
#elif __GNUC__
	return __builtin_popcount(x);
#else
	const uint64_t m1 = 0x5555555555555555; //binary: 0101...
	const uint64_t m2 = 0x3333333333333333; //binary: 00110011..
	const uint64_t m4 = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
	const uint64_t h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...

	x -= (x >> 1) & m1;				//put count of each 2 bits into those 2 bits
	x = (x & m2) + ((x >> 2) & m2);	//put count of each 4 bits into those 4 bits 
	x = (x + (x >> 4)) & m4;		//put count of each 8 bits into those 8 bits 
	return (x * h01) >> 56;			//returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
#endif
}

void bitCountTest()
{
	uint64_t ull[] = { 0, 0xF0F0F0F0F0F0F0, 0xFFFFFFF, 0xFFFFFFFFFFFFFFFF };
	volatile uint64_t u64;

	auto t1 = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 1000; ++i)
	{
		for (const auto& val : ull)
		{
			u64 = BitCount(val);
		}
	}

	auto t2 = std::chrono::high_resolution_clock::now();
	auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
	std::cout << "BitCount for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
}

void test_add()
{
	volatile uint_fast64_t k(0b1011'0110'1101'1011'0110'1101'1011'0110);
	// uint_fast64_t k = 0;
	volatile uint_fast64_t l = 0b111;
	auto t1 = std::chrono::high_resolution_clock::now();
	for (uint_fast64_t i = 0; i < 1000; ++i)
	{
		k = Morton::add(k, l);
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
	std::cout << "Add for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
}

void test_sub()
{
	volatile uint_fast64_t k = 0b111111111111111111111111111111;
	volatile uint_fast64_t l = 0b111;
	auto t1 = std::chrono::high_resolution_clock::now();
	for (uint_fast64_t i = 0; i < 1000; ++i)
	{
		k = Morton::subtract(k, l);
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
	std::cout << "Subtract for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
}

void test_mul()
{
	volatile uint_fast64_t k = 0b0000'0000'0000'0000'1000'1000'1000'1000;
	volatile uint_fast64_t l = 0b1001'0000;
	auto t1 = std::chrono::high_resolution_clock::now();
	for (uint_fast64_t i = 0; i < 1000; ++i)
	{
		volatile auto res_morton = Morton::multiply(k, l);
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
	std::cout << "Multiply for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
}

void test_div()
{
	volatile uint_fast64_t k = 0b1000'0000'0000'0000'1000'1000'1000'1000'0000'0000'0000'0000'0000'0000'0000'0000;
	volatile uint_fast64_t l = 0b0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0101'0000;

	auto t1 = std::chrono::high_resolution_clock::now();
	for (uint_fast64_t i = 0; i < 1000; ++i)
	{
		volatile auto res_morton = Morton::divide(k, l);
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
	std::cout << "Divide for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
}

void TestMortonEncodeMagicBits()
{
	auto t1 = std::chrono::high_resolution_clock::now();
	for (int x = 0; x < 1000; ++x)
	{
		for (int y = 0; y < 1000; ++y)
		{
			for (int z = 0; z < 1000; ++z)
			{
				volatile auto index = Morton::morton_encode_magic_bits(x, y, z);
			}
		}
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
	std::cout << "Encode for 1000 * 1000 * 1000 runs took " << d.count() << " nanoseconds." << std::endl;
}


auto TestEncode_Decode()
{
	auto t1 = std::chrono::high_resolution_clock::now();
	for (uint_fast32_t x = 0; x < 2048; ++x)
	{
		for (uint_fast32_t y = 0; y < 2048; ++y)
		{
			for (uint_fast32_t z = 0; z < 2048; ++z)
			{
				volatile auto mortonNumber = Morton::morton_encode_magic_bits(x, y, z);
				Morton::morton_decode_magic_bits(x, y, z, mortonNumber);
			}
		}
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
	std::cout << "Encode/Decode for 2048 * 2048 * 2048 runs took " << d.count() << " nanoseconds." << std::endl;
}






