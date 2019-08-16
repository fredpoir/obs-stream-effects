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

#include "filter-color-grade.hpp"
#include "strings.hpp"
#include "util-math.hpp"

// OBS
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <util/platform.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define ST "Filter.ColorGrade"
#define ST_TOOL ST ".Tool"
#define ST_LIFT ST ".Lift"
#define ST_LIFT_(x) ST_LIFT "." D_VSTR(x)
#define ST_GAMMA ST ".Gamma"
#define ST_GAMMA_(x) ST_GAMMA "." D_VSTR(x)
#define ST_GAIN ST ".Gain"
#define ST_GAIN_(x) ST_GAIN "." D_VSTR(x)
#define ST_OFFSET ST ".Offset"
#define ST_OFFSET_(x) ST_OFFSET "." D_VSTR(x)
#define ST_TINT ST ".Tint"
#define ST_TINT_(x, y) ST_TINT "." D_VSTR(x) "." D_VSTR(y)
#define ST_CORRECTION ST ".Correction"
#define ST_CORRECTION_(x) ST_CORRECTION "." D_VSTR(x)

#define RED Red
#define GREEN Green
#define BLUE Blue
#define ALL All
#define HUE Hue
#define SATURATION Saturation
#define LIGHTNESS Lightness
#define CONTRAST Contrast
#define TONE_LOW Shadow
#define TONE_MID Midtone
#define TONE_HIG Highlight

// Initializer & Finalizer
P_INITIALIZER(FilterColorGradeInit)
{
	initializer_functions.push_back([] { filter::color_grade::color_grade_factory::initialize(); });
	finalizer_functions.push_back([] { filter::color_grade::color_grade_factory::finalize(); });
}

const char* get_name(void*)
{
	return D_TRANSLATE(ST);
}

void get_defaults(obs_data_t* data)
{
	obs_data_set_default_string(data, ST_TOOL, ST_CORRECTION);
	obs_data_set_default_double(data, ST_LIFT_(RED), 0);
	obs_data_set_default_double(data, ST_LIFT_(GREEN), 0);
	obs_data_set_default_double(data, ST_LIFT_(BLUE), 0);
	obs_data_set_default_double(data, ST_LIFT_(ALL), 0);
	obs_data_set_default_double(data, ST_GAMMA_(RED), 0);
	obs_data_set_default_double(data, ST_GAMMA_(GREEN), 0);
	obs_data_set_default_double(data, ST_GAMMA_(BLUE), 0);
	obs_data_set_default_double(data, ST_GAMMA_(ALL), 0);
	obs_data_set_default_double(data, ST_GAIN_(RED), 100.0);
	obs_data_set_default_double(data, ST_GAIN_(GREEN), 100.0);
	obs_data_set_default_double(data, ST_GAIN_(BLUE), 100.0);
	obs_data_set_default_double(data, ST_GAIN_(ALL), 100.0);
	obs_data_set_default_double(data, ST_OFFSET_(RED), 0.0);
	obs_data_set_default_double(data, ST_OFFSET_(GREEN), 0.0);
	obs_data_set_default_double(data, ST_OFFSET_(BLUE), 0.0);
	obs_data_set_default_double(data, ST_OFFSET_(ALL), 0.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_LOW, RED), 100.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_LOW, GREEN), 100.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_LOW, BLUE), 100.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_MID, RED), 100.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_MID, GREEN), 100.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_MID, BLUE), 100.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_HIG, RED), 100.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_HIG, GREEN), 100.0);
	obs_data_set_default_double(data, ST_TINT_(TONE_HIG, BLUE), 100.0);
	obs_data_set_default_double(data, ST_CORRECTION_(HUE), 0.0);
	obs_data_set_default_double(data, ST_CORRECTION_(SATURATION), 100.0);
	obs_data_set_default_double(data, ST_CORRECTION_(LIGHTNESS), 100.0);
	obs_data_set_default_double(data, ST_CORRECTION_(CONTRAST), 100.0);
}

