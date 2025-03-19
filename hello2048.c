#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

#include <conio.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <Shlobj.h>

#else

#include <termio.h>
#include <unistd.h>
//#include <signum.h>
#include <signal.h>

#define KEY_CODE_UP    0x41
#define KEY_CODE_DOWN  0x42
#define KEY_CODE_LEFT  0x44
#define KEY_CODE_RIGHT 0x43
#define KEY_CODE_QUIT  0x71

struct termios old_config; /* Linux 終端機設定 */

#endif

static char config_path[4096] = {0}; /* 設定檔路徑 */

static void init_game();
static void loop_game();
static void reset_game();
static void release_game(int signal);

static int read_keyboard();

static void move_left();
static void move_right();
static void move_up();
static void move_down();

/* 隨機生成 2 和 4，機率是 9 比 1 */
void add_rand_num();
static void check_game_over();
static int get_null_count();
static void clear_screen();
static void refresh_show();

static int board[4][4];
static int score;
static int best;
static int if_need_add_num;
static int if_game_over;
static int if_prepare_exit;

int main() {
    init_game();
    loop_game();
    release_game(0);
    return 0;
}

int read_keyboard() {
#ifdef _WIN32
    return _getch();
#else
    int key_code;
    if (read(0, &key_code, 1) < 0) {
        return -1;
    }
    return key_code;
#endif
}

void loop_game() {
    while (1) {
        int cmd = read_keyboard();

        if (if_prepare_exit) {
            if (cmd == 'y' || cmd == 'Y') {
                clear_screen();
                return;
            } else if (cmd == 'n' || cmd == 'N') {
                if_prepare_exit = 0;
                refresh_show();
                continue;
            } else {
                continue;
            }
        }

        if (if_game_over) {
            if (cmd == 'y' || cmd == 'Y') {
                reset_game();
                continue;
            } else if (cmd == 'n' || cmd == 'N') {
                clear_screen();
                return;
            } else {
                continue;
            }
        }

        if_need_add_num = 0;

#ifdef _WIN32
        switch (cmd) {
          case 'a':
          case 75:
            move_left();
            break;
          case 's':
          case 80:
            move_down();
            break;
          case 'w':
          case 72:
            move_up();
            break;
          case 'd':
          case 77:
            move_right();
            break;
          case 'q':
          case 27:
            if_prepare_exit = 1;
            break;
          default:continue;
        }
#else
        switch (cmd) {
            case 'a':
            case KEY_CODE_LEFT:
                move_left();
                break;
            case 's':
            case KEY_CODE_DOWN:
                move_down();
                break;
            case 'w':
            case KEY_CODE_UP:
                move_up();
                break;
            case 'd':
            case KEY_CODE_RIGHT:
                move_right();
                break;
            case KEY_CODE_QUIT:
                if_prepare_exit = 1;
                break;
            default:continue;
        }
#endif
        if (score > best) {
            best = score;
            FILE *fp = fopen(config_path, "w");
            if (fp) {
                fwrite(&best, sizeof(best), 1, fp);
                fclose(fp);
            }
        }

        /* 預設為需要生成隨機數時也同時需要更新顯示 */
        if (if_need_add_num) {
            add_rand_num();
            refresh_show();
        } else if (if_prepare_exit) {
            refresh_show();
        }
    }
}

void reset_game() {
    score = 0;
    if_need_add_num = 1;
    if_game_over = 0;
    if_prepare_exit = 0;

    /* 隨機生成一個 2 */
    int n = rand() % 16;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            board[i][j] = (n-- == 0 ? 2 : 0);
        }
    }

    add_rand_num();
    refresh_show();
    return;
}

void add_rand_num() {
    srand((unsigned int) time(0));
    int n = rand() % get_null_count(); /* 在空位置生成隨機數 */

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            /* 定位待生成的位置 */
            if (board[i][j] == 0 && n-- == 0) {
                board[i][j] = (rand() % 10 ? 2 : 4); /* 生成 2 或 4，機率為 9 比 1 */
                return;
            }
        }
    }

    return;
}

/* 空位置數量 */
int get_null_count() {
    int n = 0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            board[i][j] == 0 ? ++n : 1;
        }
    }

    return n;
}

void check_game_over() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            /* 檢查相鄰的元素是否相等，如果相等則不結束 */
            if (board[i][j] == board[i][j + 1] || board[j][i] == board[j + 1][i]) {
                if_game_over = 0;
                return;
            }
        }
    }

    if_game_over = 1;
    return;
}

