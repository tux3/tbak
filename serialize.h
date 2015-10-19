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


#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <cstdint>
#include <vector>
#include <string>

/// Most of those functions are unsafe unless otherwise specified
/// Do not use them on untrusted data (e.g. check a signature first)

std::vector<char> doubleToData(double num);
std::vector<char> floatToData(float num);
float dataToFloat(std::vector<char> data);
std::vector<char> stringToData(const std::string &str);
std::vector<char> stringToData(std::string&& str);
std::string dataToString(std::vector<char>::const_iterator& data);
float dataToRangedSingle(float min, float max, int numberOfBits, std::vector<char> data);
std::vector<char> rangedSingleToData(float value, float min, float max, int numberOfBits);
uint8_t dataToUint8(std::vector<char>::const_iterator& data);
uint16_t dataToUint16(std::vector<char>::const_iterator& data);
uint32_t dataToUint32(std::vector<char>::const_iterator& data);
uint64_t dataToUint64(std::vector<char>::const_iterator& data);
size_t dataToVUint(std::vector<char>::const_iterator& data);
unsigned getVUint32Size(std::vector<char> data);
std::vector<char> uint8ToData(uint8_t num);
std::vector<char> uint16ToData(uint16_t num);
std::vector<char> uint32ToData(uint32_t num);
std::vector<char> uint64ToData(uint64_t num);
void uint64ToData(std::vector<char>& dest, uint64_t num);
std::vector<char> vuintToData(size_t num);
std::vector<std::vector<char>> dataToDatavec(std::vector<char>::const_iterator& data);
std::vector<char> datavecToData(const std::vector<std::vector<char>>& datavec);

/// Some helpers to cope with the stdlib's hatred of simple things

void vectorAppend(std::vector<char>& dst, const std::vector<char>& src);
void vectorAppend(std::vector<char>& dst, std::vector<char>&& src);

/// Templates wrappers for a clean interface

template<typename T> T deserializeConsume(std::vector<char>::const_iterator& data);
template<typename T> std::vector<char> serialize(T arg);
template<> std::vector<char> serialize(std::string&& arg);
template<typename T> void serializeAppend(std::vector<char>& dst, T arg)
{
    vectorAppend(dst, ::serialize(arg));
}


#endif // SERIALIZE_H
