#include "wforge/assets.h"
#include "wforge/scene.h"
#include <nlohmann/json.hpp>

namespace wf::scene {

KeyGuide::KeyGuide() {
	const auto &json_data = AssetsManager::instance().getAsset<nlohmann::json>(
		"ui-config/key-guide"
	);

	_width = json_data.at("width");
	_height = json_data.at("height");

	_background_texture = &AssetsManager::instance().getAsset<sf::Texture>(
		"ui/key-guide"
	);
}

std::array<int, 2> KeyGuide::size() const {
	return {_width, _height};
}

void KeyGuide::handleEvent(SceneManager &mgr, sf::Event &evt) {
	if (auto kb = evt.getIf<sf::Event::KeyPressed>()) {
		switch (kb->code) {
		case sf::Keyboard::Key::Escape:
		case sf::Keyboard::Key::Enter:
		case sf::Keyboard::Key::Space:
			UISounds::instance().forward.play();
			mgr.changeScene(pro::make_proxy<SceneFacade, MainMenu>());
			return;

		default:
			break;
		}
	}
}

void KeyGuide::setup(SceneManager &mgr) {}
void KeyGuide::step(SceneManager &mgr) {}

void KeyGuide::render(
	const SceneManager &mgr, sf::RenderTarget &target, int scale
) const {
	sf::Sprite background_sprite(*_background_texture);
	background_sprite.setPosition(sf::Vector2f(0, 0));
	background_sprite.setScale(sf::Vector2f(scale, scale));

	target.draw(background_sprite);
}

} // namespace wf::scene