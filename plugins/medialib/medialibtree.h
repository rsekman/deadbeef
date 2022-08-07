/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#ifndef medialibtree_h
#define medialibtree_h

#include <stdint.h>
#include "../../deadbeef.h"
#include "medialibdb.h"

typedef enum {
    SEL_ALBUMS = 1,
    SEL_ARTISTS = 2,
    SEL_GENRES = 3,
    SEL_FOLDERS = 4,
    SEL_FILLER = -1UL,
} medialibSelector_t;

typedef struct ml_tree_item_s {
    uint64_t row_id; // a unique ID of the associated ml_string_t

    const char *text; // e.g. the genre

    DB_playItem_t *track; // NULL in non-leaf nodes

    struct ml_tree_item_s *next;
    struct ml_tree_item_s *children;
    int num_children;
} ml_tree_item_t;

void
ml_tree_init (DB_functions_t *deadbeef);

void
ml_tree_free (void);

ml_tree_item_t *
_create_item_tree_from_collection(ml_collection_t *coll, const char *filter, medialibSelector_t index, medialib_source_t *source);

void
ml_free_list (ddb_mediasource_source_t source, ddb_medialib_item_t *list);

#endif /* medialibtree_h */
