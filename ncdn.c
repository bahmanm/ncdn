#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <gc.h>


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


int
main(int argc, char ** argv)
{
     GC_INIT();
     struct params_cat * params = GC_MALLOC(sizeof(struct params_cat));
     if (params_cat_from_args(params, argc, argv) != 0)
          return 1;
     if (params_cat_validate(params) != 0)
          return 1;
     params_cat_fprintf(params, stderr);
     return 0;
}
