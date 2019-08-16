/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "gs-texture.hpp"
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>
#include "obs/gs/gs-helper.hpp"
#include "util-math.hpp"

// OBS
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <obs.h>
#include <util/platform.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

gs::texture::texture(uint32_t width, uint32_t height, gs_color_format format, uint32_t mip_levels,
					 const uint8_t** mip_data, gs::texture::flags texture_flags)
{
	if (width == 0)
		throw std::logic_error("width must be at least 1");
	if (height == 0)
		throw std::logic_error("height must be at least 1");
	if (mip_levels == 0)
		throw std::logic_error("mip_levels must be at least 1");

	if (mip_levels > 1 || ((texture_flags & flags::BuildMipMaps) == flags::BuildMipMaps)) {
		bool isPOT = util::math::is_power_of_two(width) && util::math::is_power_of_two(height);
		if (!isPOT)
			throw std::logic_error("mip mapping requires power of two dimensions");
	}

	auto gctx = gs::context();
	_texture = gs_texture_create(
		width, height, format, mip_levels, mip_data,
		(((texture_flags & flags::Dynamic) == flags::Dynamic) ? GS_DYNAMIC : 0)
			| (((texture_flags & flags::BuildMipMaps) == flags::BuildMipMaps) ? GS_BUILD_MIPMAPS : 0));

	if (!_texture)
		throw std::runtime_error("Failed to create texture.");

	_type = type::Normal;
}

gs::texture::texture(uint32_t width, uint32_t height, uint32_t depth, gs_color_format format, uint32_t mip_levels,
					 const uint8_t** mip_data, gs::texture::flags texture_flags)
{
	if (width == 0)
		throw std::logic_error("width must be at least 1");
	if (height == 0)
		throw std::logic_error("height must be at least 1");
	if (depth == 0)
		throw std::logic_error("depth must be at least 1");
	if (mip_levels == 0)
		throw std::logic_error("mip_levels must be at least 1");

	if (mip_levels > 1 || ((texture_flags & flags::BuildMipMaps) == flags::BuildMipMaps)) {
		bool isPOT = (util::math::is_equal(pow(2, (int64_t)floor(log(width) / log(2))), width)
					  && util::math::is_equal(pow(2, (int64_t)floor(log(height) / log(2))), height)
					  && util::math::is_equal(pow(2, (int64_t)floor(log(depth) / log(2))), depth));
		if (!isPOT)
			throw std::logic_error("mip mapping requires power of two dimensions");
	}

	auto gctx = gs::context();
	_texture = gs_voltexture_create(
		width, height, depth, format, mip_levels, mip_data,
		(((texture_flags & flags::Dynamic) == flags::Dynamic) ? GS_DYNAMIC : 0)
			| (((texture_flags & flags::BuildMipMaps) == flags::BuildMipMaps) ? GS_BUILD_MIPMAPS : 0));

	if (!_texture)
		throw std::runtime_error("Failed to create texture.");

	_type = type::Volume;
}

gs::texture::texture(uint32_t size, gs_color_format format, uint32_t mip_levels, const uint8_t** mip_data,
					 gs::texture::flags texture_flags)
{
	if (size == 0)
		throw std::logic_error("size must be at least 1");
	if (mip_levels == 0)
		throw std::logic_error("mip_levels must be at least 1");

	if (mip_levels > 1 || ((texture_flags & flags::BuildMipMaps) == flags::BuildMipMaps)) {
		bool isPOT = util::math::is_equal(pow(2, (int64_t)floor(log(size) / log(2))), size);
		if (!isPOT)
			throw std::logic_error("mip mapping requires power of two dimensions");
	}

	auto gctx = gs::context();
	_texture = gs_cubetexture_create(
		size, format, mip_levels, mip_data,
		(((texture_flags & flags::Dynamic) == flags::Dynamic) ? GS_DYNAMIC : 0)
			| (((texture_flags & flags::BuildMipMaps) == flags::BuildMipMaps) ? GS_BUILD_MIPMAPS : 0));

	if (!_texture)
		throw std::runtime_error("Failed to create texture.");

	_type = type::Cube;
}

gs::texture::texture(std::string file)
{
	struct stat st;
	if (os_stat(file.c_str(), &st) != 0)
		throw std::ios_base::failure(file);

	auto gctx = gs::context();
	_texture = gs_texture_create_from_file(file.c_str());

	if (!_texture)
		throw std::runtime_error("Failed to load texture.");
}

gs::texture::~texture()
{
	if (_is_owner && _texture) {
		auto gctx = gs::context();
		switch (gs_get_texture_type(_texture)) {
		case GS_TEXTURE_2D:
			gs_texture_destroy(_texture);
			break;
		case GS_TEXTURE_3D:
			gs_voltexture_destroy(_texture);
			break;
		case GS_TEXTURE_CUBE:
			gs_cubetexture_destroy(_texture);
			break;
		}
	}
	_texture = nullptr;
}

void gs::texture::load(int unit)
{
	auto gctx = gs::context();
	gs_load_texture(_texture, unit);
}

gs_texture_t* gs::texture::get_object()
{
	return _texture;
}

uint32_t gs::texture::get_width()
{
	switch (_type) {
	case type::Normal:
		return gs_texture_get_width(_texture);
	case type::Volume:
		return gs_voltexture_get_width(_texture);
	case type::Cube:
		return gs_cubetexture_get_size(_texture);
	}
	return 0;
}

uint32_t gs::texture::get_height()
{
	switch (_type) {
	case type::Normal:
		return gs_texture_get_height(_texture);
	case type::Volume:
		return gs_voltexture_get_height(_texture);
	case type::Cube:
		return gs_cubetexture_get_size(_texture);
	}
	return 0;
}

uint32_t gs::texture::get_depth()
{
	switch (_type) {
	case type::Normal:
		return 1;
	case type::Volume:
		return gs_voltexture_get_depth(_texture);
	case type::Cube:
		return 6;
	}
	return 0;
}

gs::texture::type gs::texture::get_type()
{
	return _type;
}

gs_color_format gs::texture::get_color_format()
{
	return gs_texture_get_color_format(_texture);
}
