#include "MortonTests.h"

int main()
{
	MortonBench::bitCountTest();
	MortonBench::test_add();
	MortonBench::test_subtract();
	MortonBench::test_mul();
	// MortonBench::test_div();
	MortonBench::test_modulo();
	MortonBench::test_traversal();
	MortonBench::TestMortonEncodeMagicBits();
	MortonBench::TestMortonDecodeMagicBits();
	MortonBench::TestMortonEncodeDecodeRoundTrip();
}
