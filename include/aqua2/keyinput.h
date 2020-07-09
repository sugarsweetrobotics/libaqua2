#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <conio.h>

#else
#include <unistd.h>
#include <termios.h>
#endif

namespace ssr {
    namespace aqua2 {


#ifdef WIN32


#else
        static struct termios m_oldTermios;
#endif


        using AQUA2_KEY = int;
        
        enum AQUA2_KEY_ENUM {
            AQUA2_KEY_UP = 65536,
            AQUA2_KEY_DOWN,
            AQUA2_KEY_LEFT,
            AQUA2_KEY_RIGHT,
            AQUA2_KEY_SPACE,
            AQUA2_KEY_ESCAPE,
        };

        static void init_scr() {
        #ifdef WIN32
            system("cls");
        #else
            struct termios myTermios;
            tcgetattr(fileno(stdin), &m_oldTermios);
            tcgetattr(fileno(stdin), &myTermios);
            
            myTermios.c_cc[VTIME] = 0;
        #ifdef linux
            myTermios.c_cc[VMIN] = 0;
        #else // MacOS
            myTermios.c_cc[VMIN] = 1;
        #endif
            myTermios.c_lflag &= (~ECHO & ~ICANON);
            tcsetattr(fileno(stdin), TCSANOW, &myTermios);
                
        #endif
        }

        static void clear_scr() {
        #ifdef WIN32
            system("cls");
        #else
            system("clear");
        #endif
        }

        static void exit_scr() {
        #ifdef WIN32
            system("cls");
        #else
            system("reset");
        #endif
        }

        static int kbhit() {
        #ifdef WIN32
            return _kbhit();
        #else
            fd_set fdset;
            struct timeval timeout;
            FD_ZERO( &fdset ); 
            FD_SET( 0, &fdset );
            timeout.tv_sec = 0; 
            timeout.tv_usec = 0;
            //  std::cout << "selecting.." << std::ends;
            int ret = select(0+1 , &fdset , NULL , NULL , &timeout );
            //  std::cout << "selected(" << ret << ")" << std::endl;
            return ret;
        #endif
        }


        static int getch() {
        #ifdef WIN32
            return _getch();
        #else
            int keys[5] = {-1, -1, -1, -1, -1};
            int key = getchar();
            switch(key) {
                case -1:
                case 0:
                    return -1;
                    
                case ' ':
                    return AQUA2_KEY_SPACE;
                case 27:
                    key = getchar();
                    switch(key) {
                        case -1:
                            return AQUA2_KEY_ESCAPE;
                        case 79:
                            key = getchar();
                            return key;
                        case '[':
                            for(int i = 0;i < 5;i++) {
                                if(key == -1 || key == 27) break;
                                keys[i] = key = getchar();
                            }
                            ungetc(key, stdin);
                            
                            switch(keys[0]) {
                                case 65: return AQUA2_KEY_UP;
                                case 66: return AQUA2_KEY_DOWN;
                                case 67: return AQUA2_KEY_RIGHT;
                                case 68: return AQUA2_KEY_LEFT;
                                default: return keys[0];
                            }
                    }
            }
            return key;
        #endif
        }


    }
} // namespace ssr