bool tool_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings)
{
	const std::string mode{obs_data_get_string(settings, ST_TOOL)};

	std::vector<std::pair<std::string, std::vector<std::string>>> tool_to_property{
		{ST_LIFT, {ST_LIFT_(RED), ST_LIFT_(GREEN), ST_LIFT_(BLUE), ST_LIFT_(ALL)}},
		{ST_GAMMA, {ST_GAMMA_(RED), ST_GAMMA_(GREEN), ST_GAMMA_(BLUE), ST_GAMMA_(ALL)}},
		{ST_GAIN, {ST_GAIN_(RED), ST_GAIN_(GREEN), ST_GAIN_(BLUE), ST_GAIN_(ALL)}},
		{ST_OFFSET, {ST_OFFSET_(RED), ST_OFFSET_(GREEN), ST_OFFSET_(BLUE), ST_OFFSET_(ALL)}},
		{ST_TINT,
		 {
			 ST_TINT_(TONE_LOW, RED),
			 ST_TINT_(TONE_LOW, GREEN),
			 ST_TINT_(TONE_LOW, BLUE),
			 ST_TINT_(TONE_MID, RED),
			 ST_TINT_(TONE_MID, GREEN),
			 ST_TINT_(TONE_MID, BLUE),
			 ST_TINT_(TONE_HIG, RED),
			 ST_TINT_(TONE_HIG, GREEN),
			 ST_TINT_(TONE_HIG, BLUE),
		 }},
		{ST_CORRECTION,
		 {ST_CORRECTION_(HUE), ST_CORRECTION_(SATURATION), ST_CORRECTION_(LIGHTNESS), ST_CORRECTION_(CONTRAST)}},
	};

	for (auto kv : tool_to_property) {
		if (mode == kv.first) {
			for (auto kv2 : kv.second) {
				obs_property_set_visible(obs_properties_get(props, kv2.c_str()), true);
			}
		} else {
			for (auto kv2 : kv.second) {
				obs_property_set_visible(obs_properties_get(props, kv2.c_str()), false);
			}
		}
	}

	return true;
}

