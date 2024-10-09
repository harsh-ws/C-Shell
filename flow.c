/***-------------- INCLUDES ---------------***/

#include "unistd.h"
#include "termios.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"
#include "errno.h"
#include "sys/ioctl.h"

/***---------------- DEFINES ------------------***/

#define CTRL_KEY(k) ((k) & 0x1f)


/***---------------- DATA ------------------***/
struct editorConfiguration{
    struct termios orig_termios;
    int screenrows;
    int screencols;
};

struct windowSize{
    int ws_col;
    int ws_row;
};

struct editorConfiguration E;


/***-------------- FUNCTIONS ---------------***/

void crash(const char *s){
    write(STDOUT_FILENO, "\x1b[2J",4);
    write(STDOUT_FILENO, "\x1b[H",3);
    
    // perror() looks at the global errno variable and prints a descriptive error message for it
    perror(s);
    exit(1);
}

void disableRawMode(void){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        crash("tcsetattr");
}
int getWindowSize(int *rows, int *cols){
    struct windowSize ws;

    if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) 
            return -1;

    return getCursorPos(rows, cols);
    }
    else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
void enableRawMode(void){

    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        crash("tcgetattr");

    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    //disabling IXON disables CTRL+S (stops data flow) & CTRL+Q (resumes data flow) which controls the flow control. 
    //disable ICRNL disables carriage returns
    // When BRKINT is ON, a break condition causes SIGINT which is similar to CTRL+C
    // ISTRIP flips the 8th bit of each input byte, hence disabling it. (likley it is disabled by default)
    raw.c_iflag &= ~(IXON | ICRNL | ISTRIP | BRKINT | INPCK);

    // disables the Echo mode, Canonical mode (switches to byte by byte rather than line by line)
    // disabling ISIG disables the program termination by CTRL+C & CTRL+Z
    // disabling IEXTEN fixes CTRL+O in mac & disables wait for extra character in CTRL+V
    // INPCK enables parity checking, we dont need this
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    //sets the character size (CS) to 8bits per byte. (Already set on most systems)
    raw.c_cflag |= (CS8);

    // disabling the carriage returns on new line
    raw.c_oflag &= ~(OPOST);

    // VMIN sets the min no of bytes of input needed before read() can return
    // VTIME sets max time to wait before read() returns [in ms generally 100/1000], it returns
    // 0 on timeout
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    // pass the modified termios structure to tcsetattr to write the new terminal attributes
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        crash("tcsetattr");
}
int getCursorPos(int *rows, int *cols){
    char buffer[32];
    unsigned int x = 0;

    if (write(STDOUT_FILENO, "\x1b[6n]",4) != 4)
        return -1;

    while(x < sizeof(buffer)-1){
        if (read(STDOUT_FILENO, &buffer[x], 1) != 1)
            break;
        if (buffer[x] == 'R')
            break;
        x++;
    }

    buffer[x]='\0';
    char ch;

    printf("\r\n&buffer[1]: '%s'\r\n", &buffer[1]);
    
    readKeypress();
    return -1;
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
/***-------------- OUTPUT ---------------***/
void drawRows(){
    int y;
    for (y = 0; y < E.screenrows; y++){
        write(STDOUT_FILENO,"~\r\n",3);
    }
}

void refreshScreen(){
    // clear screen to the end of the screen
    write(STDOUT_FILENO, "\x1b[2J",4);

    // repostion cursor to top left corner
    write(STDOUT_FILENO, "\x1b[H",3);

    drawRows();

    // repostion cursor to top left corner
    write(STDOUT_FILENO, "\x1b[H",3);
}

/***-------------- INPUT ---------------***/

void processKeypress(){
    char ch = readKeypress();
    switch (ch){
    // making the program quit on pressing Ctrl+Q
    case CTRL_KEY('q'):
        write(STDIN_FILENO, "\x1b[2J",4);
        write(STDIN_FILENO, "\x1b[H",3);
        exit(0);
        break;
    }
}

/***-------------- INIT ---------------***/

void initEditor(){
    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
        crash("getWindowSize");
}

int main(){
    enableRawMode();
    initEditor();

    while (1){
        refreshScreen();
        processKeypress();
    }
    return 0;
}
