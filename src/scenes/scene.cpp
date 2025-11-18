#include "wforge/scene.h"
#include "wforge/assets.h"
#include "wforge/xoroshiro.h"
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Window.hpp>
#include <random>

namespace wf {

namespace {

sf::RenderWindow createWindow(Scene &scene) {
	auto [width, height] = scene->size();
	sf::Vector2u window_size(width, height);
	return sf::RenderWindow(
		sf::VideoMode(window_size), "Waveforge Demo",
		sf::Style::Titlebar | sf::Style::Close
	);
}

} // namespace

SceneManager::SceneManager(Scene initial_scene)
	: window(createWindow(initial_scene))
	, _current_scene(std::move(initial_scene))
	, _bgm_collection(nullptr)
	, _cur_bgm(nullptr) {
	window.setFramerateLimit(24);
	_current_scene->init(*this);
}

SceneManager::~SceneManager() {
	if (_cur_bgm) {
		_cur_bgm->stop();
	}
	window.close();
}

void SceneManager::changeScene(Scene new_scene) {
	unsetBGMCollection();
	_cur_bgm->stop();
	_cur_bgm = nullptr;

	auto [old_width, old_height] = _current_scene->size();
	_current_scene = std::move(new_scene);
	auto [width, height] = _current_scene->size();
	if (width != old_width || height != old_height) {
		window.setSize(sf::Vector2u(width, height));
	}
	_current_scene->init(*this);
}

void SceneManager::handleEvent(sf::Event &evt) {
	_current_scene->handleEvent(*this, evt);
}

void SceneManager::tick() {
	_current_scene->tick(*this, window);

	if (_bgm_collection && !_bgm_collection->music.empty()) {
		if (!_cur_bgm || _cur_bgm->getStatus() == sf::Music::Status::Stopped) {
			if (_bgm_collection->music.size() == 1) {
				// quick path for single-track collections
				_cur_bgm = _bgm_collection->music.front();
			} else {
				// randomly select a new track
				auto &rng = Xoroshiro128PP::globalInstance();
				std::uniform_int_distribution<std::size_t> dist(
					0, _bgm_collection->music.size() - 1
				);
				_cur_bgm = _bgm_collection->music[dist(rng)];
			}
			_cur_bgm->play();
		}
	}

	window.display();
}

void SceneManager::setBGMCollection(const std::string &collection_id) {
	_bgm_collection = &AssetsManager::instance().getMusicCollection(
		collection_id
	);
}

void SceneManager::unsetBGMCollection() {
	_bgm_collection = nullptr;
}

sf::Vector2i SceneManager::mousePosition() const {
	return sf::Mouse::getPosition(window);
}

} // namespace wf
