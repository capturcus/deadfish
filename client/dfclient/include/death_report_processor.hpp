#pragma once

/** 
 * The helper object class, responsible for performing the processDeathReport() procedure and everything that goes with it.
 * This includes handling the killcount mechanics. Killcount mechanics are:
 *  - killing spree - multiple kills without dying
 *  - shutdown - stopping somebody's killing spree
 *  - domination - killing a player without him getting a kill back
 *  - revenge - getting a kill back at a dominating player
 *  - comeback - getting a kill for the first time after dying multiple times
 *  - multikill - getting several kills in a quick succession
 */
#include <ncine/Application.h>

#include "resources.hpp"
#include "text_creator.hpp"
#include "../../../common/types.hpp"

class DeathReportProcessor {
private:
	TextCreator _textCreator;
	Resources& _resources;
	const ncine::SceneNode& _rootNode;
	const float _screenWidth, _screenHeight;
	const std::array<float, 3> _textsXPos;
	const std::array<float, 4> _textsYPos;
	std::unique_ptr<ncine::TextNode> _killingSpreeText;

public:
	DeathReportProcessor(Resources& resources)
		: _textCreator(resources)
		, _resources(resources)
		, _rootNode(ncine::theApplication().rootNode())
		, _screenWidth(ncine::theApplication().width())
		, _screenHeight(ncine::theApplication().height())
		, _textsXPos{0.41f * _screenWidth, 0.5f * _screenWidth, 0.59f * _screenWidth}
		, _textsYPos{0.75f * _screenHeight, 0.5f * _screenHeight, 0.30f * _screenHeight, 0.25f * _screenHeight}
	{}

	// Main method declaration
	DrawableNodeVector ProcessDeathReport(const FlatBuffGenerated::DeathReport* report);

private:
	/** A ProcessDeathReport helper function, responsible for handling killing somebody.
	 */
	DrawableNodeVector processKill(const FlatBuffGenerated::DeathReport* deathReport);

	/** A ProcessDeathReport helper function, responsible for handling being killed.
	 */
	DrawableNodeVector processDeath(const FlatBuffGenerated::DeathReport* deathReport);

	/** A ProcessDeathReport helper function, responsible for handling reports of a kill 
	 * which doesn't involve the player.
	 */
	DrawableNodeVector processObituary(const FlatBuffGenerated::DeathReport* deathReport);

	/** A processKill helper function, responsible for managing the client's reaction to the player's multikill.
	 * @return A textnode with the adequate multikill text, to be included in the summary
	 */
	std::unique_ptr<ncine::TextNode> processMultikill(int multikillness, DrawableNodeVector& retTextNodes);

	std::unique_ptr<ncine::DrawableNode> endKillingSpree();
};
