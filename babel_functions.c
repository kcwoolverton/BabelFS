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
char toTextDigits[100];
const int length_of_page = 3239;

// algorithm converted from python at
// https://github.com/cakenggt/Library-Of-Pybel/blob/master/library_of_babel.py

char *baseConvert(int base, int num) {
    // adapted from https://stackoverflow.com/a/19073176
    char charSet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    int i;
    char buf[66]; //64 bits

    // boundary check
    if (base < 2 || base > 62)
        // TODO: consider throwing an error here
        return NULL;
    if (!num)
        return strdup("0"); // base is 0 case, throw error

    buf[65] = '\0';
    i = 65;
    if (num > 0) {
        while (num) {
            buf[--i] = charSet[num % base];
            num /= base;
        }
    } else {
        while(num) {
            buf[--i] = charSet[-(num % base)];
            num /= base;
        }
        buf[--i] = '-';
    }
    return strdup(buf + i);
}

static char *toText(int x) {
    int sign;
    char* digits = "";

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
        char charDigit = digs[x % BASE];
        char digit[2];
        digit[0] = charDigit;
        digit[1] = '\0';
        digits = strcat(digits, digit);
        x = (int) x / 29;
    }
    if (sign == -1) {
        char* newChar = "-";
        digits = strcat(newChar, digits);
    }
    strcpy(toTextDigits, digits);
    return toTextDigits;
}


static char *int2base(int x) {
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
        printf("Before adding the negative sign");
        digits = strcat(neg, digits);
        printf("After adding the negative sign");
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


static long long stringToNumber(char* string) {
    long long result = 0;
    size_t i;
    size_t strSize = strlen(string);
    for (i = 0; i < strSize; ++i) {
        char* ind = strchr(letters, string[strSize - i - 1]);
        int indLoc = (ind - letters); // yay pointer math
        result = result + indLoc * pow(29, i);
        printf("result is %lli\n", result);
    }
    return result;
}

char *getPage(const char *address) {
    const size_t addr_len = strlen(address);
    size_t vol_len;
    size_t page_len;
    int i;
    int j;
    char volume[2];
    char page[3];
    int loc_int;
    int page_int;
    int volume_int;
    int shelf_int;
    int wall_int;
    //TODO, this needs initialized length
    char *key_str;
    int key;
    int loc_mult;
    //TODO, this needs initialized length
    char *str_36;
    //TODO, this needs initialized length
    char *result;
    int rand_index;
    int digs_len;
    char new_addr[addr_len];
    strcpy(new_addr, address);
    // address.split(':')
    const char colon[2] = ":";
    char *hex_addr = strtok(new_addr, colon);
    char *wall = strtok(NULL, colon);
    char *shelf = strtok(NULL, colon);
    char *temp_volume = strtok(NULL, colon);
    char *temp_page = strtok(NULL, colon);

    //volume = volume.zfill(2) & page = page.zfill(3)
    vol_len = strlen(temp_volume);
    page_len = strlen(temp_page);

    j = vol_len;
    for (i = 1; i >= 0; i--) {
        if (vol_len > 0) {
            volume[i] = temp_volume[j];
        }
        else {
            volume[i] = '0';
        }
        --j;
    }
    j = page_len;
    for (i = 2; i >= 0; i--) {
        if (page_len > 0) {
            page[i] = temp_page[j];
        }
        else {
            page[i] = '0';
        }
        --j;
    }

    //loc_int = int(page+volume+shelf+wall)
    page_int = atoi(page);
    volume_int = atoi(volume);
    shelf_int = atoi(shelf);
    wall_int = atoi(wall);
    loc_int = page_int + volume_int + shelf_int + wall_int;

    //key = int(hex_addr, 36) & key -= loc_int*loc_mult
    loc_mult = pow(30, length_of_page);
    // TODO: need strcpy
    key_str = baseConvert(36, stringToNumber(hex_addr));
    key = stringToNumber(key_str);
    key = key - (loc_int*loc_mult);

    //str_36 = int2base(key, 36) & result = toText(int(str_36, 36))
    str_36 = int2base(key);
    int baseConverted = atoi(baseConvert(36, atoi(str_36)));
    result = toText(baseConverted);
    if (strlen(result) < length_of_page) {
        // seed pseudorandom generator with the result
        srand(stringToNumber(result));
        digs_len = strlen(digs);
        while (strlen(result) < length_of_page) {
            rand_index = (double)rand() / (double)((unsigned)RAND_MAX + 1) * digs_len;
            char charDigit = digs[rand_index];
            char digit[2];
            digit[0] = charDigit;
            digit[1] = '\0';
            result = strcat(result, digit);
        }
    }
    else if (strlen(result) > length_of_page) {
        char *temp = "";
        char *pointerToEnd = &result[strlen(result) - 1];
        strcpy(temp, pointerToEnd - length_of_page);
        strcpy(result, temp);
    }
    return result;
}




/*
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
*/

static void runTests() {

    char* test1 = "a";
    char* test2 = "ba";
    char* test3 = "hello kitty";
    //char* test7 = "................................................."; //TODO - implement test7

    long long result1 = stringToNumber(test1);
    printf("Should be 0: %lli\n", result1);

    long long result2 = stringToNumber(test2);
    printf("Should be 29: %lli\n", result2);


    //TODO: fix (test case returns l instead of Hello Kitty)
    long long test3Num = stringToNumber(test3);
    printf("checkpoint 0\n");
    printf("%lli\n", test3Num);
    char* test3S = int2base(test3Num);
    printf("checkpoint 1\n");
    int test3ConvNumIsh = atoi(test3S);
    printf("checkpoint 1.5\n");
    int test3ConvNum = baseConvert(36,test3ConvNumIsh);
    printf("checkpoint 2\n");
    char* result3 = toText(test3ConvNum);
    printf("Should be hello kitty: %s\n", result3);

    // int result4a = strlen(getPage(test4a));
    // printf("Should be length_of_page (so 3239): %d\n", result4a);
    //
    // int result4b = strlen(getPage(test4b));
    // printf("Should be length_of_page (so 3239): %d\n", result4b);
    //
    // char* result5 = int2base(4);
    // printf("Should be 4: %s\n", result5);
    //
    // char* result6 = int2base(10);
    // printf("Should be A; %s\n", result6);

    //strstr(getPage(search(test7)), test7) != NULL;

    return;
}

int main(int argc, char *argv[]) {
    runTests();
    return 0;
}
