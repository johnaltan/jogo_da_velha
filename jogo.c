#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include "uv.h"

#define CHECK(r, msg) \
  if (r) { \
    uv_err_t err = uv_last_error(uv_loop); \
    fprintf(stderr, "%s: %s\n", msg, uv_strerror(err)); \
    exit(1); \
  }

#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
#define LOG(msg) puts(msg);
#define LOGF(fmt, params...) fprintf(stderr,fmt "\n", params);
#define LOG_ERROR(msg) puts(msg);

#define SERVER 1
#define CLIENT 0

uv_tcp_t *server = NULL, *tcp_out = NULL;
uv_timer_t tmr_atualiza;
uv_timer_t tmr_conexoes;
char server_host[17];
uv_loop_t *uv_loop = NULL;
int mode = SERVER;
uv_pipe_t stdin_pipe;
int porta_local = 8000;
char tcp_conectado = 0;

char win,casa[3][3], casa_ant[3][3],i, i_inicial,waitn_user;
int ponto_meu = 0, ponto_advrs = 0;

void write_data(uv_stream_t *dest, size_t size, uv_buf_t buf, uv_write_cb callback);
static void after_write(uv_write_t* req, int status);

void game_limpa(void) {
  fprintf(stderr,"\e[H\e[2J");
}

void game_draw(int x, int y) {
  if (casa[x][y] == '0') fprintf(stderr," ");
  if (casa[x][y] == '1') fprintf(stderr,"X");
  if (casa[x][y] == '2') fprintf(stderr,"O");
}

void game_jogo(void) {
  if(memcmp(casa,casa_ant,sizeof(casa))){
    memcpy(casa_ant,casa,sizeof(casa));
    game_limpa();
    fprintf(stderr,"PLACAR: Voce %d X Adversario %d\n\n",ponto_meu,ponto_advrs);
    fprintf(stderr,"  1   2   3\n");
    fprintf(stderr,"1 ");
    game_draw(0,0);
    fprintf(stderr," | ");
    game_draw(0,1);
    fprintf(stderr," | ");
    game_draw(0,2);
    fprintf(stderr,"\n ---+---+---\n2 ");
    game_draw(1,0);
    fprintf(stderr," | ");
    game_draw(1,1);
    fprintf(stderr," | ");
    game_draw(1,2);
    fprintf(stderr,"\n ---+---+---\n3 ");
    game_draw(2,0);
    fprintf(stderr," | ");
    game_draw(2,1);
    fprintf(stderr," | ");
    game_draw(2,2);
    if ((i%2) == 0){
      if (mode == SERVER){
        fprintf(stderr,"\nDigite as coordenadas (linha,coluna)\n");
        waitn_user = 0;
      } else {
        fprintf(stderr,"\nAguardando adversario\n");
        waitn_user = 1;
      }
    }else{
      if (mode == CLIENT){
        fprintf(stderr,"\nDigite as coordenadas (linha,coluna)\n");
        waitn_user = 0;
      } else {
        fprintf(stderr,"\nAguardando adversario\n");
        waitn_user = 1;
      }
    }
  }
}

void game_check(void) {
  int i=0;
  for (i=0;i<3;i++) { /* Horizontal */
    if (casa[i][0] == casa[i][1] && casa[i][0] == casa[i][2]) {
      if (casa[i][0] == '1') win=1;
      if (casa[i][0] == '2') win=2;
    }
  }
  for (i=0;i<3;i++) { /* Vertical */
    if (casa[0][i] == casa[1][i] && casa[0][i] == casa[2][i]) {
      if (casa[0][i] == '1') win=1;
      if (casa[0][i] == '2') win=2;
    }
  }
  if (casa[0][0] == casa[1][1] && casa[0][0] == casa[2][2]) { /* Diagonal Cima->Baixo*/
    if (casa[0][0] == '1') win=1;
    if (casa[0][0] == '2') win=2;
  }
  if (casa[0][2] == casa[1][1] && casa[0][2] == casa[2][0]) { /* Diagonal Baixo->Cima */
    if (casa[0][2] == '1') win=1;
    if (casa[0][2] == '2') win=2;
  }
}

