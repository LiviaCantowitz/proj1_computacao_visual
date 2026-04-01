# proj1_computacao_visual
# Processamento de Imagens com SDL3

## Descrição do Projeto

Este projeto implementa um software de processamento de imagens em **linguagem C**, utilizando a biblioteca **SDL3**. O programa é capaz de carregar imagens em formatos comuns (PNG, JPG, BMP), convertê-las para escala de cinza, analisar seu histograma e aplicar técnicas de equalização para melhorar o contraste.

**Disciplina:** Computação Visual  
**Professor:** André Kishimoto  
**Instituição:** Universidade Presbiteriana Mackenzie  


---

## Integrantes do Grupo

| Nome | RA |
|------|-----|
| Alexandre Ribeiro de Souza | 10417845 |
| Livia Negrucci Cantowitz | 10389419 |

---

## Funcionalidades Implementadas

### 1. **Carregamento de Imagem**
- Suporte aos formatos PNG, JPG e BMP via biblioteca SDL_image
- Validação e tratamento de erros para arquivos não encontrados ou inválidos
- Carregamento eficiente através de linha de comando

### 2. **Análise e Conversão para Escala de Cinza**
- Detecção automática se a imagem já está em escala de cinza
- Conversão para escala de cinza usando a fórmula padrão ITU-R BT.709:
  ```
  Y = 0.2125 × R + 0.7154 × G + 0.0721 × B
  ```
- A imagem em escala de cinza serve como base para todas as operações posteriores

### 3. **Interface Gráfica com Duas Janelas**

#### Janela Principal
- Exibe a imagem sendo processada (colorida ou em escala de cinza)
- Tamanho adaptável ao tamanho da imagem carregada
- Inicializa centralizada no monitor principal
- Suporta redimensionamento dinâmico

#### Janela Secundária (Filha)
- Tamanho fixo (340 × 540 pixels)
- Posicionada automaticamente ao lado da janela principal
- Contém histograma, análise estatística e botão de controle

### 4. **Análise e Exibição do Histograma**
- Cálculo preciso do histograma com 256 níveis de intensidade
- Visualização gráfica proporcionada do histograma
- Análise estatística:
  - **Média de Intensidade**: Classifica a imagem como "clara", "média" ou "escura"
  - **Desvio Padrão**: Classifica o contraste como "alto", "médio" ou "baixo"
- Rótulos (0, 128, 255) para referência visual

---

##  Estrutura do Código

### Principais Estruturas

```c
typedef struct {
    SDL_Surface  *orig, *gray, *eq;     // Superfícies de imagem
    SDL_Texture  *tex;                   // Textura renderizada
    SDL_Window   *wMain, *wSec;         // Janelas principal e secundária
    SDL_Renderer *rMain, *rSec;         // Renderizadores
    TTF_Font     *font;                  // Fonte para texto
    int   hist[HIST_SIZE];              // Histograma (256 valores)
    double mean, stddev;                // Média e desvio padrão
    bool  equalized;                    // Flag de estado
    int   btnState;                     // Estado do botão (0/1/2)
} App;
```

### Funções Principais

- **`calcHist()`**: Calcula o histograma da imagem
- **`analyzeHist()`**: Analisa média e desvio padrão
- **`isGray()`**: Detecta se imagem já está em escala de cinza
- **`toGray()`**: Converte para escala de cinza usando fórmula ITU-R BT.709
- **`equalize()`**: Aplica equalização do histograma
- **`updateDisplay()`**: Atualiza textura e histograma
- **`renderMain()`**: Renderiza janela principal
- **`renderSec()`**: Renderiza janela secundária com gráficos e análise

---

## Elementos da Interface

### Cores Utilizadas

| Elemento | Cor |
|----------|-----|
| Fundo secundário | #1C1C22 (cinza escuro) |
| Texto padrão | Branco (#FFFFFF) |
| Média de intensidade | Amarelo (#FFD73C) |
| Desvio padrão | Ciano (#46C8DC) |
| Histograma | Azul (#5AAAFF) |
| Botão normal | Azul (#2673D2) |
| Botão hover | Azul claro (#50A0FF) |
| Botão pressionado | Azul escuro (#144BB4) |

### Layout da Janela Secundária

```
┌─────────────────────┐
│   Histograma        │ (Título)
│  ┌───────────────┐  │
│  │               │  │ (Gráfico 200×200px)
│  │  Histograma   │  │
│  └───────────────┘  │
│  0    128    255    │
│                     │
│ Média: XX (clara)   │ (Análise)
│ Desvio: XX (alto)   │
│                     │
│  ┌───────────────┐  │
│  │   Equalizar   │  │ (Botão interativo)
│  └───────────────┘  │
└─────────────────────┘
```

---

##  Detalhes Técnicos

### Algoritmo de Equalização do Histograma

1. **Cálculo da CDF (Cumulative Distribution Function)**
   ```
   CDF[0] = hist[0]
   CDF[i] = CDF[i-1] + hist[i]  (para i = 1 até 255)
   ```

2. **Normalização**
   ```
   LUT[i] = round((CDF[i] - CDFmin) / (total - CDFmin) × 255)
   ```

3. **Aplicação da LUT** a cada pixel da imagem

### Formato de Armazenamento

- Formato de pixel: RGBA8888 (32 bits por pixel)
- Profundidade: 8 bits por canal
- Intervalo de valores: 0–255

### Performance

- Processamento eficiente em tempo real
- Cálculo do histograma: O(W × H)
- Renderização a ~60 FPS (SDL_Delay de 16ms)

---

## Tratamento de Erros

O programa implementa verificações robustas:

- Validação de argumentos da linha de comando
- Tratamento de arquivo não encontrado
- Validação de formato de imagem
- Gerenciamento seguro de memória
- Fallback para fontes do sistema (se assets/font.ttf não existir)
- Tratamento de erros de SDL e SDL_image

---


## Repositório

Este projeto está disponível em: https://github.com/LiviaCantowitz/proj1_computacao_visual

---

## Referências e Fontes

- **SDL3 Documentation**: https://wiki.libsdl.org/SDL3/
- **SDL_image**: https://github.com/libsdl-org/SDL_image
- **SDL_ttf**: https://github.com/libsdl-org/SDL_ttf
- **Image Processing**: Digital Image Processing (Gonzalez & Woods)
- **Histogram Equalization**: Conceitos padrão de processamento digital de imagens
- **ITU-R BT.709**: Standard for HDTV color space (fórmula de conversão para escala de cinza)

---

## Licença

Este projeto foi desenvolvido como atividade acadêmica da disciplina Computação Visual na Universidade Presbiteriana Mackenzie.

---

**Última atualização:** 28/03/2026  
**Status:** Completo e funcional
