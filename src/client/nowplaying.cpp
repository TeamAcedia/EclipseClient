// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License

#include "client/nowplaying.h"

#include "porting.h"

#if defined(HAVE_DBUS)
	#include <dbus/dbus.h>
	#include <cstring>
	#include <vector>
#elif defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <tlhelp32.h>
	#include <string>
	#if defined(HAVE_WINRT_SMTC)
		#include <winrt/base.h>
		#include <winrt/Windows.Media.Control.h>
		#include <winrt/Windows.Foundation.h>
	#endif
#elif defined(__APPLE__)
	#include <dlfcn.h>
	#include <dispatch/dispatch.h>
	#include <CoreFoundation/CoreFoundation.h>
	#include <mutex>
	#include <atomic>
#endif

namespace {
	// How often poll() actually queries the backend when nothing else
	// throttles it further (e.g. WinRT's own async callback cadence).
	constexpr unsigned long long POLL_INTERVAL_MS = 1000;
}

#if defined(HAVE_DBUS)

// MPRIS2 (https://specifications.freedesktop.org/mpris-spec/latest/) over
// D-Bus: the standard most Linux media players and browsers (Firefox,
// Chrome, Spotify, VLC, mpv w/ mpris plugin, ...) implement. Polls every
// currently-running "org.mpris.MediaPlayer2.*" bus name and reports the
// first one that's actually Playing, so switching which app has focus/is
// playing doesn't need any special handling here.
class NowPlayingProviderImpl
{
public:
	NowPlayingProviderImpl()
	{
		DBusError err;
		dbus_error_init(&err);
		m_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
		if (dbus_error_is_set(&err)) {
			dbus_error_free(&err);
			m_conn = nullptr;
		}
	}

	~NowPlayingProviderImpl()
	{
		// dbus_bus_get() returns a shared connection owned by libdbus;
		// unref (not close) is correct here.
		if (m_conn)
			dbus_connection_unref(m_conn);
	}

