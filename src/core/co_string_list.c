#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string_list.h>

//---------------------------------------------------------------------------//
// string list
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_string_list_t*
co_string_list_create(
    void
)
{
    co_list_ctx_st list_ctx = { 0 };

    list_ctx.destroy_value =
        (co_item_destroy_fn)co_string_destroy;
    list_ctx.duplicate_value =
        (co_item_duplicate_fn)co_string_duplicate;
    list_ctx.compare_values =
        (co_item_compare_fn)strcmp;

    return (co_string_list_t*)co_list_create(&list_ctx);
}
