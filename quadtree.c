#include "quadtree.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <OpenGL/gl.h>


unsigned int first = 1;
char desenhaBorda = 1;

QuadNode* newNode(int x, int y, int width, int height) {
    QuadNode* n = malloc(sizeof(QuadNode));
    n->x = x;
    n->y = y;
    n->width = width;
    n->height = height;
    n->NW = n->NE = n->SW = n->SE = NULL;
    n->color[0] = n->color[1] = n->color[2] = 0;
    n->id = first++;
    return n;
}

int findindex(Img* pic, int x, int y) {
    return pic->width * 3 * y + 3 * x;
}
void geraQuad(Img* pic, float minDetail, QuadNode* node) {
    // calculo da area da regiao:
    int reg_size = 3 * node->width * node->height;

    // calculo da cor media de cada componente RGB
    float rm = 0.0;
    float gm = 0.0;
    float bm = 0.0;

    for (int i = node->x; i < node->width * 3; i += 3) {
        for (int j = node->y; j < node->height; j++) {
            int indexRed = findindex(pic, i, j);
            rm += pic->img[indexRed];
            gm += pic->img[indexRed + 1];
            bm += pic->img[indexRed + 2];
        }
    }
    rm = rm / reg_size;
    gm = gm / reg_size;
    bm = bm / reg_size;

    // o desvio padrao das componentes -> nivel do detalhe
    float dr, dg, db;
    float detalhe = 0.0;
    int r, g, b;
    for (int i = node->x; i < node->width * 3; i += 3) {
        for (int j = node->y; j < node->height; j++) {
            int indexRed = findindex(pic, i, j);
            r = pic->img[indexRed];
            g = pic->img[indexRed + 1];
            b = pic->img[indexRed + 2];
            dr = pow((r - rm), 2);
            dg = pow((g - gm), 2);
            db = pow((b - bm), 2);
            detalhe += sqrt(dr + dg + db);
        }
    }
    detalhe = detalhe / reg_size;

    // caso de parada da possivel recursao
    if (detalhe > minDetail) {
        // atualiza o nodo atual
        node->status = PARCIAL;
        node->color[0] = 0;
        node->color[1] = 255;
        node->color[2] = 0;

        // divide o quadrante
        int x = node->x;
        int y = node->y;
        QuadNode* nw = newNode(x, y, node->width / 2, node->height / 2);
        x = x + 3 * node->width / 2;
        QuadNode* ne = newNode(x, y, node->width / 2, node->height / 2);
        x = node->x;
        y = node->y + node->height / 2;
        QuadNode* sw = newNode(x, y, node->width / 2, node->height / 2);
        x = x + 3 * node->width / 2;
        QuadNode* se = newNode(x, y, node->width / 2, node->height / 2);

        //chama recursivamente
        geraQuad(pic, minDetail, nw);
        geraQuad(pic, minDetail, ne);
        geraQuad(pic, minDetail, sw);
        geraQuad(pic, minDetail, se);

        // linkando
        node->NW = nw;
        node->NE = ne;
        node->SW = sw;
        node->SE = se;
    } else {
        // pinta e para recursao
        node->status = CHEIO;
        node->color[0] = rm;
        node->color[1] = gm;
        node->color[2] = bm;
        return;
    }
}
QuadNode* geraQuadtree(Img* pic, float minDetail) {
    int width = pic->width;
    int height = pic->height;

    QuadNode* raiz = newNode(0, 0, width, height);
    geraQuad(pic, minDetail, raiz);
    return raiz;
}

// Limpa a memória ocupada pela árvore
void clearTree(QuadNode* n) {
    if (n == NULL) return;
    if (n->status == PARCIAL) {
        clearTree(n->NE);
        clearTree(n->NW);
        clearTree(n->SE);
        clearTree(n->SW);
    }
    //printf("Liberando... %d - %.2f %.2f %.2f %.2f\n", n->status, n->x, n->y, n->width, n->height);
    free(n);
}

// Ativa/desativa o desenho das bordas de cada região
void toggleBorder() {
    desenhaBorda = !desenhaBorda;
    printf("Desenhando borda: %s\n", desenhaBorda ? "SIM" : "NÃO");
}

// Desenha toda a quadtree
void drawTree(QuadNode* raiz) {
    if (raiz != NULL)
        drawNode(raiz);
}

// Grava a árvore no formato do Graphviz
void writeTree(QuadNode* raiz) {
    FILE* fp = fopen("quad.dot", "w");
    fprintf(fp, "digraph quadtree {\n");
    if (raiz != NULL)
        writeNode(fp, raiz);
    fprintf(fp, "}\n");
    fclose(fp);
    printf("\nFim!\n");
}

void writeNode(FILE* fp, QuadNode* n) {
    if (n == NULL) return;
    
    if (n->NE != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->NE->id);
    if (n->NW != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->NW->id);
    if (n->SE != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->SE->id);
    if (n->SW != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->SW->id);
    writeNode(fp, n->NE);
    writeNode(fp, n->NW);
    writeNode(fp, n->SE);
    writeNode(fp, n->SW);
}

// Desenha todos os nodos da quadtree, recursivamente
void drawNode(QuadNode* n) {
    if (n == NULL) return;

    glLineWidth(0.1);

    if (n->status == CHEIO) {
        glBegin(GL_QUADS);
        glColor3ubv(n->color);
        glVertex2f(n->x, n->y);
        glVertex2f(n->x + n->width - 1, n->y);
        glVertex2f(n->x + n->width - 1, n->y + n->height - 1);
        glVertex2f(n->x, n->y + n->height - 1);
        glEnd();
    }

    else if (n->status == PARCIAL) {
        if (desenhaBorda) {
            glBegin(GL_LINE_LOOP);
            glColor3ubv(n->color);
            glVertex2f(n->x, n->y);
            glVertex2f(n->x + n->width - 1, n->y);
            glVertex2f(n->x + n->width - 1, n->y + n->height - 1);
            glVertex2f(n->x, n->y + n->height - 1);
            glEnd();
        }
        drawNode(n->NE);
        drawNode(n->NW);
        drawNode(n->SE);
        drawNode(n->SW);
    }
    // Nodos vazios não precisam ser desenhados... nem armazenados!
}
