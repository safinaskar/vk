#define _POSIX_C_SOURCE 1

#include <libsh.h>

#include <libsh/cxx.hpp>
#include "libvk.hpp"

using namespace std::literals;

int
main (int, char *argv[])
{
  sh_init (argv[0]);
  sh_arg_parse (&argv, "Usage: "s + sh_get_program () + " [OPTION]... SCOPE REQUEST\nExample: "s + sh_get_program () + " 0 friends.get?v=5.34\n"s, "",
    sh_arg_make_opt ({}, {"app-id"}, sh_arg_mandatory, [](const char *s){ vk_set_app_id (s); }, "APP-ID", "")
  );
  const char *scope = sh_arg_operand (&argv);
  const char *request = sh_arg_operand (&argv);
  sh_arg_end (argv);

  vk_init (scope);

  struct json_object *response = vk_req (request);
  sh_x_printf ("%s\n", json_object_to_json_string_ext (response, JSON_C_TO_STRING_NOZERO));
  json_object_put (response);
}
