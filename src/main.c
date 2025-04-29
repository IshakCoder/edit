
/* Include */

#include "../lib/lib.h"
#include <fcntl.h>

/* Define */

#define FALSE 0
#define TRUE 1

#define EDITOR_TAB 4
#define EDITOR_NAME "Editor"
#define EDITOR_VERSION "1.0"

/* Typedef */

typedef struct EditorRow
{
	char* chars;
	char* render;
	int size, renderSize;
} EditorRow;

/* Struct */

struct EditorConfig
{
    char* fileName;
	char* newFileName;
    int row, col, cursorX, cursorY,
    numRow, rowOff, colOff, renderX,
    modifined, existsFile, save,
	saveError, exit;
    EditorRow* editorRow;
};

/* Variable */

struct EditorConfig editorConfig;

/* Function */

// Инициализация переменных из EditorConfig
void InitEditorConfig (void)
{
	editorConfig.rowOff = 0;
	editorConfig.colOff = 0;
	editorConfig.numRow = 0;
    editorConfig.cursorX = 0;
	editorConfig.cursorY = 0;
	editorConfig.renderX = 0;
	editorConfig.exit = FALSE;
	editorConfig.save = FALSE;
    editorConfig.fileName = NULL;
	editorConfig.editorRow = NULL;
	editorConfig.modifined = FALSE;
	editorConfig.existsFile = TRUE;
	editorConfig.saveError = FALSE;
	editorConfig.newFileName = NULL;

    RUN_FUNCTION(GetTerminalSize(&editorConfig.col, &editorConfig.row), "GetTerminalSize");
	editorConfig.row -= 2;
}

// Обновить строку (перевести знаки табуляции в пробелы и т.д.)
void UpdateRow (EditorRow* row)
{
	int tabs = 0;
	for (int j = 0; j < row->size; j++)
		if (row->chars[j] == '\t') tabs++;

	free(row->render);
	row->render = malloc(row->size + tabs * (EDITOR_TAB - 1) + 1);

	int index = 0;
	for (int j = 0; j < row->size; j++)
	{
		if (row->chars[j] == '\t')
		{
			row->render[index++] = ' ';
			while (index % EDITOR_TAB != 0) row->render[index++] = ' ';
		}
		else row->render[index++] = row->chars[j];
	}

	row->render[index] = '\0';
	row->renderSize = index;
}

// Добавить строку в файл
void InsertRow (int index, char* str, size_t len)
{
	if (index < 0 || index > editorConfig.numRow) return;

	editorConfig.editorRow = realloc(editorConfig.editorRow,
	sizeof(EditorRow) * (editorConfig.numRow + 1));

	memmove(&editorConfig.editorRow[index + 1],
	&editorConfig.editorRow[index],
	sizeof(EditorRow) * (editorConfig.numRow - index));

  	editorConfig.editorRow[index].size = len;
  	editorConfig.editorRow[index].chars = malloc(len + 1);

  	memcpy(editorConfig.editorRow[index].chars, str, len);
  	editorConfig.editorRow[index].chars[len] = '\0';

	editorConfig.editorRow[index].renderSize = 0;
	editorConfig.editorRow[index].render = NULL;
	UpdateRow(&editorConfig.editorRow[index]);

  	editorConfig.numRow++;
}

// Открытие файла
void FileOpen (void)
{
	FILE* fp = fopen(editorConfig.fileName, "r");
	if (fp == NULL) return;

    char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen = 0;

    while ((linelen = getline(&line, &linecap, fp)) != -1)
    {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
		InsertRow(editorConfig.numRow, line, linelen);
    }

    free(line);
	fclose(fp);
}

// Узнать реальнное положение курсора (т.к. табуляция считается за 4 символа)
int RowXToRenderX (EditorRow* row, int x)
{
	int renderX = 0;
	for (int j = 0; j < x; j++)
	{
		if (row->chars[j] == '\t')
			renderX += (EDITOR_TAB - 1) - (renderX % EDITOR_TAB);
		renderX++;
	}
	return renderX;
}