int game_play(char *string){
  char linha, coluna;
  if((strlen(string) != 4) || (string[0] <= '0') || (string[2] <= '0') || (string[0] >= '4')|| (string[2] >= '4')){
    fprintf(stderr,"Entrada nao reconhecida (use linha coluna)\n");
    return 2;
  }
  linha = string[0] - '1';
  coluna = string[2] - '1';
  if (casa[linha][coluna] != '1' && casa[linha][coluna] != '2') {
      casa[linha][coluna] = mode + '1';
      i++;
      game_check();
    }
  else {
    fprintf(stderr,"A casa esta em uso! Jogue Novamente..\n");
    sleep(2);
    game_jogo();
  }
  return 0;
}

int game_sincroniza(void){
  char *str = malloc(sizeof(casa)+2);
  memcpy(str,casa,sizeof(casa));
  str[sizeof(casa)] = i + '0';
  str[sizeof(casa) + 1] = '\0';
  uv_buf_t *b = malloc(sizeof(uv_buf_t));
  b->base = str;
  b->len = sizeof(casa)+2;
  write_data((uv_stream_t *)tcp_out,sizeof(casa) + 2,*b,after_write);
  free(str);
  return 0;
}

int game_update(char *data){
  memcpy(casa,data,sizeof(casa));
  i = data[sizeof(casa)] - '0';
  game_check();
  return 0;
}

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

void write_data(uv_stream_t *dest, size_t size, uv_buf_t buf, uv_write_cb callback) {
    if(dest){
      write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
      req->buf = uv_buf_init((char*) malloc(size), size);
      memcpy(req->buf.base, buf.base, size);
      uv_write((uv_write_t*) req, (uv_stream_t*)dest, &req->buf, 1, callback);
    }
}

void on_close(uv_handle_t* handle) {
  fprintf(stderr,"Conexao perdida com %s. Aguardando reconectar...!\n",mode == SERVER ? "cliente" : "servidor");
  ponto_meu = ponto_advrs = 0;
  tcp_conectado = 0;
  free(handle);
}

uv_buf_t on_alloc(uv_handle_t* client, size_t suggested_size) {
  uv_buf_t buf;
  buf.base = malloc(suggested_size);
  buf.len = suggested_size;
  return buf;
}

static void after_write(uv_write_t* req, int status) {
  CHECK(status, "write");
  write_req_t *wr = (write_req_t*) req;
  free(wr->buf.base);
  free(wr);
}

static void atualiza_cb(uv_timer_t* handle, int status){
  game_jogo();
  if (i >= (9 + i_inicial) || win){
    if (win == 1 || win == 2) {
      fprintf(stderr,"\nVoce %s o jogo!\n",win == (mode + 1) ? "venceu" : "perdeu");
      if(win == (mode + 1))
        ponto_meu++;
      else
        ponto_advrs++;
    }
    else fprintf(stderr,"\nEmpate!\n");
    memset(casa,'0',sizeof(casa));
    i = (i % 2) ? 1 : 0;
    i_inicial = i;
    win = 0;
    uv_timer_stop(&tmr_atualiza);
    uv_timer_start(&tmr_atualiza, atualiza_cb, 5000, 200);
  }
  return;
}

static char ** stringsplit(char *buf, int sz, char ch) {
    int i, iniped, ped = 0;
    char **res, **pres;
    for(i = 0;;) {
        while((i < sz) && (buf[i] == ch)) i++;
        iniped = i;
        while((i < sz) && (buf[i] != ch)) i++;
        if(i <= iniped) break;
        ped++;
    }
    pres = res = calloc(ped+1, sizeof(char *));
    for(i = 0;;) {
        while((i < sz) && (buf[i] == ch)) i++;
        iniped = i;
        while((i < sz) && (buf[i] != ch)) i++;
        if(i <= iniped) break;
        *(pres++) = strndup(buf+iniped, i-iniped);
    }
    return res;
}

void on_read_tcp(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf){
    if(nread < 0) {
    uv_err_t err = uv_last_error(uv_loop);
    if (err.code != UV_EOF) {
      UVERR(err, "read");
    }
    uv_close((uv_handle_t*)tcp_out, on_close);
    if(buf.base) free(buf.base);
    return;
  }
  game_update(buf.base);
}

