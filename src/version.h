// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <string>
#include "util/hashing.h"
#include <sstream>
#include <iomanip>
#include "log.h"

extern const char *g_version_string;
extern const char *g_version_hash;
extern const char *g_build_info;
extern const char *g_eclipse_version_string;
extern const char *g_eclipse_official_hash;

inline std::string hash_against_official_build_hash(const std::string &target)
{
	const std::string digest = hashing::sha256(target + g_eclipse_official_hash);

	std::ostringstream oss;
	oss << std::hex << std::setfill('0');

	for (unsigned char byte : digest) {
		oss << std::setw(2) << static_cast<int>(byte);
	}

	return oss.str();
}
