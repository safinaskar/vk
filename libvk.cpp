// Я забил на корректное освобождение ресурсов в случае ошибки
// Никак не учитываются права на файл с токеном
// Периодически увеличивать версию API
// TODO - это всё, отмеченное TODO и NEVER

//@ #ifndef _VK_LIBVK_HPP
//@ #define _VK_LIBVK_HPP
//@
//@ #include <json-c/json.h>
//@
//@ #include <string>
//@
//@ // NEVER: int? И то же везде в libvk и написанных на ней приложениях (в том числе json_object_get_int)
//@ #define VK_JSON_ARRAY_FOR_EACH_START(array, i, item, start) \
//@   if (1) \
//@     { \
//@       struct json_object *_vk_array = (array); \
//@       for (int i = (start); i < json_object_array_length (_vk_array); ++i) \
//@         { \
//@           struct json_object *item = json_object_array_get_idx (_vk_array, i); \
//@           if (1)
//@
//@ #define VK_JSON_ARRAY_FOR_EACH_START_END \
//@           else \
//@             { \
//@             } \
//@         } \
//@     } \
//@   else \
//@     do \
//@       { \
//@       } \
//@     while (0)
//@
//@ #define VK_JSON_ARRAY_RFOR_EACH(array, i, item) \
//@   if (1) \
//@     { \
//@       struct json_object *_vk_array = (array); \
//@       for (int i = json_object_array_length (_vk_array) - 1; i >= 0; --i) \
//@         { \
//@           struct json_object *item = json_object_array_get_idx (_vk_array, i); \
//@           if (1)
//@
//@ #define VK_JSON_ARRAY_RFOR_EACH_END \
//@           else \
//@             { \
//@             } \
//@         } \
//@     } \
//@   else \
//@     do \
//@       { \
//@       } \
//@     while (0)
//@
//@ #define VK_JSON_ARRAY_FOR_EACH(array, i, item) VK_JSON_ARRAY_FOR_EACH_START (array, i, item, 0)
//@ #define VK_JSON_ARRAY_FOR_EACH_END VK_JSON_ARRAY_FOR_EACH_START_END

#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <curl/curl.h>

#include <libsh.h>

#include <string>

#include "libvk.hpp"

using namespace std::literals;

static CURL *curl_handle;

namespace
{
  class NCzvkiQa_t
  {
  public:
    NCzvkiQa_t (void)
    {
      sh_curl_wrapper (curl_global_init (CURL_GLOBAL_ALL));

      curl_handle = sh_x_curl_easy_init ();
    }
    ~NCzvkiQa_t (void)
    {
      curl_easy_cleanup (curl_handle);

      curl_global_cleanup ();
    }
  };
  NCzvkiQa_t NCzvkiQa;
}

// You should json_object_put (...)
static struct json_object *
parse (const char *text)
{
  enum json_tokener_error error;

  struct json_object *result = json_tokener_parse_verbose (text, &error);

  if (result == NULL)
    {
      sh_throwx ("cannot parse JSON: %s", json_tokener_error_desc (error));
    }

  return result;
}

// You should not json_object_put (...)
struct json_object * //@
vk_extract (struct json_object *object, const char *key)//@;
{
  struct json_object *result;

  if (!json_object_object_get_ex (object, key, &result))
    {
      sh_throwx ("cannot find \"%s\" in a json object", key);
    }

  return result;
}

// NEVER: сколько поставить?
static const int max_answer_size = 10000000;

struct wtm_t
{
  char *memory;
  size_t size;
};

static size_t
write_to_memory (char *ptr, size_t size, size_t nmemb, void *userdata)
{
  struct wtm_t *wtm_struct = (struct wtm_t *) userdata;

  size_t new_size = wtm_struct->size + size * nmemb;

  if (new_size >= max_answer_size)
    {
      sh_throwx ("answer is too big");
    }

  memcpy (wtm_struct->memory + wtm_struct->size, ptr, size * nmemb);

  wtm_struct->size = new_size;

  return size * nmemb;
}

// Хорошо бы в программах это устанавливать
//@ extern bool verbose;
bool verbose = false;

// You should free (...)
static char *
https (void)
{
  struct wtm_t wtm_struct;

  sh_curl_wrapper (curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, &write_to_memory));
  sh_curl_wrapper (curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, (void *) &wtm_struct));
  sh_curl_wrapper (curl_easy_setopt (curl_handle, CURLOPT_TIMEOUT, 300L)); // NEVER: хорошо бы, скажем, таймаут 300 секунд для POST и 5 секунд для GET

  perform:;

  wtm_struct.memory = (char *) sh_x_malloc (max_answer_size);
  wtm_struct.size = 0;

  CURLcode errornum = curl_easy_perform (curl_handle);

  switch (errornum)
    {
      case CURLE_OK:
        wtm_struct.memory[wtm_struct.size] = '\0';

        if (verbose)
          {
            sh_x_fprintf (stderr, "Log: got: %s\n", wtm_struct.memory);
          }

        break;

      case CURLE_OPERATION_TIMEDOUT:
        sh_warnx ("https: timeout, restarting");
        free (wtm_struct.memory);
        goto perform;

      case CURLE_COULDNT_CONNECT:
        sh_warnx ("https: cannot connect, restarting");
        free (wtm_struct.memory);
        goto perform;

      default:
        sh_throwx ("curl_easy_perform (curl_handle): %s", curl_easy_strerror (errornum));
    }

  {
    long response_code;

    sh_curl_wrapper (curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &response_code));

    if (response_code != 200)
      {
        sh_throwx ("response code is %ld (200 expected)", response_code);
      }
  }

  curl_easy_reset (curl_handle);

  return wtm_struct.memory;
}

