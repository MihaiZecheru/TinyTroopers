#include "common/Vec2.hpp"

#include <cmath>

Vec2::Vec2() : x(0.0F), y(0.0F) {}

Vec2::Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}

float Vec2::Length() const {
    return std::sqrt((x * x) + (y * y));
}

Vec2 Vec2::Normalized() const {
    const float length = Length();
    if (length <= 0.0001F) {
        return Vec2();
    }
    return Vec2(x / length, y / length);
}

Vec2 Vec2::operator+(const Vec2& other) const {
    return Vec2(x + other.x, y + other.y);
}

Vec2 Vec2::operator-(const Vec2& other) const {
    return Vec2(x - other.x, y - other.y);
}

Vec2 Vec2::operator*(float scalar) const {
    return Vec2(x * scalar, y * scalar);
}

Vec2& Vec2::operator+=(const Vec2& other) {
    x += other.x;
    y += other.y;
    return *this;
}
