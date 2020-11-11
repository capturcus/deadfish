#pragma once

#include "deadfish.hpp"

// skill handlers return a bool value whether the skill has been used or not
// apparently this is correct cpp syntax
using skillHandler_t = bool (*)(Player& p, Skills skill, b2Vec2 mousePos);

bool executeSkillInkbomb(Player& p, Skills skill, b2Vec2 mousePos);
bool executeSkillMobManipulator(Player& p, Skills skill, b2Vec2 mousePos);
bool executeSkillBlink(Player& p, Skills skill, b2Vec2 mousePos);

static skillHandler_t skillHandlers[(uint16_t) Skills::SKILLS_MAX] = {
	[(uint16_t) Skills::INK_BOMB] = &executeSkillInkbomb,
	[(uint16_t) Skills::ATTRACTOR] = &executeSkillMobManipulator,
	[(uint16_t) Skills::DISPERSOR] = &executeSkillMobManipulator,
	[(uint16_t) Skills::BLINK] = &executeSkillBlink,
};