// You should free (...)
static char *
text_get (const char *url)
{
  if (verbose)
    {
      sh_x_fprintf (stderr, "Log: http request to %s\n", url);
    }

  sh_curl_wrapper (curl_easy_setopt (curl_handle, CURLOPT_URL, url));

  return https ();
}

// You should json_object_put (...)
static struct json_object *
get (const char *url)
{
  char *text = text_get (url);
  struct json_object *result = parse (text);

  free (text);

  return result;
}

static std::string
getline_string_fclose (FILE *fin)
{
  char *_result = sh_getline_fclose (fin);
  std::string result = _result;
  free (_result);
  return result;
}

static const char token_file[] = "/var/tmp/vk-token";

static std::string scope;
static std::string token;
static std::string app_id = "3313802"; // NEVER: захардкодено

void //@
vk_set_app_id (const std::string &new_add_id)//@;
{
  app_id = new_add_id;
}

std::string //@
vk_enc (const std::string &s)//@;
{
  std::string result;

  for (const char c : s)
    {
      if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || strchr ("0123456789-_.~", c) != NULL)
        {
          result.push_back (c);
        }
      else
        {
          char a[4];
          sh_xx_snprintf (a, sizeof (a), "%%%02X", (int) (unsigned char) c);
          result += a;
        }
    }

  return result;
}

static void
force_prompt (void)
{
  // vk_enc (scope), чтобы ссылку было удобно копировать из консоли
  FILE *tty = sh_x_fopen ("/dev/tty", "r+");
  sh_x_fprintf (tty, "%s\n", ("https://oauth.vk.com/authorize?client_id="s + app_id + "&scope="s + vk_enc (scope) + "&redirect_uri=https://oauth.vk.com/blank.html&display=popup&response_type=token"s).c_str ());
  token = getline_string_fclose (tty);

  FILE *token_fp = sh_x_fopen (token_file, "w");
  sh_x_fprintf (token_fp, "%s\n", token.c_str ());
  sh_x_fclose (token_fp);
}

void //@
vk_init (const std::string &arg_scope)//@;
{
  scope = arg_scope;

  FILE *token_fp = fopen (token_file, "r");

  if (token_fp == NULL)
    {
      force_prompt ();
    }
  else
    {
      token = getline_string_fclose (token_fp);
    }
}

// Удаляет файл в случае ошибки!
bool //@
vk_curl (const std::string &url, const std::string &file)//@;
{
  sh_x_fprintf (stderr, "%s", (url + " -> "s + file + "... "s).c_str ());

  FILE *fout = sh_x_fopen (file.c_str (), "w");

  sh_curl_wrapper (curl_easy_setopt (curl_handle, CURLOPT_URL, url.c_str ()));
  sh_curl_wrapper (curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, (void *) fout));
  sh_curl_wrapper (curl_easy_perform (curl_handle));

  sh_x_fclose (fout);

  {
    long response_code;

    sh_curl_wrapper (curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &response_code));

    curl_easy_reset (curl_handle);

    switch (response_code)
      {
        case 200:
          sh_x_fprintf (stderr, "OK\n");
          return true;
        case 302:
        case 404:
          sh_x_unlink (file.c_str ());
          sh_x_fprintf (stderr, "http error\n");
          return false;
        default:
          sh_throwx ("response code is %ld (200 expected)", response_code);
      }
  }

  /* NOTREACHED */
}

void //@
vk_x_curl (const std::string &url, const std::string &file)//@;
{
  if (!vk_curl (url, file))
    {
      sh_throwx ("http error");
    }
}

