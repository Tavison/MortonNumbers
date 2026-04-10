#pragma once

#pragma once

#include "MortonCore.h"

#include <cstdint>
#include <cassert>

namespace Morton
{
	/**
		* @brief Mask selecting all 63 bit positions used by a 64-bit 3D Morton code.
		*
		* A 64-bit 3D Morton value uses bit positions 0 through 62. Bit 63 is not
		* part of any Morton lane and must not participate in the masked-lane
		* addition trick.
		*/
	inline constexpr std::uint64_t occupied_mask_3d =
		MortonCore::Detail::lane_mask_3d
		| (MortonCore::Detail::lane_mask_3d << 1)
		| (MortonCore::Detail::lane_mask_3d << 2);

	/**
	* @brief Adds two Morton-coded values using carry propagation spaced three
	* bits apart.
	*
	* The carry is generated from bit positions that overlap in both operands and
	* is then moved left by three positions so that it remains aligned with the
	* same coordinate lane in the Morton representation.
	*
	* @param op1 Left-hand operand.
	* @param op2 Right-hand operand.
	* @return Sum of the two Morton-coded operands.
	*/
	auto add_iterative(std::uint64_t op1, std::uint64_t op2)
	{
		std::uint64_t sum, carry;
		do
		{
			sum = op1 ^ op2;
			carry = op1 & op2;
			carry <<= 3;
			op1 = sum;
			op2 = carry;
		} while (carry);
		return sum & occupied_mask_3d;
	}

	/**
		* @brief Adds two values from a single Morton lane using a masked integer add.
		*
		* The selected Morton lane contains one bit every third bit position. To let
		* a normal integer addition propagate carry from one lane bit to the next,
		* this function fills only the spacer bits inside the occupied 63-bit Morton
		* domain with 1s.
		*
		* The fill mask must not extend outside the Morton domain. In particular,
		* bit 63 must remain clear. If that top unused bit is set, the integer add
		* can wrap modulo 2^64 and corrupt the low bits of the result.
		*
		* @param op1 Left-hand operand.
		* @param op2 Right-hand operand.
		* @param lane_mask Mask selecting one Morton lane.
		* @return Sum of the selected lane from @p op1 and @p op2.
		*/
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		add_lane(std::uint64_t op1, std::uint64_t op2, std::uint64_t lane_mask) noexcept
	{
		const std::uint64_t fill_mask = occupied_mask_3d ^ lane_mask;

		return (((op1 & lane_mask) | fill_mask) + (op2 & lane_mask)) & lane_mask;
	}

	/**
		* @brief Adds two 64-bit 3D Morton-coded values by adding each lane
		* independently and recombining the results.
		*
		* The x lane uses bit positions 0, 3, 6, ...
		* The y lane uses bit positions 1, 4, 7, ...
		* The z lane uses bit positions 2, 5, 8, ...
		*
		* @param op1 Left-hand operand.
		* @param op2 Right-hand operand.
		* @return Sum of the two Morton-coded operands.
		*/
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		add_masked(std::uint64_t op1, std::uint64_t op2) noexcept
	{
		using namespace MortonCore::Detail;

		constexpr std::uint64_t x_mask = lane_mask_3d;
		constexpr std::uint64_t y_mask = lane_mask_3d << 1;
		constexpr std::uint64_t z_mask = lane_mask_3d << 2;

		return add_lane(op1, op2, x_mask)
			| add_lane(op1, op2, y_mask)
			| add_lane(op1, op2, z_mask);
	}

	/**
	* @brief Subtracts one Morton-coded value from another using borrow
	* propagation spaced three bits apart.
	*
	* The borrow is generated from positions where the subtrahend contains a set
	* bit and the minuend does not. The borrow is then moved left by three
	* positions so that it remains aligned with the same coordinate lane in the
	* Morton representation.
	*
	* @param op1 Minuend.
	* @param op2 Subtrahend.
	* @return Difference of the operands, or 0 when the final result does not
	* compare as less than the original minuend.
	*/
	auto subtract(std::uint64_t op1, std::uint64_t op2)
	{
		auto check = op1;
		std::uint64_t dif, borrow;
		while (op2 != 0)
		{
			dif = op1 ^ op2;
			borrow = (~op1) & op2;
			borrow <<= 3;
			op1 = dif;
			op2 = borrow;
		};
		return (dif < check) ? dif : 0;
	}

