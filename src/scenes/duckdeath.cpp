#include "wforge/assets.h"
#include "wforge/level.h"
#include "wforge/scene.h"
#include <SFML/Audio/SoundBuffer.hpp>
#include <limits>
#include <proxy/v4/proxy.h>

namespace wf::scene {

namespace {

PixelAnimationFrames &duckDeathAnimation() {
	static PixelAnimationFrames *ptr = nullptr;
	if (ptr == nullptr) {
		ptr = &AssetsManager::instance().getAsset<PixelAnimationFrames>(
			"duckdeath/animation"
		);
	}
	return *ptr;
}

} // namespace

DuckDeath::DuckDeath(
	int level_width, int level_height, int duck_x, int duck_y, int scale,
	LevelMetadata level_metadata
)
	: _level_width(level_width)
	, _level_height(level_height)
	, _duck_x(duck_x)
	, _duck_y(duck_y)
	, _scale(scale)
	, _pending_timer(0)
	, _animation_frame(0)
	, _level_metadata(std::move(level_metadata))
	, _animation(duckDeathAnimation())
	, _duck_death_sound(
		  AssetsManager::instance().getAsset<sf::SoundBuffer>("sfx/duckdeath")
	  )
	, _duck_death_separate_sound(
		  AssetsManager::instance().getAsset<sf::SoundBuffer>(
			  "sfx/duckdeath-separate"
		  )
	  ) {
	_duck_anchor_bx = std::numeric_limits<int>::max();
	_duck_anchor_by = std::numeric_limits<int>::max();
	auto &raw_duck = AssetsManager::instance().getAsset<sf::Image>("duck/raw");
	int raw_duck_width = raw_duck.getSize().x;
	int raw_duck_height = raw_duck.getSize().y;
	for (unsigned int x = 0; x < raw_duck_width; ++x) {
		for (unsigned int y = 0; y < raw_duck_height; ++y) {
			sf::Color color = raw_duck.getPixel({x, y});
			if (color.a != 0) {
				_duck_anchor_bx = std::min<int>(_duck_anchor_bx, x);
				_duck_anchor_by = std::min<int>(_duck_anchor_by, y);
			}
		}
	}
}

std::array<int, 2> DuckDeath::size() const {
	return {_level_width * _scale, _level_height * _scale};
}

void DuckDeath::setup(SceneManager &mgr) {}

void DuckDeath::handleEvent(SceneManager &mgr, sf::Event &evt) {}

void DuckDeath::step(SceneManager &mgr) {
	constexpr int pending_duration = 24 * 2.5;
	constexpr int frame_duration = 2;

	if (_pending_timer < pending_duration) {
		_pending_timer++;
		if (_animation_frame == 0) {
			if (_pending_timer == 1) {
				_duck_death_separate_sound.play();
			} else if (_pending_timer == pending_duration - 24) {
				_duck_death_sound.play();
			}
		}
	} else if (_animation_frame + 1 < _animation.length()) {
		_animation_frame++;
		if (_animation_frame == _animation.length() - 1) {
			_pending_timer = -5;
		} else {
			_pending_timer = pending_duration - frame_duration;
		}
	} else {
		_duck_death_sound.stop();
		mgr.changeScene(
			pro::make_proxy<SceneFacade, LevelPlaying>(
				Level::loadFromMetadata(std::move(_level_metadata)), _scale
			)
		);
		return;
	}
}

void DuckDeath::render(
	const SceneManager &mgr, sf::RenderTarget &target
) const {
	target.clear(sf::Color(220, 220, 220, 255));
	int rx = _duck_x - _duck_anchor_bx;
	int ry = _duck_y - _duck_anchor_by;
	_animation.render(target, _animation_frame, rx, ry, _scale);
}

} // namespace wf::scene
