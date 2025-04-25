#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>  // para stat()
#include "vina.h"
#include "lz.h"  

int load_directory(FILE *archive, EntradaVC **table, int *num_members)
{
    if (!archive || !table || !num_members) {
        fprintf(stderr, "Erro: parâmetros inválidos em load_directory.\n");
        return -1;
    }

    // Vai até o final para ler os metadados
    fseek(archive, -((long)sizeof(int) + sizeof(long)), SEEK_END);

    // Lê o número de membros
    int qtd;
    fread(&qtd, sizeof(int), 1, archive);

    // Lê o offset do início do diretório
    long offset;
    fread(&offset, sizeof(long), 1, archive);

    // Vai até o início do diretório
    fseek(archive, offset, SEEK_SET);

    // Aloca a tabela e lê os membros
    EntradaVC *tabela = malloc(qtd * sizeof(EntradaVC));
    if (!tabela) {
        perror("Erro ao alocar memória para tabela");
        return -1;
    }

    size_t lidos = fread(tabela, sizeof(EntradaVC), qtd, archive);
    if (lidos != (size_t)qtd) {
        fprintf(stderr, "Erro ao ler tabela do diretório.\n");
        free(tabela);
        return -1;
    }

    *table = tabela;
    *num_members = qtd;
    return 0;

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