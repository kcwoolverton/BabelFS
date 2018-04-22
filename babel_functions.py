import string
import random
import sys
import os


length_of_page = 3239
loc_mult = pow(30, length_of_page)
title_mult = pow(30, 25)

#29 output letters: alphabet plus comma, space, and period
#alphanumeric in hex address (base 36): 3260
#in wall: 4
#in shelf: 5
#in volumes: 32
#pages: 410
#letters per page: 3239
#titles have 25 char

help_text = '''
--checkout <addr> - Checks out a page of a book. Also displays the page's title.
--fcheckout <file>   Does exactly the search does, but with address in the file.
--search <'text'> - Does 3 searches for the text you input:
>Page contains: Finds a page which contains the text.
>Page only contains: Finds a page which only contains that text and nothing else.
>Title match: Finds a title which is exactly this string. For a title match, it will only match the first 25 characters. Addresses returned for title matches will need to have a page number added to the tail end, since they lack this.
Mind the quotemarks.
--fsearch <file> - Does exactly the search does, but with text in the file.
--file <file> - Dump rusult into the file
--help (or help, or nothing, or word salad) - Prints this message'''





def text_prep(text):
    digs = set('abcdefghijklmnopqrstuvwxyz, .')
    prepared = ''
    for letter in text:
        if letter in digs:
            prepared += letter
        elif letter.lower() in digs:
            prepared += letter.lower()
        elif letter == '\n':
            prepared += ' '
    return prepared



def arg_check(input_array):
    coms = {'--checkout': [0, None],
            '--search': [0, None],
             '--test': [0, None],
             '--fsearch': [0, None],
             '--fcheckout': [0, None],
             '--file': [0, None]}
    try:
        for argv in input_array[1:]:
            if argv == '--checkout':
                coms['--checkout'][0] = 1
                coms['--checkout'][1] = input_array[input_array.index(argv) + 1]
            if argv == '--search':
                coms['--search'][0] = 1
                coms['--search'][1] = input_array[input_array.index(argv) + 1]
            if argv == '--test':
                coms['--test'][0] = 1
            if argv == '--fsearch':
                coms['--fsearch'][0] = 1
                coms['--fsearch'][1] = input_array[input_array.index(argv) + 1]
            if argv == '--fcheckout':
                coms['--fcheckout'][0] = 1
                coms['--fcheckout'][1] = input_array[input_array.index(argv) + 1]
            if argv == '--file':
                coms['--file'][0] = 1
                coms['--file'][1] = input_array[input_array.index(argv) + 1]
        if input_array[1:] is not False or len(input_array) == 1:
            in_coms = False
            for inp in input_array[1:]:
                if inp in coms:
                    in_coms = True
            if not in_coms:
                print(help_text)
    except Exception as e:
        print('Due to \'' + str(e) + ' error\' read this:')
        print(help_text)
        sys.exit()
    return coms


def filed(input_dict, text):
    if input_dict['--file'][0]:
        with open(input_dict['--file'][1], 'w') as file:
            file.writelines(text)
        print('\nFile '+ input_dict['--file'][1] + ' was writen')



def test():
    assert stringToNumber('a') == 0, stringToNumber('a')
    assert stringToNumber('ba') == 29, stringToNumber('ba')
    assert len(getPage('asaskjkfsdf:2:2:2:33')) == length_of_page, len(getPage('asasrkrtjfsdf:2:2:2:33'))
    assert 'hello kitty' == toText(int(int2base(stringToNumber('hello kitty'), 36), 36))
    assert int2base(4, 36) == '4', int2base(4, 36)
    assert int2base(10, 36) == 'A', int2base(10, 36)
    test_string = '.................................................'
    assert test_string in getPage(search(test_string))
    print ('Tests completed')

def main():
    print("PYTHON")
    while True:
        # TODO: Pseudocode
        asker = os.open("ask")
        input_str = read(asker, 2 * 4096)
        os.close(asker)
        if len(input_str) != 0:
            print("input_str is: " + repr(input_str))
            # This is the signal from the C code that it is time to stop
            if input_str == "?":
                break
            input_str = input_str.rstrip('\n\x00')
            print("after rstrip, input_str is: " + input_str)
            if input_str[0] == 'u':
                search_str = search(input_str[1:])
                print("search str is: " + search_str)
                final_str = search_str + '\n'
            elif input_str[0] == 'e':
                if len(input_str) == 1:
                    final_str = ""
                else:
                    print("In e else block.")
                    print("Length of string is: " + str(len(input_str)))
                    get_page_str = getPage(input_str[1:])
                    print("getpage str is: " + get_page_str)
                    get_page_str = get_page_str.rstrip()
                    final_str = get_page_str + '\n'
            print("final str is: " + final_str)
            # pad the length of the string to be 2 * block_length
            current_length = len(final_str)
            final_str = final_str + (' ' * ((2 * 4096) - current_length))

            print("Modified final str is: " + final_str)
            print("Modified length of final str i: " + str(len(final_str)))
            answer = os.open("ans")
            write(answer, final_str)
            os.close(answer)