	bool poll(NowPlayingInfo *out)
	{
		if (!m_conn)
			return false;
		dbus_connection_read_write(m_conn, 0);

		for (const std::string &name : listMediaPlayerNames()) {
			if (queryPlayer(name, out))
				return true;
		}
		return false;
	}

private:
	std::vector<std::string> listMediaPlayerNames()
	{
		std::vector<std::string> names;
		DBusMessage *msg = dbus_message_new_method_call(
			"org.freedesktop.DBus", "/org/freedesktop/DBus",
			"org.freedesktop.DBus", "ListNames");
		if (!msg)
			return names;

		DBusMessage *reply = dbus_connection_send_with_reply_and_block(
			m_conn, msg, 200, nullptr);
		dbus_message_unref(msg);
		if (!reply)
			return names;

		DBusMessageIter iter, sub;
		if (dbus_message_iter_init(reply, &iter) &&
				dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
			dbus_message_iter_recurse(&iter, &sub);
			while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
				const char *s = nullptr;
				dbus_message_iter_get_basic(&sub, &s);
				if (s && strncmp(s, "org.mpris.MediaPlayer2.", 23) == 0)
					names.emplace_back(s);
				dbus_message_iter_next(&sub);
			}
		}
		dbus_message_unref(reply);
		return names;
	}

	// Reads the "org.mpris.MediaPlayer2.Player" interface's "Metadata"
	// and "PlaybackStatus" properties via the standard
	// org.freedesktop.DBus.Properties.Get method. Returns true (and
	// fills *out) only if this specific player is actually playing.
	bool queryPlayer(const std::string &bus_name, NowPlayingInfo *out)
	{
		std::string status = getStringProperty(bus_name, "PlaybackStatus");
		if (status != "Playing")
			return false;

		DBusMessage *msg = dbus_message_new_method_call(bus_name.c_str(),
			"/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
		if (!msg)
			return false;
		const char *iface = "org.mpris.MediaPlayer2.Player";
		const char *prop = "Metadata";
		dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface,
			DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

		DBusMessage *reply = dbus_connection_send_with_reply_and_block(
			m_conn, msg, 200, nullptr);
		dbus_message_unref(msg);
		if (!reply)
			return false;

		std::string title, artist;
		DBusMessageIter iter, variant, dict;
		if (dbus_message_iter_init(reply, &iter) &&
				dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
			dbus_message_iter_recurse(&iter, &variant);
			if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY) {
				dbus_message_iter_recurse(&variant, &dict);
				while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
					DBusMessageIter entry, value;
					dbus_message_iter_recurse(&dict, &entry);
					const char *key = nullptr;
					dbus_message_iter_get_basic(&entry, &key);
					dbus_message_iter_next(&entry);
					dbus_message_iter_recurse(&entry, &value);

					if (key && strcmp(key, "xesam:title") == 0 &&
							dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING) {
						const char *s = nullptr;
						dbus_message_iter_get_basic(&value, &s);
						if (s) title = s;
					} else if (key && strcmp(key, "xesam:artist") == 0 &&
							dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_ARRAY) {
						DBusMessageIter artists;
						dbus_message_iter_recurse(&value, &artists);
						if (dbus_message_iter_get_arg_type(&artists) == DBUS_TYPE_STRING) {
							const char *s = nullptr;
							dbus_message_iter_get_basic(&artists, &s);
							if (s) artist = s;
						}
					}
					dbus_message_iter_next(&dict);
				}
			}
		}
		dbus_message_unref(reply);

		if (title.empty() && artist.empty())
			return false;

		out->active = true;
		out->title = title;
		out->artist = artist;
		out->has_progress = false; // See the comment on NowPlayingInfo in the header.
		return true;
	}

	std::string getStringProperty(const std::string &bus_name, const char *prop)
	{
		DBusMessage *msg = dbus_message_new_method_call(bus_name.c_str(),
			"/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
		if (!msg)
			return "";
		const char *iface = "org.mpris.MediaPlayer2.Player";
		dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface,
			DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

		DBusMessage *reply = dbus_connection_send_with_reply_and_block(
			m_conn, msg, 200, nullptr);
		dbus_message_unref(msg);
		if (!reply)
			return "";

		std::string result;
		DBusMessageIter iter, variant;
		if (dbus_message_iter_init(reply, &iter) &&
				dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
			dbus_message_iter_recurse(&iter, &variant);
			if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING) {
				const char *s = nullptr;
				dbus_message_iter_get_basic(&variant, &s);
				if (s) result = s;
			}
		}
		dbus_message_unref(reply);
		return result;
	}

	DBusConnection *m_conn = nullptr;
};

#elif defined(_WIN32)

#if defined(HAVE_WINRT_SMTC)

using namespace winrt::Windows::Media::Control;

// System Media Transport Controls: the same API backing Windows' own
// media overlay/volume-mixer-adjacent "now playing" widget, so it tracks
// whichever app currently has media focus (Spotify, browser tab, etc.)
// without needing per-app integration.
class NowPlayingProviderImpl
{
public:
	NowPlayingProviderImpl()
	{
		try {
			m_manager_op = GlobalSystemMediaTransportControlsSessionManager::RequestAsync();
		} catch (...) {
			// Leave m_manager_op empty; poll() below just reports nothing.
		}
	}

	~NowPlayingProviderImpl() = default;

