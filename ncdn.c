#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <gc.h>
#include <jansson.h>
#include <glib.h>
#include <glib/gi18n.h>


///////////////////////////////////////////////////////////////////////////////


char PATH_DELIM[2] = "/\0";


///////////////////////////////////////////////////////////////////////////////


#define CMD_STR_cat "cat\0"
#define CMD_STR_set "set\0"


enum CMD {
     CMD_cat,
     CMD_set
};


struct params {
     enum CMD cmd;
     char * cmd_str;
     char * dir;
     char * file;
     char * field;
     char * value;
     gboolean from_file;
};


void
params_fprintf(struct params * params, FILE * f);
int
params_from_args(struct params * params, int argc, char ** argv);
int
params_validate(struct params * params);


//////////////////////////////////////////////////


void
params_fprintf(struct params * params, FILE * f)
{
     if (params == NULL)
          return;
     fprintf(f, "params{dir='%s', file='%s', field='%s', value='%s'}\n",
             params->dir, params->file, params->field, params->value);
}


int
params_from_args(struct params * params, int argc, char ** argv)
{
     assert(params != NULL);
     int opt;
     params->from_file = FALSE;
     while ((opt = getopt(argc, argv, ":fd:")) != -1) {
          switch (opt) {
          case 'd':
               params->dir = strdup(optarg);
               assert(params->dir != NULL);
               break;
          case 'f':
               params->from_file = TRUE;
               break;
          default:
               printf("unknown option '%c'\n", optopt);
               return -1;
          }
     }
     if (optind < argc) {
          params->cmd_str = strdup(argv[optind]);
          assert(params->cmd_str != NULL);
          optind += 1;
     }
     if (optind < argc) {
          params->file = strdup(argv[optind]);
          assert(params->file != NULL);
          optind += 1;
     }
     if (optind < argc) {
          params->field = strdup(argv[optind]);
          assert(params->field != NULL);
          optind += 1;
     }
     if (optind < argc) {
          params->value = strdup(argv[optind]);
          assert(params->value != NULL);
          optind += 1;
     }
     return 0;
}


int
params_validate_set(struct params * params)
{
     if (params->field == NULL || strcmp("\0", params->field) == 0) {
          fprintf(stderr, "missing field\n");
          return -1;
     }
     return 0;
}


int
params_validate(struct params * params)
{
     if (params->dir == NULL || strcmp("\0", params->dir) == 0)
          params->dir = strdup(".\0");

     if (params->file == NULL || strcmp("\0", params->file) == 0) {
          fprintf(stderr, "missing note\n");
          return -1;
     }

     if (strcmp(params->cmd_str, CMD_STR_cat) == 0) {
          params->cmd = CMD_cat;
          return 0;
     } else if (strcmp(params->cmd_str, CMD_STR_set) == 0) {
          params->cmd = CMD_set;
          return params_validate_set(params);
     } else {
          fprintf(stderr, "unknown cmd\n");
          return -1;
     }
}


///////////////////////////////////////////////////////////////////////////////


size_t
file_slurp(char ** contents, char * filepath)
{
     FILE * f = fopen(filepath, "r");
     if (f == NULL) {
          fprintf(stderr, "fopen(): %s\n", strerror(errno));
          return -1;
     }
     if (fseek(f, 0, SEEK_END) == -1) {
          fprintf(stderr, "fseek(): %s\n", strerror(errno));
          fclose(f);
          return -1;
     }
     long fsz = ftell(f);
     if (fsz == -1) {
          fprintf(stderr, "ftell(): %s\n", strerror(errno));
          fclose(f);
          return -1;
     }
     if (fseek(f, 0, SEEK_SET) == -1) {
          fprintf(stderr, "fseek(): %s\n", strerror(errno));
          fclose(f);
          return -1;
     }
     *contents = GC_MALLOC(sizeof(char) * fsz);
     fread(*contents, sizeof(char), fsz, f);
     if (ferror(f) != 0) {
          fprintf(stderr, "fread(): %s\n", strerror(errno));
          fclose(f);
          return -1;
     }
     return fsz;
}


int
str_concat(char ** dst, char * src1, char * src2, char * src3)
{
     int dst_sz = strlen(src1) + strlen(src2) + strlen(src3) + 1;
     *dst = GC_MALLOC(dst_sz);
     return snprintf(*dst, dst_sz, "%s%s%s", src1, src2, src3);
}


char *
str_repeat(char * s, size_t times)
{
     size_t slen;
     size_t dstlen;
     size_t i;
     char * dst;
     char * dst_wip;

     slen = strlen(s);
     dstlen = slen * times + 1;
     if ((dst = GC_MALLOC(dstlen * sizeof(char))) == NULL)
          return NULL;

     dst_wip = dst;
     for (i=0; i<times; i++)
          dst_wip = stpcpy(dst_wip, s);

     return dst;
}


