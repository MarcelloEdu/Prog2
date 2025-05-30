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
    if (!archive || !table || !num_members)
    {
        fprintf(stderr, "Erro: parâmetros inválidos em load_directory.\n");
        return -1;
    }

    //vai até o final do arquivo menos os últimos 12 bytes (int + long)
    //é aqui que estão salvos: [ qtd | offset ]
    fseek(archive, -((long)sizeof(int) + sizeof(long)), SEEK_END);

    //lê o número de membros
    int qtd;
    fread(&qtd, sizeof(int), 1, archive);

    //lê o offset do início do diretório
    long offset;
    fread(&offset, sizeof(long), 1, archive);

    //vai até o início do diretório
    fseek(archive, offset, SEEK_SET);

    //aloca a tabela e lê os membros
    EntradaVC *tabela = malloc(qtd * sizeof(EntradaVC));
    if (!tabela)
    {
        perror("Erro ao alocar memória para tabela");
        return -1;
    }

    //lê o diretorio inteiro, onde cada entrada representa um membro
    size_t lidos = fread(tabela, sizeof(EntradaVC), qtd, archive);
    if (lidos != (size_t)qtd)
    {
        fprintf(stderr, "Erro ao ler tabela do diretório.\n");
        free(tabela);
        return -1;
    }

    //retorna os dados pra quem chamou
    *table = tabela;
    *num_members = qtd;
    return 0;

}

int save_directory(FILE *archive, EntradaVC **table, int *num_membros)
{
    if (!archive || !table || !num_membros)
    {
        fprintf(stderr, "Erro: parâmetros inválidos em save_directory.\n");
        return -1;
    }

    //move o cursor para o fim do arquivo
    fseek(archive, 0, SEEK_END);

    //marca a posição atual do cursor
    //byte onde começa o diretório
    long dir_offset = ftell(archive);

    //se há membros a salvar, grava a tabela no arquivo
    if (*num_membros > 0 && *table != NULL)
    {
        size_t written = fwrite(*table, sizeof(EntradaVC), *num_membros, archive);
        
        //confere se deu certo a gravação
        if (written != (size_t)(*num_membros))
        {
            fprintf(stderr, "erro ao escrever a tabela de diretório.\n");
            return -1;
        }
    }

    fwrite(num_membros, sizeof(int), 1, archive);//grava numero de membros
    fwrite(&dir_offset, sizeof(long), 1, archive);//grava o offset do diretório

    return 0;
}

int encontrar_membro(EntradaVC *tabela, int num, const char *nome) 
{
    for (int i = 0; i < num; i++)
    {
        //compara o nome do membro com o nome procurado e retorna o indice
        if (strcmp(tabela[i].nome, nome) == 0)
        {
            return i;
        }
    }
    return -1;
}

