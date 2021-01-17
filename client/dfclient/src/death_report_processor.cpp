#include "death_report_processor.hpp"

#include "../../../common/constants.hpp"

#include "game_data.hpp"
#include "gameplay_state.hpp"

DrawableNodeVector DeathReportProcessor::ProcessDeathReport(const FlatBuffGenerated::DeathReport* deathReport) {
	
	if (deathReport->killer() == gameData.myPlayerID) {
		// i killed someone
		return processKill(deathReport);
		
	} else if (deathReport->victim() == gameData.myPlayerID) {
		// i died :c
		return processDeath(deathReport);
	} else {
		// somebody died
		return processObituary(deathReport);
	}
}

/* ----------- I KILLED SOMEBODY ------------ */

DrawableNodeVector DeathReportProcessor::processKill(const FlatBuffGenerated::DeathReport* deathReport) {
		_resources.playSound(SoundType::KILL);

		// a vector of texts to be returned and put into GameplayState->textNodes
		DrawableNodeVector retTextNodes;

		// a buffer to manage the position of summary texts before appending to retTextNodes
		std::vector<std::unique_ptr<ncine::TextNode>> texts;

		if (deathReport->victim() == (uint16_t)-1) {
			// it was a CIVILIAN
			auto killtext = _textCreator.CreateText(
				"you killed a civilian",
				ncine::Color::Black,
				_textsXPos[1], _textsXPos[0],
				3.0f
				);
			
			_textCreator.setTweenParams(200, 180, 30);
			texts.push_back(_textCreator.CreateText(
				"civilian casualty",
				ncine::Color::Black,
				_textsXPos[0], _textsYPos[1]
				));
			texts.back()->setPosition(_textsXPos[0]+texts.back()->width()/2, _textsYPos[1]);
			texts.push_back(_textCreator.CreateText(
				std::to_string(CIVILIAN_PENALTY),
				ncine::Color::Black,
				_textsXPos[2], _textsYPos[1]
				));

			if (deathReport->multikill() >= 2 || deathReport->killing_spree()) {
				retTextNodes.push_back(_textCreator.CreateText(
					"killing spree stopped",
					ncine::Color(140, 0, 0),
					_textsXPos[1], _textsYPos[0] - killtext->height(),
					2.0f,
					255, 30, 70
					));
				retTextNodes.push_back(_textCreator.CreateOutline());
				retTextNodes.push_back(this->endKillingSpree());
			}
			retTextNodes.push_back(std::move(killtext));

		} else if (deathReport->victim() == (uint16_t)-2) {
			// it was a GOLDFISH!
			retTextNodes.push_back(_textCreator.CreateText(
				"+1 skill!",
				ncine::Color(230, 230, 0),
				_textsXPos[1], _textsYPos[0],
				2.5f,
				255, 30, 60
				));
			retTextNodes.push_back(_textCreator.CreateOutline());
			_resources.playSound(SoundType::GOLDFISH);	// !

		} else {
			// it was a PLAYER

			// display "you killed <player>!" and score tally against that player
			_textCreator.setTweenParams(255, 45, 90); // timing for "you killed <player>"
			_textCreator.setColor(0, 220, 0);
			auto killtext = _textCreator.CreateText(
				"you killed " + gameData.players[deathReport->victim()].name + "!",
				std::nullopt,
				_textsXPos[1], _textsYPos[0],
				3.0f,
				std::nullopt, std::nullopt, std::nullopt,
				std::nullopt,
				tweeny::easing::sinusoidalIn
				);
			auto killtextOutline = _textCreator.CreateOutline(tweeny::easing::sinusoidalIn);

			auto score = _textCreator.CreateText(
				"[" + std::to_string(deathReport->killerKills()) + "  :  " + std::to_string(deathReport->victimKills()) + "]");
			auto scoreOutline = _textCreator.CreateOutline();
			score->setPosition(_textsXPos[1], killtext->position().y + killtext->height()/2.f + score->height()/2);
			scoreOutline->setPosition(score->position().x, score->position().y);
			
			retTextNodes.push_back(std::move(killtext));
			retTextNodes.push_back(std::move(killtextOutline));
			retTextNodes.push_back(std::move(score));
			retTextNodes.push_back(std::move(scoreOutline));

			// display score summary: base score and bonuses
			_textCreator.setTweenParams(200, 180, 30); // timing for total score

			_textCreator.setScale(1.2f);
			_textCreator.setColor(0, 200, 0);
			texts.push_back(_textCreator.CreateText("enemy killed"));
			texts.push_back(_textCreator.CreateOutline());

			texts.push_back(_textCreator.CreateText(std::to_string(KILL_REWARD)));
			texts.push_back(_textCreator.CreateOutline());

			int totalScore = KILL_REWARD;
			int scoreBonus = 0;

			_textCreator.setTweenParams(200, 120, 60); // timing for score texts
			_textCreator.setScale(1.1f);
			_textCreator.setPosition(_textsXPos[1], _textsYPos[1]);
			if (deathReport->multikill()) {
				scoreBonus = deathReport->multikill() * MULTIKILL_REWARD;
				totalScore += scoreBonus;
				texts.push_back(processMultikill(deathReport->multikill(), retTextNodes));
				texts.push_back(_textCreator.CreateOutline());
				texts.push_back(_textCreator.CreateText(std::to_string(scoreBonus)));
				texts.push_back(_textCreator.CreateOutline());
			}
			if (deathReport->killing_spree()) {
				scoreBonus = deathReport->killing_spree() * KILLING_SPREE_REWARD;
				totalScore += scoreBonus;
				texts.push_back(_textCreator.CreateText("killing spree [" + std::to_string(deathReport->killing_spree()) + "]"));
				texts.push_back(_textCreator.CreateOutline());
				texts.push_back(_textCreator.CreateText(std::to_string(scoreBonus)));
				texts.push_back(_textCreator.CreateOutline());
				if (!_resources._killingSpreeTween.has_value()) {
					// create giant red indicator text
					_killingSpreeText = _textCreator.CreateText(
						"KILLING SPREE",
						ncine::Color(240, 0, 0),
						_textsXPos[1], _screenHeight * 0.8f,
						6.0f,
						200, 0, -1,
						Layers::ADDITIONAL_TEXT
						);
					auto killingSpreeTextNode = _killingSpreeText.get();
					_resources._killingSpreeTween =
						tweeny::from((int)_killingSpreeText->alpha()).to(40).during(45).via(tweeny::easing::sinusoidalInOut).onStep(
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
				texts.push_back(_textCreator.CreateText("shutdown [" + std::to_string(deathReport->shutdown()) + "]"));
				texts.push_back(_textCreator.CreateOutline());
				texts.push_back(_textCreator.CreateText(std::to_string(scoreBonus)));
				texts.push_back(_textCreator.CreateOutline());
				retTextNodes.push_back(_textCreator.CreateText(
					"you ended " + gameData.players[deathReport->victim()].name + "'s killing spree!",
					ncine::Color(255, 70, 0),
					_textsXPos[1], _textsYPos[2],
					1.5f,
					255, 60, 120
				));
				retTextNodes.push_back(_textCreator.CreateOutline());
			}
			if (deathReport->domination()) {
				scoreBonus = deathReport->domination() * DOMINATION_REWARD;
				totalScore += scoreBonus;
				texts.push_back(_textCreator.CreateText("domination[" + std::to_string(deathReport->domination())+ "]"));
				texts.push_back(_textCreator.CreateOutline());
				texts.push_back(_textCreator.CreateText(std::to_string(scoreBonus)));
				texts.push_back(_textCreator.CreateOutline());
				retTextNodes.push_back(_textCreator.CreateText(
					"you are dominating " + gameData.players[deathReport->victim()].name + "!",
					ncine::Color(240, 0, 0),
					_textsXPos[1], _textsYPos[3],
					1.5f,
					255, 60, 120
				));
				retTextNodes.push_back(_textCreator.CreateOutline());
			}
			if (deathReport->revenge()) {
				scoreBonus = deathReport->revenge() * REVENGE_REWARD;
				totalScore += scoreBonus;
				texts.push_back(_textCreator.CreateText("revenge [" + std::to_string(deathReport->revenge())+ "]"));
				texts.push_back(_textCreator.CreateOutline());
				texts.push_back(_textCreator.CreateText(std::to_string(scoreBonus)));
				texts.push_back(_textCreator.CreateOutline());
				retTextNodes.push_back(_textCreator.CreateText(
					"you got revenge on " + gameData.players[deathReport->victim()].name + "!",
					ncine::Color(255, 70, 0),
					_textsXPos[1], _textsYPos[3],
					1.5f,
					255, 60, 120
				));
				retTextNodes.push_back(_textCreator.CreateOutline());
			}
			if (deathReport->comeback()) {
				scoreBonus = deathReport->comeback() * COMEBACK_REWARD;
				totalScore += scoreBonus;
				texts.push_back(_textCreator.CreateText("comeback [" + std::to_string(deathReport->revenge())+ "]"));
				texts.push_back(_textCreator.CreateOutline());
				texts.push_back(_textCreator.CreateText(std::to_string(scoreBonus)));
				texts.push_back(_textCreator.CreateOutline());
			}
			// place the texts from queue neatly under each other
			float heightOffset = 0;
			for (int i = 0; i < texts.size(); ++i) {
				auto text = texts[i].get();
				if (i%4 == 0 || i%4 == 1) {
					text->setPosition(_textsXPos[0] + text->width()/2.f, _textsYPos[1] - heightOffset);
				} else {
					text->setPosition(_textsXPos[2] - text->width()/2.f, _textsYPos[1] - heightOffset);
					// create position tween to make the score fly up to the total
					this->_resources._floatTweens.push_back(
						tweeny::from(text->position().y).to(text->position().y).during(120)
						.to(_textsYPos[1]).during(60).via(tweeny::easing::circularOut).onStep(
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
			// create number tween to make the total go up to the final value
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
		for (auto& text : texts) {
			retTextNodes.push_back(std::move(text));
		}
	return retTextNodes;
}

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

/** Manage everything regarding multikill:
 * - create flash texts
 * - return the appropriate text (with the number, if necessary) for the summary
 */
std::unique_ptr<ncine::TextNode> DeathReportProcessor::processMultikill(int multikillness, DrawableNodeVector& retTextNodes) {
	std::string killstring;
	killstring = (multikillness <= 11) ? multikillText[multikillness] : multikillText[11];
	
	// create floating texts

	// determine the flashtext color (red with growing intensity the higher the multikill)
	uint16_t redness = 40 + 20*multikillness; // starting from 80
	if (redness > 255) redness = 255;
	ncine::Color color = ncine::Color(redness,0,0);

	const float floatingTextFadeout = 150;
	const auto easing = tweeny::easing::circularOut;

	auto primaryFlash = _textCreator.CreateText(killstring, color, std::nullopt, std::nullopt, 5.f, 180, 0, floatingTextFadeout, Layers::ADDITIONAL_TEXT, easing);
	auto secondaryFlash = _textCreator.CreateText(killstring, color, std::nullopt, std::nullopt, 5.f, 100, 0, floatingTextFadeout, Layers::ADDITIONAL_TEXT, easing);
	auto secondaryFlashNode = secondaryFlash.get();
	// create growing tween
	this-> _resources._floatTweens.push_back(
		tweeny::from(5.f).to(10.f * _screenWidth/1800.f).during(floatingTextFadeout).via(easing).onStep(
			[secondaryFlashNode] (tweeny::tween<float> t, float v) {
				secondaryFlashNode->setScale(v);
				return false;
			}
		)
	);
	retTextNodes.push_back(std::move(primaryFlash));
	retTextNodes.push_back(std::move(secondaryFlash));

	// standardizing text over "penta kill" in order to:
	// - make clear at what number we're on
	// - fit into summary table xd
	if (multikillness > 5) {
		killstring = "MULTIKILL [" + std::to_string(multikillness) + "]";
	}
	return _textCreator.CreateText(killstring);
}

/** Handle the killing spree end:
 *	- add a transition effect for the giant indicator text to get it off the screen
 *	- make sure the tweens and text are properly managed memory-wise
 *		- remove the neverending tween from its own, unmanaged place
 *		- add a tween to reduce text alpha to 0 and place the tween in civilized _intTweens,
 *			from where it will be removed once it reaches its end
 *		- return the text, so it can be moved to textNodes be removed once the abovementioned tween
 *			gets the text's alpha to 0
 * @returns the blinking text node to be inserted to GameplayState->textNodes
 */
std::unique_ptr<ncine::DrawableNode> DeathReportProcessor::endKillingSpree() {
	auto killingSpreeTextNode = _killingSpreeText.get();
	_resources._killingSpreeTween.reset();
	_resources._intTweens.push_back(_textCreator.CreateTextTween(
		killingSpreeTextNode,
		(int)_killingSpreeText->alpha(), 0, 12,
		tweeny::easing::sinusoidalOut
	));
	_resources._floatTweens.push_back(
		tweeny::from(_killingSpreeText->scale().x)
			.to(10.f * ncine::theApplication().width()/1800.f)
			.during(12)
			.via(tweeny::easing::quarticIn)
			.onStep([killingSpreeTextNode](tweeny::tween<float> & t, float v) {
				killingSpreeTextNode->setScale(v);
				return false;
			})
	);
	return std::move(_killingSpreeText);
}

/* ----------- I GOT KILLED ------------ */

/** Manage the fact that somebody just killed the player
 */
DrawableNodeVector DeathReportProcessor::processDeath(const FlatBuffGenerated::DeathReport* deathReport) {
	_resources.playSound(SoundType::KILL, 0.4f);
	_resources.playSound(SoundType::DEATH);

	// a vector of texts to be returned and put into GameplayState->textNodes
	DrawableNodeVector retTextNodes;

	// display the sad message about the unfortunate event (and score tally, to make it even worse)
	_textCreator.setColor(230, 0, 0);
	auto killtext = _textCreator.CreateText(
		"you have been killed by " + gameData.players[deathReport->killer()].name,
		std::nullopt,
		_textsXPos[1], _screenHeight * 0.6f,
		2.0f
		);
	auto killtextOutline = _textCreator.CreateOutline();
	auto score = _textCreator.CreateText(
		"[" + std::to_string(deathReport->killerKills()) + "  :  " + std::to_string(deathReport->victimKills()) + "]");
	auto scoreOutline = _textCreator.CreateOutline();
	score->setPosition(_textsXPos[1], killtext->position().y + killtext->height()/2.f + score->height()/2);
	scoreOutline->setPosition(score->position().x, score->position().y);

	// display additional texts to put some salt onto the wound
	if (deathReport->shutdown()) {
		retTextNodes.push_back(_textCreator.CreateText(
			"killing spree stopped",
			ncine::Color(140, 0, 0),
			_textsXPos[1], killtext->position().y - killtext->height(),
			2.0f,
			255, 30, 70
			));
		retTextNodes.push_back(_textCreator.CreateOutline());
		retTextNodes.push_back(endKillingSpree());
	}
	if (deathReport->domination()) {
		retTextNodes.push_back(_textCreator.CreateText(
			gameData.players[deathReport->killer()].name + " is dominating you!",
			ncine::Color(200, 0, 0),
			_textsXPos[1], _textsYPos[3],
			2.0f,
			255, 60, 40
		));
		retTextNodes.push_back(_textCreator.CreateOutline());
	}
	if (deathReport->revenge()) {
		retTextNodes.push_back(_textCreator.CreateText(
			gameData.players[deathReport->killer()].name + " got revenge!",
			ncine::Color(50, 0, 0),
			_textsXPos[1], _textsYPos[3],
			2.0f,
			255, 60, 90
		));
		retTextNodes.push_back(_textCreator.CreateOutline());
	}
	retTextNodes.push_back(std::move(killtext));
	retTextNodes.push_back(std::move(killtextOutline));
	retTextNodes.push_back(std::move(score));
	retTextNodes.push_back(std::move(scoreOutline));

	return retTextNodes;
}

/* ----------- SOMEBODY GOT KILLED ------------ */

/** Inform the player that somebody somewhere said goodbye to this world
 * TODO: a proper message queue with proper event descriptions ("asd is dominating asdd!")
 */
DrawableNodeVector DeathReportProcessor::processObituary(const FlatBuffGenerated::DeathReport* deathReport){

	// a vector of texts to be returned and put into GameplayState->textNodes
	DrawableNodeVector retTextNodes;

	auto killer = gameData.players[deathReport->killer()].name;
	auto victim = gameData.players[deathReport->victim()].name;
	retTextNodes.push_back(_textCreator.CreateText(
		killer + " killed " + victim,
		ncine::Color::Black,
		_screenWidth * 0.8f, _screenHeight * 0.8f,
		0.5f,
		255, 120, 90
		));

	return retTextNodes;
}
