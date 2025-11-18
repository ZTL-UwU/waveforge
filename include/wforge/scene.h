#ifndef WFORGE_SCENE_H
#define WFORGE_SCENE_H

#include "wforge/assets.h"
#include "wforge/level.h"
#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>
#include <proxy/v4/proxy.h>
#include <proxy/v4/proxy_macros.h>

namespace wf {

namespace _dispatch {

PRO_DEF_MEM_DISPATCH(MemSize, size);
PRO_DEF_MEM_DISPATCH(MemInit, init);
PRO_DEF_MEM_DISPATCH(MemHandleEvent, handleEvent);
PRO_DEF_MEM_DISPATCH(MemTick, tick);

}; // namespace _dispatch

class SceneManager;

/* clang-format off */
struct SceneFacade : pro::facade_builder
	::add_convention<_dispatch::MemSize, std::array<int, 2>() const>
	::add_convention<_dispatch::MemInit, void(SceneManager &)>
	::add_convention<_dispatch::MemHandleEvent, void(SceneManager &, sf::Event &)>
	::add_convention<_dispatch::MemTick, void(SceneManager &, sf::RenderTarget &)>
	::build {};
/* clang-format on */

using Scene = pro::proxy<SceneFacade>;

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
};

namespace scene {

struct LevelScene {
	LevelScene(Level level);
	LevelScene(Level level, int scale);

	std::array<int, 2> size() const;
	void init(SceneManager &mgr);
	void handleEvent(SceneManager &mgr, sf::Event &evt);
	void tick(SceneManager &mgr, sf::RenderTarget &target);

private:
	int _scale;
	Level _level;
	LevelRenderer _renderer;
	PixelFont &font;
};

} // namespace scene

} // namespace wf

#endif // WFORGE_SCENE_H
