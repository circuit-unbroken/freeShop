#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Installer.hpp"


namespace FreeShop {

Installer::Installer(cpp3ds::Uint64 titleId, int contentIndex)
: m_titleId(titleId)
, m_isSuspended(false)
, m_isInstalling(false)
, m_isInstallingTmd(false)
, m_isInstallingContent(false)
, m_currentContentIndex(-1)
, m_currentContentPosition(0)
{
	if (contentIndex >= 0)
		m_currentContentIndex = contentIndex;
}

Installer::~Installer()
{
	//
}

bool Installer::installTicket(cpp3ds::Uint16 titleVersion)
{
	return true;
}

bool Installer::installSeed(const void *seed)
{
	return true;
}

bool Installer::start(bool deleteTitle)
{
	return true;
}


bool Installer::resume()
{
	return true;
}

bool Installer::suspend()
{
	return true;
}

void Installer::abort()
{
	//
}

bool Installer::commit()
{
	return true;
}

bool Installer::finalizeTmd()
{
	return true;
}

bool Installer::finalizeContent()
{
	m_currentContentPosition = 0;
	return true;
}

bool Installer::importContents(size_t count, cpp3ds::Uint16 *indices)
{
	if (m_isInstalling && !commit())
		return false;

	return start(false);
}

bool Installer::installTmd(const void *data, size_t size)
{
	return true;
}

bool Installer::installContent(const void *data, size_t size, cpp3ds::Uint16 index)
{
	m_currentContentPosition += size;
	m_currentContentIndex = index;
	return true;
}

cpp3ds::Int32 Installer::getErrorCode() const
{
	return 0;
}

const cpp3ds::String &Installer::getErrorString() const
{
	return m_errorStr;
}

int Installer::getCurrentContentIndex() const
{
	return m_currentContentIndex;
}

cpp3ds::Uint64 Installer::getCurrentContentPosition() const
{
	return m_currentContentPosition;
}

cpp3ds::Uint64 Installer::getTitleId() const
{
	return m_titleId;
}

} // namespace FreeShop