static void on_connect_server(uv_connect_t *connect, int status) {
    if (status == -1) {
        return;
    }
    fprintf(stderr,"Servidor conectou!\n");
    memset(casa,'0',sizeof(casa_ant));
    memset(casa_ant,'1',sizeof(casa));
    tcp_conectado = 1;
    uv_timer_start(&tmr_atualiza, atualiza_cb, 200, 200);
    uv_read_start((uv_stream_t*)tcp_out, on_alloc, on_read_tcp);
}

static void faz_conexoes_cb(uv_timer_t* handle, int status){
  if(!tcp_conectado){
    tcp_out = malloc(sizeof(*tcp_out));
    char **hostporta = stringsplit(server_host,strlen(server_host),':');
    struct sockaddr_in dest;
    uv_connect_t *conn;
    conn = calloc(1, sizeof(*conn));
    uv_tcp_init(uv_default_loop(), tcp_out);
    dest = uv_ip4_addr(hostporta[0], atoi(hostporta[1]));
    uv_tcp_connect(conn, tcp_out, dest,
        on_connect_server);
  }
  return;
}

void on_read_stdin(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
    if (nread == -1) {
        if (uv_last_error(uv_loop).code == UV_EOF) {
            uv_close((uv_handle_t*)&stdin_pipe, NULL);
        }
    }
    else {
        if (nread > 0){
            buf.base[nread] = '\0';
            if (!waitn_user){
              game_play(buf.base);
              game_sincroniza();
            }
        }
    }
    if (buf.base)
        free(buf.base);
}

void on_connect_client(uv_stream_t* server_handle, int status) {
  if(!tcp_conectado){
    int r;
    tcp_conectado = 1;
    CHECK(status, "connect");

    fprintf(stderr,"Cliente conectou\n");

    tcp_out = malloc(sizeof(*tcp_out));

    uv_tcp_init(uv_loop, tcp_out);

    r = uv_accept(server_handle, (uv_stream_t*)tcp_out);
    CHECK(r, "accept");

    memset(casa,'0',sizeof(casa));
    memset(casa_ant,'1',sizeof(casa_ant));
    i = 0;
    uv_read_start((uv_stream_t*)tcp_out, on_alloc, on_read_tcp);
    uv_timer_start(&tmr_atualiza, atualiza_cb, 200, 200);
  }
}

int main(int argc, char **argv){
  for(;;) {
      int c;
      int option_index = 0;
      static struct option long_options[] = {
          {"port", required_argument, 0, 'p'},
          {"server", required_argument, 0, 's'},
          {0, 0, 0, 0 }
      };
      c = getopt_long(argc, argv, "p:s:", long_options, &option_index);
      if(c == -1) break;
      if(c == 'p') porta_local = atoi(optarg);
      if(c == 's') {
        strcpy(server_host,optarg);
        mode = CLIENT;
      }
  }
  uv_loop = uv_default_loop();
  memset(casa,'0',sizeof(casa));
  memset(casa_ant,'1',sizeof(casa_ant));
  if (mode == SERVER) {
    server = malloc(sizeof(*server));
    int r;
    r = uv_tcp_init(uv_loop, server);
    CHECK(r, "bind");

    struct sockaddr_in address = uv_ip4_addr("0.0.0.0", porta_local);

    r = uv_tcp_bind(server, address);
    CHECK(r, "bind");
    r = uv_listen((uv_stream_t*)server, 128, on_connect_client);
    CHECK(r, "listen");
  }

  uv_pipe_init(uv_loop, &stdin_pipe, 0);
  uv_pipe_open(&stdin_pipe, 0);

  uv_read_start((uv_stream_t*)&stdin_pipe, on_alloc, on_read_stdin);

  uv_timer_init(uv_default_loop(), &tmr_atualiza);
  uv_timer_init(uv_default_loop(), &tmr_conexoes);
  if(mode == CLIENT)
    uv_timer_start(&tmr_conexoes, faz_conexoes_cb, 0, 1000);

  fprintf(stderr,"Aguardando %s...\n",mode == CLIENT ? "servidor" : "cliente");

  uv_run(uv_loop,UV_RUN_DEFAULT);
  return 0;
}
