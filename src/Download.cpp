#include <iostream>
#include <stdio.h>
#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/Lock.hpp>
#include <cpp3ds/System/Service.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include "Download.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "States/DialogState.hpp"
#include "States/BrowseState.hpp"

namespace FreeShop {


Download::Download(const std::string &url, const std::string &destination)
: m_thread(&Download::run, this)
, m_progress(0.f)
, m_canSendTop(true)
, m_markedForDelete(false)
, m_cancelFlag(false)
, m_status(Queued)
, m_downloadPos(0)
, m_appItem(nullptr)
, m_timesToRetry(3)
, m_timeout(cpp3ds::seconds(10))
, m_bufferSize(16*1024)
{
	setUrl(url);
	setDestination(destination);

//	m_thread.setStackSize(64*1024);
//	m_thread.setAffinity(1);
}


Download::~Download()
{
	cancel();
}


bool Download::setUrl(const std::string &url)
{
	size_t pos;

	pos = url.find("://");
	if (pos != std::string::npos) {
		pos = url.find("/", pos + 3);
		if (pos == std::string::npos) {
			m_host = url;
			m_uri = "/";
		} else {
			m_host = url.substr(0, pos);
			m_uri = url.substr(pos);
		}
	}

	m_http.setHost(m_host);
	m_request.setUri(m_uri);
	return true;
}


void Download::start()
{
	m_buffer.clear();
	m_thread.launch(); // run()
}


void Download::run()
{
	if (!m_destination.empty())
	{
		m_file = fopen(cpp3ds::FileSystem::getFilePath(m_destination).c_str(), "w");
		if (!m_file)
			return;
	}

	m_status = Downloading;
	m_cancelFlag = false;
	bool failed = false;
	int retryCount = 0;
	m_request.setField("Range", _("bytes=%u-", m_downloadPos));

	cpp3ds::Http::RequestCallback dataCallback = [&](const void* data, size_t len, size_t processed, const cpp3ds::Http::Response& response)
	{
		cpp3ds::Lock lock(m_mutex);

		if (!m_destination.empty())
		{
			if (response.getStatus() == cpp3ds::Http::Response::Ok || response.getStatus() == cpp3ds::Http::Response::PartialContent)
			{
				const char *bufdata = reinterpret_cast<const char*>(data);
				m_buffer.insert(m_buffer.end(), bufdata, bufdata + len);

				if (m_buffer.size() > 1024 * 50)
				{
					fwrite(&m_buffer[0], sizeof(char), m_buffer.size(), m_file);
					m_buffer.clear();
				}
			}
		}

		if (!m_cancelFlag && m_onData)
			failed = !m_onData(data, len, processed, response);

		if (getStatus() != Suspended)
			m_downloadPos += len;

		if (getStatus() == Failed)
			failed = true;

		return !m_cancelFlag && !failed && getStatus() == Downloading;
	};

	// Loop in case there are URLs pushed to queue later
	while (1)
	{
		retryCount = 0;

		// Retry loop in case of connection failures
		do {
			// Wait in case of internet loss, but exit when canceled or suspended
			while (!cpp3ds::Service::isEnabled(cpp3ds::Httpc) && !m_cancelFlag && getStatus() != Suspended)
			{
				setProgressMessage(_("Waiting for internet..."));
				cpp3ds::sleep(cpp3ds::milliseconds(200));
			}

			m_response = m_http.sendRequest(m_request, m_timeout, dataCallback, m_bufferSize);
			// Follow all redirects
			while (m_response.getStatus() == cpp3ds::Http::Response::MovedPermanently || m_response.getStatus() == cpp3ds::Http::Response::MovedTemporarily)
			{
				setUrl(m_response.getField("Location"));
				m_response = m_http.sendRequest(m_request, m_timeout, dataCallback, m_bufferSize);
			}

			// Retry failed connections (all error codes >= 1000)
			if (m_response.getStatus() >= 1000)
			{
				if (retryCount >= m_timesToRetry)
				{
					if (m_timesToRetry == 0)
						setProgressMessage(_("Failed"));
					else
						setProgressMessage(_("Retry attempts exceeded"));
					failed = true;
					break;
				}
				std::cout << _("Retrying... %d", retryCount).toAnsiString() << std::endl;
				setProgressMessage(_("Retrying... %d", ++retryCount));
				m_request.setField("Range", _("bytes=%u-", m_downloadPos));
				cpp3ds::sleep(cpp3ds::milliseconds(1000));
			}
		} while (m_response.getStatus() >= 1000 &&
		         !m_cancelFlag && getStatus() != Suspended);

		if (m_response.getStatus() != cpp3ds::Http::Response::Ok && m_response.getStatus() != cpp3ds::Http::Response::PartialContent)
			break;
		if (m_cancelFlag || failed || getStatus() == Suspended)
			break;

		// Download next in queue
		if (m_urlQueue.size() > 0)
		{
			auto nextUrl = m_urlQueue.front();
			setUrl(nextUrl.first);
			m_downloadPos = nextUrl.second;
			m_urlQueue.pop();
			m_request.setField("Range", _("bytes=%u-", m_downloadPos));
		}
		else
			break;
	}

	if (m_urlQueue.size() > 0 && getStatus() != Suspended)
		std::queue<std::pair<std::string,cpp3ds::Uint64>>().swap(m_urlQueue);

	if (getStatus() != Suspended)
	{
		m_canSendTop = false;
		if (m_cancelFlag)
		{
			m_markedForDelete = true;
			m_status = Canceled;
		}
		else if (failed)
			m_status = Failed;
		else
			m_status = Finished;
	}

	// Write remaining buffer and close downloaded file
	if (!m_destination.empty())
	{
		if (m_buffer.size() > 0)
			fwrite(&m_buffer[0], sizeof(char), m_buffer.size(), m_file);
		fclose(m_file);

		if (m_status != Finished)
			remove(cpp3ds::FileSystem::getFilePath(m_destination).c_str());
	}

	if (m_onFinish)
		m_onFinish(m_cancelFlag, failed);

	m_http.close();
}


void Download::cancel(bool wait)
{
	m_cancelFlag = true;
	if (wait)
		m_thread.wait();
}


bool Download::isCanceled()
{
	return m_cancelFlag;
}


void Download::setProgress(float progress)
{
	m_progress = progress;
	m_progressBar.setSize(cpp3ds::Vector2f(m_progress * m_size.x, m_size.y));
}


float Download::getProgress() const
{
	return m_progress;
}


void Download::setProgressMessage(const std::string &message)
{
	m_progressMessage = message;
	m_textProgress.setString(message);
}


const std::string &Download::getProgressMessage() const
{
	return m_progressMessage;
}


void Download::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	target.draw(m_background, states);
	target.draw(m_icon, states);
	target.draw(m_textTitle, states);
	target.draw(m_textProgress, states);

