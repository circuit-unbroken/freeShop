#include <cpp3ds/System/Err.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/Network/Http.hpp>
#include <cpp3ds/System/Lock.hpp>
#include "Installer.hpp"
#include "TitleKeys.hpp"
#include "ticket.h"

namespace {

Result FSUSER_AddSeed(u64 titleId, const void* seed)
{
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x087a0180;
	cmdbuf[1] = (u32) (titleId & 0xFFFFFFFF);
	cmdbuf[2] = (u32) (titleId >> 32);
	memcpy(&cmdbuf[3], seed, 16);

	Result ret = 0;
	if(R_FAILED(ret = svcSendSyncRequest(*fsGetSessionHandle()))) return ret;

	ret = cmdbuf[1];
	return ret;
}

}


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
	m_titleType = titleId >> 32;
	m_mediaType = (m_titleType == TitleKeys::DSiWare) ? MEDIATYPE_NAND : MEDIATYPE_SD;
	if (contentIndex >= 0)
	{
		m_currentContentIndex = contentIndex;
		m_isSuspended = true;
		m_isInstalling = true;
		m_isInstallingContent = true;
	}
}

Installer::~Installer()
{
	if (!m_isSuspended)
		abort();
}

void Installer::abort()
{
	if (m_isSuspended)
		AM_DeletePendingTitle(m_mediaType, m_titleId);
	else
	{
		if (m_isInstallingContent)
			AM_InstallContentCancel(m_handleContent);
		if (m_isInstallingTmd)
			AM_InstallTmdAbort(m_handleTmd);
		if (m_isInstalling)
			AM_InstallTitleAbort();
	}

	m_isInstallingContent = false;
	m_isInstallingTmd = false;
	m_isInstalling = false;
	m_isSuspended = false;
	m_currentContentIndex = -1;
	m_currentContentPosition = 0;
}

bool Installer::installTicket(cpp3ds::Uint16 titleVersion)
{
	Handle ticket;
	u8 tikData[sizeof(tikTemp)];
	const u16 sigSize = 0x140;
	cpp3ds::Uint64 titleIdBE = __builtin_bswap64(m_titleId);

	cpp3ds::Uint32 *key = TitleKeys::get(m_titleId);
	if (!key)
		return false;

	// Build ticket
	memcpy(tikData, tikTemp, sizeof(tikData));
	memcpy(tikData + sigSize + 0x9C, &titleIdBE, 8);
	memcpy(tikData + sigSize + 0xA6, &titleVersion, 2);
	memcpy(tikData + sigSize + 0x7F, key, 16);

	AM_QueryAvailableExternalTitleDatabase(nullptr);
	AM_DeleteTicket(m_titleId);

	if (R_SUCCEEDED(m_result = AM_InstallTicketBegin(&ticket)))
	{
		if (R_SUCCEEDED(m_result = FSFILE_Write(ticket, nullptr, 0, tikData, sizeof(tikData), 0)))
		{
			if (R_SUCCEEDED(m_result = AM_InstallTicketFinish(ticket)))
				return true;
		}
		AM_InstallTicketAbort(ticket);
	}

	m_errorStr = _("Failed to install ticket: 0x%08lX", m_result);
	return false;
}

bool Installer::installSeed(const void *seed)
{
	if (seed)
	{
		if (R_SUCCEEDED(m_result = FSUSER_AddSeed(m_titleId, seed)))
			return true;
	}
	else // Retrieve from server
	{
		// DSiWare games can be skipped
		if (((m_titleId >> 32) & 0x8010) != 0)
			return true;

		cpp3ds::Http http("https://kagiya-ctr.cdn.nintendo.net");
		cpp3ds::Http::Request request(_("title/0x%016llX/ext_key?country=%s", m_titleId, "US"));
		cpp3ds::Http::Response response = http.sendRequest(request);
		auto status = response.getStatus();
		if (status == cpp3ds::Http::Response::Ok)
		{
			std::string seedStr = response.getBody();
			if (seedStr.size() == 16)
			{
				if (R_SUCCEEDED(m_result = FSUSER_AddSeed(m_titleId, seedStr.c_str())))
					return true;
			}
		}
		else if (status == cpp3ds::Http::Response::NotFound)
			return true; // Title has no seed, so it's fine
		else
		{
			m_errorStr = _("Failed to get seed: HTTP %d", (int)status);
			return false;
		}
	}

	m_errorStr = _("Need FW 9.6+, Seed failed: %08lX", m_result);
	return false;
}

