#ifndef _MESSENGER_H_
#define _MESSENGER_H_

#define MASTER 1
#define SLAVE 0

struct messenger{
  char hostporta[64];
  char desejo;
};

int messenger_init(struct messenger *messenger);
int messenger_update(struct messenger *messenger,void *data,size_t sz);

#endif