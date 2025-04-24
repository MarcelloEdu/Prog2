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

int insert_plan();

int insert_compressed();

void move_member();

int extract();

void remove();

void list_content();