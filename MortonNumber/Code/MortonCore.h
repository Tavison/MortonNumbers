#pragma once

#include <cstdint>

#if defined(_MSC_VER)
#   define MORTON_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#   define MORTON_FORCEINLINE __attribute__((always_inline)) inline
#else
#   define MORTON_FORCEINLINE inline
#endif

#define MORTON_USE_BMI2

#if defined(MORTON_USE_BMI2)
#include <immintrin.h>
#endif

namespace MortonCore
{
	/**
	* @brief Core functions for encoding and decoding 3D Morton codes using
	* magic bits.
	*
	* This header defines the core bit manipulation routines for interleaving
	* and deinterleaving coordinate bits in a 64-bit Morton code. The functions
	* use a staged approach with precomputed masks to efficiently spread and
	* compact bits.
	*/

	/**
	* @brief Point 3 for decoding Morton Numbers
	*
	*/
	struct DecodedMorton
	{
		std::uint32_t x;
		std::uint32_t y;
		std::uint32_t z;
	};

	namespace Detail
	{
		/**
		 * @brief Staged masks used by the portable 64-bit 3D Morton encoder/decoder.
		 *
		 * The portable implementation does not spread or compact coordinate bits in a
		 * single step. Instead, it progressively moves valid groups of bits farther
		 * apart during encoding, and progressively compacts them during decoding.
		 *
		 * stage_1 limits the coordinate to the low 21 bits.
		 * stage_2 through stage_5 preserve the correct groups after each intermediate
		 * shift-and-combine step.
		 * stage_6 is the final 3D lane mask, containing bit positions 0, 3, 6, 9, ...
		 *
		 * stage_6 is also ailiased as lane_mask_3d as the canonical lane-selection mask used by hardware-assisted
		 * BMI2 implementations based on PDEP and PEXT.
		 */
		 
		 /**
		 * @brief Mask that preserves the lowest 21 coordinate bits.
		 *
		 * A 64-bit 3D Morton code can interleave at most 21 bits from each
		 * coordinate, for a total of 63 occupied bit positions.
		 */
		inline constexpr std::uint64_t stage_1 =
			0b0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0001'1111'1111'1111'1111'1111ull;

		/**
		 * @brief Mask used after the 32-bit expansion stage in @ref zip_by_3_64.
		 *
		 * This mask preserves the bit groups that remain valid after shifting and
		 * combining the source value by 32 positions.
		 */
		inline constexpr std::uint64_t stage_2 =
			0b0000'0000'0001'1111'0000'0000'0000'0000'0000'0000'0000'0000'1111'1111'1111'1111ull;

		/**
		 * @brief Mask used after the 16-bit expansion stage in @ref zip_by_3_64.
		 */
		inline constexpr std::uint64_t stage_3 =
			0b0000'0000'0001'1111'0000'0000'0000'0000'1111'1111'0000'0000'0000'0000'1111'1111ull;

		/**
		 * @brief Mask used after the 8-bit expansion stage in @ref zip_by_3_64.
		 */
		inline constexpr std::uint64_t stage_4 =
			0b0001'0000'0000'1111'0000'0000'1111'0000'0000'1111'0000'0000'1111'0000'0000'1111ull;

		/**
		 * @brief Mask used after the 4-bit expansion stage in @ref zip_by_3_64.
		 */
		inline constexpr std::uint64_t stage_5 =
			0b0001'0000'1100'0011'0000'1100'0011'0000'1100'0011'0000'1100'0011'0000'1100'0011ull;

		/**
		 * @brief Final 3D Morton lane mask.
		 *
		 * This mask keeps every third bit position. After the final spreading step,
		 * the source coordinate bits occupy these positions.
		 */
		inline constexpr std::uint64_t stage_6 =
			0b0001'0010'0100'1001'0010'0100'1001'0010'0100'1001'0010'0100'1001'0010'0100'1001ull;

		/**
		 * @brief Final 3D Morton lane mask aliased to make BMI2 code easier to understand.
		 */
		inline constexpr std::uint64_t lane_mask_3d = stage_6;
	}

