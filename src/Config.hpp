#ifndef FREESHOP_CONFIG_HPP
#define FREESHOP_CONFIG_HPP

#include <string>
#include <rapidjson/document.h>

#ifndef FREESHOP_VERSION
#error "No version defined"
#endif

namespace FreeShop {

class Config {
public:
	// See string deifnitions in Config.coo
	enum Key {
		CacheVersion,
		TriggerUpdateFlag,
		ShowNews,
		// Filter
		FilterRegion,
		FilterGenre,
		FilterLanguage,
		FilterPlatform,
		// Sort

		// Update
		AutoUpdate,
		LastUpdatedTime,
		DownloadTitleKeys,
		KeyURLs,
		// Download
		DownloadTimeout,
		DownloadBufferSize,
		PlaySoundAfterDownload,
		PowerOffAfterDownload,
		PowerOffTime,
		// Other
		SleepMode,
		Language,

		KEY_COUNT,
	};

	static Config& getInstance();

	void loadDefaults();

public:
	static bool loadFromFile(const std::string& filename = FREESHOP_DIR "/config.json");
	static void saveToFile(const std::string& filename = FREESHOP_DIR "/config.json");

	static bool keyExists(const char *key);

	static const rapidjson::Value &get(Key key);
	static rapidjson::Document::AllocatorType &getAllocator();

	template <typename T>
	static void set(Key key, T val)
	{
		rapidjson::Value v(val);
		set(key, v);
	}

	static void set(Key key, const char *val);
	static void set(Key key, rapidjson::Value &val);

private:
	Config();

	rapidjson::Document m_json;
};

} // namespace FreeShop

#endif // FREESHOP_CONFIG_HPP
