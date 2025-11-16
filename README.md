# FDTool --- Analisador de Dependências Funcionais

Este projeto implementa um conjunto completo de ferramentas para
manipulação de **dependências funcionais (DFs)**

- **Cálculo de fecho**\
- **Cobertura mínima**\
- **Chaves candidatas**\
- **Verificação de formas normais (BCNF e 3NF)**

---

# 📁 Estrutura do Projeto

    src/
    ├── closure/      # Cálculo de fecho X+
    ├── keys/         # Cálculo de chaves candidatas
    ├── mincover/     # Algoritmo de cobertura mínima
    ├── normalform/   # Verificação de BCNF e 3NF
    ├── parser/       # Leitura/parsing de arquivos .fds
    ├── main.c        # Interface CLI

---

# 📄 Formato dos Arquivos `.fds`

Um arquivo `.fds` deve seguir exatamente esta estrutura:

    U={A,B,C,D}
    F={A->BC, B->C, AB->D}

- **U** contém os atributos do universo\
- **F** contém as dependências funcionais\
- Não há espaço obrigatório, mas o parser ignora espaços

---

# 🧩 Funcionalidades

## ✔️ 1. Fecho de atributos (X⁺)

    fdtool closure --fds arquivo.fds --X AC

Saída: conjunto de atributos alcançáveis por AC com base nas DFs do
arquivo.

---

## ✔️ 2. Cobertura mínima

    fdtool mincover --fds arquivo.fds

Saída: conjunto de DFs equivalente, seguindo: 1. RHS unitário\
2. LHS mínimo\
3. Nenhuma DF redundante

---

## ✔️ 3. Chaves candidatas

    fdtool keys --fds arquivo.fds

Usa _breadth-first search_ + filtragem de minimalidade.

---

## ✔️ 4. Verificação de Formas Normais (BCNF / 3NF)

    fdtool normalform --fds arquivo.fds

Mostra:

    BCNF: Violations (X)
    A -> B (LHS is not a superkey)
    ...
    3NF: Violations (Y)
    A -> C (Not superkey AND RHS is not prime)

---

# 🔍 Exemplos Práticos

Arquivo `exemplo.fds`:

    U={A,B,C,D}
    F={A->BC, B->C, A->B, AB->D}

### MinCover:

    ./fdtool mincover --fds exemplos/exemplo.fds
    A->B
    B->C
    A->D

### Fecho:

    ./fdtool closure --fds exemplos/exemplo.fds --X A
    ABCD

### Chaves:

    ./fdtool keys --fds exemplos/exemplo.fds
    A

### BCNF / 3NF:

    ./fdtool normalform --fds exemplos/exemplo.fds
    BCNF: Violations (1)
    B -> C (LHS is not a superkey)
    3NF: Violations (1)
    B -> C (Not superkey AND RHS is not prime)

---

# 🧠 Algoritmos Implementados

## 🔹 _Closure (X⁺)_

Iterativo, adicionando atributos enquanto houver mudança.

## 🔹 _Minimum Cover_

1.  Decomposição do RHS\
2.  Redução do LHS\
3.  Remoção de DFs redundantes

## 🔹 _Candidate Keys_

Geração incremental de subconjuntos + testes de superchave.

## 🔹 _Normal Forms_

- BCNF: LHS deve ser superchave\
- 3NF: LHS superchave **ou** RHS atributo primo
