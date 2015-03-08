#include "minisphere.h"
#include "api.h"
#include "image.h"

#include "spriteset.h"

#pragma pack(push, 1)
struct rss_header {
	uint8_t signature[4];
	int16_t version;
	int16_t num_images;
	int16_t frame_width;
	int16_t frame_height;
	int16_t num_directions;
	int16_t base_x1;
	int16_t base_y1;
	int16_t base_x2;
	int16_t base_y2;
	uint8_t reserved[106];
};

struct v3_direction {
	int16_t num_frames;
	uint8_t reserved[6];
};

struct v3_frame {
	int16_t image_idx;
	int16_t delay;
	uint8_t reserved[4];
};
#pragma pack(pop)

static const spriteset_pose_t* find_sprite_pose (const spriteset_t* spriteset, const char* pose_name);
static char*                  _fread_string          (ALLEGRO_FILE* file);
static duk_ret_t             _js_LoadSpriteset      (duk_context* ctx);
static duk_ret_t             _js_Spriteset_finalize (duk_context* ctx);

spriteset_t*
clone_spriteset(const spriteset_t* spriteset)
{
	return NULL;
}

spriteset_t*
load_spriteset(const char* path)
{
	struct v3_direction direction;
	struct v3_frame     frame;
	ALLEGRO_FILE*       file = NULL;
	struct rss_header   rss;
	spriteset_t*        spriteset = NULL;
	int                 i, j;

	if ((spriteset = calloc(1, sizeof(spriteset_t))) == NULL) goto on_error;
	if ((file = al_fopen(path, "rb")) == NULL) goto on_error;
	if (al_fread(file, &rss, sizeof(struct rss_header)) != sizeof(struct rss_header))
		goto on_error;
	if (memcmp(rss.signature, ".rss", 4) != 0) goto on_error;
	spriteset->base.x1 = rss.base_x1;
	spriteset->base.y1 = rss.base_y1;
	spriteset->base.x2 = rss.base_x2;
	spriteset->base.y2 = rss.base_y2;
	switch (rss.version) {
	case 1:
		spriteset->num_images = rss.num_images;
		spriteset->num_poses = 8;
		spriteset->poses = calloc(spriteset->num_poses, sizeof(spriteset_pose_t));
		spriteset->poses[0].name = strdup("north");
		spriteset->poses[1].name = strdup("northeast");
		spriteset->poses[2].name = strdup("east");
		spriteset->poses[3].name = strdup("southeast");
		spriteset->poses[4].name = strdup("south");
		spriteset->poses[5].name = strdup("southwest");
		spriteset->poses[6].name = strdup("west");
		spriteset->poses[7].name = strdup("northwest");
		if ((spriteset->images = calloc(spriteset->num_images, sizeof(image_t*))) == NULL)
			goto on_error;
		for (i = 0; i < spriteset->num_images; ++i) {
			if ((spriteset->images[i] = read_image(file, rss.frame_width, rss.frame_height)) == NULL)
				goto on_error;
		}
		for (i = 0; i < spriteset->num_poses; ++i) {
			if ((spriteset->poses[i].frames = calloc(8, sizeof(spriteset_frame_t))) == NULL)
				goto on_error;
			for (j = 0; j < 8; ++j) {
				spriteset->poses[i].frames[j].image_idx = j + i * 8;
				spriteset->poses[i].frames[j].delay = 8;
			}
		}
		break;
	case 2:
		goto on_error;
		break;
	case 3:
		spriteset->num_images = rss.num_images;
		spriteset->num_poses = rss.num_directions;
		if ((spriteset->images = calloc(spriteset->num_images, sizeof(image_t*))) == NULL)
			goto on_error;
		if ((spriteset->poses = calloc(spriteset->num_poses, sizeof(spriteset_pose_t))) == NULL)
			goto on_error;
		for (i = 0; i < rss.num_images; ++i) {
			if ((spriteset->images[i] = read_image(file, rss.frame_width, rss.frame_height)) == NULL)
				goto on_error;
		}
		for (i = 0; i < rss.num_directions; ++i) {
			if (al_fread(file, &direction, sizeof(struct v3_direction)) != sizeof(struct v3_direction))
				goto on_error;
			if ((spriteset->poses[i].name = _fread_string(file)) == NULL) goto on_error;
			spriteset->poses[i].num_frames = direction.num_frames;
			if ((spriteset->poses[i].frames = calloc(direction.num_frames, sizeof(spriteset_frame_t))) == NULL)
				goto on_error;
			for (j = 0; j < spriteset->poses[i].num_frames; ++j) {
				if (al_fread(file, &frame, sizeof(struct v3_frame)) != sizeof(struct v3_frame))
					goto on_error;
				spriteset->poses[i].frames[j].image_idx = frame.image_idx;
				spriteset->poses[i].frames[j].delay = frame.delay;
			}
		}
		break;
	default:  // invalid version number
		goto on_error;
	}
	al_fclose(file);
	spriteset->c_refs = 1;
	return spriteset;

on_error:
	if (file != NULL) al_fclose(file);
	if (spriteset != NULL) {
		if (spriteset->poses != NULL) {
			for (i = 0; i < spriteset->num_poses; ++i) {
				free(spriteset->poses[i].frames);
				free(spriteset->poses[i].name);
			}
			free(spriteset->poses);
		}
		free(spriteset);
	}
	return NULL;
}

