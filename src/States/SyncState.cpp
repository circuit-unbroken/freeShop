#include "SyncState.hpp"
#include "../Download.hpp"
#include "../Installer.hpp"
#include "../Util.hpp"
#include "../AssetManager.hpp"
#include "../Config.hpp"
#include "../TitleKeys.hpp"
#include "../FreeShop.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include <sys/stat.h>
#include <fstream>
#include <cpp3ds/System/Service.hpp>
#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <cpp3ds/System/FileInputStream.hpp>

namespace {

int copy_data(struct archive *ar, struct archive *aw)
{
	int r;
	const void *buff;
	size_t size;
	int64_t offset;

	while (1)
	{
		r = archive_read_data_block(ar, &buff, &size, &offset);
		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (r < ARCHIVE_OK)
			return (r);
		r = archive_write_data_block(aw, buff, size, offset);
		if (r < ARCHIVE_OK)
		{
			fprintf(stderr, "%s\n", archive_error_string(aw));
			return (r);
		}
	}
}

bool extract(const std::string &filename, const std::string &destDir)
{
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int r = ARCHIVE_FAILED;

	a = archive_read_new();
	archive_read_support_format_tar(a);
	archive_read_support_compression_gzip(a);
	ext = archive_write_disk_new();
	if ((r = archive_read_open_filename(a, cpp3ds::FileSystem::getFilePath(filename).c_str(), 32*1024))) {
		std::cout << "failure! " << r << std::endl;
		return r;
	}

	while(1)
	{
		r = archive_read_next_header(a, &entry);

		if (r == ARCHIVE_EOF)
			break;
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(a));
		// TODO: handle these fatal error
		std::string path = cpp3ds::FileSystem::getFilePath(destDir + archive_entry_pathname(entry));

		if (FreeShop::pathExists(path.c_str(), false))
			unlink(path.c_str());

		archive_entry_set_pathname(entry, path.c_str());
		r = archive_write_header(ext, entry);
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(ext));
		else if (archive_entry_size(entry) > 0)
		{
			r = copy_data(a, ext);

			if (r < ARCHIVE_OK)
				fprintf(stderr, "%s\n", archive_error_string(ext));
		}
		r = archive_write_finish_entry(ext);
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(ext));
	}

	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);

	return r == ARCHIVE_EOF;
}

}

namespace FreeShop {

bool g_syncComplete = false;
bool g_browserLoaded = false;

SyncState::SyncState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
, m_threadSync(&SyncState::sync, this)
, m_threadStartupSound(&SyncState::startupSound, this)
{
	g_syncComplete = false;
	g_browserLoaded = false;

	m_soundStartup.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get("sounds/startup.ogg"));

	m_soundLoading.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get("sounds/loading.ogg"));
	m_soundLoading.setLoop(true);

	m_textStatus.setCharacterSize(14);
	m_textStatus.setFillColor(cpp3ds::Color::Black);
	m_textStatus.setOutlineColor(cpp3ds::Color(0, 0, 0, 70));
	m_textStatus.setOutlineThickness(2.f);
	m_textStatus.setPosition(160.f, 155.f);
	m_textStatus.useSystemFont();
	TweenEngine::Tween::to(m_textStatus, util3ds::TweenText::FILL_COLOR_ALPHA, 0.15f)
			.target(180)
			.repeatYoyo(-1, 0)
			.start(m_tweenManager);

	m_threadSync.launch();
	m_threadStartupSound.launch();
}


SyncState::~SyncState()
{
	m_threadSync.wait();
	m_threadStartupSound.wait();
}


void SyncState::renderTopScreen(cpp3ds::Window& window)
{
	// Nothing
}

void SyncState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_textStatus);
}

bool SyncState::update(float delta)
{
	m_tweenManager.update(delta);
	return true;
}

bool SyncState::processEvent(const cpp3ds::Event& event)
{
	return true;
}

