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

static const char* multikillText[11] = {
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
	"KILLIONAIRE"
	// KILLHARMONIC
}; 

std::unique_ptr<ncine::TextNode> processMultikill(int multikillness) {
	std::string killstring;
	
}

void GameplayState::ProcessDeathReport(const FlatBuffGenerated::DeathReport* deathReport) {
	TextCreator textCreator(_resources);
	auto& rootNode = ncine::theApplication().rootNode();
	std::vector<std::unique_ptr<ncine::TextNode>> texts;
	const float screenWidth = ncine::theApplication().width();
	const float screenHeight = ncine::theApplication().height();
	if (deathReport->killer() == gameData.myPlayerID) {
		// i killed someone
		if (deathReport->victim() == (uint16_t)-1) {
			// it was an npc
			this->nodes.push_back(textCreator.CreateText(
				"you killed a civilian",
				ncine::Color(0, 0, 0),
				screenWidth * 0.5f, screenHeight * 0.75f,
				3.0f,
				255, 60, 45				
			));

		} else {
			// it was a player
			this->nodes.push_back(textCreator.CreateText(
				"you killed " + gameData.players[deathReport->victim()].name + "!",
				ncine::Color(0, 80, 0),
				screenWidth * 0.5f, screenHeight * 0.75f,
				3.0f,
				255, 60, 45				
			));
			this->nodes.push_back(textCreator.CreateText(
				"[" + std::to_string(deathReport->killerKills()) + "  :  " + std::to_string(deathReport->victimKills()) + "]",
				ncine::Color(0, 80, 0),
				screenWidth * 0.5f, screenHeight * 0.75f - this->nodes.back()->height(),
				1.0f,
				255, 60, 45
			));

			texts.push_back(textCreator.CreateText("enemy killed", ncine::Color(0, 40, 0, 255)));
			texts.push_back(textCreator.CreateText(std::to_string(KILL_REWARD)));

			if (deathReport->killing_spree()) {
				texts.push_back(textCreator.CreateText("killing spree [" + std::to_string(deathReport->killing_spree()) + "]"));
				texts.push_back(textCreator.CreateText(std::to_string(deathReport->killing_spree() * KILLING_SPREE_REWARD)));
			}
			if (deathReport->shutdown()) {
				texts.push_back(textCreator.CreateText("shutdown [" + std::to_string(deathReport->shutdown())+ "]"));
				texts.push_back(textCreator.CreateText(std::to_string(deathReport->shutdown() * SHUTDOWN_REWARD)));
			}
			if (deathReport->domination()) {
				texts.push_back(textCreator.CreateText("domination[" + std::to_string(deathReport->domination())+ "]"));
				texts.push_back(textCreator.CreateText(std::to_string(deathReport->domination() * DOMINATION_REWARD)));
			}
			if (deathReport->revenge()) {
				texts.push_back(textCreator.CreateText("revenge [" + std::to_string(deathReport->revenge())+ "]"));
				texts.push_back(textCreator.CreateText(std::to_string(deathReport->revenge() * REVENGE_REWARD)));
			}
			if (deathReport->comeback()) {
				texts.push_back(textCreator.CreateText("comeback [" + std::to_string(deathReport->revenge())+ "]"));
				texts.push_back(textCreator.CreateText(std::to_string(deathReport->comeback() * COMEBACK_REWARD)));
			}
			if (deathReport->multikill()) {
				texts.push_back(processMultikill(deathReport->multikill()));
			}
		}
		// place the texts from queue neatly under each other
		for (int i = 0; i < texts.size(); ++i) {
			if (i%2 == 0) {
				texts[i]->setPosition(screenWidth * 0.42f + texts[i]->width()/2.f, screenHeight * 0.5f - texts[i]->height()*(i/2));
			} else {
				texts[i]->setPosition(screenWidth * 0.57f - texts[i]->width()/2.f, screenHeight * 0.5f - texts[i]->height()*(i/2));
			}
		}

		_resources.playKillSound();
		
	} else if (deathReport->victim() == gameData.myPlayerID) {
		// i died :c
		_resources.playKillSound(0.4f);
		_resources.playRandomDeathSound();

		this->nodes.push_back(textCreator.CreateText(
			"you have been killed by " + gameData.players[deathReport->killer()].name,
			ncine::Color(255, 0, 0),
			screenWidth * 0.5f, screenHeight * 0.5f,
			2.0f
			));
	} else {
		auto killer = gameData.players[deathReport->killer()].name;
		auto victim = gameData.players[deathReport->victim()].name;
		this->nodes.push_back(textCreator.CreateText(
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