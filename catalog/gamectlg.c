#include "gamectlg.h"

/**
 * DOS Game Jam Demo Disc Game Catalog Format
 * 2023-08-09 Thomas Perl <m@thp.io>
 **/

#include <stdlib.h>

static uint8_t
_game_catalog_read_u8(struct GameCatalog *cat)
{
    uint8_t result = *((unsigned char *)cat->buf_ptr);
    cat->buf_ptr++;
    return result;
}

static uint16_t
_game_catalog_read_u16(struct GameCatalog *cat)
{
    uint16_t result = *((uint16_t *)cat->buf_ptr);
    cat->buf_ptr += 2;
    return result;
}

static struct GameCatalogStrings *
_game_catalog_read_string_list(struct GameCatalog *cat)
{
    uint16_t n = _game_catalog_read_u16(cat);

    struct GameCatalogStrings *result = malloc(sizeof(struct GameCatalogStrings) + n * sizeof(const char *));

    if (!result) {
        return NULL;
    }

    result->n = n;

    // one additional offset that points to the end
    const char *strings_begin = cat->buf_ptr + n * sizeof(uint8_t);
    uint16_t offset = 0;
    for (int i=0; i<n; ++i) {
        result->d[i] = strings_begin + offset;
        offset += _game_catalog_read_u8(cat);
    }

    // skip over string table (consumed above)
    cat->buf_ptr += offset;

    return result;
}

static struct GameCatalogStringListTable *
_game_catalog_read_index_list(struct GameCatalog *cat)
{
    uint8_t n = _game_catalog_read_u8(cat);

    struct GameCatalogStringListTable *result = malloc(sizeof(struct GameCatalogStringListTable) + n * sizeof(struct GameCatalogStringList));

    if (!result) {
        return NULL;
    }

    result->n = n;

    for (int i=0; i<n; ++i) {
        result->d[i].n = _game_catalog_read_u8(cat);
        result->d[i].d = (const uint8_t *)cat->buf_ptr;
        cat->buf_ptr += result->d[i].n * sizeof(uint8_t);
    }

    return result;
}

static struct GameCatalogGroup *
_game_catalog_read_groups_recursive(struct GameCatalog *cat, struct GameCatalogGroup *parent_group)
{
    uint8_t title_idx = _game_catalog_read_u8(cat);
    uint8_t num_children = _game_catalog_read_u8(cat);
    uint8_t num_subgroups = _game_catalog_read_u8(cat);

    struct GameCatalogGroup *result = malloc(sizeof(struct GameCatalogGroup) + num_subgroups * sizeof(struct GameCatalogGroup *));

    if (!result) {
        return NULL;
    }

    result->title_idx = title_idx;
    result->num_children = num_children;
    result->num_subgroups = num_subgroups;
    result->cursor_index = 0;
    result->scroll_offset = 0;

    result->children = (const uint8_t *)cat->buf_ptr;
    cat->buf_ptr += num_children * sizeof(uint8_t);

    result->parent_group = parent_group;

    for (int i=0; i<num_subgroups; ++i) {
        result->subgroups[i] = _game_catalog_read_groups_recursive(cat, result);
    }

    return result;
}

struct GameCatalog *
game_catalog_parse(char *buf, int len)
{
    if (!buf) {
        return NULL;
    }

    struct GameCatalog *cat = malloc(sizeof(struct GameCatalog));

    if (!cat) {
        return NULL;
    }

    cat->buf = buf;
    cat->buf_ptr = buf;
    cat->buf_len = len;

    cat->n_games = _game_catalog_read_u8(cat);
    cat->games = (struct GameCatalogGame *)cat->buf_ptr;
    cat->buf_ptr += cat->n_games * sizeof(struct GameCatalogGame);

    if ((cat->names = _game_catalog_read_string_list(cat)) == NULL) { goto fail; }
    if ((cat->descriptions = _game_catalog_read_string_list(cat)) == NULL) { goto fail; }
    if ((cat->urls = _game_catalog_read_string_list(cat)) == NULL) { goto fail; }
    if ((cat->ids = _game_catalog_read_string_list(cat)) == NULL) { goto fail; }
    if ((cat->readmes = _game_catalog_read_string_list(cat)) == NULL) { goto fail; }
    if ((cat->strings = _game_catalog_read_string_list(cat)) == NULL) { goto fail; }

    if ((cat->string_lists = _game_catalog_read_index_list(cat)) == NULL) { goto fail; }
    if ((cat->grouping = _game_catalog_read_groups_recursive(cat, NULL)) == NULL) { goto fail; }

    return cat;

fail:
    game_catalog_free(cat);
    return NULL;
}

static void
_game_catalog_free_groups_recursive(struct GameCatalogGroup *group)
{
    for (int i=0; i<group->num_subgroups; ++i) {
        _game_catalog_free_groups_recursive(group->subgroups[i]);
    }

    free(group);
}

void
game_catalog_free(struct GameCatalog *cat)
{
    _game_catalog_free_groups_recursive(cat->grouping);

    free(cat->string_lists);

    free(cat->strings);
    free(cat->ids);
    free(cat->urls);
    free(cat->descriptions);
    free(cat->names);

    free(cat->buf);
    free(cat);
}
