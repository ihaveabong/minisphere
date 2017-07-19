#include "minisphere.h"
#include "galileo.h"

#include "color.h"
#include "vector.h"

static void free_cached_uniform (model_t* model, const char* name);
static void render_shape        (shape_t* shape);

enum uniform_type
{
	UNIFORM_INT,
	UNIFORM_INT_ARR,
	UNIFORM_INT_VEC,
	UNIFORM_FLOAT,
	UNIFORM_FLOAT_ARR,
	UNIFORM_FLOAT_VEC,
	UNIFORM_MATRIX,
};
struct uniform
{
	char              name[256];
	enum uniform_type type;
	int               num_values;
	union {
		int*              int_list;
		int               int_value;
		int               int_vec[4];
		float*            float_list;
		float             float_value;
		float             float_vec[4];
		ALLEGRO_TRANSFORM mat_value;
	};
};

struct model
{
	unsigned int refcount;
	unsigned int id;
	shader_t*    shader;
	vector_t*    shapes;
	transform_t* transform;
	vector_t*    uniforms;
};

struct shader
{
	unsigned int    id;
	unsigned int    refcount;
	ALLEGRO_SHADER* program;
};

struct shape
{
	unsigned int           refcount;
	unsigned int           id;
	ALLEGRO_INDEX_BUFFER*  ibo;
	vector_t*              indices;
	image_t*               texture;
	shape_type_t           type;
	vector_t*              vertices;
	ALLEGRO_VERTEX_BUFFER* vbo;
};

static shader_t*    s_def_shader = NULL;
static shader_t*    s_last_shader = NULL;
static unsigned int s_next_group_id = 0;
static unsigned int s_next_shader_id = 1;
static unsigned int s_next_shape_id = 0;

void
galileo_init(void)
{
	console_log(1, "initializing Galileo subsystem");
}

void
galileo_uninit(void)
{
	console_log(1, "shutting down Galileo subsystem");
	shader_free(s_def_shader);
}

shader_t*
galileo_shader(void)
{
	const char* fs_filename;
	char*       fs_pathname;
	const char* vs_filename;
	char*       vs_pathname;

	if (s_def_shader == NULL) {
		console_log(3, "compiling Galileo default shaders");
		vs_filename = kev_read_string(g_sys_conf, "GalileoVertShader", "shaders/galileo.vert.glsl");
		fs_filename = kev_read_string(g_sys_conf, "GalileoFragShader", "shaders/galileo.frag.glsl");
		vs_pathname = strdup(system_path(vs_filename));
		fs_pathname = strdup(system_path(fs_filename));
		s_def_shader = shader_new(vs_pathname, fs_pathname);
		free(vs_pathname);
		free(fs_pathname);
	}
	return s_def_shader;
}

void
galileo_reset(void)
{
	// note: this resets internal render state to the default settings for Sphere v1.
	//       Sv1 APIs should call this before drawing anything; doing so avoids Galileo
	//       haivng to undo its own state changes all the time, keeping things snappy.

	image_render_to(screen_backbuffer(g_screen), NULL);
	shader_use(NULL, false);
}

model_t*
model_new(shader_t* shader)
{
	model_t* model;

	console_log(4, "creating new model #%u", s_next_group_id);

	model = calloc(1, sizeof(model_t));
	model->shapes = vector_new(sizeof(shape_t*));
	model->transform = transform_new();
	model->shader = shader_ref(shader);
	model->uniforms = vector_new(sizeof(struct uniform));

	model->id = s_next_group_id++;
	return model_ref(model);
}

model_t*
model_ref(model_t* it)
{
	if (it != NULL)
		++it->refcount;
	return it;
}

void
model_free(model_t* it)
{
	shape_t** i_shape;

	iter_t iter;

	if (it == NULL || --it->refcount > 0)
		return;

	console_log(4, "disposing model #%u no longer in use", it->id);

	iter = vector_enum(it->shapes);
	while (i_shape = vector_next(&iter))
		shape_free(*i_shape);
	vector_free(it->shapes);
	shader_free(it->shader);
	transform_free(it->transform);
	vector_free(it->uniforms);
	free(it);
}

shader_t*
model_get_shader(const model_t* it)
{
	return it->shader;
}

transform_t*
model_get_transform(const model_t* it)
{
	return it->transform;
}

