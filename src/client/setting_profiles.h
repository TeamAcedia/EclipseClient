#pragma once

#include <vector>
#include <string>
#include "irrlichttypes_bloated.h"

class SettingsProfile {
public:
	std::string name;
    std::unordered_map<std::string, std::string> settings;

	SettingsProfile() = default;
	explicit SettingsProfile(const std::string &data);

	bool LoadFromFile(const std::string &filepath);
	bool LoadFromGSettings();
	bool SaveToFile(const std::string &filepath) const;
	bool SaveToGSettings() const;
};

class ProfilesManager {
public:
	// Load all .profile files in a folder
	void LoadProfiles(const std::string &folderpath);

	// Save all profiles to a folder
	void SaveProfiles(const std::string &folderpath) const;

	// Return available profile names
	std::vector<std::string> GetProfiles() const;

	// Get a profile by name (returns a placeholder profile if not found)
	SettingsProfile GetProfileByName(const std::string &name) const;

private:
	std::vector<SettingsProfile> profiles;
};