	namespace MortonCoreSoftware
	{
		/**
		 * @brief Expands a 21-bit coordinate so that each source bit occupies every
		 * third position in a 64-bit Morton lane.
		 *
		 * The function progressively spreads the low 21 bits of the input by shifting,
		 * combining, and masking. The returned value contains the coordinate bits in
		 * positions 0, 3, 6, 9, and so on.
		 *
		 * @param a Coordinate value whose lowest 21 bits will be expanded.
		 * @return Expanded 64-bit Morton lane containing the bits of @p a.
		 */
		[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t zip_by_3_64(std::uint32_t a) noexcept
		{
			using namespace MortonCore::Detail;

			std::uint64_t x = static_cast<std::uint64_t>(a) & stage_1;
			x = (x | (x << 32)) & stage_2;
			x = (x | (x << 16)) & stage_3;
			x = (x | (x << 8)) & stage_4;
			x = (x | (x << 4)) & stage_5;
			x = (x | (x << 2)) & stage_6;

			return x;
		}

		/**
		 * @brief Interleaves three coordinates into a single 64-bit 3D Morton code.
		 *
		 * The x coordinate is placed in bit positions 0, 3, 6, ...; the y coordinate
		 * in positions 1, 4, 7, ...; and the z coordinate in positions 2, 5, 8, ....
		 *
		 * @param x X coordinate whose lowest 21 bits will be encoded.
		 * @param y Y coordinate whose lowest 21 bits will be encoded.
		 * @param z Z coordinate whose lowest 21 bits will be encoded.
		 * @return 64-bit Morton code containing the interleaved coordinate bits.
		 */
		[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint64_t
			morton_encode(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
		{
			return zip_by_3_64(x)
				| (zip_by_3_64(y) << 1)
				| (zip_by_3_64(z) << 2);
		}

		/**
		 * @brief Extracts one coordinate lane from a 64-bit 3D Morton code.
		 *
		 * The function reverses the spreading process performed by @ref zip_by_3_64.
		 * It compacts every third bit from the input value back into a contiguous
		 * coordinate value.
		 *
		 * @param morton Morton lane value to compact.
		 * @return Coordinate reconstructed from the extracted Morton lane.
		 */
		[[nodiscard]] constexpr MORTON_FORCEINLINE std::uint32_t
			unzip_by_3_64(std::uint64_t morton) noexcept
		{
			using namespace MortonCore::Detail;

			std::uint64_t x = morton & stage_6;
			x = (x ^ (x >> 2)) & stage_5;
			x = (x ^ (x >> 4)) & stage_4;
			x = (x ^ (x >> 8)) & stage_3;
			x = (x ^ (x >> 16)) & stage_2;
			x = (x ^ (x >> 32)) & stage_1;

			return static_cast<std::uint32_t>(x);
		}

		/**
		 * @brief Decodes a 64-bit 3D Morton code into x, y, and z coordinates.
		 *
		 * The function extracts each coordinate lane by shifting the Morton value so
		 * that the requested lane is aligned with bit positions 0, 3, 6, ... and then
		 * compacting that lane with @ref unzip_by_3_64.
		 *
		 * @param x Output x coordinate.
		 * @param y Output y coordinate.
		 * @param z Output z coordinate.
		 * @param morton Morton code to decode.
		 */
		[[nodiscard]] constexpr MORTON_FORCEINLINE DecodedMorton
			morton_decode(std::uint64_t morton) noexcept
		{
			return {
				unzip_by_3_64(morton),
				unzip_by_3_64(morton >> 1),
				unzip_by_3_64(morton >> 2)
			};
		}
	}
	namespace MortonCoreHardware
	{
		/**
		* @brief Expands a 21-bit coordinate into a 64-bit Morton lane using the
		* x86 BMI2 PDEP instruction.
		*
		* PDEP, or Parallel Bits Deposit, takes the low-order bits from the source
		* value and deposits them into the bit positions selected by the mask.
		*
		* In this case, the mask is the 3D Morton lane mask:
		* bit positions 0, 3, 6, 9, and so on.
		*
		* That means:
		* - source bit 0 is written to Morton bit 0
		* - source bit 1 is written to Morton bit 3
		* - source bit 2 is written to Morton bit 6
		* - and so on
		*
		* Functionally, this produces the same kind of result as @ref zip_by_3_64,
		* but uses a dedicated hardware instruction instead of the staged
		* shift-and-mask expansion process.
		*
		* This function should only be compiled or called on CPUs that support BMI2.
		*
		* @param a Coordinate value whose lowest 21 bits will be deposited into
		* the x-lane positions of a 64-bit Morton value.
		* @return 64-bit Morton lane containing the bits of @p a in positions
		* 0, 3, 6, 9, and so on.
		*/
		[[nodiscard]] MORTON_FORCEINLINE std::uint64_t
			zip_by_3_64(std::uint32_t a) noexcept
		{
			return _pdep_u64(static_cast<std::uint64_t>(a) & Detail::stage_1, Detail::lane_mask_3d);
		}

		/**
		* @brief Compacts one coordinate lane from a 64-bit Morton value using the
		* x86 BMI2 PEXT instruction.
		*
		* PEXT, or Parallel Bits Extract, reads the bits from the source value only
		* at the positions selected by the mask and packs those bits densely into
		* the low-order portion of the result.
		*
		* In this case, the mask is the 3D Morton lane mask:
		* bit positions 0, 3, 6, 9, and so on.
		*
		* That means:
		* - Morton bit 0 becomes result bit 0
		* - Morton bit 3 becomes result bit 1
		* - Morton bit 6 becomes result bit 2
		* - and so on
		*
		* Functionally, this performs the same job as @ref unzip_by_3_64, but uses
		* a dedicated hardware instruction instead of the staged reverse compaction
		* process.
		*
		* This function should only be compiled or called on CPUs that support BMI2.
		*
		* @param morton Morton lane value from which one coordinate lane will be
		* extracted and compacted.
		* @return Coordinate reconstructed from the selected Morton lane.
		*/
		[[nodiscard]] MORTON_FORCEINLINE std::uint32_t
			unzip_by_3_64(std::uint64_t morton) noexcept
		{
			return static_cast<std::uint32_t>(_pext_u64(morton, Detail::lane_mask_3d));
		}

		/**
		* @brief Interleaves three coordinates into a single 64-bit 3D Morton code
		* using x86 BMI2 hardware instructions.
		*
		* Each coordinate is first expanded into its own Morton lane with
		* @ref zip_by_3_64.
		*
		* The x coordinate occupies bit positions 0, 3, 6, ...
		* The y coordinate occupies bit positions 1, 4, 7, ...
		* The z coordinate occupies bit positions 2, 5, 8, ...
		*
		* This function produces the same logical result as
		* @ref morton_encode, but uses PDEP-based expansion rather than
		* the portable staged magic-bits method.
		*
		* This function should only be compiled or called on CPUs that support BMI2.
		*
		* @param x X coordinate whose lowest 21 bits will be encoded.
		* @param y Y coordinate whose lowest 21 bits will be encoded.
		* @param z Z coordinate whose lowest 21 bits will be encoded.
		* @return 64-bit Morton code containing the interleaved coordinate bits.
		*/
		[[nodiscard]] MORTON_FORCEINLINE std::uint64_t
			morton_encode(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
		{
			return zip_by_3_64(x)
				| (zip_by_3_64(y) << 1)
				| (zip_by_3_64(z) << 2);
		}

		/**
		* @brief Decodes a 64-bit 3D Morton code into x, y, and z coordinates using
		* x86 BMI2 hardware instructions.
		*
		* The function extracts each coordinate lane by shifting the Morton value
		* so that the desired lane aligns with the x-lane mask positions
		* 0, 3, 6, ... and then compacts that lane with @ref unzip_by_3_64.
		*
		* This function produces the same logical result as
		* @ref morton_decode, but uses PEXT-based extraction rather than
		* the portable staged magic-bits method.
		*
		* This function should only be compiled or called on CPUs that support BMI2.
		*
		* @param morton Morton code to decode.
		* @return Structure containing the decoded x, y, and z coordinates.
		*/
		[[nodiscard]] MORTON_FORCEINLINE DecodedMorton
			morton_decode(std::uint64_t morton) noexcept
		{
			return {
				unzip_by_3_64(morton),
				unzip_by_3_64(morton >> 1),
				unzip_by_3_64(morton >> 2)
			};
		}
	}

#if defined(MORTON_USE_BMI2)
	using MortonCoreHardware::zip_by_3_64;
	using MortonCoreHardware::unzip_by_3_64;
	using MortonCoreHardware::morton_encode;
	using MortonCoreHardware::morton_decode;
#else
	using MortonCoreSoftware::zip_by_3_64;
	using MortonCoreSoftware::unzip_by_3_64;
	using MortonCoreSoftware::morton_encode;
	using MortonCoreSoftware::morton_decode;
#endif

}
