#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h> 
#include <unistd.h>
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

int encontrar_membro(EntradaVC *tabela, int num, const char *nome) {
    for (int i = 0; i < num; i++) {
        if (strcmp(tabela[i].nome, nome) == 0) {
            return i;
        }
    }
    return -1;
}

int insert_plan(const char *archive_name, char **members, int num_members)
{
    // 1. Abre ou cria o archive
    FILE *archive = fopen(archive_name, "r+b");
    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (!archive) {
        // Se o arquivo ainda não existe, criamos um novo vazio
        archive = fopen(archive_name, "w+b");
        if (!archive) {
            perror("Erro ao abrir/ criar archive");
            return 1;
        }
    } else {
        // Se já existir, tenta carregar o diretório
        load_directory(archive, &tabela, &qtd);
        // Trunca o final antigo (remoção do diretório anterior)
        fseek(archive, 0, SEEK_END);
        long final = ftell(archive);
        final -= (qtd * sizeof(EntradaVC)) + sizeof(int) + sizeof(long);
        ftruncate(fileno(archive), final);
    }

    // 2. Para cada membro, insere no final do arquivo
    for (int i = 0; i < num_members; i++) {
        const char *nome_membro = members[i];

        FILE *f = fopen(nome_membro, "rb");
        if (!f) {
            perror("Erro ao abrir membro");
            continue;
        }

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);

        unsigned char *buffer = malloc(size);
        fread(buffer, 1, size, f);
        fclose(f);

        // Pega metadata do membro
        struct stat s;
        stat(nome_membro, &s);

        // Vai até o fim do archive atual
        fseek(archive, 0, SEEK_END);
        long offset = ftell(archive);
        fwrite(buffer, 1, size, archive);
        free(buffer);

        // Atualiza ou adiciona no diretório
        int idx = encontrar_membro(tabela, qtd, nome_membro);
        if (idx == -1) {
            // Novo membro
            tabela = realloc(tabela, (qtd + 1) * sizeof(EntradaVC));
            idx = qtd++;
        }

        tabela[idx] = (EntradaVC){
            .uid = idx + 1,
            .tamanho_original = size,
            .tamanho_em_disco = size,
            .data_modificacao = s.st_mtime,
            .ordem = idx,
            .offset = offset,
            .is_compressed = 0
        };
        strncpy(tabela[idx].nome, nome_membro, sizeof(tabela[idx].nome) - 1);
    }

    // 3. Salva diretório atualizado
    save_directory(archive, &tabela, &qtd);
    fclose(archive);
    free(tabela);

    return 0;
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
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Erro ao abrir o archive para listar conteúdo");
        return;
    }

    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (load_directory(archive, &tabela, &qtd) != 0) {
        fprintf(stderr, "Erro ao carregar diretório.\n");
        fclose(archive);
        return;
    }

    printf("Conteúdo do archive '%s':\n", archive_name);
    printf("--------------------------------------------------\n");
    for (int i = 0; i < qtd; i++) {
        printf("Membro %d:\n", i + 1);
        printf(" Nome: %s\n", tabela[i].nome);
        printf(" UID: %d\n", tabela[i].uid);
        printf(" Tamanho original: %d bytes\n", tabela[i].tamanho_original);
        printf(" Tamanho em disco: %d bytes\n", tabela[i].tamanho_em_disco);
        printf(" Data de modificação: %ld\n", tabela[i].data_modificacao); // depois podemos formatar essa data
        printf(" Ordem: %d\n", tabela[i].ordem);
        printf(" Offset: %ld\n", tabela[i].offset);
        printf(" Comprimido: %s\n", tabela[i].is_compressed ? "Sim" : "Não");
        printf("--------------------------------------------------\n");
    }

    free(tabela);
    fclose(archive);
}