def search(search_str):
    wall = str(int(random.random()*4))
    shelf = str(int(random.random()*5))
    volume = str(int(random.random()*32)).zfill(2)
    page = str(int(random.random()*512)).zfill(3)
    #the string made up of all of the location numbers
    loc_str = page + volume + shelf + wall
    loc_int = int(loc_str) #make integer
    an = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    digs = 'abcdefghijklmnopqrstuvwxyz, .'
    hex_addr = ''
    # Making random padding that goes after the text.
    back_padding = ''
    for x in xrange(length_of_page-len(search_str)):
        back_padding += ' '
    search_str = search_str + back_padding
    # Change to base 36 and add loc_int, then make string.
    hex_addr = int2base(stringToNumber(search_str)+(loc_int*loc_mult), 36)
    key_str = hex_addr + ':' + wall + ':' + shelf + ':' + volume + ':' + page
    page_text = getPage(key_str)
    assert page_text == search_str, '\npage text:\n'+page_text+'\nstrings:\n'+search_str
    return key_str

def getTitle(address):
    addressArray = address.split(':')
    hex_addr = addressArray[0]
    wall = addressArray[1]
    shelf = addressArray[2]
    volume = addressArray[3].zfill(2)
    loc_int = int(volume+shelf+wall)
    key = int(hex_addr, 36)
    key -= loc_int*title_mult
    str_36 = int2base(key, 36)
    result = toText(int(str_36, 36))
    if len(result) < 25:
        #adding pseudorandom chars
        random.seed(result)
        digs = 'abcdefghijklmnopqrstuvwxyz, .'
        while len(result) < 25:
            result += digs[int(random.random()*len(digs))]
    elif len(result) > 25:
        result = result[-25:]
    return result

def searchTitle(search_str):
    wall = str(int(random.random()*4))
    shelf = str(int(random.random()*5))
    volume = str(int(random.random()*32)).zfill(2)
    #the string made up of all of the location numbers
    loc_str = volume + shelf + wall
    loc_int = int(loc_str) #make integer
    hex_addr = ''
    search_str = search_str[:25].ljust(25)
    hex_addr = int2base(stringToNumber(search_str)+(loc_int*title_mult), 36) #change to base 36 and add loc_int, then make string
    key_str = hex_addr + ':' + wall + ':' + shelf + ':' + volume
    assert search_str == getTitle(key_str)
    return key_str

def getPage(address):
    hex_addr, wall, shelf, volume, page = address.split(':')
    volume = volume.zfill(2)
    page = page.zfill(3)
    loc_int = int(page+volume+shelf+wall)
    key = int(hex_addr, 36)
    key -= loc_int*loc_mult
    str_36 = int2base(key, 36)
    result = toText(int(str_36, 36))
    if len(result) < length_of_page:
        #adding pseudorandom chars
        random.seed(result)
        digs = 'abcdefghijklmnopqrstuvwxyz, .'
        while len(result) < length_of_page:
            result += digs[int(random.random()*len(digs))]
    elif len(result) > length_of_page:
        result = result[-length_of_page:]
    return result

def toText(x):
    digs = 'abcdefghijklmnopqrstuvwxyz, .'
    if x < 0: sign = -1
    elif x == 0: return digs[0]
    else: sign = 1
    x *= sign
    digits = []
    while x:
        digits.append(digs[x % 29])
        x /= 29
    if sign < 0:
        digits.append('-')
    digits.reverse()
    return ''.join(digits)

def stringToNumber(iString):
    digs = 'abcdefghijklmnopqrstuvwxyz, .'
    result = 0
    for x in xrange(len(iString)):
        result += digs.index(iString[len(iString)-x-1])*pow(29,x)
    return result

def int2base(x, base):
    digs = string.digits + 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    if x < 0: sign = -1
    elif x == 0: return digs[0]
    else: sign = 1
    x *= sign
    digits = []
    while x:
        digits.append(digs[x % base])
        x /= base
    if sign < 0:
        digits.append('-')
    digits.reverse()
    return ''.join(digits)

if __name__ == "__main__":
    main()
