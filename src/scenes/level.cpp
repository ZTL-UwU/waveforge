#include "wforge/level.h"
#include "wforge/assets.h"
#include "wforge/colorpalette.h"
#include "wforge/scene.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cstdlib>
#include <format>
#include <iostream>
#include <proxy/v4/proxy.h>
#include <string_view>

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

constexpr std::string_view restart_hint_text = "Press R again to restart";
constexpr int restart_hint_fade_speed = 3;
constexpr int restart_hint_max_opacity = 200;

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
			if (restart_hint_opacity > 0) {
				std::cerr << std::format(
					"Restarting level '{}'\n", _level.metadata.name
				);
				_restartLevel(mgr);
			} else {
				restart_hint_opacity = restart_hint_max_opacity;
			}
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

	if (restart_hint_opacity > 0) {
		int text_width = restart_hint_text.size() * font.charWidth();
		int x = (_level.width() - text_width) / 2;
		int y = _level.height() - font.charHeight() - 10;
		sf::Color text_color = ui_text_color;
		text_color.a = static_cast<uint8_t>(restart_hint_opacity);
		font.renderText(target, restart_hint_text, text_color, x, y, _scale);
	}
}

void LevelScene::step(SceneManager &mgr) {
	_level.step();

	if (restart_hint_opacity > 0) {
		restart_hint_opacity -= restart_hint_fade_speed;
	}

	if (_level.isFailed()) {
		std::cerr << std::format(
			"Level '{}' failed. Restarting...\n", _level.metadata.name
		);
		_restartLevel(mgr);
	}

	if (_level.isCompleted()) {
		std::puts("Level completed! Congratulations!");
		// TODO: go to next level
		std::exit(0);
	}
}

} // namespace wf::scene
