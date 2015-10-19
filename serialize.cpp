/*
    Copyright (C) 2014 by Project Tox <https://tox.im>

    This file is part of qTox, a Qt-based graphical interface for Tox.

    This program is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the COPYING file for more details.
*/

#include <iostream>
#include <cassert>
#include <algorithm>
#include "serialize.h"
#include "crypto.h"
#include "pathhash.h"

using namespace std;

std::vector<char> doubleToData(double num)
{
    union
    {
        char tab[8];
        double n;
    } castUnion;
    //char n[8];
    //*((double*) n) = num;

    castUnion.n=num;
    return std::vector<char>(castUnion.tab,castUnion.tab+8);
}

std::vector<char> floatToData(float num)
{
    union
    {
        char tab[4];
        float n;
    } castUnion;

    castUnion.n=num;
    return std::vector<char>(castUnion.tab,castUnion.tab+4);
}

float dataToFloat(std::vector<char> data)
{
    union
    {
        char tab[4];
        float n;
    } castUnion;

    castUnion.tab[0]=data.data()[0];
    castUnion.tab[1]=data.data()[1];
    castUnion.tab[2]=data.data()[2];
    castUnion.tab[3]=data.data()[3];
    return castUnion.n;
}

// Converts a string into PNet string data
std::vector<char> stringToData(const std::string& str)
{
    std::vector<char> data(8,0);
    data.reserve(8+str.size());
    // Write the size in a Uint of variable lenght (8-32 bits)
    int i=0;
    uint num1 = (uint)str.size();
    while (num1 >= 0x80)
    {
        data[i] = (unsigned char)(num1 | 0x80); i++;
        num1 = num1 >> 7;
    }
    data[i]=num1;
    data.resize(i+1);
    data.insert(end(data), begin(str), end(str));
    return data;
}

std::vector<char> stringToData(std::string&& str)
{
    std::vector<char> data(8,0);
    data.reserve(8+str.size());
    // Write the size in a Uint of variable lenght (8-32 bits)
    int i=0;
    uint num1 = (uint)str.size();
    while (num1 >= 0x80)
    {
        data[i] = (unsigned char)(num1 | 0x80); i++;
        num1 = num1 >> 7;
    }
    data[i]=num1;
    data.resize(i+1);
    data.insert(end(data), make_move_iterator(begin(str)), make_move_iterator(end(str)));
    return data;
}

std::string dataToString(std::vector<char>::const_iterator& data)
{
    // Variable UInt32
    unsigned char num3;
    int num = 0;
    int num2 = 0;
    int i=0;
    do
    {
        num3 = data[i]; i++;
        num |= (num3 & 0x7f) << num2;
        num2 += 7;
    } while ((num3 & 0x80) != 0);
    unsigned int strlen = (uint) num;

    if (!strlen)
        return std::string{};

    auto v = std::vector<char>(data+i, data+strlen+i); // Remove the strlen and truncate
    data += i+strlen;
    return std::string(begin(v), end(v));
}

float dataToRangedSingle(float min, float max, int numberOfBits, std::vector<char> data)
{
    uint endvalue=0;
    uint value=0;
    if (numberOfBits <= 8)
    {
        endvalue = (uint8_t)data[0];
        goto done;
    }
    value = (uint8_t)data[0];
    numberOfBits -= 8;
    if (numberOfBits <= 8)
    {
        endvalue = (value | ((uint) ((uint8_t)data[1]) << 8));
        goto done;
    }
    value |= (uint) (((uint8_t)data[1]) << 8);
    numberOfBits -= 8;
    if (numberOfBits <= 8)
    {
        uint num2 = (uint) (((uint8_t)data[2]) << 0x10);
        endvalue = (value | num2);
        goto done;
    }
    value |= (uint) (((uint8_t)data[2]) << 0x10);
    numberOfBits -= 8;
    endvalue =  (value | ((uint) (((uint8_t)data[3]) << 0x18)));
    goto done;

    done:

    float num = max - min;
    int num2 = (((int) 1) << numberOfBits) - 1;
    float num3 = endvalue;
    float num4 = num3 / ((float) num2);
    return (min + (num4 * num));
}

std::vector<char> rangedSingleToData(float value, float min, float max, int numberOfBits)
{
    std::vector<char> data;
    float num = max - min;
    float num2 = (value - min) / num;
    int num3 = (((int) 1) << numberOfBits) - 1;
    uint source = num3 * num2;

    if (numberOfBits <= 8)
    {
        data.insert(end(data), (unsigned char)source);
        return data;
    }
    data.insert(end(data), (unsigned char)source);
    numberOfBits -= 8;
    if (numberOfBits <= 8)
    {
        data.insert(end(data), (unsigned char)source>>8);
        return data;
    }
    data.insert(end(data), (unsigned char)source>>8);
    numberOfBits -= 8;
    if (numberOfBits <= 8)
    {
        data.insert(end(data), (unsigned char)source>>16);
        return data;
    }
    data.insert(end(data), (unsigned char)source>>16);
    data.insert(end(data), (unsigned char)source>>24);

    return data;
}