// Пролистывание файла
void Scroll (void)
{
	editorConfig.renderX = 0;
	if (editorConfig.cursorY < editorConfig.numRow)
		editorConfig.renderX = RowXToRenderX(&editorConfig.editorRow[editorConfig.cursorY], editorConfig.cursorX);

	if (editorConfig.cursorY < editorConfig.rowOff) editorConfig.rowOff = editorConfig.cursorY;
	if (editorConfig.cursorY >= editorConfig.rowOff + editorConfig.row)
		editorConfig.rowOff = editorConfig.cursorY - editorConfig.row + 1;

	if (editorConfig.renderX < editorConfig.colOff) editorConfig.colOff = editorConfig.renderX;
	if (editorConfig.renderX >= editorConfig.colOff + editorConfig.col)
		editorConfig.colOff = editorConfig.renderX - editorConfig.col + 1;
}

// Отрисовка верхней лини редактора
void DrawHomeBar (struct Buffer* buf)
{
	Append(buf, "\x1b[7m", 4);
	for (int i = 0; i < editorConfig.col; i++)
	{
		Append(buf, " ", 1);
		if (i == 0)
		{
			Append(buf, EDITOR_NAME, sizeof(EDITOR_NAME));
			i += sizeof(EDITOR_NAME);
			Append(buf, " ", 1);
			i += 1;
			Append(buf, EDITOR_VERSION, sizeof(EDITOR_VERSION));
			i += sizeof(EDITOR_VERSION) - 2;
		}
		else if ((int)(editorConfig.col * 0.5 - ((int)strlen(editorConfig.fileName) / 2)) == i)
		{
			Append(buf, editorConfig.fileName, strlen(editorConfig.fileName));
			i += strlen(editorConfig.fileName);
			if (editorConfig.modifined == TRUE)
			{
				Append(buf, " *", 2);
				i += 2;
			}
		}
	}
	Append(buf, "\x1b[m", 3);
}

// Отрисовка самого текста
void DrawRows (struct Buffer* buf)
{
  	for (int y = 0; y < editorConfig.row; y++)
	{
		if (y == 0) Append(buf, "\r\n", 2);
		int fileRow = y + editorConfig.rowOff;
		if (fileRow >= editorConfig.numRow)
		{
			if (editorConfig.numRow == 0 && y == editorConfig.row / 2)
			{
				int len = 0;
				if (len > editorConfig.col) len = editorConfig.col;
				int padding = (editorConfig.col - len) / 2;
				if (padding)
				{
					Append(buf, "~", 1);
					padding--;
				}
				while (padding--) Append(buf, " ", 1);
			}
			else Append(buf, "~", 1);
		}
		else
		{
			int len = editorConfig.editorRow[fileRow].renderSize - editorConfig.colOff;
			if (len < 0) len = 0;
			if (len > editorConfig.col) len = editorConfig.col;
			Append(buf, &editorConfig.editorRow[fileRow].render[editorConfig.colOff], len);
		}
		Append(buf, "\x1b[K", 3);
		Append(buf, "\r\n", 2);
	}
}

// Отрисовка нижней лини редактора
void DrawEndBar (struct Buffer* buf)
{
	Append(buf, "\x1b[7m", 4);

	if (editorConfig.exit == TRUE)
	{
		for (int i = 0; i < editorConfig.col; i++)
		{
			if (i == 0)
			{
				Append(buf, " Save changes to file (y/n): ", 29);
				i += 29;
			}
			else Append(buf, " ", 1);
		}
		editorConfig.exit = FALSE;
	}
	else if (editorConfig.saveError == TRUE) // Ошибка сохранения
	{
		char errorMessage[80];
		int len = snprintf(errorMessage, sizeof(errorMessage), " Can't save! I/O error: %s", strerror(errno));
		for (int i = 0; i < editorConfig.col; i++)
		{
			if (i == 0)
			{
				Append(buf, errorMessage, len);
				i += len - 1;
			}
			else Append(buf, " ", 1);
		}
		editorConfig.saveError = FALSE;
	}
	else if (editorConfig.existsFile == FALSE && editorConfig.save == TRUE) // Ввод имени файла
	{
		char saveMessage[] = " File name: ";
		for (int i = 0; i < editorConfig.col; i++)
		{
			if (i == 0)
			{
				Append(buf, saveMessage, sizeof(saveMessage) - 1);
				Append(buf, editorConfig.newFileName, strlen(editorConfig.newFileName));
				i += sizeof(saveMessage) - 1 + strlen(editorConfig.newFileName);
			}
			else Append(buf, " ", 1);
		}
	}
	else // Статус редактора
	{
		char status[80], line[80];
		int len = snprintf(status, sizeof(status), "Row = %i, Col = %i",
						   editorConfig.cursorY + 1, editorConfig.cursorX + 1);
		int linelen = snprintf(line, sizeof(line), "%i Rows", editorConfig.numRow);
		for (int i = 0; i < editorConfig.col; i++)
		{
			Append(buf, " ", 1);
			if (i == 0)
			{
				Append(buf, line, linelen);
				i += linelen;
			}
			else if (i + len + 2 == editorConfig.col)
			{
				Append(buf, status, len);
				i += len;
			}
		}
	}

	Append(buf, "\x1b[m", 3);
}

