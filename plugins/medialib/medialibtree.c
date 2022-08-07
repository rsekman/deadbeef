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

#include <stdlib.h>
#include <sys/time.h>
#include "medialibsource.h"
#include "medialibtree.h"

static DB_functions_t *deadbeef;

static char *artist_album_bc;
static char *title_bc;

static int _is_blank_text (const char *track_field) {
    if (!track_field) {
        return 1;
    }
    for (int i = 0; track_field[i]; i++) {
        if (track_field[i] < 0 || track_field[i] > 0x20) {
            return 0;
        }
    }
    return 1;
}

static ml_tree_item_t *
_tree_item_alloc (uint64_t row_id) {
    ml_tree_item_t *item = calloc (1, sizeof (ml_tree_item_t));
    item->row_id = row_id;
    return item;
}

static void
get_albums_for_collection_group_by_field (medialib_source_t *source, ml_tree_item_t *root, ml_collection_t *coll, const char *field, int field_tf, const char /* nonnull */ *default_field_value, int selected) {

    default_field_value = deadbeef->metacache_add_string (default_field_value);

    ml_string_t *album = source->db.albums.head;
    char text[1024];

    char *tf = NULL;
    if (field_tf) {
        tf = deadbeef->tf_compile (field);
    }

    ml_tree_item_t *root_tail = NULL;

    for (int i = 0; i < source->db.albums.count; i++, album = album->next) {
        if (!album->items_count) {
            continue;
        }

        ml_tree_item_t *album_item = NULL;
        ml_tree_item_t *album_tail = NULL;


        // find the bucket -- a genre or artist
        const char *mc_str_for_track_field = NULL;
        const char *track_field = NULL;
        if (!tf) {
            track_field = deadbeef->pl_find_meta (album->items->it, field);
        }
        else {
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = album->items->it,
            };

            deadbeef->tf_eval (&ctx, tf, text, sizeof (text));
            track_field = mc_str_for_track_field = deadbeef->metacache_add_string (text);
        }

        if (_is_blank_text (track_field)) {
            track_field = default_field_value;
        }

        // Find the bucket of this album - e.g. a genre or an artist
        // NOTE: multiple albums may belong to the same bucket
        ml_string_t *s = NULL;
        for (s = coll->head; s; s = s->next) {
            if (track_field == s->text) {
                break;
            }
        }

        if (s == NULL) {
            if (mc_str_for_track_field) {
                deadbeef->metacache_remove_string (mc_str_for_track_field);
            }
            continue;
        }

        // Add all of the album's tracks into that bucket
        ml_collection_item_t *album_coll_item = album->items;
        for (int j = 0; j < album->items_count; j++, album_coll_item = album_coll_item->next) {
            if (selected && !deadbeef->pl_is_selected (album_coll_item->it)) {
                continue;
            }
            int append = 0;

            if (!s->coll_item) {
                s->coll_item = _tree_item_alloc (s->row_id);
                s->coll_item->text = deadbeef->metacache_add_string (s->text);
                append = 1;
            }

            ddb_playItem_t *it = album_coll_item->it;

            ml_tree_item_t *libitem = s->coll_item;

            if (!album_item && s->coll_item) {
                album_item = _tree_item_alloc (s->row_id);
                if (s->coll_item_tail) {
                    s->coll_item_tail->next = album_item;
                    s->coll_item_tail = album_item;
                }
                else {
                    s->coll_item_tail = album_item;
                    libitem->children = album_item;
                }

                ddb_tf_context_t ctx = {
                    ._size = sizeof (ddb_tf_context_t),
                    .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                    .it = it,
                };

                deadbeef->tf_eval (&ctx, artist_album_bc, text, sizeof (text));

                album_item->text = deadbeef->metacache_add_string (text);
                libitem->num_children++;
            }

            ml_tree_item_t *track_item = calloc(1, sizeof (ml_tree_item_t));

            if (album_tail) {
                album_tail->next = track_item;
                album_tail = track_item;
            }
            else {
                album_tail = track_item;
                album_item->children = track_item;
            }
            album_item->num_children++;

            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = it,
            };

            deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

            track_item->text = deadbeef->metacache_add_string (text);
            deadbeef->pl_item_ref (it);
            track_item->track = it;

            if (!libitem->children) {
                ml_free_list (source, (ddb_medialib_item_t *)libitem);
                s->coll_item = NULL;
                s->coll_item_tail = NULL;
                continue;
            }

            if (append) {
                if (root_tail) {
                    root_tail->next = libitem;
                    root_tail = libitem;
                }
                else {
                    root_tail = libitem;
                    root->children = libitem;
                }
                root->num_children++;
            }
        }

        if (mc_str_for_track_field) {
            deadbeef->metacache_remove_string (mc_str_for_track_field);
        }
    }

    deadbeef->metacache_remove_string (default_field_value);

    if (tf) {
        deadbeef->tf_free (tf);
    }
}