uint8_t dataToUint8(std::vector<char>::const_iterator& data)
{
    uint8_t r = (uint8_t)data[0];
    data += sizeof(r);
    return r;
}

uint16_t dataToUint16(std::vector<char>::const_iterator& data)
{
    uint16_t r = ((uint16_t)(uint8_t)data[0])
            +(((uint16_t)(uint8_t)data[1])<<8);
    data += sizeof(r);
    return r;
}

uint32_t dataToUint32(std::vector<char>::const_iterator& data)
{
    uint32_t r = ((uint32_t)(uint8_t)data[0])
            +(((uint32_t)(uint8_t)data[1])<<8)
            +(((uint32_t)(uint8_t)data[2])<<16)
            +(((uint32_t)(uint8_t)data[3])<<24);
    data += sizeof(r);
    return r;
}

uint64_t dataToUint64(std::vector<char>::const_iterator& data)
{
    uint64_t r = ((uint64_t)(uint8_t)data[0])
            +(((uint64_t)(uint8_t)data[1])<<8)
            +(((uint64_t)(uint8_t)data[2])<<16)
            +(((uint64_t)(uint8_t)data[3])<<24)
            +(((uint64_t)(uint8_t)data[4])<<32)
            +(((uint64_t)(uint8_t)data[5])<<40)
            +(((uint64_t)(uint8_t)data[6])<<48)
            +(((uint64_t)(uint8_t)data[7])<<56);
    data += sizeof(r);
    return r;
}

size_t dataToVUint(std::vector<char>::const_iterator& data)
{
    unsigned char num3;
    size_t num = 0;
    int num2 = 0;
    int i=0;
    do
    {
        num3 = data[i]; i++;
        num |= (num3 & 0x7f) << num2;
        num2 += 7;
    } while ((num3 & 0x80) != 0);
    data += i;
    return num;
}

unsigned getVUint32Size(std::vector<char> data)
{
    unsigned lensize=0;
    {
        unsigned char num3;
        do {
            num3 = data[lensize];
            lensize++;
        } while ((num3 & 0x80) != 0);
    }
    return lensize;
}

std::vector<char> uint8ToData(uint8_t num)
{
    std::vector<char> data(1,0);
    data[0] = (uint8_t)num;
    return data;
}

std::vector<char> uint16ToData(uint16_t num)
{
    std::vector<char> data(2,0);
    data[0] = (uint8_t)(num & 0xFF);
    data[1] = (uint8_t)((num>>8) & 0xFF);
    return data;
}

std::vector<char> uint32ToData(uint32_t num)
{
    std::vector<char> data(4,0);
    data[0] = (uint8_t)(num & 0xFF);
    data[1] = (uint8_t)((num>>8) & 0xFF);
    data[2] = (uint8_t)((num>>16) & 0xFF);
    data[3] = (uint8_t)((num>>24) & 0xFF);
    return data;
}

std::vector<char> uint64ToData(uint64_t num)
{
    std::vector<char> data(8,0);
    data[0] = (uint8_t)(num & 0xFF);
    data[1] = (uint8_t)((num>>8) & 0xFF);
    data[2] = (uint8_t)((num>>16) & 0xFF);
    data[3] = (uint8_t)((num>>24) & 0xFF);
    data[4] = (uint8_t)((num>>32) & 0xFF);
    data[5] = (uint8_t)((num>>40) & 0xFF);
    data[6] = (uint8_t)((num>>48) & 0xFF);
    data[7] = (uint8_t)((num>>56) & 0xFF);
    return data;
}

void uint64ToData(std::vector<char> &dest, uint64_t num)
{
    char* alias = (char*)&num;
    size_t size = dest.size();
    dest.resize(size+sizeof(num));
    dest[size+0] = alias[0];
    dest[size+1] = alias[1];
    dest[size+2] = alias[2];
    dest[size+3] = alias[3];
    dest[size+4] = alias[4];
    dest[size+5] = alias[5];
    dest[size+6] = alias[6];
    dest[size+7] = alias[7];
}

std::vector<char> vuintToData(size_t num)
{
    std::vector<char> data(sizeof(size_t),0);
    // Write the size in a Uint of variable lenght (8-32 bits)
    int i=0;
    while (num >= 0x80)
    {
        data[i] = (unsigned char)(num | 0x80); i++;
        num = num >> 7;
    }
    data[i]=num;
    data.resize(i+1);
    return data;
}

