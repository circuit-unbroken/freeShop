#ifndef FREESHOP_ASSETMANAGER_HPP
#define FREESHOP_ASSETMANAGER_HPP

#include <cpp3ds/Audio/Sound.hpp>
#include <cpp3ds/Audio/SoundBuffer.hpp>
#include <cpp3ds/System/Err.hpp>
#include <memory>
#include <map>
#include <assert.h>

namespace FreeShop {

template <class T>
class AssetManager {
public:
	static T& get(const std::string& filename)
	{
		static AssetManager<T> manager; // Yep, singleton

		auto item = manager.m_assets.find(filename);
		if (item == manager.m_assets.end())
		{
			std::unique_ptr<T> asset(new T);
			if (!asset->loadFromFile(filename))
				cpp3ds::err() << "Failed to load asset: " << filename << std::endl;
			manager.m_assets.insert(std::make_pair(filename, std::move(asset)));
			return get(filename);
		}
		return *item->second;
	}

private:
	AssetManager<T>() {} // Empty constructor

	std::map<std::string, std::unique_ptr<T>> m_assets;
};

} // namespace FreeShop

#endif // FREESHOP_ASSETMANAGER_HPP
