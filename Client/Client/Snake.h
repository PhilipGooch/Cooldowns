#pragma once
#include "Resources.h"
#include <deque>

#define PI 3.1415926f

// Struct containing information about where this snake was and it's direction vector at a specific time.
struct ConfirmedState
{
	sf::Vector2f position;
	sf::Vector2f direction;
	int timeStamp;
	bool boosting;
};

class Snake
{
public:

	Snake(sf::Vector2f position_, int length_, float fpsConstant_, sf::Color colour, int ID);
	~Snake();

	// Getters.
	sf::Vector2f getPosition() { return snake[0]; }
	sf::Vector2f getDirection() { return direction; }
	sf::Color getColour() { return colour; }
	inline float getRadius() { return radius; }
	inline std::vector<sf::Vector2f> getSnake() { return snake; };
	inline bool getBoosting() { return boosting; }
	//inline bool isAlive() { return alive; }
	//inline float getCollisionZoneRadius() { return collisionZoneRadius; }

	// Setters.
	inline void setDirection(sf::Vector2f direction_) { direction = direction_; }
	void setSnakeHead(sf::Vector2f position) { snake[0] = position; }

	//void die();

	// Adds a confirmed state struct to the deque of confirmed states. Used for prediction.
	void addConfirmedPosition(ConfirmedState confirmedState);

	// Sets the snake's speed depending on the boosting variable passed in.
	void boost(bool boosting);

	// When changing speed from slow to fast, the snake's tail needs to run an algorithm to "compress" or catch up with the head. 
	// This prevents stretching of the tail.
	void compress();

	// Update function for the local snake.
	void update(float deltaTime);

	// Update function that uses prediction for all other client's snakes.
	void predictionUpdate(float deltaTime, float time);

	// Draws this snake.
	void render(sf::RenderWindow* window, sf::Vector2f offset, sf::Vector2f center);

private:
	// SNAKE
	const int ID;

	// Speed of the snake.
	const float slow;
	const float fast;					// <--- Must be set to double slow. This allows for use of much cheaper algorithm for snake tail.
	float speed;
	bool boosting;
	
	// Indicates the maximum angle between the snake's direction and the direction of the mouse where the snakes direction will snap to the mouse's direction.
	// If it the angle is greater than this, then the snake will move slowly towards that direction.
	const float maxAngle;	

	// How fast the snake will turn to get to mouse direction vector.
	const float rotationSpeed;

	// How many segments the snake's tail has.
	int length;
	//bool alive;

	// Radius of each snake segment.
	float radius;

	// Used for initial check to see if a collision is possible. This is the size of the snake stretched out. 
	// This is an optimization. no need to check each snake segment if there is no possibility of hitting any of them. Can do in one check.
	//float collisionZoneRadius;

	sf::Vector2f direction;
	sf::Vector2f velocity;
	std::vector<sf::Vector2f> snake;

	// Used for snake compression algorithm.
	std::vector<sf::Vector2f> lastSnake;

	// Confirmed position structs with network data. Used for prediction.
	std::deque<ConfirmedState> lastConfirmedPositions;

	// Vector storing predicted position of snake.
	sf::Vector2f predictionPosition;
	// Vector storing the last predicted position of snake. Used for interpolation.
	sf::Vector2f lastPredictionPosition;
	
	// RENDERING
	sf::Color colour;
	std::vector<sf::CircleShape> snakeCircleShapes;

	// EYES
	sf::CircleShape eye[2];
	sf::CircleShape pupil[2];
	sf::Vector2f eyePosition[2];
	sf::Vector2f pupilPosition[2];

	// Variables used to calculate the positions of the eyes and pupils of the snakes. 
	const float eyeAngle;
	const float eyeDistance;
	const float pupilDistance;
	const float eyeRadius;
	const float pupilRadius;
};



