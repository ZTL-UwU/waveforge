#include "wforge/save.h"
#include "wforge/scene.h"
#include <format>
#include <string>

namespace wf::scene {

struct SettingsMenu::Option {
	virtual ~Option() = default;

	virtual std::string displayText() const = 0;
	virtual std::string valueText() const {
		return "";
	}

	virtual bool handleEnter() {
		return false;
	}

	virtual void handleLeft() {}
	virtual void handleRight() {}
};

namespace {

struct ScaleOption : SettingsMenu::Option {
	std::string displayText() const override {
		return "Scale";
	}

	std::string valueText() const override {
		int scale = SaveData::instance().user_settings.scale;
		if (scale == 0) {
			return "Auto";
		} else {
			return std::format("{}x", scale);
		}
	}

	void handleLeft() override {
		auto &settings = SaveData::instance().user_settings;
		if (settings.scale > 0) {
			settings.scale -= 1;
			SaveData::instance().save();
		}
	}

	void handleRight() override {
		constexpr int max_scale = 12;

		auto &settings = SaveData::instance().user_settings;
		if (settings.scale < max_scale) {
			settings.scale += 1;
			SaveData::instance().save();
		}
		SaveData::instance().save();
	}
};

struct ResetSettingsOption : SettingsMenu::Option {
	std::string displayText() const override {
		return "Reset Settings";
	}

	bool handleEnter() override {
		SaveData::instance().resetSettings();
		return false;
	}
};

struct ResetAllOption : SettingsMenu::Option {
	bool comfirmed = false;

	std::string displayText() const override {
		if (comfirmed) {
			return "Reset All(Again to Confirm)";
		} else {
			return "Reset All";
		}
	}

	bool handleEnter() override {
		if (comfirmed) {
			SaveData::instance().resetAll();
			return true;
		} else {
			comfirmed = true;
		}
		return false;
	}
};

struct GoBackOption : SettingsMenu::Option {
	std::string displayText() const override {
		return "Back";
	}

	bool handleEnter() override {
		return true;
	}
};

} // namespace

SettingsMenu::~SettingsMenu() = default;

SettingsMenu::SettingsMenu(int scale)
	: font(AssetsManager::instance().getAsset<PixelFont>("font"))
	, _current_option_index(0) {
	const auto &json_data = AssetsManager::instance().getAsset<nlohmann::json>(
		"ui-config/settings-menu"
	);

	_width = json_data.at("width");
	_height = json_data.at("height");
	_scale = automaticScale(_width, _height, scale);

	_header = UITextDescriptor::fromJson(json_data.at("header"));
	_restart_hint = UITextDescriptor::fromJson(json_data.at("restart-hint"));

	const auto &option_data = json_data.at("option");
	_option_start_pos[0] = option_data.at("x");
	_option_start_pos[1] = option_data.at("y");

	_option_spacing = option_data.at("spacing");
	_option_text_size = option_data.at("size");
	_option_width = option_data.at("width");

	_option_color = sf::Color(
		option_data.at("color").at(0), option_data.at("color").at(1),
		option_data.at("color").at(2), option_data.at("color").at(3)
	);

	_option_active_color = sf::Color(
		option_data.at("active-color").at(0),
		option_data.at("active-color").at(1),
		option_data.at("active-color").at(2),
		option_data.at("active-color").at(3)
	);

	_options.push_back(std::make_unique<ScaleOption>());
	_options.push_back(std::make_unique<ResetSettingsOption>());
	_options.push_back(std::make_unique<ResetAllOption>());
	_options.push_back(std::make_unique<GoBackOption>());
}

std::array<int, 2> SettingsMenu::size() const {
	return {_width * _scale, _height * _scale};
}

void SettingsMenu::setup(SceneManager &mgr) {
	// nothing to do here
}

void SettingsMenu::handleEvent(SceneManager &mgr, sf::Event &evt) {
	if (auto kb = evt.getIf<sf::Event::KeyPressed>()) {
		switch (kb->code) {
		case sf::Keyboard::Key::Escape:
			mgr.changeScene(pro::make_proxy<SceneFacade, MainMenu>(_scale));
			return;

		case sf::Keyboard::Key::Tab:
			if (kb->shift) {
				if (_current_option_index > 0) {
					_current_option_index--;
				} else {
					_current_option_index = _options.size() - 1;
				}
			} else {
				if (_current_option_index + 1 < _options.size()) {
					_current_option_index++;
				} else {
					_current_option_index = 0;
				}
			}

		case sf::Keyboard::Key::Up:
		case sf::Keyboard::Key::W:
			if (_current_option_index > 0) {
				_current_option_index--;
			}
			break;

		case sf::Keyboard::Key::Down:
		case sf::Keyboard::Key::S:
			if (_current_option_index + 1 < _options.size()) {
				_current_option_index++;
			}
			break;

		case sf::Keyboard::Key::Left:
		case sf::Keyboard::Key::A:
			_options[_current_option_index]->handleLeft();
			break;

		case sf::Keyboard::Key::Right:
		case sf::Keyboard::Key::D:
			_options[_current_option_index]->handleRight();
			break;

		case sf::Keyboard::Key::Enter:
		case sf::Keyboard::Key::Space:
			if (_options[_current_option_index]->handleEnter()) {
				// go back to main menu
				mgr.changeScene(pro::make_proxy<SceneFacade, MainMenu>(_scale));
				return;
			}
			break;

		default:
			break;
		}
	}
}

void SettingsMenu::step(SceneManager &mgr) {
	// nothing to do here
}

void SettingsMenu::render(
	const SceneManager &mgr, sf::RenderTarget &target
) const {
	// Render header & restart hint
	_header.render(target, font, "Settings", _scale);
	_restart_hint.render(target, font, "some changes require restart", _scale);

	// Render options
	for (size_t i = 0; i < _options.size(); ++i) {
		const auto &option = _options[i];
		sf::Color color = (i == _current_option_index)
			? _option_active_color
			: _option_color;

		std::string option_text = option->displayText();
		std::string value_text = option->valueText();

		int text_opt_x = _option_start_pos[0];
		int text_y = _option_start_pos[1] + i * _option_spacing;
		font.renderText(
			target, option_text, color, text_opt_x, text_y, _scale,
			_option_text_size
		);

		// Right-aligned value text
		if (value_text.empty()) {
			continue;
		}

		value_text = "<" + value_text + ">";
		int value_text_width = value_text.size()
			* font.charWidth(_option_text_size);
		int border_x = _option_start_pos[0] + _option_width;
		int text_value_x = border_x - value_text_width;
		font.renderText(
			target, value_text, color, text_value_x, text_y, _scale,
			_option_text_size
		);
	}
}

} // namespace wf::scene
