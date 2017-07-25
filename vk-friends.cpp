#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>

#include <libsh.h>

#include <libsh/cxx.hpp>
#include "libvk.hpp"

using namespace std::literals;

int
main (int, char *argv[])
{
  std::string start = "0";
  bool force = false;

  sh_init (argv[0]);
  sh_arg_parse (&argv, "Usage: "s + sh_get_program () + " [OPTION]... [ID]\n"s, "",
    sh_arg_make_opt ({   }, {"app-id"},   sh_arg_mandatory, [ ](const char *s){ vk_set_app_id (s); }, "APP-ID",   ""),
    sh_arg_make_opt ({'f'}, {"force"},    sh_arg_optional,  [&](const char * ){ force = true;      }, NULL,       "")
  );
  if (*argv != NULL)
    {
      start = *argv;
      ++argv;
    }
  sh_arg_end (argv);

  vk_init ("friends");

  struct json_object *response;

  int code = vk_request ("friends.get?v=5.34&user_id="s + start + "&fields=id"s, &response);

  // 15. Access denied
  if (force && code == 15)
    {
      json_object_put (response);
      exit (EXIT_SUCCESS);
    }

  if (code != 0)
    {
      sh_throwx ("%s", json_object_to_json_string_ext (response, JSON_C_TO_STRING_NOZERO));
    }

  VK_JSON_ARRAY_FOR_EACH (vk_extract (response, "items"), i, item)
    {
      printf ("%s\n", (std::to_string (json_object_get_int (vk_extract (item, "id"))) + " "s + json_object_get_string (vk_extract (item, "first_name")) + " " + json_object_get_string (vk_extract (item, "last_name"))).c_str ());
    }
  VK_JSON_ARRAY_FOR_EACH_END;

  json_object_put (response);
}
