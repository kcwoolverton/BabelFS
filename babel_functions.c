#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <math.h>

const size_t NUMCHARS = 29;
const size_t BASE = 36;
char* letters = "abcdefghijklmnopqrstuvwxyz, .";
char digs[36] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char int2baseDigits[100];
const int length_of_page = 3239;


// converted from python at
// https://github.com/cakenggt/Library-Of-Pybel/blob/master/library_of_babel.py


static char* toText(int x) {
    int sign;
    char* digits;
    if (x < 0) {
        sign = -1;
    }
    else if (x == 0) {
        return &letters[0];
    }
    else {
        sign = 1;
    }
    x = x * sign;
    while (x > 0) {
        digits = strcat(&letters[x % 29], digits);
	x = (int) x / 29;
    }
    if (sign == -1) {
        char* newChar = "-";
	digits = strcat(newChar, digits);
    }
    return digits;
}


static char* int2base(int x) {
    int sign;

    if (x < 0) {
        sign = -1;
    }
    else if (x == 0) {
        return &digs[0];
    }
    else {
        sign = 1;
    }
    x = sign * x;
    char* digits = "";
    char* neg = "-";
    if (sign < 0) {
        digits = strcat(neg, digits);
    }
    while (x > 0) {
        char charDigit = digs[x % BASE];
        char digit[2];
        digit[0] = charDigit;
        digit[1] = '\0';
        digits = strcat(digit, digits);
        x = (int) x / BASE;
    }
    strcpy(int2baseDigits, digits);
    return int2baseDigits;
}


static int stringToNumber(char* string) {
    int result = 0;
    int i = 0;
    size_t strSize = strlen(string);
    for (i = 0; i < strSize; ++i) {
        char* ind = index(letters, string[strSize - i - 1]);
	int indLoc = (int) (ind - letters); // yay pointer math
	result = result + indLoc * pow(29, i);
    }
    return result;
}

static double random01() {
    printf("%s\n", "in random01()");
    double random;
    double rand_max; 
    random = (double)rand();
    printf("%s\n", "random set");
    rand_max = (double)RAND_MAX;
    printf("%s\n", "rand_max set");
    return random/rand_max;
}

static int intRandomDig(int digLength) {
    return (int)(random01() * digLength);
}

// We got this function from: https://stackoverflow.com/questions/8465006/how-do-i-concatenate-two-strings-in-c
char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the null-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

// We got this function from: https://ubuntuforums.org/showthread.php?t=1016188
void append(char* s, char c)
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

static char* search(char* search_str) {
    printf("%s\n", "check0");
    srand(time(NULL));
    printf("%s\n", "check0.5");
    double* wallRandom;
    double* shelfRandom;
    double* volumeRandom;
    double* pageRandom;
    double* depthRandom;

    int loc_int;
    int depth;
    int loc_mult;

    char* wall;
    char* shelf;
    char* volume;
    char* volumeNumber;
    char* page;
    char* pageNumber;
    char* loc_str;

    char an[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char digs[] = "abcdefghijklmnopqrstuvwxyz, .";

    printf("%s\n", "check0.6");

    int anLength = strlen(an);
    int digsLength = strlen(digs);

    char* front_padding;
    char* back_padding;
    char* cat_str;
    char* return_str;
    char* hex_addr;
    char* key_str;

    size_t i;

    printf("%s\n", "check0.7");

    *wallRandom = random01() * 4;
    printf("%s\n", "check0.75");
    (*shelfRandom) = random01() * 5;
    (*volumeRandom) = random01() * 32;
    (*pageRandom) = random01() * 410;
    (*depthRandom) = random01();

    printf("%s\n", "check0.8");

    strcpy(volume, "00");
    strcpy(page, "000");

    depth = (*depthRandom) * (length_of_page - strlen(search_str));
    loc_mult = pow(30, length_of_page);

    printf("%s\n", "check1");

    wall = (char*) wallRandom;
    shelf = (char*) shelfRandom;
    volumeNumber = (char*) volumeRandom;
    pageNumber = (char*) pageRandom;

    printf("%s\n", "check2");

    strcpy(volume, volume + strlen(volume) - strlen(volumeNumber));
    strcpy(page, page + strlen(page) - strlen(pageNumber));

    printf("%s\n", "check3");

    for(i = 0; i < depth; i++) {
        append(front_padding, digs[intRandomDig(strlen(digs))]);
    }

    for(i = 0; i < (length_of_page - (depth + strlen(search_str))); i++) {
        append(back_padding, digs[intRandomDig(strlen(digs))]);
    }

    printf("%s\n", "check4");

    cat_str = concat(front_padding, search_str);
    return_str = concat(cat_str, back_padding);

    hex_addr = int2base(stringToNumber(search_str) + (loc_int * loc_mult));

    printf("%s\n", "check5");

    append(hex_addr, ':');
    key_str = concat(key_str, wall);
    append(key_str, ':');
    key_str = concat(key_str, shelf);
    append(key_str, ':');
    key_str = concat(key_str, volume);
    append(key_str, ':');
    key_str = concat(key_str, page);

    free(wallRandom);
    free(shelfRandom);
    free(volumeRandom);
    free(pageRandom);
    free(depthRandom);

    free(wall);
    free(shelf);
    free(volume);
    free(volumeNumber);
    free(page);
    free(pageNumber);
    free(loc_str);
    free(front_padding);
    free(back_padding);
    free(cat_str);
    free(return_str);
    free(hex_addr);

    return key_str;
}

static void runTests() {
    char* test1 = "a";
    char* test2 = "ba";
    int result1 = stringToNumber(test1);
    printf("%d\n", result1); //should be 0
    int result2 = stringToNumber(test2);
    printf("%d\n", result2); //should be 29
    char* result3 = int2base(4);
    printf("%s\n", result3); //should be 4
    char* result4 = int2base(10);
    printf("%s\n", result4); //should be A
    printf("%s\n", "About to run search.");
    char* result5 = search("test");
    printf("%s\n", result5); //should contain "test"
    return;
}

int main(int argc, char *argv[]) {
    runTests();
    return 0;
}
