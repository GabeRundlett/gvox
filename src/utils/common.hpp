#pragma once

#include <concepts>
#include "gvox/core.h"

template <typename T>
concept GvoxOffsetOrExtentType =
    std::same_as<T, GvoxOffset> ||
    std::same_as<T, GvoxOffsetMut> ||
    std::same_as<T, GvoxExtent> ||
    std::same_as<T, GvoxExtentMut>;

template <typename T>
concept GvoxOffsetType =
    std::same_as<T, GvoxOffset> ||
    std::same_as<T, GvoxOffsetMut>;

template <typename T>
concept GvoxExtentType =
    std::same_as<T, GvoxExtent> ||
    std::same_as<T, GvoxExtentMut>;
