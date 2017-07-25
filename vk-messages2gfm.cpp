// Выдаёт локальное время, т. е. время машины, на которой запускаем программу

#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#include <libsh.h>

#include <iostream>

#include <libsh/cxx.hpp>
#include "libvk.hpp"

using namespace std::literals;

int
main (int, char *argv[])
{
  sh_init (argv[0]);

  std::string me = "I";
  std::string peer = "Peer";
  std::string head = "#";

  bool time = true;

  sh_arg_parse (&argv, "Usage: "s + sh_get_program () + " [OPTION]...\n"s, "",
    sh_arg_make_opt ({}, {"app-id"},   sh_arg_mandatory, [ ](const char *s){ vk_set_app_id (s);                         }, "APP-ID",   ""),
    sh_arg_make_opt ({}, {"me"},       sh_arg_mandatory, [&](const char *s){ me = s;                                    }, "ME",       ""),
    sh_arg_make_opt ({}, {"peer"},     sh_arg_mandatory, [&](const char *s){ peer = s;                                  }, "PEER",     ""),
    sh_arg_make_opt ({}, {"no-time"},  sh_arg_optional,  [&](const char * ){ time = false;                              }, NULL,       ""),
    sh_arg_make_opt ({}, {"head"},     sh_arg_mandatory, [&](const char *s){ head = s;                                  }, "HEAD",     "")
  );
  sh_arg_end (argv);

  setlocale (LC_TIME, "ru_RU.UTF-8");

  void *buf;

  size_t n = sh_read_all_close (0, &buf);

  std::string text ((const char *)buf, n);

  enum json_tokener_error error;

  struct json_object *json = json_tokener_parse_verbose (text.c_str (), &error);

  if (json == NULL)
    {
      sh_throwx ("cannot parse JSON: %s", json_tokener_error_desc (error));
    }

  int year = 0;
  int mon = 0;
  int mday = 0;

  VK_JSON_ARRAY_FOR_EACH (json, _i, item)
    {
      bool complex = false;

      // I add braces, because libjson-c doesn't add them
      {
        json_object_object_foreach (item, key, _value)
          {
            std::string k = key;

            if (!(k == "id" || k == "body" || k == "user_id" || k == "from_id" || k == "date" || k == "read_state" || k == "out" || k == "random_id"))
              {
                complex = true;
              }
          }
      }

      if (complex)
        {
          std::cout << json_object_to_json_string_ext (item, JSON_C_TO_STRING_SPACED) << "\n\n";
        }

      time_t t = json_object_get_int (vk_extract (item, "date"));

      struct tm *t2 = localtime (&t);

      if (t2->tm_year != year || t2->tm_mon != mon || t2->tm_mday != mday)
        {
          year = t2->tm_year;
          mon = t2->tm_mon;
          mday = t2->tm_mday;

          std::cout << head << " " << t2->tm_mday << " ";

          char t4[sizeof "янв 2000 г. (some additional space)"];

          strftime (t4, sizeof t4, "%b %Y г.", t2);

          std::cout << t4 << "\n\n";
        }

      char t3[sizeof "00:00:00"];

      strftime (t3, sizeof t3, "%H:%M:%S", t2);

      char *body = strdup (json_object_get_string (vk_extract (item, "body")));

      const char *line = strtok (body, "\n");

      while (line != NULL)
        {
          std::cout << "**";

          if (json_object_get_int (vk_extract (item, "out")) == 1)
            {
              std::cout << me;
            }
          else
            {
              std::cout << peer;
            }

          if (time)
            {
              std::cout << " (" << t3 << ")";
            }

          std::cout << ".** " << line << "\n\n";

          line = strtok (NULL, "\n");
        }
    }
  VK_JSON_ARRAY_FOR_EACH_END;
}
