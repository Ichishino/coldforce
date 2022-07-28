#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/http/co_http_auth.h>
#include <coldforce/http/co_http_string_list.h>
#include <coldforce/http/co_base64.h>
#include <coldforce/http/co_random.h>
#include <coldforce/http/co_md5.h>

//---------------------------------------------------------------------------//
// http auth
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static char*
co_http_digest_auth_hash(
    const char* algorithm,
    uint32_t count,
    ...
)
{
    va_list args;
    va_start(args, count);

    size_t length = 0;

    for (size_t index = 0; index < count; ++index)
    {
        length += strlen(va_arg(args, const char*));
    }

    va_end(args);

    length += (count - 1);

    char* raw_data =
        (char*)co_mem_alloc(length + 2);
    raw_data[0] = '\0';

    va_start(args, count);

    for (size_t index = 0; index < count; ++index)
    {
        strcat(raw_data, va_arg(args, const char*));
        strcat(raw_data, ":");
    }

    va_end(args);

    raw_data[length] = '\0';

    char* hash_hex = NULL;

    if ((algorithm == NULL) ||
        (co_string_case_compare(algorithm, "MD5") == 0) ||
        (co_string_case_compare(algorithm, "MD5-sess") == 0))
    {
        uint8_t hash[CO_MD5_HASH_SIZE];
        co_md5(raw_data, (uint32_t)length, hash);

        hash_hex = (char*)co_mem_alloc(CO_MD5_HASH_SIZE * 2 + 1);
        co_string_hex(hash, sizeof(hash), hash_hex, false);
    }

    co_mem_free(raw_data);

    return hash_hex;
}

void
co_http_auth_serialize_items(
    const co_http_auth_t* auth,
    const char** unquate_names,
    co_byte_array_t* buffer
)
{
    if (co_ss_map_get_count(auth->items) == 0)
    {
        return;
    }

    co_byte_array_add(buffer, " ", 1);

    co_ss_map_iterator_t it;
    co_ss_map_iterator_init(auth->items, &it);

    while (co_ss_map_iterator_has_next(&it))
    {
        co_ss_map_data_st* data =
            co_ss_map_iterator_get_next(&it);

        co_byte_array_add(buffer, data->key, strlen(data->key));

        bool unquotes = false;

        if (unquate_names != NULL)
        {
            for (size_t index = 0; unquate_names[index] != NULL; ++index)
            {
                if (co_string_case_compare(unquate_names[index], data->key) == 0)
                {
                    unquotes = true;

                    break;
                }
            }
        }

        if (unquotes)
        {
            co_byte_array_add(buffer, "=", 1);
            co_byte_array_add(buffer, data->value, strlen(data->value));
            co_byte_array_add(buffer, ", ", 2);
        }
        else
        {
            co_byte_array_add(buffer, "=\"", 2);
            co_byte_array_add(buffer, data->value, strlen(data->value));
            co_byte_array_add(buffer, "\", ", 3);
        }
    }

    co_byte_array_set_count(
        buffer, co_byte_array_get_count(buffer) - 2);
}

