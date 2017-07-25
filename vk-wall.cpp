#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <libsh.h>

#include <iostream>

#include <libsh/cxx.hpp>
#include <libvk.hpp>

using namespace std::literals;

int
main (int, char *argv[])
{
  std::string id = "0";

  sh_init (argv[0]);
  sh_arg_parse (&argv, "Usage: "s + sh_get_program () + " [OPTION]... ID\n"s, "",
    sh_arg_make_opt ({}, {"app-id"},   sh_arg_mandatory, [ ](const char *s){ vk_set_app_id (s);                         }, "APP-ID",   ""),
    sh_arg_make_opt ({}, {"id"},       sh_arg_mandatory, [&](const char *s){ id = s;                                    }, "ID",       "")
  );
  sh_arg_end (argv);

  vk_init ("messages"); // На самом деле права не нужны

  const int size = 100;

  for (int i = 0;; i += size)
    {
      struct json_object *response = vk_req ("wall.get?v=5.59&owner_id=" + id + "&offset=" + std::to_string (i) + "&count=" + std::to_string (size));

      struct json_object *items = vk_extract (response, "items");

      VK_JSON_ARRAY_FOR_EACH (items, i, item)
        {
          time_t t = json_object_get_int (vk_extract (item, "date"));

          struct tm *t2 = localtime (&t);

          char t4[sizeof "2000-01-01 00:00:00"];

          strftime (t4, sizeof t4, "%Y-%m-%d %H:%M:%S", t2);

          std::cout << "---------------- https://vk.com/wall" << id << "_" << json_object_get_int (vk_extract (item, "id")) << " " << t4 << "\n\n" << json_object_get_string (vk_extract (item, "text")) << "\n\n----------------\n\n";

          struct json_object *res = vk_req ("wall.getComments?v=5.59&owner_id=" + id + "&post_id=" + std::to_string (json_object_get_int (vk_extract (item, "id"))) + "&sort=desc&count=2");

          VK_JSON_ARRAY_FOR_EACH (vk_extract (res, "items"), j, item2)
            {
              std::cout << "* " << json_object_get_string (vk_extract (item2, "text")) << "\n";
            }
          VK_JSON_ARRAY_FOR_EACH_END;

          std::cout << "\n";

          json_object_put (res);
        }
      VK_JSON_ARRAY_FOR_EACH_END;

      bool br = (json_object_array_length (items) < size);

      json_object_put (response);

      if (br)
        {
          break;
        }
    }
}
