/***-------------- INCLUDES ---------------***/

#include "unistd.h"
#include "termios.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"
#include "errno.h"


/***---------------- DEFINES ------------------***/

#define CTRL_KEY(k) ((k) & 0x1f)


/***---------------- DATA ------------------***/

struct termios orig_termios;


/***-------------- FUNCTIONS ---------------***/

void crash(const char *s){
    // perror() looks at the global errno variable and prints a descriptive error message for it
    perror(s);
    exit(1);
}

char readKeypress(){
    int kread;
    char ch;
    while ((kread = read(STDIN_FILENO, &ch, 1)) != 1){
        //EAGAIN is returned after timout hence we ignore it as an error
        if (kread == -1 && errno != EAGAIN)
            crash("read");
    }
    return ch;
}

void processKeypress(){
    char ch = readKeypress();
    switch (ch){
    // making the program quit on pressing Ctrl+Q
    case CTRL_KEY('q'):
        exit(0);
        break;
    }
}

void disableRawMode(void){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        crash("tcsetattr");
}

void enableRawMode(void){

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        crash("tcgetattr");

    atexit(disableRawMode);

    struct termios raw = orig_termios;

    //disabling IXON disables CTRL+S (stops data flow) & CTRL+Q (resumes data flow) which controls the flow control. 
    //disable ICRNL disables carriage returns
    // When BRKINT is ON, a break condition causes SIGINT which is similar to CTRL+C
    // ISTRIP flips the 8th bit of each input byte, hence disabling it. (likley it is disabled by default)
    raw.c_lflag &= ~(IXON | ICRNL | ISTRIP | BRKINT | INPCK);

    // disables the Echo mode, Canonical mode (switches to byte by byte rather than line by line)
    // disabling ISIG disables the program termination by CTRL+C & CTRL+Z
    // disabling IEXTEN fixes CTRL+O in mac & disables wait for extra character in CTRL+V
    // INPCK enables parity checking, we dont need this
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    //sets the character size (CS) to 8bits per byte. (Already set on most systems)
    raw.c_cflag |= (CS8);

    // disabling the carriage returns on new line
    raw.c_lflag &= ~(OPOST);

    // VMIN sets the min no of bytes of input needed before read() can return
    // VTIME sets max time to wait before read() returns [in ms generally 100/1000], it returns
    // 0 on timeout
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    // pass the modified termios structure to tcsetattr to write the new terminal attributes
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        crash("tcsetattr");
}


/***-------------- INIT ---------------***/

int main(){
    enableRawMode();

    while (1){
        processKeypress();
    }
    return 0;
}
