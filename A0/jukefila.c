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

    if(!fila)
        return;
    if(!elemento)
        return;

    if(fila->final)
    {
        fila->final->proximo = elemento;
        elemento->anterior = fila->final;
    }
    else{
        fila->inicio = elemento;
        fila->final = elemento;
    }
        
    if(elemento->proximo && elemento->valor > elemento->proximo->valor){
        trocar_posicoes(elemento, elemento->proximo);//troca de lugar o atual com o ultimo da fila
    }

}

pedido* consumir_jukefila(jukefila* fila)//após a reprodução da musica de maior valor, toca a proxima
{
    if(!fila)
        return NULL;

    if(!fila->inicio)
        return NULL;

    pedido* proximo = fila->inicio;
    fila->inicio = fila->inicio->proximo;

    return proximo;
}

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