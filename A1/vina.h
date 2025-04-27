#ifndef VINA_H
#define VINA_H

#include <time.h>

typedef struct {
    char nome[1025];        // nome do arquivo
    int uid;               // UID do arquivo
    int tamanho_original;
    int tamanho_em_disco; 
    time_t data_modificacao;
    int ordem;
    long offset;           // onde os dados do arquivo estão no .vc
    int is_compressed;        // flag: 0 = plano, 1 = comprimido
} EntradaVC;

// Carrega o diretório do archive (.vc) para a memória (tabela de membros)
int load_directory(FILE *archive, EntradaVC **table, int *num_members);

// Salva o diretório atualizado no final do archive
int save_directory(FILE *archive, EntradaVC **table, int *num_membros);

//ponteiro duplo pois é uma lista de strings
//const char para indicar que o nome do arquivo é apenas para leitura

// Insere arquivos sem compressão no archive (.vc)
int insert_plan(const char *archive_name, char **members, int num_members);

// Insere arquivos comprimidos com LZ (usa plano se compressão não for efetiva)
int insert_compressed(const char *archive_name, char **members, int num_members);

// Move um membro para depois de outro (muda apenas a ordem lógica no diretório)
void move_member(const char *archive_name, const char *target, const char *member_to_move);

// Extrai os arquivos indicados (ou todos se members == NULL)
int extract(const char *archive_name, char **members, int num_members);

// Remove os membros indicados do archive (ajusta diretório)
void remove_members(const char *archive_name, char **members, int num_members);

// Lista as informações dos membros do archive no terminal
void list_content(const char *archive_name);

#endif