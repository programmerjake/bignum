#ifndef BIGMATH_H
#define BIGMATH_H

#include <stdint.h>
#include <ostream>
#include <cctype>
#include <stdexcept>
#include <cmath>
#include <climits>
#include <utility> // for swap

using namespace std;

typedef uint32_t WordType;
typedef uint64_t DoubleWordType;
const WordType WordMax = ~(WordType)0;
const size_t BytesPerWord = sizeof(WordType) / sizeof(uint8_t);
const size_t BitsPerWord = BytesPerWord * 8;

class BigUnsigned
{
    struct Data
    {
        WordType * words;
        WordType word;
        size_t size, allocated;
        size_t refCount;
        Data(WordType v = 0, size_t size = 1)
            : words(&word), word(v), size(size), allocated(size), refCount(1)
        {
            if(size > 1)
            {
                words = new WordType[allocated];
                words[0] = v;
                for(size_t i = 1; i < size; i++)
                    words[i] = 0;
            }
        }
        Data(Data & rt)
            : refCount(1)
        {
            if(rt.size <= 1)
            {
                size = 1;
                words = &word;
                word = 0;
                if(rt.size != 0)
                    word = rt.words[0];
                allocated = 1;
            }
            else
            {
                size = rt.size;
                allocated = rt.size;
                words = new WordType[allocated];
                for(size_t i = 0; i < size; i++)
                {
                    words[i] = rt.words[i];
                }
            }
        }
        ~Data()
        {
            if(words != &word)
                delete []words;
        }
        void expand(size_t newSize)
        {
            if(newSize <= size)
                return;
            if(newSize > allocated)
            {
                allocated = newSize + size / 4;
                WordType * newWords = new WordType[allocated];
                for(size_t i = 0; i < size; i++)
                {
                    newWords[i] = words[i];
                }
                if(words != &word)
                    delete []words;
                words = newWords;
            }
            for(size_t i = size; i < newSize; i++)
            {
                words[i] = 0;
            }
            size = newSize;
        }
        void resize(size_t newSize)
        {
            expand(newSize);
            size = newSize;
        }
        void addRef()
        {
            refCount++;
        }
        void delRef()
        {
            refCount--;
            if(refCount == 0)
            {
                delete this;
            }
        }
    };
    Data * data;
    void onWrite()
    {
        if(data->refCount <= 1)
            return;
        Data * newData = new Data(*data);
        data->delRef();
        data = newData;
    }
    void normalize()
    {
        onWrite();
        while(data->size > 1 && data->words[data->size - 1] == 0)
            data->size--;
    }
    BigUnsigned(WordType v, size_t size)
        : data(new Data(v, size))
    {
    }
    static char getHexDigit(unsigned digit)
    {
        if(digit <= 9)
            return digit + '0';
        return digit - 0xA + 'A';
    }
    enum {SmallNumberCount = 32};
    static Data * smallNumbers;
public:
    BigUnsigned(WordType v = 0)
    {
        if(v < SmallNumberCount)
        {
            if(!smallNumbers)
            {
                smallNumbers = new Data[SmallNumberCount];
                for(WordType i = 0; i < SmallNumberCount; i++)
                {
                    smallNumbers[i].words[0] = i;
                }
            }
            data = &smallNumbers[v];
            data->addRef();
        }
        else
            data = new Data(v);
    }
    ~BigUnsigned()
    {
        data->delRef();
    }
    BigUnsigned(const BigUnsigned & rt)
    {
        data = rt.data;
        data->addRef();
    }
    BigUnsigned(BigUnsigned && rt)
        : BigUnsigned(0)
    {
        swap(rt);
    }
    const BigUnsigned & operator =(BigUnsigned && rt)
    {
        swap(rt);
        return *this;
    }
    const BigUnsigned & operator =(const BigUnsigned & rt)
    {
        rt.data->addRef();
        data->delRef();
        data = rt.data;
        return *this;
    }
    const BigUnsigned & operator =(WordType v)
    {
        if(data->size == 1 && data->words[0] == v)
            return *this;
        if(data->refCount > 1)
        {
            BigUnsigned bv(v);
            swap(bv);
        }
        else
        {
            data->size = 1;
            data->words[0] = v;
        }
        return *this;
    }
    static BigUnsigned parseHexByteString(string str);
    string toHexByteString() const;
    static BigUnsigned fromByteString(string str);
    string toByteString() const;
    static BigUnsigned parse(string str, unsigned base);
    static BigUnsigned parse(string str, bool useOctal = false)
    {
        if(str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X")
            return parse(str.substr(2), 0x10U);
        if(useOctal && str.substr(0, 1) == "0")
            return parse(str.substr(1), 010U);
        return parse(str, 10U);
    }
    string toString(unsigned base = 10) const;
    static BigUnsigned parseBase64(string str);
    string toBase64() const;
    friend bool operator ==(WordType w, BigUnsigned n)
    {
        if(n.data->size > 1)
            return false;
        return n.data->words[0] == w;
    }
    friend bool operator ==(BigUnsigned n, WordType w)
    {
        if(n.data->size > 1)
            return false;
        return n.data->words[0] == w;
    }
    friend bool operator ==(BigUnsigned a, BigUnsigned b)
    {
        if(a.data == b.data)
            return true;
        if(a.data->size != b.data->size)
            return false;
        for(size_t i = 0; i < a.data->size; i++)
        {
            if(a.data->words[i] != b.data->words[i])
                return false;
        }
        return true;
    }
    friend bool operator !=(WordType a, BigUnsigned b)
    {
        return !operator ==(a, b);
    }
    friend bool operator !=(BigUnsigned a, WordType b)
    {
        return !operator ==(a, b);
    }
    friend bool operator !=(BigUnsigned a, BigUnsigned b)
    {
        return !operator ==(a, b);
    }
    friend bool operator >(WordType a, BigUnsigned b)
    {
        if(b.data->size > 1)
            return false;
        return a > b.data->words[0];
    }
    friend bool operator >(BigUnsigned a, WordType b)
    {
        if(a.data->size > 1)
            return true;
        return a.data->words[0] > b;
    }
    friend bool operator >(BigUnsigned a, BigUnsigned b)
    {
        if(a.data == b.data)
            return false;
        if(a.data->size > b.data->size)
            return true;
        if(a.data->size < b.data->size)
            return false;
        for(size_t i = 0, j = a.data->size - 1; i < a.data->size; i++, j--)
        {
            if(a.data->words[j] > b.data->words[j])
                return true;
            if(a.data->words[j] < b.data->words[j])
                return false;
        }
        return false;
    }
    friend int compare(BigUnsigned a, BigUnsigned b)
    {
        if(a.data == b.data)
            return 0;
        if(a.data->size > b.data->size)
            return 1;
        if(a.data->size < b.data->size)
            return -1;
        for(size_t i = 0, j = a.data->size - 1; i < a.data->size; i++, j--)
        {
            if(a.data->words[j] > b.data->words[j])
                return 1;
            if(a.data->words[j] < b.data->words[j])
                return -1;
        }
        return 0;
    }
    friend bool operator <(WordType a, BigUnsigned b)
    {
        return operator >(b, a);
    }
    friend bool operator <(BigUnsigned a, WordType b)
    {
        return operator >(b, a);
    }
    friend bool operator <(BigUnsigned a, BigUnsigned b)
    {
        return operator >(b, a);
    }
    friend bool operator >=(WordType a, BigUnsigned b)
    {
        return !operator <(a, b);
    }
    friend bool operator >=(BigUnsigned a, WordType b)
    {
        return !operator <(a, b);
    }
    friend bool operator >=(BigUnsigned a, BigUnsigned b)
    {
        return !operator <(a, b);
    }
    friend bool operator <=(WordType a, BigUnsigned b)
    {
        return !operator >(a, b);
    }
    friend bool operator <=(BigUnsigned a, WordType b)
    {
        return !operator >(a, b);
    }
    friend bool operator <=(BigUnsigned a, BigUnsigned b)
    {
        return !operator >(a, b);
    }
    const BigUnsigned & operator +=(BigUnsigned b);
    const BigUnsigned & operator +=(WordType b);
    friend BigUnsigned operator +(BigUnsigned a, BigUnsigned b)
    {
        a += b;
        return a;
    }
    friend BigUnsigned operator +(BigUnsigned a, WordType b)
    {
        a += b;
        return a;
    }
    friend BigUnsigned operator +(WordType a, BigUnsigned b)
    {
        b += a;
        return b;
    }
    const BigUnsigned & operator -=(BigUnsigned b);
    const BigUnsigned & operator -=(WordType b);
    friend BigUnsigned operator -(BigUnsigned a, WordType b)
    {
        a -= b;
        return a;
    }
    friend BigUnsigned operator -(WordType a, BigUnsigned b)
    {
        if(a < b)
        {
            BigUnsigned retval(a);
            retval -= b; // so it uses the error handling code
            return retval;
        }
        b.onWrite();
        b.data->words[0] = a - b.data->words[0];
        return b;
    }
    friend BigUnsigned operator -(BigUnsigned a, BigUnsigned b)
    {
        a -= b;
        return a;
    }
    friend BigUnsigned operator *(BigUnsigned a, WordType b);
    friend BigUnsigned operator *(WordType a, BigUnsigned b)
    {
        return operator *(b, a);
    }
    friend BigUnsigned operator *(BigUnsigned a, BigUnsigned b);
    const BigUnsigned & operator *=(WordType b)
    {
        return operator =(operator *(*this, b));
    }
    const BigUnsigned & operator *=(BigUnsigned b)
    {
        return operator =(operator *(*this, b));
    }
    void swap(BigUnsigned & b)
    {
        Data * temp = data;
        data = b.data;
        b.data = temp;
    }
private:
    static void divMod(BigUnsigned dividend, BigUnsigned divisor, BigUnsigned * pquotient, BigUnsigned * premainder);
    static void divMod(WordType dividend, BigUnsigned divisor, BigUnsigned * pquotient, BigUnsigned * premainder);
    static void divMod(BigUnsigned dividend, WordType divisor, BigUnsigned * pquotient, BigUnsigned * premainder);
public:
    static void divMod(BigUnsigned dividend, BigUnsigned divisor, BigUnsigned & quotient, BigUnsigned & remainder)
    {
        divMod(dividend, divisor, &quotient, &remainder);
    }
    static void divMod(WordType dividend, BigUnsigned divisor, BigUnsigned & quotient, BigUnsigned & remainder)
    {
        divMod(dividend, divisor, &quotient, &remainder);
    }
    static void divMod(BigUnsigned dividend, WordType divisor, BigUnsigned & quotient, BigUnsigned & remainder)
    {
        divMod(dividend, divisor, &quotient, &remainder);
    }
    friend BigUnsigned operator /(BigUnsigned dividend, BigUnsigned divisor)
    {
        BigUnsigned retval;
        divMod(dividend, divisor, &retval, NULL);
        return retval;
    }
    friend BigUnsigned operator /(WordType dividend, BigUnsigned divisor)
    {
        BigUnsigned retval;
        divMod(dividend, divisor, &retval, NULL);
        return retval;
    }
    friend BigUnsigned operator /(BigUnsigned dividend, WordType divisor)
    {
        BigUnsigned retval;
        divMod(dividend, divisor, &retval, NULL);
        return retval;
    }
    friend BigUnsigned operator %(BigUnsigned dividend, BigUnsigned divisor)
    {
        BigUnsigned retval;
        divMod(dividend, divisor, NULL, &retval);
        return retval;
    }
    friend BigUnsigned operator %(WordType dividend, BigUnsigned divisor)
    {
        BigUnsigned retval;
        divMod(dividend, divisor, NULL, &retval);
        return retval;
    }
    friend BigUnsigned operator %(BigUnsigned dividend, WordType divisor)
    {
        BigUnsigned retval;
        divMod(dividend, divisor, NULL, &retval);
        return retval;
    }
    const BigUnsigned & operator /=(BigUnsigned b)
    {
        return operator =(operator /(*this, b));
    }
    const BigUnsigned & operator /=(WordType b)
    {
        return operator =(operator /(*this, b));
    }
    const BigUnsigned & operator %=(BigUnsigned b)
    {
        return operator =(operator %(*this, b));
    }
    const BigUnsigned & operator %=(WordType b)
    {
        return operator =(operator %(*this, b));
    }
    explicit operator WordType() const
    {
        return data->words[0];
    }
    explicit operator bool() const
    {
        return data->size != 1 || data->words[0] != 0;
    }
    bool operator !() const
    {
        return data->size == 1 && data->words[0] == 0;
    }
    const BigUnsigned & operator ^=(BigUnsigned b)
    {
        onWrite();
        data->expand(b.data->size);
        for(size_t i = 0; i < b.data->size; i++)
        {
            data->words[i] ^= b.data->words[i];
        }
        normalize();
        return *this;
    }
    const BigUnsigned & operator ^=(WordType b)
    {
        onWrite();
        data->words[0] ^= b;
        return *this;
    }
    friend BigUnsigned operator ^(WordType a, BigUnsigned b)
    {
        b ^= a;
        return b;
    }
    friend BigUnsigned operator ^(BigUnsigned a, WordType b)
    {
        a ^= b;
        return a;
    }
    friend BigUnsigned operator ^(BigUnsigned a, BigUnsigned b)
    {
        a ^= b;
        return a;
    }
    const BigUnsigned & operator |=(BigUnsigned b)
    {
        onWrite();
        data->expand(b.data->size);
        for(size_t i = 0; i < b.data->size; i++)
        {
            data->words[i] |= b.data->words[i];
        }
        return *this;
    }
    const BigUnsigned & operator |=(WordType b)
    {
        onWrite();
        data->words[0] |= b;
        return *this;
    }
    friend BigUnsigned operator |(WordType a, BigUnsigned b)
    {
        b |= a;
        return b;
    }
    friend BigUnsigned operator |(BigUnsigned a, WordType b)
    {
        a |= b;
        return a;
    }
    friend BigUnsigned operator |(BigUnsigned a, BigUnsigned b)
    {
        a |= b;
        return a;
    }
    const BigUnsigned & operator &=(WordType b)
    {
        onWrite();
        data->size = 1;
        data->words[0] &= b;
        return *this;
    }
    const BigUnsigned & operator &=(BigUnsigned b)
    {
        onWrite();
        data->size = min(data->size, b.data->size);
        for(size_t i = 0; i < data->size; i++)
        {
            data->words[i] &= b.data->words[i];
        }
        normalize();
        return *this;
    }
    friend BigUnsigned operator &(WordType a, BigUnsigned b)
    {
        b &= a;
        return b;
    }
    friend BigUnsigned operator &(BigUnsigned a, WordType b)
    {
        a &= b;
        return a;
    }
    friend BigUnsigned operator &(BigUnsigned a, BigUnsigned b)
    {
        a &= b;
        return a;
    }
    const BigUnsigned & operator <<=(size_t shiftCount);
    const BigUnsigned & operator >>=(size_t shiftCount);
    friend BigUnsigned operator <<(BigUnsigned v, size_t shiftCount)
    {
        v <<= shiftCount;
        return v;
    }
    friend BigUnsigned operator >>(BigUnsigned v, size_t shiftCount)
    {
        v >>= shiftCount;
        return v;
    }
    friend BigUnsigned pow(BigUnsigned base, BigUnsigned exponent)
    {
        BigUnsigned retval(1);
        exponent.onWrite();
        size_t exponentWordIndex = 0, exponentWordBitIndex = 0;
        if((exponent.data->words[exponentWordIndex] & ((WordType)1 << exponentWordBitIndex)) != 0)
        {
            retval = base;
            exponent.data->words[exponentWordIndex] &= ~((WordType)1 << exponentWordBitIndex);
        }
        exponentWordBitIndex++;
        while(exponentWordIndex < exponent.data->size && exponent.data->words[exponent.data->size - 1] != 0)
        {
            base *= base;
            if((exponent.data->words[exponentWordIndex] & ((WordType)1 << exponentWordBitIndex)) != 0)
            {
                exponent.data->words[exponentWordIndex] &= ~((WordType)1 << exponentWordBitIndex);
                retval *= base;
            }
            if(++exponentWordBitIndex >= BitsPerWord)
            {
                exponentWordBitIndex = 0;
                exponentWordIndex++;
            }
        }
        return retval;
    }
    friend BigUnsigned powMod(BigUnsigned base, BigUnsigned exponent, BigUnsigned modulus)
    {
        if(modulus == (WordType)1)
            return BigUnsigned(0);
        base %= modulus;
        BigUnsigned retval(1);
        exponent.onWrite();
        size_t exponentWordIndex = 0, exponentWordBitIndex = 0;
        if((exponent.data->words[exponentWordIndex] & ((WordType)1 << exponentWordBitIndex)) != 0)
        {
            retval = base;
            exponent.data->words[exponentWordIndex] &= ~((WordType)1 << exponentWordBitIndex);
        }
        exponentWordBitIndex++;
        while(exponentWordIndex < exponent.data->size && exponent.data->words[exponent.data->size - 1] != 0)
        {
            base *= base;
            base %= modulus;
            if((exponent.data->words[exponentWordIndex] & ((WordType)1 << exponentWordBitIndex)) != 0)
            {
                exponent.data->words[exponentWordIndex] &= ~((WordType)1 << exponentWordBitIndex);
                retval *= base;
                retval %= modulus;
            }
            if(++exponentWordBitIndex >= BitsPerWord)
            {
                exponentWordBitIndex = 0;
                exponentWordIndex++;
            }
        }
        return retval;
    }
    friend ostream & operator <<(ostream & os, BigUnsigned v)
    {
        unsigned base;
        switch(os.flags() & ios::basefield)
        {
        case ios::oct:
            base = 8;
            break;
        case ios::hex:
            base = 16;
            break;
        default:
            base = 10;
            break;
        }
        return os << v.toString(base);
    }
    friend BigUnsigned gcd(BigUnsigned a, BigUnsigned b)
    {
        if(a == 0 || b == 0)
            return 0;
        if(a == 1 || b == 1)
            return 1;
        if(a < b)
            a.swap(b);
        for(;;)
        {
            BigUnsigned c = a % b;
            if(c == 0)
                return b;
            a = b;
            b = c;
        }
    }
    const BigUnsigned & operator ++()
    {
        return *this += (WordType)1;
    }
    BigUnsigned operator ++(int)
    {
        BigUnsigned retval = *this;
        *this += (WordType)1;
        return retval;
    }
    const BigUnsigned & operator --()
    {
        return *this -= (WordType)1;
    }
    BigUnsigned operator --(int)
    {
        BigUnsigned retval = *this;
        *this -= (WordType)1;
        return retval;
    }
};

inline BigUnsigned operator ""_bu(const char * str)
{
    return BigUnsigned::parse(str, true);
}

namespace std
{
template <>
inline void swap<BigUnsigned>(BigUnsigned & a, BigUnsigned & b)
{
    a.swap(b);
}
}

class BigInteger
{
    BigUnsigned value;
    bool negative;
    BigInteger(BigUnsigned value, bool negative)
        : value(value), negative(negative)
    {
        if(value == (WordType)0)
            negative = false;
    }
public:
    BigInteger(BigUnsigned value)
        : value(value), negative(false)
    {
    }
    BigInteger(WordType value)
        : value(value), negative(false)
    {
    }
    BigInteger(int value)
        : value((WordType)abs(value)), negative(value < 0)
    {
    }
    BigInteger(long value)
        : value((WordType)abs(value)), negative(value < 0)
    {
    }
    static BigInteger parse(string str, bool useOctal = false)
    {
        if(str.size() == 0)
            return BigInteger(BigUnsigned::parse(str, useOctal), false);
        if(str[0] == '-')
        {
            return BigInteger(BigUnsigned::parse(str.substr(1), useOctal), true);
        }
        else if(str[0] == '+')
        {
            return BigInteger(BigUnsigned::parse(str.substr(1), useOctal), false);
        }
        return BigInteger(BigUnsigned::parse(str, useOctal), false);
    }
    explicit operator BigUnsigned() const
    {
        if(value != (WordType)0 && negative)
            throw runtime_error("can't cast negative value to BigUnsigned in BigInteger::operator BigUnsigned()");
        return value;
    }
    explicit operator int() const
    {
        if(negative)
            return -(int)(WordType)value;
        return (int)(WordType)value;
    }
    const BigInteger & operator +=(BigInteger rt)
    {
        if(negative && rt.negative)
        {
            value += rt.value;
            return *this;
        }
        if(!negative && !rt.negative)
        {
            value += rt.value;
            return *this;
        }
        if(negative) // add negative to positive
        {
            negative = (value > rt.value);
            if(negative)
                value -= rt.value;
            else
                value = rt.value - value;
            return *this;
        }
        negative = (value < rt.value);
        if(negative)
            value = rt.value - value;
        else
            value -= rt.value;
        return *this;
    }
    BigInteger operator -() const
    {
        return BigInteger(value, !negative);
    }
    friend BigInteger abs(BigInteger v)
    {
        return BigInteger(v.value, false);
    }
    const BigInteger & operator -=(BigInteger rt)
    {
        return *this += -rt;
    }
    friend BigInteger operator +(BigInteger a, BigInteger b)
    {
        return a += b;
    }
    friend BigInteger operator -(BigInteger a, BigInteger b)
    {
        return a + -b;
    }
    friend BigInteger operator *(BigInteger a, BigInteger b)
    {
        return BigInteger(a.value * b.value, a.negative ? !b.negative : b.negative);
    }
    const BigInteger & operator *=(BigInteger b)
    {
        negative = (negative ? !b.negative : b.negative);
        value *= b.value;
        if(value == (WordType)0)
            negative = false;
        return *this;
    }
    friend BigInteger operator /(BigInteger a, BigInteger b)
    {
        return BigInteger(a.value / b.value, a.negative ? !b.negative : b.negative);
    }
    const BigInteger & operator /=(BigInteger b)
    {
        negative = (negative ? !b.negative : b.negative);
        value /= b.value;
        if(value == (WordType)0)
            negative = false;
        return *this;
    }
    friend BigInteger operator %(BigInteger a, BigInteger b)
    {
        return BigInteger(a.value % b.value, a.negative);
    }
    const BigInteger & operator %=(BigInteger b)
    {
        value %= b.value;
        if(value == (WordType)0)
            negative = false;
        return *this;
    }
    string toString(unsigned base = 10) const
    {
        if(negative)
            return "-" + value.toString(base);
        return value.toString(base);
    }
    friend ostream & operator <<(ostream & os, BigInteger v)
    {
        if(v.negative)
            os << "-";
        return os << v.value;
    }
    friend int compare(BigInteger a, BigInteger b)
    {
        if(a.value == (WordType)0 && b.value == (WordType)0)
            return 0;
        if(a.negative && !b.negative)
            return -1;
        if(!a.negative && b.negative)
            return 1;
        if(a.negative)
            return -compare(a.value, b.value);
        return compare(a.value, b.value);
    }
    friend bool operator <(BigInteger a, BigInteger b)
    {
        return compare(a, b) < 0;
    }
    friend bool operator <=(BigInteger a, BigInteger b)
    {
        return compare(a, b) <= 0;
    }
    friend bool operator >(BigInteger a, BigInteger b)
    {
        return compare(a, b) > 0;
    }
    friend bool operator >=(BigInteger a, BigInteger b)
    {
        return compare(a, b) >= 0;
    }
    friend bool operator ==(BigInteger a, BigInteger b)
    {
        if(a.value == (WordType)0 && b.value == (WordType)0)
            return true;
        return a.negative == b.negative && a.value == b.value;
    }
    friend bool operator !=(BigInteger a, BigInteger b)
    {
        return !operator ==(a, b);
    }
    friend BigInteger pow(BigInteger base, BigInteger exponent)
    {
        if(exponent < 0)
            throw runtime_error("exponent is less than zero in pow(BigInteger, BigInteger)");
        return BigInteger(pow(base.value, exponent.value), base.negative && (exponent.value & (WordType)1) != 0);
    }
    friend BigInteger powMod(BigInteger base, BigInteger exponent, BigInteger modulus)
    {
        if(exponent < 0)
            throw runtime_error("exponent is less than zero in powMod(BigInteger, BigInteger, BigInteger)");
        return BigInteger(powMod(base.value, exponent.value, modulus.value), base.negative && (exponent.value & (WordType)1) != 0);
    }
    friend BigInteger gcd(BigInteger a, BigInteger b)
    {
        return BigInteger(gcd(a.value, b.value), false);
    }
    const BigInteger & operator ++()
    {
        return *this += (WordType)1;
    }
    BigInteger operator ++(int)
    {
        BigInteger retval = *this;
        *this += (WordType)1;
        return retval;
    }
    const BigInteger & operator --()
    {
        return *this -= (WordType)1;
    }
    BigInteger operator --(int)
    {
        BigInteger retval = *this;
        *this -= (WordType)1;
        return retval;
    }
};

inline BigInteger operator ""_bi(const char * str)
{
    return BigInteger::parse(str, true);
}

#endif