int insert_plan(const char *archive_name, char **members, int num_members)
{
    //abre ou cria o archive
    FILE *archive = fopen(archive_name, "r+b");
    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (!archive)
    {
        //se o arquivo ainda não existe, criamos um novo vazio
        archive = fopen(archive_name, "w+b");
        if (!archive)
        {
            perror("Erro ao abrir/criar archive");
            return 1;
        }
    } else
    {
        //se já existir, tenta carregar o diretório
        load_directory(archive, &tabela, &qtd);

        //precisamos truncar (cortar) o final do arquivo, removendo o diretório antigo
        //isso evita ter duplicatas do diretório no fim
        fseek(archive, 0, SEEK_END);
        long final = ftell(archive);

        //subtrai o tamanho total do diretório atual (tabela + qtd + offset)
        final -= (qtd * sizeof(EntradaVC)) + sizeof(int) + sizeof(long);
        ftruncate(fileno(archive), final);
    }

    // 2. para cada membro, insere no final do arquivo
    for (int i = 0; i < num_members; i++)
    {
        const char *nome_membro = members[i];

        //abre o arquivo original
        FILE *f = fopen(nome_membro, "rb");
        if (!f)
        {
            perror("Erro ao abrir membro");
            continue;//pula pro proximo
        }

        //determina o tamanho do arquivo
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);

        //aloca memoria para armazenar o conteudo
        unsigned char *buffer = malloc(size);
        fread(buffer, 1, size, f);
        fclose(f);

        //pega os metadados do membro
        struct stat s;
        stat(nome_membro, &s);

        //vai até o fim do archive atual
        fseek(archive, 0, SEEK_END);
        long offset = ftell(archive); //posicao onde o conteudo começa
        fwrite(buffer, 1, size, archive);
        free(buffer);

        //verifica se já existe o membro
        int idx = encontrar_membro(tabela, qtd, nome_membro);
        if (idx == -1)
        {
            //novo membro, então precisamos alocar mais espaço
            tabela = realloc(tabela, (qtd + 1) * sizeof(EntradaVC));
            idx = qtd++;
        }

        //preenche os dados do novo membro
        tabela[idx] = (EntradaVC){
            .uid = idx + 1,
            .tamanho_original = size,
            .tamanho_em_disco = size,
            .data_modificacao = s.st_mtime,
            .ordem = idx,
            .offset = offset,
            .is_compressed = 0
        };

        //copia o nome do membro
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

    //abre o arquivo 
    FILE *archive = fopen(archive_name, "r+b");
    EntradaVC *tabela = NULL;
    int qtd = 0;

    //se o arquivo não existe, cria um novo
    if (!archive)
    {
        archive = fopen(archive_name, "w+b");
        if (!archive)
        {
            perror("Erro ao abrir/criar archive");
            return 1;
        }
    } else
    {
        //se existe carrega o diretório
        if (load_directory(archive, &tabela, &qtd) != 0)
        {
            tabela = NULL;
            qtd = 0;
        }

        //corta o final do arquivo, removendo o diretório antigo
        fseek(archive, 0, SEEK_END);
        long final = ftell(archive);
        final -= (qtd * sizeof(EntradaVC)) + sizeof(int) + sizeof(long);
        ftruncate(fileno(archive), final);
    }

    // 2. Para cada membro, insere no final do arquivo
    for (int i = 0; i < num_members; i++)
    {
        const char *nome_membro = members[i];

        //abre o arquivo original
        FILE *f = fopen(nome_membro, "rb");
        if (!f)
        {
            fprintf(stderr, "Erro ao abrir membro %s. Pulando.\n", nome_membro);
            continue;
        }

        //calcula o tamanho do arquivo
        fseek(f, 0, SEEK_END);
        long size_original_long = ftell(f);
        rewind(f);

        //se estiver vazio, pula
        if (size_original_long == 0)
        {
            fprintf(stderr, "Arquivo %s vazio. Pulando.\n", nome_membro);
            fclose(f);
            continue;
        }

        unsigned int size_original = (unsigned int)size_original_long;

        //aloca buffers:
        // - original: conteúdo do arquivo original
        // - comprimido: buffer para armazenar os dados comprimidos
        // - work: memória auxiliar usada pelo algoritmo de compressão
        unsigned char *buffer_original = malloc(size_original);
        unsigned char *buffer_comprimido = malloc(size_original * 2);

        /*
        * aloca o work utilizado pelo LZ_CompressFast.
        * 
        * o algoritmo precisa de dois vetores auxiliares:
        * 
        * 1. lastindex[65536] - usado para armazenar a última posição conhecida de cada par de bytes 
        *
        * 2. jumptable[size_original] - usado como uma tabela de saltos para otimizar a compressão,
        *0
        *     total = 65536 + size_original
        * 
        * em resumo, alocando o tamanho total de 65536 + size_original de elementos do tipo unsigned int.
        */
        unsigned int *work = malloc((65536 + size_original) * sizeof(unsigned int));

        if (!buffer_original || !buffer_comprimido || !work)
        {
            fprintf(stderr, "Erro de memória para arquivo %s.\n", nome_membro);
            if (buffer_original) free(buffer_original);
            if (buffer_comprimido) free(buffer_comprimido);
            if (work) free(work);
            fclose(f);
            fclose(archive);
            return 1;
        }

        //le o arquivo original
        fread(buffer_original, 1, size_original, f);
        fclose(f);
        
        //comprime o buffer original
        int tamanho_comprimido = LZ_CompressFast(buffer_original, buffer_comprimido, size_original, work);
        free(work);

        //prepara os dados para salvar
        fseek(archive, 0, SEEK_END);
        long offset = ftell(archive);

        EntradaVC entrada;
        strncpy(entrada.nome, nome_membro, sizeof(entrada.nome) - 1);
        entrada.nome[sizeof(entrada.nome) - 1] = '\0';
        entrada.uid = qtd + 1;

        struct stat s;
        if (stat(nome_membro, &s) != 0)
        {
            perror("Erro no stat");
            free(buffer_original);
            free(buffer_comprimido);
            fclose(archive);
            return 1;
        }

        entrada.data_modificacao = s.st_mtime;
        entrada.ordem = qtd;
        entrada.offset = offset;

        //verifica se a compressão foi eficaz
        if (tamanho_comprimido > 0 && tamanho_comprimido < (int)size_original)
        {
            //grava a versão comprimida
            fwrite(buffer_comprimido, 1, tamanho_comprimido, archive);
            entrada.is_compressed = 1;
            entrada.tamanho_original = size_original;
            entrada.tamanho_em_disco = tamanho_comprimido;
            printf("Arquivo %s armazenado comprimido (%d bytes).\n", nome_membro, tamanho_comprimido);
        } else
        {
            //grava a versão original
            fwrite(buffer_original, 1, size_original, archive);
            entrada.is_compressed = 0;
            entrada.tamanho_original = size_original;
            entrada.tamanho_em_disco = size_original;
            printf("Compressão ineficaz, armazenando %s sem compressão (%u bytes).\n", nome_membro, size_original);
        }

        free(buffer_original);
        free(buffer_comprimido);

        int idx = encontrar_membro(tabela, qtd, nome_membro);
        if (idx != -1) {
            //já existe — verificar se está comprimido
            if (tabela[idx].is_compressed) {
                printf("Membro '%s' já existe no diretório (comprimido). Substituindo...\n", nome_membro);
                tabela[idx] = entrada;
            } else {
                char resposta[4];
                printf("Membro '%s' já existe no diretório em modo plano. Substituir por versão comprimida? (s/N): ", nome_membro);
                fgets(resposta, sizeof(resposta), stdin);
                if (resposta[0] != 's' && resposta[0] != 'S') {
                    printf("Substituição cancelada para '%s'.\n", nome_membro);
                    continue;
                }
                tabela[idx] = entrada;
            }
        } else {
            //mmbro novo
            tabela = realloc(tabela, (qtd + 1) * sizeof(EntradaVC));
            if (!tabela) {
                fprintf(stderr, "Erro de memória ao expandir tabela.\n");
                fclose(archive);
                return 1;
            }
        tabela[qtd] = entrada;
        qtd++;
    }
    //salvar o diretório de uma vez só
    save_directory(archive, &tabela, &qtd);
    fclose(archive);
    free(tabela);

    return 0;
    }
    return 0;
}

