#include <cpp3ds/System/FileSystem.hpp>
#include <fstream>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Config.hpp"
#include "Util.hpp"

namespace {
	// Matches with Config::Key enum in Config.hpp
	const char *keyStrings[] = {
		"cache_version",
		"trigger_update",
		"show_news",
		// Filter
		"filter_region",
		"filter_genre",
		"filter_language",
		"filter_platform",
		// Sort
		// Update
		"auto-update",
		"last_updated",
		"download_title_keys",
		"key_urls",
		// Download
		"download_timeout",
		"download_buffer_size",
		"play_sound_after_download",
		"power_off_after_download",
		"power_off_time",
		// Other
		"sleep_mode",
		"language",
	};
}

namespace FreeShop {

Config::Config()
{
	static_assert(KEY_COUNT == sizeof(keyStrings)/sizeof(*keyStrings), "Key string count must match the enum count.");
	loadDefaults();
}

Config &Config::getInstance()
{
	static Config config;
	return config;
}

bool Config::loadFromFile(const std::string &filename)
{
	rapidjson::Document &json = getInstance().m_json;
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::string jsonString;
	cpp3ds::FileInputStream file;
	if (!file.open(filename))
		return false;
	jsonString.resize(file.getSize());
	file.read(&jsonString[0], jsonString.size());
	json.Parse(jsonString.c_str());
	getInstance().loadDefaults();
	return !json.HasParseError();
}

void Config::saveToFile(const std::string &filename)
{
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::ofstream file(path);
	rapidjson::OStreamWrapper osw(file);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	getInstance().m_json.Accept(writer);
}

bool Config::keyExists(const char *key)
{
	return getInstance().m_json.HasMember(key);
}

const rapidjson::Value &Config::get(Key key)
{
	return getInstance().m_json[keyStrings[key]];
}

#define ADD_DEFAULT(key, val) \
	if (!m_json.HasMember(keyStrings[key])) \
		m_json.AddMember(rapidjson::StringRef(keyStrings[key]), val, m_json.GetAllocator());

void Config::loadDefaults()
{
	if (!m_json.IsObject())
		m_json.SetObject();

	ADD_DEFAULT(CacheVersion, "");
	ADD_DEFAULT(TriggerUpdateFlag, false);
	ADD_DEFAULT(ShowNews, true);

	// Filter
	ADD_DEFAULT(FilterRegion, rapidjson::kArrayType);
	ADD_DEFAULT(FilterGenre, rapidjson::kArrayType);
	ADD_DEFAULT(FilterLanguage, rapidjson::kArrayType);
	ADD_DEFAULT(FilterPlatform, rapidjson::kArrayType);

	// Update
	ADD_DEFAULT(AutoUpdate, true);
	ADD_DEFAULT(LastUpdatedTime, 0);
	ADD_DEFAULT(DownloadTitleKeys, false);
	ADD_DEFAULT(KeyURLs, rapidjson::kArrayType);

	// Download
	ADD_DEFAULT(DownloadTimeout, 6.f);
	ADD_DEFAULT(DownloadBufferSize, 128u);
	ADD_DEFAULT(PlaySoundAfterDownload, true);
	ADD_DEFAULT(PowerOffAfterDownload, false);
	ADD_DEFAULT(PowerOffTime, 120);

	// Other
	ADD_DEFAULT(SleepMode, true);
	ADD_DEFAULT(Language, "auto");
}

void Config::set(Key key, const char *val)
{
	rapidjson::Value v(val, getInstance().m_json.GetAllocator());
	set(key, v);
}

void Config::set(Key key, rapidjson::Value &val)
{
	const char *keyStr = keyStrings[key];
	if (keyExists(keyStr))
		getInstance().m_json[keyStr] = val;
	else
		getInstance().m_json.AddMember(rapidjson::StringRef(keyStr), val, getInstance().m_json.GetAllocator());
}

rapidjson::Document::AllocatorType &Config::getAllocator()
{
	return getInstance().m_json.GetAllocator();
}


} // namespace FreeShop
