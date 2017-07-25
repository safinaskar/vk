// см. NEVER

#define _POSIX_C_SOURCE 1

#include <stdlib.h>

#include <libsh.h>

#include <libsh/cxx.hpp>
#include <libvk.hpp>

using namespace std::literals;

int
main (int, char *argv[])
{
  std::string owner = "37305740"; // I
  int start = 0;

  sh_init (argv[0]);
  sh_arg_parse (&argv, "Usage: "s + sh_get_program () + " [OPTION]... [START=0]\n"s, "",
    sh_arg_make_opt ({}, {"app-id"},   sh_arg_mandatory, [ ](const char *s){ vk_set_app_id (s); }, "APP-ID",   ""),
    sh_arg_make_opt ({}, {"owner-id"}, sh_arg_mandatory, [&](const char *s){ owner = s;         }, "OWNER-ID", "")
  );
  if (*argv != NULL)
    {
      start = sh_long2int (sh_xx_strtol (*argv, 0));
      ++argv;
    }
  sh_arg_end (argv);

  sh_x_printf ("Пропускает аудиозапись только если она удалена\n");

  vk_init ("audio");

  struct json_object *response = vk_req ("audio.get?v=5.34&owner_id="s + owner + "&count=6000"s);

  VK_JSON_ARRAY_FOR_EACH_START (vk_extract (response, "items"), i, track, start)
    {
      sh_x_printf ("%s\n", ("\033[1;31m"s + std::to_string (i) + ". "s + json_object_get_string (vk_extract (track, "artist")) + " — "s + json_object_get_string (vk_extract (track, "title")) + ", "s + std::to_string (json_object_get_int (vk_extract (track, "duration"))) + " s\033[0m"s).c_str ());
      if (vk_download_audio (track, "/tmp/.vk-dura-lex"))
        {
          sh_success (sh_x_system ("mplayer /tmp/.vk-dura-lex"));
        }
      else
        {
          sh_warnx ("audio deleted");
        }
    }
  VK_JSON_ARRAY_FOR_EACH_START_END;

  json_object_put (response);
}
