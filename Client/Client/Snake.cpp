#include "Snake.h"

Snake::Snake(sf::Vector2f position_, int length_, float fpsConstant_, sf::Color colour, int ID) :
	ID(ID),
	slow(100.0f),
	fast(slow * 2.0f),
	maxAngle(0.065f),
	rotationSpeed(3.0f),
	length(length_),
	speed(slow),
	//alive(true),
	radius(22.0f),
	//collisionZoneRadius((length - 1)* speed / fpsConstant_ + radius),
	colour(colour),
	eyeAngle(0.75f),
	eyeDistance(radius * 0.625f),
	pupilDistance(radius * 0.2136363f),
	eyeRadius(radius * 0.35f),
	pupilRadius(radius * 0.2136363f)
{
	// Initializing direction and velocity variables to avoid division by zero bug when normalizing the vector on start.
	direction = normalize(sf::Vector2f(-1, -1));
	velocity = normalize(sf::Vector2f(-1, -1));		

	// Setting psotions of all segments of the snake. 
	// Populating an array of sf::CircleShapes to render the snake.
	for (int i = 0; i < length; i++)
	{
		snake.push_back(sf::Vector2f(position_.x, position_.y));
		sf::CircleShape circleShape;
		if (i != 0)
			circleShape.setPointCount(16);
		circleShape.setPosition(snake[i]);
		circleShape.setRadius(radius);
		circleShape.setOrigin(radius, radius);
		circleShape.setFillColor(colour);
		snakeCircleShapes.push_back(circleShape);
	}

	// EYES
	eye[0].setFillColor(sf::Color::White);
	eye[0].setRadius(eyeRadius);
	eye[0].setOrigin(eyeRadius, eyeRadius);
	eye[1].setFillColor(sf::Color::White);
	eye[1].setRadius(eyeRadius);
	eye[1].setOrigin(eyeRadius, eyeRadius);
	pupil[0].setFillColor(sf::Color::Black);
	pupil[0].setRadius(pupilRadius);
	pupil[0].setOrigin(pupilRadius, pupilRadius);
	pupil[1].setFillColor(sf::Color::Black);
	pupil[1].setRadius(pupilRadius);
	pupil[1].setOrigin(pupilRadius, pupilRadius);

	// Populating the lastConfirmedPositions deque with 3 default structs to avoid vector out of range bug on start.
	ConfirmedState confirmedState;
	confirmedState.position.x = 0;
	confirmedState.position.y = 0;
	confirmedState.direction.x = 0;
	confirmedState.direction.y = 0;
	confirmedState.timeStamp = 0;
	confirmedState.boosting = false;
	for (int i = 0; i < 3; i++)
	{
		lastConfirmedPositions.push_back(confirmedState);
	}
}

Snake::~Snake()
{
}

//void Snake::die()
//{
//	alive = false;
//	colour = sf::Color(100, 100, 100);
//	for (int i = 0; i < snakeCircleShapes.size(); i++)
//	{
//		snakeCircleShapes[i].setFillColor(colour);
//	}
//}

void Snake::addConfirmedPosition(ConfirmedState confirmedState)
{
	// Handling unordered packets.
	if (confirmedState.timeStamp > lastConfirmedPositions.at(0).timeStamp)
	{
		lastConfirmedPositions.push_front(confirmedState);
		lastConfirmedPositions.pop_back();
	}
	else if (confirmedState.timeStamp <= lastConfirmedPositions.at(0).timeStamp && confirmedState.timeStamp > lastConfirmedPositions.at(1).timeStamp)
	{
		lastConfirmedPositions.insert(lastConfirmedPositions.begin() + 1, confirmedState);
		lastConfirmedPositions.pop_back();
	}
	else if (confirmedState.timeStamp <= lastConfirmedPositions.at(1).timeStamp && confirmedState.timeStamp > lastConfirmedPositions.at(2).timeStamp)
	{
		lastConfirmedPositions.insert(lastConfirmedPositions.begin() + 2, confirmedState);
		lastConfirmedPositions.pop_back();
	}
}

void Snake::boost(bool boosting_)
{
	// Setting speed of snake.
	boosting = boosting_;
	if (boosting)
	{
		speed = fast;
	}
	else
	{
		speed = slow;
	}
}

void Snake::compress()
{
	// Shifting the snake tail forward. This algorithm only works if the fast speed is double the slow speed and the game is working with a fixed time step.
	lastSnake = snake;
	sf::Vector2f shift = (lastSnake[0] - lastSnake[1]) / 2.0f;
	snake[1] += shift;
	for (int i = 2; i < (int)lastSnake.size(); i++)
	{
		snake[i] = lastSnake[i - 1];
	}
}

