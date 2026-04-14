

#include "MortonTests.h"


int main()
{
	MortonBench::bitCountTest();
	MortonBench::test_add_compare();
	MortonBench::test_sub();
	MortonBench::test_mul();
	MortonBench::test_div();
	MortonBench::TestMortonEncodeMagicBits();
	MortonBench::TestMortonDecodeMagicBits();
	MortonBench::TestMortonEncodeDecodeRoundTrip();
}