// You should json_object_put (...)
// Не проверяем, что токен обладает нужными привелегиями. Например, friends.get успешно сработает и без прав, но при этом ему не будет доступен список моих друзей, если нет scope friends
int //@
vk_request (const std::string &request, struct json_object **response)//@;
{
  {
    const char *q = strchr (request.c_str (), '?');

    if (q == NULL || q[1] != 'v' || q[2] != '=')
      {
        sh_throwx ("vk_request: no \"?v=\"");
      }
  }

  bool captcha_ignored = false;
  std::string captcha_params;

  for (;;)
    {
      struct json_object *answer = get (("https://api.vk.com/method/"s + request + captcha_params + "&access_token="s + token).c_str ());

      if (json_object_object_get_ex (answer, "response", response))
        {
          json_object_get (*response);
          json_object_put (answer);
          return 0;
        }

      *response = vk_extract (answer, "error");

      int result = json_object_get_int (vk_extract (*response, "error_code"));
      std::string error_msg = json_object_get_string (vk_extract (*response, "error_msg"));

      // Не удалось вопроизвести в тестах ошибку 7. Нет прав для выполнения этого действия / Permission to perform this action is denied

      // 5. Авторизация пользователя не удалась / User authorization failed, в том числе пустой токен
      // 10. Произошла внутренняя ошибка сервера / Internal server error, в том числе (в случае нужного error_msg) неправильный токен
      // 15. Доступ запрещён / Access denied, в том числе (в случае нужного error_msg) действие выходит за пределы scope токена
      if (result == 5 || (result == 10 && error_msg == "Internal server error: could not get application") || (result == 15 && error_msg == "Access denied: no access to call this method"))
        {
          force_prompt ();
        }
      // 6. Too many requests per second / Слишком много запросов в секунду
      else if (result == 6)
        {
        }
      // 14. Captcha needed / Требуется ввод кода с картинки (Captcha)
      else if (result == 14)
        {
          // NEVER: дать опциональную возможность юзеру ввести капчу в любом случае?
          if (captcha_ignored)
            {
              std::string sid = json_object_get_string (vk_extract (*response, "captcha_sid"));
              std::string img = json_object_get_string (vk_extract (*response, "captcha_img"));

              vk_x_curl (img, "/tmp/libvk-captcha.jpg");

              pid_t pid = sh_safe_fork ();

              if (pid == 0)
                {
                  sh_x_close (2);
                  sh_x_creat ("/dev/null", 0666);
                  sh_x_execlp ("eog", "eog", "/tmp/libvk-captcha.jpg", (char *) NULL);
                }

              captcha_params = "&captcha_sid="s + sid + "&captcha_key="s + vk_enc (getline_string_fclose (sh_x_fopen ("/dev/tty", "r")));

              // NEVER: что если это убивает весь eog со всеми картинками?
              sh_x_kill (pid, SIGTERM);
              sh_waitpid_status (pid, 0);
              sh_x_unlink ("/tmp/libvk-captcha.jpg");
            }
          else
            {
              sh_warnx ("ignoring captcha, keep patience");
              sh_success (sh_x_system ("sleep 50")); // 45 seconds is too few
              captcha_ignored = true;
            }
        }
      else
        {
          json_object_get (*response);
          json_object_put (answer);
          return result;
        }

      json_object_put (answer);
    }

  /* NOTREACHED */
}

struct json_object * //@
vk_req (const std::string &request)//@;
{
  struct json_object *result;

  if (vk_request (request, &result) == 0)
    {
      return result;
    }
  else
    {
      sh_throwx ("%s", json_object_to_json_string_ext (result, JSON_C_TO_STRING_NOZERO));
    }
}

bool //@
vk_download_audio (struct json_object *track, const std::string &file)//@;
{
  std::string url = json_object_get_string (vk_extract (track, "url"));

  if (url.empty ())
    {
      return false;
    }

  if (vk_curl (url, file))
    {
      return true;
    }

  struct json_object *response = vk_req ("audio.getById?v=5.34&audios="s + std::to_string (json_object_get_int (vk_extract (track, "owner_id"))) + "_"s + std::to_string (json_object_get_int (vk_extract (track, "id"))));

  // Что если тут тоже пустой url?
  vk_x_curl (json_object_get_string (vk_extract (json_object_array_get_idx (response, 0), "url")), file);

  json_object_put (response);

  return true;
}

void //@
vk_x_download_audio (struct json_object *track, const std::string &file)//@;
{
  if (!vk_download_audio (track, file))
    {
      sh_throwx ("vk_x_download_audio: audio deleted");
    }
}

// Нужен scope audio
// NEVER: как загрузить сразу несколько?
struct json_object * //@
vk_upload_audio (const std::string &mp3_file, const std::string &artist, const std::string &title)//@;
{
  std::string upload_url;

  {
    struct json_object *response = vk_req ("audio.getUploadServer?v=5.34");
    upload_url = json_object_get_string (vk_extract (response, "upload_url"));
    json_object_put (response);
  }

  struct json_object *answer;

  {
    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    curl_formadd (&post, &last, CURLFORM_COPYNAME, "file", CURLFORM_FILE, mp3_file.c_str (), CURLFORM_FILENAME, "a.mp3", CURLFORM_END);

    sh_curl_wrapper (curl_easy_setopt (curl_handle, CURLOPT_HTTPPOST, post));

    answer = get (upload_url.c_str ());

    curl_formfree (post);
  }

  struct json_object *result = vk_req ("audio.save?v=5.34&server="s + std::to_string (json_object_get_int (vk_extract (answer, "server"))) + "&audio="s + vk_enc (json_object_get_string (vk_extract (answer, "audio"))) + "&hash="s + json_object_get_string (vk_extract (answer, "hash")) + (artist.empty () ? ""s : "&artist="s + vk_enc (artist)) + (title.empty () ? ""s : "&title="s + vk_enc (title)));

  json_object_put (answer);

  return result;
}

//@
//@ #endif // ! _VK_LIBVK_HPP
