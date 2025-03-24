#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

#include <stdio.h>
#include <stdlib.h>

typedef struct No {
    void *dado;
    struct No *proximo;
    struct No *anterior;
    //int prioridade;
} No;

typedef struct {
    No *cabeca;
    No *cauda;
    size_t tamanho_dado;  // Armazena o tamanho do tipo de dado
} Lista, Fila, Pilha;

// Funções para Lista
Lista *criar_lista(size_t tamanho_dado);  // Cria uma lista vazia
void inserir(Lista *lista, void *dado);  // Insere um elemento no final da lista
void remover(Lista *lista, void *dado, int (*cmp)(void*, void*));  // Remove um elemento da lista
void imprimir_lista(Lista *lista, void (*imprimir)(void*));  // Imprime todos os elementos da lista
void liberar_lista(Lista *lista);  // Libera toda a memória da lista

// Funções para Fila
Fila *criar_fila(size_t tamanho_dado);  // Cria uma fila vazia
void enqueue(Fila *fila, void *dado);  // Insere um elemento no final da fila
void dequeue(Fila *fila);  // Remove o elemento do início da fila
void *front(Fila *fila);  // Retorna o elemento do início da fila sem removê-lo
int fila_vazia(Fila *fila);  // Retorna 1 se a fila estiver vazia, 0 caso contrário
void liberar_fila(Fila *fila);  // Libera toda a memória da fila

// Funções para Pilha
Pilha *criar_pilha(size_t tamanho_dado);  // Cria uma pilha vazia
void push(Pilha *pilha, void *dado);  // Adiciona um elemento ao topo da pilha
void pop(Pilha *pilha);  // Remove o elemento do topo da pilha
void *top(Pilha *pilha);  // Retorna o elemento do topo da pilha sem removê-lo
int pilha_vazia(Pilha *pilha);  // Retorna 1 se a pilha estiver vazia, 0 caso contrário
void liberar_pilha(Pilha *pilha);  // Libera toda a memória da pilha

#endif
