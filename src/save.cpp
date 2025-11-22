#include "wforge/save.h"
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace wf {

namespace {

fs::path resolveSaveFilePath() noexcept {
	fs::path result = fs::current_path(); // default path

	// Try platform-specific paths
#ifdef _WIN32
	if (const char *appdata = std::getenv("APPDATA")) {
		result = fs::path(appdata) / "waveforge";
	}
#elifdef __APPLE__
	if (const char *home = std::getenv("HOME")) {
		result = fs::path(home) / "Library" / "Application Support"
			/ "waveforge";
	}
#elifdef __linux__
	if (const char *xdg_config = std::getenv("XDG_CONFIG_HOME")) {
		result = fs::path(xdg_config) / "waveforge";
	} else if (const char *home = std::getenv("HOME")) {
		result = fs::path(home) / ".config" / "waveforge";
	}
#else
	std::cerr << "Warning: unknown platform, using current directory for "
				 "save file.\n";
#endif

	if (!fs::exists(result)) {
		try {
			fs::create_directories(result);
		} catch (std::exception &e) {
			std::cerr << "Warning: failed to create save directory '"
					  << result.string() << "': " << e.what()
					  << "\nUsing current directory for save file.\n";
			result = fs::current_path();
		}
	}
	return result / "save.json";
}

fs::path saveFilePath() noexcept {
	// cache the resolved path
	static fs::path path = resolveSaveFilePath();
	return path;
}

void loadUserSettings(UserSettings &settings, nlohmann::json &json_data) {
	settings.test_field = json_data.value("test_field", 0);
}

void loadSaveData(SaveData &data, nlohmann::json &json_data) {
	data.completed_levels = json_data.value("completed_levels", 0);
	loadUserSettings(data.user_settings, json_data.at("user_settings"));
}

} // namespace

SaveData &SaveData::instance() noexcept {
	static SaveData *instance = nullptr;
	if (instance == nullptr) {
		auto path = saveFilePath();
		instance = new SaveData();
		if (fs::exists(path)) {
			std::ifstream file(path);
			if (!file.is_open()) {
				std::cerr << "Failed to open save file at '" << path.string()
						  << "'. Using default save data.\n";
			} else {
				try {
					auto json_data = nlohmann::json::parse(file);
					loadSaveData(*instance, json_data);
				} catch (std::exception &e) {
					std::cerr << "Failed to load save data from file '"
							  << path.string() << "': " << e.what()
							  << "\nUsing default save data.\n";
				}
			}
		}

		// Please don't relay on this saving mechanism
		// it only works as an extra safeguard against data loss
		// Please save manually whenever change is made
		std::atexit([]() {
			instance->save();
		});
	}
	return *instance;
}

void SaveData::save() const {
	auto path = saveFilePath();
	nlohmann::json json_data;
	json_data["completed_levels"] = completed_levels;
	json_data["user_settings"] = {
		{"test_field", user_settings.test_field},
	};

	std::ofstream file(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open save file at '" << path.string()
				  << "' for writing. Save data not saved.\n";
		return;
	}

	try {
		file << json_data.dump();
	} catch (std::exception &e) {
		std::cerr << "Failed to write save data to file '" << path.string()
				  << "': " << e.what() << "\n";
	}
}

void SaveData::resetSettings() {
	user_settings = UserSettings::defaultSettings();
	save();
}

void SaveData::resetAll() {
	completed_levels = 0;
	user_settings = UserSettings::defaultSettings();
	save();
}

UserSettings UserSettings::defaultSettings() noexcept {
	return UserSettings{
		.test_field = 0,
	};
}

SaveData::SaveData()
	: completed_levels(0), user_settings(UserSettings::defaultSettings()) {}

} // namespace wf
