#include "setting_profiles.h"
#include "settings.h"
#include <fstream>
#include <sstream>

#if defined(_WIN32) && __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
    #define USE_STD_FILESYSTEM
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos || end == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// ------------------- SettingsProfile -------------------

SettingsProfile::SettingsProfile(const std::string &filepath) {
#ifdef USE_STD_FILESYSTEM
    name = fs::path(filepath).stem().string();
#else
    auto filename = filepath.substr(filepath.find_last_of("/\\") + 1);
    name = filename.substr(0, filename.find_last_of('.'));
#endif
    LoadFromFile(filepath);
}

bool SettingsProfile::LoadFromFile(const std::string &filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        settings[key] = value;
    }
    return true;
}

bool SettingsProfile::LoadFromGSettings()
{
	std::vector<std::string> setting_keys = g_settings->getNames();
	for (const auto &key : setting_keys) {
		std::string value = g_settings->get(key);
		settings[key] = value;
	}
	return true;
}

bool SettingsProfile::SaveToFile(const std::string &filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    for (const auto &pair : settings) {
        file << pair.first << " = " << pair.second << "\n";
    }

    return file.good();
}

bool SettingsProfile::SaveToGSettings() const
{
	for (const auto &pair : settings) {
		g_settings->set(pair.first, pair.second);
	}
	return true;
}

// ------------------- ProfilesManager -------------------

void ProfilesManager::LoadProfiles(const std::string &folderpath) {
    profiles.clear();

#ifdef USE_STD_FILESYSTEM
    for (const auto &entry : fs::directory_iterator(folderpath)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".profile") continue;

        SettingsProfile profile(entry.path().string());
        if (!profile.name.empty())
            profiles.push_back(profile);
    }
#else
    DIR *dir = opendir(folderpath.c_str());
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename == "." || filename == "..") continue;
        if (filename.size() < 8 || filename.substr(filename.size() - 8) != ".profile") continue;

        std::string fullpath = folderpath + "/" + filename;

        struct stat st;
        if (stat(fullpath.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;

        SettingsProfile profile(fullpath);
        if (!profile.name.empty())
            profiles.push_back(profile);
    }

    closedir(dir);
#endif
}

void ProfilesManager::SaveProfiles(const std::string &folderpath) const
{
	for (const auto &profile : profiles) {
#ifdef USE_STD_FILESYSTEM
		std::string filepath = (fs::path(folderpath) / (profile.name + ".profile")).string();
#else
		std::string filepath = folderpath + "/" + profile.name + ".profile";
#endif
		profile.SaveToFile(filepath);
	}
}

std::vector<std::string> ProfilesManager::GetProfiles() const {
    std::vector<std::string> names;
    names.reserve(profiles.size());
    for (const auto &profile : profiles)
        names.push_back(profile.name);
    return names;
}

SettingsProfile ProfilesManager::GetProfileByName(const std::string &name) const {
    for (const auto &profile : profiles) {
        if (profile.name == name)
            return profile;
    }
    return SettingsProfile(); // placeholder if not found
}