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
#define LOGF(fmt, params...) printf(fmt "\n", params);
#define LOG_ERROR(msg) puts(msg);

#define SERVER 1
#define CLIENT 0

uv_tcp_t server_me, client, server;
uv_timer_t tmr_atualiza;
uv_timer_t tmr_conexoes;
char server_host[17];
uv_loop_t *uv_loop = NULL;
int mode = SERVER;
uv_pipe_t stdin_pipe, stdout_pipe;
int porta_local = 8000;
char server_conectado = 0;

char win,casa[3][3], casa_ant[3][3],i, waitn_user;

void write_data(uv_stream_t *dest, size_t size, uv_buf_t buf, uv_write_cb callback);
static void after_write(uv_write_t* req, int status);

void game_limpa(void) {
  printf("\e[H\e[2J");
  //system("clear");
}

void game_draw(int x, int y) {
  if (casa[x][y] == '{FONTE}') printf(" ");
  if (casa[x][y] == '1') printf("X");
  if (casa[x][y] == '2') printf("O");
}

void game_jogo(void) {
  if(memcmp(casa,casa_ant,sizeof(casa))){
    memcpy(casa_ant,casa,sizeof(casa));
    game_limpa();
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
  if(strlen(string) != 4)
    return 2;
  linha = string[0] - '1';
  coluna = string[2] - '1';
  if (casa[linha][coluna] != '1' && casa[linha][coluna] != '2') {
      casa[linha][coluna] = mode;
      i++;
    }
    else {
      printf("A casa esta em uso! Jogue Novamente..\n");
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
  if(mode == CLIENT)
    write_data((uv_stream_t *)&server,sizeof(casa),*b,after_write);
  else
    write_data((uv_stream_t *)&client,sizeof(casa),*b,after_write);
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

void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

void on_stdout_write(uv_write_t *req, int status) {
    free_write_req(req);
}

void on_file_write(uv_write_t *req, int status) {
    free_write_req(req);
}

void write_data(uv_stream_t *dest, size_t size, uv_buf_t buf, uv_write_cb callback) {
    write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
    req->buf = uv_buf_init((char*) malloc(size), size);
    memcpy(req->buf.base, buf.base, size);
    uv_write((uv_write_t*) req, (uv_stream_t*)dest, &req->buf, 1, callback);
}

void on_close_client(uv_handle_t* handle) {
  fprintf(stderr,"Cliente foi embora\n");
}

uv_buf_t on_alloc(uv_handle_t* client, size_t suggested_size) {
  uv_buf_t buf;
  buf.base = malloc(suggested_size);
  buf.len = suggested_size;
  return buf;
}

static void after_write(uv_write_t* req, int status) {
  CHECK(status, "write");
  free(req);
}


static void atualiza_cb(uv_timer_t* handle, int status){
  if (i > 9){
    game_jogo();
    if (win == 1 || win == 2) printf("\nJogador %d venceu o jogo!\n",win);
    else printf("\nEmpate!\n");
    memset(casa,'0',sizeof(casa));
    i = 0;
    sleep(5);
  }
  //game_sincroniza();
  game_jogo();
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

void on_close_server(uv_handle_t* handle){
  fprintf(stderr,"Servidor caiu\n");
}

void on_read_server(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf){
    if(nread < 0) {
    uv_err_t err = uv_last_error(uv_loop);
    if (err.code != UV_EOF) {
      UVERR(err, "read");
    }
    uv_close((uv_handle_t*)&server, on_close_server);
    if(buf.base) free(buf.base);
    return;
  }
  game_update(buf.base);
  fprintf(stderr,"Li: %s\n",buf.base);
}

static void on_connect_server(uv_connect_t *connect, int status) {
    if (status == -1) {
        return;
    }
    server_conectado = 1;
    uv_timer_start(&tmr_atualiza, atualiza_cb, 200, 200);
    uv_read_start((uv_stream_t*)&server, on_alloc, on_read_server);
}

static void faz_conexoes_cb(uv_timer_t* handle, int status){
  if(!server_conectado){
    char **hostporta = stringsplit(server_host,strlen(server_host),':');
    struct sockaddr_in dest;
    uv_connect_t *conn;
    conn = calloc(1, sizeof(*conn));
    uv_tcp_init(uv_default_loop(), &server);
    dest = uv_ip4_addr(hostporta[0], atoi(hostporta[1]));
    uv_tcp_connect(conn, &server, dest,
        on_connect_server);
  }
  return;
}

void on_read_stdin(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
    if (nread == -1) {
        if (uv_last_error(uv_loop).code == UV_EOF) {
            uv_close((uv_handle_t*)&stdin_pipe, NULL);
            uv_close((uv_handle_t*)&stdout_pipe, NULL);
        }
    }
    else {
        if (nread > 0){
            if (!waitn_user){
              game_play(buf.base);
              game_sincroniza();
            }
        }
    }
    if (buf.base)
        free(buf.base);
}

void on_read_client(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf) {
  if(nread < 0) {
    uv_err_t err = uv_last_error(uv_loop);
    if (err.code != UV_EOF) {
      UVERR(err, "read");
    }
    uv_close((uv_handle_t*)&client, on_close_client);
    if(buf.base) free(buf.base);
    return;
  }
  game_update(buf.base);
  fprintf(stderr,"Li: %s\n",buf.base);
}

void on_connect_client(uv_stream_t* server_handle, int status) {
  int r;
  CHECK(status, "connect");

  assert((uv_tcp_t*)server_handle == &server_me);

  fprintf(stderr,"Alguem conectou\n");

  uv_tcp_init(uv_loop, &client);

  r = uv_accept(server_handle, (uv_stream_t*)&client);
  CHECK(r, "accept");

  uv_read_start((uv_stream_t*)&client, on_alloc, on_read_client);
  uv_timer_start(&tmr_atualiza, atualiza_cb, 200, 200);
}

int main(int argc, char **argv){
  int r;

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
        strcpy(server_host,optarg);//configura_masters(optarg);
        mode = CLIENT;
      }
  }
  uv_loop = uv_default_loop();
  memset(casa_ant,'1',sizeof(casa_ant));
  r = uv_tcp_init(uv_loop, &server_me);
  CHECK(r, "bind");

  struct sockaddr_in address = uv_ip4_addr("0.0.0.0", porta_local);

  r = uv_tcp_bind(&server_me, address);
  CHECK(r, "bind");
  r = uv_listen((uv_stream_t*)&server_me, 128, on_connect_client);
  CHECK(r, "listen");
  //LOG("listening on port");

  uv_pipe_init(uv_loop, &stdin_pipe, 0);
  uv_pipe_open(&stdin_pipe, 0);
  uv_pipe_init(uv_loop, &stdout_pipe, 0);
  uv_pipe_open(&stdout_pipe, 1);

  uv_read_start((uv_stream_t*)&stdin_pipe, on_alloc, on_read_stdin);

  uv_timer_init(uv_default_loop(), &tmr_atualiza);
  //uv_timer_start(&tmr_atualiza, atualiza_cb, 200, 200);
  uv_timer_init(uv_default_loop(), &tmr_conexoes);
  if(mode == CLIENT)
    uv_timer_start(&tmr_conexoes, faz_conexoes_cb, 1000, 1000);

  fprintf(stderr,"Aguardando cliente...\n");

  uv_run(uv_loop,UV_RUN_DEFAULT);
  return 0;
}