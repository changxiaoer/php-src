/*
   +----------------------------------------------------------------------+
   | Zend JIT                                                             |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2016 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@zend.com>                             |
   |          Xinchen Hui <xinchen.h@zend.com>                            |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_gdb.h"

enum {
	ZEND_GDBJIT_NOACTION,
	ZEND_GDBJIT_REGISTER,
	ZEND_GDBJIT_UNREGISTER
};

typedef struct _zend_gdbjit_code_entry {
	struct _zend_gdbjit_code_entry *next_entry;
	struct _zend_gdbjit_code_entry *prev_entry;
	const char                     *symfile_addr;
	uint64_t                        symfile_size;
} zend_gdbjit_code_entry;

typedef struct _zend_gdbjit_descriptor {
	uint32_t                         version;
	uint32_t                         action_flag;
	struct _zend_gdbjit_code_entry *relevant_entry;
	struct _zend_gdbjit_code_entry *first_entry;
} zend_gdbjit_descriptor;

ZEND_API zend_gdbjit_descriptor __jit_debug_descriptor = {
	1, ZEND_GDBJIT_NOACTION, NULL, NULL
};

ZEND_API zend_never_inline void __jit_debug_register_code()
{
	__asm__ __volatile__("");
}

ZEND_API int zend_gdb_register_code(const void *object, size_t size)
{
	zend_gdbjit_code_entry *entry;

	entry = malloc(sizeof(zend_gdbjit_code_entry) + size);
	if (entry == NULL) {
		return 0;
	}

	entry->symfile_addr = ((char*)entry) + sizeof(zend_gdbjit_code_entry);
	entry->symfile_size = size;

	memcpy((char *)entry->symfile_addr, object, size);

	entry->next_entry = NULL;
	entry->prev_entry = __jit_debug_descriptor.relevant_entry;

	if (entry->prev_entry) {
		entry->prev_entry->next_entry = entry;
	} else {
		__jit_debug_descriptor.first_entry = entry;
	}
	__jit_debug_descriptor.relevant_entry = entry;

	__jit_debug_descriptor.action_flag = ZEND_GDBJIT_REGISTER;
	__jit_debug_register_code();

	return 1;
}

ZEND_API void zend_gdb_unregister_all(void)
{
	zend_gdbjit_code_entry *entry;

	__jit_debug_descriptor.action_flag = ZEND_GDBJIT_UNREGISTER;
	while ((entry = __jit_debug_descriptor.relevant_entry)) {
		if (entry->prev_entry) {
			entry->prev_entry->next_entry = NULL;
		} else {
			__jit_debug_descriptor.first_entry = NULL;
		}

		__jit_debug_register_code();

		__jit_debug_descriptor.relevant_entry = entry->prev_entry;

		free(entry);
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
