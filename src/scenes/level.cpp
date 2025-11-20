#include "wforge/level.h"
#include "wforge/assets.h"
#include "wforge/colorpalette.h"
#include "wforge/save.h"
#include "wforge/scene.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <proxy/v4/proxy.h>
#include <string_view>

namespace wf::scene {

namespace {

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

LevelPlaying::LevelPlaying(Level level): LevelPlaying(std::move(level), 0) {}

LevelPlaying::LevelPlaying(Level level, int scale)
	: _scale(automaticScale(level.width(), level.height(), scale))
	, _level(std::move(level))
	, _renderer(_level, _scale)
	, font(*loadFont()) {}

std::array<int, 2> LevelPlaying::size() const {
	return {_level.width() * _scale, _level.height() * _scale};
}

void LevelPlaying::setup(SceneManager &mgr) {
	_level.selectItem(0);
	mgr.setBGMCollection("background/level-music");
}

void LevelPlaying::handleEvent(SceneManager &mgr, sf::Event &ev) {
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
		switch (kb->code) {
		case sf::Keyboard::Key::R:
			if (restart_hint_opacity > 0) {
				int duck_x = std::round(_level.duck.position.x);
				int duck_y = std::round(_level.duck.position.y);
				mgr.changeScene(
					pro::make_proxy<SceneFacade, DuckDeath>(
						_level.width(), _level.height(), duck_x, duck_y, _scale,
						LevelMetadata(_level.metadata)
					)
				);
				return;
			} else {
				restart_hint_opacity = restart_hint_max_opacity;
			}
			break;

		case sf::Keyboard::Key::Up:
		case sf::Keyboard::Key::PageUp:
			_level.prevItem();
			break;

		case sf::Keyboard::Key::Down:
		case sf::Keyboard::Key::PageDown:
			_level.nextItem();
			break;

		default:
			break;
		}
	}
}

void LevelPlaying::_restartLevel(SceneManager &mgr) {
	mgr.changeScene(
		pro::make_proxy<SceneFacade, LevelPlaying>(
			Level::loadFromMetadata(std::move(_level.metadata)), _scale
		)
	);
}

void LevelPlaying::render(
	const SceneManager &mgr, sf::RenderTarget &target
) const {
	auto mouse_pos = mgr.mousePosition();
	_renderer.render(target, mouse_pos.x, mouse_pos.y);

	if (restart_hint_opacity > 0) {
		int text_width = restart_hint_text.size() * font.charWidth();
		int x = (_level.width() - text_width) / 2;
		int y = _level.height() - font.charHeight() - 10;
		sf::Color text_color = ui_text_color(restart_hint_opacity);
		font.renderText(target, restart_hint_text, text_color, x, y, _scale);
	}
}

void LevelPlaying::step(SceneManager &mgr) {
	_level.step();

	if (restart_hint_opacity > 0) {
		restart_hint_opacity -= restart_hint_fade_speed;
	}

	if (_level.isFailed()) {
		_restartLevel(mgr);
		return;
	}

	if (_level.isCompleted()) {
		auto &save = SaveData::instance();
		if (save.completed_levels < _level.metadata.index + 1) {
			save.completed_levels = _level.metadata.index + 1;
			save.save();
		}

		mgr.changeScene(
			pro::make_proxy<SceneFacade, LevelComplete>(
				_level.width(), _level.height(),
				std::round(_level.duck.position.x),
				std::round(_level.duck.position.y), _scale
			)
		);
		return;
	}
}

} // namespace wf::scene
