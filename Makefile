make:
	gcc -g -Og `pkg-config fuse --cflags --libs` fat.c -o fat
