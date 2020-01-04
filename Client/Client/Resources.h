#pragma once
#include <SFML/Graphics.hpp>
#include <cassert>


// Returns the magnitude of a given vector.
static float magnitude(sf::Vector2f vector)
{
	return sqrt(pow(vector.x, 2) + pow(vector.y, 2));
}

// Returns the normalized vector of the given vector.
static sf::Vector2f normalize(sf::Vector2f vector)
{
	// assert() handles division by zero.
	assert(vector != sf::Vector2f(0, 0));
	vector /= magnitude(vector);
	return vector;
}

// Returns a rotated vector from the given vector and an angle.
static sf::Vector2f rotate(sf::Vector2f vector, float angle)
{
	float rotatedX = vector.x * cos(angle) + vector.y * -sin(angle);
	float rotatedY = vector.x * sin(angle) + vector.y * cos(angle);
	vector.x = rotatedX;
	vector.y = rotatedY;
	return vector;
}
