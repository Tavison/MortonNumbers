#pragma once

#include "MortonMath.h"

#include <cstdint>
#include <bitset>
#include <iostream>
#include <tuple>

static const int DEBUG_PRINT = 1;

namespace Morton
{
	namespace
	{
		static const uint_fast64_t stage_1(0b0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0001'1111'1111'1111'1111'1111);
		static const uint_fast64_t stage_2(0b0000'0000'0001'1111'0000'0000'0000'0000'0000'0000'0000'0000'1111'1111'1111'1111);
		static const uint_fast64_t stage_3(0b0000'0000'0001'1111'0000'0000'0000'0000'1111'1111'0000'0000'0000'0000'1111'1111);
		static const uint_fast64_t stage_4(0b0001'0000'0000'1111'0000'0000'1111'0000'0000'1111'0000'0000'1111'0000'0000'1111);
		static const uint_fast64_t stage_5(0b0001'0000'1100'0011'0000'1100'0011'0000'1100'0011'0000'1100'0011'0000'1100'0011);
		static const uint_fast64_t stage_6(0b0001'0010'0100'1001'0010'0100'1001'0010'0100'1001'0010'0100'1001'0010'0100'1001);
	}

	template <typename T>
	void print_bits(const T& val)
	{
		std::bitset<sizeof(T) * 8> bs(val);
		std::cout << "Bitset Size: " << sizeof(std::bitset<sizeof(T) * 8>) << std::endl;
		std::cout << "Size: " << sizeof(T) << " Bits: " << bs << std::endl;
	}

	// method to seperate bits from a given integer 3 positions apart
	auto zip_by_3_64(uint_fast32_t a)
	{
		uint_fast64_t x = a & stage_1; // we only look at the first 21 bits
		x = (x | (x << 32)) & stage_2;
		x = (x | (x << 16)) & stage_3;
		x = (x | (x << 8)) & stage_4;
		x = (x | (x << 4)) & stage_5;
		x = (x | (x << 2)) & stage_6;

		return x;
	}

	// method to seperate bits from a given integer 3 positions apart
	auto zip_by_3_64_debug(uint_fast32_t a)
	{
		if constexpr(DEBUG_PRINT) { print_bits(a); }

		std::cout << "____________________________________________" << std::endl;
		std::cout << stage_1 << std::endl;
		print_bits(stage_1);
		print_bits(stage_2);
		print_bits(stage_3);
		print_bits(stage_4);
		print_bits(stage_5);
		print_bits(stage_6);
		std::cout << "____________________________________________" << std::endl;
		
		uint_fast64_t x = a & stage_1;
		print_bits(x);
		x = (x | (x << 32)) & stage_2;
		print_bits(x);
		x = (x | (x << 16)) & stage_3;
		print_bits(x);
		x = (x | (x << 8)) & stage_4;
		print_bits(x);
		x = (x | (x << 4)) & stage_5;
		print_bits(x);
		x = (x | (x << 2)) & stage_6;
		print_bits(x);

		return x;
	}

	auto morton_encode_magic_bits(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
	{
		uint_fast64_t answer = 0;
		answer |= zip_by_3_64(x) | zip_by_3_64(y) << 1 | zip_by_3_64(z) << 2;
		return answer;
	}

	auto unzip_by_3_64(uint_fast64_t morton)
	{
		uint_fast64_t x = morton & stage_6;
		x = (x ^ (x >> 2)) & stage_5;
		x = (x ^ (x >> 4)) & stage_4;
		x = (x ^ (x >> 8)) & stage_3;
		x = (x ^ (x >> 16)) & stage_2;
		x = (x ^ ((uint_fast64_t)x >> 32)) & stage_1;
		return static_cast<uint_fast32_t>(x);
	}

	auto morton_decode_magic_bits(uint_fast32_t& x, uint_fast32_t& y, uint_fast32_t& z, uint_fast64_t morton)
	{
		x = unzip_by_3_64(morton);
		y = unzip_by_3_64(morton >> 1);
		z = unzip_by_3_64(morton >> 2);
	}

	class MortonNumber
	{
		uint_fast64_t m_value;

	public:
		explicit MortonNumber(unsigned int x, unsigned int y, unsigned int z) : m_value(morton_encode_magic_bits(x, y, z)) { ; }

		uint_fast64_t value() const { return m_value; }

		MortonNumber& operator += (const MortonNumber& rhs)
		{
			m_value = add(m_value, rhs.m_value);
			return *this;
		}

		MortonNumber& operator -= (const MortonNumber& rhs)
		{
			m_value = subtract(m_value, rhs.m_value);
			return *this;
		}

		MortonNumber& operator *= (const MortonNumber& rhs)
		{
			m_value = multiply(m_value, rhs.m_value);
			return *this;
		}

		MortonNumber& operator /= (const MortonNumber& rhs)
		{
			m_value = divide(m_value, rhs.m_value);
			return *this;
		}
	};

	MortonNumber operator+(MortonNumber lhs, const MortonNumber& rhs)
	{
		return lhs += rhs;
	}

	MortonNumber operator-(MortonNumber lhs, const MortonNumber& rhs)
	{
		return lhs -= rhs;
	}

	MortonNumber operator*(MortonNumber lhs, const MortonNumber& rhs)
	{
		return lhs *= rhs;
	}

	MortonNumber operator/(MortonNumber lhs, const MortonNumber& rhs)
	{
		return lhs /= rhs;
	}








	// Magicbits masks (3D decode)
	static uint_fast32_t magicbit3D_masks32_decode[6] = { 0, 0x000003ff, 0x30000ff, 0x0300f00f, 0x30c30c3, 0x9249249 }; // we add a 0 on position 0 in this array to use same code for 32-bit and 64-bit cases
	static uint_fast64_t magicbit3D_masks64_decode[6] = { 0x1fffff, 0x1f00000000ffff, 0x1f0000ff0000ff, 0x100f00f00f00f00f, 0x10c30c30c30c30c3, 0x1249249249249249 };

	template<typename morton, typename coord>
	static inline coord morton3D_GetThirdBits(const morton m) 
	{
		morton* masks = (sizeof(morton) <= 4) ? reinterpret_cast<morton*>(magicbit3D_masks32_decode) : reinterpret_cast<morton*>(magicbit3D_masks64_decode);
		morton x = m & masks[5];
		x = (x ^ (x >> 2)) & masks[4];
		x = (x ^ (x >> 4)) & masks[3];
		x = (x ^ (x >> 8)) & masks[2];
		x = (x ^ (x >> 16)) & masks[1];
		if (sizeof(morton) > 4) { x = (x ^ ((uint_fast64_t)x >> 32)) & masks[0]; }
		return static_cast<coord>(x);
	}

	// DECODE 3D Morton code : Magic bits
	// This method splits the morton codes bits by using certain patterns (magic bits)
	template<typename morton, typename coord>
	inline void m3D_d_magicbits(const morton m, coord& x, coord& y, coord& z) 
	{
		x = morton3D_GetThirdBits<morton, coord>(m);
		y = morton3D_GetThirdBits<morton, coord>(m >> 1);
		z = morton3D_GetThirdBits<morton, coord>(m >> 2);
	}


}	// namespace Morton