// For local snake.
void Snake::update(float deltaTime)
{
	// Saving the last velocity.
	sf::Vector2f lastVelocity = velocity;

	// Calculating the new velocity.
	velocity = direction * speed * deltaTime;

	// Calculating angle between the last direction and the direction it wants to go.
	float lastAngle = atan2(lastVelocity.y, lastVelocity.x);
	float angle = atan2(velocity.y, velocity.x) - lastAngle;
	if (angle < -PI)
		angle = PI - (angle - PI);
	if (angle > PI)
		angle = -(PI - (angle - PI));

	// If the angle is greater than the specified maximum angle, rotate the velocity vector towards the mouse by the rotation speed.
	if (angle * angle > maxAngle * maxAngle)
	{
		velocity = normalize(lastVelocity);
		velocity *= speed * deltaTime;

		float theta = (angle > 0) ? rotationSpeed : -rotationSpeed;
		theta *= deltaTime;

		velocity = rotate(velocity, theta);
	}

	// Saving the positions of the last snake. This is used for the compress algorithm.
	lastSnake = snake;

	// Move the snakes head.
	snake.front() += velocity;

	// Move the tail to follow the tail segment in front of it.
	for (int i = 1; i < (int)snake.size(); i++)
	{
		snake[i] = lastSnake[i - 1];
	}

	// If the snake is moving fast, run the compressing algorithm so that the tail keeps up with the head.
	if (speed == fast)
	{
		compress();
	}

	// EYES
	// Calculating the positions of the eye circle shapes.
	sf::Vector2f normalizedVelocity = normalize(velocity);
	eyePosition[0] = rotate(normalizedVelocity * eyeDistance, -eyeAngle);
	eyePosition[1] = rotate(normalizedVelocity * eyeDistance, eyeAngle);
	pupilPosition[0] = direction * pupilDistance;
	pupilPosition[1] = direction * pupilDistance;
}

float lerp(float a, float b, float time)
{
	return a + (b - a) * time;
}

void Snake::predictionUpdate(float deltaTime, float time)
{
	// Velocity is only used for calculating the eyes position here. Consider re-naming...
	// Snake head is set with prediction from last confirmed positions.
	sf::Vector2f lastVelocity = velocity;
	velocity = direction * speed * deltaTime;
	
	// Calculating angle between the last direction and the direction it wants to go.
	float lastAngle = atan2(lastVelocity.y, lastVelocity.x);
	float angle = atan2(velocity.y, velocity.x) - lastAngle;
	if (angle < -PI)
		angle = PI - (angle - PI);
	if (angle > PI)
		angle = -(PI - (angle - PI));

	// If the angle is greater than the specified maximum angle, rotate the velocity vector towards the mouse by the rotation speed.
	if (angle * angle > maxAngle * maxAngle)
	{
		velocity = normalize(lastVelocity);
		velocity *= speed * deltaTime;

		float theta = (angle > 0) ? rotationSpeed : -rotationSpeed;
		theta *= deltaTime;

		velocity = rotate(velocity, theta);
	}

	// Saving the positions of the last snake. This is used for the compress algorithm.
	lastSnake = snake;

	// PREDICTION

	// Saving the last predicted position.
	lastPredictionPosition = predictionPosition;

	// t = time since last confirmed position
	float t = time - lastConfirmedPositions.front().timeStamp;
	
	// Getting the last two positions in the deque.
	ConfirmedState msg0 = lastConfirmedPositions.at(0);
	ConfirmedState msg1 = lastConfirmedPositions.at(1);

	// Calculating velocity based on the last two confirmed positions.
	// Speed = Distance / Time.
	float linearVelocityX = (msg0.position.x - msg1.position.x) / (msg0.timeStamp - msg1.timeStamp);
	float linearVelocityY = (msg0.position.y - msg1.position.y) / (msg0.timeStamp - msg1.timeStamp);

	// Adding the velocity to the most recent confirmed position multiplied by the time since the last packet was received to get the predicted position the snake head is.
	predictionPosition.x = msg0.position.x + (t * linearVelocityX);
	predictionPosition.y = msg0.position.y + (t * linearVelocityY);

	// INTERPOLATION
	// Interpolating between the last predicted position and this predicted position.
	// Aiming for a point half way between the two. This smooths the movement of the snake when there are sudden movements from the player.
	snake.front().x = lerp(lastPredictionPosition.x, predictionPosition.x, 0.5);		
	snake.front().y = lerp(lastPredictionPosition.y, predictionPosition.y, 0.5);


	// Move the tail to follow the tail segment in front of it.
	for (int i = 1; i < (int)snake.size(); i++)
	{
		snake[i] = lastSnake[i - 1];
	}

	// If the enemy snake is boosting, run the compressing algorithm so that the tail keeps up with the head.
	if (boosting) 
	{
		if (speed == fast)
		{
			compress();
		}
	}

	// EYES
		// Calculating the positions of the eye circle shapes.
	sf::Vector2f normalizedVelocity = normalize(velocity);
	eyePosition[0] = rotate(normalizedVelocity * eyeDistance, -eyeAngle);
	eyePosition[1] = rotate(normalizedVelocity * eyeDistance, eyeAngle);
	pupilPosition[0] = direction * pupilDistance;
	pupilPosition[1] = direction * pupilDistance;
}

void Snake::render(sf::RenderWindow* window, sf::Vector2f offset, sf::Vector2f center)
{
	// SNAKE
	for (int i = 0; i < (int)snake.size(); i++)
	{
		//if (i % 2 == 0 || i == (int)snake.size() - 1)		
		{
			snakeCircleShapes[i].setPosition(snake[i] - offset + center);
			window->draw(snakeCircleShapes[i]);
		}
	}

	// EYES
	for (int i = 0; i < 2; i++)
	{
		eye[i].setPosition(snake[0] + eyePosition[i] - offset + center);
		window->draw(eye[i]);
		pupil[i].setPosition(snake[0] + eyePosition[i] + pupilPosition[i] - offset + center);
		window->draw(pupil[i]);
	}
}