void move_member(const char *archive_name, const char *destino, const char *alvo) 
{
    if (!alvo) {
        fprintf(stderr, "Erro: membro a mover não pode ser NULL.\n");
        return;
    }

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

    int idx_alvo = encontrar_membro(tabela, qtd, alvo);
    if (idx_alvo == -1) {
        fprintf(stderr, "Membro '%s' não encontrado.\n", alvo);
        free(tabela);
        fclose(archive);
        return;
    }

    //se destino for NULL, move para o início (posição 0)
    // int idx_destino = 0;
    // if (destino != NULL) {
        // idx_destino = encontrar_membro(tabela, qtd, destino);
        // if (idx_destino == -1) {
            // fprintf(stderr, "Destino '%s' não encontrado.\n", destino);
            // free(tabela);
            // fclose(archive);
            // return;
        // }
// 
        //se o alvo está antes do destino, após removê-lo, o índice do destino muda
        // if (idx_alvo < idx_destino) {
            // idx_destino--;
        // }
    // }

    EntradaVC movido = tabela[idx_alvo];
    EntradaVC *nova_tabela = malloc(qtd * sizeof(EntradaVC));
    if (!nova_tabela) {
        perror("Erro ao alocar memória para nova_tabela");
        free(tabela);
        fclose(archive);
        return;
    }
    
    int j = 0;

    /*
    
    Versão com erro de lógica, estava inserindo antes do
    alvo, quando deveria inserir depois do destino.

    for (int i = 0; i < qtd; i++) {
        if (i == idx_destino) {
            nova_tabela[j++] = movido; //insere o movido na posição certa
        }

        if (i != idx_alvo) {
            nova_tabela[j++] = tabela[i]; //copia os outros membros (exceto o movido)
        }
    }

    FILE *novo = fopen("temp.vc", "w+b");
    if (!novo) {
        perror("Erro ao criar novo arquivo temporário");
        free(nova_tabela);
        free(tabela);
        fclose(archive);
        return;
    }

    //regrava dados dos membros e atualiza offsets e ordem
    for (int i = 0; i < qtd; i++) {
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

    save_directory(novo, &nova_tabela, &qtd);
    fclose(novo);
    fclose(archive);
    free(tabela);
    free(nova_tabela);

    remove(archive_name);
    rename("temp.vc", archive_name);

    printf("Membro '%s' movido com sucesso.\n", alvo);
*/

    if(destino == NULL){
        nova_tabela[j++] = movido; // insere o movido na posição certa
        for (int i = 0; i < qtd; i++) {
            if (i != idx_alvo) {
                nova_tabela[j++] = tabela[i]; // copia os outros membros (exceto o movido)
            }
        }
    }else{
        int idx_target_og = encontrar_membro(tabela, qtd, destino);
        if (idx_target_og == -1) {
            fprintf(stderr, "Destino '%s' não encontrado.\n", destino);
            free(tabela);
            fclose(archive);
            free(nova_tabela);
            return;
        }   

        if(idx_alvo == idx_target_og){
            //apenas copia a tabela original
            for (int i = 0; i < qtd; i++) {
                nova_tabela[j++] = tabela[i]; // copia todos os membros
            }
            j=qtd;
        }else {
            for(int i = 0; i < qtd; i++) {
                if (i == idx_alvo){
                    continue; // pula o alvo
                }
                nova_tabela[j++] = tabela[i]; // copia os outros membros (exceto o movido)
                if(i== idx_target_og){
                    nova_tabela[j++] = movido; // insere o movido na posição certa
                }
            }
        }
    }

    if(j != qtd) {
        fprintf(stderr, "Erro: número de membros na nova tabela não corresponde ao original.\n");
        free(nova_tabela);
        free(tabela);
        fclose(archive);
        return;
    }

    FILE *novo = fopen("temp.vc", "w+b");
    if (!novo) {
        perror("Erro ao criar novo arquivo temporário");
        free(nova_tabela);
        free(tabela);
        fclose(archive);
        return;
    }
    // regrava dados dos membros e atualiza offsets e ordem
 for (int i = 0; i < qtd; i++) {
        
    EntradaVC *entrada_atual_nova_tabela = &nova_tabela[i]; // Ponteiro para modificar a entrada na nova_tabela
    unsigned char *buffer_dados_membro = malloc(entrada_atual_nova_tabela->tamanho_em_disco);
        if (!buffer_dados_membro) {
            perror("Erro ao alocar buffer para dados do membro durante a movimentação");
            fclose(novo);
            remove("temp.vc");
            free(tabela);
            free(nova_tabela);
            fclose(archive);
            return;
        }

        fseek(archive, entrada_atual_nova_tabela->offset, SEEK_SET); // Usa o offset antigo para ler do archive original
        if (fread(buffer_dados_membro, 1, (size_t)entrada_atual_nova_tabela->tamanho_em_disco, archive) != (size_t)entrada_atual_nova_tabela->tamanho_em_disco) {
            fprintf(stderr, "Erro ao ler dados do membro %s do archive original.\n", entrada_atual_nova_tabela->nome);
            free(buffer_dados_membro);
            fclose(novo);
            remove("temp.vc");
            free(tabela);
            free(nova_tabela);
            fclose(archive);
            return;
        }

        fseek(novo, 0, SEEK_END);
        long novo_offset = ftell(novo);
         if (fwrite(buffer_dados_membro, 1, (size_t)entrada_atual_nova_tabela->tamanho_em_disco, novo) != (size_t)entrada_atual_nova_tabela->tamanho_em_disco) {
            fprintf(stderr, "Erro ao escrever dados do membro %s no arquivo temporário.\n", entrada_atual_nova_tabela->nome);
            free(buffer_dados_membro);
            fclose(novo);
            remove("temp.vc");
            free(tabela);
            free(nova_tabela);
            fclose(archive);
            return;
        }
        free(buffer_dados_membro);

        // Atualiza o offset e a ordem na entrada da nova_tabela
        entrada_atual_nova_tabela->offset = novo_offset; // O novo offset é onde os dados foram escritos no arquivo temporário
        entrada_atual_nova_tabela->ordem = i; // A ordem é o índice na nova_tabela
    }

    // Salva o diretório (nova_tabela) no final do arquivo temporário
    save_directory(novo, &nova_tabela, &qtd);

    // Fecha os arquivos
    fclose(archive); // Fecha o original
    fclose(novo); // Fecha o temporário preenchido

    // Libera as tabelas de diretório da memória
    free(tabela);
    free(nova_tabela);

    // Substitui o arquivo original pelo novo arquivo temporário
    if (remove(archive_name) != 0) {
        perror("Erro ao remover o arquivo original após mover membro");
        // O arquivo temp.vc ainda existe, mas o processo falhou.
        return;
    }
    if (rename("temp.vc", archive_name) != 0) {
        perror("Erro ao renomear o arquivo temporário após mover membro");
        // Isso é problemático, pois o original foi removido.
        // Poderia tentar renomear de volta se possível, ou informar o usuário.
        return;
    }

    printf("Membro '%s' movido com sucesso.\n", alvo);
}

