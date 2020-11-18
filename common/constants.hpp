#pragma once
#include <cstdint>
#include <cmath>

const int FRAME_TIME = 50; // 20 fps
const int SECOND = 1000/FRAME_TIME; // 1s

const uint64_t ROUND_LENGTH = 10 * 60 * 20; // 10 minutes
const int CIVILIAN_TIME = 40;
const int MAX_CIVILIANS = 100;
const float INSTA_KILL_DISTANCE = 0.61f;
const int KILL_REWARD = 5;
const int CIVILIAN_PENALTY = -1;
const int MULTIKILL_REWARD = 5;
const uint16_t KILLING_SPREE_THRESHOLD = 5;
const int KILLING_SPREE_REWARD = 1;
const int SHUTDOWN_REWARD = 2;
const uint16_t DOMINATION_THRESHOLD = 4;
const int DOMINATION_REWARD = 1;
const int REVENGE_REWARD = 2;
const uint16_t COMEBACK_THRESHOLD = 4;
const int COMEBACK_REWARD = 2;

const float TURN_SPEED = 4.f;
const float WALK_SPEED = 1.f;
const float RUN_MODIFIER = 2.f;
const float CLOSE = 0.05f;
const float ANGULAR_CLOSE = 0.1f;
const float TARGET_OFFSET = 0.3f;
const float RANDOM_OFFSET = 0.15f;
const int DEATH_TIMEOUT = 2 * SECOND;
const int MULTIKILL_TIMEOUT = DEATH_TIMEOUT + 4 * SECOND;
const float TOO_SLOW = 0.4f;
const int CIV_SLOW_FRAMES = 40;
const int CIV_RESET_FRAMES = 80;
const float CHASE_SPEED_BONUS = 1.25f;

const float TO_DEGREES = (180.f / M_PI);
const float TO_RADIANS = (M_PI / 180.f);

const uint16_t GOLDFISH_SPECIES = (uint16_t) -1;

enum class Skills {
    INK_BOMB = 0,
    ATTRACTOR,
    DISPERSOR,
    BLINK,
    SKILLS_MAX
};

const int MAX_SKILLS = 3;
const float BLINK_RANGE = 10.f;
