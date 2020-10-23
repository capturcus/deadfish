#pragma once

#include "deadfish.hpp"

// apparently this is correct cpp syntax
using skillHandler_t = void (*)(Player& p, Skills skill, b2Vec2 mousePos);

void executeSkillInkbomb(Player& p, Skills skill, b2Vec2 mousePos);
void executeSkillMobManipulator(Player& p, Skills skill, b2Vec2 mousePos);
void executeSkillBlink(Player& p, Skills skill, b2Vec2 mousePos);

static skillHandler_t skillHandlers[(uint16_t) Skills::SKILLS_MAX] = {
    [(uint16_t) Skills::INK_BOMB] = &executeSkillInkbomb,
    [(uint16_t) Skills::ATTRACTOR] = &executeSkillMobManipulator,
    [(uint16_t) Skills::DISPERSOR] = &executeSkillMobManipulator,
    [(uint16_t) Skills::BLINK] = &executeSkillBlink,
};
