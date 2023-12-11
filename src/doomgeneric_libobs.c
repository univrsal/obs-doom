#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include <stdio.h>
#include <unistd.h>
#include <util/platform.h>
#include <stdbool.h>
#include <obs-module.h>

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

struct doom_source {
	obs_source_t *src;
	gs_texture_t *texture;
};

struct doom_source *g_src = NULL;

static unsigned char convertToDoomKey(unsigned int key)
{
	switch (key) {
	case OBS_KEY_RETURN:
	case OBS_KEY_ENTER:
		key = KEY_ENTER;
		break;
	case OBS_KEY_ESCAPE:
		key = KEY_ESCAPE;
		break;
	case OBS_KEY_LEFT:
		key = KEY_LEFTARROW;
		break;
	case OBS_KEY_RIGHT:
		key = KEY_RIGHTARROW;
		break;
	case OBS_KEY_UP:
		key = KEY_UPARROW;
		break;
	case OBS_KEY_DOWN:
		key = KEY_DOWNARROW;
		break;
	case OBS_KEY_CONTROL:
		key = KEY_FIRE;
		break;
	case OBS_KEY_SPACE:
		key = KEY_USE;
		break;
	case OBS_KEY_SHIFT:
		key = KEY_RSHIFT;
		break;
	case OBS_KEY_ALT:
		key = KEY_LALT;
		break;
	case OBS_KEY_F2:
		key = KEY_F2;
		break;
	case OBS_KEY_F3:
		key = KEY_F3;
		break;
	case OBS_KEY_F4:
		key = KEY_F4;
		break;
	case OBS_KEY_F5:
		key = KEY_F5;
		break;
	case OBS_KEY_F6:
		key = KEY_F6;
		break;
	case OBS_KEY_F7:
		key = KEY_F7;
		break;
	case OBS_KEY_F8:
		key = KEY_F8;
		break;
	case OBS_KEY_F9:
		key = KEY_F9;
		break;
	case OBS_KEY_F10:
		key = KEY_F10;
		break;
	case OBS_KEY_F11:
		key = KEY_F11;
		break;
	case OBS_KEY_EQUAL:
	case OBS_KEY_PLUS:
		key = KEY_EQUALS;
		break;
	case OBS_KEY_MINUS:
		key = KEY_MINUS;
		break;
	default:
		// key = tolower(key);
		break;
	}

	return key;
}

static void addKeyToQueue(int pressed, unsigned int keyCode)
{
	unsigned char key = convertToDoomKey(keyCode);

	unsigned short keyData = (pressed << 8) | key;

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

uint32_t buffer[DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4];

void DG_DrawFrame()
{
	memcpy(buffer, DG_ScreenBuffer,
	       DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
}

static float sleep_time = 0;
void DG_SleepMs(uint32_t ms)
{
	sleep_time += ms / 1000.f;
}

uint32_t DG_GetTicksMs()
{
	static uint64_t start = 0;
	if (start == 0)
		start = os_gettime_ns();
	return (os_gettime_ns() - start) / 1000000;
}

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
		//key queue is empty
		return 0;
	} else {
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = keyData >> 8;
		*doomKey = keyData & 0xFF;

		return 1;
	}

	return 0;
}

void DG_SetWindowTitle(const char *title)
{
	UNUSED_PARAMETER(title);
}

static uint32_t doom_source_getwidth(void *data)
{
	UNUSED_PARAMETER(data);
	return DOOMGENERIC_RESX;
}

static uint32_t doom_source_getheight(void *data)
{
	UNUSED_PARAMETER(data);
	return DOOMGENERIC_RESY;
}

static void doom_source_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct doom_source *context = data;

	uint8_t *ptr;
	uint32_t linesize;
	if (gs_texture_map(g_src->texture, &ptr, &linesize)) {
		memcpy(ptr, buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
		gs_texture_unmap(g_src->texture);
	}

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	gs_eparam_t *const param = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture_srgb(param, context->texture);

	gs_draw_sprite(context->texture, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY);

	gs_blend_state_pop();

	gs_enable_framebuffer_srgb(previous);
}

static void *doom_source_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);
	struct doom_source *context = bzalloc(sizeof(struct doom_source));
	context->src = source;
	obs_enter_graphics();
	context->texture = gs_texture_create(DOOMGENERIC_RESX, DOOMGENERIC_RESY, GS_BGRX, 1, NULL, GS_DYNAMIC);
	obs_leave_graphics();
	doomgeneric_Create(context); //asdf
	if (g_src == NULL) {
		g_src = context;
	}
	return context;
}

static void doom_source_destroy(void *unused)
{
	UNUSED_PARAMETER(unused);
}

static const char *doom_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "DOOM";
}

static obs_properties_t *doom_source_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return NULL;
}

static void doom_source_key_event(void *data, const struct obs_key_event *event,
				  bool key_up)
{
	UNUSED_PARAMETER(data);
	obs_key_t k = obs_key_from_virtual_key(event->native_vkey);
	addKeyToQueue(!key_up, convertToDoomKey(k));
}

static void doom_source_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(data);
	if (sleep_time > 0) {
		sleep_time = fminf(sleep_time - seconds, 0);
	} else {
		doomgeneric_Tick();
	}
}

struct obs_source_info doom_source_info_v3 = {
	.id = "doom_source",
	.version = 3,
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB |
			OBS_SOURCE_INTERACTION,
	.create = doom_source_create,
	.destroy = doom_source_destroy,
	//.update = doom_source_update,
	.get_name = doom_source_get_name,
	.get_width = doom_source_getwidth,
	.get_height = doom_source_getheight,
	.video_render = doom_source_render,
	.video_tick = doom_source_tick,
	.get_properties = doom_source_properties,
	.key_click = doom_source_key_event,
	.icon_type = OBS_ICON_TYPE_GAME_CAPTURE,
};
