#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

//pretty much hacked this together in an evening from different pieces.  it's not very performant (actually, it's terrible), but it is better than putting the key events in manually, which is what I was doing.
//would still use the remote over it, though, but at least it saves you if you don't have the remote because you left it somewhere else.
//the main issue with performance is obviously the multiple system calls to `adb`.  if there was a way of getting one session and spitting the commands through that, it may be worth doing.

#define MAX_COMMAND_SIZE 1024

//the android keyevents here.
//taken from https://stackoverflow.com/questions/7789826/adb-shell-input-events

#define KEYCODE_UNKNOWN 0
#define KEYCODE_MENU 1
#define KEYCODE_SOFT_RIGHT 2
#define KEYCODE_HOME 3
#define KEYCODE_BACK 4
#define KEYCODE_CALL 5
#define KEYCODE_ENDCALL 6
#define KEYCODE_0 7
//not defining 1-9 add the value to 0.

#define KEYCODE_STAR 17
#define KEYCODE_POUND 18
#define KEYCODE_DPAD_UP 19
#define KEYCODE_DPAD_DOWN 20
#define KEYCODE_DPAD_LEFT 21
#define KEYCODE_DPAD_RIGHT 22
#define KEYCODE_DPAD_CENTER 23
#define KEYCODE_VOLUME_UP 24
#define KEYCODE_VOLUME_DOWN 25
#define KEYCODE_POWER 26
#define KEYCODE_CAMERA 27
#define KEYCODE_CLEAR 28
#define KEYCODE_A 29
//not defining b-z just add the value.

#define KEYCODE_COMMA 55
#define KEYCODE_PERIOD 56
#define KEYCODE_ALT_LEFT 57
#define KEYCODE_ALT_RIGHT 58
#define KEYCODE_SHIFT_LEFT 59
#define KEYCODE_SHIFT_RIGHT 60
#define KEYCODE_TAB 61
#define KEYCODE_SPACE 62
#define KEYCODE_SYM 63
#define KEYCODE_EXPLORER 64
#define KEYCODE_ENTER 66
#define KEYCODE_DEL 67
#define KEYCODE_GRAVE 68 //hashtag?
#define KEYCODE_MINUS 69
#define KEYCODE_EQUALS 70
#define KEYCODE_LEFT_BRACKET 71
#define KEYCODE_RIGHT_BRACKET 72
#define KEYCODE_BACKSLASH 73
#define KEYCODE_SEMICOLON 74
#define KEYCODE_APOSTROPHE 75
#define KEYCODE_SLASH 76
#define KEYCODE_AT 77 // @ ??
#define KEYCODE_NUM 78 
#define KEYCODE_HEADSETHOOK 79
#define KEYCODE_FOCUS 80
#define KEYCODE_PLUS 81
//#define KEYCODE_MENU 82 //not sure why this is defined twice in the list but that obviously won't work for the code here.  as long as it outputs ok i guess.
#define KEYCODE_NOTIFICATION 83
#define KEYCODE_SEARCH 84
#define TAG_LAST_KEYCODE 85

static void err_exit(const char *msg);

