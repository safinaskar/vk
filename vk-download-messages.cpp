#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <libsh.h>

#include <iostream>

#include <libsh/cxx.hpp>
#include "libvk.hpp"

using namespace std::literals;

int
main (int, char *argv[])
{
  sh_init (argv[0]);
  sh_arg_parse (&argv, "Usage: "s + sh_get_program () + " [OPTION]... ID\n"s, "",
    sh_arg_make_opt ({}, {"app-id"},   sh_arg_mandatory, [ ](const char *s){ vk_set_app_id (s);                         }, "APP-ID",   "")
  );
  std::string id = sh_arg_operand (&argv);
  sh_arg_end (argv);

  vk_init ("messages");

  const int size = 200;

  std::cout << "[\n";

  for (int i = -size;; i -= size)
    {
      struct json_object *response = vk_req ("messages.getHistory?v=5.52&user_id=" + id + "&start_message_id=0&offset=" + std::to_string (i) + "&count=" + std::to_string (size));

      struct json_object *items = vk_extract (response, "items");

      VK_JSON_ARRAY_RFOR_EACH (items, i, item)
        {
          std::cout << json_object_to_json_string_ext (item, JSON_C_TO_STRING_SPACED) << ",\n";
        }
      VK_JSON_ARRAY_RFOR_EACH_END;

      bool br = (json_object_array_length (items) < size);

      json_object_put (response);

      if (br)
        {
          break;
        }
    }

  std::cout << "{}\n]\n";
}
