#pragma once

#include "MortonCore.h"
#include "MortonMath.h"
#include <bitset>
#include <iostream>

namespace Morton
{

	/**
	 * @brief Stores a 3D Morton code and provides arithmetic and traversal
	 * operations on the stored value.
	 *
	 * All methods delegate to the free functions in MortonMath.h. Callers who
	 * need to eliminate every cycle of overhead can call those free functions
	 * directly with the raw @ref value().
	 */
	class MortonNumber
	{
		std::uint64_t m_value;

		/**
		 * @brief Constructs a MortonNumber directly from a raw Morton code.
		 *
		 * Used internally to wrap results from MortonMath free functions without
		 * an unnecessary encode call.
		 *
		 * @param raw Already-encoded Morton value.
		 */
		explicit MortonNumber(std::uint64_t raw) : m_value(raw) {}

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
		[[nodiscard]] std::uint64_t value() const noexcept { return m_value; }

		// -----------------------------------------------------------------------
		// Arithmetic
		// -----------------------------------------------------------------------

		/**
		 * @brief Adds another Morton number to this object.
		 *
		 * @param rhs Right-hand operand.
		 * @return Reference to this object after modification.
		 */
		MortonNumber& operator+=(const MortonNumber& rhs) noexcept
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
		MortonNumber& operator-=(const MortonNumber& rhs) noexcept
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
		MortonNumber& operator*=(const MortonNumber& rhs) noexcept
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
		MortonNumber& operator/=(const MortonNumber& rhs) noexcept
		{
			m_value = divide(m_value, rhs.m_value);
			return *this;
		}

		// -----------------------------------------------------------------------
		// Modulo
		// -----------------------------------------------------------------------

		/**
		 * @brief Computes per-coordinate modulo and returns the result as a new
		 * Morton number.
		 *
		 * Automatically dispatches to the power-of-two fast path when all three
		 * periods are powers of two; otherwise falls back to decode/mod/encode.
		 * Callers who know their periods are powers of two at the call site should
		 * use @ref modulo_pow2 directly to skip the runtime test.
		 *
		 * @param px Period along X (must be > 0).
		 * @param py Period along Y (must be > 0).
		 * @param pz Period along Z (must be > 0).
		 * @return Morton number of (x % px, y % py, z % pz).
		 */
		[[nodiscard]] MortonNumber modulo(
			std::uint32_t px, std::uint32_t py, std::uint32_t pz) const noexcept
		{
			return MortonNumber(Morton::modulo(m_value, px, py, pz));
		}

		/**
		 * @brief Computes per-coordinate modulo against power-of-two periods
		 * given as bit counts.
		 *
		 * Each period is 2^kN. Operates entirely in Morton space with no
		 * decode/encode round-trip.
		 *
		 * @param kx Bit count for X (period = 2^kx).
		 * @param ky Bit count for Y (period = 2^ky).
		 * @param kz Bit count for Z (period = 2^kz).
		 * @return Morton number of (x % 2^kx, y % 2^ky, z % 2^kz).
		 */
		[[nodiscard]] MortonNumber modulo_pow2(
			std::uint32_t kx, std::uint32_t ky, std::uint32_t kz) const noexcept
		{
			return MortonNumber(Morton::modulo_pow2(m_value, kx, ky, kz));
		}

		/**
		 * @brief Computes per-coordinate modulo against power-of-two periods
		 * given as a Morton-coded period mask.
		 *
		 * @p period_mask must be morton_encode(px-1, py-1, pz-1) where each
		 * period is a power of two. This is the fastest modulo path: a single
		 * AND with no branches and no decode/encode.
		 *
		 * @param period_mask morton_encode(px-1, py-1, pz-1).
		 * @return Morton number of (x % px, y % py, z % pz).
		 */
		[[nodiscard]] MortonNumber modulo_pow2(std::uint64_t period_mask) const noexcept
		{
			return MortonNumber(Morton::modulo_pow2(m_value, period_mask));
		}

		// -----------------------------------------------------------------------
		// Traversal
		// -----------------------------------------------------------------------

		/**
		 * @brief Descends one level into the specified child octant.
		 *
		 * The three bits of @p octant select which half of the current node is
		 * entered along each axis:
		 *   bit 0 – X half (0 = lower, 1 = upper)
		 *   bit 1 – Y half (0 = lower, 1 = upper)
		 *   bit 2 – Z half (0 = lower, 1 = upper)
		 *
		 * @param octant Child octant index in the range [0, 7].
		 * @return Morton number of the path extended by one level.
		 */
		[[nodiscard]] MortonNumber descend(std::uint64_t octant) const noexcept
		{
			return MortonNumber(Morton::descend(m_value, octant));
		}

		/**
		 * @brief Ascends one level toward the root.
		 *
		 * Strips the current octant index from the path and returns the parent
		 * node. Use @ref octant_of before ascending if the current octant index
		 * is needed.
		 *
		 * @return Morton number of the parent node.
		 */
		[[nodiscard]] MortonNumber ascend() const noexcept
		{
			return MortonNumber(Morton::ascend(m_value));
		}

		/**
		 * @brief Returns the octant index of this node within its parent.
		 *
		 * Extracts the three low bits of the stored path, which identify which
		 * child octant of the parent this node occupies.
		 *
		 * @return Octant index in the range [0, 7].
		 */
		[[nodiscard]] std::uint64_t octant_of() const noexcept
		{
			return Morton::octant_of(m_value);
		}
	};

	// ---------------------------------------------------------------------------
	// Binary arithmetic operators
	// ---------------------------------------------------------------------------

	/**
	 * @brief Returns the sum of two Morton numbers.
	 *
	 * @param lhs Left-hand operand.
	 * @param rhs Right-hand operand.
	 * @return Sum of the two operands.
	 */
	[[nodiscard]] inline MortonNumber operator+(MortonNumber lhs, const MortonNumber& rhs) noexcept
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
	[[nodiscard]] inline MortonNumber operator-(MortonNumber lhs, const MortonNumber& rhs) noexcept
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
	[[nodiscard]] inline MortonNumber operator*(MortonNumber lhs, const MortonNumber& rhs) noexcept
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
	[[nodiscard]] inline MortonNumber operator/(MortonNumber lhs, const MortonNumber& rhs) noexcept
	{
		return lhs /= rhs;
	}

} // namespace Morton
