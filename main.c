#define _DEFAULT_SOURCE

#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/**************************************************************************************************************************************/

#define CTRL_KEY(key) ((key) & 0x1f)

#define CURSOR_HOME                                                           \
  (appbuffer_append (                                                         \
      &ab, "\x1b[H",                                                          \
      3)) // doesnt have much use anymore which is great of course
#define CLEAR_SCREEN                                                          \
  (appbuffer_append (&ab, "\x1b[2J",                                          \
                     4)) // i really shouldnt have made these macros...

/**************************************************************************************************************************************/

struct editor_conf
{
  int cursor_x;
  int cursor_y;
  int screen_rows;
  int screen_cols;
  struct termios original_terminal;
};

struct editor_conf Default;
/**************************************************************************************************************************************/

void
fugly (const char *s)
{
  perror (s);
  exit (1);
}

void
restore_terminal ()
{
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &Default.original_terminal) == -1)
    fugly ("tcsetattr");
}

void
raw_mode ()
{
  if (tcgetattr (STDIN_FILENO, &Default.original_terminal) == -1)
    fugly ("tcgetattr");
  atexit (restore_terminal);
  struct termios raw = Default.original_terminal;

  cfmakeraw (&raw); // turns out i dont have to fiddle with it manually
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    fugly ("tcsetattr");
}

char
read_key ()
{
  int nread;
  char chr;
  while ((nread = read (STDIN_FILENO, &chr, 1)) != 1)
    {
      if (nread == -1 && errno != EAGAIN)
        fugly ("read");
    }
  return chr;
}

int
get_size (int *row, int *col)
{
  struct winsize ws;
  if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    return -1;
  else
    {
      *row = ws.ws_row;
      *col = ws.ws_col;
      return 0;
    };
}

/*******************APPEND BUFFER****/

struct append_buffer
{
  char *bchr;
  int len;
};

#define APPBUFFER_INIT { NULL, 0 }

void
appbuffer_append (struct append_buffer *ab, const char *s, int len)
{
  char *new = realloc (ab->bchr, ab->len + len);

  if (new == NULL)
    return;
  memcpy (&new[ab->len], s, len);
  ab->bchr = new;
  ab->len += len;
}

void
appbuffer_free (struct append_buffer *ab)
{
  free (ab->bchr);
}
/************************************/

void
draw_rows (struct append_buffer *ab)
{
  int i;
  for (i = 0; i < Default.screen_rows; i++)
    {

      /* may add a welcome message here but i just dont see why rn ngl*/

      appbuffer_append (ab, ">", 1);

      appbuffer_append (ab, "\x1b[K",
                        3); // for one line at a time approach? im
                            // not sure wtf im doing anymore...

      if (i < Default.screen_rows - 1)
        {
          appbuffer_append (ab, "\r\n", 2);
        }
    }
}

void
refresh ()
{
  struct append_buffer ab = APPBUFFER_INIT;

  appbuffer_append (&ab, "\x1b[?25L", 6); // just hides the cursor

  CURSOR_HOME;

  draw_rows (&ab);

  char buf[32];
  sprintf (buf, "\x1b[%d;%dH", Default.cursor_y + 1, Default.cursor_x + 1);
  appbuffer_append (&ab, buf, strlen (buf));

  appbuffer_append (&ab, "\x1b[?25L", 6); // just hides the cursor

  write (STDOUT_FILENO, ab.bchr, ab.len);
  appbuffer_free (&ab);
}

void
move_cursor (char key)
{
  switch (key)
    {
      // TODO
    }
}

void
process_key ()
{
  struct append_buffer ab = APPBUFFER_INIT;
  char chr = read_key ();
  switch (chr)
    {
    case CTRL_KEY ('q'):
      CLEAR_SCREEN;
      CURSOR_HOME;
      write (STDOUT_FILENO, ab.bchr, ab.len);
      appbuffer_free (&ab);
      exit (0);
      break;
    }
}

/**************************************************************************************************************************************/

void
init_editor ()
{

  Default.cursor_x = 0;
  Default.cursor_y = 0;

  if (get_size (&Default.screen_rows, &Default.screen_cols) == -1)
    fugly ("get_size");
}

/**************************************************************************************************************************************/

int
main ()
{
  raw_mode ();
  init_editor ();

  while (true)
    {
      refresh ();
      process_key ();
    }

  return 0;
}
