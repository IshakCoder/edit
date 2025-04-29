
#ifndef __LIB__
#define __LIB__

/* Include */

#pragma once
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/* Define */

#define BUFFER_INIT { NULL, 0 }
#define CTRL_KEY(k) ((k) & 0x1f)

#define WRITE(buf, len) RUN_FUNCTION(write(STDOUT_FILENO, buf, len), "write")
#define RUN_FUNCTION(function, error_str) if (function == -1) Error(error_str)

#define CLEAR_SCREEN RUN_FUNCTION(write(STDOUT_FILENO, "\x1b[H", 3), "write")
#define CLEAR_TERMINAL RUN_FUNCTION(write(STDOUT_FILENO, "\033c", 2), "write")
#define HIDE_CURSOR RUN_FUNCTION(write(STDOUT_FILENO, "\x1b[?25l", 6), "write")
#define SHOW_CURSOR RUN_FUNCTION(write(STDOUT_FILENO, "\x1b[?25h", 6), "write")

/* Structure */

struct Buffer
{
	char* buf;
	int len;
};

/* Enum */

enum TerminalKey
{
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
  	END_KEY,
  	PAGE_UP,
  	PAGE_DOWN
};

/* Prototype */

/* Append string to the end of buffer */
int Append (struct Buffer* buf, const char* str, int len);

/* Get terminal col and row */
int GetTerminalSize (int* col, int* row);

/* Set terminal window size */
int SetTerminalSize (int row, int col);

/* Disable raw terminal mode */
void DisableRawTerminalMode (void);

/* Enable raw terminal mode */
void EnableRawTerminalMode (void);

/* Free up buffer memory */
void Free (struct Buffer* buf);

/* Print the reason for the error and terminate the program */
void Error (const char* error);

/* Set cursor coordinate in terminal */
int SetCursor (int x, int y);

/* Reads keystrokes */
int ReadKey (void);

#endif
