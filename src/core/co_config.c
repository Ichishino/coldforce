#include <coldforce/core/co_std.h>
#include <coldforce/core/co_config.h>
#include <coldforce/core/co_string.h>

//---------------------------------------------------------------------------//
// config
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_string_map_t*
co_config_read_file(
    const char* file_path
)
{
    FILE* file = fopen(file_path, "r");

    if (file == NULL)
    {
        return NULL;
    }

    co_string_map_t* config_map = co_string_map_create();

    char line[CO_CONFIG_LINE_MAX_LENGTH];

    while (fgets(line, 1024, file))
    {
        size_t length = co_string_trim(line, strlen(line));

        if ((length == 0) || (line[0] == '#'))
        {
            continue;
        }

        size_t key_length = 0;

        while (key_length < length)
        {
            if (line[key_length] == '=')
            {
                break;
            }

            ++key_length;
        }

        if ((key_length == 0) || (key_length == length))
        {
            continue;
        }

        char key[CO_CONFIG_KEY_MAX_LENGTH];
        strncpy(key, line, key_length);
        key[key_length] = 0x00;

        size_t value_length = length - key_length - 1;

        char value[CO_CONFIG_VALUE_MAX_LENGTH];
        strncpy(value, &line[key_length + 1], value_length);
        value[value_length] = 0x00;

        co_string_trim_right(key, key_length);
        co_string_trim_left(value, value_length);

        co_string_map_set(config_map, key, value);
    }

    fclose(file);

    return config_map;
}