	if (!m_cancelFlag)
	{
		target.draw(m_textCancel, states);
		if (m_canSendTop && (m_status == Queued || m_status == Suspended || m_status == Downloading))
			target.draw(m_textSendTop, states);
		else if (m_status == Failed)
			target.draw(m_textRestart, states);
	}
	if (m_progress > 0.f && m_progress < 1.f)
		target.draw(m_progressBar, states);
}


const cpp3ds::Vector2f &Download::getSize() const
{
	return m_size;
}


void Download::fillFromAppItem(std::shared_ptr<AppItem> app)
{
	m_appItem = app;

	setProgressMessage(_("Queued"));

	m_background.setTexture(&AssetManager<cpp3ds::Texture>::get("images/listitembg.9.png"));
	m_background.setContentSize(320.f + m_background.getPadding().width - m_background.getTexture()->getSize().x + 2.f, 24.f);
	m_size = m_background.getSize();
	float paddingRight = m_size.x - m_background.getContentSize().x - m_background.getPadding().left;
	float paddingBottom = m_size.y - m_background.getContentSize().y - m_background.getPadding().top;

	m_icon.setSize(cpp3ds::Vector2f(48.f, 48.f));
	cpp3ds::IntRect textureRect;
	m_icon.setTexture(app->getIcon(textureRect), true);
	m_icon.setTextureRect(textureRect);
	m_icon.setPosition(m_background.getPadding().left, m_background.getPadding().top);
	m_icon.setScale(0.5f, 0.5f);

	m_textCancel.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textCancel.setString(L"\uf00d");
	m_textCancel.setCharacterSize(18);
	m_textCancel.setFillColor(cpp3ds::Color::White);
	m_textCancel.setOutlineColor(cpp3ds::Color(0, 0, 0, 200));
	m_textCancel.setOutlineThickness(1.f);
	m_textCancel.setOrigin(0, m_textCancel.getLocalBounds().top + m_textCancel.getLocalBounds().height / 2.f);
	m_textCancel.setPosition(m_size.x - paddingRight - m_textCancel.getLocalBounds().width,
							 m_background.getPadding().top + m_background.getContentSize().y / 2.f);

	m_textSendTop = m_textCancel;
	m_textSendTop.setString(L"\uf077");
	m_textSendTop.move(-25.f, 0);

	m_textRestart = m_textCancel;
	m_textRestart.setString(L"\uf01e");
	m_textRestart.move(-25.f, 0);

	m_textTitle.setString(app->getTitle());
	m_textTitle.setCharacterSize(10);
	m_textTitle.setPosition(m_background.getPadding().left + 30.f, m_background.getPadding().top);
	m_textTitle.setFillColor(cpp3ds::Color::Black);
	m_textTitle.useSystemFont();

	m_textProgress = m_textTitle;
	m_textProgress.setFillColor(cpp3ds::Color(130, 130, 130, 255));
	m_textProgress.move(0.f, 12.f);

	m_progressBar.setFillColor(cpp3ds::Color(0, 0, 0, 50));
}