void SyncState::sync()
{
	m_timer.restart();

	if (!cpp3ds::Service::isEnabled(cpp3ds::Httpc))
	{
		while (!cpp3ds::Service::isEnabled(cpp3ds::Httpc) && m_timer.getElapsedTime() < cpp3ds::seconds(30.f))
		{
			setStatus(_("Waiting for internet connection... %.0fs", 30.f - m_timer.getElapsedTime().asSeconds()));
			cpp3ds::sleep(cpp3ds::milliseconds(200));
		}
		if (!cpp3ds::Service::isEnabled(cpp3ds::Httpc))
		{
			requestStackClear();
			return;
		}
	}

	// If auto-dated, boot into launch newest freeShop
#ifdef NDEBUG
	if (updateFreeShop())
	{
		Config::set(Config::ShowNews, true);
		g_requestJump = 0x400000F12EE00;
		return;
	}
#endif

	updateCache();
	updateTitleKeys();

	if (Config::get(Config::ShowNews).GetBool())
	{
		setStatus(_("Fetching news for %s ...", FREESHOP_VERSION));
		std::string url = _("https://raw.githubusercontent.com/Cruel/freeShop/master/news/%s.txt", FREESHOP_VERSION).toAnsiString();
		Download download(url, FREESHOP_DIR "/news.txt");
		download.run();
	}

	setStatus(_("Loading game list..."));

	// TODO: Figure out why browse state won't load without this sleep
	cpp3ds::sleep(cpp3ds::milliseconds(100));
	requestStackPush(States::Browse);

	Config::saveToFile();
	while (m_timer.getElapsedTime() < cpp3ds::seconds(7.f))
		cpp3ds::sleep(cpp3ds::milliseconds(50));

	g_syncComplete = true;
}


bool SyncState::updateFreeShop()
{
	if (!Config::get(Config::AutoUpdate).GetBool() && !Config::get(Config::TriggerUpdateFlag).GetBool())
		return false;

	setStatus(_("Looking for freeShop update..."));
	const char *url = "https://api.github.com/repos/Cruel/freeShop/releases/latest";
	const char *latestJsonFilename = FREESHOP_DIR "/tmp/latest.json";
	Download download(url, latestJsonFilename);
	download.run();

	cpp3ds::FileInputStream jsonFile;
	if (jsonFile.open(latestJsonFilename))
	{
		std::string json;
		rapidjson::Document doc;
		int size = jsonFile.getSize();
		json.resize(size);
		jsonFile.read(&json[0], size);
		if (json.empty())
			return false;
		doc.Parse(json.c_str());

		if (!doc.HasMember("tag_name"))
			return false;
		std::string tag = doc["tag_name"].GetString();

		Config::set(Config::LastUpdatedTime, static_cast<int>(time(nullptr)));
		Config::saveToFile();

		if (!tag.empty() && tag.compare(FREESHOP_VERSION) != 0)
		{
#ifndef EMULATION
			Result ret;
			Handle cia;
			bool suceeded = false;
			size_t total = 0;
			std::string freeShopFile = FREESHOP_DIR "/tmp/freeShop.cia";
			std::string freeShopUrl = _("https://github.com/Cruel/freeShop/releases/download/%s/freeShop-%s.cia", tag.c_str(), tag.c_str());
			setStatus(_("Installing freeShop %s ...", tag.c_str()));
			Download freeShopDownload(freeShopUrl, freeShopFile);
			AM_QueryAvailableExternalTitleDatabase(nullptr);

			freeShopDownload.run();
			cpp3ds::Int64 bytesRead;
			cpp3ds::FileInputStream f;
			f.open(freeShopFile);
			char *buf = new char[128*1024];

			AM_StartCiaInstall(MEDIATYPE_SD, &cia);
			while (bytesRead = f.read(buf, 128*1024))
			{
				if (R_FAILED(ret = FSFILE_Write(cia, nullptr, total, buf, bytesRead, 0)))
					break;
				total += bytesRead;
			}
			delete[] buf;

			if (R_FAILED(ret))
				setStatus(_("Failed to install update: 0x%08lX", ret));
			suceeded = R_SUCCEEDED(ret = AM_FinishCiaInstall(cia));
			if (!suceeded)
				setStatus(_("Failed to install update: 0x%08lX", ret));
			return suceeded;
#endif
		}
	}
	return false;
}


