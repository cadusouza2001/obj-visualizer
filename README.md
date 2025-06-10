# Visualizador de Modelos 3D com OpenGL

Projeto universitário de Computação Gráfica desenvolvido para o Trabalho de Grau B (GB). Este aplicativo carrega modelos 3D no formato `.OBJ`, aplica materiais definidos via `.MTL`, renderiza com texturas e iluminação baseada em shaders GLSL, e permite navegação interativa por uma cena 3D com múltiplos objetos.

---

## 🎯 Funcionalidades Principais

- ✅ Leitura de arquivos `.OBJ`, `.MTL` e texturas associadas
- ✅ Conversão de faces com 4 ou mais vértices em triângulos
- ✅ Geração de VAOs/VBOs por grupo de objeto
- ✅ Renderização com modelo de iluminação **Phong completo + fog**
- ✅ Aplicação de transformações via matriz `model`
- ✅ Navegação com câmera virtual estilo FPS (`WASD` ou setas)
- ✅ Cálculo de bounding box para cada objeto
- ✅ Cena com múltiplos modelos carregados de forma coerente

---

## 📁 Estrutura do Projeto

- `main.cpp`: lógica principal, carga de arquivos, câmera, renderização
- `Mesh.h`, `Group.h`, `Face.h`: estrutura de dados do modelo `.OBJ`
- `stb_image.h`: biblioteca para leitura de texturas (já incluída)
- `shaders`: shaders GLSL embutidos no código
- `barrels.obj`, `water.obj`: exemplos de modelos carregados
- `*.mtl`, `*.png`: arquivos auxiliares de materiais e texturas
- `OBJ_Visualizer.sln`: solução pronta para Visual Studio 2022
- `scene.txt`: define quais `.obj` e curva de animação serão carregados

---

## 🧠 Conexões com Conteúdo Teórico

| Tópico Teórico                        | Implementação no Código                                             |
|--------------------------------------|----------------------------------------------------------------------|
| **Modelagem 3D e Formato OBJ/MTL**   | Leitura de `v`, `vt`, `vn`, `f`, `mtllib`, `usemtl`, `map_Kd`       |
| **Shaders GLSL**                     | Vertex e Fragment shaders com Phong + Fog                           |
| **Iluminação (Phong)**               | Ambient (`Ka`), Diffuse (`Kd`), Specular (`Ks`, `Ns`)               |
| **Transformações 3D**               | `glm::translate`, `rotate`, `scale`, uso de `model`, `view`, `proj` |
| **Câmera Virtual**                   | `glm::lookAt`, movimentação com `WASD`                              |
| **Texturização**                     | `sampler2D`, `TexCoord`, interpolação via UV                        |
| **Bounding Box**                     | Função `computeBoundingBox()` por malha                             |
| **VAO/VBO e OpenGL moderno**         | Um VAO por `Group`, VBOs para posições, normais e texturas          |
| **Fog (neblina)**                    | Implementado no fragment shader com fator exponencial               |

---

## ⌨️ Controles

- `W / ↑`: mover para frente
- `S / ↓`: mover para trás
- `A / ←`: mover para esquerda
- `D / →`: mover para direita
- **Mouse (botão esquerdo)**: rotaciona a câmera
- `O`: alterna entre câmera livre e câmera que segue o carro
- `F`: mostra/oculta a curva de animação
- `R`: reinicia o percurso do carro
- `1`: liga/desliga luz ambiente
- `2`: liga/desliga luz difusa
- `3`: liga/desliga luz especular
- `+`/`=`: aumenta a atenuação das luzes
- `-`: diminui a atenuação das luzes
- `N`: diminui a densidade do fog
- `M`: aumenta a densidade do fog
- `ESC`: fecha o programa

---

## 🛠️ Setup no Visual Studio 2022

1. Abra o projeto via o arquivo de solução `OBJ_Visualizer.sln`
2. Todas as dependências necessárias (GLFW, GLEW, GLM, stb_image) **já estão incluídas no repositório**
3. Compile e execute com `F5` diretamente no Visual Studio

---

## 📌 Observações

- A triangulação das faces é obrigatória para garantir compatibilidade com `glDrawArrays(GL_TRIANGLES)`.
- O programa suporta múltiplos modelos `.OBJ`, cada um com suas texturas e materiais distintos.
- As transformações e animações serão estendidas no Trabalho do Grau B com curvas B-Spline e movimentação de veículos.
- Os caminhos dos modelos e do arquivo de curva são configurados em `scene.txt`.

---

## 📚 Créditos e Licença

Projeto acadêmico desenvolvido por Carlos Souza.  
Texturas e modelos OBJ foram obtidos de fontes públicas para fins didáticos.