void Download::processEvent(const cpp3ds::Event &event)
{
	if (event.type == cpp3ds::Event::TouchBegan)
	{
		cpp3ds::FloatRect bounds = m_textCancel.getGlobalBounds();
		bounds.left += getPosition().x;
		bounds.top += getPosition().y + DownloadQueue::getInstance().getPosition().y;
		if (bounds.contains(event.touch.x, event.touch.y))
		{
			if (getStatus() != Downloading && getStatus() != Suspended)
				m_markedForDelete = true;
			else
			{
				g_browseState->requestStackPush(States::Dialog, false, [=](void *data) mutable {
					auto event = reinterpret_cast<DialogState::Event*>(data);
					if (event->type == DialogState::GetText)
					{
						auto str = reinterpret_cast<cpp3ds::String*>(event->data);
						*str = _("%s\n\nAre you sure you want to cancel\nthis installation and lose all progress?", m_appItem->getTitle().toAnsiString().c_str());
						return true;
					}
					else if (event->type == DialogState::Response)
					{
						bool *accepted = reinterpret_cast<bool*>(event->data);
						if (*accepted)
						{
							// Check status again in case it changed while dialog was open
							if (getStatus() == Downloading)
							{
								setProgressMessage(_("Canceling..."));
								cancel(false);
							}
							else if (getStatus() == Suspended)
								m_markedForDelete = true;
						}
						return true;
					}
					return false;
				});
			}
		}
		else if (m_canSendTop && m_onSendTop && (getStatus() == Queued || getStatus() == Suspended || getStatus() == Downloading))
		{
			bounds = m_textSendTop.getGlobalBounds();
			bounds.left += getPosition().x;
			bounds.top += getPosition().y + DownloadQueue::getInstance().getPosition().y;
			if (bounds.contains(event.touch.x, event.touch.y))
			{
				m_onSendTop(this);
			}
		}
		else if (getStatus() == Failed)
		{
			bounds = m_textRestart.getGlobalBounds();
			bounds.left += getPosition().x;
			bounds.top += getPosition().y + DownloadQueue::getInstance().getPosition().y;
			if (bounds.contains(event.touch.x, event.touch.y))
			{
				// Restart failed download
				DownloadQueue::getInstance().restartDownload(this);
			}
		}
	}
}

bool Download::markedForDelete()
{
	return m_markedForDelete;
}

void Download::setDestination(const std::string &destination)
{
	m_destination = destination;
}

void Download::setDataCallback(cpp3ds::Http::RequestCallback onData)
{
	m_onData = onData;
}

void Download::setFinishCallback(DownloadFinishCallback onFinish)
{
	m_onFinish = onFinish;
}

void Download::setSendTopCallback(SendTopCallback onSendTop)
{
	m_onSendTop = onSendTop;
}

void Download::setField(const std::string &field, const std::string &value)
{
	m_request.setField(field, value);
}

void Download::pushUrl(const std::string &url, cpp3ds::Uint64 position)
{
	m_urlQueue.push(std::make_pair(url, position));
}

Download::Status Download::getStatus() const
{
	return m_status;
}

void Download::suspend()
{
	if (getStatus() != Downloading)
		return;
	cpp3ds::Lock lock(m_mutex);
	m_status = Suspended;
}

void Download::resume()
{
	if (getStatus() == Suspended || getStatus() == Queued)
	{
		start();
	}
}

void Download::setRetryCount(int timesToRetry)
{
	m_timesToRetry = timesToRetry;
}

const cpp3ds::Http::Response &Download::getLastResponse() const
{
	return m_response;
}

void Download::setTimeout(cpp3ds::Time timeout)
{
	m_timeout = timeout;
}

void Download::setBufferSize(size_t size)
{
	if (size % 64 > 0)
		return;
	m_bufferSize = size;
}


} // namespace FreeShop