/*
	如下四個函式，實現上下左右移動時數字面板的變化演算法
	左右移動的本質一樣，差別只有列的遍歷方向相反
	上下移動的本質一樣，差別只有行的遍歷方向相反
	上下和左右移動的本質一樣，差別只有遍歷時行列互換
*/

void move_left() {
    /* 變數 i 用於遍歷行，並且在移動時所有行相互獨立，互不影響 */
    for (int i = 0; i < 4; ++i) {
        /* 變數 j 用於遍歷列，變數 k 是待比較項的索引，迴圈中 k < j */
        /* 变量j为列下标，变量k为待比较（合并）项的下标，循环进入时k<j */
        for (int j = 1, k = 0; j < 4; ++j) {
            if (board[i][j] > 0) /* 找出 k 之後第一個不為空的項，索引為 j，之後分成三種情況 */
            {
                if (board[i][k] == board[i][j]) {
                    /* 情況1：k 項和 j 項相等，此時合併方塊並計分 */
                    score += board[i][k++] *= 2;
                    board[i][j] = 0;
                    if_need_add_num = 1; /* 需要生成隨機數並更新界面 */
                } else if (board[i][k] == 0) {
                    /* 情況2：k 項為空，則把 j 項賦值給 k 項（把 j 方塊移動到 k 方塊 */
                    board[i][k] = board[i][j];
                    board[i][j] = 0;
                    if_need_add_num = 1;
                } else {
                    /* 情況3：k 項不為空，且和 j 項不相等，此時把 j 項賦值給 k+1 項（移動到 k+1 的位置） */
                    board[i][++k] = board[i][j];
                    if (j != k) {
                        /* 判断j项和k项是否原先就挨在一起，若不是则把j项赋值为空（值为0） */
                        board[i][j] = 0;
                        if_need_add_num = 1;
                    }
                }
            }
        }
    }
}

void move_right() {
    /* 仿照左移操作，區別僅僅是行列互換後遍歷 */
    for (int i = 0; i < 4; ++i) {
        for (int j = 2, k = 3; j >= 0; --j) {
            if (board[i][j] > 0) {
                if (board[i][k] == board[i][j]) {
                    score += board[i][k--] *= 2;
                    board[i][j] = 0;
                    if_need_add_num = 1;
                } else if (board[i][k] == 0) {
                    board[i][k] = board[i][j];
                    board[i][j] = 0;
                    if_need_add_num = 1;
                } else {
                    board[i][--k] = board[i][j];
                    if (j != k) {
                        board[i][j] = 0;
                        if_need_add_num = 1;
                    }
                }
            }
        }
    }
}

void move_up() {
    /* 仿照左移操作，區別僅僅是行列互換後遍歷，且 j 和 k 都反向遍歷 */
    for (int i = 0; i < 4; ++i) {
        for (int j = 1, k = 0; j < 4; ++j) {
            if (board[j][i] > 0) {
                if (board[k][i] == board[j][i]) {
                    score += board[k++][i] *= 2;
                    board[j][i] = 0;
                    if_need_add_num = 1;
                } else if (board[k][i] == 0) {
                    board[k][i] = board[j][i];
                    board[j][i] = 0;
                    if_need_add_num = 1;
                } else {
                    board[++k][i] = board[j][i];
                    if (j != k) {
                        board[j][i] = 0;
                        if_need_add_num = 1;
                    }
                }
            }
        }
    }
}

void move_down() {
    /* 仿照左移操作，區別僅僅是行列互換後遍歷，且 j 和 k 都反向遍歷 */
    for (int i = 0; i < 4; ++i) {
        for (int j = 2, k = 3; j >= 0; --j) {
            if (board[j][i] > 0) {
                if (board[k][i] == board[j][i]) {
                    score += board[k--][i] *= 2;
                    board[j][i] = 0;
                    if_need_add_num = 1;
                } else if (board[k][i] == 0) {
                    board[k][i] = board[j][i];
                    board[j][i] = 0;
                    if_need_add_num = 1;
                } else {
                    board[--k][i] = board[j][i];
                    if (j != k) {
                        board[j][i] = 0;
                        if_need_add_num = 1;
                    }
                }
            }
        }
    }
}

void clear_screen() {
#ifdef _WIN32
    /* 重設光標減少閃爍 */
    COORD pos = {0, 0};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    CONSOLE_CURSOR_INFO info = {1, 0};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
#else
    printf("\033c");     /* Linux 清空終端機 */
    printf("\033[?25l"); /* Linux 隱藏輸入光標 */
#endif
}

