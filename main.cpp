#include <iostream>
#include "bigmath.h"
#include <random>

using namespace std;

template <typename R>
bool randomBit(R & r)
{
    return uniform_int_distribution<int>(0, 1)(r) != 0;
}

template <typename R>
BigUnsigned randomBits(size_t bitCount, R & r)
{
    BigUnsigned retval = 0;
    for(size_t i = 0; i < bitCount; i++)
    {
        retval *= 2;
        if(randomBit(r))
            retval++;
    }
    return retval;
}

int main()
{
#if 1
    mt19937 r;
    for(size_t i = 0; i < 100; i++)
    {
        cout << "\x1b[2J\x1b[H";
        BigUnsigned encryptionExponent = 0x10001_bu;
        BigUnsigned decryptionExponent = 0x96273921ef49f01189187b4f5b25fbb87a8f4f90720c4a8fe98cd327708948b0542a07c15e72b0c51ace91d95fd06e53fd9d15784087816c67fd6fa439c58f2eab7f4e0c615a1d6300f289fa5f63cedbce998ebb0b6a90f6ef058a06e9d68922c8de3a40d39d164574b5107c1db2a56c5bcde7dafc20e3f1eeb63509287dfb51_bu;
        BigUnsigned modulus = 0xab7ee239ab86f342a52de0e2ab15457ac40ace5b599c537272f024911e885ae08be27280f8a4b62f631e1ceb28c59d3c396a07b964cdedee4ff72ebfe43458734ad83e1c0a61285ba1b2f8a20da7b4f3681fda70facc7898b35b9b2665ade17d9314bd0709716af443b45e570a3ec0c4ed3dde4cc9228e6ea987012c5b0ecf89_bu;
        size_t randomBitCount = 100;
        string v = powMod((BigUnsigned::fromByteString("This is a test.") << randomBitCount) | randomBits(randomBitCount, r), encryptionExponent, modulus).toBase64();
        cout << v << "\n";
        cout << (powMod(BigUnsigned::parseBase64(v), decryptionExponent, modulus) >> randomBitCount).toByteString() << "\n";
    }
#else
    mt19937 r;
    for(size_t i = 0; i < 64; i++)
    {
        BigUnsigned v = 0;
        for(size_t j = 0; j < 100; j++)
            v |= randomBits(i, r);
        cout << hex << v << "\n" << pow(2_bu, i) << "\n\n";
    }
#endif
    return 0;
}
