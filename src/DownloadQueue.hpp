#ifndef FREESHOP_DOWNLOADQUEUE_HPP
#define FREESHOP_DOWNLOADQUEUE_HPP

#include <bits/unique_ptr.h>
#include <cpp3ds/Window/Event.hpp>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Audio/Sound.hpp>
#include <cpp3ds/Audio/SoundBuffer.hpp>
#include "Download.hpp"
#include "GUI/AppItem.hpp"
#include "Installer.hpp"
#include "GUI/Scrollable.hpp"

namespace FreeShop {

typedef std::function<void(bool)> DownloadCompleteCallback;

struct DownloadItem {
	DownloadItem(std::shared_ptr<AppItem> appItem, Download *download, Installer *installer);
	~DownloadItem();
	std::shared_ptr<AppItem> appItem;
	Download *download;
	Installer *installer;
};

class DownloadQueue : public cpp3ds::Drawable, public Scrollable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	~DownloadQueue();

	static DownloadQueue &getInstance();

	void addDownload(std::shared_ptr<AppItem> app, cpp3ds::Uint64 titleId = 0, DownloadCompleteCallback callback = nullptr, int contentIndex = -1, float progress = 0.f);
	void restartDownload(Download *download);

	bool isDownloading(std::shared_ptr<AppItem> app);
	bool isDownloading(cpp3ds::Uint64 titleId);

	void suspend();
	void resume();
	void save();

	void sendTop(Download* download);

	size_t getCount();
	size_t getActiveCount();

	void update(float delta);
	bool processEvent(const cpp3ds::Event& event);

	virtual void setScroll(float position);
	virtual float getScroll();
	virtual const cpp3ds::Vector2f &getScrollSize();

protected:
	DownloadQueue();
	void load();
	void realign();
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;

	// Go through list make sure only top item is downloading and rest are suspended if necessary
	void refresh();

private:
	std::vector<std::unique_ptr<DownloadItem>> m_downloads;
	TweenEngine::TweenManager m_tweenManager;
	cpp3ds::SoundBuffer m_soundBufferFinish;
	cpp3ds::Sound m_soundFinish;

	float m_scrollPos;
	cpp3ds::Vector2f m_size;

	cpp3ds::Thread m_threadRefresh;
	cpp3ds::Mutex m_mutexRefresh;
	cpp3ds::Clock m_clockRefresh;
	volatile bool m_refreshEnd;
};

} // namespace FreeShop

#endif // FREESHOP_DOWNLOADQUEUE_HPP
