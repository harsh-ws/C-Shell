#include "unistd.h"
#include "termios.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"

struct termios orig_termios;

void disableRawMode(void){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode(void){

    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;

    //disabling IXON disables CTRL+S (stops data flow) & CTRL+Q (resumes data flow) which controls the flow control. 
    //disable ICRNL disables carriage returns
    raw.c_lflag &= ~(IXON | ICRNL);

    // disables the Echo mode, Canonical mode (switches to byte by byte rather than line by line)
    // disabling ISIG disables the program termination by CTRL+C & CTRL+Z
    // disabling IEXTEN fixes CTRL+O in mac & disables wait for extra character in CTRL+V
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(){
    enableRawMode();

    char c;

    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
        if (iscntrl(c)){
            printf("%d\n", c);
        }
        else{
            printf("%d (%c)\n",c, c);
        }
    }
    return 0;
}
