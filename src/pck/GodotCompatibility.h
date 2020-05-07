// Things to help make the taken Godot code compile
#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

using String = std::string;

template<class T1, class T2>
using Map = std::map<T1, T2>;

template<class T1>
using Set = std::set<T1>;

template<class T1>
using Vector = std::vector<T1>;


#define _FORCE_INLINE_
