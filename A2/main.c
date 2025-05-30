#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vina.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: vinac <opcao> <archive> [membros...]\n");
        return 1;
    }

    const char *opcao = argv[1];

    //copia o nome do arquivo e garante a extensão .vc
    char nome_completo[1024];
    strncpy(nome_completo, argv[2], sizeof(nome_completo) - 1);
    nome_completo[sizeof(nome_completo) - 1] = '\0';

    if (strlen(nome_completo) < 3 || strcmp(nome_completo + strlen(nome_completo) - 3, ".vc") != 0) {
        strncat(nome_completo, ".vc", sizeof(nome_completo) - strlen(nome_completo) - 1);
    }
    const char *archive_name = nome_completo;



    // -ip: inserir plano
    if (strcmp(opcao, "-ip") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Erro: você deve fornecer ao menos um membro para inserir.\n");
            return 1;
        }
        return insert_plan(archive_name, &argv[3], argc - 3);
    }

    // -ic: inserir comprimido
    if (strcmp(opcao, "-ic") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Erro: você deve fornecer ao menos um membro para inserir.\n");
            return 1;
        }
        return insert_compressed(archive_name, &argv[3], argc - 3);
    }

    // -x: extrair
    if (strcmp(opcao, "-x") == 0) {
        if (argc == 3) {
            // extrair todos
            return extract(archive_name, NULL, 0);
        } else {
            return extract(archive_name, &argv[3], argc - 3);
        }
    }

    // -r: remover
    if (strcmp(opcao, "-r") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Erro: você deve fornecer ao menos um membro para remover.\n");
            return 1;
        }
        remove_members(archive_name, &argv[3], argc - 3);
        return 0;
    }

    // -c: listar conteúdo
    if (strcmp(opcao, "-c") == 0) {
        list_content(archive_name);
        return 0;
    }

    // -m: mover membro
    if (strcmp(opcao, "-m") == 0) {
       if (argc == 5) {
           // Forma normal: -m <archive> <target> <alvo>
           const char *target = (strcmp(argv[3], "NULL") == 0) ? NULL : argv[3];
           move_member(archive_name, target, argv[4]);
           return 0;
       } else if (argc == 4) {
           // Forma alternativa: -m <archive> <alvo> (mover para o início)
           move_member(archive_name, NULL, argv[3]);
           return 0;
       } else {
           fprintf(stderr, "Uso correto:\n");
           fprintf(stderr, "  vinac -m <archive> <target> <membro_a_mover>\n");
           fprintf(stderr, "  vinac -m <archive> <membro_a_mover>            (para mover ao início)\n");
           return 1;
       }
    }

    if(strcmp(opcao, "-z") == 0){
        if (argc < 4){
            fprintf(stderr, "erro, voce deve passar ao menos um membro para o novo aquivo \n");
            return 1;
        }
        derivada(archive_name, &argv[3], argc -3);
        return 0;
    }

    //opção inválida
    fprintf(stderr, "Erro: opção desconhecida '%s'\n", opcao);
    return 1;
}