// Создание следующего кадра редактора
void RefreshScreen (void)
{
	Scroll();

	RUN_FUNCTION(GetTerminalSize(&editorConfig.col, &editorConfig.row), "GetTerminalSize");
	editorConfig.row -= 2;

	struct Buffer buf = BUFFER_INIT;

	Append(&buf, "\x1b[?25l", 6); // Hide cursor
  	Append(&buf, "\x1b[H", 3); // Clear screen

	DrawHomeBar(&buf);
	DrawRows(&buf);
	DrawEndBar(&buf);

	Append(&buf, "\x1b[?25h", 6); // Show cursor

  	WRITE(buf.buf, buf.len);

    SetCursor((editorConfig.renderX - editorConfig.colOff) + 1,
              (editorConfig.cursorY - editorConfig.rowOff) + 2);

	Free(&buf);
}

// Добавить новую строку
void InsertNewline (void)
{
	editorConfig.modifined = TRUE;
  	if (editorConfig.cursorX == 0) InsertRow(editorConfig.cursorY, "", 0);
	else
	{
    	EditorRow* row = &editorConfig.editorRow[editorConfig.cursorY];
    	InsertRow(editorConfig.cursorY + 1,
                  &row->chars[editorConfig.cursorX],
                  row->size - editorConfig.cursorX);
    	row = &editorConfig.editorRow[editorConfig.cursorY];
    	row->size = editorConfig.cursorX;
    	row->chars[row->size] = '\0';
    	UpdateRow(row);
  	}
  	editorConfig.cursorX = 0;
  	editorConfig.cursorY++;
}

// Перевод текста из многострочного в одну строку для удобного сохранения
char* RowsToString (int* buflen)
{
	int totlen = 0;
	for (int j = 0; j < editorConfig.numRow; j++)
		totlen += editorConfig.editorRow[j].size + 1;
	*buflen = totlen;

	char *buf = malloc(totlen);
	char *ptr = buf;

	for (int j = 0; j < editorConfig.numRow; j++)
	{
		memcpy(ptr, editorConfig.editorRow[j].chars, editorConfig.editorRow[j].size);
		ptr += editorConfig.editorRow[j].size;
		*ptr = '\n';
		ptr++;
	}
	return buf;
}

