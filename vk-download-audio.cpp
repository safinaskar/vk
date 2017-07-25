#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <libsh.h>

#include <libsh/cxx.hpp>
#include <libvk.hpp>

using namespace std::literals;

// Это хак
std::string
encode (const std::string &a)
{
  std::string result;

  for (char c : a)
    {
      if ((unsigned char)c >= 128)
        {
          result += c;
        }
      // Prohibited symbols for NTFS on Windows
      else if (c <= ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
        {
          result += '_';
        }
      else
        {
          result += c;
        }
    }

  return result;
}

std::string
truncate (const std::string &a, int len)
{
  if (a.size () > len)
    {
      return std::string (a.begin (), a.begin () + len);
    }
  else
    {
      return a;
    }
}

int
main (int, char *argv[])
{
  std::string id = "37305740"; // I
  int start = 0;

  sh_init (argv[0]);
  sh_arg_parse (&argv, "Usage: "s + sh_get_program () + " [OPTION]... [ID]\n"s, "",
    sh_arg_make_opt ({}, {"app-id"},   sh_arg_mandatory, [ ](const char *s){ vk_set_app_id (s);                         }, "APP-ID",   ""),
    sh_arg_make_opt ({}, {"start"},    sh_arg_mandatory, [&](const char *s){ start = sh_long2int (sh_xx_strtol (s, 0)); }, "START",    "")
  );
  if (argv[0] != NULL)
    {
      id = argv[0];
      ++argv;
    }
  sh_arg_end (argv);

  vk_init ("audio");

  struct json_object *response = vk_req ("audio.get?v=5.34&owner_id="s + id + "&count=6000"s);

  VK_JSON_ARRAY_FOR_EACH_START (vk_extract (response, "items"), i, track, start)
    {
      // Не уверен, что правильная строка формата
      if (!vk_download_audio (track, truncate (sh_s_asprintf ("%04d", i) + "—"s + encode (json_object_get_string (vk_extract (track, "artist"))) + "—"s + encode (json_object_get_string (vk_extract (track, "title"))), 255 - strlen (".mp3")) + ".mp3"s))
        {
          sh_warnx ("audio deleted");
        }
    }
  VK_JSON_ARRAY_FOR_EACH_END;

  json_object_put (response);
}
