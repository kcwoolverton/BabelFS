make:
	gcc -g -Og `pkg-config fuse --cflags --libs` fat_original.c -o fat
