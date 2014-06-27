#include <Python.h>
#include "structmember.h"
#include <dhcpctl/dhcpctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <isc/result.h>
#include <arpa/inet.h>

#define FREE      1
#define ACTIVE    2
#define EXPIRED   3
#define RELEASED  4
#define ABANDONED 5
#define RESET     6
#define BACKUP    7

int convert_dhcpctl_value_to_int(dhcpctl_data_string value, int* valref) {
	int val;

	if(value->len == sizeof(int)) {
		memcpy(&val, value->value, value->len);

		*valref = ntohl(val);

		return 0;
	}

	return 1;
}

int lease_get_value_as_int(dhcpctl_handle lease, const char* strval) {
	dhcpctl_data_string value = NULL;
	int                 ret;

	dhcpctl_get_value(&value, lease, strval);

	if(value) convert_dhcpctl_value_to_int(value, &ret);

	dhcpctl_data_string_dereference(&value, MDL);

	return ret;
}

char* lease_get_value_as_string(dhcpctl_handle lease, const char* strval) {
	dhcpctl_data_string value = NULL;
	char*               ret   = NULL;

	dhcpctl_get_value(&value, lease, strval);

	if(value) {
		ret = malloc(value->len + 1);

		memset(ret, 0, value->len + 1);
		memcpy(ret, value->value, value->len);
	}

	dhcpctl_data_string_dereference(&value, MDL);

	return ret;
}

typedef struct {
	PyObject_HEAD
	dhcpctl_handle handle;
} connection_object;

static PyObject* connection_object_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	connection_object* self = (connection_object*)(type->tp_alloc(type, 0));

	if(self) self->handle = NULL;

	return (PyObject*)(self);
}

static int initialized = 0;

static int connection_object_init(connection_object* self, PyObject* args) {
	const char* address;
	int         port = 7911;

	if(!PyArg_ParseTuple(args, "s|i", &address, &port)) return -1;

	if(!initialized) {
		dhcpctl_initialize();

		initialized = 1;
	}

	if(dhcpctl_connect(&self->handle, address, port, 0)) {
		PyErr_SetString(PyExc_RuntimeError, "couldn't connect");

		return -1;
	}

	return 0;
}

static PyObject* connection_object_get_lease(connection_object* self, PyObject* args) {
	dhcpctl_data_string ipaddrstring = NULL;
	dhcpctl_handle      lease        = NULL;
	PyObject*           obj          = NULL;
	char*               name         = NULL;
	char*               ipaddr       = NULL;

	isc_result_t   waitstatus;
	struct in_addr convaddr;

	time_t time = 0;

	if(!PyArg_ParseTuple(args, "s", &ipaddr)) return NULL;

	dhcpctl_new_object(&lease, self->handle, "lease");

	inet_pton(AF_INET, ipaddr, &convaddr);

	omapi_data_string_new(&ipaddrstring, 4, MDL);

	memcpy(ipaddrstring->value, &convaddr.s_addr, 4);

	dhcpctl_set_value(lease, ipaddrstring, "ip-address");
	dhcpctl_open_object(lease, self->handle, 0);
	dhcpctl_wait_for_completion(lease, &waitstatus);

	if(waitstatus == ISC_R_SUCCESS) {
		if(lease_get_value_as_int(lease, "state") == ACTIVE) {
			time = (time_t)(lease_get_value_as_int(lease, "starts"));
			name = lease_get_value_as_string(lease, "client-hostname");
		}
	}

	obj = Py_BuildValue("sls", ipaddr, (long)(time), name);

	if(name) free(name);

	dhcpctl_data_string_dereference(&ipaddrstring, MDL);

	omapi_object_dereference(&lease, MDL);

	return obj;
}

static PyObject* connection_object_close(connection_object* self, PyObject* args) {
	if(self->handle) {
		omapi_disconnect(self->handle->outer->outer, 1);
		omapi_object_dereference(&self->handle, MDL);

		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

static PyMethodDef connection_object_methods[] = {
	{
		"get_lease", (PyCFunction)(connection_object_get_lease), METH_VARARGS,
		"DOCSTRING"
	},
	{
		"close", (PyCFunction)(connection_object_close), METH_NOARGS,
		"DOCSTRING"
	},
	{ NULL, NULL, 0, NULL }
};

static PyTypeObject connection_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"lease.Connection", /* tp_name */
	sizeof(connection_object), /* tp_basicsize */
	0, /* tp_itemsize */
	0, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_reserved */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	0, /* tp_hash  */
	0, /* tp_call */
	0, /* tp_str */
	0, /* tp_getattro */
	0, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"DOCSTRING", /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	connection_object_methods, /* tp_methods */
	0, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	(initproc)(connection_object_init), /* tp_init */
	0, /* tp_alloc */
	connection_object_new, /* tp_new */
	0, /* tp_free */
	0, /* tp_is_gc  */
	0, /* tp_bases */
	0, /* tp_mro */
	0, /* tp_cache */
	0, /* tp_subclasses */
	0, /* tp_weaklist */
	0, /* tp_del */
	0 /* tp_version_tag */
};

PyDoc_STRVAR(
	MODULE_DOCSTRING,
	"C wrapper module for querying lease information using the dhcpctl library."
);

static PyModuleDef MODULE_DEF = {
	PyModuleDef_HEAD_INIT,
	"lease",
	MODULE_DOCSTRING,
	-1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

PyMODINIT_FUNC PyInit_dhcpctl() {
	PyObject* module;

	if(PyType_Ready(&connection_type) < 0) return NULL;

	module = PyModule_Create(&MODULE_DEF);

	if(!module) return NULL;

	Py_INCREF(&connection_type);

	PyModule_AddObject(module, "Connection", (PyObject*)(&connection_type));

	return module;
}

