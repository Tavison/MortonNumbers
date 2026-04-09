#pragma once

#include "MortonCore.h"

#include <iostream>
#include <bitset>

namespace MortonDebug
{
	static const int DEBUG_PRINT = 1;

	/**
	 * @brief Writes the bit pattern of a value to standard output.
	 *
	 * @tparam T Value type to display.
	 * @param val Value whose bits will be printed.
	 */
	template <typename T>
	void print_bits(const T& val)
	{
		std::bitset<sizeof(T) * 8> bs(val);
		std::cout << "Bitset Size: " << sizeof(std::bitset<sizeof(T) * 8>) << std::endl;
		std::cout << "Size: " << sizeof(T) << " Bits: " << bs << std::endl;
	}

	/**
	 * @brief Debug version of @ref zip_by_3_64 that prints intermediate values.
	 *
	 * This function performs the same staged spreading process as
	 * @ref zip_by_3_64, but also prints the masks and the intermediate results
	 * after each transformation step.
	 *
	 * @param a Coordinate value whose lowest 21 bits will be expanded.
	 * @return Expanded 64-bit Morton lane containing the bits of @p a.
	 */
	inline std::uint64_t zip_by_3_64_debug(std::uint32_t a) noexcept
	{
		using namespace MortonCore::Detail;
		if constexpr (DEBUG_PRINT)
		{
			print_bits(a);
		}

		std::cout << "____________________________________________" << std::endl;
		std::cout << stage_1 << std::endl;
		print_bits(stage_1);
		print_bits(stage_2);
		print_bits(stage_3);
		print_bits(stage_4);
		print_bits(stage_5);
		print_bits(stage_6);
		std::cout << "____________________________________________" << std::endl;

		std::uint64_t x = a & stage_1;
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


}