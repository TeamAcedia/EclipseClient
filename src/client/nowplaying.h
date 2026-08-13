// Eclipse
// Copyright (C) 2025 ProunceDev
// MIT License
//
// Cross-platform "what's currently playing" detection, feeding
// NowPlayingHudElement in hud_elements.cpp. Three independent backends,
// selected at compile time (see nowplaying.cpp):
//   - Linux: MPRIS2 over D-Bus (the standard most Linux media players/
//     browsers implement) -- needs libdbus-1-dev at build time.
//   - Windows: System Media Transport Controls via WinRT (title, artist,
//     playback position) -- needs the cppwinrt package at build time;
//     falls back to reading the Spotify window's title bar (title/artist
//     only, no position) if that's unavailable.
//   - macOS: the private MediaRemote framework, loaded at runtime via
//     dlopen (there's no public API for reading *another* app's
//     now-playing info on macOS) -- no extra build-time dependency.
// On any other platform, or if none of the above is available, polling
// just always reports "nothing playing" -- this file is safe to compile
// and link everywhere.

#pragma once

#include <string>

struct NowPlayingInfo
{
	bool active = false; // Something is actually playing right now.
	std::string title;
	std::string artist;

	// Playback position, when the backend can provide it (Windows SMTC
	// and macOS; not MPRIS/Linux, whose position property most players
	// don't bother implementing accurately enough to be worth showing).
	bool has_progress = false;
	int position_seconds = 0;
	int duration_seconds = 0;
};

class NowPlayingProviderImpl;

class NowPlayingProvider
{
public:
	NowPlayingProvider();
	~NowPlayingProvider();

	// Internally throttled (see POLL_INTERVAL_MS in nowplaying.cpp) --
	// safe to call every frame from NowPlayingHudElement::render();
	// actual backend queries only happen periodically.
	const NowPlayingInfo &poll();

private:
	NowPlayingProviderImpl *m_impl;
	NowPlayingInfo m_last;
	unsigned long long m_last_poll_ms = 0;
};
