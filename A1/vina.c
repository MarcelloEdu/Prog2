#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>  // para stat()
#include "vina.h"
#include "lz.h"  

int load_directory(FILE *archive, EntradaVC **table, int *num_members)
{

}

int save_directory(FILE *archive, EntradaVC **table, int *num_membros)
{
    if (!archive || !table || !*table || !num_membros) {
        fprintf(stderr, "Erro: parâmetros inválidos em save_directory.\n");
        return -1;
    }

    fseek(archive, 0, SEEK_END);//move para o final do arquivo

    long dir_offset = ftell(archive);//marca o offset de onde o diretório comeca

    //escreve todos os membros (tabela da struct)
    size_t written = fwrite(*table, sizeof(EntradaVC), *num_membros, archive);
    if(written != (size_t)*num_membros){
        fprintf(stderr, "erro ao escrever a tabela de diretorio. \n");
        return -1;
    }

    //escreve numero de membros
    fwrite(num_membros, sizeof(int), 1, archive);

    fwrite(&dir_offset, sizeof(long), 1, archive);

    return 0;
}

int insert_plan(const char *archive_name, char **members, int num_members)
{

}

int insert_compressed(const char *archive_name, char **members, int num_members)
{

}

void move_member(const char *archive_name, const char *target, const char *member_to_move)
{

}

int extract(const char *archive_name, char **members, int num_members)
{

}

void remove_members(const char *archive_name, char **members, int num_members)
{

}

void list_content(const char *archive_name)
{

}