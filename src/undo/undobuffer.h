/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/



#ifndef undobuffer_h
#define undobuffer_h

struct _undobuffer_s;

typedef struct _undobuffer_s undobuffer_t;

struct undo_operation_s;
typedef void (*undo_operation_perform_fn)(undobuffer_t *undobuffer, struct undo_operation_s *op);
typedef void (*undo_operation_deinit_fn)(struct undo_operation_s *op);

typedef struct undo_operation_s {
    struct undo_operation_s *next;
    undo_operation_perform_fn perform;
    undo_operation_deinit_fn deinit;
} undo_operation_t;

undobuffer_t *
undobuffer_alloc(void);

void
undobuffer_free (undobuffer_t *undobuffer);

void
undobuffer_set_enabled (undobuffer_t *undobuffer, int enabled);

int
undobuffer_is_enabled (undobuffer_t *undobuffer);

void
undobuffer_group_begin (undobuffer_t *undobuffer);

void
undobuffer_group_end (undobuffer_t *undobuffer);

int
undobuffer_is_grouping (undobuffer_t *undobuffer);

undo_operation_t *
undobuffer_get_current_operation (undobuffer_t *undobuffer);

void
undobuffer_append_operation (undobuffer_t *undobuffer, undo_operation_t *op);

/// @param undobuffer the buffer to execute
/// @param current_undobuffer the current buffer to register the resulting undo operations
void
undobuffer_execute (undobuffer_t *undobuffer, undobuffer_t *current_undobuffer);

int
undobuffer_has_operations(undobuffer_t *undobuffer);

#endif /* undobuffer_h */
