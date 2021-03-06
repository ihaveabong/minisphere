#include "minisphere.h"
#include "sockets.h"

struct server
{
	unsigned int refcount;
	unsigned int id;
	size_t       buffer_size;
	int          max_backlog;
	int          num_backlog;
	dyad_Stream* stream4;
	dyad_Stream* stream6;
	dyad_Stream* *backlog;
};

struct socket
{
	unsigned int refcount;
	unsigned int id;
	size_t       buffer_size;
	uint8_t*     recv_buffer;
	size_t       recv_size;
	dyad_Stream* stream;
};

static void on_dyad_accept  (dyad_Event* e);
static void on_dyad_receive (dyad_Event* e);

static unsigned int s_next_client_id = 0;
static unsigned int s_next_server_id = 0;
static unsigned int s_num_refs       = 0;

bool
sockets_init(void)
{
	if (++s_num_refs > 1)
		return true;

	console_log(1, "initializing sockets subsystem");
	console_log(2, "    Dyad.c %s", dyad_getVersion());
	dyad_init();
	dyad_setUpdateTimeout(0.0);
	return true;
}

void
sockets_uninit(void)
{
	if (--s_num_refs > 0)
		return;

	console_log(1, "shutting down sockets subsystem");
	dyad_shutdown();
}

void
sockets_update(void)
{
	dyad_update();
}

socket_t*
socket_new(size_t buffer_size)
{
	socket_t* socket;

	console_log(2, "creating TCP socket #%u", s_next_client_id);

	socket = calloc(1, sizeof(socket_t));
	socket->buffer_size = buffer_size;
	if (!(socket->recv_buffer = malloc(buffer_size)))
		goto on_error;
	socket->id = s_next_client_id++;
	return socket_ref(socket);

on_error:
	console_log(2, "couldn't create TCP socket #%u", s_next_client_id++);
	free(socket);
	return NULL;
}

socket_t*
socket_ref(socket_t* it)
{
	++it->refcount;
	return it;
}

void
socket_free(socket_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	console_log(3, "disposing TCP socket #%u no longer in use", it->id);
	if (it->stream != NULL)
		dyad_end(it->stream);
	free(it);
}

bool
socket_connected(const socket_t* it)
{
	int state;

	if (it->stream == NULL)
		return false;
	state = dyad_getState(it->stream);
	return state == DYAD_STATE_CONNECTED
		|| state == DYAD_STATE_CLOSING;
}

bool
socket_connect(socket_t* it, const char* hostname, int port)
{
	socket_close(it);

	if (!(it->stream = dyad_newStream()))
		goto on_error;
	dyad_setNoDelay(it->stream, true);
	dyad_addListener(it->stream, DYAD_EVENT_DATA, on_dyad_receive, it);
	if (dyad_connect(it->stream, hostname, port) == -1)
		goto on_error;
	return true;

on_error:
	console_log(2, "couldn't connect TCP socket #%u to %s:%d", it->id, hostname, port);
	return false;
}

const char*
socket_hostname(const socket_t* it)
{
	return dyad_getAddress(it->stream);
}

int
socket_port(const socket_t* it)
{
	return dyad_getPort(it->stream);
}

void
socket_close(socket_t* it)
{
	if (it->stream == NULL)
		return;
	console_log(2, "closing connection on TCP socket #%u", it->id);
	dyad_end(it->stream);
	it->stream = NULL;
	it->recv_size = 0;
}

size_t
socket_peek(const socket_t* it)
{
	return it->recv_size;
}

size_t
socket_read(socket_t* it, void* buffer, size_t num_bytes)
{
	num_bytes = num_bytes <= it->recv_size ? num_bytes : it->recv_size;
	console_log(4, "reading %zd bytes from TCP socket #%u", num_bytes, it->id);
	memcpy(buffer, it->recv_buffer, num_bytes);
	memmove(it->recv_buffer, it->recv_buffer + num_bytes, it->recv_size - num_bytes);
	it->recv_size -= num_bytes;
	return num_bytes;
}

void
socket_write(socket_t* it, const void* data, size_t num_bytes)
{
	console_log(4, "writing %zd bytes to TCP socket #%u", num_bytes, it->id);
	dyad_write(it->stream, data, (int)num_bytes);
}

