#include "wforge/level.h"
#include "wforge/assets.h"
#include "wforge/scene.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cstdlib>
#include <iostream>
#include <proxy/v4/proxy.h>

namespace wf::scene {

namespace {

int automaticScale(const Level &level) {
	auto player_screen_size = sf::VideoMode::getDesktopMode().size;
	return std::max<int>(
		1,
		std::min(
			player_screen_size.x / level.width() - 1,
			player_screen_size.y / level.height() - 1
		)
	);
}

auto loadFont() {
	static PixelFont *font = nullptr;
	if (!font) {
		font = &AssetsManager::instance().getAsset<PixelFont>("font");
	}
	return font;
}

} // namespace

LevelScene::LevelScene(Level level): LevelScene(std::move(level), 0) {}

LevelScene::LevelScene(Level level, int scale)
	: _scale(scale ? scale : automaticScale(level))
	, _level(std::move(level))
	, _renderer(_level, _scale)
	, font(*loadFont()) {}

std::array<int, 2> LevelScene::size() const {
	return {_level.width() * _scale, _level.height() * _scale};
}

void LevelScene::setup(SceneManager &mgr) {
	_level.selectItem(0);
	mgr.setBGMCollection("background/level-music");
}

void LevelScene::handleEvent(SceneManager &mgr, sf::Event &ev) {
	if (auto mw = ev.getIf<sf::Event::MouseWheelScrolled>()) {
		if (mw->delta > 0) {
			_level.changeActiveItemBrushSize(1);
		} else if (mw->delta < 0) {
			_level.changeActiveItemBrushSize(-1);
		}
	}

	if (auto mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
		auto mouse_pos = mgr.mousePosition();
		if (mb->button == sf::Mouse::Button::Left) {
			_level.useActiveItem(mouse_pos.x, mouse_pos.y, _scale);
		}
	}

	if (auto kb = ev.getIf<sf::Event::KeyPressed>()) {
		if (kb->code == sf::Keyboard::Key::R) {
			std::cerr << "Restarting level...\n";
			_restartLevel(mgr);
		}
	}
}

void LevelScene::_restartLevel(SceneManager &mgr) {
	mgr.changeScene(
		pro::make_proxy<SceneFacade, LevelScene>(
			Level::loadFromMetadata(std::move(_level.metadata)), _scale
		)
	);
}

void LevelScene::render(
	const SceneManager &mgr, sf::RenderTarget &target
) const {
	auto mouse_pos = mgr.mousePosition();
	target.clear(sf::Color::White);
	_renderer.render(target, mouse_pos.x, mouse_pos.y);
}

void LevelScene::step(SceneManager &mgr) {
	_level.step();

	if (_level.isFailed()) {
		std::cerr << "Level failed! Restarting...\n";
		_restartLevel(mgr);
	}

	if (_level.isCompleted()) {
		std::puts("Level completed! Congratulations!");
		// TODO: go to next level
		std::exit(0);
	}
}

} // namespace wf::scene
