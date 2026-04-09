#pragma once

#include "MortonCore.h"
#include "MortonMath.h"
#include <bitset>
#include <iostream>

namespace Morton
{

	/**
	 * @brief Stores a 3D Morton code and provides arithmetic operators on the
	 * stored value.
	 */
	class MortonNumber
	{
		std::uint64_t m_value;

	public:
		/**
		 * @brief Constructs a Morton number from x, y, and z coordinates.
		 *
		 * @param x X coordinate.
		 * @param y Y coordinate.
		 * @param z Z coordinate.
		 */
		explicit MortonNumber(unsigned int x, unsigned int y, unsigned int z)
			: m_value(MortonCore::morton_encode(x, y, z))
		{
		}

		/**
		 * @brief Returns the stored Morton value.
		 *
		 * @return Encoded Morton value.
		 */
		std::uint64_t value() const { return m_value; }

		/**
		 * @brief Adds another Morton number to this object.
		 *
		 * @param rhs Right-hand operand.
		 * @return Reference to this object after modification.
		 */
		MortonNumber& operator+=(const MortonNumber& rhs)
		{
			m_value = add(m_value, rhs.m_value);
			return *this;
		}

		/**
		 * @brief Subtracts another Morton number from this object.
		 *
		 * @param rhs Right-hand operand.
		 * @return Reference to this object after modification.
		 */
		MortonNumber& operator-=(const MortonNumber& rhs)
		{
			m_value = subtract(m_value, rhs.m_value);
			return *this;
		}

		/**
		 * @brief Multiplies this object by another Morton number.
		 *
		 * @param rhs Right-hand operand.
		 * @return Reference to this object after modification.
		 */
		MortonNumber& operator*=(const MortonNumber& rhs)
		{
			m_value = multiply(m_value, rhs.m_value);
			return *this;
		}

		/**
		 * @brief Divides this object by another Morton number.
		 *
		 * @param rhs Right-hand operand.
		 * @return Reference to this object after modification.
		 */
		MortonNumber& operator/=(const MortonNumber& rhs)
		{
			m_value = divide(m_value, rhs.m_value);
			return *this;
		}
	};

	/**
	 * @brief Returns the sum of two Morton numbers.
	 *
	 * @param lhs Left-hand operand.
	 * @param rhs Right-hand operand.
	 * @return Sum of the two operands.
	 */
	MortonNumber operator+(MortonNumber lhs, const MortonNumber& rhs)
	{
		return lhs += rhs;
	}

	/**
	 * @brief Returns the difference of two Morton numbers.
	 *
	 * @param lhs Left-hand operand.
	 * @param rhs Right-hand operand.
	 * @return Difference of the two operands.
	 */
	MortonNumber operator-(MortonNumber lhs, const MortonNumber& rhs)
	{
		return lhs -= rhs;
	}

	/**
	 * @brief Returns the product of two Morton numbers.
	 *
	 * @param lhs Left-hand operand.
	 * @param rhs Right-hand operand.
	 * @return Product of the two operands.
	 */
	MortonNumber operator*(MortonNumber lhs, const MortonNumber& rhs)
	{
		return lhs *= rhs;
	}

	/**
	 * @brief Returns the quotient of two Morton numbers.
	 *
	 * @param lhs Left-hand operand.
	 * @param rhs Right-hand operand.
	 * @return Quotient of the two operands.
	 */
	MortonNumber operator/(MortonNumber lhs, const MortonNumber& rhs)
	{
		return lhs /= rhs;
	}
} // namespace Morton