void refresh_show() {
    clear_screen();

    printf("\n\n\n\n");
    printf("                  GAME: 2048     SCORE: %05d     BEST: %06d\n", score, best);
    printf("               --------------------------------------------------");

    /* 算繪遊戲畫面 */
    printf("\n\n                             +----+----+----+----+\n");
    for (int i = 0; i < 4; ++i) {
        printf("                             %s","|");
        for (int j = 0; j < 4; ++j) {
            if (board[i][j] != 0) {
                if (board[i][j] < 10) {
                    printf("  %d %s", board[i][j],"|");
                } else if (board[i][j] < 100) {
                    printf(" %d %s", board[i][j],"|");
                } else if (board[i][j] < 1000) {
                    printf(" %d%s", board[i][j],"|");
                } else if (board[i][j] < 10000) {
                    printf("%4d%s", board[i][j],"|");
                } else {
                    int n = board[i][j];
                    int k;
                    for (k = 1; k < 20; ++k) {
                        n = n >> 1;
                        if (n == 1) {
                            printf("2^%02d│", k); /* 超過四位數的時候用 2 的冪顯示 */
                            break;
                        }
                    }
                }
            } else printf("    %s","|");
        }

        printf("\n                             +----+----+----+----+\n");
    }

    printf("\n               --------------------------------------------------\n");
    printf("                  [W]:UP [S]:DOWN [A]:LEFT [D]:RIGHT [Q]:EXIT");

    if (get_null_count() == 0) {
        check_game_over();

        if (if_game_over) {
            printf("\r                      GAME OVER! TRY THE GAME AGAIN? [Y/N]:     \b\b\b\b");
#ifdef _WIN32
            CONSOLE_CURSOR_INFO info = {1, 1};
            SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
#else
            printf("\033[?25h"); /* Linux 的輸入光標 */
#endif
        }
    }

    if (if_prepare_exit) {
        printf("\r                   DO YOU REALLY WANT TO QUIT THE GAME? [Y/N]:   \b\b");
#ifdef _WIN32
        CONSOLE_CURSOR_INFO info = {1, 1};
            SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
#else
        printf("\033[?25h"); /* Linux 的輸入光標 */
#endif
    }

    fflush(0);
}

void init_game() {
#ifdef _WIN32
    system("cls");

    /* 存檔路徑放在 C:\Users\Username\AppData\Local\2048 下 */
    char m_lpszDefaultDir[MAX_PATH] = {0};
    char szDocument[MAX_PATH] = {0};
    LPITEMIDLIST pidl = NULL;

    SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);

    if (pidl && SHGetPathFromIDList(pidl, szDocument)) {
        GetShortPathName(szDocument, m_lpszDefaultDir, _MAX_PATH);
    }

    snprintf(config_path, sizeof config_path, "%s\\2048", m_lpszDefaultDir);

    if (_access(config_path, 0) == -1) {
        _mkdir(config_path);
    }

    snprintf(config_path, sizeof config_path, "%s\\2048\\2048.dat", m_lpszDefaultDir);
#else
    /* 存檔放在使用者的家目錄下 */
    snprintf(config_path, sizeof config_path, "%s/.2048", getenv("HOME"));

    tcgetattr(0, &old_config);              /* 取得終端機屬性 */
    struct termios new_config = old_config; /* 創建新的終端機屬性 */
    new_config.c_lflag &= ~ICANON;          /* 非正規模式 */
    new_config.c_lflag &= ~ECHO;            /* 不顯示輸入 */
    new_config.c_cc[VMIN] = 1;              /* 可以讀取的最小字元數 */
    new_config.c_cc[VTIME] = 0;             /* 讀取延遲 */
    tcsetattr(0, TCSANOW, &new_config);     /* 設定新的終端機屬性 */

    printf("\033[?25l");
    signal(SIGINT, release_game);
#endif

    /* 讀取遊戲最高分數 */
    FILE *fp = fopen(config_path, "r");
    if (fp) {
        fread(&best, sizeof(best), 1, fp);
        fclose(fp);
    } else {
        best = 0;
        fp = fopen(config_path, "w");
        if (fp) {
            fwrite(&best, sizeof(best), 1, fp);
            fclose(fp);
        }
    }

    reset_game();
}

void release_game(int signal) {
#ifdef _WIN32
    system("cls");
    CONSOLE_CURSOR_INFO info = {1, 1};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
#else
    if (signal == SIGINT) {
        printf("\n");
    }
    tcsetattr(0, TCSANOW, &old_config); /* 還原本來的終端機屬性 */
    printf("\033[?25h");
#endif
    exit(0);
}