	bool poll(NowPlayingInfo *out)
	{
		if (!m_manager) {
			if (!m_manager_op || m_manager_op.Status() != winrt::Windows::Foundation::AsyncStatus::Completed)
				return false;
			try {
				m_manager = m_manager_op.GetResults();
			} catch (...) {
				return false;
			}
		}

		try {
			auto session = m_manager.GetCurrentSession();
			if (!session)
				return false;

			auto playback = session.GetPlaybackInfo();
			if (!playback || playback.PlaybackStatus() !=
					GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing)
				return false;

			auto props_op = session.TryGetMediaPropertiesAsync();
			props_op.get(); // Small, local IPC to the session owner; blocking here is fine.
			auto props = props_op.GetResults();
			if (!props)
				return false;

			out->active = true;
			out->title = winrt::to_string(props.Title());
			out->artist = winrt::to_string(props.Artist());

			auto timeline = session.GetTimelineProperties();
			if (timeline) {
				auto pos = timeline.Position();
				auto dur = timeline.EndTime() - timeline.StartTime();
				long long pos_ms = std::chrono::duration_cast<std::chrono::milliseconds>(pos).count();
				long long dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
				if (dur_ms > 0) {
					out->has_progress = true;
					out->position_seconds = (int)(pos_ms / 1000);
					out->duration_seconds = (int)(dur_ms / 1000);
				}
			}
			return true;
		} catch (...) {
			return false;
		}
	}

private:
	winrt::Windows::Foundation::IAsyncOperation<GlobalSystemMediaTransportControlsSessionManager> m_manager_op{nullptr};
	GlobalSystemMediaTransportControlsSessionManager m_manager{nullptr};
};

#else // !HAVE_WINRT_SMTC

// Fallback when cppwinrt wasn't available at build time (see
// HAVE_WINRT_SMTC in src/CMakeLists.txt): scrape whatever's running for a
// window whose title looks like Spotify's "Artist - Title" format. Title
// and artist only -- no playback position, since a window title can't
// carry that.
class NowPlayingProviderImpl
{
public:
	bool poll(NowPlayingInfo *out)
	{
		HWND wnd = FindWindowW(L"Chrome_WidgetWin_0", nullptr); // Spotify's window class.
		if (!wnd)
			return false;

		wchar_t title[256];
		int len = GetWindowTextW(wnd, title, 256);
		if (len <= 0)
			return false;

		std::wstring wtitle(title, len);
		if (wtitle == L"Spotify" || wtitle == L"Spotify Free" ||
				wtitle == L"Spotify Premium" || wtitle.empty())
			return false; // Idle/paused: Spotify's title reverts to just "Spotify".

		size_t sep = wtitle.find(L" - ");
		if (sep == std::wstring::npos)
			return false;

		out->active = true;
		out->artist = narrow(wtitle.substr(0, sep));
		out->title = narrow(wtitle.substr(sep + 3));
		out->has_progress = false;
		return true;
	}

private:
	static std::string narrow(const std::wstring &s)
	{
		if (s.empty())
			return "";
		int size = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(),
			nullptr, 0, nullptr, nullptr);
		std::string out(size, '\0');
		WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(),
			out.data(), size, nullptr, nullptr);
		return out;
	}
};

#endif // HAVE_WINRT_SMTC

#elif defined(__APPLE__)

namespace {
	// MediaRemote is a private, undocumented Apple framework -- there's
	// no public API for reading *another* app's now-playing info on
	// macOS. This is the same technique widely-used third-party "now
	// playing" tools rely on: dlopen the framework at its fixed system
	// path and dlsym the function it exports, since there's no header to
	// link against normally. Being private/undocumented, this can in
	// principle stop working in a future macOS release; if it ever does,
	// dlopen/dlsym simply fail and poll() below always returns false,
	// same as "unsupported platform" -- no crash, no code change needed
	// to keep working everywhere else.
	typedef void (^NowPlayingInfoBlock)(CFDictionaryRef information);
	typedef void (*GetNowPlayingInfoFunc)(dispatch_queue_t queue, NowPlayingInfoBlock handler);

	std::string cfStringToStd(CFStringRef s)
	{
		if (!s)
			return "";
		CFIndex len = CFStringGetLength(s);
		CFIndex max_size = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8) + 1;
		std::string out(max_size, '\0');
		if (!CFStringGetCString(s, out.data(), max_size, kCFStringEncodingUTF8))
			return "";
		out.resize(strlen(out.c_str()));
		return out;
	}
}

