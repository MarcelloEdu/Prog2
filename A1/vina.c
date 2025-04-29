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
    if (!archive || !table || !num_membros) {
        fprintf(stderr, "Erro: parâmetros inválidos em save_directory.\n");
        return -1;
    }

    fseek(archive, 0, SEEK_END);
    long dir_offset = ftell(archive);

    if (*num_membros > 0 && *table != NULL) {
        size_t written = fwrite(*table, sizeof(EntradaVC), *num_membros, archive);
        if (written != (size_t)(*num_membros)) {
            fprintf(stderr, "erro ao escrever a tabela de diretório.\n");
            return -1;
        }
    }

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

int insert_compressed(const char *archive_name, char **members, int num_members) {
    FILE *archive = fopen(archive_name, "r+b");
    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (!archive) {
        archive = fopen(archive_name, "w+b");
        if (!archive) {
            perror("Erro ao abrir/criar archive");
            return 1;
        }
    } else {
        if (load_directory(archive, &tabela, &qtd) != 0) {
            tabela = NULL;
            qtd = 0;
        }
        fseek(archive, 0, SEEK_END);
        long final = ftell(archive);
        final -= (qtd * sizeof(EntradaVC)) + sizeof(int) + sizeof(long);
        ftruncate(fileno(archive), final);
    }

    for (int i = 0; i < num_members; i++) {
        const char *nome_membro = members[i];

        FILE *f = fopen(nome_membro, "rb");
        if (!f) {
            fprintf(stderr, "Erro ao abrir membro %s. Pulando.\n", nome_membro);
            continue;
        }

        fseek(f, 0, SEEK_END);
        long size_original_long = ftell(f);
        rewind(f);

        if (size_original_long == 0) {
            fprintf(stderr, "Arquivo %s vazio. Pulando.\n", nome_membro);
            fclose(f);
            continue;
        }

        unsigned int size_original = (unsigned int)size_original_long;

        unsigned char *buffer_original = malloc(size_original);
        unsigned char *buffer_comprimido = malloc(size_original * 2);
        unsigned int *work = malloc((65536 + size_original) * sizeof(unsigned int));

        if (!buffer_original || !buffer_comprimido || !work) {
            fprintf(stderr, "Erro de memória para arquivo %s.\n", nome_membro);
            if (buffer_original) free(buffer_original);
            if (buffer_comprimido) free(buffer_comprimido);
            if (work) free(work);
            fclose(f);
            fclose(archive);
            return 1;
        }

        fread(buffer_original, 1, size_original, f);
        fclose(f);

        int tamanho_comprimido = LZ_CompressFast(buffer_original, buffer_comprimido, size_original, work);
        free(work); // Área de trabalho não é mais necessária

        fseek(archive, 0, SEEK_END);
        long offset = ftell(archive);

        EntradaVC entrada;
        strncpy(entrada.nome, nome_membro, sizeof(entrada.nome) - 1);
        entrada.nome[sizeof(entrada.nome) - 1] = '\0';
        entrada.uid = qtd + 1;
        struct stat s;
        if (stat(nome_membro, &s) != 0) {
            perror("Erro no stat");
            free(buffer_original);
            free(buffer_comprimido);
            fclose(archive);
            return 1;
        }
        entrada.data_modificacao = s.st_mtime;
        entrada.ordem = qtd;
        entrada.offset = offset;

        if (tamanho_comprimido > 0 && tamanho_comprimido < (int)size_original) {
            fwrite(buffer_comprimido, 1, tamanho_comprimido, archive);
            entrada.is_compressed = 1;
            entrada.tamanho_original = size_original;
            entrada.tamanho_em_disco = tamanho_comprimido;
            printf("Arquivo %s armazenado comprimido (%d bytes).\n", nome_membro, tamanho_comprimido);
        } else {
            fwrite(buffer_original, 1, size_original, archive);
            entrada.is_compressed = 0;
            entrada.tamanho_original = size_original;
            entrada.tamanho_em_disco = size_original;
            printf("Compressão ineficaz, armazenando %s sem compressão (%u bytes).\n", nome_membro, size_original);
        }

        free(buffer_original);
        free(buffer_comprimido);

        tabela = realloc(tabela, (qtd + 1) * sizeof(EntradaVC));
        if (!tabela) {
            fprintf(stderr, "Erro de memória ao expandir tabela.\n");
            fclose(archive);
            return 1;
        }
        tabela[qtd++] = entrada;
    }

    // Agora sim: salvar o diretório de uma vez só
    save_directory(archive, &tabela, &qtd);
    fclose(archive);
    free(tabela);

    return 0;
}
void move_member(const char *archive_name, const char *destino, const char *alvo) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Erro ao abrir arquivador para mover");
        return;
    }

    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (load_directory(archive, &tabela, &qtd) != 0) {
        fprintf(stderr, "Erro ao carregar diretório.\n");
        fclose(archive);
        return;
    }

    int idx_destino = encontrar_membro(tabela, qtd, destino);
    int idx_alvo = encontrar_membro(tabela, qtd, alvo);

    if (idx_destino == -1 || idx_alvo == -1) {
        fprintf(stderr, "Membro não encontrado: %s ou %s\n", destino, alvo);
        free(tabela);
        fclose(archive);
        return;
    }

    // Extrai o membro alvo
    EntradaVC movido = tabela[idx_alvo];

    // Remove o alvo da tabela
    for (int i = idx_alvo; i < qtd - 1; i++) {
        tabela[i] = tabela[i + 1];
    }
    qtd--;

    // Reinsere o alvo antes do destino
    EntradaVC *nova_tabela = malloc((qtd + 1) * sizeof(EntradaVC));
    int j = 0;
    for (int i = 0; i < qtd + 1; i++) {
        if (i == idx_destino) {
            nova_tabela[i] = movido;
        } else {
            nova_tabela[i] = tabela[j++];
        }
    }

    // Atualiza campo .ordem e copia os dados reais
    FILE *novo = fopen("temp.vc", "w+b");
    if (!novo) {
        perror("Erro ao criar novo arquivo");
        free(nova_tabela);
        free(tabela);
        fclose(archive);
        return;
    }

    for (int i = 0; i < qtd + 1; i++) {
        EntradaVC entrada = nova_tabela[i];
        entrada.ordem = i;

        unsigned char *buffer = malloc(entrada.tamanho_em_disco);
        fseek(archive, entrada.offset, SEEK_SET);
        fread(buffer, 1, entrada.tamanho_em_disco, archive);

        fseek(novo, 0, SEEK_END);
        long novo_offset = ftell(novo);
        fwrite(buffer, 1, entrada.tamanho_em_disco, novo);
        free(buffer);

        entrada.offset = novo_offset;
        nova_tabela[i] = entrada;
    }

    save_directory(novo, &nova_tabela, &(int){qtd + 1});
    fclose(novo);
    fclose(archive);
    free(tabela);
    free(nova_tabela);

    remove(archive_name);
    rename("temp.vc", archive_name);

    printf("Membro '%s' movido antes de '%s' com sucesso.\n", alvo, destino);
}

