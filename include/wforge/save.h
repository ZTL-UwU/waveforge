#ifndef WFORGE_SAVE_H
#define WFORGE_SAVE_H

#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>

namespace wf {

struct UserSettings {
	int scale;

	static UserSettings defaultSettings() noexcept;
};

struct SaveData {
	int completed_levels;
	UserSettings user_settings;

	static SaveData &instance() noexcept;
	void save() const;
	void resetSettings();
	void resetAll();

private:
	SaveData();
};

} // namespace wf

#endif // WFORGE_SAVE_H
