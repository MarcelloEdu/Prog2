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
    const char *archive_name = argv[2];

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
        if (argc != 5) {
            fprintf(stderr, "Uso correto: vinac -m <archive> <target> <membro_a_mover>\n");
            return 1;
        }
        move_member(archive_name, argv[3], argv[4]);
        return 0;
    }

    // Opção inválida
    fprintf(stderr, "Erro: opção desconhecida '%s'\n", opcao);
    return 1;
}
