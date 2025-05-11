# VINAc - O Arquivador Virtual

**Autor:** Marcello Eduardo Pereira Jorge
**GRR:** 20244350

---

## ✨ Sobre o Projeto

VINAc é um arquivador desenvolvido em C, inspirado em ferramentas como `tar` e `zip`, capaz de:

* Inserir arquivos (com ou sem compressão);
* Remover membros;
* Mover membros na ordem interna do arquivo;
* Listar o conteúdo do arquivo;
* Extrair arquivos.

---

## 📁 Arquivos no Pacote

| Arquivo         | Descrição breve                                          |
| --------------- | -------------------------------------------------------- |
| `vina.c`        | Implementação principal das funcionalidades do VINAc     |
| `main.c`        | Ponto de entrada do programa     |
| `lz.c` / `lz.h` | Algoritmo de compressão LZ utilizado pelo arquivador     |
| `vina.h`        | Protótipos das funções e definição da struct `EntradaVC` |
| `Makefile`      | Script para compilar o projeto                           |
| `README.md`     | Este arquivo, versão Markdown                            |
| `testes.sh`     | Script para testes automatizados                         |

---

## Algoritmos, Estruturas e Decisões de Projeto

### Estrutura de Dados

* **`EntradaVC`**: struct que representa metadados de cada membro do arquivador (nome, UID, offset, tamanho original, comprimido etc). (inicio do vina.h)
* **Diretório no final do arquivo**: o arquivo `.vc` termina com:

  * tabela de `EntradaVC`
  * inteiro com número de membros
  * long com offset do início da tabela
    *(implementado em `save_directory`)*
* A organização respeita o formato obrigatório do enunciado, com a área de diretório no final, contendo apenas os metadados dos membros. O restante do arquivo contém apenas os dados brutos dos arquivos inseridos.

### Compressão

* Utilizamos o algoritmo `LZ_CompressFast`, incluído no projeto.(insert_compressed)
* Ele foi escolhido no lugar de `LZ_Compress` por ser mais rápido e direto, ideal para nosso uso em arquivos pequenos e médios.
* Foi implementada a alocação correta de buffers auxiliares `work` (com 65536 + tamanho do arquivo), exigida pelo algoritmo.
* Decidimos **sempre tentar comprimir** e apenas manter comprimido se for eficaz (ou seja, se o tamanho comprimido for menor que o original).
* **Compressão de arquivos muito pequenos** (com menos de 16 bytes) foi evitada, pois resultava em falhas no algoritmo (acesso a memória inválida). Para esses casos, os arquivos são automaticamente armazenados sem compressão.
* A compressão e descompressão são feitas **arquivo por arquivo**, como exigido, e nunca mantemos dois membros simultaneamente em buffers na RAM.

### Funções e Bibliotecas Específicas Utilizadas

* **`ftruncate` (unistd.h)**: utilizada para cortar o final do arquivo `.vc` antes de salvar um novo diretório. Essencial para evitar sobreposição de dados antigos.
* **`stat` (sys/stat.h)**: usada para obter metadados dos arquivos reais, como a data de modificação.
* **`strftime` (time.h)**: usada para exibir a data de modificação em formato legível ao listar o conteúdo.
* **`utime` (utime.h)**: opcionalmente usada para restaurar a data de modificação original ao extrair arquivos.
* **`fseek` / `ftell`**: controle de posicionamento dentro do arquivo `.vc`, essencial para organizar os dados binários.
* **`fread`, `fwrite`**: leitura e escrita binária dos arquivos e do diretório interno.

### Alternativas e Dificuldades

* **Modularização parcial**: embora tenha dividido em múltiplos arquivos, optei por manter algumas funções no `vina.c` para facilitar o fluxo e testes.(encontrar membro por exemplo)
* **Remoção e regravação**: requer recriar o `.vc`, copiando os dados binários e atualizando todos os offsets.
* **Função `move_member`**: foi desafiador garantir que a ordem dos membros fosse mantida corretamente, e sem sobrescrever dados.
* **Compressão seletiva**: a compressão é aceita apenas quando traz ganho real. Arquivos pequenos, já comprimidos ou ineficazes de comprimir são mantidos planos.
* **Data de modificação**: exibida em formato legível via `strftime()`.

---

## Bugs conhecidos

* Inserção de arquivos grandes (com muitos megabytes) ainda não foi testada extensivamente.

---

## Detalhes de Uso e Comportamento

* **Criação do arquivo `.vc`**: o arquivo é criado automaticamente quando uma inserção (`-ip` ou `-ic`) é feita e o arquivo de destino não existe. A função `insert_plan` ou `insert_compressed` se encarrega de abrir em modo `w+b` quando necessário.

* **Comando `-m` (mover)**:

  * Quando o usuário executa `vinac -m <archive> <target> <membro>`, o `membro` é movido **logo após o `target`**.
  * Para mover um membro para o **início do arquivo**, o usuário pode apenas não preencher o argumento `target`, por exemplo: `vina -m MeuArquivo.vc membro.txt`.

* **Comando `-c` (listar conteúdo)**:

  * Exibe todos os membros em ordem, com informações como nome, UID, tamanhos, e data de modificação.

* **Comando `-x` (extrair)**:

  * Ao extrair arquivos, o sistema respeita o nome original do membro e tenta recriá-lo no diretório atual.
  * Arquivos comprimidos são automaticamente descomprimidos utilizando `LZ_Uncompress`, respeitando o tamanho original.

* **UIDs**:

  * Atribuídos com base na quantidade de membros existente no momento da inserção. Mesmo que o membro seja reordenado, sua UID permanece.

* **Compressão seletiva**:

  * É feita tentativa de compressão sempre que solicitado com `-ic`, e só é mantido o resultado comprimido se ele for menor que o original.
  * Caso o arquivo seja muito pequeno ou já comprimido (e a compressão não for eficaz), ele é armazenado em modo plano automaticamente.