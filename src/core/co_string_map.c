#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string_map.h>

//---------------------------------------------------------------------------//
// string map
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_string_map_t*
co_string_map_create(
    void
)
{
    co_map_ctx_st map_ctx = { 0 };

    map_ctx.hash_size = CO_MAP_DEFAULT_HASH_SIZE;
    map_ctx.hash_key = (co_item_hash_fn)co_string_hash;
    map_ctx.destroy_key = (co_item_destroy_fn)co_string_destroy;
    map_ctx.destroy_value = (co_item_destroy_fn)co_string_destroy;
    map_ctx.duplicate_key = (co_item_duplicate_fn)co_string_duplicate;
    map_ctx.duplicate_value = (co_item_duplicate_fn)co_string_duplicate;
    map_ctx.compare_keys = (co_item_compare_fn)strcmp;

    return (co_string_map_t*)co_map_create(&map_ctx);
}
