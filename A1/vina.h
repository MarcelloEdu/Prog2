#include <stdio.h>
#include <stdlib.h>
#include "lz.h"

typedef struct {
    char nome[1024];        // nome do arquivo
    int uid;               // UID do arquivo
    int tamanho_original;
    int tamanho_em_disco; 
    time_t data_modificacao;
    int ordem;
    long offset;           // onde os dados do arquivo estão no .vc
    int is_compressed;        // flag: 0 = plano, 1 = comprimido
} EntradaVC;

//ponteiro duplo pois é uma lista de strings
//const char para indicar que o nome do arquivo é apenas para leitura
int insert_plan(const char *archive_name, char **members, int num_members);

int insert_compressed(const char *archive_name, char **members, int num_members);

void move_member(const char *archive_name, const char *target, const char *member_to_move);

int extract(const char *archive_name, char **members, int num_members);

void remove(const char *archive_name, char **members, int num_members);

void list_content(const char *archive_name);