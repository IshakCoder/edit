/* Include */
#include "lib.h"

/* Variable */
struct termios canonConfig;

/* Function */

int Append (struct Buffer* buf, const char* str, int len)
{	
	char* newStr = realloc(buf->buf, buf->len + len);
	if (newStr == NULL) return -1;

	memcpy(&newStr[buf->len], str, len);
	buf->buf = newStr;
	buf->len += len;
	return 0;
}

int GetTerminalSize (int* col, int* row)
{
  	struct winsize ws;
  	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) return -1;
	else
	{
    	*col = ws.ws_col;
    	*row = ws.ws_row;
    	return 0;
  	}
}

int SetTerminalSize (int row, int col)
{
	char terminal[32];
  	snprintf(terminal, sizeof(terminal), "\x1b[8;%i;%it", row, col);
	return write(STDOUT_FILENO, terminal, strlen(terminal));
}

void DisableRawTerminalMode (void)
{
	RUN_FUNCTION(tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonConfig), "tcsetattr");
}

void EnableRawTerminalMode (void)
{
	RUN_FUNCTION(tcgetattr(STDIN_FILENO, &canonConfig), "tcsetattr");
	atexit(DisableRawTerminalMode);

	struct termios raw = canonConfig;

	raw.c_cflag |= (CS8);
	raw.c_oflag &= ~(OPOST);
  	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	raw.c_cc[VMIN] = 0;
  	raw.c_cc[VTIME] = 1;

	RUN_FUNCTION(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw), "tcsetattr");
}

void Free (struct Buffer* buf)
{
	free(buf->buf);
}

void Error (const char* error)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
  	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(error);
	exit(EXIT_FAILURE);
}

int SetCursor (int x, int y)
{
	char cursor[32];
  	snprintf(cursor, sizeof(cursor), "\x1b[%d;%dH", y, x);
	return write(STDOUT_FILENO, cursor, strlen(cursor));
}

int ReadKey (void)
{
	char c;
	int nread;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
		if (nread == -1 && errno != EAGAIN) Error("read");
	if (c == '\x1b')
	{
    	char seq[3];

    	if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    	if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    	if (seq[0] == '[')
		{
			if (seq[1] >= '0' && seq[1] <= '9')
			{
        		if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        		if (seq[2] == '~')
				{
          			switch (seq[1])
					{
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
            			case '4': return END_KEY;
            			case '5': return PAGE_UP;
            			case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
            			case '8': return END_KEY;
          			}
        		}
			}
			else
			{
      			switch (seq[1])
				{
        			case 'A': return ARROW_UP;
        			case 'B': return ARROW_DOWN;
        			case 'C': return ARROW_RIGHT;
     		   		case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
          			case 'F': return END_KEY;
   		   		}
			}
		}
		else if (seq[0] == 'O')
		{
			switch (seq[1])
			{
        		case 'H': return HOME_KEY;
        		case 'F': return END_KEY;
      		}
		}
    	return '\x1b';
  	}
	else return c;
}