std::vector<std::vector<char>> dataToDatavec(std::vector<char>::const_iterator& data)
{
    std::vector<std::vector<char>> datavec;

    // Variable UInt32 number of elements
    unsigned char num3;
    int num = 0;
    int num2 = 0;
    int i=0;
    do
    {
        num3 = data[i]; i++;
        num |= (num3 & 0x7f) << num2;
        num2 += 7;
    } while ((num3 & 0x80) != 0);
    unsigned int count = (uint) num;
    data += i;

    if (!count)
        return datavec;

    datavec.resize(count);
    for (unsigned int veci=0; veci<count; ++veci)
    {
        // Variable UInt32 size of element
        unsigned char num3;
        int num = 0;
        int num2 = 0;
        int i=0;
        do
        {
            num3 = data[i]; i++;
            num |= (num3 & 0x7f) << num2;
            num2 += 7;
        } while ((num3 & 0x80) != 0);
        data += i;
        unsigned int size = (uint) num;
        auto& vec = datavec[veci];
        vec.insert(end(vec), data, data+size);
        data+=size;
    }
    return datavec;
}

std::vector<char> datavecToData(const std::vector<std::vector<char>>& datavec)
{
    std::vector<char> data(8,0);
    // Write the number of elements in a Uint of variable lenght (8-32 bits)
    int i=0;
    uint num1 = (uint)datavec.size();
    while (num1 >= 0x80)
    {
        data[i] = (unsigned char)(num1 | 0x80); i++;
        num1 = num1 >> 7;
    }
    data[i]=num1;
    data.resize(i+1);
    for (const auto& e : datavec)
    {
        std::vector<char> esize(8,0);
        // Write the size of one element in a Uint of variable lenght (8-32 bits)
        i=0;
        num1 = (uint)e.size();
        while (num1 >= 0x80)
        {
            esize[i] = (unsigned char)(num1 | 0x80); i++;
            num1 = num1 >> 7;
        }
        esize[i]=num1;
        esize.resize(i+1);
        data.insert(end(data), begin(esize), end(esize));
        data.insert(end(data), begin(e), end(e));
    }
    return data;
}

void vectorAppend(std::vector<char> &dst, const std::vector<char> &src)
{
    dst.insert(end(dst), begin(src), end(src));
}

void vectorAppend(std::vector<char>& dst, std::vector<char>&& src)
{
    dst.reserve(dst.size()+src.size());
    dst.insert(dst.end(), make_move_iterator(src.begin()),
             make_move_iterator(src.end()));
    src.shrink_to_fit();
}

template<> vector<char> serialize<uint8_t>(uint8_t arg) {return uint8ToData(arg);}
template<> vector<char> serialize<uint16_t>(uint16_t arg) {return uint16ToData(arg);}
template<> vector<char> serialize<uint32_t>(uint32_t arg) {return uint32ToData(arg);}
template<> vector<char> serialize<uint64_t>(uint64_t arg) {return uint64ToData(arg);}
template<> vector<char> serialize<string>(string arg) {return stringToData(move(arg));}
template<> vector<char> serialize<vector<vector<char>>>(vector<vector<char>> arg) {return datavecToData(arg);}
template<> vector<char> serialize<PublicKey>(PublicKey arg) {return vector<char>(&arg[0],&arg[0]+sizeof(arg));}
template<> vector<char> serialize<PathHash>(PathHash arg) {return arg.serialize();}

template<> uint8_t deserializeConsume<uint8_t>(vector<char>::const_iterator& data) {return dataToUint8(data);}
template<> uint16_t deserializeConsume<uint16_t>(vector<char>::const_iterator& data) {return dataToUint16(data);}
template<> uint32_t deserializeConsume<uint32_t>(vector<char>::const_iterator& data) {return dataToUint32(data);}
template<> uint64_t deserializeConsume<uint64_t>(vector<char>::const_iterator& data) {return dataToUint64(data);}
template<> string deserializeConsume<string>(vector<char>::const_iterator& data) {return dataToString(data);}
template<> vector<vector<char>> deserializeConsume<vector<vector<char>>>(vector<char>::const_iterator& data)
{
    return dataToDatavec(data);
}
template<> PublicKey deserializeConsume<PublicKey>(vector<char>::const_iterator& data)
{
    PublicKey r;
    std::copy_n(data, sizeof(r), begin(r));
    data+=sizeof(PublicKey);
    return r;
}
template<> PathHash deserializeConsume<PathHash>(vector<char>::const_iterator& data)
{
    PathHash ph((uint8_t*)&*data);
    data+=sizeof(PathHash);
    return ph;
}
template<> void serializeAppend<uint8_t>(std::vector<char>& dst, uint8_t arg)
{
    dst.push_back(arg);
}
template<> void serializeAppend<uint32_t>(std::vector<char>& dst, uint32_t arg)
{
    char* alias = (char*)&arg;
    dst.insert(end(dst), alias, alias+4);
}
template<> void serializeAppend<uint64_t>(std::vector<char>& dst, uint64_t arg)
{
    char* alias = (char*)&arg;
    dst.insert(end(dst), alias, alias+8);
}
template<> void serializeAppend<string>(std::vector<char>& dst, string arg)
{
    auto v = stringToData(arg);
    dst.insert(end(dst), make_move_iterator(begin(v)), make_move_iterator(end(v)));
}
