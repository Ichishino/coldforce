#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls_log.h>

//---------------------------------------------------------------------------//
// tls log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_tls_log_write_certificate(
    int level,
    X509* x509
)
{
    co_log_t* log = co_log_get_default();

    if (level > log->category[CO_LOG_CATEGORY_TLS].level)
    {
        return;
    }

    if (x509 == NULL)
    {
        return;
    }

    BIO* mem = BIO_new(BIO_s_mem());
    X509_print(mem, x509);

    const int length = BIO_pending(mem);

    char* str =
        (char*)co_mem_alloc(length + 1);
    str[length] = '\0';

    BIO_read(mem, str, length);

    co_mutex_lock(log->mutex);

    FILE* fp =
        (FILE*)log->category[CO_LOG_CATEGORY_TLS].output;

    co_log_write_header(
        level, CO_LOG_CATEGORY_TLS);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    char* token = strtok(str, "\n");

    while (token != NULL)
    {
        co_log_write_header(
            level, CO_LOG_CATEGORY_TLS);

        fprintf(fp, "%s\n", token);

        token = strtok(NULL, "\n");
    }

    co_log_write_header(
        level, CO_LOG_CATEGORY_TLS);
    fprintf(fp,
        "-------------------------------------------------------------\n");

    fflush(fp);

    co_mutex_unlock(log->mutex);

    co_mem_free(str);
    BIO_free(mem);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_tls_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_LOG_CATEGORY_TLS, level);

    co_log_add_category(
        CO_LOG_CATEGORY_TLS,
        CO_LOG_CATEGORY_NAME_TLS);
}