// Сохранение файла
void FileSave (void)
{
	if (editorConfig.fileName == NULL || editorConfig.modifined == FALSE) return;

	if (editorConfig.existsFile == FALSE)
	{
		editorConfig.save = TRUE;
		editorConfig.newFileName = (char*)malloc(1);
		editorConfig.newFileName[0] = '\0';

		int c = 0;
		do
		{
			if ((c == DEL_KEY || c == BACKSPACE) && strlen(editorConfig.newFileName) != 0)
			{
				int len = strlen(editorConfig.newFileName);
				editorConfig.newFileName = (char*)realloc(editorConfig.newFileName, len);
				editorConfig.newFileName[len - 1] = '\0';
			}
			else if (c > 31 && c != 127)
			{
				int len = strlen(editorConfig.newFileName);
				editorConfig.newFileName = (char*)realloc(editorConfig.newFileName, len + 2);
				editorConfig.newFileName[len] = c;
				editorConfig.newFileName[len + 1] = '\0';
			}
			CLEAR_SCREEN;
			RefreshScreen();
			SetCursor(13 + strlen(editorConfig.newFileName), editorConfig.row + 2);
			c = ReadKey();
		}
		while (c != '\r' && c != CTRL_KEY('X'));

		editorConfig.save = FALSE;
		if (c != CTRL_KEY('X')) editorConfig.fileName = editorConfig.newFileName;
		else
		{
			free(editorConfig.newFileName);
			return;
		}
	}

	int len = 0;
	char* buf = RowsToString(&len);

	int fd = open(editorConfig.fileName, O_RDWR | O_CREAT, 0644);
	if (fd != -1)
	{
		if (ftruncate(fd, len) != -1)
		{
			if (write(fd, buf, len) == len)
			{
				close(fd);
				free(buf);
				editorConfig.modifined = FALSE;
				editorConfig.existsFile = TRUE;
				return;
			}
		}
		close(fd);
	}

	editorConfig.saveError = TRUE;
 	free(buf);
}

// Логика действий при нажатии клавиши
void MoveCursor (int key)
{
	EditorRow* row = (editorConfig.cursorY >= editorConfig.numRow) ?
    NULL : &editorConfig.editorRow[editorConfig.cursorY];

	switch (key)
	{
		case ARROW_LEFT:
			if (editorConfig.cursorX != 0) editorConfig.cursorX--;
			else if (editorConfig.cursorY > 0)
				editorConfig.cursorX = editorConfig.editorRow[--editorConfig.cursorY].size;
			break;

		case ARROW_RIGHT:
			if (row && editorConfig.cursorX < row->size) editorConfig.cursorX++;
			else if (row && editorConfig.cursorX == row->size
			        && editorConfig.numRow != editorConfig.cursorY + 1)
			{
				editorConfig.cursorY++;
				editorConfig.cursorX = 0;
			}
			break;

		case ARROW_UP:
			if (editorConfig.cursorY != 0) editorConfig.cursorY--;
			break;

		case ARROW_DOWN:
			if (editorConfig.cursorY < editorConfig.numRow - 1) editorConfig.cursorY++;
			break;
	}

	row = (editorConfig.cursorY >= editorConfig.numRow) ?
    NULL : &editorConfig.editorRow[editorConfig.cursorY];

	int rowlen = row ? row->size : 0;
	if (editorConfig.cursorX > rowlen) editorConfig.cursorX = rowlen;
}

// Удалить символ из строки
void RowDelChar(EditorRow* row, int index)
{
	if (index < 0 || index >= row->size) return;
  	memmove(&row->chars[index], &row->chars[index + 1], row->size-- - index);
  	UpdateRow(row);
}

// Добавить текст в строку
void RowAppendString(EditorRow* row, char* s, size_t len)
{
  	row->chars = realloc(row->chars, row->size + len + 1);
  	memcpy(&row->chars[row->size], s, len);
  	row->size += len;
  	row->chars[row->size] = '\0';
  	UpdateRow(row);
}

// Очистка данных строки
void FreeRow (EditorRow* row)
{
	free(row->render);
	free(row->chars);
}

// Удалить строку
void DelRow (int index)
{
  	if (index < 0 || index >= editorConfig.numRow) return;
  	FreeRow(&editorConfig.editorRow[index]);
  	memmove(&editorConfig.editorRow[index],
	&editorConfig.editorRow[index + 1],
	sizeof(EditorRow) * (editorConfig.numRow - index - 1));
  	editorConfig.numRow--;
}

// Удалить символ
void DelChar (void)
{
  	if (editorConfig.cursorY == editorConfig.numRow) return;
	if (editorConfig.cursorX == 0 && editorConfig.cursorY == 0) return;

	editorConfig.modifined = TRUE;
  	EditorRow* row = &editorConfig.editorRow[editorConfig.cursorY];
  	if (editorConfig.cursorX > 0)
	{
    	RowDelChar(row, editorConfig.cursorX - 1);
    	editorConfig.cursorX--;
  	}
	else
	{
		editorConfig.cursorX = editorConfig.editorRow[editorConfig.cursorY - 1].size;
		RowAppendString(&editorConfig.editorRow[editorConfig.cursorY - 1], row->chars, row->size);
		DelRow(editorConfig.cursorY);
		editorConfig.cursorY--;
	}
}

