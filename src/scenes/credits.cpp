#include "wforge/assets.h"
#include "wforge/audio.h"
#include "wforge/scene.h"
#include <nlohmann/json.hpp>
#include <string>

namespace wf::scene {

Credits::Credits()
	: font(AssetsManager::instance().getAsset<PixelFont>("font")) {
	const auto &json_data = AssetsManager::instance().getAsset<nlohmann::json>(
		"ui-config/credits"
	);

	_width = json_data.at("width");
	_height = json_data.at("height");

	_header = UITextDescriptor::fromJson(json_data.at("header"));

	const auto &credits_data = json_data.at("credits");
	_credits_pos[0] = credits_data.at("x");
	_credits_pos[1] = credits_data.at("y");
	_credits_size = credits_data.at("size");
	_credits_spacing = credits_data.at("spacing");
	_credits_width = credits_data.at("width");
	_credits_color = sf::Color(
		credits_data.at("color").at(0), credits_data.at("color").at(1),
		credits_data.at("color").at(2), credits_data.at("color").at(3)
	);

	for (const auto &row : json_data.at("content")) {
		// Expect each row to be an array of two strings: [label, value]
		if (row.size() >= 2) {
			_content.emplace_back(
				row.at(0).get<std::string>(), row.at(1).get<std::string>()
			);
		}
	}
}

std::array<int, 2> Credits::size() const {
	return {_width, _height};
}

void Credits::setup(SceneManager &mgr) {
	// nothing to do
}

void Credits::handleEvent(SceneManager &mgr, sf::Event &evt) {
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

void Credits::step(SceneManager &mgr) {
	// no-op
}

void Credits::render(
	const SceneManager &mgr, sf::RenderTarget &target, int scale
) const {
	// Render header
	_header.render(target, font, "Credits", scale);

	// Render content rows
	for (size_t i = 0; i < _content.size(); ++i) {
		const auto &p = _content[i];
		const std::string &left = p.first;
		const std::string &right = p.second;

		int text_x = _credits_pos[0];
		int text_y = _credits_pos[1] + static_cast<int>(i) * _credits_spacing;

		// left label
		font.renderText(
			target, left, _credits_color, text_x, text_y, scale, _credits_size
		);

		// right value (right-aligned within width)
		int value_text_width = static_cast<int>(right.size())
			* font.charWidth(_credits_size);
		int border_x = _credits_pos[0] + _credits_width;
		int text_value_x = border_x - value_text_width;
		font.renderText(
			target, right, _credits_color, text_value_x, text_y, scale,
			_credits_size
		);
	}
}

} // namespace wf::scene
