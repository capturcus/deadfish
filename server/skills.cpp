#include "skills.hpp"

void executeSkillInkbomb(Player& p, Skills skill, b2Vec2 mousePos) {
    std::cout << "ink bomb\n";
}

void executeSkillMobManipulator(Player& p, Skills skill, b2Vec2 mousePos) {
    std::cout << "mob manipulator " << (uint16_t) skill << "\n";
}

void executeSkillBlink(Player& p, Skills skill, b2Vec2 mousePos) {
    std::cout << "skill blink\n";
}
