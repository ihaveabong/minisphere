#ifndef SSJ__REMOTE_H__INCLUDED
#define SSJ__REMOTE_H__INCLUDED

typedef struct remote  remote_t;
typedef struct dvalue  dvalue_t;
typedef struct message message_t;

typedef
enum dvalue_tag
{
	DVALUE_TAG_EOM = 0x00,
	DVALUE_TAG_REQ = 0x01,
	DVALUE_TAG_REP = 0x02,
	DVALUE_TAG_ERR = 0x03,
	DVALUE_TAG_NFY = 0x04,
	DVALUE_TAG_INT = 0x10,
	DVALUE_TAG_STRING = 0x11,
	DVALUE_TAG_STR16 = 0x12,
	DVALUE_TAG_BUFFER = 0x13,
	DVALUE_TAG_BUF16 = 0x14,
	DVALUE_TAG_UNUSED = 0x15,
	DVALUE_TAG_UNDEF = 0x16,
	DVALUE_TAG_NULL = 0x17,
	DVALUE_TAG_TRUE = 0x18,
	DVALUE_TAG_FALSE = 0x19,
	DVALUE_TAG_FLOAT = 0x1A,
	DVALUE_TAG_OBJ = 0x1B,
	DVALUE_TAG_PTR = 0x1C,
	DVALUE_TAG_LIGHTFUNC = 0x1D,
	DVALUE_TAG_HEAPPTR = 0x1E,
} dvalue_tag_t;

typedef
enum atom_type
{
	ATOM_UNDEFINED,
	ATOM_OBJECT,
	ATOM_FLOAT,
	ATOM_INT,
	ATOM_STRING,
} atom_type_t;

typedef
enum msg_class
{
	MSG_CLASS_UNKNOWN,
	MSG_CLASS_REQ,
	MSG_CLASS_REP,
	MSG_CLASS_ERR,
	MSG_CLASS_NFY,
} msg_class_t;

void          initialize_client (void);
void          shutdown_client   (void);
remote_t*     connect_remote    (const char* hostname, int port);
void          close_remote      (remote_t* remote);
message_t*    msg_new           (msg_class_t msg_class);
message_t*    msg_receive       (remote_t* remote);
void          msg_free          (message_t* msg);
msg_class_t   msg_get_class     (const message_t* message);
size_t        msg_len           (const message_t* message);
void          msg_send          (remote_t* remote, message_t* msg);
void          msg_add_float     (message_t* msg, double value);
void          msg_add_int       (message_t* msg, int32_t value);
void          msg_add_string    (message_t* msg, const char* value);
atom_type_t   msg_atom_type     (const message_t* msg, size_t index);
double        msg_atom_float    (const message_t* msg, size_t index);
int32_t       msg_atom_int      (const message_t* msg, size_t index);
const char*   msg_atom_string   (const message_t* msg, size_t index);

#endif // SSJ__REMOTE_H__INCLUDED
