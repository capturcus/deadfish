#pragma once
#include <cstdint>
#include <cmath>

const int FRAME_TIME = 50; // 20 fps
const int CIVILIAN_TIME = 40;
const int MAX_CIVILIANS = 100;
const float KILL_DISTANCE = 1.f;
const float INSTA_KILL_DISTANCE = 0.61f;
const int CIVILIAN_PENALTY = -1;
const int KILL_REWARD = 5;
const uint64_t ROUND_LENGTH = 10 * 60 * 20; // 10 minutes

const float TURN_SPEED = 4.f;
const float WALK_SPEED = 1.f;
const float RUN_SPEED = 2.f;
const float CLOSE = 0.05f;
const float ANGULAR_CLOSE = 0.1f;
const float TARGET_OFFSET = 0.3f;
const float RANDOM_OFFSET = 0.15f;
const int DEATH_TIMEOUT = 20 * 2;
const float TOO_SLOW = 0.4f;
const int CIV_SLOW_FRAMES = 40;
const int CIV_RESET_FRAMES = 80;

const float TO_DEGREES = (180.f / M_PI);
const float TO_RADIANS = (M_PI / 180.f);

const uint16_t GOLDFISH_SPECIES = (uint16_t) -1;
