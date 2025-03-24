#include "tad.h"
#include <string.h>

// LISTA

Lista *criar_lista(size_t tamanho_dado) {  // Cria uma lista vazia
    Lista *lista = (Lista *)malloc(sizeof(Lista));
    lista->cabeca = lista->cauda = NULL;
    lista->tamanho_dado = tamanho_dado;
    return lista;
}

void inserir(Lista *lista, void *dado) {  // Insere um elemento no final da lista
    No *novo_no = (No *)malloc(sizeof(No));
    novo_no->dado = malloc(lista->tamanho_dado);
    memcpy(novo_no->dado, dado, lista->tamanho_dado);
    novo_no->proximo = NULL;
    novo_no->anterior = lista->cauda;

    if (lista->cauda)
        lista->cauda->proximo = novo_no;
    else
        lista->cabeca = novo_no;

    lista->cauda = novo_no;
}

void remover(Lista *lista, void *dado, int (*cmp)(void*, void*)) {  // Remove um elemento da lista
    No *atual = lista->cabeca;
    while (atual) {
        if (cmp(atual->dado, dado) == 0) {
            if (atual->anterior)
                atual->anterior->proximo = atual->proximo;
            else
                lista->cabeca = atual->proximo;

            if (atual->proximo)
                atual->proximo->anterior = atual->anterior;
            else
                lista->cauda = atual->anterior;

            free(atual->dado);
            free(atual);
            return;
        }
        atual = atual->proximo;
    }
}

void imprimir_lista(Lista *lista, void (*imprimir)(void*)) {  // Imprime todos os elementos da lista
    No *atual = lista->cabeca;
    while (atual) {
        imprimir(atual->dado);
        atual = atual->proximo;
    }
    printf("\n");
}

void liberar_lista(Lista *lista) {  // Libera toda a memória da lista
    No *atual = lista->cabeca;
    while (atual) {
        No *prox = atual->proximo;
        free(atual->dado);
        free(atual);
        atual = prox;
    }
    free(lista);
}

// FILA 

Fila *criar_fila(size_t tamanho_dado) {  // Cria uma fila vazia
    return criar_lista(tamanho_dado);
}

void enqueue(Fila *fila, void *dado) {  // Insere um elemento no final da fila
    inserir(fila, dado);
}
/*void enqueue(Fila *fila, void *dado, int prioridade) {
    No *novo = malloc(sizeof(No));
    novo->dado = dado;
    novo->prioridade = prioridade;
    novo->proximo = NULL;
    novo->anterior = NULL;

    if (!fila->cabeca) {  // Fila vazia
        fila->cabeca = fila->cauda = novo;
    } else {
        No *atual = fila->cabeca;

        // Encontrar a posição correta para inserir com base na prioridade
        while (atual && atual->prioridade >= prioridade) {
            atual = atual->proximo;
        }

        if (!atual) {  // Inserir no final
            novo->anterior = fila->cauda;
            fila->cauda->proximo = novo;
            fila->cauda = novo;
        } else if (!atual->anterior) {  // Inserir no início
            novo->proximo = fila->cabeca;
            fila->cabeca->anterior = novo;
            fila->cabeca = novo;
        } else {  // Inserir no meio
            novo->proximo = atual;
            novo->anterior = atual->anterior;
            atual->anterior->proximo = novo;
            atual->anterior = novo;
        }
    }
}*/

void dequeue(Fila *fila) {  // Remove o elemento do início da fila
    if (fila->cabeca) {
        No *temp = fila->cabeca;
        fila->cabeca = fila->cabeca->proximo;
        if (fila->cabeca)
            fila->cabeca->anterior = NULL;
        else
            fila->cauda = NULL;
        
        free(temp->dado);
        free(temp);
    }
}

/*void dequeue(Fila *fila) {  // Remove o elemento com maior prioridade
    if (fila->cabeca) {
        No *temp = fila->cabeca;
        No *maiorPrioridade = fila->cabeca;
        
        // Encontrar o nó com maior prioridade
        while (temp) {
            if (temp->prioridade > maiorPrioridade->prioridade) {
                maiorPrioridade = temp;
            }
            temp = temp->proximo;
        }

        // Ajustar os ponteiros para remover o nó de maior prioridade
        if (maiorPrioridade->anterior) {
            maiorPrioridade->anterior->proximo = maiorPrioridade->proximo;
        } else {
            fila->cabeca = maiorPrioridade->proximo;
        }

        if (maiorPrioridade->proximo) {
            maiorPrioridade->proximo->anterior = maiorPrioridade->anterior;
        } else {
            fila->cauda = maiorPrioridade->anterior;
        }

        // Liberar memória do nó removido
        free(maiorPrioridade->dado);
        free(maiorPrioridade);
    }
}*/

void *front(Fila *fila) {  // Retorna o primeiro elemento da fila sem removê-lo
    if (fila->cabeca) {
        return fila->cabeca->dado;
    } else {
        return NULL;
    }
}

int fila_vazia(Fila *fila) {  // Verifica se a fila está vazia
    return fila->cabeca == NULL;
}

void liberar_fila(Fila *fila) {  // Libera a memória da fila
    liberar_lista(fila);
}

// PILHA

Pilha *criar_pilha(size_t tamanho_dado) {  // Cria uma pilha vazia
    return criar_lista(tamanho_dado);
}

void push(Pilha *pilha, void *dado) {  // Adiciona um elemento ao topo da pilha
    inserir(pilha, dado);
}

void pop(Pilha *pilha) {  // Remove o elemento do topo da pilha
    if (pilha->cauda) {
        No *temp = pilha->cauda;
        pilha->cauda = pilha->cauda->anterior;
        if (pilha->cauda)
            pilha->cauda->proximo = NULL;
        else
            pilha->cabeca = NULL;

        free(temp->dado);
        free(temp);
    }
}

void *top(Pilha *pilha) {  // Retorna o elemento do topo da pilha sem removê-lo
    if (pilha->cauda) {
        return pilha->cauda->dado;
    } else {
        return NULL;
    }
}

int pilha_vazia(Pilha *pilha) {  // Verifica se a pilha está vazia
    return pilha->cauda == NULL;
}

void liberar_pilha(Pilha *pilha) {  // Libera a memória da pilha
    liberar_lista(pilha);
}
