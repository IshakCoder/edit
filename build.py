
import os
os.system("gcc -c -Oz -Wall -Wextra -Werror -o lib/lib.o lib/lib.c")
os.system("gcc -Oz -Wall -Wextra -Werror -o bin/edit src/main.c lib/lib.o")