static void
get_list_of_tracks_for_album (ml_tree_item_t *libitem, ml_string_t *album, int selected) {
    char text[1024];

    ml_tree_item_t *album_item = NULL;
    ml_tree_item_t *album_tail = NULL;

    ml_collection_item_t *album_coll_item = album->items;
    for (int j = 0; j < album->items_count; j++, album_coll_item = album_coll_item->next) {
        ddb_playItem_t *it = album_coll_item->it;
        if (selected && !deadbeef->pl_is_selected(it)) {
            continue;
        }
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
            .it = it,
        };

        if (!album_item) {
            album_item = libitem;
            deadbeef->tf_eval (&ctx, artist_album_bc, text, sizeof (text));

            if (!_is_blank_text(text)) {
                album_item->text = deadbeef->metacache_add_string (text);
            }
            else {
                album_item->text = deadbeef->metacache_add_string ("<?>");
            }
        }

        ml_tree_item_t *track_item = _tree_item_alloc(album_coll_item->row_id);

        if (album_tail) {
            album_tail->next = track_item;
            album_tail = track_item;
        }
        else {
            album_tail = track_item;
            album_item->children = track_item;
        }
        album_item->num_children++;

        deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

        track_item->text = deadbeef->metacache_add_string (text);
        deadbeef->pl_item_ref (it);
        track_item->track = it;
    }
}

static void
get_subfolders_for_folder (ml_tree_item_t *folderitem, ml_tree_node_t *folder, int selected) {
    if (!folderitem->text) {
        folderitem->text = deadbeef->metacache_add_string (folder->text);
    }

    ml_tree_item_t *tail = NULL;
    if (folder->children) {
        for (ml_tree_node_t *c = folder->children; c; c = c->next) {
            ml_tree_item_t *subfolder = _tree_item_alloc(folder->row_id);
            get_subfolders_for_folder (subfolder, c, selected);
            if (subfolder->num_children > 0) {
                if (tail) {
                    tail->next = subfolder;
                    tail = subfolder;
                }
                else {
                    folderitem->children = subfolder;
                    tail = subfolder;
                }
                folderitem->num_children++;
            }
            else {
                ml_free_list (NULL, (ddb_medialib_item_t *)subfolder);
            }
        }
    }
    if (folder->items) {
        for (ml_collection_item_t *i = folder->items; i; i = i->next) {
            if (selected && !deadbeef->pl_is_selected(i->it)) {
                continue;
            }
            ml_tree_item_t *trackitem = _tree_item_alloc(i->row_id);
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = i->it,
            };
            char text[1000];
            deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

            trackitem->text = deadbeef->metacache_add_string (text);
            trackitem->track = i->it;
            deadbeef->pl_item_ref (i->it);

            if (tail) {
                tail->next = trackitem;
                tail = trackitem;
            }
            else {
                folderitem->children = trackitem;
                tail = trackitem;
            }
            folderitem->num_children++;
        }
    }
}

ml_tree_item_t *
_create_item_tree_from_collection(ml_collection_t *coll, const char *filter, medialibSelector_t index, medialib_source_t *source) {
    int selected = 0;
    if (filter && source->ml_playlist) {
        deadbeef->plt_search_reset (source->ml_playlist);
        deadbeef->plt_search_process2 (source->ml_playlist, filter, 1);
        selected = 1;
    }

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);

    ml_tree_item_t *root = _tree_item_alloc(0);
    root->text = deadbeef->metacache_add_string ("All Music");

    // make sure no dangling pointers from the previous run
    if (coll) {
        for (ml_string_t *s = coll->head; s; s = s->next) {
            s->coll_item = NULL;
            s->coll_item_tail = NULL;
        }
    }

    if (index == SEL_FOLDERS) {
        get_subfolders_for_folder(root, source->db.folders_tree, selected);
    }
    else if (index == SEL_ARTISTS) {
        // list of albums for artist
        get_albums_for_collection_group_by_field (source, root, coll, "artist", 0, "<?>", selected);
    }
    else if (index == SEL_GENRES) {
        // list of albums for genre
        get_albums_for_collection_group_by_field (source, root, coll, "genre", 0, "<?>", selected);
    }
    else if (index == SEL_ALBUMS) {
        // list of tracks for album
        ml_tree_item_t *tail = NULL;
        ml_tree_item_t *parent = root;
        for (ml_string_t *s = coll->head; s; s = s->next) {
            ml_tree_item_t *item = _tree_item_alloc (s->row_id);

            get_list_of_tracks_for_album (item, s, selected);

            if (!item->children) {
                ml_free_list (source, (ddb_medialib_item_t *)item);
                continue;
            }

            if (tail) {
                tail->next = item;
                tail = item;
            }
            else {
                tail = item;
                parent->children = item;
            }
            parent->num_children++;
        }
    }

    // cleanup
    if (coll) {
        for (ml_string_t *s = coll->head; s; s = s->next) {
            s->coll_item = NULL;
            s->coll_item_tail = NULL;
        }
    }

    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    fprintf (stderr, "tree build time: %f seconds\n", ms / 1000.f);
    return root;
}

void
ml_free_list (ddb_mediasource_source_t source, ddb_medialib_item_t *_list) {
    ml_tree_item_t *list = (ml_tree_item_t *)_list;
    while (list) {
        ml_tree_item_t *next = list->next;
        if (list->children) {
            ml_free_list(source, (ddb_medialib_item_t *)list->children);
            list->children = NULL;
        }
        if (list->track) {
            deadbeef->pl_item_unref (list->track);
        }
        if (list->text) {
            deadbeef->metacache_remove_string (list->text);
        }

        free (list);
        list = next;
    }
}

void
ml_tree_init (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
    artist_album_bc = deadbeef->tf_compile ("[%album artist% - ]%album%");
    title_bc = deadbeef->tf_compile ("[%tracknumber%. ]%title%");
}

void
ml_tree_free (void) {
    if (artist_album_bc) {
        deadbeef->tf_free (artist_album_bc);
        artist_album_bc = NULL;
    }

    if (title_bc) {
        deadbeef->tf_free (title_bc);
        title_bc = NULL;
    }
}
