#include "wforge/scene.h"
#include "wforge/assets.h"
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Window.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

namespace wf {

void UITextDescriptor::render(
	sf::RenderTarget &target, const PixelFont &font, const std::string &text,
	int scale
) const {
	font.renderText(target, text, color, x, y, scale, size);
}

UITextDescriptor UITextDescriptor::fromJson(const nlohmann::json &json_data) {
	UITextDescriptor desc;
	desc.x = json_data.at("x");
	desc.y = json_data.at("y");
	desc.size = json_data.at("size");
	desc.color = sf::Color(
		json_data.at("color").at(0), json_data.at("color").at(1),
		json_data.at("color").at(2), json_data.at("color").at(3)
	);
	return desc;
}

namespace {

sf::RenderWindow createWindow(Scene &scene) {
	auto [width, height] = scene->size();
	sf::Vector2u window_size(width, height);
	sf::RenderWindow window(
		sf::VideoMode(window_size), "Waveforge 0.1alpha",
		sf::Style::Titlebar | sf::Style::Close
	);
	window.setFramerateLimit(24);
	return window;
}

} // namespace

int automaticScale(int width, int height, int scale_configured) {
	if (scale_configured > 0) {
		return scale_configured;
	}

	auto player_screen_size = sf::VideoMode::getDesktopMode().size;
	int res = std::max<int>(
		1,
		std::min<int>(
			player_screen_size.x / width - 1, player_screen_size.y / height - 1
		)
	);

	std::cerr << "Automatic scale selected: " << res << "x\n";
	return res;
}

SceneManager::SceneManager(Scene initial_scene)
	: window(createWindow(initial_scene))
	, _current_scene(std::move(initial_scene))
	, _scene_changed(false) {
	_current_scene->setup(*this);
}

SceneManager::~SceneManager() {
	window.close();
}

void SceneManager::changeScene(Scene new_scene) {
	auto [old_width, old_height] = _current_scene->size();
	_current_scene = std::move(new_scene);
	auto [width, height] = _current_scene->size();
	if (width != old_width || height != old_height) {
		window.close();
		window = createWindow(_current_scene);
	}
	_current_scene->setup(*this);
	_scene_changed = true;
}

void SceneManager::handleEvent(sf::Event &evt) {
	_current_scene->handleEvent(*this, evt);
}

void SceneManager::tick() {
	_scene_changed = false;
	_current_scene->step(*this);
	if (_scene_changed) {
		return;
	}

	bgm.step();
	window.clear(sf::Color::White);
	_current_scene->render(*this, window);
	window.display();
}

sf::Vector2i SceneManager::mousePosition() const {
	return sf::Mouse::getPosition(window);
}

} // namespace wf