void
model_set_shader(model_t* it, shader_t* shader)
{
	shader_t* old_shader;

	old_shader = it->shader;
	it->shader = shader_ref(shader);
	shader_free(old_shader);
}

void
model_set_transform(model_t* it, transform_t* transform)
{
	transform_t* old_transform;

	old_transform = it->transform;
	it->transform = transform_ref(transform);
	transform_free(old_transform);
}

bool
model_add_shape(model_t* it, shape_t* shape)
{
	console_log(4, "adding shape #%u to model #%u", shape->id, it->id);

	shape = shape_ref(shape);
	vector_push(it->shapes, &shape);
	return true;
}

void
model_draw(const model_t* it, image_t* surface)
{
	iter_t iter;
	struct uniform* p;

	image_render_to(surface, it->transform);

	shader_use(it->shader != NULL ? it->shader : galileo_shader(), false);
	iter = vector_enum(it->uniforms);
	while (p = vector_next(&iter)) {
		switch (p->type) {
		case UNIFORM_FLOAT:
			al_set_shader_float(p->name, p->float_value);
			break;
		case UNIFORM_FLOAT_ARR:
			al_set_shader_float_vector(p->name, 1, p->float_list, p->num_values);
			break;
		case UNIFORM_FLOAT_VEC:
			al_set_shader_float_vector(p->name, p->num_values, p->float_vec, 1);
			break;
		case UNIFORM_INT:
			al_set_shader_int(p->name, p->int_value);
			break;
		case UNIFORM_INT_ARR:
			al_set_shader_int_vector(p->name, 1, p->int_list, p->num_values);
			break;
		case UNIFORM_INT_VEC:
			al_set_shader_int_vector(p->name, p->num_values, p->int_vec, 1);
			break;
		case UNIFORM_MATRIX:
			al_set_shader_matrix(p->name, &p->mat_value);
			break;
		}
	}

	iter = vector_enum(it->shapes);
	while (vector_next(&iter))
		render_shape(*(shape_t**)iter.ptr);
}

void
model_put_float(model_t* it, const char* name, float value)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_FLOAT;
	unif.float_value = value;
	vector_push(it->uniforms, &unif);
}

void
model_put_float_array(model_t* it, const char* name, float values[], int size)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_FLOAT_ARR;
	unif.float_list = malloc(size * sizeof(float));
	unif.num_values = size;
	memcpy(unif.float_list, values, size * sizeof(float));
	vector_push(it->uniforms, &unif);
}

void
model_put_float_vector(model_t* it, const char* name, float values[], int size)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_FLOAT_VEC;
	unif.num_values = size;
	memcpy(unif.float_vec, values, sizeof(float) * size);
	vector_push(it->uniforms, &unif);
}

void
model_put_int(model_t* it, const char* name, int value)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_INT;
	unif.int_value = value;
	vector_push(it->uniforms, &unif);
}

void
model_put_int_array(model_t* it, const char* name, int values[], int size)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_INT_ARR;
	unif.int_list = malloc(size * sizeof(int));
	unif.num_values = size;
	memcpy(unif.int_list, values, size * sizeof(int));
	vector_push(it->uniforms, &unif);
}

void
model_put_int_vector(model_t* it, const char* name, int values[], int size)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_INT_VEC;
	unif.num_values = size;
	memcpy(unif.int_vec, values, sizeof(int) * size);
	vector_push(it->uniforms, &unif);
}

void
model_put_matrix(model_t* it, const char* name, const transform_t* matrix)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_MATRIX;
	al_copy_transform(&unif.mat_value, transform_matrix(matrix));
	vector_push(it->uniforms, &unif);
}

