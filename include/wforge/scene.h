#ifndef WFORGE_SCENE_H
#define WFORGE_SCENE_H

#include "wforge/assets.h"
#include "wforge/fallsand.h"
#include "wforge/level.h"
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>
#include <proxy/v4/proxy.h>
#include <proxy/v4/proxy_macros.h>
#include <vector>

namespace wf {

namespace _dispatch {

PRO_DEF_MEM_DISPATCH(MemSize, size);
PRO_DEF_MEM_DISPATCH(MemHandleEvent, handleEvent);

}; // namespace _dispatch

class SceneManager;

/* clang-format off */
struct SceneFacade : pro::facade_builder
	::add_convention<_dispatch::MemSize, std::array<int, 2>() const>
	::add_convention<_dispatch::MemSetup, void(SceneManager &)>
	::add_convention<_dispatch::MemHandleEvent, void(SceneManager &, sf::Event &)>
	::add_convention<_dispatch::MemStep, void(SceneManager &)>
	::add_convention<_dispatch::MemRender, void(const SceneManager &, sf::RenderTarget &) const>
	::build {};
/* clang-format on */

using Scene = pro::proxy<SceneFacade>;

int automaticScale(int width, int height, int scale_configured = 0);

class SceneManager {
public:
	SceneManager(Scene initial_scene);
	~SceneManager();

	void changeScene(Scene new_scene);
	void handleEvent(sf::Event &evt);
	void tick();

	void setBGMCollection(const std::string &collection_id);
	void unsetBGMCollection();

	sf::Vector2i mousePosition() const;

	sf::RenderWindow window;

private:
	Scene _current_scene;

	MusicCollection *_bgm_collection;
	sf::Music *_cur_bgm;
	bool _scene_changed;
};

namespace scene {

struct LevelPlaying {
	LevelPlaying(Level level);
	LevelPlaying(Level level, int scale);

	std::array<int, 2> size() const;
	void setup(SceneManager &mgr);
	void handleEvent(SceneManager &mgr, sf::Event &evt);
	void step(SceneManager &mgr);
	void render(const SceneManager &mgr, sf::RenderTarget &target) const;

private:
	void _restartLevel(SceneManager &mgr, bool is_failed = true);

	int _scale;
	Level _level;
	mutable LevelRenderer _renderer;
	int _hint_type;
	int _hint_opacity;
	PixelFont &font;
};

struct DuckDeath {
	DuckDeath(
		int level_width, int level_height, int duck_x, int duck_y, int scale,
		LevelMetadata level_metadata
	);

	std::array<int, 2> size() const;
	void setup(SceneManager &mgr);
	void handleEvent(SceneManager &mgr, sf::Event &evt);
	void step(SceneManager &mgr);
	void render(const SceneManager &mgr, sf::RenderTarget &target) const;

private:
	int _level_width;
	int _level_height;

	int _duck_x;
	int _duck_y;
	int _scale;
	int _duck_anchor_bx;
	int _duck_anchor_by;

	int _tick;
	int _animation_frame;

	int _total_duration;
	int _animation_start;
	int _animation_frame_duration;
	int _sperate_sfx_start;
	int _reborn_sfx_start;

	LevelMetadata _level_metadata;
	PixelAnimationFrames &_animation;
	sf::Sound _reborn_sound;
	sf::Sound _duck_death_separate_sound;
};

struct LevelComplete {
	LevelComplete(
		int level_width, int level_height, int duck_x, int duck_y, int scale
	);

	std::array<int, 2> size() const;
	void setup(SceneManager &mgr);
	void handleEvent(SceneManager &mgr, sf::Event &evt);
	void step(SceneManager &mgr);
	void render(const SceneManager &mgr, sf::RenderTarget &target) const;

private:
	int _level_width;
	int _level_height;
	int _scale;
	int _pending_timer;
	int _current_step;
	bool _display_text;
	const PixelFont &font;
	sf::Texture &_duck_texture;
	std::vector<std::array<int, 2>> _step_positions;
	int _top_left_x;
};

struct LevelSelectionMenu {
	LevelSelectionMenu(int scale);

	std::array<int, 2> size() const;
	void setup(SceneManager &mgr);
	void handleEvent(SceneManager &mgr, sf::Event &evt);
	void step(SceneManager &mgr);
	void render(const SceneManager &mgr, sf::RenderTarget &target) const;

private:
	int _scale;
	int _selected_index;
	const LevelSequence &_level_seq;

	int _width;
	int _height;
	const PixelFont &font;

	struct TextDescriptor {
		int x;
		int y;
		int size;
		sf::Color color;
	};

	TextDescriptor _header;
	TextDescriptor _level_button_text;
	TextDescriptor _level_title;
	TextDescriptor _level_desc;
	TextDescriptor _enter_hint;
	std::vector<std::array<int, 2>> _level_button;
	std::vector<std::array<int, 2>> _level_links;
	std::array<int, 2> _duck_rel;

	sf::Texture *_duck_texture;
	sf::Texture *_level_button_texture_normal;
	sf::Texture *_level_button_texture_selected;
	sf::Texture *_level_button_texture_locked;
	sf::Texture *_level_link_texture_activated;
	sf::Texture *_level_link_texture_locked;
};

struct MainMenu {
	MainMenu(int scale);

	std::array<int, 2> size() const;
	void setup(SceneManager &mgr);
	void handleEvent(SceneManager &mgr, sf::Event &evt);
	void step(SceneManager &mgr);
	void render(const SceneManager &mgr, sf::RenderTarget &target) const;

private:
	int _scale;
	int _width;
	int _height;
	const PixelFont &font;
	sf::Texture *_background_texture;

	struct ButtonDescriptor {
		int x;
		int y;
		int size;
		sf::Color color;
		sf::Color active_color;
	};

	int _current_button_index;
	ButtonDescriptor _play_button;
	ButtonDescriptor _settings_button;
	ButtonDescriptor _exit_button;
};

} // namespace scene

} // namespace wf

#endif // WFORGE_SCENE_H