spriteset_t*
ref_spriteset(spriteset_t* spriteset)
{
	++spriteset->c_refs;
	return spriteset;
}

void
free_spriteset(spriteset_t* spriteset)
{
	int i;
	
	if (spriteset == NULL || --spriteset->c_refs > 0)
		return;
	for (i = 0; i < spriteset->num_images; ++i) {
		free_image(spriteset->images[i]);
	}
	free(spriteset->images);
	for (i = 0; i < spriteset->num_poses; ++i) {
		free(spriteset->poses[i].frames);
		free(spriteset->poses[i].name);
	}
	free(spriteset->poses);
	free(spriteset);
}

rect_t
get_sprite_base(const spriteset_t* spriteset)
{
	return spriteset->base;
}

int
get_sprite_frame_delay(const spriteset_t* spriteset, const char* pose_name, int frame_index)
{
	const spriteset_pose_t* pose;
	
	if ((pose = find_sprite_pose(spriteset, pose_name)) == NULL)
		return 0;
	frame_index %= pose->num_frames;
	return pose->frames[frame_index].delay;
}

void
draw_sprite(const spriteset_t* spriteset, const char* pose_name, float x, float y, int frame_index)
{
	int                      image_index;
	const spriteset_pose_t*  pose;
	
	if ((pose = find_sprite_pose(spriteset, pose_name)) == NULL)
		return;
	frame_index = frame_index % pose->num_frames;
	image_index = pose->frames[frame_index].image_idx;
	x -= (spriteset->base.x1 + spriteset->base.x2) / 2;
	y -= (spriteset->base.y1 + spriteset->base.y2) / 2;
	al_draw_bitmap(get_image_bitmap(spriteset->images[image_index]), x, y, 0x0);
}

void
init_spriteset_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "LoadSpriteset", _js_LoadSpriteset);
}