obs_properties_t* get_properties(void*)
{
	obs_properties_t* pr = obs_properties_create();

	if (util::are_property_groups_broken()) {
		auto p =
			obs_properties_add_list(pr, ST_TOOL, D_TRANSLATE(ST_TOOL), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
		obs_property_list_add_string(p, D_TRANSLATE(ST_LIFT), ST_LIFT);
		obs_property_list_add_string(p, D_TRANSLATE(ST_GAMMA), ST_GAMMA);
		obs_property_list_add_string(p, D_TRANSLATE(ST_GAIN), ST_GAIN);
		obs_property_list_add_string(p, D_TRANSLATE(ST_OFFSET), ST_OFFSET);
		obs_property_list_add_string(p, D_TRANSLATE(ST_TINT), ST_TINT);
		obs_property_list_add_string(p, D_TRANSLATE(ST_CORRECTION), ST_CORRECTION);
		obs_property_set_modified_callback(p, &tool_modified);
	}

	{
		obs_properties_t* grp = pr;
		if (!util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(pr, ST_LIFT, D_TRANSLATE(ST_LIFT), OBS_GROUP_NORMAL, grp);
		}

		obs_properties_add_float_slider(grp, ST_LIFT_(RED), D_TRANSLATE(ST_LIFT_(RED)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_LIFT_(GREEN), D_TRANSLATE(ST_LIFT_(GREEN)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_LIFT_(BLUE), D_TRANSLATE(ST_LIFT_(BLUE)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_LIFT_(ALL), D_TRANSLATE(ST_LIFT_(ALL)), -1000.0, 1000.0, 0.01);
	}

	{
		obs_properties_t* grp = pr;
		if (!util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(pr, ST_GAMMA, D_TRANSLATE(ST_GAMMA), OBS_GROUP_NORMAL, grp);
		}

		obs_properties_add_float_slider(grp, ST_GAMMA_(RED), D_TRANSLATE(ST_GAMMA_(RED)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_GAMMA_(GREEN), D_TRANSLATE(ST_GAMMA_(GREEN)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_GAMMA_(BLUE), D_TRANSLATE(ST_GAMMA_(BLUE)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_GAMMA_(ALL), D_TRANSLATE(ST_GAMMA_(ALL)), -1000.0, 1000.0, 0.01);
	}

	{
		obs_properties_t* grp = pr;
		if (!util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(pr, ST_GAIN, D_TRANSLATE(ST_GAIN), OBS_GROUP_NORMAL, grp);
		}

		obs_properties_add_float_slider(grp, ST_GAIN_(RED), D_TRANSLATE(ST_GAIN_(RED)), 0.01, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_GAIN_(GREEN), D_TRANSLATE(ST_GAIN_(GREEN)), 0.01, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_GAIN_(BLUE), D_TRANSLATE(ST_GAIN_(BLUE)), 0.01, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_GAIN_(ALL), D_TRANSLATE(ST_GAIN_(ALL)), 0.01, 1000.0, 0.01);
	}

	{
		obs_properties_t* grp = pr;
		if (!util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(pr, ST_OFFSET, D_TRANSLATE(ST_OFFSET), OBS_GROUP_NORMAL, grp);
		}

		obs_properties_add_float_slider(grp, ST_OFFSET_(RED), D_TRANSLATE(ST_OFFSET_(RED)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_OFFSET_(GREEN), D_TRANSLATE(ST_OFFSET_(GREEN)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_OFFSET_(BLUE), D_TRANSLATE(ST_OFFSET_(BLUE)), -1000.0, 1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_OFFSET_(ALL), D_TRANSLATE(ST_OFFSET_(ALL)), -1000.0, 1000.0, 0.01);
	}

	{
		obs_properties_t* grp = pr;
		if (!util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(pr, ST_TINT, D_TRANSLATE(ST_TINT), OBS_GROUP_NORMAL, grp);
		}

		obs_properties_add_float_slider(grp, ST_TINT_(TONE_LOW, RED), D_TRANSLATE(ST_TINT_(TONE_LOW, RED)), 0, 1000.0,
										0.01);
		obs_properties_add_float_slider(grp, ST_TINT_(TONE_LOW, GREEN), D_TRANSLATE(ST_TINT_(TONE_LOW, GREEN)), 0,
										1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_TINT_(TONE_LOW, BLUE), D_TRANSLATE(ST_TINT_(TONE_LOW, BLUE)), 0, 1000.0,
										0.01);

		obs_properties_add_float_slider(grp, ST_TINT_(TONE_MID, RED), D_TRANSLATE(ST_TINT_(TONE_MID, RED)), 0, 1000.0,
										0.01);
		obs_properties_add_float_slider(grp, ST_TINT_(TONE_MID, GREEN), D_TRANSLATE(ST_TINT_(TONE_MID, GREEN)), 0,
										1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_TINT_(TONE_MID, BLUE), D_TRANSLATE(ST_TINT_(TONE_MID, BLUE)), 0, 1000.0,
										0.01);

		obs_properties_add_float_slider(grp, ST_TINT_(TONE_HIG, RED), D_TRANSLATE(ST_TINT_(TONE_HIG, RED)), 0, 1000.0,
										0.01);
		obs_properties_add_float_slider(grp, ST_TINT_(TONE_HIG, GREEN), D_TRANSLATE(ST_TINT_(TONE_HIG, GREEN)), 0,
										1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_TINT_(TONE_HIG, BLUE), D_TRANSLATE(ST_TINT_(TONE_HIG, BLUE)), 0, 1000.0,
										0.01);
	}

	{
		obs_properties_t* grp = pr;
		if (!util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(pr, ST_CORRECTION, D_TRANSLATE(ST_CORRECTION), OBS_GROUP_NORMAL, grp);
		}

		obs_properties_add_float_slider(grp, ST_CORRECTION_(HUE), D_TRANSLATE(ST_CORRECTION_(HUE)), -180, 180.0, 0.01);
		obs_properties_add_float_slider(grp, ST_CORRECTION_(SATURATION), D_TRANSLATE(ST_CORRECTION_(SATURATION)), 0.0,
										1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_CORRECTION_(LIGHTNESS), D_TRANSLATE(ST_CORRECTION_(LIGHTNESS)), 0.0,
										1000.0, 0.01);
		obs_properties_add_float_slider(grp, ST_CORRECTION_(CONTRAST), D_TRANSLATE(ST_CORRECTION_(CONTRAST)), 0.0,
										1000.0, 0.01);
	}

	return pr;
}

void* create(obs_data_t* data, obs_source_t* source)
try {
	return new filter::color_grade::color_grade_instance(data, source);
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to create: %s", obs_source_get_name(source), ex.what());
	return nullptr;
}

void destroy(void* ptr)
try {
	delete reinterpret_cast<filter::color_grade::color_grade_instance*>(ptr);
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to destroy: %s", ex.what());
}

uint32_t get_width(void* ptr)
try {
	return reinterpret_cast<filter::color_grade::color_grade_instance*>(ptr)->get_width();
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to get width: %s", ex.what());
	return 0;
}

uint32_t get_height(void* ptr)
try {
	return reinterpret_cast<filter::color_grade::color_grade_instance*>(ptr)->get_height();
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to get height: %s", ex.what());
	return 0;
}

void update(void* ptr, obs_data_t* data)
try {
	reinterpret_cast<filter::color_grade::color_grade_instance*>(ptr)->update(data);
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to update: %s", ex.what());
}

void activate(void* ptr)
try {
	reinterpret_cast<filter::color_grade::color_grade_instance*>(ptr)->activate();
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to activate: %s", ex.what());
}

void deactivate(void* ptr)
try {
	reinterpret_cast<filter::color_grade::color_grade_instance*>(ptr)->deactivate();
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to deactivate: %s", ex.what());
}

void video_tick(void* ptr, float time)
try {
	reinterpret_cast<filter::color_grade::color_grade_instance*>(ptr)->video_tick(time);
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to tick video: %s", ex.what());
}

void video_render(void* ptr, gs_effect_t* effect)
try {
	reinterpret_cast<filter::color_grade::color_grade_instance*>(ptr)->video_render(effect);
} catch (std::exception& ex) {
	P_LOG_ERROR("<filter-color-grade> Failed to render video: %s", ex.what());
}

static std::shared_ptr<filter::color_grade::color_grade_factory> factory_instance = nullptr;

void filter::color_grade::color_grade_factory::initialize()
{
	factory_instance = std::make_shared<filter::color_grade::color_grade_factory>();
}

void filter::color_grade::color_grade_factory::finalize()
{
	factory_instance.reset();
}

std::shared_ptr<filter::color_grade::color_grade_factory> filter::color_grade::color_grade_factory::get()
{
	return factory_instance;
}

filter::color_grade::color_grade_factory::color_grade_factory()
{
	memset(&sourceInfo, 0, sizeof(obs_source_info));
	sourceInfo.id             = "obs-stream-effects-filter-color-grade";
	sourceInfo.type           = OBS_SOURCE_TYPE_FILTER;
	sourceInfo.output_flags   = OBS_SOURCE_VIDEO;
	sourceInfo.get_name       = get_name;
	sourceInfo.get_defaults   = get_defaults;
	sourceInfo.get_properties = get_properties;

	sourceInfo.create       = create;
	sourceInfo.destroy      = destroy;
	sourceInfo.update       = update;
	sourceInfo.activate     = activate;
	sourceInfo.deactivate   = deactivate;
	sourceInfo.video_tick   = video_tick;
	sourceInfo.video_render = video_render;

	obs_register_source(&sourceInfo);
}

filter::color_grade::color_grade_factory::~color_grade_factory() {}

filter::color_grade::color_grade_instance::~color_grade_instance() {}

filter::color_grade::color_grade_instance::color_grade_instance(obs_data_t* data, obs_source_t* context)
	: _active(true), _self(context)
{
	update(data);

	{
		char* file = obs_module_file("effects/color-grade.effect");
		if (file) {
			try {
				_effect = gs::effect::create(file);
				bfree(file);
			} catch (std::runtime_error& ex) {
				P_LOG_ERROR("<filter-color-grade> Loading _effect '%s' failed with error(s): %s", file, ex.what());
				bfree(file);
				throw ex;
			}
		} else {
			throw std::runtime_error("Missing file color-grade.effect.");
		}
	}
	{
		_rt_source = std::make_unique<gs::rendertarget>(GS_RGBA, GS_ZS_NONE);
		{
			auto op = _rt_source->render(1, 1);
		}
		_tex_source = _rt_source->get_texture();
	}
	{
		_rt_grade = std::make_unique<gs::rendertarget>(GS_RGBA, GS_ZS_NONE);
		{
			auto op = _rt_grade->render(1, 1);
		}
		_tex_grade = _rt_grade->get_texture();
	}
}

uint32_t filter::color_grade::color_grade_instance::get_width()
{
	return 0;
}

uint32_t filter::color_grade::color_grade_instance::get_height()
{
	return 0;
}

float_t fix_gamma_value(double_t v)
{
	if (v < 0.0) {
		return static_cast<float_t>(-v + 1.0);
	} else {
		return static_cast<float_t>(1.0 / (v + 1.0));
	}
}

void filter::color_grade::color_grade_instance::update(obs_data_t* data)
{
	_lift.x       = static_cast<float_t>(obs_data_get_double(data, ST_LIFT_(RED)) / 100.0);
	_lift.y       = static_cast<float_t>(obs_data_get_double(data, ST_LIFT_(GREEN)) / 100.0);
	_lift.z       = static_cast<float_t>(obs_data_get_double(data, ST_LIFT_(BLUE)) / 100.0);
	_lift.w       = static_cast<float_t>(obs_data_get_double(data, ST_LIFT_(ALL)) / 100.0);
	_gamma.x      = fix_gamma_value(obs_data_get_double(data, ST_GAMMA_(RED)) / 100.0);
	_gamma.y      = fix_gamma_value(obs_data_get_double(data, ST_GAMMA_(GREEN)) / 100.0);
	_gamma.z      = fix_gamma_value(obs_data_get_double(data, ST_GAMMA_(BLUE)) / 100.0);
	_gamma.w      = fix_gamma_value(obs_data_get_double(data, ST_GAMMA_(ALL)) / 100.0);
	_gain.x       = static_cast<float_t>(obs_data_get_double(data, ST_GAIN_(RED)) / 100.0);
	_gain.y       = static_cast<float_t>(obs_data_get_double(data, ST_GAIN_(GREEN)) / 100.0);
	_gain.z       = static_cast<float_t>(obs_data_get_double(data, ST_GAIN_(BLUE)) / 100.0);
	_gain.w       = static_cast<float_t>(obs_data_get_double(data, ST_GAIN_(ALL)) / 100.0);
	_offset.x     = static_cast<float_t>(obs_data_get_double(data, ST_OFFSET_(RED)) / 100.0);
	_offset.y     = static_cast<float_t>(obs_data_get_double(data, ST_OFFSET_(GREEN)) / 100.0);
	_offset.z     = static_cast<float_t>(obs_data_get_double(data, ST_OFFSET_(BLUE)) / 100.0);
	_offset.w     = static_cast<float_t>(obs_data_get_double(data, ST_OFFSET_(ALL)) / 100.0);
	_tint_low.x   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_LOW, RED)) / 100.0);
	_tint_low.y   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_LOW, GREEN)) / 100.0);
	_tint_low.z   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_LOW, BLUE)) / 100.0);
	_tint_mid.x   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_MID, RED)) / 100.0);
	_tint_mid.y   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_MID, GREEN)) / 100.0);
	_tint_mid.z   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_MID, BLUE)) / 100.0);
	_tint_hig.x   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_HIG, RED)) / 100.0);
	_tint_hig.y   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_HIG, GREEN)) / 100.0);
	_tint_hig.z   = static_cast<float_t>(obs_data_get_double(data, ST_TINT_(TONE_HIG, BLUE)) / 100.0);
	_correction.x = static_cast<float_t>(obs_data_get_double(data, ST_CORRECTION_(HUE)) / 360.0);
	_correction.y = static_cast<float_t>(obs_data_get_double(data, ST_CORRECTION_(SATURATION)) / 100.0);
	_correction.z = static_cast<float_t>(obs_data_get_double(data, ST_CORRECTION_(LIGHTNESS)) / 100.0);
	_correction.w = static_cast<float_t>(obs_data_get_double(data, ST_CORRECTION_(CONTRAST)) / 100.0);
}

