#ifndef WFORGE_SAVE_H
#define WFORGE_SAVE_H

namespace wf {

struct UserSettings {
	int scale;
	int global_volume;

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
