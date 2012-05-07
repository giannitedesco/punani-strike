#include <punani/punani.h>
#include <punani/cvar.h>
#include <punani/console.h>
#include "list.h"



#define CVAR_TYPE_UNDEFINED 0
#define CVAR_TYPE_FLOAT 1
#define CVAR_TYPE_UNSIGNED_INT 2

struct _cvar {
	struct list_head c_list;
	
	int c_type;
	union {
		float *f;
		unsigned int *ui;
	} c_ptr;
	
	char *c_ns, *c_name;
};


static LIST_HEAD(cvars);

cvar_t cvar_locate(const char *ns, const char *name)
{
	cvar_t c;
	list_for_each_entry(c, &cvars, c_list) {
		if ( !strcmp(c->c_ns, ns) && !strcmp(c->c_name, name)) {
			return c;
		}
	}
	
	return NULL;
}

void cvar_register_float(const char *ns, const char *name, float *ptr)
{
	/* see if it already exists first. */
	cvar_t c;
	
	c = cvar_locate(ns, name);
	
	if ( NULL != c ) {
		con_printf("cvar already registered: %s.%s\n", ns, name);
		return;
	}

	c = (cvar_t) calloc(1, sizeof(*c));
	
	c->c_ns = strdup(ns);
	c->c_name = strdup(name);
	c->c_type = CVAR_TYPE_FLOAT;
	c->c_ptr.f = ptr;
	
	list_add_tail(&c->c_list, &cvars);
}

void cvar_register_uint(const char *ns, const char *name, unsigned int *ptr)
{
	/* see if it already exists first. */
	cvar_t c;
	
	c = cvar_locate(ns, name);
	
	if ( NULL != c ) {
		con_printf("cvar already registered: %s.%s\n", ns, name);
		return;
	}

	c = (cvar_t) calloc(1, sizeof(*c));
	
	c->c_ns = strdup(ns);
	c->c_name = strdup(name);
	c->c_type = CVAR_TYPE_UNSIGNED_INT;
	c->c_ptr.ui = ptr;
	
	list_add_tail(&c->c_list, &cvars);
}

static int parse_float(const char *str, float *val)
{
	char *end;

	float newval = strtod(str, &end);
	if ( end == str || (*end != '\0' && *end != 'f') )
		return 0;
		
	*val = newval;

	return 1;
}

static int parse_uint(const char *str, unsigned int *val)
{
	char *end;

	unsigned int newval = strtol(str, &end, 10);
	if ( end == str || (*end != '\0' && *end != 'f') )
		return 0;
		
	*val = newval;

	return 1;
}


void cvar_set(cvar_t cvar, const char *value) 
{
	if ( NULL == cvar ) {
		con_printf("cvar_set with NULL: value '%s' ignored\n", value);
		return;
	}
	
	switch(cvar->c_type) {
	case CVAR_TYPE_UNDEFINED:
		con_printf("cvar_set: CVAR_TYPE_UNDEFINED passed. %s.%s\n", cvar->c_ns, cvar->c_name);
		break;
	case CVAR_TYPE_FLOAT:
		if (!parse_float(value, cvar->c_ptr.f)) {
			con_printf("%s.%s: bad float %s\n", cvar->c_ns, cvar->c_name, value);
		}
		break;
	case CVAR_TYPE_UNSIGNED_INT:
		if (!parse_uint(value, cvar->c_ptr.ui)) {
			con_printf("%s.%s: bad uint %s\n", cvar->c_ns, cvar->c_name, value);
		}
		break;
	}
}