void
duk_push_spriteset(duk_context* ctx, spriteset_t* spriteset)
{
	int i, j;

	ref_spriteset(spriteset);

	duk_push_object(ctx);
	duk_push_pointer(ctx, "spriteset"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_pointer(ctx, spriteset); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_c_function(ctx, &_js_Spriteset_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);

	// Spriteset:base
	duk_push_object(ctx);
	duk_push_int(ctx, spriteset->base.x1); duk_put_prop_string(ctx, -2, "x1");
	duk_push_int(ctx, spriteset->base.y1); duk_put_prop_string(ctx, -2, "y1");
	duk_push_int(ctx, spriteset->base.x2); duk_put_prop_string(ctx, -2, "x2");
	duk_push_int(ctx, spriteset->base.y2); duk_put_prop_string(ctx, -2, "y2");
	duk_put_prop_string(ctx, -2, "base");

	// Spriteset:images
	duk_push_array(ctx);
	for (i = 0; i < spriteset->num_images; ++i) {
		duk_push_sphere_image(ctx, spriteset->images[i]);
		duk_put_prop_index(ctx, -2, i);
	}
	duk_put_prop_string(ctx, -2, "images");

	// Spriteset:directions
	duk_push_array(ctx);
	for (i = 0; i < spriteset->num_poses; ++i) {
		duk_push_object(ctx);
		duk_push_string(ctx, spriteset->poses[i].name); duk_put_prop_string(ctx, -2, "name");
		duk_push_array(ctx);
		for (j = 0; j < spriteset->poses[i].num_frames; ++j) {
			duk_push_object(ctx);
			duk_push_int(ctx, spriteset->poses[i].frames[j].image_idx); duk_put_prop_string(ctx, -2, "index");
			duk_push_int(ctx, spriteset->poses[i].frames[j].delay); duk_put_prop_string(ctx, -2, "delay");
			duk_put_prop_index(ctx, -2, j);
		}
		duk_put_prop_string(ctx, -2, "frames");
		duk_put_prop_index(ctx, -2, i);
	}
	duk_put_prop_string(ctx, -2, "directions");
}

spriteset_t*
duk_require_spriteset(duk_context* ctx, duk_idx_t index)
{
	spriteset_t* spriteset;
	const char*  type;

	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	if (!duk_get_prop_string(ctx, index, "\xFF" "sphere_type"))
		goto on_error;
	type = duk_get_string(ctx, -1); duk_pop(ctx);
	if (strcmp(type, "spriteset") != 0) goto on_error;
	duk_get_prop_string(ctx, index, "\xFF" "ptr");
	spriteset = duk_get_pointer(ctx, -1); duk_pop(ctx);
	return spriteset;

on_error:
	duk_error(ctx, DUK_ERR_TYPE_ERROR, "Not a Sphere spriteset");
}

static const spriteset_pose_t*
find_sprite_pose(const spriteset_t* spriteset, const char* pose_name)
{
	const char*             alt_name;
	const char*             name_to_find;
	const spriteset_pose_t* pose = NULL;
	int                     i, j = 0;

	alt_name = strcasecmp(pose_name, "northeast") == 0 ? "north"
		: strcasecmp(pose_name, "southeast") == 0 ? "south"
		: strcasecmp(pose_name, "southwest") == 0 ? "south"
		: strcasecmp(pose_name, "northwest") == 0 ? "north"
		: "";
	name_to_find = pose_name;
	do {
		for (i = 0; i < spriteset->num_poses; ++i) {
			if (strcasecmp(name_to_find, spriteset->poses[i].name) == 0) {
				pose = &spriteset->poses[i];
				break;
			}
		}
		name_to_find = alt_name;
	} while (pose == NULL && name_to_find != alt_name);
	return pose != NULL ? pose : &spriteset->poses[0];
}

static char*
_fread_string(ALLEGRO_FILE* file)
{
	uint16_t length;
	char*    string = NULL;

	if (al_fread(file, &length, 2) != 2) goto on_error;
	if ((string = calloc(length, sizeof(char))) == NULL) goto on_error;
	if (al_fread(file, string, length) != length) goto on_error;
	return string;

on_error:
	free(string);
	return NULL;
}

static duk_ret_t
_js_LoadSpriteset(duk_context* ctx)
{
	const char*  filename  = NULL;
	char*        path      = NULL;
	spriteset_t* spriteset = NULL;

	filename = duk_require_string(ctx, 0);
	path = get_asset_path(filename, "spritesets", false);
	if ((spriteset = load_spriteset(path)) == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "LoadSpriteset(): Failed to load spriteset file '%s'", filename);
	free(path);
	duk_push_spriteset(ctx, spriteset);
	free_spriteset(spriteset);
	return 1;
}

static duk_ret_t
_js_Spriteset_finalize(duk_context* ctx)
{
	spriteset_t* spriteset;
	
	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); spriteset = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_spriteset(spriteset);
	return 0;
}