	/**
	* @brief Multiplies two Morton-coded values by repeated shifted addition.
	*
	* The multiplier is examined three bits at a time. When the current three-bit
	* group is non-zero, the current multiplicand is added into the accumulated
	* result. The multiplicand is then shifted left by three positions and the
	* multiplier is shifted right by three positions for the next step.
	*
	* @param a Multiplicand.
	* @param b Multiplier.
	* @return Product of the two Morton-coded operands.
	*/
	auto multiply(std::uint64_t a, std::uint64_t b)
	{
		std::uint64_t res = 0;
		while (b > 0)
		{
			if (b & 7)
			{
				res = add_iterative(res, a);
			}
			a <<= 3;
			b >>= 3;
		}
		return res;
	}

	/**
	* @brief Divides one Morton-coded value by another and returns the quotient.
	*
	* The function repeatedly subtracts the divisor from the dividend until the
	* dividend becomes smaller than the divisor. During each iteration it derives
	* a three-bit increment value by comparing the corresponding Morton lanes of
	* the current dividend and divisor, then adds that increment into the running
	* quotient.
	*
	* @param dividend Value to be divided.
	* @param divisor Value by which to divide.
	* @return Quotient accumulated during the repeated subtraction process.
	*/
	auto divide(std::uint64_t dividend, std::uint64_t divisor)
	{
		static const std::uint64_t mask = 0b1001001001001001001001001001001001001001001001001001001001001001;
		std::uint64_t quotient{ 0 };
		while (dividend >= divisor)
		{
			auto mask_dividend1 = (mask) & dividend;
			auto mask_divisor1 = (mask) & divisor;
			auto b1 = mask_divisor1 ? mask_dividend1 >= mask_divisor1 : 0;
			auto t1 = static_cast<std::uint64_t>(b1);

			auto mask_dividend2 = (mask << 1) & dividend;
			auto mask_divisor2 = (mask << 1) & divisor;
			auto b2 = mask_divisor2 ? mask_dividend2 >= mask_divisor2 : 0;
			auto t2 = static_cast<std::uint64_t>(b2);

			auto mask_dividend3 = (mask << 2) & dividend;
			auto mask_divisor3 = (mask << 2) & divisor;
			auto b3 = mask_divisor3 ? mask_dividend3 >= mask_divisor3 : 0;
			auto t3 = static_cast<std::uint64_t>(b3);

			auto incrementer = t1 + (t2 << 1) + (t3 << 2);
			quotient = add_iterative(quotient, incrementer);

			dividend = subtract(dividend, divisor);
		}

		return quotient;
	}

	/**
	* @brief Adds two Morton-coded values while propagating toward lower-order
	* lanes.
	*
	* This function performs the same XOR-and-carry style accumulation as
	* @ref add, but updates the intermediate sum by shifting it left by three
	* positions before the next iteration.
	*
	* @param op1 Left-hand operand.
	* @param op2 Right-hand operand.
	* @return Sum produced by the reverse-direction accumulation process.
	*/
	auto radd(std::uint64_t op1, std::uint64_t op2)
	{
		std::uint64_t sum, carry;
		do
		{
			sum = op1 ^ op2;
			carry = op1 & op2;
			sum <<= 3;
			op1 = sum;
			op2 = carry;
		} while (carry);

		return sum;
	}

	/**
	* @brief Subtracts two Morton-coded values while propagating borrows through
	* the reverse-direction routine.
	*
	* @param op1 Minuend.
	* @param op2 Subtrahend.
	* @return Difference produced by the reverse-direction subtraction process.
	*/
	auto rsub(std::uint64_t op1, std::uint64_t op2)
	{
		std::uint64_t dif, borrow;
		while (op2 != 0)
		{
			dif = op1 ^ op2;
			borrow = (~op1) & op2;
			borrow <<= 3;
			op1 = dif;
			op2 = borrow;
		};
		return dif;
	}
} // namespace Morton