shader_t*
shader_new(const char* vs_filename, const char* fs_filename)
{
	char*      fs_source = NULL;
	char*      vs_source = NULL;
	shader_t*  shader;

	shader = calloc(1, sizeof(shader_t));

	console_log(2, "compiling new shader program #%u", s_next_shader_id);

	if (!(vs_source = sfs_fslurp(g_fs, vs_filename, NULL, NULL)))
		goto on_error;
	if (!(fs_source = sfs_fslurp(g_fs, fs_filename, NULL, NULL)))
		goto on_error;
	if (!(shader->program = al_create_shader(ALLEGRO_SHADER_GLSL)))
		goto on_error;
	if (!al_attach_shader_source(shader->program, ALLEGRO_VERTEX_SHADER, vs_source)) {
		fprintf(stderr, "\nvertex shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_attach_shader_source(shader->program, ALLEGRO_PIXEL_SHADER, fs_source)) {
		fprintf(stderr, "\nfragment shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_build_shader(shader->program)) {
		fprintf(stderr, "\nerror building shader program:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	free(vs_source);
	free(fs_source);

	shader->id = s_next_shader_id++;
	return shader_ref(shader);

on_error:
	free(vs_source);
	free(fs_source);
	if (shader->program != NULL)
		al_destroy_shader(shader->program);
	free(shader);
	return NULL;
}

shader_t*
shader_ref(shader_t* shader)
{
	if (shader == NULL)
		return shader;
	++shader->refcount;
	return shader;
}

void
shader_free(shader_t* shader)
{
	if (shader == NULL || --shader->refcount > 0)
		return;

	console_log(3, "disposing shader program #%u no longer in use", shader->id);
	al_destroy_shader(shader->program);
	free(shader);
}

bool
shader_use(shader_t* shader, bool force_set)
{
	ALLEGRO_SHADER* al_shader;

	if (shader == s_last_shader && !force_set)
		return true;

	if (shader != NULL)
		console_log(4, "activating shader program #%u", shader->id);
	else
		console_log(4, "activating legacy shaders");

	al_shader = shader != NULL ? shader->program : NULL;
	if (!al_use_shader(al_shader))
		return false;
	s_last_shader = shader;
	return true;
}

shape_t*
shape_new(shape_type_t type, image_t* texture)
{
	shape_t*    shape;
	const char* type_name;

	type_name = type == SHAPE_POINTS ? "point list"
		: type == SHAPE_LINES ? "line list"
		: type == SHAPE_LINE_LOOP ? "line loop"
		: type == SHAPE_TRIANGLES ? "triangle list"
		: type == SHAPE_TRI_FAN ? "triangle fan"
		: type == SHAPE_TRI_STRIP ? "triangle strip"
		: "automatic";
	console_log(4, "creating shape #%u as %s", s_next_shape_id, type_name);

	shape = calloc(1, sizeof(shape_t));
	shape->texture = image_ref(texture);
	shape->type = type;
	shape->vertices = vector_new(sizeof(vertex_t));
	shape->indices = vector_new(sizeof(uint16_t));

	shape->id = s_next_shape_id++;
	return shape_ref(shape);
}

shape_t*
shape_ref(shape_t* it)
{
	if (it != NULL)
		++it->refcount;
	return it;
}

void
shape_free(shape_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	console_log(4, "disposing shape #%u no longer in use", it->id);
	image_free(it->texture);
	if (it->vbo != NULL)
		al_destroy_vertex_buffer(it->vbo);
	free(it->vertices);
	free(it);
}

float_rect_t
shape_bounds(const shape_t* it)
{
	float_rect_t bounds;
	vertex_t*    vertex;

	iter_t iter;

	if (vector_len(it->vertices) < 1)
		return new_float_rect(0.0, 0.0, 0.0, 0.0);
	vertex = vector_get(it->vertices, 0);
	bounds = new_float_rect(
		vertex->x, vertex->y,
		vertex->x, vertex->y);
	iter = vector_enum(it->vertices);
	while (vertex = vector_next(&iter)) {
		bounds.x1 = fmin(vertex->x, bounds.x1);
		bounds.y1 = fmin(vertex->y, bounds.y1);
		bounds.x2 = fmax(vertex->x, bounds.x2);
		bounds.y2 = fmax(vertex->y, bounds.y2);
	}
	return bounds;
}

image_t*
shape_texture(const shape_t* it)
{
	return it->texture;
}

void
shape_set_texture(shape_t* it, image_t* texture)
{
	image_t* old_texture;

	old_texture = it->texture;
	it->texture = image_ref(texture);
	image_free(old_texture);
	shape_upload(it);
}

bool
shape_add_index(shape_t* it, uint16_t index)
{
	return vector_push(it->indices, &index);
}

bool
shape_add_vertex(shape_t* it, vertex_t vertex)
{
	return vector_push(it->vertices, &vertex);
}

void
shape_draw(shape_t* it, image_t* surface, transform_t* transform)
{
	image_render_to(surface, transform);
	shader_use(galileo_shader(), false);
	render_shape(it);
}

void
shape_upload(shape_t* it)
{
	ALLEGRO_BITMAP* bitmap;
	uint16_t        index;
	int             num_indices;
	int             num_vertices;
	vertex_t*       vertex;
	uint16_t*       indices = NULL;
	ALLEGRO_VERTEX* vertices = NULL;

	iter_t iter;

	console_log(3, "uploading shape #%u vertices to GPU", it->id);
	bitmap = it->texture != NULL ? image_bitmap(it->texture) : NULL;

	// create vertex and index buffers for the shape
	if (it->vbo != NULL)
		al_destroy_vertex_buffer(it->vbo);
	if (it->ibo != NULL)
		al_destroy_index_buffer(it->ibo);
	num_vertices = vector_len(it->vertices);
	num_indices = vector_len(it->indices);
	if (it->vbo = al_create_vertex_buffer(NULL, NULL, num_vertices, ALLEGRO_PRIM_BUFFER_STATIC))
		vertices = al_lock_vertex_buffer(it->vbo, 0, num_vertices, ALLEGRO_LOCK_WRITEONLY);
	if (num_indices > 0 && (it->ibo = al_create_index_buffer(2, NULL, num_indices, ALLEGRO_PRIM_BUFFER_STATIC)))
		indices = al_lock_index_buffer(it->ibo, 0, num_vertices, ALLEGRO_LOCK_WRITEONLY);
	if (vertices == NULL) {
		console_log(3, "couldn't create VBO for shape #%u", it->id);
		return;
	}
	if (indices == NULL && num_indices > 0) {
		console_log(3, "couldn't create IBO for shape #%u", it->id);
		return;
	}

	// upload vertices into the VBO
	iter = vector_enum(it->vertices);
	while (vertex = vector_next(&iter)) {
		vertices[iter.index].x = vertex->x;
		vertices[iter.index].y = vertex->y;
		vertices[iter.index].z = vertex->z;
		vertices[iter.index].color = nativecolor(vertex->color);
		vertices[iter.index].u = vertex->u;
		vertices[iter.index].v = vertex->v;
	}

	// upload indices into the IBO.  note this is safe even if it doesn't
	// seem to be; if indices is NULL at this point, then num_indices is 0
	// by definition and the loop will not be entered.
	iter = vector_enum(it->indices);
	while (vector_next(&iter)) {
		index = *(uint16_t*)iter.ptr;
		indices[iter.index] = index;
	}

	al_unlock_vertex_buffer(it->vbo);
}

static void
free_cached_uniform(model_t* group, const char* name)
{
	iter_t iter;
	struct uniform* p;

	iter = vector_enum(group->uniforms);
	while (p = vector_next(&iter)) {
		if (strcmp(p->name, name) == 0)
			iter_remove(&iter);
	}
}

static void
render_shape(shape_t* shape)
{
	ALLEGRO_BITMAP* bitmap;
	int             draw_mode;
	int             num_indices;
	int             num_vertices;

	num_indices = vector_len(shape->indices);
	num_vertices = vector_len(shape->vertices);
	if (num_vertices == 0)
		return;

	if (shape->vbo == NULL || (shape->ibo == NULL && num_indices > 0))
		shape_upload(shape);
	if (shape->type == SHAPE_AUTO)
		draw_mode = num_vertices == 1 ? ALLEGRO_PRIM_POINT_LIST
			: num_vertices == 2 ? ALLEGRO_PRIM_LINE_LIST
			: ALLEGRO_PRIM_TRIANGLE_STRIP;
	else
		draw_mode = shape->type == SHAPE_LINES ? ALLEGRO_PRIM_LINE_LIST
			: shape->type == SHAPE_LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
			: shape->type == SHAPE_LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
			: shape->type == SHAPE_TRIANGLES ? ALLEGRO_PRIM_TRIANGLE_LIST
			: shape->type == SHAPE_TRI_STRIP ? ALLEGRO_PRIM_TRIANGLE_STRIP
			: shape->type == SHAPE_TRI_FAN ? ALLEGRO_PRIM_TRIANGLE_FAN
			: ALLEGRO_PRIM_POINT_LIST;

	bitmap = shape->texture != NULL ? image_bitmap(shape->texture) : NULL;
	if (shape->ibo != NULL)
		al_draw_indexed_buffer(shape->vbo, bitmap, shape->ibo, 0, num_indices, draw_mode);
	else
		al_draw_vertex_buffer(shape->vbo, bitmap, 0, num_vertices, draw_mode);
}
