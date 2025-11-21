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

constexpr int hint_fade_speed = 3;
constexpr int hint_max_opacity = 200;

enum HintType {
	None = 0,
	RestartLevel,
	QuitLevel,
};

std::string_view hintTextOf(int type) {
	switch (type) {
	case HintType::RestartLevel:
		return "Press R again to retry";

	case HintType::QuitLevel:
		return "Press ESC again to quit";

	default:
		return "";
	}
}

} // namespace

LevelPlaying::LevelPlaying(Level level): LevelPlaying(std::move(level), 0) {}

LevelPlaying::LevelPlaying(Level level, int scale)
	: _scale(automaticScale(level.width(), level.height(), scale))
	, _level(std::move(level))
	, _renderer(_level, _scale)
	, _hint_type(HintType::None)
	, _hint_opacity(0)
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
			if (_hint_opacity > 0 && _hint_type == HintType::RestartLevel) {
				_restartLevel(mgr, false);
				return;
			} else {
				_hint_type = HintType::RestartLevel;
				_hint_opacity = hint_max_opacity;
			}
			break;

		case sf::Keyboard::Key::Escape:
			if (_hint_opacity > 0 && _hint_type == HintType::QuitLevel) {
				mgr.changeScene(
					pro::make_proxy<SceneFacade, LevelSelectionMenu>(_scale)
				);
				return;
			} else {
				_hint_type = HintType::QuitLevel;
				_hint_opacity = hint_max_opacity;
			}
			break;

		case sf::Keyboard::Key::Up:
		case sf::Keyboard::Key::PageUp:
		case sf::Keyboard::Key::W:
			_level.prevItem();
			break;

		case sf::Keyboard::Key::Down:
		case sf::Keyboard::Key::PageDown:
		case sf::Keyboard::Key::S:
			_level.nextItem();
			break;

		default:
			break;
		}
	}
}

void LevelPlaying::_restartLevel(SceneManager &mgr, bool is_failed) {
	int duck_x = std::round(_level.duck.position.x);
	int duck_y = std::round(_level.duck.position.y);

	if (is_failed) {
		// duck is out of world bounds
		// place it to center of the level for animation
		int duck_w = _level.duck.width();
		int duck_h = _level.duck.height();
		duck_x = (_level.width() - duck_w) / 2;
		duck_y = (_level.height() - duck_h) / 2;
	}

	mgr.changeScene(
		pro::make_proxy<SceneFacade, DuckDeath>(
			_level.width(), _level.height(), duck_x, duck_y, _scale,
			LevelMetadata(_level.metadata)
		)
	);
	return;
}

void LevelPlaying::render(
	const SceneManager &mgr, sf::RenderTarget &target
) const {
	auto mouse_pos = mgr.mousePosition();
	_renderer.render(target, mouse_pos.x, mouse_pos.y);

	if (_hint_opacity > 0) {
		auto hint_text = hintTextOf(_hint_type);
		int text_width = hint_text.size() * font.charWidth();
		int x = (_level.width() - text_width) / 2;
		int y = _level.height() - font.charHeight() - 10;
		sf::Color text_color = ui_text_color(_hint_opacity);
		font.renderText(target, hint_text, text_color, x, y, _scale);
	}
}

void LevelPlaying::step(SceneManager &mgr) {
	_level.step();

	if (_hint_opacity > 0) {
		_hint_opacity -= hint_fade_speed;

		if (_hint_opacity <= 0) {
			_hint_opacity = 0;
			_hint_type = HintType::None;
		}
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
