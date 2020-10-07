#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <gc.h>
#include <jansson.h>


///////////////////////////////////////////////////////////////////////////////


char PATH_DELIM[2] = "/\0";


///////////////////////////////////////////////////////////////////////////////


struct params_cat {
     char * dir;
     char * file;
     char * field;
};


void
params_cat_fprintf(struct params_cat * params, FILE * f);
int
params_cat_from_args(struct params_cat * params, int argc, char ** argv);
int
params_cat_validate(struct params_cat * params);


//////////////////////////////////////////////////


void
params_cat_fprintf(struct params_cat * params, FILE * f)
{
     if (params == NULL)
          return;
     fprintf(f, "params{dir='%s', file='%s', field='%s'}\n",
             params->dir, params->file, params->field);
}


int
params_cat_from_args(struct params_cat * params, int argc, char ** argv)
{
     assert(params != NULL);
     int opt;
     while ((opt = getopt(argc, argv, ":d:")) != -1)
          switch (opt) {
          case 'd':
               params->dir = strdup(optarg);
               assert(params->dir != NULL);
               break;
          default:
               printf("unknown option '%c'\n", optopt);
               return -1;
          }

     if (optind < argc) {
          params->file = strdup(argv[optind]);
          assert(params->file != NULL);
          optind += 1;
     }
     if (optind < argc) {
          params->field = strdup(argv[optind]);
          assert(params->field != NULL);
     }
     return 0;
}


int
params_cat_validate(struct params_cat * params)
{
     if (params->dir == NULL || strcmp("\0", params->dir) == 0)
          params->dir = strdup(".\0");
     if (params->file == NULL || strcmp("\0", params->file) == 0)
     {
          fprintf(stderr, "missing note\n");
          return -1;
     }
     return 0;
}


///////////////////////////////////////////////////////////////////////////////


size_t
file_slurp(char * * contents, char * filepath)
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
     dst[dstlen-1] = '\0';

     dst_wip = dst;
     for (i=0; i<times; i++)
          dst_wip = stpncpy(dst_wip, s, slen);

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
main(int argc, char ** argv)
{
     GC_INIT();
     //
     struct params_cat * params;
     char * filepath;
     json_error_t error;
     json_t * root;

     params = GC_MALLOC(sizeof(struct params_cat));
     if (params_cat_from_args(params, argc, argv) != 0)
          return 1;
     if (params_cat_validate(params) != 0)
          return 1;

     str_concat(&filepath, params->dir, PATH_DELIM, params->file);
     root = json_load_file(filepath, 0, &error);
     if (root == NULL) {
          fprintf(stderr, "json_load_file(): %d %s\n", error.line, error.text);
          return 1;
     }
     if (params->field == NULL || strcmp("\0", params->field) == 0) {
          visit_json(root, 0);
     } else {
          json_t * value = json_object_get(root, params->field);
          visit_json(value, 0);
     }

     return 0;
}