server_t*
server_new(const char* hostname, int port, size_t buffer_size, int max_backlog)
{
	server_t* server = NULL;

	console_log(2, "creating server #%u on %s:%d", s_next_server_id, hostname, port);
	if (max_backlog > 0)
		console_log(3, "    backlog size: %d", max_backlog);

	server = calloc(1, sizeof(server_t));
	server->buffer_size = buffer_size;
	server->backlog = malloc(max_backlog * sizeof(dyad_Stream*));
	server->max_backlog = max_backlog;
	if (!(server->stream4 = dyad_newStream()))
		goto on_error;
	dyad_setNoDelay(server->stream4, true);
	dyad_addListener(server->stream4, DYAD_EVENT_ACCEPT, on_dyad_accept, server);
	if (hostname == NULL) {
		if (!(server->stream6 = dyad_newStream()))
			goto on_error;
		dyad_setNoDelay(server->stream6, true);
		dyad_addListener(server->stream6, DYAD_EVENT_ACCEPT, on_dyad_accept, server);
		if (dyad_listenEx(server->stream4, "0.0.0.0", port, max_backlog) == -1)
			goto on_error;
		if (dyad_listenEx(server->stream6, "::", port, max_backlog) == -1)
			goto on_error;
	}
	else {
		if (dyad_listenEx(server->stream4, hostname, port, max_backlog) == -1)
			goto on_error;
	}

	server->id = s_next_server_id++;
	return server_ref(server);

on_error:
	console_log(2, "failed to create server #%u", s_next_server_id++);
	if (server != NULL) {
		free(server->backlog);
		if (server->stream4 != NULL)
			dyad_close(server->stream4);
		free(server);
	}
	return NULL;
}

server_t*
server_ref(server_t* it)
{
	++it->refcount;
	return it;
}

void
server_free(server_t* it)
{
	int i;
	
	if (it == NULL || --it->refcount > 0)
		return;
	console_log(3, "disposing server #%u no longer in use", it->id);
	for (i = 0; i < it->num_backlog; ++i)
		dyad_end(it->backlog[i]);
	dyad_end(it->stream4);
	if (it->stream6 != NULL)
		dyad_end(it->stream6);
	free(it);
}

socket_t*
server_accept(server_t* it)
{
	socket_t* client;

	int i;

	if (it->num_backlog == 0)
		return NULL;

	console_log(2, "accepting new TCP socket #%u from server #%u",
		s_next_client_id, it->id);
	console_log(2, "    remote address: %s:%d",
		dyad_getAddress(it->backlog[0]),
		dyad_getPort(it->backlog[0]));

	// construct a socket object for the new connection
	client = calloc(1, sizeof(socket_t));
	client->buffer_size = it->buffer_size;
	client->recv_buffer = malloc(it->buffer_size);
	client->stream = it->backlog[0];
	dyad_addListener(client->stream, DYAD_EVENT_DATA, on_dyad_receive, client);

	// we accepted the connection, remove it from the backlog
	--it->num_backlog;
	for (i = 0; i < it->num_backlog; ++i)
		it->backlog[i] = it->backlog[i + 1];

	client->id = s_next_client_id++;
	return socket_ref(client);
}

static void
on_dyad_accept(dyad_Event* e)
{
	int       new_backlog_len;
	server_t* server = e->udata;

	new_backlog_len = server->num_backlog + 1;
	if (new_backlog_len <= server->max_backlog) {
		console_log(4, "taking connection from %s:%d on server #%u",
			dyad_getAddress(e->remote), dyad_getPort(e->remote), server->id);
		server->backlog[server->num_backlog++] = e->remote;
	}
	else {
		console_log(4, "backlog full on server #%u, refusing %s:%d", server->id,
			dyad_getAddress(e->remote), dyad_getPort(e->remote), server->id);
		dyad_close(e->remote);
	}
}

static void
on_dyad_receive(dyad_Event* e)
{
	size_t    new_size;
	socket_t* client = e->udata;

	// buffer any data received until read() is called
	new_size = client->recv_size + e->size;
	if (new_size > client->buffer_size) {
		client->buffer_size = new_size * 2;
		client->recv_buffer = realloc(client->recv_buffer, client->buffer_size);
	}
	memcpy(client->recv_buffer + client->recv_size, e->data, e->size);
	client->recv_size += e->size;
}
