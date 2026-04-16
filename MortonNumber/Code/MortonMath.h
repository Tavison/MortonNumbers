#pragma once

#include "MortonCore.h"

#include <bit>
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
		add(std::uint64_t op1, std::uint64_t op2) noexcept
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
	 * @brief Subtracts two values from a single Morton lane using a masked integer subtract.
	 *
	 * Borrow propagation through spacer bits requires those bits to be 0 in the
	 * minuend so that a borrow can ripple freely from one lane bit to the next.
	 * Masking with @p lane_mask naturally satisfies this — no fill is needed,
	 * making this simpler than the corresponding add_lane.
	 *
	 * The result is masked back to @p lane_mask before returning, so any borrow
	 * that escapes the Morton domain (e.g. past bit 62 on the z lane) is discarded.
	 *
	 * @param op1 Minuend.
	 * @param op2 Subtrahend.
	 * @param lane_mask Mask selecting one Morton lane.
	 * @return Difference of the selected lane from @p op1 and @p op2.
	 */
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		subtract_lane(std::uint64_t op1, std::uint64_t op2, std::uint64_t lane_mask) noexcept
	{
		return ((op1 & lane_mask) - (op2 & lane_mask)) & lane_mask;
	}

	/**
	 * @brief Subtracts two 64-bit 3D Morton-coded values by subtracting each lane
	 * independently and recombining the results.
	 *
	 * The x lane uses bit positions 0, 3, 6, ...
	 * The y lane uses bit positions 1, 4, 7, ...
	 * The z lane uses bit positions 2, 5, 8, ...
	 *
	 * @param op1 Minuend.
	 * @param op2 Subtrahend.
	 * @return Difference of the two Morton-coded operands.
	 */
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		subtract(std::uint64_t op1, std::uint64_t op2) noexcept
	{
		using namespace MortonCore::Detail;

		constexpr std::uint64_t x_mask = lane_mask_3d;
		constexpr std::uint64_t y_mask = lane_mask_3d << 1;
		constexpr std::uint64_t z_mask = lane_mask_3d << 2;

		return subtract_lane(op1, op2, x_mask)
			| subtract_lane(op1, op2, y_mask)
			| subtract_lane(op1, op2, z_mask);
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
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		multiply(std::uint64_t a, std::uint64_t b) noexcept
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
	 * The function repeatedly subtracts the divisor from the dividend until any
	 * coordinate lane of the dividend falls below the corresponding lane of the
	 * divisor. During each iteration it derives a three-bit increment value by
	 * comparing the corresponding Morton lanes of the current dividend and
	 * divisor, then adds that increment into the running quotient.
	 *
	 * The loop condition is evaluated per-lane rather than as a raw integer
	 * comparison. A raw comparison is not safe here because @ref subtract
	 * operates per-lane: if any lane of the dividend were smaller than the
	 * corresponding lane of the divisor, that lane would wrap to a very large
	 * value and the loop would never terminate.
	 *
	 * @param dividend Value to be divided.
	 * @param divisor Value by which to divide.
	 * @return Quotient accumulated during the repeated subtraction process.
	 */
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		divide(std::uint64_t dividend, std::uint64_t divisor) noexcept
	{
		using namespace MortonCore::Detail;

		constexpr std::uint64_t x_mask = lane_mask_3d;
		constexpr std::uint64_t y_mask = lane_mask_3d << 1;
		constexpr std::uint64_t z_mask = lane_mask_3d << 2;

		std::uint64_t quotient{ 0 };

		while (
			(dividend & x_mask) >= (divisor & x_mask) &&
			(dividend & y_mask) >= (divisor & y_mask) &&
			(dividend & z_mask) >= (divisor & z_mask))
		{
			const auto mask_dividend1 = x_mask & dividend;
			const auto mask_divisor1  = x_mask & divisor;
			const auto t1 = static_cast<std::uint64_t>(mask_divisor1 ? mask_dividend1 >= mask_divisor1 : 0);

			const auto mask_dividend2 = y_mask & dividend;
			const auto mask_divisor2  = y_mask & divisor;
			const auto t2 = static_cast<std::uint64_t>(mask_divisor2 ? mask_dividend2 >= mask_divisor2 : 0);

			const auto mask_dividend3 = z_mask & dividend;
			const auto mask_divisor3  = z_mask & divisor;
			const auto t3 = static_cast<std::uint64_t>(mask_divisor3 ? mask_dividend3 >= mask_divisor3 : 0);

			quotient = add(quotient, t1 + (t2 << 1) + (t3 << 2));
			dividend = subtract(dividend, divisor);
		}

		return quotient;
	}

	/**
	 * @brief Computes per-coordinate modulo of a Morton-coded value against
	 * power-of-two periods given as bit counts.
	 *
	 * When all three periods are powers of two (px = 2^kx, etc.) the modulo
	 * per coordinate reduces to masking the lowest @p kN bits of each lane.
	 * Those bits occupy positions 0, 3, …, 3*(kN-1) within the Morton code,
	 * so the entire operation is a single AND with no decode/encode round-trip.
	 *
	 * @param morton Morton-coded value.
	 * @param kx     Bit count for X (period = 2^kx).
	 * @param ky     Bit count for Y (period = 2^ky).
	 * @param kz     Bit count for Z (period = 2^kz).
	 * @return Morton code of (x % 2^kx, y % 2^ky, z % 2^kz).
	 */
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		modulo_pow2(std::uint64_t morton, std::uint32_t kx, std::uint32_t ky, std::uint32_t kz) noexcept
	{
		using namespace MortonCore::Detail;

		const std::uint64_t x_mask = lane_mask_3d        & ((1ULL << (3 * kx))     - 1);
		const std::uint64_t y_mask = (lane_mask_3d << 1) & ((1ULL << (3 * ky + 1)) - 1);
		const std::uint64_t z_mask = (lane_mask_3d << 2) & ((1ULL << (3 * kz + 2)) - 1);

		return morton & (x_mask | y_mask | z_mask);
	}

	/**
	 * @brief Computes per-coordinate modulo of a Morton-coded value.
	 *
	 * Applies the given period independently to each axis and returns the
	 * result as a new Morton code. Works for any period including
	 * non-powers-of-two.
	 *
	 * When all three periods are powers of two the function detects this with
	 * a single bitwise AND per period and automatically dispatches to
	 * @ref modulo_pow2, avoiding the decode/encode round-trip entirely. The
	 * test is:
	 *
	 *   ((px & (px-1)) | (py & (py-1)) | (pz & (pz-1))) == 0
	 *
	 * All three terms are OR-ed so the compiler can evaluate them in parallel
	 * and fold the result into one comparison. When the periods are
	 * compile-time constants the branch is eliminated entirely.
	 *
	 * Callers who already know their periods are powers of two should call
	 * @ref modulo_pow2 directly to skip even the test.
	 *
	 * @param morton Morton-coded value.
	 * @param px     Period along X (must be > 0).
	 * @param py     Period along Y (must be > 0).
	 * @param pz     Period along Z (must be > 0).
	 * @return Morton code of (x % px, y % py, z % pz).
	 */
	[[nodiscard]] MORTON_FORCEINLINE std::uint64_t
		modulo(std::uint64_t morton, std::uint32_t px, std::uint32_t py, std::uint32_t pz) noexcept
	{
		if (((px & (px - 1)) | (py & (py - 1)) | (pz & (pz - 1))) == 0)
		{
			return modulo_pow2(morton,
				std::countr_zero(px),
				std::countr_zero(py),
				std::countr_zero(pz));
		}

		const auto d = MortonCore::morton_decode(morton);
		return MortonCore::morton_encode(d.x % px, d.y % py, d.z % pz);
	}

	/**
	 * @brief Computes per-coordinate modulo of a Morton-coded value against
	 * power-of-two periods given as a Morton-coded period mask.
	 *
	 * This overload accepts @p period_mask as the Morton code of the largest
	 * valid coordinate in each axis — that is, morton_encode(px-1, py-1, pz-1)
	 * where each period is a power of two. Because each (pN-1) fills exactly
	 * the low kN bits of its lane with 1s, ANDing with this mask is equivalent
	 * to computing (x % px, y % py, z % pz) per coordinate.
	 *
	 * @note Only correct when every period is a power of two. For non-power-of-
	 * two periods, @p period_mask would not be an all-ones lane prefix and the
	 * result would be incorrect — use @ref modulo instead.
	 *
	 * Example:
	 * @code
	 * // 4×4×4 period: largest coordinate is (3,3,3)
	 * auto mask = morton_encode(3, 3, 3);
	 * auto wrapped = modulo_pow2(position, mask);
	 * @endcode
	 *
	 * @param morton      Morton-coded value.
	 * @param period_mask morton_encode(px-1, py-1, pz-1) where each pN is a
	 *                    power of two.
	 * @return Morton code of (x % px, y % py, z % pz).
	 */
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		modulo_pow2(std::uint64_t morton, std::uint64_t period_mask) noexcept
	{
		return morton & period_mask;
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
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		radd(std::uint64_t op1, std::uint64_t op2) noexcept
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
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		rsub(std::uint64_t op1, std::uint64_t op2) noexcept
	{
		std::uint64_t dif = op1, borrow;
		while (op2 != 0)
		{
			dif = op1 ^ op2;
			borrow = (~op1) & op2;
			borrow <<= 3;
			op1 = dif;
			op2 = borrow;
		}
		return dif;
	}

	/**
	 * @brief Descends one level into a child octant of the current node.
	 *
	 * During octree traversal the path and the octant index never share bit
	 * positions: the accumulated path occupies bits 3 and above while the
	 * octant index occupies only bits 0–2. Because there is no overlap,
	 * no carry can occur and the operation reduces to a single OR followed
	 * by a three-bit left shift. This makes descent O(1) and branch-free,
	 * replacing the iterative @ref radd for the traversal case.
	 *
	 * The three bits of @p octant encode which half of the current node is
	 * entered along each axis:
	 *   bit 0 – X half (0 = lower, 1 = upper)
	 *   bit 1 – Y half (0 = lower, 1 = upper)
	 *   bit 2 – Z half (0 = lower, 1 = upper)
	 *
	 * @param path   Accumulated Morton path to the current node.
	 * @param octant Child octant index in the range [0, 7].
	 * @return Morton path extended by one level into the selected child octant.
	 */
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		descend(std::uint64_t path, std::uint64_t octant) noexcept
	{
		return (path | octant) << 3;
	}

	/**
	 * @brief Ascends one level toward the root of the octree.
	 *
	 * Reverses a single @ref descend step by shifting the path right by three
	 * positions, stripping the most recently entered child octant. The discarded
	 * low three bits held the octant index for the current level; use
	 * @ref octant_of before ascending if that value is needed.
	 *
	 * @param path Accumulated Morton path to the current node.
	 * @return Morton path retreated by one level toward the root.
	 */
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		ascend(std::uint64_t path) noexcept
	{
		return path >> 3;
	}

	/**
	 * @brief Returns the octant index of the current node within its parent.
	 *
	 * Extracts the three low bits of @p path, which encode which child octant
	 * of the parent node the current path occupies. This is the value that was
	 * passed as @p octant in the @ref descend call that produced this path.
	 *
	 * @param path Accumulated Morton path to the current node.
	 * @return Octant index in the range [0, 7].
	 */
	[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
		octant_of(std::uint64_t path) noexcept
	{
		return path & 0x7ULL;
	}
} // namespace Morton
