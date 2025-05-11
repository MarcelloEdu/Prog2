#!/bin/bash

VC="./vina"
VCFILE="teste_final.vc"

clear
echo "==== TESTE VINAc ===="
echo ""

# Etapa 1: criar binários com tamanho definido pelo usuário
BINARIOS_CRIADOS=()
while true; do
    read -p "Informe o tamanho (em bytes) do binário a ser criado: " TAM
    nome="arquivo_bin_$TAM.bin"
    dd if=/dev/zero of=$nome bs=1 count=$TAM status=none
    echo "Arquivo $nome criado com $TAM bytes."
    BINARIOS_CRIADOS+=("$nome")
    
    read -p "Deseja criar outro binário? (s/N): " resp
    [[ "$resp" != "s" && "$resp" != "S" ]] && break
done
sleep 1

# Etapa 2: insere plano e lista
echo ">> Inserindo em modo plano os binários..."
$VC -ip $VCFILE "${BINARIOS_CRIADOS[@]}"
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 3: remove e lista
echo ">> Removendo todos os binários..."
$VC -r $VCFILE "${BINARIOS_CRIADOS[@]}"
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 4: insere comprimido e lista
echo ">> Inserindo novamente (comprimido)..."
$VC -ic $VCFILE "${BINARIOS_CRIADOS[@]}"
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 5: remove e lista
echo ">> Removendo novamente..."
$VC -r $VCFILE "${BINARIOS_CRIADOS[@]}"
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 6: criar arquivo txt com N linhas de lorem
read -p "Quantas linhas de texto deseja no arquivo TXT? " LINHAS
txtfile="texto_teste.txt"
txtcopy="texto_teste_OG.txt"
rm -f $txtfile $txtcopy
for ((i=1; i<=LINHAS; i++)); do
    echo "Linha $i: Lorem ipsum dolor sit amet, consectetur adipiscing elit." >> $txtfile
done
cp $txtfile $txtcopy
echo "Arquivo $txtfile criado com $LINHAS linhas."
sleep 1

# Etapa 7: insere plano e lista
echo ">> Inserindo TXT plano..."
$VC -ip $VCFILE $txtfile
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 8: remove e lista
echo ">> Removendo TXT..."
$VC -r $VCFILE $txtfile
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 9: insere comprimido e lista
echo ">> Inserindo TXT comprimido..."
$VC -ic $VCFILE $txtfile
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 10: extrai e lista
echo ">> Extraindo arquivo TXT..."
rm -f $txtfile
$VC -x $VCFILE $txtfile
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 11: compara extraído com original
echo ">> Comparando arquivo extraído com original..."
diff $txtfile $txtcopy && echo "✅ Arquivo extraído igual ao original!" || echo "❌ Diferença encontrada!"
sleep 1

# Etapa 12: insere plano com 3 binários
echo ">> Criando 3 binários de 1024 bytes para teste de movimentação..."
dd if=/dev/zero of=move1.bin bs=1 count=1024 status=none
dd if=/dev/zero of=move2.bin bs=1 count=1024 status=none
dd if=/dev/zero of=move3.bin bs=1 count=1024 status=none
$VC -ip $VCFILE move1.bin move2.bin move3.bin
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 13: move move3 -> move1 e lista
echo ">> Movendo move1.bin depois de move3.bin"
$VC -m $VCFILE move3.bin move1.bin
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 14: move move1 -> move2 e lista
echo ">> Movendo move2.bin depois de move1.bin"
$VC -m $VCFILE move1.bin move2.bin
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa 15: remove tudo e lista final
echo ">> Removendo tudo..."
$VC -r $VCFILE $txtfile move1.bin move2.bin move3.bin
sleep 1
$VC -c $VCFILE
sleep 1

# Etapa final: limpeza
echo ">> Limpando arquivos gerados..."
rm -f "${BINARIOS_CRIADOS[@]}" move1.bin move2.bin move3.bin $txtfile $txtcopy
make clean
sleep 1

echo ""
echo "Fim dos testes!"
