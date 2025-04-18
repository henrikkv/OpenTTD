/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file xaudio2_s.h Base for XAudio2 sound handling. */

#ifndef SOUND_XAUDIO2_H
#define SOUND_XAUDIO2_H

#include "sound_driver.hpp"

/** Implementation of the XAudio2 sound driver. */
class SoundDriver_XAudio2 : public SoundDriver {
public:
	std::optional<std::string_view> Start(const StringList &param) override;

	void Stop() override;
	std::string_view GetName() const override { return "xaudio2"; }
};

/** Factory for the XAudio2 sound driver. */
class FSoundDriver_XAudio2 : public DriverFactoryBase {
public:
	FSoundDriver_XAudio2() : DriverFactoryBase(Driver::DT_SOUND, 10, "xaudio2", "XAudio2 Sound Driver (param hz,samples)") {}
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<SoundDriver_XAudio2>(); }
};

#endif /* SOUND_XAUDIO2_H */
