#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <dirent.h>
#include "Download.hpp"
#include "TitleKeys.hpp"

namespace {

std::map<cpp3ds::Uint64, cpp3ds::Uint32[4]> titleKeys;
std::vector<cpp3ds::Uint64> titleIds;

}


namespace FreeShop {

/* encTitleKeys.bin format

  4 bytes  Number of entries
 12 bytes  Reserved

 entry(32 bytes in size):
   4 bytes  Common key index(0-5)
   4 bytes  Reserved
   8 bytes  Title Id
  16 bytes  Encrypted title key
*/
void TitleKeys::ensureTitleKeys()
{
	static bool keysChecked = false;
	if (!keysChecked)
	{
		keysChecked = true;
		cpp3ds::FileInputStream file;
		std::string keyDirectory = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/keys/");

		struct dirent *ent;
		DIR *dir = opendir(keyDirectory.c_str());
		if (!dir)
			return;

		while (ent = readdir(dir))
		{
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
				continue;

			std::string filename = keyDirectory + ent->d_name;
			file.open(filename);
			if (isValidFile(file))
			{
				size_t count = file.getSize() / 32;

				cpp3ds::Uint64 titleId;
				cpp3ds::Uint32 titleKey[4];

				for (int i = 0; i < count; ++i)
				{
					file.seek(24 + i * 32);
					file.read(&titleId, 8);
					file.read(titleKey, 16);
					titleId = __builtin_bswap64(titleId);

					int type = titleId >> 32;
					switch (type) {
						case Game:
						case Update:
						case Demo:
						case DLC:
						case DSiWare:
							if (titleKeys.find(titleId) == titleKeys.end())
							{
								titleIds.push_back(titleId);
								for (int j = 0; j < 4; ++j)
									titleKeys[titleId][j] = titleKey[j];
							}
						default:
							break;
					}
				}
			}
		}

		closedir(dir);
	}
}

std::vector<cpp3ds::Uint64> TitleKeys::getRelated(cpp3ds::Uint64 titleId, TitleType type)
{
	ensureTitleKeys();
	std::vector<cpp3ds::Uint64> related;
	cpp3ds::Uint32 titleLower = (titleId & 0xFFFFFFFF) >> 8;
	for (const auto &key : titleIds)
		if ((key >> 32 == type) && (titleLower == (key & 0xFFFFFFFF) >> 8))
			related.push_back(key);
	return related;
}

std::vector<cpp3ds::Uint64> &TitleKeys::getIds()
{
	ensureTitleKeys();
	return titleIds;
}

cpp3ds::Uint32 *TitleKeys::get(cpp3ds::Uint64 titleId)
{
	ensureTitleKeys();
	if (titleKeys.find(titleId) == titleKeys.end())
		return nullptr;
	return titleKeys[titleId];
}

bool TitleKeys::isValidFile(cpp3ds::FileInputStream &file)
{
	std::vector<char> data;
	cpp3ds::Int64 size = file.getSize() - 16; // Size minus header

	int count;
	file.seek(0);
	file.read(&count, 4);
	if (size % 32 != 0 || size / 32 != count)
		return false;
	count = (count > 3) ? 3 : count; // Check a max of 3 items

	data.resize(count * 32);
	file.seek(16);
	file.read(&data[0], data.size());

	return isValidData(&data[0], data.size());
}

bool TitleKeys::isValidFile(const std::string &filename)
{
	cpp3ds::FileInputStream file;
	if (!file.open(filename))
		return false;
	return isValidFile(file);
}

bool TitleKeys::isValidData(const void *data, size_t size)
{
	auto ptr = reinterpret_cast<const cpp3ds::Uint8*>(data);
	size_t count = size / 32;
	for (int i = 0; i < count; ++i)
	{
		cpp3ds::Uint64 titleId = *reinterpret_cast<const cpp3ds::Uint64*>(ptr + 8 + i * 32);
		if (__builtin_bswap64(titleId) >> 48 != 4)
			return false;
	}
	return true;
}

bool TitleKeys::isValidUrl(const std::string &url, cpp3ds::String *errorOut)
{
	std::vector<char> keyBuf;
	bool passed = false;
	errorOut->clear();

	FreeShop::Download download(url);
	download.setDataCallback([&](const void* data, size_t len, size_t processed, const cpp3ds::Http::Response& response)
	{
		auto httpStatus = response.getStatus();
		if (httpStatus == cpp3ds::Http::Response::MovedPermanently || httpStatus == cpp3ds::Http::Response::MovedTemporarily)
			return true;
		// If HTTP request is good, verify that contents are a titlekey dump
		if (passed = httpStatus == cpp3ds::Http::Response::Ok || httpStatus == cpp3ds::Http::Response::PartialContent)
		{
			const char *buf = reinterpret_cast<const char*>(data);
			keyBuf.insert(keyBuf.end(), buf + 16, buf + len);
			return false;
		}

		*errorOut = _("Failed.\n\nHTTP Status: %d", static_cast<int>(httpStatus));
		return false;
	});
	download.run();

	if (passed)
	{
		passed = isValidData(&keyBuf[0], keyBuf.size());
		if (!passed)
			*errorOut = _("Invalid title key format.");
	}

	if (download.getLastResponse().getStatus() == cpp3ds::Http::Response::TimedOut)
		*errorOut = _("Request timed out.\nTry again.");
	if (!passed && errorOut->isEmpty())
		*errorOut = _("Invalid URL.");

	return passed;
}


} // namespace FreeShop
