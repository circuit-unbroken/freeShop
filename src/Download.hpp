#ifndef FREESHOP_DOWNLOAD_HPP
#define FREESHOP_DOWNLOAD_HPP

#include <functional>
#include <queue>
#include <cpp3ds/System/Thread.hpp>
#include <cpp3ds/Network/Http.hpp>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/Graphics/Sprite.hpp>
#include <cpp3ds/Window/Event.hpp>
#include <cpp3ds/System/Mutex.hpp>
#include "TweenObjects.hpp"
#include "GUI/NinePatch.hpp"
#include "AppItem.hpp"


namespace FreeShop {

class Download;
typedef std::function<bool(const void*,size_t,size_t)> DownloadDataCallback;
typedef std::function<void(bool,bool)> DownloadFinishCallback;
typedef std::function<void(Download*)> SendTopCallback;

class Download : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable>{

friend class DownloadQueue;

public:
	enum Status {
		Queued,
		Downloading,
		Finished,
		Failed,
		Canceled,
		Suspended,
	};

	Download(const std::string& url, const std::string& destination = std::string());
	~Download();

	void setProgressMessage(const std::string& message);
	const std::string& getProgressMessage() const;

	void setProgress(float progress);
	float getProgress() const;

	const cpp3ds::Vector2f &getSize() const;

	void fillFromAppItem(std::shared_ptr<AppItem> app);

	void processEvent(const cpp3ds::Event& event);

	bool setUrl(const std::string& url);
	void setDestination(const std::string& destination);

	void pushUrl(const std::string& url, cpp3ds::Uint64 position = 0);

	void start();

	void setDataCallback(cpp3ds::Http::RequestCallback onData);
	void setFinishCallback(DownloadFinishCallback onFinish);
	void setSendTopCallback(SendTopCallback onSendTop);

	void setField(const std::string &field, const std::string &value);
	void setRetryCount(int timesToRetry);

	void run();
	void suspend();
	void resume();

	void cancel(bool wait = true);
	bool isCanceled();

	Status getStatus() const;

	const cpp3ds::Http::Response &getLastResponse() const;
	void setTimeout(cpp3ds::Time timeout);
	void setBufferSize(size_t size);

	bool markedForDelete();

protected:
	virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;

private:
	cpp3ds::Http::Response m_response;
	cpp3ds::Http m_http;
	cpp3ds::Http::Request m_request;
	std::string m_host;
	std::string m_uri;
	std::string m_destination;
	std::queue<std::pair<std::string, cpp3ds::Uint64>> m_urlQueue;
	cpp3ds::Http::RequestCallback m_onData;
	DownloadFinishCallback m_onFinish;
	cpp3ds::Thread m_thread;

	std::shared_ptr<AppItem> m_appItem;

	cpp3ds::Mutex m_mutex;
	Status m_status;
	bool m_canSendTop;
	bool m_cancelFlag;
	bool m_markedForDelete;
	SendTopCallback m_onSendTop;
	FILE* m_file;
	std::vector<char> m_buffer;

	cpp3ds::Time m_timeout;
	size_t m_bufferSize;

	// For queue UI
	gui3ds::NinePatch m_background;
	cpp3ds::RectangleShape m_icon;
	cpp3ds::RectangleShape m_progressBar;

	cpp3ds::Text m_textTitle;
	cpp3ds::Text m_textProgress;

	util3ds::TweenText m_textCancel;
	util3ds::TweenText m_textSendTop;
	util3ds::TweenText m_textRestart;

	size_t m_downloadPos;
	float m_progress;
	int m_timesToRetry;
	cpp3ds::Vector2f m_size;
	std::string m_progressMessage;
};

} // namespace FreeShop


#endif // FREESHOP_DOWNLOAD_HPP
