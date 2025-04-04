/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file cocoa_ogl.h The Cocoa OpenGL video driver. */

#ifndef VIDEO_COCOA_OGL_H
#define VIDEO_COCOA_OGL_H

#include "cocoa_v.h"

@class OTTD_OpenGLView;

class VideoDriver_CocoaOpenGL : public VideoDriver_Cocoa {
	CGLContextObj gl_context;

	uint8_t *anim_buffer; ///< Animation buffer from OpenGL back-end.
	std::string driver_info; ///< Information string about selected driver.

	std::optional<std::string_view> AllocateContext(bool allow_software);

public:
	VideoDriver_CocoaOpenGL() : VideoDriver_Cocoa(true), gl_context(nullptr), anim_buffer(nullptr), driver_info(this->GetName()) {}

	std::optional<std::string_view> Start(const StringList &param) override;
	void Stop() override;

	bool HasEfficient8Bpp() const override { return true; }

	bool UseSystemCursor() override { return true; }

	void ClearSystemSprites() override;

	void PopulateSystemSprites() override;

	bool HasAnimBuffer() override { return true; }
	uint8_t *GetAnimBuffer() override { return this->anim_buffer; }

	/** Return driver name */
	std::string_view GetName() const override { return "cocoa-opengl"; }

	std::string_view GetInfoString() const override { return this->driver_info; }

	void AllocateBackingStore(bool force = false) override;

protected:
	void Paint() override;

	void *GetVideoPointer() override;
	void ReleaseVideoPointer() override;

	NSView *AllocateDrawView() override;
};

class FVideoDriver_CocoaOpenGL : public DriverFactoryBase {
public:
	FVideoDriver_CocoaOpenGL() : DriverFactoryBase(Driver::DT_VIDEO, 9, "cocoa-opengl", "Cocoa OpenGL Video Driver") {}
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<VideoDriver_CocoaOpenGL>(); }

protected:
	bool UsesHardwareAcceleration() const override { return true; }
};

#endif /* VIDEO_COCOA_OGL_H */