char *
str_nfill(char ch, size_t n)
{
     char * dst;
     size_t i;

     if ((dst = GC_MALLOC(n * sizeof(char) + 1)) == NULL)
          return NULL;
     for (i=0; i<n; i++)
          dst[i] = ch;
     dst[i] = '\0';
     return dst;
}


///////////////////////////////////////////////////////////////////////////////


int
visit_json(json_t * obj, int level)
{
     const char * key;
     json_t * value;
     char * indent_str;

     if ((indent_str = str_nfill(' ', level*2)) == NULL)
          return -1;

     if (json_is_array(obj)) {
          for (size_t i=0; i<json_array_size(obj); i++)
               visit_json(json_array_get(obj, i), level+1);
     } else if (json_is_object(obj)) {
          void * iter = json_object_iter(obj);
          while (iter) {
               key = json_object_iter_key(iter);
               value = json_object_iter_value(iter);
               if (json_is_integer(value))
                    printf("%s%s = %lld\n", indent_str, key, json_integer_value(value));
               else if (json_is_number(value))
                    printf("%s%s = %f\n", indent_str, key, json_number_value(value));
               else if (json_is_string(value))
                    printf("%s%s = %s\n", indent_str, key, json_string_value(value));
               else if (json_is_object(value)) {
                    printf("%s%s =\n", indent_str, key);
                    visit_json(value, level+1);
               } else if (json_is_array(value)) {
                    printf("%s%s =\n", indent_str, key);
                    visit_json(value, level+1);
               }
               iter = json_object_iter_next(obj, iter);
          }
     } else {
          if (json_is_integer(obj))
               printf("%s%lld\n", indent_str, json_integer_value(obj));
          else if (json_is_number(obj))
               printf("%s%f\n", indent_str, json_number_value(obj));
          else if (json_is_string(obj))
               printf("%s%s\n", indent_str, json_string_value(obj));
          else
               printf("%sunknown type\n", indent_str);
     }
     return 0;
}


///////////////////////////////////////////////////////////////////////////////


int
run_cmd_cat(struct params * params)
{
     char * filepath;
     json_error_t error;
     json_t * root;

     str_concat(&filepath, params->dir, PATH_DELIM, params->file);
     root = json_load_file(filepath, 0, &error);
     if (root == NULL) {
          fprintf(stderr, "json_load_file(): %d %s\n", error.line, error.text);
          return 1;
     }
     if (params->field == NULL || strcmp("\0", params->field) == 0) {
          return visit_json(root, 0);
     } else {
          json_t * value = json_object_get(root, params->field);
          return visit_json(value, 0);
     }
}

gchar *
file_base64_encode(gchar * filepath)
{
     gchar * contents;
     gsize len;
     GError * error;
     gchar * enc_contents;

     if (g_file_get_contents(filepath, &contents, &len, &error) == FALSE) {
          fprintf(stderr, "g_file_get_contents(): %s\n", error->message);
          return NULL;
     }
     enc_contents = g_base64_encode((guchar *) contents, len);
     if (enc_contents == NULL) {
          fprintf(stderr, "g_base64_encode(): %s\n", error->message);
          return NULL;
     }
     return enc_contents;
}


int
run_cmd_set(struct params * params)
{
     char * filepath;
     json_error_t error;
     json_t * root;
     json_t * value;

     str_concat(&filepath, params->dir, PATH_DELIM, params->file);
     root = json_load_file(filepath, 0, &error);
     if (root == NULL) {
          fprintf(stderr, "json_load_file(): %d %s\n", error.line, error.text);
          return 1;
     }
     if (!json_is_object(root)) {
          fprintf(stderr, "malformed note\n");
          return 1;
     }
     if (params->from_file) {
          gchar * file_contents = file_base64_encode(params->value);
          value = json_pack("s", file_contents);
     } else {
          value = json_pack("s", params->value);
     }
     json_object_set(root, params->field, value);
     if (json_dump_file(root, filepath, 0) != 0) {
          fprintf(stderr, "json_dump_file(): failed\n");
          return 1;
     }
     return 0;
}


int
run_cmd(struct params * params)
{
     switch (params->cmd) {
     case CMD_cat:
          return run_cmd_cat(params);
     case CMD_set:
          return run_cmd_set(params);
     default:
          fprintf(stderr, "unimplemented command\n");
          return -1;
     }
}


///////////////////////////////////////////////////////////////////////////////


int
main(int argc, char ** argv)
{
     GC_INIT();

     struct params * params;

     params = GC_MALLOC(sizeof(struct params));
     if (params_from_args(params, argc, argv) != 0)
          return 1;
     if (params_validate(params) != 0)
          return 1;

     return run_cmd(params);
}
