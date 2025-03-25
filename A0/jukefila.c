#include "jukefila.h"
#include <stdlib.h>
#include <stdio.h>


void trocar_posicoes(pedido* a, pedido* b)
{
    pedido* temp = a->proximo;

    a->proximo = b->proximo;
    b->proximo = temp;

    temp = a->anterior;

    a->anterior = b->anterior;
    b->anterior = temp;
}

jukefila* criar_jukefila()//cria a fila de reprodução da jukebox
{
    jukefila* nova = (jukefila*) malloc(sizeof(jukefila));
    
    if(!nova)
        return NULL;

    nova->inicio = NULL;
    nova->final = NULL;
    return nova;
}

void inserir_jukefila(pedido* elemento, jukefila* fila)
{
    //recebe um elemento e coloca em ordem de elemento->valor

    if(!fila || !elemento)
        return;

    // Se a fila estiver vazia, insere no início
    if (!fila->inicio)
    {
        fila->inicio = elemento;
        fila->final = elemento;
        elemento->proximo = NULL;
        elemento->anterior = NULL;
        return;
    }

    pedido* atual = fila->inicio;//ponteiro para percorrer a fila
    
    // Encontra a posição correta para inserir
    while (atual && atual->valor >= elemento->valor)
        atual = atual->proximo;

    if (!atual)//se for o maior valor, insere no final
    {
        fila->final->proximo = elemento;
        elemento->anterior = fila->final;
        elemento->proximo = NULL;
        fila->final = elemento;
    }
    else if (!atual->anterior) // se for o menor valor, insere no início
    {
        elemento->proximo = fila->inicio;
        fila->inicio->anterior = elemento;
        fila->inicio = elemento;
        elemento->anterior = NULL;
    }
    else // se for um valor intermediário, insere no meio
    {
        elemento->proximo = atual;
        elemento->anterior = atual->anterior;
        atual->anterior->proximo = elemento;
        atual->anterior = elemento;
    }
}

/*
IMPLEMENTAÇÃO DE INSERIR_JUKEFILA *INCORRETA* (NÃO FUNCIONA)
o correto eh inserir no inicio e depois verificar se o elemento inserido é maior que o proximo

void inserir_jukefila(pedido* elemento, jukefila* fila)
{
    //recebe um elemento e coloca em ordem de elemento->valor

    if(!fila)
        return;
    if(!elemento)
        return;

    if(fila->final) //se a fila não estiver vazia, insere no final
    {
        fila->final->proximo = elemento;
        elemento->anterior = fila->final;
    }
    else{ //se a fila estiver vazia, insere no inicio
        fila->inicio = elemento;
        fila->final = elemento;
    }
        
    if(elemento->proximo && elemento->valor > elemento->proximo->valor){ //
        trocar_posicoes(elemento, elemento->proximo);//troca de lugar o atual com o ultimo da fila
    }

}
*/

pedido* consumir_jukefila(jukefila* fila)//após a reprodução da musica de maior valor, toca a proxima
{
    if(!fila || !fila->inicio)
        return NULL;

    pedido* removido = fila->inicio;
    fila->inicio = fila->inicio->proximo;

    if(fila->inicio)//se a fila não estiver vazia
        fila->inicio->anterior = NULL;//o anterior do inicio é NULL
    else
        fila->final = NULL;//se a fila estiver vazia, o final é NULL

    removido->proximo = NULL;//o proximo do removido é NULL

    return removido;
}

/*

IMPLEMENTAÇÃO DE CONSUMIR_JUKEFILA *INCORRETA* (NÃO FUNCIONA)
faltou a verificação de fila->inicio->proximo

pedido* consumir_jukefila(jukefila* fila)//após a reprodução da musica de maior valor, toca a proxima
{
    if(!fila)
        return NULL;

    if(!fila->inicio)
        return NULL;

    pedido* proximo = fila->inicio;
    fila->inicio = fila->inicio->proximo;

    return proximo;
}*/

unsigned int contar_jukefila(jukefila* fila)//conta as musicas na fila (tamanho da fila)
{
    //retorna o tamanho da fila
    unsigned int cont = 0;
    pedido* aux = fila->inicio;
    while(aux)
    {
        cont++;
        aux = aux->proximo;
    }
    return cont;
}

void destruir_jukefila(jukefila *fila)//desalocar memoria
{
    if(fila)
    {
        pedido* aux = fila->inicio;
        while(aux)
        {
            pedido* temp = aux;
            aux = aux->proximo;
            free(temp->musica);
            free(temp);
        }
        free(fila);
    }
}