int extract(const char *archive_name, char **members, int num_members) 
{
    FILE *archive = fopen(archive_name, "rb");
    if (!archive)
    {
        perror("Erro ao abrir arquivador para extração");
        return 1;
    }

    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (load_directory(archive, &tabela, &qtd) != 0)
    {
        fprintf(stderr, "Erro ao carregar diretório.\n");
        fclose(archive);
        return 1;
    }

    //procura o membro a ser extraído
    for (int i = 0; i < num_members; i++)
    {
        const char *nome = members[i];
        int idx = encontrar_membro(tabela, qtd, nome);

        if (idx == -1)
        {
            fprintf(stderr, "Arquivo '%s' não encontrado no arquivador.\n", nome);
            continue;
        }

        EntradaVC entrada = tabela[idx];

        //lê os dados do arquivo no .vc
        unsigned char *buffer = malloc(entrada.tamanho_em_disco);
        if (!buffer)
        {
            fprintf(stderr, "Erro de memória para %s.\n", nome);
            continue;
        }

        //vai ate a posição do conteudo e le os dados
        fseek(archive, entrada.offset, SEEK_SET);
        fread(buffer, 1, entrada.tamanho_em_disco, archive);

        //cria o arquivo extraído no diretório atual
        FILE *out = fopen(entrada.nome, "wb");
        if (!out)
        {
            perror("Erro ao criar arquivo extraído");
            free(buffer);
            continue;
        }

        if (entrada.is_compressed)
        {
            //descomprime
            unsigned char *descomprimido = malloc(entrada.tamanho_original);
            if (!descomprimido)
            {
                fprintf(stderr, "Erro de memória ao descomprimir %s.\n", nome);
                fclose(out);
                free(buffer);
                continue;
            }

            LZ_Uncompress(buffer, descomprimido, entrada.tamanho_em_disco);
            fwrite(descomprimido, 1, entrada.tamanho_original, out);
            free(descomprimido);
        } else
        {
            fwrite(buffer, 1, entrada.tamanho_em_disco, out);
        }

        fclose(out);
        free(buffer);
        printf("Arquivo '%s' extraído com sucesso.\n", nome);
    }

    fclose(archive);
    free(tabela);
    return 0;
}