void filter::color_grade::color_grade_instance::activate()
{
	_active = true;
}

void filter::color_grade::color_grade_instance::deactivate()
{
	_active = false;
}

void filter::color_grade::color_grade_instance::video_tick(float)
{
	_source_updated = false;
	_grade_updated  = false;
}

void filter::color_grade::color_grade_instance::video_render(gs_effect_t*)
{
	// Grab initial values.
	obs_source_t* parent         = obs_filter_get_parent(_self);
	obs_source_t* target         = obs_filter_get_target(_self);
	uint32_t      width          = obs_source_get_base_width(target);
	uint32_t      height         = obs_source_get_base_height(target);
	gs_effect_t*  effect_default = obs_get_base_effect(obs_base_effect::OBS_EFFECT_DEFAULT);

	// Skip filter if anything is wrong.
	if (!_active || !parent || !target || !width || !height || !effect_default) {
		obs_source_skip_video_filter(_self);
		return;
	}

	if (!_source_updated) {
		if (obs_source_process_filter_begin(_self, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
			auto op = _rt_source->render(width, height);
			gs_blend_state_push();
			gs_reset_blend_state();
			gs_set_cull_mode(GS_NEITHER);
			gs_enable_color(true, true, true, true);
			gs_enable_blending(false);
			gs_enable_depth_test(false);
			gs_enable_stencil_test(false);
			gs_enable_stencil_write(false);
			gs_ortho(0, static_cast<float_t>(width), 0, static_cast<float_t>(height), -1., 1.);
			obs_source_process_filter_end(_self, effect_default, width, height);
			gs_blend_state_pop();
		}

		_tex_source     = _rt_source->get_texture();
		_source_updated = true;
	}

	if (!_grade_updated) {
		{
			auto op = _rt_grade->render(width, height);
			gs_blend_state_push();
			gs_reset_blend_state();
			gs_set_cull_mode(GS_NEITHER);
			gs_enable_color(true, true, true, true);
			gs_enable_blending(false);
			gs_enable_depth_test(false);
			gs_enable_stencil_test(false);
			gs_enable_stencil_write(false);
			gs_ortho(0, static_cast<float_t>(width), 0, static_cast<float_t>(height), -1., 1.);

			if (_effect->has_parameter("image"))
				_effect->get_parameter("image")->set_texture(_tex_source);
			if (_effect->has_parameter("pLift"))
				_effect->get_parameter("pLift")->set_float4(_lift);
			if (_effect->has_parameter("pGamma"))
				_effect->get_parameter("pGamma")->set_float4(_gamma);
			if (_effect->has_parameter("pGain"))
				_effect->get_parameter("pGain")->set_float4(_gain);
			if (_effect->has_parameter("pOffset"))
				_effect->get_parameter("pOffset")->set_float4(_offset);
			if (_effect->has_parameter("pTintLow"))
				_effect->get_parameter("pTintLow")->set_float3(_tint_low);
			if (_effect->has_parameter("pTintMid"))
				_effect->get_parameter("pTintMid")->set_float3(_tint_mid);
			if (_effect->has_parameter("pTintHig"))
				_effect->get_parameter("pTintHig")->set_float3(_tint_hig);
			if (_effect->has_parameter("pCorrection"))
				_effect->get_parameter("pCorrection")->set_float4(_correction);

			while (gs_effect_loop(_effect->get_object(), "Draw")) {
				gs_draw_sprite(nullptr, 0, width, height);
			}

			gs_blend_state_pop();
		}

		_tex_grade      = _rt_grade->get_texture();
		_source_updated = true;
	}

	// Render final result.
	{
		auto shader = obs_get_base_effect(OBS_EFFECT_DEFAULT);
		gs_enable_depth_test(false);
		while (gs_effect_loop(shader, "Draw")) {
			gs_effect_set_texture(gs_effect_get_param_by_name(shader, "image"),
								  _tex_grade ? _tex_grade->get_object() : nullptr);
			gs_draw_sprite(nullptr, 0, width, height);
		}
	}
}
