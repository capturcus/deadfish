/** 
 * The processDeathReport() procedure and everything that goes with it.
 * This includes handling the killcount mechanics. Killcount mechanics are:
 *  - killing spree - multiple kills without dying
 *  - shutdown - stopping somebody's killing spree
 *  - domination - killing a player without him getting a kill back
 *  - revenge - getting a kill back at a dominating player
 *  - comeback - getting a kill for the first time after dying multiple times
 *  - multikill - getting several kills in a quick succession
 */

#include <ncine/Application.h>

#include "../../../common/constants.hpp"

#include "game_data.hpp"
#include "gameplay_state.hpp"
#include "resources.hpp"
#include "text_creator.hpp"

static const char* multikillText[12] = {
	"",
	"",
	"DOUBLE KILL",
	"TRIPLE KILL",
	"QUADRA KILL",
	"PENTA KILL",
	"OVERKILL",
	"KILLIMANJARO",
	"KILLANTHROPIST",
	"KILLICIOUS",
	"KILLIONAIRE",
	"KILLHARMONIC"
}; 

void GameplayState::ProcessDeathReport(void const* ev) {
	auto deathReport = (const FlatBuffGenerated::DeathReport*) ev;
	auto& rootNode = ncine::theApplication().rootNode();
	std::vector<std::unique_ptr<ncine::TextNode>> texts;
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();
	const float textsXPos[] = {0.41f * screenWidth, 0.59f * screenWidth};
	const float textsYPos = 0.5 * screenHeight;

	// timing for "you killed <player>"
	textCreator->setTweenParams(255, 45, 105);

	if (deathReport->killer() == gameData.myPlayerID) {
		// i killed someone
		if (deathReport->victim() == (uint16_t)-1) {
			// it was an npc
			auto killtext = textCreator->CreateText(
				"you killed a civilian",
				ncine::Color(0, 0, 0),
				screenWidth * 0.5f, screenHeight * 0.75f,
				3.0f
				);
			// timing for score texts
			textCreator->setTweenParams(200, 180, 30);
			texts.push_back(textCreator->CreateText(
				"civilian casualty",
				std::nullopt,
				textsXPos[0], textsYPos
				));
			texts.push_back(textCreator->CreateText(
				std::to_string(CIVILIAN_PENALTY),
				std::nullopt,
				textsXPos[1], textsYPos
				));

			if (deathReport->multikill() || deathReport->killing_spree()) {
				this->textNodes.push_back(textCreator->CreateText(
					"killing spree stopped",
					ncine::Color(70, 0, 0),
					screenWidth * 0.5f, screenHeight * 0.75f - killtext->height(),
					2.0f,
					255, 30, 70
					));
			}
			this->textNodes.push_back(std::move(killtext));

		} else if (deathReport->victim() == (uint16_t)-2) {
			// it was a goldfish!
			this->textNodes.push_back(textCreator->CreateText(
				"+1 skill!",
				ncine::Color(150, 150, 0),
				screenWidth * 0.5, screenHeight * 0.75,
				2.5f,
				255, 30, 60
				));

		} else {
			// it was a player
			auto killtext = textCreator->CreateText(
				"you killed " + gameData.players[deathReport->victim()].name + "!",
				ncine::Color(0, 80, 0),
				screenWidth * 0.5f, screenHeight * 0.75f,
				3.0f
				);
			auto score = textCreator->CreateText(
				"[" + std::to_string(deathReport->killerKills()) + "  :  " + std::to_string(deathReport->victimKills()) + "]",
				ncine::Color(0, 80, 0),
				screenWidth * 0.5f, screenHeight * 0.75f - killtext->height(),
				1.0f
				);
			this->textNodes.push_back(std::move(killtext));
			this->textNodes.push_back(std::move(score));

			// timing for score texts
			textCreator->setTweenParams(200, 180, 30);

			textCreator->setScale(1.0f);
			textCreator->setColor(0, 70, 0);
			texts.push_back(textCreator->CreateText("enemy killed"));
			texts.push_back(textCreator->CreateText(std::to_string(KILL_REWARD)));

			int totalScore = KILL_REWARD;
			int scoreBonus;

			textCreator->setTweenParams(200, 120, 60);
			textCreator->setScale(0.9f);
			textCreator->setPosition(screenWidth/2.f, textsYPos);
			if (deathReport->multikill()) {
				scoreBonus = deathReport->multikill() * MULTIKILL_REWARD;
				totalScore += scoreBonus;
				texts.push_back(processMultikill(deathReport->multikill()));
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
			}
			if (deathReport->killing_spree()) {
				scoreBonus = deathReport->killing_spree() * KILLING_SPREE_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("killing spree [" + std::to_string(deathReport->killing_spree()) + "]"));
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
			}
			if (deathReport->shutdown()) {
				scoreBonus = deathReport->shutdown() * SHUTDOWN_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("shutdown [" + std::to_string(deathReport->shutdown())+ "]"));
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
			}
			if (deathReport->domination()) {
				scoreBonus = deathReport->domination() * DOMINATION_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("domination[" + std::to_string(deathReport->domination())+ "]"));
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
			}
			if (deathReport->revenge()) {
				scoreBonus = deathReport->revenge() * REVENGE_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("revenge [" + std::to_string(deathReport->revenge())+ "]"));
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
			}
			if (deathReport->comeback()) {
				scoreBonus = deathReport->comeback() * COMEBACK_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("comeback [" + std::to_string(deathReport->revenge())+ "]"));
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
			}
			// place the texts from queue neatly under each other
			float heightOffset = 0;
			for (int i = 0; i < texts.size(); ++i) {
				auto text = texts[i].get();
				if (i%2 == 0) {
					text->setPosition(textsXPos[0] + text->width()/2.f, textsYPos - heightOffset);
				} else {
					text->setPosition(textsXPos[1] - text->width()/2.f, textsYPos - heightOffset);
					heightOffset += texts[i]->height();
					// create position tween to make the score fly to the total
					this->_resources._floatTweens.push_back(
						tweeny::from(text->position().y).to(text->position().y).during(120)
						.to(textsYPos).during(60).via(tweeny::easing::circularOut).onStep(
							[text] (tweeny::tween<float>& t, float y) {
								text->setPosition(text->position().x, y);
								return false;
							}
						)
					);
				}
			}

			auto scoreNode = texts[1].get();
			// create number tween to make the total go up to the value
			this->_resources._intTweens.push_back(
				tweeny::from(KILL_REWARD).to(KILL_REWARD).during(120)
					.to(totalScore).during(20).onStep(
						[scoreNode] (tweeny::tween<int>& t, int v) -> bool {
							scoreNode->setString(nctl::String(std::to_string(v).c_str()));
							return false;
						}
					));
		}
		_resources.playKillSound();
		
	} else if (deathReport->victim() == gameData.myPlayerID) {
		// i died :c
		_resources.playKillSound(0.4f);
		_resources.playRandomDeathSound();

		this->textNodes.push_back(textCreator->CreateText(
			"you have been killed by " + gameData.players[deathReport->killer()].name,
			ncine::Color(255, 0, 0),
			screenWidth * 0.5f, screenHeight * 0.5f,
			2.0f
			));
	} else {
		auto killer = gameData.players[deathReport->killer()].name;
		auto victim = gameData.players[deathReport->victim()].name;
		this->textNodes.push_back(textCreator->CreateText(
			killer + " killed " + victim,
			ncine::Color(0, 0, 0),
			screenWidth * 0.8f, screenHeight * 0.8f,
			0.5f
			));
	}
	for (auto& text : texts) {
		this->textNodes.push_back(std::move(text));
	}
}