void remove_members(const char *archive_name, char **members, int num_members) 
{
    FILE *archive = fopen(archive_name, "rb");
    if (!archive)
    {
        perror("Erro ao abrir arquivador para remoção");
        return;
    }

    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (load_directory(archive, &tabela, &qtd) != 0)
    {
        fprintf(stderr, "Erro ao carregar diretório.\n");
        fclose(archive);
        return;
    }

    // marca quais arquivos devem ser mantidos
    int *manter = calloc(qtd, sizeof(int));
    for (int i = 0; i < qtd; i++)
    {
        manter[i] = 1; // manter por padrão
        for (int j = 0; j < num_members; j++)
        {
            if (strcmp(tabela[i].nome, members[j]) == 0)
            {
                manter[i] = 0; // marcar para remoção
                break;
            }
        }
    }

    //cria novo arquivo temporário
    FILE *novo = fopen("temp.vc", "w+b");
    if (!novo)
    {
        perror("Erro ao criar arquivo temporário");
        fclose(archive);
        free(tabela);
        free(manter);
        return;
    }

    EntradaVC *nova_tabela = NULL;
    int nova_qtd = 0;

    for (int i = 0; i < qtd; i++)
    {
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

    //substitui o original pelo novo
    remove(archive_name);
    rename("temp.vc", archive_name);

    printf("Remoção concluída.\n");
}

void list_content(const char *archive_name)
{
    FILE *archive = fopen(archive_name, "rb");
    if (!archive)
    {
        perror("Erro ao abrir o archive para listar conteúdo");
        return;
    }

    EntradaVC *tabela = NULL;
    int qtd = 0;

    if (load_directory(archive, &tabela, &qtd) != 0)
    {
        fprintf(stderr, "Erro ao carregar diretório.\n");
        fclose(archive);
        return;
    }

    printf("Conteúdo do archive '%s':\n", archive_name);
    printf("--------------------------------------------------\n");
    for (int i = 0; i < qtd; i++)
    {
        char data_str[64];
        struct tm *tm_info = localtime(&tabela[i].data_modificacao);
        strftime(data_str, sizeof(data_str), "%Y-%m-%d %H:%M:%S", tm_info);

        printf("Membro %d:\n", i + 1);
        printf(" Nome: %s\n", tabela[i].nome);
        printf(" UID: %d\n", tabela[i].uid);
        printf(" Tamanho original: %d bytes\n", tabela[i].tamanho_original);
        printf(" Tamanho em disco: %d bytes\n", tabela[i].tamanho_em_disco);
        printf(" Data de modificação: %s\n", data_str);
        printf(" Ordem: %d\n", tabela[i].ordem);
        //printf(" Offset: %ld\n", tabela[i].offset);
        printf(" Comprimido: %s\n", tabela[i].is_compressed ? "Sim" : "Não");
        printf("--------------------------------------------------\n");
    }

    free(tabela);
    fclose(archive);
}

void derivada(const char *archive_name, char **members, int num_members)
{
    FILE *archive = fopen(archive_name, "r+b");
    EntradaVC *tabela = NULL;
    int qtd = 0;


    if (!archive)
    {
        //se o arquivo não existe, interrompe a execução
        perror("Arquivo .vc não existente");
        return;
    } else
    {
        //se já existir, tenta carregar o diretório
        load_directory(archive, &tabela, &qtd);
    }

    // marca quais arquivos devem ser copiados
    int *copiar = calloc(qtd, sizeof(int));
    for (int i = 0; i < qtd; i++)
    {
        copiar[i] = 0; // não copiar por padrão
        for (int j = 0; j < num_members; j++)
        {
            if (strcmp(tabela[i].nome, members[j]) == 0)
            {
                copiar[i] = 1; // marcar para ser copiado
                break;
            }
        }
    }

    //cria novo arquivo com o nome arbitrário pois não consegui apenas adicionar o _z corretamente
    FILE *derivado = fopen(/*archive_name*/"meu_arq_z.vc", "wb+");
    //strncat(archive_name, "_z.vc", sizeof(archive_name) - strlen(archive_name) - 1);


    if (!derivado)
    {
        perror("Erro ao criar arquivo temporário");
        fclose(archive);
        free(tabela);
        free(copiar);
        return;
    }

    EntradaVC *derivado_tabela = NULL;
    int derivado_qtd = 0;

    for (int i = 0; i < qtd; i++)
    {
        if (!copiar[i]) continue;

        EntradaVC entrada = tabela[i];
        unsigned char *buffer = malloc(entrada.tamanho_em_disco);
        fseek(archive, entrada.offset, SEEK_SET);
        fread(buffer, 1, entrada.tamanho_em_disco, archive);

        fseek(derivado, 0, SEEK_END);
        long novo_offset = ftell(derivado);
        fwrite(buffer, 1, entrada.tamanho_em_disco, derivado);
        free(buffer);

        entrada.offset = novo_offset;
        entrada.ordem = derivado_qtd;

        derivado_tabela = realloc(derivado_tabela, (derivado_qtd + 1) * sizeof(EntradaVC));
        derivado_tabela[derivado_qtd++] = entrada;
    }

    save_directory(derivado, &derivado_tabela, &derivado_qtd);

    fclose(archive);
    fclose(derivado);
    free(tabela);
    free(derivado_tabela);
    free(copiar);

}
