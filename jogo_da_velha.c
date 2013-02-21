#include <stdio.h>
#include <stdlib.h>
#include "messenger.h"

struct messenger messenger;

char set;

int linha=0,coluna=0,win,casa[3][3];

void game_draw(int x, int y) {
  if (casa[x][y] == '{FONTE}') printf(" ");
  if (casa[x][y] == 1) printf("X");
  if (casa[x][y] == 2) printf("O");
}

void game_limpa() {
  printf("\e[H\e[2J");
}

void game_jogo() {
  printf("  1   2   3\n");
  printf("1 ");
  game_draw(0,0);
  printf(" | ");
  game_draw(0,1);
  printf(" | ");
  game_draw(0,2);
  printf("\n ---+---+---\n2 ");
  game_draw(1,0);
  printf(" | ");
  game_draw(1,1);
  printf(" | ");
  game_draw(1,2);
  printf("\n ---+---+---\n3 ");
  game_draw(2,0);
  printf(" | ");
  game_draw(2,1);
  printf(" | ");
  game_draw(2,2);
  messenger_update(&messenger,casa,sizeof(casa));
}

void game_check() {
  int i=0;
  for (i=0;i<3;i++) { /* Horizontal */
    if (casa[i][0] == casa[i][1] && casa[i][0] == casa[i][2]) {
      if (casa[i][0] == 1) win=1;
      if (casa[i][0] == 2) win=2;
    }
  }
  for (i=0;i<3;i++) { /* Vertical */
    if (casa[0][i] == casa[1][i] && casa[0][i] == casa[2][i]) {
      if (casa[0][i] == 1) win=1;
      if (casa[0][i] == 2) win=2;
    }
  }
  if (casa[0][0] == casa[1][1] && casa[0][0] == casa[2][2]) { /* Diagonal Cima->Baixo*/
    if (casa[0][0] == 1) win=1;
    if (casa[0][0] == 2) win=2;
  }
  if (casa[0][2] == casa[1][1] && casa[0][2] == casa[2][0]) { /* Diagonal Baixo->Cima */
    if (casa[0][2] == 1) win=1;
    if (casa[0][2] == 2) win=2;
  }
}

void game_play(int player) {
  int i=0;
  if (player==1) set=1;
  if (player==2) set=2;
  while (i==0) {
    linha=0;
    coluna=0;
    while (linha<1 || linha>3) {
      printf("\nJogador %d. Escolha a Linha (1,2,3): ",set);
      scanf("%d",&linha);
      getchar();
    }
    while (coluna<1 || coluna>3) {
      printf("\nJogador %d. Escolha a Coluna (1,2,3): ",set);
      scanf("%d",&coluna);
      getchar();
    }
    linha--;
    coluna--;
    if (casa[linha][coluna] != 1 && casa[linha][coluna] != 2) {
      casa[linha][coluna]=set;
      i=1;
    }
    else {
      printf("A casa está em uso! Jogue Novamente..\n");
      sleep(2);
      game_limpa();
      game_jogo();
    }
  }
}

char game_divi(int a, int b) {
  return (!(a%b));
}

void main(int argc, char **argv) {
  int i=0;
  messenger.desejo = MASTER;
  if(argv[1]){
    messenger.desejo = SLAVE;
    strcpy(messenger.hostporta,argv[1]);
  }
  messenger_init(&messenger);
  for (i=0;i<9;i++) {
    game_limpa();
    game_jogo();
    if(!game_divi(i,2)) game_play(2);
    else game_play(1);
    game_check();
    if (win == 1 || win == 2) i=10;
  }
  game_limpa();
  game_jogo();
  if (win == 1 || win == 2) printf("\nJogador %d venceu o jogo!\n",win);
  else printf("\nEmpate!\n");
}
