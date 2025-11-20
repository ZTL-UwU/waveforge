#ifndef WFORGE_SAVE_H
#define WFORGE_SAVE_H

#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>

namespace wf {

struct UserSettings {
	// future settings can go here
	int test_field;

	static UserSettings defaultSettings() noexcept;
};

struct SaveData {
	int completed_levels;
	UserSettings user_settings;

	static SaveData &instance() noexcept;
	void save() const;

private:
	SaveData();
};

} // namespace wf

#endif // WFORGE_SAVE_H