void extract(const char *archive_name, char **members, int num_members) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Erro ao abrir arquivador para extração");
        return;
    }

    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (load_directory(archive, &tabela, &qtd) != 0) {
        fprintf(stderr, "Erro ao carregar diretório.\n");
        fclose(archive);
        return;
    }

    for (int i = 0; i < num_members; i++) {
        const char *nome = members[i];
        int idx = encontrar_membro(tabela, qtd, nome);

        if (idx == -1) {
            fprintf(stderr, "Arquivo '%s' não encontrado no arquivador.\n", nome);
            continue;
        }

        EntradaVC entrada = tabela[idx];

        // Lê os dados do arquivo no .vc
        unsigned char *buffer = malloc(entrada.tamanho_em_disco);
        if (!buffer) {
            fprintf(stderr, "Erro de memória para %s.\n", nome);
            continue;
        }

        fseek(archive, entrada.offset, SEEK_SET);
        fread(buffer, 1, entrada.tamanho_em_disco, archive);

        // Cria o arquivo extraído no diretório atual
        FILE *out = fopen(entrada.nome, "wb");
        if (!out) {
            perror("Erro ao criar arquivo extraído");
            free(buffer);
            continue;
        }

        if (entrada.is_compressed) {
            // Descomprime usando LZ_Uncompress
            unsigned char *descomprimido = malloc(entrada.tamanho_original);
            if (!descomprimido) {
                fprintf(stderr, "Erro de memória ao descomprimir %s.\n", nome);
                fclose(out);
                free(buffer);
                continue;
            }

            LZ_Uncompress(buffer, descomprimido, entrada.tamanho_em_disco);
            fwrite(descomprimido, 1, entrada.tamanho_original, out);
            free(descomprimido);
        } else {
            fwrite(buffer, 1, entrada.tamanho_em_disco, out);
        }

        fclose(out);
        free(buffer);
        printf("Arquivo '%s' extraído com sucesso.\n", nome);
    }

    fclose(archive);
    free(tabela);
}

void remove_members(const char *archive_name, char **members, int num_members) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Erro ao abrir arquivador para remoção");
        return;
    }

    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (load_directory(archive, &tabela, &qtd) != 0) {
        fprintf(stderr, "Erro ao carregar diretório.\n");
        fclose(archive);
        return;
    }

    // Marca quais arquivos devem ser mantidos
    int *manter = calloc(qtd, sizeof(int));
    for (int i = 0; i < qtd; i++) {
        manter[i] = 1; // manter por padrão
        for (int j = 0; j < num_members; j++) {
            if (strcmp(tabela[i].nome, members[j]) == 0) {
                manter[i] = 0; // marcar para remoção
                break;
            }
        }
    }

    // Cria novo arquivo temporário
    FILE *novo = fopen("temp.vc", "w+b");
    if (!novo) {
        perror("Erro ao criar arquivo temporário");
        fclose(archive);
        free(tabela);
        free(manter);
        return;
    }

    EntradaVC *nova_tabela = NULL;
    int nova_qtd = 0;

    for (int i = 0; i < qtd; i++) {
        if (!manter[i]) continue;

        EntradaVC entrada = tabela[i];
        unsigned char *buffer = malloc(entrada.tamanho_em_disco);
        fseek(archive, entrada.offset, SEEK_SET);
        fread(buffer, 1, entrada.tamanho_em_disco, archive);

        fseek(novo, 0, SEEK_END);
        long novo_offset = ftell(novo);
        fwrite(buffer, 1, entrada.tamanho_em_disco, novo);
        free(buffer);

        entrada.offset = novo_offset;
        entrada.ordem = nova_qtd;

        nova_tabela = realloc(nova_tabela, (nova_qtd + 1) * sizeof(EntradaVC));
        nova_tabela[nova_qtd++] = entrada;
    }

    save_directory(novo, &nova_tabela, &nova_qtd);
    fclose(archive);
    fclose(novo);
    free(tabela);
    free(nova_tabela);
    free(manter);

    // Substitui o original pelo novo
    remove(archive_name);
    rename("temp.vc", archive_name);

    printf("Remoção concluída.\n");
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