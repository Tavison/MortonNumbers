#pragma once

#include <cstdint>

namespace Morton
{
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
	auto add(std::uint64_t op1, std::uint64_t op2)
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

		return sum;
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
				res = add(res, a);
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
			quotient = add(quotient, incrementer);

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