char*
co_http_auth_serialize_request(
    const co_http_auth_t* auth
)
{
    if (auth->credentials == NULL)
    {
        return NULL;
    }

    co_byte_array_t* buffer = co_byte_array_create();

    co_byte_array_add(buffer,
        auth->scheme, strlen(auth->scheme));

    const char** unquate_names = NULL;

    if (co_string_case_compare(auth->scheme, "Basic") == 0)
    {
        co_byte_array_add(buffer, " ", 1);
        co_byte_array_add(buffer, auth->credentials, strlen(auth->credentials));
    }
    else if (co_string_case_compare(auth->scheme, "Digest") == 0)
    {
        if (auth->method == NULL)
        {
            co_byte_array_destroy(buffer);

            return NULL;
        }

        static const char* digest_names[] = {
            "qop", "nc", "algorithm", "stale", NULL
        };
        unquate_names = digest_names;

        const char* algorithm =
            co_http_auth_get_item(auth, "algorithm");

        bool algorithm_sess = false;

        if ((algorithm == NULL) ||
            co_string_case_compare(algorithm, "MD5") == 0)
        {
            algorithm_sess = false;
        }
        else if (co_string_case_compare(algorithm, "MD5-sess") == 0)
        {
            algorithm_sess = true;
        }
        else
        {
            co_byte_array_destroy(buffer);

            return NULL;
        }

        const char* uri = co_http_auth_get_item(auth, "uri");
        const char* nonce = co_http_auth_get_item(auth, "nonce");
        const char* cnonce = co_http_auth_get_item(auth, "cnonce");
        const char* nc = co_http_auth_get_item(auth, "nc");
        const char* qop = co_http_auth_get_item(auth, "qop");

        char* sess = NULL;
        const char* a1_hash = NULL;

        if (algorithm_sess)
        {
            sess = co_http_digest_auth_hash(
                algorithm, 3, auth->credentials, nonce, cnonce);
            a1_hash = sess;
        }
        else
        {
            a1_hash = auth->credentials;
        }

        char* a2_hash =
            co_http_digest_auth_hash(
                algorithm, 2, auth->method, uri);
        char* response =
            co_http_digest_auth_hash(
                algorithm, 6, a1_hash, nonce, nc, cnonce, qop, a2_hash);

        co_byte_array_add(buffer, " response=", 10);
        co_byte_array_add(buffer, "\"", 1);
        co_byte_array_add(buffer, response, strlen(response));
        co_byte_array_add(buffer, "\",", 2);

        co_string_destroy(sess);
        co_string_destroy(a2_hash);
        co_string_destroy(response);
    }

    co_http_auth_serialize_items(auth, unquate_names, buffer);

    co_byte_array_add(buffer, "\0", 1);

    char* str = (char*)co_byte_array_detach(buffer);
    co_byte_array_destroy(buffer);

    return str;
}

bool
co_http_auth_deserialize_request(
    const char* str,
    co_http_auth_t* auth
)
{
    auth->request = true;

    co_http_string_item_st items[32] = { 0 };

    size_t item_count =
        co_http_string_list_parse(str, items, 32);

    if (item_count == 0)
    {
        return false;
    }

    if (items[0].second != NULL)
    {
        return false;
    }

    auth->scheme = items[0].first;
    items[0].first = NULL;

    if (auth->scheme == NULL)
    {
        return false;
    }

    if (items[1].second == NULL)
    {
        if (co_string_case_compare(auth->scheme, "Basic") == 0)
        {
            auth->credentials = items[1].first;
            items[1].first = NULL;
        }
    }

    for (size_t index = 0; index < item_count; ++index)
    {
        if (items[index].first != NULL)
        {
            co_ss_map_set(auth->items,
                items[index].first, items[index].second);
        }
    }

    co_http_string_list_cleanup(items, item_count);

    return true;
}

char*
co_http_auth_serialize_response(
    const co_http_auth_t* auth
)
{
    co_byte_array_t* buffer = co_byte_array_create();

    co_byte_array_add(buffer,
        auth->scheme, strlen(auth->scheme));

    const char** unquate_names = NULL;

    if (co_string_case_compare(auth->scheme, "Basic") == 0)
    {
    }
    else if (co_string_case_compare(auth->scheme, "Digest") == 0)
    {
    }

    co_http_auth_serialize_items(auth, unquate_names, buffer);

    co_byte_array_add(buffer, "\0", 1);

    char* str = (char*)co_byte_array_detach(buffer);
    co_byte_array_destroy(buffer);

    return str;
}

bool
co_http_auth_deserialize_response(
    const char* str,
    co_http_auth_t* auth
)
{
    auth->request = false;

    co_http_string_item_st items[32] = { 0 };

    size_t item_count =
        co_http_string_list_parse(str, items, 32);

    if (item_count == 0)
    {
        return false;
    }

    if (items[0].second != NULL)
    {
        return false;
    }

    auth->scheme = items[0].first;
    items[0].first = NULL;

    if (auth->scheme == NULL)
    {
        return false;
    }

    if (items[1].second == NULL)
    {
        if (co_string_case_compare(auth->scheme, "Basic") == 0)
        {
            auth->credentials = items[1].first;
            items[1].first = NULL;
        }
    }

    for (size_t index = 0; index < item_count; ++index)
    {
        if (items[index].first != NULL)
        {
            co_ss_map_set(auth->items,
                items[index].first, items[index].second);
        }
    }

    co_http_string_list_cleanup(items, item_count);

    return true;
}

