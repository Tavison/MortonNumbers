#pragma once


#include "../Code/MortonNumbers.h"	// Morton Numbers
#include "../Code/MortonDebug.h"

#include <cstdint>	// For specific int types
#include <chrono>	// For timers and clocks
#include <iostream>	// For cout
#include <bit>
#include <string_view>


namespace MortonBench
{
	uint64_t BitCount(uint64_t x)
	{
		return std::popcount(x);
	}

	/**
	 * @brief Runs a benchmark loop and returns the elapsed time in nanoseconds.
	 *
	 * The supplied function is called once per iteration with the current loop
	 * index. Its return value is mixed into a checksum so the compiler cannot
	 * discard the work as dead code.
	 *
	 * The loop body is intentionally generic so the same benchmark harness can be
	 * reused for Morton addition, subtraction, encoding, decoding, and other
	 * operations.
	 *
	 * @tparam ResultFn Callable type taking std::uint64_t and returning a value
	 * convertible to std::uint64_t.
	 * @param iterations Number of loop iterations to execute.
	 * @param result_fn Function that produces the benchmarked result for each
	 * iteration.
	 * @param checksum_out Receives the final checksum value.
	 * @return Elapsed time in nanoseconds for the benchmark loop.
	 */
	template <typename ResultFn>
	std::uint64_t benchmark_loop(
		std::uint64_t iterations,
		ResultFn result_fn,
		std::uint64_t& checksum_out)
	{
		std::uint64_t checksum = 0;

		const auto t1 = std::chrono::high_resolution_clock::now();

		for (std::uint64_t i = 0; i < iterations; ++i)
		{
			const std::uint64_t result =
				static_cast<std::uint64_t>(result_fn(i));

			checksum = std::rotl(checksum, 7)
				^ (result + 0x9e3779b97f4a7c15ull);
			checksum *= 0xbf58476d1ce4e5b9ull;
		}

		const auto t2 = std::chrono::high_resolution_clock::now();

		checksum_out = checksum;

		return static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count());
	}

	/**
	 * @brief Runs a reusable baseline benchmark loop.
	 *
	 * This baseline is meant to approximate the loop, input generation, and
	 * checksum cost of a benchmark without including the actual operation under
	 * test. The closer this baseline resembles the structure of the real benchmark,
	 * the more meaningful the "net" subtraction becomes.
	 *
	 * The supplied function should usually generate the same per-iteration inputs
	 * as the real benchmark, but replace the expensive operation with a trivial
	 * expression.
	 *
	 * @tparam BaselineFn Callable type taking std::uint64_t and returning a value
	 * convertible to std::uint64_t.
	 * @param label Name to print for the baseline.
	 * @param iterations Number of loop iterations to execute.
	 * @param baseline_fn Function producing the baseline result for each iteration.
	 * @return Elapsed time in nanoseconds for the baseline loop.
	 */
	template <typename BaselineFn>
	std::uint64_t benchmark_baseline(
		std::string_view label,
		std::uint64_t iterations,
		BaselineFn baseline_fn)
	{
		std::uint64_t checksum = 0;

		const std::uint64_t elapsed_ns =
			benchmark_loop(iterations, baseline_fn, checksum);

		std::cout << label << " for " << iterations << " runs took "
			<< elapsed_ns << " ns. Checksum: "
			<< checksum << '\n';

		return elapsed_ns;
	}

	/**
	 * @brief Benchmarks one operation and prints its elapsed time and checksum.
	 *
	 * @tparam OperationFn Callable type taking std::uint64_t and returning a value
	 * convertible to std::uint64_t.
	 * @param label Name to print for the benchmark.
	 * @param iterations Number of loop iterations to execute.
	 * @param operation_fn Function producing the benchmark result for each
	 * iteration.
	 * @return Elapsed time in nanoseconds for the benchmark loop.
	 */
	template <typename OperationFn>
	std::uint64_t benchmark_operation(
		std::string_view label,
		std::uint64_t iterations,
		OperationFn operation_fn)
	{
		std::uint64_t checksum = 0;

		const std::uint64_t elapsed_ns =
			benchmark_loop(iterations, operation_fn, checksum);

		std::cout << label << " for " << iterations << " runs took "
			<< elapsed_ns << " ns. Checksum: "
			<< checksum << '\n';

		return elapsed_ns;
	}

	/**
	 * @brief Benchmarks a baseline loop and an operation loop, then prints the
	 * net time difference.
	 *
	 * This is useful when the measured operation is small enough that loop,
	 * input-generation, and checksum costs meaningfully affect the total time.
	 *
	 * The baseline and operation functions should have the same broad structure and
	 * input-generation pattern so that the subtraction is as fair as possible.
	 *
	 * @tparam BaselineFn Callable type taking std::uint64_t and returning a value
	 * convertible to std::uint64_t.
	 * @tparam OperationFn Callable type taking std::uint64_t and returning a value
	 * convertible to std::uint64_t.
	 * @param label Name of the operation under test.
	 * @param iterations Number of loop iterations to execute.
	 * @param baseline_fn Function that models loop/input/checksum overhead.
	 * @param operation_fn Function that performs the real operation under test.
	 */
	template <typename BaselineFn, typename OperationFn>
	void benchmark_with_baseline(
		std::string_view label,
		std::uint64_t iterations,
		BaselineFn baseline_fn,
		OperationFn operation_fn)
	{
		std::uint64_t baseline_checksum = 0;
		std::uint64_t operation_checksum = 0;

		const std::uint64_t baseline_ns =
			benchmark_loop(iterations, baseline_fn, baseline_checksum);

		const std::uint64_t operation_ns =
			benchmark_loop(iterations, operation_fn, operation_checksum);

		std::cout << label << '\n';
		std::cout << "  baseline for " << iterations << " runs took "
			<< baseline_ns << " ns. Checksum: "
			<< baseline_checksum << '\n';
		std::cout << "  full     for " << iterations << " runs took "
			<< operation_ns << " ns. Checksum: "
			<< operation_checksum << '\n';
		std::cout << "  net: " << (operation_ns - baseline_ns) << " ns\n";
	}

	/**
	 * @brief Compares two operations against the same baseline.
	 *
	 * This is useful when evaluating two implementations of the same algorithm,
	 * such as iterative Morton addition versus masked-lane Morton addition.
	 *
	 * The same baseline function is used for both measurements so that the net
	 * comparison is performed against the same modeled loop overhead.
	 *
	 * @tparam BaselineFn Callable type taking std::uint64_t and returning a value
	 * convertible to std::uint64_t.
	 * @tparam FnA Callable type taking std::uint64_t and returning a value
	 * convertible to std::uint64_t.
	 * @tparam FnB Callable type taking std::uint64_t and returning a value
	 * convertible to std::uint64_t.
	 * @param label_a Printed name for the first implementation.
	 * @param fn_a First implementation under test.
	 * @param label_b Printed name for the second implementation.
	 * @param fn_b Second implementation under test.
	 * @param iterations Number of loop iterations to execute.
	 * @param baseline_fn Function that models loop/input/checksum overhead.
	 */
	template <typename BaselineFn, typename FnA, typename FnB>
	void benchmark_compare(
		std::string_view label_a,
		FnA fn_a,
		std::string_view label_b,
		FnB fn_b,
		std::uint64_t iterations,
		BaselineFn baseline_fn)
	{
		benchmark_with_baseline(label_a, iterations, baseline_fn, fn_a);
		benchmark_with_baseline(label_b, iterations, baseline_fn, fn_b);
	}

	template <typename ResultFn>
	std::uint64_t run_benchmark_body(ResultFn result_fn, std::uint64_t& checksum_out)
	{
		std::uint64_t checksum = 0;

		auto t1 = std::chrono::high_resolution_clock::now();

		for (std::uint64_t i = 0; i < 1'000'000; ++i)
		{
			const std::uint64_t a = i;
			const std::uint64_t b = i * 2;

			const std::uint64_t r = result_fn(a, b);

			checksum = std::rotl(checksum, 7) ^ (r + 0x9e3779b97f4a7c15ull);
			checksum *= 0xbf58476d1ce4e5b9ull;
		}

		auto t2 = std::chrono::high_resolution_clock::now();
		checksum_out = checksum;

		return std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
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
		volatile std::uint64_t k(0b1011'0110'1101'1011'0110'1101'1011'0110);
		// std::uint64_t k = 0;
		volatile std::uint64_t l = 0b111;
		auto t1 = std::chrono::high_resolution_clock::now();
		for (std::uint64_t i = 0; i < 1000; ++i)
		{
			k = Morton::add_iterative(k, l);
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
		std::cout << "Add for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
	}

	void test_add_compare()
	{
		constexpr std::uint64_t iterations = 1'000'000;

		auto baseline_fn = [](std::uint64_t i) noexcept -> std::uint64_t
			{
				const std::uint64_t a = i;
				const std::uint64_t b = i * 10;

				// Broadly similar structure to the add benchmark, but cheap.
				return a ^ b;
			};

		auto iterative_fn = [](std::uint64_t i) noexcept -> std::uint64_t
			{
				const std::uint64_t a = i;
				const std::uint64_t b = i * 10;
				return Morton::add_iterative(a, b);
			};

		auto masked_fn = [](std::uint64_t i) noexcept -> std::uint64_t
			{
				const std::uint64_t a = i;
				const std::uint64_t b = i * 10;
				return Morton::add_masked(a, b);
			};

		for (int run = 0; run < 5; ++run)
		{
			benchmark_compare(
				"Iterative add",
				iterative_fn,
				"Masked add   ",
				masked_fn,
				iterations,
				baseline_fn);

			std::cout << "----\n";
		}
	}
	void test_sub()
	{
		volatile std::uint64_t k = 0b111111111111111111111111111111;
		volatile std::uint64_t l = 0b111;
		auto t1 = std::chrono::high_resolution_clock::now();
		for (std::uint64_t i = 0; i < 1000; ++i)
		{
			k = Morton::subtract(k, l);
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
		std::cout << "Subtract for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
	}

	void test_mul()
	{
		volatile std::uint64_t k = 0b0000'0000'0000'0000'1000'1000'1000'1000;
		volatile std::uint64_t l = 0b1001'0000;
		auto t1 = std::chrono::high_resolution_clock::now();
		for (std::uint64_t i = 0; i < 1000; ++i)
		{
			volatile auto res_morton = Morton::multiply(k, l);
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
		std::cout << "Multiply for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
	}

	void test_div()
	{
		volatile std::uint64_t k = 0b1000'0000'0000'0000'1000'1000'1000'1000'0000'0000'0000'0000'0000'0000'0000'0000;
		volatile std::uint64_t l = 0b0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0101'0000;

		auto t1 = std::chrono::high_resolution_clock::now();
		for (std::uint64_t i = 0; i < 1000; ++i)
		{
			volatile auto res_morton = Morton::divide(k, l);
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
		std::cout << "Divide for 1000 runs took " << d.count() << " nanoseconds." << std::endl;
	}

	struct Coord3
	{
		std::uint32_t x;
		std::uint32_t y;
		std::uint32_t z;
	};

	inline std::vector<Coord3> BuildTestCoords()
	{
		std::vector<Coord3> coords;
		coords.reserve(100 * 100 * 100);

		for (std::uint32_t x = 0; x < 100; ++x)
		{
			for (std::uint32_t y = 0; y < 100; ++y)
			{
				for (std::uint32_t z = 0; z < 100; ++z)
				{
					coords.push_back({ x, y, z });
				}
			}
		}

		return coords;
	}

	inline void TestMortonEncodeMagicBits()
	{
		const auto coords = BuildTestCoords();
		std::vector<std::uint64_t> encoded;
		encoded.resize(coords.size());

		volatile std::uint64_t checksum = 0;

		const auto t1 = std::chrono::steady_clock::now();

		for (std::size_t i = 0; i < coords.size(); ++i)
		{
			const auto& c = coords[i];
			const std::uint64_t value = MortonCore::morton_encode(c.x, c.y, c.z);
			encoded[i] = value;
			checksum ^= value + 0x9e3779b97f4a7c15ull + (checksum << 6) + (checksum >> 2);
		}

		const auto t2 = std::chrono::steady_clock::now();
		const auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);

		std::cout << "Encode for " << coords.size() << " runs took "
			<< d.count() << " ns. Checksum: " << checksum << '\n';
	}

	inline void TestMortonDecodeMagicBits()
	{
		const auto coords = BuildTestCoords();
		std::vector<std::uint64_t> encoded;
		encoded.resize(coords.size());

		for (std::size_t i = 0; i < coords.size(); ++i)
		{
			const auto& c = coords[i];
			encoded[i] = MortonCore::morton_encode(c.x, c.y, c.z);
		}

		volatile std::uint64_t checksum = 0;

		const auto t1 = std::chrono::steady_clock::now();

		for (const std::uint64_t morton : encoded)
		{
			const auto decoded = MortonCore::morton_decode(morton);
			checksum += decoded.x;
			checksum += decoded.y;
			checksum += decoded.z;
		}

		const auto t2 = std::chrono::steady_clock::now();
		const auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);

		std::cout << "Decode for " << encoded.size() << " runs took "
			<< d.count() << " ns. Checksum: " << checksum << '\n';
	}

	inline void TestMortonEncodeDecodeRoundTrip()
	{
		const auto coords = BuildTestCoords();
		std::vector<std::uint64_t> encoded;
		encoded.resize(coords.size());

		volatile std::uint64_t checksum = 0;

		const auto t1 = std::chrono::steady_clock::now();

		for (std::size_t i = 0; i < coords.size(); ++i)
		{
			const auto& c = coords[i];
			encoded[i] = MortonCore::morton_encode(c.x, c.y, c.z);
		}

		for (const std::uint64_t morton : encoded)
		{
			const auto decoded = MortonCore::morton_decode(morton);
			checksum += decoded.x;
			checksum += decoded.y;
			checksum += decoded.z;
		}

		const auto t2 = std::chrono::steady_clock::now();
		const auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);

		std::cout << "Encode + decode round trip for " << coords.size() << " runs took "
			<< d.count() << " ns. Checksum: " << checksum << '\n';
	}
}





