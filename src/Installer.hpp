#ifndef FREESHOP_INSTALLER_HPP
#define FREESHOP_INSTALLER_HPP

#include <string>
#include <vector>
#include <cpp3ds/Config.hpp>
#include <cpp3ds/System/Mutex.hpp>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

class Installer {
public:


	Installer(cpp3ds::Uint64 titleId, int contentIndex = -1);
	~Installer();

	bool installTicket(cpp3ds::Uint16 titleVersion);
	bool installSeed(const void *seed);

	bool start(bool deleteTitle);
	bool resume();
	bool suspend();
	void abort();

	bool installTmd(const void *data, size_t size);
	bool installContent(const void *data, size_t size, cpp3ds::Uint16 index);

	bool finalizeTmd();
	bool finalizeContent();
	bool importContents(size_t count, cpp3ds::Uint16 *indices);
	bool commit();

	cpp3ds::Int32 getErrorCode() const;
	const cpp3ds::String &getErrorString() const;

	cpp3ds::Uint64 getTitleId() const;
	int getCurrentContentIndex() const;
	cpp3ds::Uint64 getCurrentContentPosition() const;

private:

private:
	cpp3ds::Uint64 m_titleId;
	cpp3ds::Uint32 m_titleType;
	int            m_currentContentIndex;
	cpp3ds::Uint64 m_currentContentPosition;
	cpp3ds::Mutex  m_mutex;
	cpp3ds::String m_errorStr;

#ifndef EMULATION
	Result m_result;
	Handle m_handleTmd;
	Handle m_handleContent;
	FS_MediaType m_mediaType;
#endif

	bool m_isSuspended;
	bool m_isInstalling;
	bool m_isInstallingTmd;
	bool m_isInstallingContent;
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLER_HPP