bool SyncState::updateCache()
{
	// Fetch cache if it doesn't exist, regardless of settings
	bool cacheExists = pathExists(FREESHOP_DIR "/cache/data.json");

	if (cacheExists && Config::get(Config::CacheVersion).GetStringLength() > 0)
		if (!Config::get(Config::AutoUpdate).GetBool() && !Config::get(Config::TriggerUpdateFlag).GetBool())
			return false;

	// In case this flag triggered the update, reset it
	Config::set(Config::TriggerUpdateFlag, false);

	setStatus(_("Checking latest cache..."));
	const char *url = "https://api.github.com/repos/Repo3DS/shop-cache/releases/latest";
	const char *latestJsonFilename = FREESHOP_DIR "/tmp/latest.json";
	Download cache(url, latestJsonFilename);
	cache.run();

	cpp3ds::FileInputStream jsonFile;
	if (jsonFile.open(latestJsonFilename))
	{
		std::string json;
		rapidjson::Document doc;
		int size = jsonFile.getSize();
		json.resize(size);
		jsonFile.read(&json[0], size);
		doc.Parse(json.c_str());

		std::string tag = doc["tag_name"].GetString();
		if (!cacheExists || (!tag.empty() && tag.compare(Config::get(Config::CacheVersion).GetString()) != 0))
		{
			std::string cacheFile = FREESHOP_DIR "/tmp/cache.tar.gz";
#ifdef _3DS
			std::string cacheUrl = _("https://github.com/Repo3DS/shop-cache/releases/download/%s/cache-%s-etc1.tar.gz", tag.c_str(), tag.c_str());
#else
			std::string cacheUrl = _("https://github.com/Repo3DS/shop-cache/releases/download/%s/cache-%s-jpg.tar.gz", tag.c_str(), tag.c_str());
#endif
			setStatus(_("Downloading latest cache: %s...", tag.c_str()));
			Download cacheDownload(cacheUrl, cacheFile);
			cacheDownload.run();

			setStatus(_("Extracting latest cache..."));
			if (extract(cacheFile, FREESHOP_DIR "/cache/"))
			{
				Config::set(Config::CacheVersion, tag.c_str());
				Config::saveToFile();
				return true;
			}
		}
	}

	return false;
}


bool SyncState::updateTitleKeys()
{
	auto urls = Config::get(Config::KeyURLs).GetArray();
	if (!Config::get(Config::DownloadTitleKeys).GetBool() || urls.Empty())
		return false;

	setStatus(_("Downloading title keys..."));

	const char *url = urls[0].GetString();
	std::string tmpFilename = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/tmp/keys.bin");
	std::string keysFilename = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/keys/download.bin");
	Download download(url, tmpFilename);
	download.run();

	if (!TitleKeys::isValidFile(tmpFilename))
		return false;

	if (pathExists(keysFilename.c_str()))
		unlink(keysFilename.c_str());
	rename(tmpFilename.c_str(), keysFilename.c_str());

	return true;
}


void SyncState::setStatus(const std::string &message)
{
	m_textStatus.setString(message);
	cpp3ds::FloatRect rect = m_textStatus.getLocalBounds();
	m_textStatus.setOrigin(rect.width / 2.f, rect.height / 2.f);
}

void SyncState::startupSound()
{
	cpp3ds::Clock clock;
	while (clock.getElapsedTime() < cpp3ds::seconds(3.f))
		cpp3ds::sleep(cpp3ds::milliseconds(50));
	m_soundStartup.play(0);
//	while (clock.getElapsedTime() < cpp3ds::seconds(7.f))
//		cpp3ds::sleep(cpp3ds::milliseconds(50));
//	m_soundLoading.play(0);
}

} // namespace FreeShop