// Добавить символ в строку
void RowInsertChar (EditorRow* row, int index, int c)
{
	if (index < 0 || index > row->size) index = row->size;
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[index + 1], &row->chars[index], row->size - index + 1);
	row->size++;
	row->chars[index] = c;
	UpdateRow(row);
}

// Добавить символ
void InsertChar (int c)
{
	editorConfig.modifined = TRUE;
	if (editorConfig.cursorY == editorConfig.numRow) InsertRow(editorConfig.numRow,"", 0);
	RowInsertChar(&editorConfig.editorRow[editorConfig.cursorY], editorConfig.cursorX, c);
	editorConfig.cursorX++;
}

// Считывание нажатия клавиш
int KeyPress (void)
{
    int c = ReadKey();
    switch (c)
    {
        case '\r':
            InsertNewline();
			break;

        case CTRL_KEY('X'):
			int c_ = 0;
			if (editorConfig.modifined == TRUE)
			{
				editorConfig.exit = TRUE;
				CLEAR_SCREEN;
				RefreshScreen();
				SetCursor(30, editorConfig.row + 2);
				while ((c_ = ReadKey()) != CTRL_KEY('X'))
				{
					if (c_ == CTRL_KEY('Y') || c_ == 'y' || c_ == 'Y')
					{
						FileSave();
						break;
					}
					else if (c_ == CTRL_KEY('N') || c_ == 'n' || c_ == 'N') break;
				}
			}
			if (c_ == CTRL_KEY('X')) return FALSE;
			return TRUE;
            break;

        case CTRL_KEY('S'):
			FileSave();
            break;

        case HOME_KEY:
      		editorConfig.cursorX = 0;
      		break;

        case END_KEY:
			if (editorConfig.cursorY < editorConfig.numRow)
				editorConfig.cursorX = editorConfig.editorRow[editorConfig.cursorY].size;
      		break;

        case BACKSPACE:
		case CTRL_KEY('h'):
		case DEL_KEY:
            if (c == DEL_KEY) MoveCursor(ARROW_RIGHT);
			DelChar();
			break;

        case PAGE_UP:
    	case PAGE_DOWN:
			if (c == PAGE_UP) editorConfig.cursorY = editorConfig.rowOff;
			else if (c == PAGE_DOWN)
			{
				editorConfig.cursorY = editorConfig.rowOff + editorConfig.col - 1;
				if (editorConfig.cursorY > editorConfig.numRow)
                    editorConfig.cursorY = editorConfig.numRow - 1;
			}
        	int times = editorConfig.row;
        	while (times--) MoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      		break;

    	case ARROW_UP:
    	case ARROW_DOWN:
        case ARROW_LEFT:
    	case ARROW_RIGHT:
     		MoveCursor(c);
            break;

        case CTRL_KEY('L'):
		case '\x1b':
			break;

        default:
			InsertChar(c);
			break;
    }
    return FALSE;
}

/* Main function */
int main (int argc, char** argv)
{
	// Инициализация
	InitEditorConfig();
	EnableRawTerminalMode();
	if (argc > 1)
	{
		editorConfig.fileName = argv[1];
		if (editorConfig.fileName != NULL) FileOpen();
	}
	else
	{
		editorConfig.existsFile = FALSE;
		editorConfig.fileName = "[New file]";
	}

	// Процесс
	WRITE("\033[?1049h", 8);
	while (RefreshScreen(), KeyPress() == FALSE);
	WRITE("\033[?1049l", 8);

	// Деинициализация
	DisableRawTerminalMode();
	for (int i = 0; i < editorConfig.numRow; i++)
	{
		free(editorConfig.editorRow[i].chars);
		free(editorConfig.editorRow[i].render);
	}
	free(editorConfig.editorRow);

	// Выход
	return EXIT_SUCCESS;
}
