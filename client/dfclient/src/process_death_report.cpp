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
	const float textsXPos[] = {0.41f * screenWidth, 0.5f * screenWidth, 0.59f * screenWidth};
	const float textsYPos[] = {0.75f * screenHeight, 0.5f * screenHeight, 0.30f * screenHeight, 0.25f * screenHeight};

	if (deathReport->killer() == gameData.myPlayerID) {
		// i killed someone
		_resources.playSound(SoundType::KILL);

		if (deathReport->victim() == (uint16_t)-1) {
			// it was an npc
			auto killtext = textCreator->CreateText(
				"you killed a civilian",
				ncine::Color::Black,
				textsXPos[1], textsXPos[0],
				3.0f
				);
			
			textCreator->setTweenParams(200, 180, 30);
			texts.push_back(textCreator->CreateText(
				"civilian casualty",
				ncine::Color::Black,
				textsXPos[0], textsYPos[1]
				));
			texts.back()->setPosition(textsXPos[0]+texts.back()->width()/2, textsYPos[1]);
			texts.push_back(textCreator->CreateText(
				std::to_string(CIVILIAN_PENALTY),
				ncine::Color::Black,
				textsXPos[2], textsYPos[1]
				));

			if (deathReport->multikill() >= 2 || deathReport->killing_spree()) {
				this->textNodes.push_back(textCreator->CreateText(
					"killing spree stopped",
					ncine::Color(140, 0, 0),
					textsXPos[1], textsYPos[0] - killtext->height(),
					2.0f,
					255, 30, 70
					));
				this->textNodes.push_back(textCreator->CreateOutline());
				this->endKillingSpree();
			}
			this->textNodes.push_back(std::move(killtext));

		} else if (deathReport->victim() == (uint16_t)-2) {
			// it was a goldfish!
			this->textNodes.push_back(textCreator->CreateText(
				"+1 skill!",
				ncine::Color(230, 230, 0),
				textsXPos[1], textsYPos[0],
				2.5f,
				255, 30, 60
				));
			this->textNodes.push_back(textCreator->CreateOutline());
			_resources.playSound(SoundType::GOLDFISH);	// !

		} else {
			// it was a player
			// timing for "you killed <player>"
			textCreator->setTweenParams(255, 45, 90);
			textCreator->setColor(0, 220, 0);
			auto killtext = textCreator->CreateText(
				"you killed " + gameData.players[deathReport->victim()].name + "!",
				std::nullopt,
				textsXPos[1], textsYPos[0],
				3.0f,
				std::nullopt, std::nullopt, std::nullopt,
				std::nullopt,
				tweeny::easing::sinusoidalIn
				);
			auto killtextOutline = textCreator->CreateOutline(tweeny::easing::sinusoidalIn);

			auto score = textCreator->CreateText(
				"[" + std::to_string(deathReport->killerKills()) + "  :  " + std::to_string(deathReport->victimKills()) + "]");
			auto scoreOutline = textCreator->CreateOutline();
			score->setPosition(textsXPos[1], killtext->position().y + killtext->height()/2.f + score->height()/2);
			scoreOutline->setPosition(score->position().x, score->position().y);
			
			this->textNodes.push_back(std::move(killtext));
			this->textNodes.push_back(std::move(killtextOutline));
			this->textNodes.push_back(std::move(score));
			this->textNodes.push_back(std::move(scoreOutline));

			// timing for total score
			textCreator->setTweenParams(200, 180, 30);

			textCreator->setScale(1.2f);
			textCreator->setColor(0, 200, 0);
			texts.push_back(textCreator->CreateText("enemy killed"));
			texts.push_back(textCreator->CreateOutline());

			texts.push_back(textCreator->CreateText(std::to_string(KILL_REWARD)));
			texts.push_back(textCreator->CreateOutline());

			int totalScore = KILL_REWARD;
			int scoreBonus;

			// timing for score texts
			textCreator->setTweenParams(200, 120, 60);
			textCreator->setScale(1.1f);
			textCreator->setPosition(textsXPos[1], textsYPos[1]);
			if (deathReport->multikill()) {
				scoreBonus = deathReport->multikill() * MULTIKILL_REWARD;
				totalScore += scoreBonus;
				texts.push_back(processMultikill(deathReport->multikill()));
				texts.push_back(textCreator->CreateOutline());
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
				texts.push_back(textCreator->CreateOutline());
			}
			if (deathReport->killing_spree()) {
				scoreBonus = deathReport->killing_spree() * KILLING_SPREE_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("killing spree [" + std::to_string(deathReport->killing_spree()) + "]"));
				texts.push_back(textCreator->CreateOutline());
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
				texts.push_back(textCreator->CreateOutline());
				if (!_resources._killingSpreeTween.has_value()) {
					this->killingSpreeText = textCreator->CreateText(
						"KILLING SPREE",
						ncine::Color(240, 0, 0),
						textsXPos[1], screenHeight * 0.8f,
						6.0f,
						200, 0, -1,
						Layers::ADDITIONAL_TEXT
						);
					auto killingSpreeTextNode = killingSpreeText.get();
					_resources._killingSpreeTween =
						tweeny::from((int)this->killingSpreeText->alpha()).to(40).during(45).via(tweeny::easing::sinusoidalInOut).onStep(
							[killingSpreeTextNode] (tweeny::tween<int> & t, int v) {
								killingSpreeTextNode->setAlpha(v);
								return false;
							}
						);
				}
			}
			if (deathReport->shutdown()) {
				scoreBonus = deathReport->shutdown() * SHUTDOWN_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("shutdown [" + std::to_string(deathReport->shutdown()) + "]"));
				texts.push_back(textCreator->CreateOutline());
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
				texts.push_back(textCreator->CreateOutline());
				this->textNodes.push_back(textCreator->CreateText(
					"you ended " + gameData.players[deathReport->victim()].name + "'s killing spree!",
					ncine::Color(255, 70, 0),
					textsXPos[1], textsYPos[2],
					1.5f,
					255, 60, 120
				));
				this->textNodes.push_back(textCreator->CreateOutline());
			}
			if (deathReport->domination()) {
				scoreBonus = deathReport->domination() * DOMINATION_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("domination[" + std::to_string(deathReport->domination())+ "]"));
				texts.push_back(textCreator->CreateOutline());
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
				texts.push_back(textCreator->CreateOutline());
				this->textNodes.push_back(textCreator->CreateText(
					"you are dominating " + gameData.players[deathReport->victim()].name + "!",
					ncine::Color(240, 0, 0),
					textsXPos[1], textsYPos[3],
					1.5f,
					255, 60, 120
				));
				this->textNodes.push_back(textCreator->CreateOutline());
			}
			if (deathReport->revenge()) {
				scoreBonus = deathReport->revenge() * REVENGE_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("revenge [" + std::to_string(deathReport->revenge())+ "]"));
				texts.push_back(textCreator->CreateOutline());
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
				texts.push_back(textCreator->CreateOutline());
				this->textNodes.push_back(textCreator->CreateText(
					"you got revenge on " + gameData.players[deathReport->victim()].name + "!",
					ncine::Color(255, 70, 0),
					textsXPos[1], textsYPos[3],
					1.5f,
					255, 60, 120
				));
				this->textNodes.push_back(textCreator->CreateOutline());
			}
			if (deathReport->comeback()) {
				scoreBonus = deathReport->comeback() * COMEBACK_REWARD;
				totalScore += scoreBonus;
				texts.push_back(textCreator->CreateText("comeback [" + std::to_string(deathReport->revenge())+ "]"));
				texts.push_back(textCreator->CreateOutline());
				texts.push_back(textCreator->CreateText(std::to_string(scoreBonus)));
				texts.push_back(textCreator->CreateOutline());
			}
			// place the texts from queue neatly under each other
			float heightOffset = 0;
			for (int i = 0; i < texts.size(); ++i) {
				auto text = texts[i].get();
				if (i%4 == 0 || i%4 == 1) {
					text->setPosition(textsXPos[0] + text->width()/2.f, textsYPos[1] - heightOffset);
				} else {
					text->setPosition(textsXPos[2] - text->width()/2.f, textsYPos[1] - heightOffset);
					// create position tween to make the score fly to the total
					this->_resources._floatTweens.push_back(
						tweeny::from(text->position().y).to(text->position().y).during(120)
						.to(textsYPos[1]).during(60).via(tweeny::easing::circularOut).onStep(
							[text] (tweeny::tween<float>& t, float y) {
								text->setPosition(text->position().x, y);
								return false;
							}
						)
					);
					if (i%4 == 3) heightOffset += texts[i]->height();
				}
			}

			auto scoreNode = texts[2].get();
			auto scoreOutlineNode = texts[3].get();
			// create number tween to make the total go up to the value
			this->_resources._intTweens.push_back(
				tweeny::from(KILL_REWARD).to(KILL_REWARD).during(120)
					.to(totalScore).during(20).onStep(
						[scoreNode, scoreOutlineNode] (tweeny::tween<int>& t, int v) -> bool {
							scoreNode->setString(nctl::String(std::to_string(v).c_str()));
							scoreOutlineNode->setString(nctl::String(std::to_string(v).c_str()));
							return false;
						}
					));
		}
		
	} else if (deathReport->victim() == gameData.myPlayerID) {
		// i died :c
		_resources.playSound(SoundType::KILL, 0.4f);
		_resources.playSound(SoundType::DEATH);

		textCreator->setColor(230, 0, 0);
		auto killtext = textCreator->CreateText(
			"you have been killed by " + gameData.players[deathReport->killer()].name,
			std::nullopt,
			textsXPos[1], screenHeight * 0.6f,
			2.0f
			);
		auto killtextOutline = textCreator->CreateOutline();
		auto score = textCreator->CreateText(
			"[" + std::to_string(deathReport->killerKills()) + "  :  " + std::to_string(deathReport->victimKills()) + "]");
		auto scoreOutline = textCreator->CreateOutline();
		score->setPosition(textsXPos[1], killtext->position().y + killtext->height()/2.f + score->height()/2);
		scoreOutline->setPosition(score->position().x, score->position().y);

		if (deathReport->shutdown()) {
			this->textNodes.push_back(textCreator->CreateText(
				"killing spree stopped",
				ncine::Color(140, 0, 0),
				textsXPos[1], killtext->position().y - killtext->height(),
				2.0f,
				255, 30, 70
				));
			this->textNodes.push_back(textCreator->CreateOutline());
			this->endKillingSpree();
		}
		if (deathReport->domination()) {
			this->textNodes.push_back(textCreator->CreateText(
				gameData.players[deathReport->killer()].name + " is dominating you!",
				ncine::Color(200, 0, 0),
				textsXPos[1], textsYPos[3],
				2.0f,
				255, 60, 40
			));
			this->textNodes.push_back(textCreator->CreateOutline());
		}
		if (deathReport->revenge()) {
			this->textNodes.push_back(textCreator->CreateText(
				gameData.players[deathReport->killer()].name + " got revenge!",
				ncine::Color(50, 0, 0),
				textsXPos[1], textsYPos[3],
				2.0f,
				255, 60, 90
			));
			this->textNodes.push_back(textCreator->CreateOutline());
		}
		this->textNodes.push_back(std::move(killtext));
		this->textNodes.push_back(std::move(killtextOutline));
		this->textNodes.push_back(std::move(score));
		this->textNodes.push_back(std::move(scoreOutline));
	} else {
		auto killer = gameData.players[deathReport->killer()].name;
		auto victim = gameData.players[deathReport->victim()].name;
		this->textNodes.push_back(textCreator->CreateText(
			killer + " killed " + victim,
			ncine::Color::Black,
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

	const auto screenWidth = ncine::theApplication().width();
	const float floatingTextFadeout = 150;
	const auto easing = tweeny::easing::circularOut;

	auto primaryFlash = textCreator->CreateText(killstring, color, std::nullopt, std::nullopt, 5.f, 180, 0, floatingTextFadeout, Layers::ADDITIONAL_TEXT, easing);
	auto secondaryFlash = textCreator->CreateText(killstring, color, std::nullopt, std::nullopt, 5.f, 100, 0, floatingTextFadeout, Layers::ADDITIONAL_TEXT, easing);
	auto secondaryFlashNode = secondaryFlash.get();
	// create growing tween
	this-> _resources._floatTweens.push_back(
		tweeny::from(5.f).to(10.f * screenWidth/1800.f).during(floatingTextFadeout).via(easing).onStep(
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

void GameplayState::endKillingSpree() {
	auto killingSpreeTextNode = this->killingSpreeText.get();
	_resources._killingSpreeTween.reset(); // remove the neverending tween
	_resources._intTweens.push_back(textCreator->CreateTextTween(
		killingSpreeTextNode,
		(int)this->killingSpreeText->alpha(), 0, 12,
		tweeny::easing::sinusoidalOut
	));
	_resources._floatTweens.push_back(
		tweeny::from(this->killingSpreeText->scale().x)
			.to(10.f * ncine::theApplication().width()/1800.f)
			.during(12)
			.via(tweeny::easing::quarticIn)
			.onStep([killingSpreeTextNode](tweeny::tween<float> & t, float v) {
				killingSpreeTextNode->setScale(v);
				return false;
			})
	);
	this->textNodes.push_back(std::move(this->killingSpreeText));	// textNodes are cleared when alpha reaches 0
}
