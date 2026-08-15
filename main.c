#define _DEFAULT_SOURCE

#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/**************************************************************************************************************************************/

#define CTRL_KEY(key) ((key) & 0x1f)

#define CURSOR_HOME (write(STDOUT_FILENO, "\x1b[H", 3))
#define CLEAR_SCREEN write(STDOUT_FILENO, "\x1b[2J", 4);

/**************************************************************************************************************************************/

struct editor_conf {
  int screen_rows;
  int screen_cols;
  struct termios original_terminal;
};

struct editor_conf Defalult;
/**************************************************************************************************************************************/

void fugly(const char *s) {
  perror(s);
  exit(1);
}

void restore_terminal() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &Defalult.original_terminal) == -1)
    fugly("tcsetattr");
}

void raw_mode() {
  if (tcgetattr(STDIN_FILENO, &Defalult.original_terminal) == -1)
    fugly("tcgetattr");
  atexit(restore_terminal);
  struct termios raw = Defalult.original_terminal;

  cfmakeraw(&raw); // turns out i dont have to fiddle with it manually
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    fugly("tcsetattr");
}

char read_key() {
  int nread;
  char chr;
  while ((nread = read(STDIN_FILENO, &chr, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      fugly("read");
  }
  return chr;
}

int get_size(int *row, int *col) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    return -1;
  else {
    *row = ws.ws_row;
    *col = ws.ws_col;
    return 0;
  };
}

void draw_rows() {
  for (int i = 0; i < Defalult.screen_rows; i++)
    write(STDOUT_FILENO, ">\r\n", 3);
}

void refresh() {
  CLEAR_SCREEN;
  CURSOR_HOME;

  draw_rows();

  CURSOR_HOME;
}

void process_key() {
  char chr = read_key();
  switch (chr) {
  case CTRL_KEY('q'):
    CLEAR_SCREEN;
    CURSOR_HOME;
    exit(0);
    break;
  }
}

/**************************************************************************************************************************************/

void init_editor() {
  if (get_size(&Defalult.screen_rows, &Defalult.screen_cols) == -1)
    fugly("get_size");
}

/**************************************************************************************************************************************/

int main() {
  raw_mode();
  init_editor();

  while (true) {
    refresh();
    process_key();
  }

  return 0;
}