bool Installer::start(bool deleteTitle)
{
	if (!m_isInstalling)
	{
		if (deleteTitle)
			AM_DeleteTitle(m_mediaType, m_titleId);
		AM_QueryAvailableExternalTitleDatabase(nullptr);
		if (R_SUCCEEDED(m_result = AM_InstallTitleBegin(m_mediaType, m_titleId, false)))
		{
			m_isInstalling = true;
			return true;
		}
		m_errorStr = _("Failed to start: 0x%08lX", m_result);
		return false;
	}
}

bool Installer::resume()
{
	if (!m_isSuspended)
		return true;

	AM_QueryAvailableExternalTitleDatabase(nullptr);
	if (m_isInstalling && R_SUCCEEDED(m_result = AM_InstallTitleResume(m_mediaType, m_titleId)))
		if (!m_isInstallingContent || R_SUCCEEDED(m_result = AM_InstallContentResume(&m_handleContent, &m_currentContentPosition, m_currentContentIndex)))
		{
			m_isSuspended = false;
			return true;
		}

	abort();
	m_errorStr = _("Failed to resume: 0x%08lX", m_result);
	return false;
}

bool Installer::suspend()
{
	cpp3ds::Lock lock(m_mutex);
	m_result = 0;

	if (m_isInstalling && !m_isSuspended)
	{
		if (m_isInstallingContent)
			m_result = AM_InstallContentStop(m_handleContent);
		if (R_SUCCEEDED(m_result = AM_InstallTitleStop()))
			return m_isSuspended = true;
	}
	else
		return true;

	m_errorStr = _("Failed to suspend: 0x%08lX", m_result);
	return false;
}

bool Installer::commit()
{
	if (!m_isInstalling || m_isInstallingTmd || m_isInstallingContent)
		return false;

	if (R_SUCCEEDED(m_result = AM_InstallTitleFinish()))
		if (R_SUCCEEDED(m_result = AM_CommitImportTitles(m_mediaType, 1, false, &m_titleId)))
		{
			m_isInstalling = false;
			return true;
		}

	abort();
	m_errorStr = _("Failed to commit title: 0x%08lX", m_result);
	return false;
}

bool Installer::finalizeTmd()
{
	if (!m_isInstallingTmd)
		return false;

	if (R_SUCCEEDED(m_result = AM_InstallTmdFinish(m_handleTmd, true)))
	{
		m_isInstallingTmd = false;
		return true;
	}

	abort();
	m_errorStr = _("Failed to finalize TMD: 0x%08lX", m_result);
	return false;
}

bool Installer::finalizeContent()
{
	if (!m_isInstallingContent)
		return false;

	if (R_SUCCEEDED(m_result = AM_InstallContentFinish(m_handleContent)))
	{
		m_isInstallingContent = false;
		return true;
	}

	abort();
	m_errorStr = _("Failed to finalize content: 0x%08lX", m_result);
	return false;
}

bool Installer::importContents(size_t count, cpp3ds::Uint16 *indices)
{
	if (m_isInstalling && !commit())
		return false;

	if (start(false))
		if (R_SUCCEEDED(m_result = AM_CreateImportContentContexts(count, indices)))
			return true;

	abort();
	m_errorStr = _("Failed to import contents: 0x%08lX", m_result);
	return false;
}

bool Installer::installTmd(const void *data, size_t size)
{
	cpp3ds::Lock lock(m_mutex);

	if (!m_isInstallingTmd && R_SUCCEEDED(m_result = AM_InstallTmdBegin(&m_handleTmd)))
		m_isInstallingTmd = true;

	if (m_isInstallingTmd)
		if (R_SUCCEEDED(m_result = FSFILE_Write(m_handleTmd, nullptr, 0, data, size, 0)))
			return true;

	abort();
	m_errorStr = _("Failed to install TMD: 0x%08lX", m_result);
	return false;
}

bool Installer::installContent(const void *data, size_t size, cpp3ds::Uint16 index)
{
	cpp3ds::Lock lock(m_mutex);

	if (m_isSuspended)
		return false;

	if (!m_isInstallingContent && R_SUCCEEDED(m_result = AM_InstallContentBegin(&m_handleContent, index)))
	{
		m_isInstallingContent = true;
		m_currentContentIndex = index;
		m_currentContentPosition = 0;
	}

	if (m_isInstallingContent)
		if (R_SUCCEEDED(m_result = FSFILE_Write(m_handleContent, nullptr, m_currentContentPosition, data, size, 0)))
		{
			m_currentContentPosition += size;
			return true;
		}

	abort();
	m_errorStr = _("Failed to install content: 0x%08lX", m_result);
	return false;
}

cpp3ds::Int32 Installer::getErrorCode() const
{
	return m_result;
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