std::unique_ptr<ncine::TextNode> GameplayState::processMultikill(int multikillness) {
	std::string killstring;
	killstring = (multikillness <= 11) ? multikillText[multikillness] : multikillText[11];

	// create floating texts
	uint16_t redness = 60 + 20*(multikillness-1);
	if (redness > 255) redness = 255;
	ncine::Color color = ncine::Color(redness,0,0);

	auto primaryFlash = textCreator->CreateText(killstring, color, std::nullopt, std::nullopt, 4.f, 130, 0, 120, Layers::ADDITIONAL_TEXT, tweeny::easing::quadraticOut);
	auto secondaryFlash = textCreator->CreateText(killstring, color, std::nullopt, std::nullopt, 4.f, 100, 0, 150, Layers::ADDITIONAL_TEXT, tweeny::easing::quadraticOut);
	auto secondaryFlashNode = secondaryFlash.get();
	// create growing tween
	this-> _resources._floatTweens.push_back(
		tweeny::from(5.f).to(10.f).during(150).via(tweeny::easing::quadraticOut).onStep(
			[secondaryFlashNode] (tweeny::tween<float> t, float v) {
				secondaryFlashNode->setScale(v);
				return false;
			}
		)
	);
	this->textNodes.push_back(std::move(primaryFlash));
	this->textNodes.push_back(std::move(secondaryFlash));

	if (multikillness > 5) {
		killstring = "MULTIKILL [" + std::to_string(multikillness) + "]";
	}
	return textCreator->CreateText(killstring);
}
