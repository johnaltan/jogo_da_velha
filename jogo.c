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

uv_tcp_t server, client;
char *server_host = NULL;
uv_loop_t *uv_loop = NULL;
int mode = SERVER;
uv_pipe_t stdin_pipe, stdout_pipe;
int porta_local = 8000;

int linha=0,coluna=0,win,casa[3][3];

void game_draw(int x, int y) {
  if (casa[x][y] == '{FONTE}') printf(" ");
  if (casa[x][y] == 1) printf("X");
  if (casa[x][y] == 2) printf("O");
}

int game_update(char *data, char mode){

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

void on_close(uv_handle_t* handle) {
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

void on_read_stdin(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
    if (nread == -1) {
        if (uv_last_error(uv_loop).code == UV_EOF) {
            uv_close((uv_handle_t*)&stdin_pipe, NULL);
            uv_close((uv_handle_t*)&stdout_pipe, NULL);
        }
    }
    else {
        if (nread > 0){
            game_update(buf.base, 0);
            //write_data((uv_stream_t*)&stdout_pipe, nread, buf, on_stdout_write);
        }
    }
    if (buf.base)
        free(buf.base);
}

void on_read(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf) {
  int readed = 0;
  if(nread < 0) {
    uv_err_t err = uv_last_error(uv_loop);
    if (err.code != UV_EOF) {
      UVERR(err, "read");
    }
    uv_close((uv_handle_t*)&client, on_close);
    if(buf.base) free(buf.base);
    return;
  }
  game_update(buf.base, 1);
}

void on_connect(uv_stream_t* server_handle, int status) {
  int r;
  CHECK(status, "connect");

  assert((uv_tcp_t*)server_handle == &server);

  fprintf(stderr,"Alguem conectou\n");

  uv_tcp_init(uv_loop, &client);

  r = uv_accept(server_handle, (uv_stream_t*)&client);
  CHECK(r, "accept");

  uv_read_start((uv_stream_t*)&client, on_alloc, on_read);
}

static void atualiza_tela_cb(uv_timer_t* handle, int status){
  game_jogo();
  return;
}

static void faz_conexoes_cb(uv_timer_t* handle, int status){
  return;
}

int main(int argc, char **argv){
  int r;
  uv_timer_t tmr_atualiza;
  uv_timer_t tmr_conexoes;

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
        server_host = malloc(sizeof(optarg));
        strcpy(server_host,optarg);//configura_masters(optarg);
        mode = CLIENT;
      }
  }
  uv_loop = uv_default_loop();

  r = uv_tcp_init(uv_loop, &server);
  CHECK(r, "bind");

  struct sockaddr_in address = uv_ip4_addr("0.0.0.0", porta_local);

  r = uv_tcp_bind(&server, address);
  CHECK(r, "bind");
  r = uv_listen((uv_stream_t*)&server, 128, on_connect);
  CHECK(r, "listen");
  //LOG("listening on port");

  uv_pipe_init(uv_loop, &stdin_pipe, 0);
  uv_pipe_open(&stdin_pipe, 0);
  uv_pipe_init(uv_loop, &stdout_pipe, 0);
  uv_pipe_open(&stdout_pipe, 1);

  uv_read_start((uv_stream_t*)&stdin_pipe, on_alloc, on_read_stdin);

  uv_timer_init(uv_default_loop(), &tmr_atualiza);
  uv_timer_start(&tmr_atualiza, atualiza_tela_cb, 10000, 10000);
  uv_timer_init(uv_default_loop(), &tmr_conexoes);
  uv_timer_start(&tmr_conexoes, faz_conexoes_cb, 1000, 1000);

  fprintf(stderr,"Aguardando clientes...\n");

  uv_run(uv_loop,UV_RUN_DEFAULT);
  return 0;
}