char*
co_http_auth_serialize(
    const co_http_auth_t* auth
)
{
    if (auth->request)
    {
        return co_http_auth_serialize_request(auth);
    }
    else
    {
        return co_http_auth_serialize_response(auth);
    }
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http_auth_t*
co_http_auth_create(
    void
)
{
    co_http_auth_t* auth =
        (co_http_auth_t*)co_mem_alloc(sizeof(co_http_auth_t));

    if (auth == NULL)
    {
        return NULL;
    }

    auth->request = false;
    auth->scheme = NULL;
    auth->method = NULL;
    auth->credentials = NULL;
    auth->nc = 0;

    co_map_ctx_st ctx = CO_SS_MAP_CTX;

    auth->items = co_map_create(&ctx);

    return auth;
}

co_http_auth_t*
co_http_auth_create_request(
    const char* request_auth
)
{
    co_http_auth_t* auth = co_http_auth_create();

    if (!co_http_auth_deserialize_request(request_auth, auth))
    {
        co_http_auth_destroy(auth);

        return NULL;
    }

    return auth;
}

co_http_auth_t*
co_http_auth_create_response(
    const char* response_auth
)
{
    co_http_auth_t* auth = co_http_auth_create();

    if (!co_http_auth_deserialize_response(response_auth, auth))
    {
        co_http_auth_destroy(auth);

        return NULL;
    }

    return auth;
}

void
co_http_auth_destroy(
    co_http_auth_t* auth
)
{
    if (auth != NULL)
    {
        co_string_destroy(auth->scheme);
        co_string_destroy(auth->method);
        co_string_destroy(auth->credentials);
        co_ss_map_destroy(auth->items);

        co_mem_free(auth);
    }
}

void
co_http_auth_set_scheme(
    co_http_auth_t* auth,
    const char* scheme
)
{
    co_string_destroy(auth->scheme);
    auth->scheme = NULL;

    if (scheme != NULL)
    {
        auth->scheme = co_string_duplicate(scheme);
    }
}

const char*
co_http_auth_get_scheme(
    const co_http_auth_t* auth
)
{
    return auth->scheme;
}

void
co_http_auth_set_item(
    co_http_auth_t* auth,
    const char* name,
    const char* value
)
{
    co_ss_map_set(auth->items, name, value);
}

const char*
co_http_auth_get_item(
    const co_http_auth_t* auth,
    const char* name
)
{
    co_ss_map_data_st* data =
        co_ss_map_get(auth->items, name);

    return (data != NULL) ? data->value : NULL;
}

void
co_http_auth_set_credentials(
    co_http_auth_t* auth,
    const char* credentials
)
{
    co_string_destroy(auth->credentials);

    if (credentials != NULL)
    {
        auth->credentials = co_string_duplicate(credentials);
    }
    else
    {
        auth->credentials = NULL;
    }
}

const char*
co_http_auth_get_credentials(
    const co_http_auth_t* auth
)
{
    return auth->credentials;
}

//---------------------------------------------------------------------------//
// basic auth
//---------------------------------------------------------------------------//

co_http_auth_t*
co_http_basic_auth_create_request(
    const char* user,
    const char* password
)
{
    co_http_auth_t* auth = co_http_auth_create();

    if (auth == NULL)
    {
        return NULL;
    }

    auth->request = true;

    co_http_auth_set_scheme(auth, "Basic");

    size_t raw_data_length =
        strlen(user) + strlen(password) + 1;

    char* raw_data =
        (char*)co_mem_alloc(raw_data_length + 1);

    sprintf(raw_data, "%s:%s", user, password);

    char* base64_data;
    size_t base64_data_length;

    co_base64_encode(
        raw_data, raw_data_length,
        &base64_data, &base64_data_length,
        true);

    co_mem_free(raw_data);

    auth->credentials = base64_data;

    return auth;
}

co_http_auth_t*
co_http_basic_auth_create_response(
    const char* realm
)
{
    co_http_auth_t* auth = co_http_auth_create();

    if (auth == NULL)
    {
        return NULL;
    }

    auth->request = false;

    co_http_auth_set_scheme(auth, "Basic");

    if (realm != NULL)
    {
        co_http_auth_set_item(auth, "realm", realm);
    }

    return auth;
}

bool
co_http_basic_auth_get_credentials(
    const co_http_auth_t* auth,
    char** user,
    char** password
)
{
    const char* scheme = co_http_auth_get_scheme(auth);

    if (scheme == NULL ||
        co_string_case_compare(scheme, "Basic") != 0)
    {
        return false;
    }

    if (auth->credentials == NULL)
    {
        return false;
    }

    size_t base64_data_length = strlen(auth->credentials);

    char* raw_data = NULL;
    size_t raw_data_length = 0;

    if (co_base64_decode(
        auth->credentials, base64_data_length,
        (uint8_t**)&raw_data, &raw_data_length))
    {
        const char* ptr = strchr(raw_data, ':');

        if (ptr != NULL)
        {
            *user = co_string_duplicate_n(
                raw_data, (size_t)(ptr - raw_data));
            *password = co_string_duplicate(ptr + 1);

            co_mem_free(raw_data);

            return true;
        }

        co_mem_free(raw_data);
    }

    return false;
}

//---------------------------------------------------------------------------//
// digest auth
//---------------------------------------------------------------------------//

co_http_auth_t*
co_http_digest_auth_create_request(
    const char* user,
    const char* password,
    const co_http_auth_t* response_auth
)
{
    const char* realm =
        co_http_auth_get_item(response_auth, "realm");
    const char* nonce =
        co_http_auth_get_item(response_auth, "nonce");

    if (realm == NULL || nonce == NULL)
    {
        return NULL;
    }

    const char* algorithm =
        co_http_auth_get_item(response_auth, "algorithm");

    if (algorithm != NULL)
    {
        co_http_string_item_st items[8];

        size_t count =
            co_http_string_list_parse(algorithm, items, 8);

        bool md5 =
            co_http_string_list_contains(items, count, "MD5") ||
            co_http_string_list_contains(items, count, "MD5-sess");

        co_http_string_list_cleanup(items, count);

        if (!md5)
        {
            return NULL;
        }
    }

    algorithm = "MD5";

    co_http_auth_t* auth = co_http_auth_create();

    if (auth == NULL)
    {
        return NULL;
    }

    auth->request = true;

    co_http_auth_set_scheme(auth, "Digest");

    co_http_auth_set_item(auth, "realm", realm);
    co_http_auth_set_item(auth, "nonce", nonce);
    co_http_auth_set_item(auth, "username", user);
    co_http_auth_set_item(auth, "qop", "auth");
    co_http_auth_set_item(auth, "algorithm", algorithm);

    char cnonce[16+1];
    co_random_hex_string(cnonce, 16);
    co_http_auth_set_item(auth, "cnonce", cnonce);

    const char* opaque =
        co_http_auth_get_item(response_auth, "opaque");

    if (opaque != NULL)
    {
        co_http_auth_set_item(auth, "opaque", opaque);
    }

    auth->credentials =
        co_http_digest_auth_hash(
            algorithm, 3, user, realm, password);

    return auth;
}

co_http_auth_t*
co_http_digest_auth_create_response(
    const char* realm,
    const char* nonce,
    const char* opaque
)
{
    co_http_auth_t* auth = co_http_auth_create();

    if (auth == NULL)
    {
        return NULL;
    }

    auth->request = false;

    co_http_auth_set_scheme(auth, "Digest");

    co_http_auth_set_item(auth, "realm", realm);
    co_http_auth_set_item(auth, "nonce", nonce);
    co_http_auth_set_item(auth, "qop", "auth");
    co_http_auth_set_item(auth, "algorithm", "MD5");

    if (opaque != NULL)
    {
        co_http_auth_set_item(auth, "opaque", opaque);
    }

    return auth;
}

bool
co_http_digest_auth_validate(
    const co_http_auth_t* request_auth,
    const char* method,
    const char* path,
    const char* realm,
    const char* user,
    const char* password,
    const char* nonce,
    uint32_t nc
)
{
    if (co_string_case_compare(
        request_auth->scheme, "Digest") != 0)
    {
        return false;
    }

    const char* request_realm =
        co_http_auth_get_item(request_auth, "realm");
    const char* request_uri =
        co_http_auth_get_item(request_auth, "uri");
    const char* request_nonce =
        co_http_auth_get_item(request_auth, "nonce");
    const char* request_qop =
        co_http_auth_get_item(request_auth, "qop");
    const char* request_cnonce =
        co_http_auth_get_item(request_auth, "cnonce");
    const char* request_nc_str =
        co_http_auth_get_item(request_auth, "nc");
    const char* request_response =
        co_http_auth_get_item(request_auth, "response");

    if (request_realm == NULL ||
        request_uri == NULL ||
        request_nonce == NULL ||
        request_qop == NULL ||
        request_cnonce == NULL ||
        request_nc_str == NULL ||
        request_response == NULL)
    {
        return false;
    }

    if (co_string_case_compare(request_nonce, nonce) != 0)
    {
        return false;
    }

    if (co_string_case_compare(request_qop, "auth") != 0)
    {
        return false;
    }

    uint32_t request_nc = 0;

    if (strlen(request_nc_str) != 8)
    {
        return false;
    }

    if (sscanf(request_nc_str, "%08x", &request_nc) == 0)
    {
        (void)sscanf(request_nc_str, "%08X", &request_nc);
    }

    if (request_nc == 0 ||
        request_nc != nc)
    {
        return false;
    }

    const char* request_algorithm =
        co_http_auth_get_item(request_auth, "algorithm");

    if (request_algorithm != NULL)
    {
        if ((co_string_case_compare(request_algorithm, "MD5") != 0) &&
            (co_string_case_compare(request_algorithm, "MD5-sess") != 0))
        {
            return false;
        }
    }

    char* a1_hash =
        co_http_digest_auth_hash(
            request_algorithm, 3, user, realm, password);
    char* a2_hash =
        co_http_digest_auth_hash(
            request_algorithm, 2, method, path);

    if (a1_hash == NULL || a2_hash == NULL)
    {
        co_string_destroy(a1_hash);
        co_string_destroy(a2_hash);

        return false;
    }

    char nc_str[16];
    sprintf(nc_str, "%08x", nc);

    char* response =
        co_http_digest_auth_hash(
            request_algorithm, 6,
            a1_hash,
            request_nonce,
            nc_str,
            request_cnonce,
            request_qop,
            a2_hash);

    bool result =
        (co_string_case_compare(
            request_response, response) == 0);

    co_string_destroy(a1_hash);
    co_string_destroy(a2_hash);
    co_string_destroy(response);

    return result;
}

void
co_http_digest_auth_set_method(
    co_http_auth_t* auth,
    const char* method
)
{
    co_string_destroy(auth->method);
    auth->method = NULL;

    if (method != NULL)
    {
        auth->method = co_string_duplicate(method);
    }
}

const char*
co_http_digest_auth_get_method(
    const co_http_auth_t* auth
)
{
    return auth->method;
}

void
co_http_digest_auth_set_path(
    co_http_auth_t* auth,
    const char* path
)
{
    co_http_auth_set_item(auth, "uri", path);
}

const char*
co_http_digest_auth_get_path(
    const co_http_auth_t* auth
)
{
    return co_http_auth_get_item(auth, "uri");
}

void
co_http_digest_auth_set_user(
    co_http_auth_t* auth,
    const char* user
)
{
    co_http_auth_set_item(auth, "username", user);
}

const char*
co_http_digest_auth_get_user(
    const co_http_auth_t* auth
)
{
    return co_http_auth_get_item(auth, "username");
}

void
co_http_digest_auth_set_count(
    co_http_auth_t* auth,
    uint32_t count
)
{
    char nc_str[16];
    sprintf(nc_str, "%08x", count);
    co_http_auth_set_item(auth, "nc", nc_str);
}

uint32_t
co_http_digest_auth_get_count(
    const co_http_auth_t* auth
)
{
    const char* nc_str =
        co_http_auth_get_item(auth, "nc");

    if (nc_str == NULL)
    {
        return 0;
    }

    uint32_t count = 0;
    (void)sscanf(nc_str, "%08x", &count);

    return count;
}

void
co_http_digest_auth_set_opaque(
    co_http_auth_t* auth,
    const char* opaque
)
{
    co_http_auth_set_item(auth, "opaque", opaque);
}

const char*
co_http_digest_auth_get_opaque(
    const co_http_auth_t* auth
)
{
    return co_http_auth_get_item(auth, "opaque");
}