// poll() never blocks: MediaRemote's own API is callback-based, so this
// kicks off a fresh async request each call and returns whatever the
// *previous* request produced -- always one poll cycle (up to
// POLL_INTERVAL_MS) behind, unnoticeable for a "what's playing" HUD, and
// avoids blocking the render thread on an inter-process call to a system
// daemon (mediaremoted).
class NowPlayingProviderImpl
{
public:
	NowPlayingProviderImpl()
	{
		m_handle = dlopen(
			"/System/Library/PrivateFrameworks/MediaRemote.framework/MediaRemote",
			RTLD_LAZY);
		if (!m_handle)
			return;
		m_get_info = (GetNowPlayingInfoFunc)dlsym(m_handle, "MRMediaRemoteGetNowPlayingInfo");
		if (!m_get_info)
			return;
		m_queue = dispatch_queue_create("org.eclipseclient.nowplaying", DISPATCH_QUEUE_SERIAL);
	}

	~NowPlayingProviderImpl()
	{
		if (m_handle)
			dlclose(m_handle);
	}

	bool poll(NowPlayingInfo *out)
	{
		if (!m_get_info)
			return false;

		bool have_result;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			have_result = m_ready && m_latest.active;
			if (have_result)
				*out = m_latest;
		}
		requestUpdate();
		return have_result;
	}

private:
	void requestUpdate()
	{
		if (m_request_pending.exchange(true))
			return; // Previous request hasn't come back yet.

		NowPlayingProviderImpl *self = this;
		m_get_info(m_queue, ^(CFDictionaryRef info) {
			self->handleInfo(info);
			self->m_request_pending = false;
		});
	}

	void handleInfo(CFDictionaryRef info)
	{
		NowPlayingInfo result;
		if (info) {
			CFStringRef title = (CFStringRef)CFDictionaryGetValue(
				info, CFSTR("kMRMediaRemoteNowPlayingInfoTitle"));
			CFStringRef artist = (CFStringRef)CFDictionaryGetValue(
				info, CFSTR("kMRMediaRemoteNowPlayingInfoArtist"));

			if (title || artist) {
				result.active = true;
				result.title = cfStringToStd(title);
				result.artist = cfStringToStd(artist);

				CFNumberRef elapsed = (CFNumberRef)CFDictionaryGetValue(
					info, CFSTR("kMRMediaRemoteNowPlayingInfoElapsedTime"));
				CFNumberRef duration = (CFNumberRef)CFDictionaryGetValue(
					info, CFSTR("kMRMediaRemoteNowPlayingInfoDuration"));
				double elapsed_s = 0.0, duration_s = 0.0;
				if (elapsed && CFNumberGetValue(elapsed, kCFNumberDoubleType, &elapsed_s) &&
						duration && CFNumberGetValue(duration, kCFNumberDoubleType, &duration_s) &&
						duration_s > 0.0) {
					result.has_progress = true;
					result.position_seconds = (int)elapsed_s;
					result.duration_seconds = (int)duration_s;
				}
			}
		}

		std::lock_guard<std::mutex> lock(m_mutex);
		m_latest = result;
		m_ready = true;
	}

	void *m_handle = nullptr;
	GetNowPlayingInfoFunc m_get_info = nullptr;
	dispatch_queue_t m_queue = nullptr;
	std::atomic<bool> m_request_pending{false};
	std::mutex m_mutex;
	bool m_ready = false;
	NowPlayingInfo m_latest;
};

#else // Unsupported platform.

class NowPlayingProviderImpl
{
public:
	bool poll(NowPlayingInfo *) { return false; }
};

#endif

NowPlayingProvider::NowPlayingProvider() : m_impl(new NowPlayingProviderImpl()) {}

NowPlayingProvider::~NowPlayingProvider()
{
	delete m_impl;
}

const NowPlayingInfo &NowPlayingProvider::poll()
{
	unsigned long long now = porting::getTimeMs();
	if (now - m_last_poll_ms < POLL_INTERVAL_MS && m_last_poll_ms != 0)
		return m_last;
	m_last_poll_ms = now;

	NowPlayingInfo info;
	if (m_impl->poll(&info))
		m_last = info;
	else
		m_last = NowPlayingInfo(); // Nothing playing (or unsupported platform).

	return m_last;
}