char handle_character_event(const char * cmd)
{
    char ret = EOF;
    struct termios staryTermios, novyTermios;
    int oflags, nflags;

    while(ret == EOF)
    {
        if (tcgetattr(STDIN_FILENO, &staryTermios) != 0)
            err_exit("tcgetattr() failed");

        novyTermios = staryTermios;
        novyTermios.c_lflag &= ~(ICANON | ECHO);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &novyTermios) != 0)
            err_exit("tcsetattr() failed to set standard input");

        oflags = fcntl(STDIN_FILENO, F_GETFL);
        if (oflags < 0)
            err_exit("fcntl() F_GETFL failed");

        nflags = oflags;
        nflags |= O_NONBLOCK;
        if (fcntl(STDIN_FILENO, F_SETFL, nflags) == -1)
            err_exit("fcntl() F_SETFL failed");

        novyTermios.c_cc[VMIN] = 0;//minimum number of bytes to read before returning.  
        novyTermios.c_cc[VTIME] = 1;//timeout = 100 msec

        (void)read(STDIN_FILENO, &ret, 1);

        if (tcsetattr(STDIN_FILENO, TCSANOW, &staryTermios) != 0)
            err_exit("tcsetattr() failed to reset standard input");

        char cmd_buffer[MAX_COMMAND_SIZE];
        if ((int)ret == 27)
        {
            char second = '\0', third = '\0';
            (void)read(STDIN_FILENO, &second, 1);
            (void)read(STDIN_FILENO, &third, 1);
            if ((int)second == 0 && (int)third == 0)
            {
                printf("\nescape key pressed, injecting BACK key.\n");
                (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_BACK);
                (void)system(cmd_buffer);            
            }
            else if ((int)second == 91)
            {
                printf("\narrow key pressed, value: %d.\n", (int)third);
                switch ((int)third)
                {
                    case 65:
                        //UP
                        printf("\nUP key pressed.\n");
                        (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_UP);
                        break;
                    case 66:
                        //DOWN
                        printf("\nDOWN key pressed.\n");
                        (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_DOWN);
                        break;
                    case 67:
                        //RIGHT
                        printf("\nRIGHT key pressed.\n");
                        (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_RIGHT);
                        break;
                    case 68:
                        //LEFT
                        printf("\nLEFT key pressed.\n");
                        (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_LEFT);
                        break;
                    case 49:
                    case 50:
                    default:
                        //TODO: fix alt & shift + arrow keys
                        //assert(false);
                        //not possible
                        break;
                }

                if (strlen(cmd_buffer) != 0)
                {
                    printf("cmd_buffer: %s.\n", cmd_buffer);
                    (void)system(cmd_buffer);
                }
            }
            printf("first byte : %d\n", (int)ret);

            if ((int)second)
            {
                printf("second value: %d\n", (int)second);
            }
            if ((int)third)
            {
                printf("third value: %d\n", (int)third);
            }
        }
        else if ((int)ret == 'a' || (int)ret == 'A')
        {
            printf("\nLEFT key pressed.\n");
            (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_LEFT);
            printf("cmd_buffer: %s.\n", cmd_buffer);
            (void)system(cmd_buffer);            
        }
        else if ((int)ret == 's' || (int)ret == 'S')
        {
            printf("\nDOWN key pressed.\n");
            (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_DOWN);
            printf("cmd_buffer: %s.\n", cmd_buffer);
            (void)system(cmd_buffer);            
        }
        else if ((int)ret == 'd' || (int)ret == 'D')
        {
            printf("\nRIGHT key pressed.\n");
            (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_RIGHT);
            printf("cmd_buffer: %s.\n", cmd_buffer);
            (void)system(cmd_buffer);            
        }
        else if ((int)ret == 'w' || (int)ret == 'W')
        {
            printf("\nUP key pressed.\n");
            (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_UP);
            printf("cmd_buffer: %s.\n", cmd_buffer);
            (void)system(cmd_buffer);            
        }
        else if ((int)ret == 4)
        {
            printf("\nctrl+d pressed.  value: %d.\n", (int)ret);
        }
        else if ((int)ret == 0)
        {
            printf("\nctrl+SPACE pressed.  Injecting DPAD_CENTER.\n");
            (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_CENTER);
            (void)system(cmd_buffer);            
        }
        else if ((int)ret == 127)
        {
            printf("\nBackspace key pressed.  Injecting BACK.\n");
            (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_BACK);
            (void)system(cmd_buffer);            
        }
        else if ((int)ret == 10)
        {
            printf("\nReturn key pressed.  Injecting DPAD_CENTER.\n");
            (void)sprintf(cmd_buffer, "%s shell input keyevent %d", cmd, KEYCODE_DPAD_CENTER);
            (void)system(cmd_buffer);            
        }
        else if ((int)ret != EOF) 
        {
            printf("\nkey pressed: %d\n", (int)ret);
        }

        if (iscntrl(ret))
        {
            printf("\nis control key, dropping it.\n");
            ret = EOF;
        }
    }

    return ret;
}

int main(void)
{
    char c = '\0';

    //TODO: get this from a config file or the command line options.
    const char *cmd = "adb";
    while (1)
    {
        c = handle_character_event(cmd);

        if (c != '\0')
        {
            putchar(c);
            c = '\0';
        }
    }

    putchar('\n');
    return 0;
}

static void err_exit(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}
