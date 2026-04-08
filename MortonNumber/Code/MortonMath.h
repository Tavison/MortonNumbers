#pragma once

#include <cstdint>

namespace Morton
{
	auto add(uint_fast64_t op1, uint_fast64_t op2)
	{
		uint_fast64_t sum, carry;
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

	auto subtract(uint_fast64_t op1, uint_fast64_t op2)
	{
		auto check = op1;
		uint_fast64_t dif, borrow;
		while (op2 != 0)
		{
			dif = op1 ^ op2;
			borrow = (~op1)&op2;
			borrow <<= 3;
			op1 = dif;
			op2 = borrow;
		};
		return (dif < check) ? dif : 0;
	}

	auto multiply(uint_fast64_t a, uint_fast64_t b)
	{
		uint_fast64_t res = 0;
		while (b > 0)
		{
			if (b & 7)
			{
				res = add(res, a); // if b is odd, add a to result
			}
			a <<= 3;
			b >>= 3;
		}
		return res;
	}

	auto divide(uint_fast64_t dividend, uint_fast64_t divisor)
	{
		static const uint_fast64_t mask = 0b1001001001001001001001001001001001001001001001001001001001001001;
		// Initialize the quotient
		uint_fast64_t quotient{ 0 };
		while (dividend >= divisor)
		{
			auto mask_dividend1 = (mask)& dividend;
			auto mask_divisor1 = (mask)& divisor;
			auto b1 = mask_divisor1 ? mask_dividend1 >= mask_divisor1 : 0;
			auto t1 = static_cast<uint_fast64_t>(b1);

			auto mask_dividend2 = (mask << 1)& dividend;
			auto mask_divisor2 = (mask << 1)& divisor;
			auto b2 = mask_divisor2 ? mask_dividend2 >= mask_divisor2 : 0;
			auto t2 = static_cast<uint_fast64_t>(b2);

			auto mask_dividend3 = (mask << 2)& dividend;
			auto mask_divisor3 = (mask << 2)& divisor;
			auto b3 = mask_divisor3 ? mask_dividend3 >= mask_divisor3 : 0;
			auto t3 = static_cast<uint_fast64_t>(b3);

			auto incrementer = t1 + (t2 << 1) + (t3 << 2);
			quotient = add(quotient, incrementer);

			dividend = subtract(dividend, divisor);
		}

		return quotient;
	}

	// write versions that carry backwords, i.e. to the right.
	// These are just copies from above as a starting point.
	auto radd(uint_fast64_t op1, uint_fast64_t op2)
	{
		uint_fast64_t sum, carry;
		do
		{
			sum = op1 ^ op2;
			carry = op1 & op2;
			sum <<= 3;	// Where to put it?
			op1 = sum;
			op2 = carry;
		} while (carry);

		return sum;
	}

	auto rsub(uint_fast64_t op1, uint_fast64_t op2)
	{
		uint_fast64_t dif, borrow;
		while (op2 != 0)
		{
			dif = op1 ^ op2;
			borrow = (~op1)&op2;
			borrow <<= 3;
			op1 = dif;
			op2 = borrow;
		};
		return dif;
	}
}	// namespace Morton

