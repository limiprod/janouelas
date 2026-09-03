/* kernel.c — Janouelas OS v0.4 (VGA + Rato + Audio PC Speaker + Dark Mode + Pong + Notepad + Shutdown) */

/* Declarações do Hardware / I/O */
static void outb(unsigned short port, unsigned char val);
static void outw(unsigned short port, unsigned short val);
static unsigned char inb(unsigned short port);
static void delay_ms(unsigned short ms);
static void sys_shutdown(void);

/* Audio PC Speaker */
static void play_sound(unsigned short freq);
static void sound_stop(void);
static void beep(unsigned short freq, unsigned short duration);

/* Teclado e Texto */
static void putchar(char c);
static char getchar(void);
static unsigned short get_key_raw(void);
static unsigned short get_key_nonblock_raw(void);
static void print(const char* s);
static void set_cursor(unsigned char x, unsigned char y);
static void clear_screen(void);
static int strcmp(const char* s1, const char* s2);
static int atoi(const char* s);
static void itoa(int n, char* str);
static void read_line(char* buffer, int max_len);

/* Modo Gráfico VGA 320x200 e Paleta de Cores */
static void set_video_mode(unsigned char mode);
static void put_pixel(int x, int y, unsigned char color);
static void draw_rect(int x, int y, int w, int h, unsigned char color);
static void set_vga_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b);
static void apply_theme(void);
static void draw_window_vga(int x, int y, int w, int h, const char* title);
static void draw_cursor_vga(int x, int y);

/* Rato BIOS (int 0x33) */
static int init_mouse(void);
static void get_mouse_status(int* x, int* y, int* btn);

/* Apps e Jogos */
static void run_calculator(void);
static void run_snake(void);
static void run_pong(void);
static void run_notepad(void);
static void run_janouelas_gui(void);

/* Variáveis Globais */
static int is_dark_mode = 0;

/* Ponto de Entrada do Kernel */
__attribute__((section(".text._start")))
void _start(void) {
    char input[64];

    clear_screen();
    beep(1000, 50);
    beep(1500, 80);

    print("==================================================\r\n");
    print("  Janouelas OS v0.4 (VGA + Som + Dark Mode + Apps)\r\n");
    print("==================================================\r\n");
    print("Comandos: help, cls, ver, calc, snake, pong, notas, color, janouelas, shutdown\r\n\r\n");

    while (1) {
        print("Janouelas> ");
        read_line(input, 64);

        if (strcmp(input, "help") == 0) {
            print("Comandos disponiveis:\r\n");
            print("  help      - Lista os comandos\r\n");
            print("  cls       - Limpa o ecra\r\n");
            print("  ver       - Versao do Kernel\r\n");
            print("  calc      - Calculadora basica\r\n");
            print("  snake     - Jogo da Cobrinha (Snake)\r\n");
            print("  pong      - Jogo Pong para 2 Jogadores\r\n");
            print("  notas     - Bloco de Notas simples\r\n");
            print("  color     - Alterna entre Dark Mode e Modo Claro\r\n");
            print("  janouelas - Interface Grafica VGA 320x200\r\n");
            print("  shutdown  - Desliga o sistema\r\n");
        }
        else if (strcmp(input, "cls") == 0) {
            clear_screen();
        }
        else if (strcmp(input, "ver") == 0) {
            print("Janouelas OS v0.4 (16-bit Multimedia Kernel)\r\n");
        }
        else if (strcmp(input, "calc") == 0) {
            run_calculator();
        }
        else if (strcmp(input, "snake") == 0) {
            run_snake();
        }
        else if (strcmp(input, "pong") == 0) {
            run_pong();
        }
        else if (strcmp(input, "notas") == 0) {
            run_notepad();
        }
        else if (strcmp(input, "color") == 0 || strcmp(input, "dark") == 0) {
            is_dark_mode = !is_dark_mode;
            beep(1200, 40);
            if (is_dark_mode) print("Dark Mode Ativado!\r\n");
            else print("Modo Claro Ativado!\r\n");
        }
        else if (strcmp(input, "janouelas") == 0 || strcmp(input, "gui") == 0) {
            run_janouelas_gui();
        }
        else if (strcmp(input, "shutdown") == 0 || strcmp(input, "off") == 0) {
            sys_shutdown();
        }
        else if (input[0] != '\0') {
            print("Comando invalido: '");
            print(input);
            print("'\r\n");
        }
    }
}

/* --- HARDWARE I/O, SOM & SISTEMA --- */

static void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__("outb %%al, %%dx" : : "a"(val), "d"(port));
}

static void outw(unsigned short port, unsigned short val) {
    __asm__ __volatile__("outw %%ax, %%dx" : : "a"(val), "d"(port));
}

static unsigned char inb(unsigned short port) {
    unsigned char val;
    __asm__ __volatile__("inb %%dx, %%al" : "=a"(val) : "d"(port));
    return val;
}

static void delay_ms(unsigned short ms) {
    while (ms--) {
        __asm__ __volatile__(
            "movb $0x86, %%ah\n\t"
            "movw $0x0000, %%cx\n\t"
            "movw $0x03E8, %%dx\n\t"
            "int $0x15"
            : : : "ax", "cx", "dx"
        );
    }
}

static void sys_shutdown(void) {
    print("Desligando o Janouelas OS...\r\n");
    beep(400, 100);
    beep(200, 150);

    /* 1. Tenta desligar via APM BIOS (int 0x15) */
    __asm__ __volatile__(
        "movw $0x5301, %%ax\n\t"
        "xorw %%bx, %%bx\n\t"
        "int $0x15\n\t"
        "movw $0x530E, %%ax\n\t"
        "xorw %%bx, %%bx\n\t"
        "movw $0x0102, %%cx\n\t"
        "int $0x15\n\t"
        "movw $0x5307, %%ax\n\t"
        "movw $0x0001, %%bx\n\t"
        "movw $0x0003, %%cx\n\t"
        "int $0x15"
        : : : "ax", "bx", "cx"
    );

    /* 2. Fallback via Portas I/O de 16-bits (QEMU / VirtualBox / Bochs) */
    outw(0x604, 0x2000);  /* QEMU ACPI */
    outw(0x4004, 0x3400); /* VirtualBox / Bochs */
    outw(0xB004, 0x2000); /* QEMU Antigo */

    /* 3. Se estiver em hardware real antigo sem APM/ACPI, para a CPU */
    __asm__ __volatile__("cli");
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

static void play_sound(unsigned short freq) {
    if (freq == 0) return;
    unsigned short div = (unsigned short)(1193180 / freq);
    outb(0x43, 0xB6);
    outb(0x42, (unsigned char)(div & 0xFF));
    outb(0x42, (unsigned char)((div >> 8) & 0xFF));
    unsigned char tmp = inb(0x61);
    if ((tmp & 3) != 3) {
        outb(0x61, tmp | 3);
    }
}

static void sound_stop(void) {
    unsigned char tmp = inb(0x61);
    outb(0x61, tmp & 0xFC);
}

static void beep(unsigned short freq, unsigned short duration) {
    play_sound(freq);
    delay_ms(duration);
    sound_stop();
}

/* --- MOTOR GRAFICO VGA 320x200 & PALETA --- */

static void set_video_mode(unsigned char mode) {
    __asm__ __volatile__(
        "int $0x10"
        : : "a"((unsigned short)mode)
    );
}

static void set_vga_color(unsigned char index, unsigned char r, unsigned char g, unsigned char b) {
    outb(0x3C8, index);
    outb(0x3C9, r & 0x3F);
    outb(0x3C9, g & 0x3F);
    outb(0x3C9, b & 0x3F);
}

static void apply_theme(void) {
    if (is_dark_mode) {
        set_vga_color(59, 4, 6, 12);    /* Fundo Desktop: Azul Noturno */
        set_vga_color(23, 12, 12, 15);  /* Fundo Janela: Cinza Escuro */
        set_vga_color(9, 25, 5, 35);    /* Barra Titulo: Roxo Neon */
    }
    else {
        set_vga_color(59, 0, 32, 32);   /* Fundo Desktop: Teal Padrao 95 */
        set_vga_color(23, 42, 42, 42);  /* Fundo Janela: Cinza Claro */
        set_vga_color(9, 0, 0, 42);     /* Barra Titulo: Azul Padrao */
    }
}

static void put_pixel(int x, int y, unsigned char color) {
    if (x < 0 || x >= 320 || y < 0 || y >= 200) return;
    unsigned short offset = (unsigned short)(y * 320 + x);
    __asm__ __volatile__(
        "pushw %%es\n\t"
        "mov $0xA000, %%ax\n\t"
        "mov %%ax, %%es\n\t"
        "movb %0, %%es:(%%bx)\n\t"
        "popw %%es"
        :
    : "q"(color), "b"(offset)
        : "ax"
        );
}

static void draw_rect(int x, int y, int w, int h, unsigned char color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

static void draw_window_vga(int x, int y, int w, int h, const char* title) {
    draw_rect(x, y, w, h, 23);               /* Fundo da Janela */
    draw_rect(x, y, w, 1, 15);               /* Bordas 3D */
    draw_rect(x, y, 1, h, 15);
    draw_rect(x + w - 1, y, 1, h, 0);
    draw_rect(x, y + h - 1, w, 1, 0);

    draw_rect(x + 3, y + 3, w - 6, 14, 9);  /* Barra de Titulo */
    draw_rect(x + w - 16, y + 4, 11, 11, 12);/* Botao Fechar [X] */

    set_cursor((x + 6) / 8, (y + 6) / 8);
    print(title);
}

static void draw_cursor_vga(int x, int y) {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j <= i; j++) {
            put_pixel(x + j, y + i, 15);
        }
    }
    put_pixel(x, y, 0);
}

/* --- SUPORTE A RATO (INT 0x33) --- */

static int init_mouse(void) {
    unsigned short ax;
    __asm__ __volatile__(
        "int $0x33"
        : "=a"(ax)
        : "a"((unsigned short)0x0000)
    );
    return (ax == 0xFFFF);
}

static void get_mouse_status(int* x, int* y, int* btn) {
    unsigned short bx_out, cx_out, dx_out;
    __asm__ __volatile__(
        "int $0x33"
        : "=b"(bx_out), "=c"(cx_out), "=d"(dx_out)
        : "a"((unsigned short)0x0003)
    );
    *btn = bx_out;
    *x = cx_out / 2;
    *y = dx_out;
}

/* --- INTERFACE GRAFICA JANOUELAS --- */

static void run_janouelas_gui(void) {
    set_video_mode(0x13);
    apply_theme();

    int has_mouse = init_mouse();
    int mouse_x = 160, mouse_y = 100, mouse_btn = 0;

    while (1) {
        draw_rect(0, 0, 320, 200, 59);
        draw_window_vga(20, 15, 280, 165, " Janouelas OS v0.4 ");

        /* Botoes da GUI */
        draw_rect(35, 45, 75, 22, 22);  /* Calc */
        draw_rect(120, 45, 80, 22, 22); /* Dark Mode */
        draw_rect(210, 45, 75, 22, 22); /* Sair */

        set_cursor(5, 7);   print("[ Calc ]");
        set_cursor(16, 7);  print("[ Theme ]");
        set_cursor(27, 7);  print("[ Sair ]");

        set_cursor(5, 11);  print("Teclas Rapidas:");
        set_cursor(5, 13);  print("  P = Pong (2 Players)");
        set_cursor(5, 15);  print("  N = Bloco de Notas");
        set_cursor(5, 17);  print("  S = Snake Game");

        if (has_mouse) {
            get_mouse_status(&mouse_x, &mouse_y, &mouse_btn);
        }

        unsigned short raw_key = get_key_nonblock_raw();
        unsigned char ascii = (unsigned char)(raw_key & 0xFF);
        unsigned char scan = (unsigned char)((raw_key >> 8) & 0xFF);

        if (ascii == 'q' || ascii == 'Q') break;
        if (ascii == 'p' || ascii == 'P') { run_pong(); apply_theme(); }
        if (ascii == 'n' || ascii == 'N') { run_notepad(); set_video_mode(0x13); apply_theme(); }
        if (ascii == 's' || ascii == 'S') { run_snake(); set_video_mode(0x13); apply_theme(); }

        /* Movimento por setas / WASD */
        if (ascii == 'a' || ascii == 'A' || scan == 0x4B) mouse_x -= 6;
        if (ascii == 'd' || ascii == 'D' || scan == 0x4D) mouse_x += 6;
        if (ascii == 'w' || ascii == 'W' || scan == 0x48) mouse_y -= 6;
        if (ascii == 's' || ascii == 'S' || scan == 0x50) mouse_y += 6;

        /* Testar Cliques */
        if ((mouse_btn & 1) || ascii == ' ' || ascii == '\r') {
            /* Botao Sair */
            if (mouse_x >= 210 && mouse_x <= 285 && mouse_y >= 45 && mouse_y <= 67) {
                beep(600, 40);
                break;
            }
            /* Botao Calc */
            if (mouse_x >= 35 && mouse_x <= 110 && mouse_y >= 45 && mouse_y <= 67) {
                beep(800, 30);
                set_video_mode(0x03);
                run_calculator();
                print("Pressione qualquer tecla para voltar...");
                getchar();
                set_video_mode(0x13);
                apply_theme();
            }
            /* Botao Theme/Dark Mode */
            if (mouse_x >= 120 && mouse_x <= 200 && mouse_y >= 45 && mouse_y <= 67) {
                beep(1200, 30);
                is_dark_mode = !is_dark_mode;
                apply_theme();
                delay_ms(150);
            }
        }

        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x > 310) mouse_x = 310;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y > 190) mouse_y = 190;

        draw_cursor_vga(mouse_x, mouse_y);
        delay_ms(20);
    }

    set_video_mode(0x03);
    clear_screen();
}

/* --- JOGO PONG (2 JOGADORES - VGA) --- */

static void run_pong(void) {
    set_video_mode(0x13);

    int p1_y = 80, p2_y = 80;
    int ball_x = 160, ball_y = 100;
    int ball_dx = 3, ball_dy = 2;
    int score1 = 0, score2 = 0;

    while (1) {
        draw_rect(0, 0, 320, 200, 0);              /* Fundo Preto */
        draw_rect(159, 0, 2, 200, 23);             /* Linha Central */

        /* Desenhar Raquetes e Bola */
        draw_rect(10, p1_y, 6, 35, 15);            /* Player 1 (Esquerda) */
        draw_rect(304, p2_y, 6, 35, 15);           /* Player 2 (Direita) */
        draw_rect(ball_x, ball_y, 6, 6, 14);       /* Bola Amarela */

        /* Exibir Placar */
        set_cursor(14, 1);
        char s1[4], s2[4];
        itoa(score1, s1); itoa(score2, s2);
        print(s1); print(" : "); print(s2);

        /* Controles */
        unsigned short raw = get_key_nonblock_raw();
        unsigned char ascii = (unsigned char)(raw & 0xFF);
        unsigned char scan = (unsigned char)((raw >> 8) & 0xFF);

        if (ascii == 'q' || ascii == 'Q') break;

        /* P1: W / S */
        if ((ascii == 'w' || ascii == 'W') && p1_y > 4) p1_y -= 5;
        if ((ascii == 's' || ascii == 'S') && p1_y < 160) p1_y += 5;

        /* P2: Setas Cima / Baixo */
        if (scan == 0x48 && p2_y > 4) p2_y -= 5;   /* Seta Cima */
        if (scan == 0x50 && p2_y < 160) p2_y += 5;  /* Seta Baixo */

        /* Movimentar Bola */
        ball_x += ball_dx;
        ball_y += ball_dy;

        /* Colisão com Parede Superior/Inferior */
        if (ball_y <= 2 || ball_y >= 192) {
            ball_dy = -ball_dy;
            beep(500, 15);
        }

        /* Colisão Raquete P1 */
        if (ball_x <= 16 && ball_y >= p1_y - 4 && ball_y <= p1_y + 35) {
            ball_dx = -ball_dx;
            beep(900, 15);
        }

        /* Colisão Raquete P2 */
        if (ball_x >= 298 && ball_y >= p2_y - 4 && ball_y <= p2_y + 35) {
            ball_dx = -ball_dx;
            beep(900, 15);
        }

        /* Pontuação */
        if (ball_x < 0) {
            score2++;
            beep(300, 100);
            ball_x = 160; ball_y = 100; ball_dx = 3;
        }
        if (ball_x > 314) {
            score1++;
            beep(300, 100);
            ball_x = 160; ball_y = 100; ball_dx = -3;
        }

        delay_ms(20);
    }

    set_video_mode(0x03);
    clear_screen();
}

/* --- BLOCO DE NOTAS (NOTEPAD) --- */

static void run_notepad(void) {
    clear_screen();
    print("==================================================\r\n");
    print("      BLOCO DE NOTAS JANOUELAS - (ESC p/ Sair)    \r\n");
    print("==================================================\r\n\r\n");

    while (1) {
        unsigned short raw = get_key_raw();
        unsigned char ascii = (unsigned char)(raw & 0xFF);
        unsigned char scan = (unsigned char)((raw >> 8) & 0xFF);

        /* Tecla ESC */
        if (scan == 0x01 || ascii == 27) {
            beep(700, 40);
            break;
        }

        /* Backspace */
        if (ascii == 8 || ascii == 127) {
            print("\b \b");
            beep(400, 10);
        }
        /* Enter */
        else if (ascii == '\r' || ascii == '\n') {
            print("\r\n");
            beep(500, 10);
        }
        /* Caracteres normais */
        else if (ascii >= 32 && ascii <= 126) {
            putchar(ascii);
            beep(1200, 5);
        }
    }
    clear_screen();
}

/* --- SNAKE GAME COM SOM --- */

static void run_snake(void) {
    clear_screen();
    print("=== JOGO DA COBRINHA (SNAKE) ===\r\n");
    print("Controles: Setinhas do Teclado ou WASD | 'q' Sair\r\n\r\n");

    int snake_x[60], snake_y[60];
    int len = 4, dir_x = 1, dir_y = 0;
    int food_x = 12, food_y = 6, score = 0;

    for (int i = 0; i < len; i++) {
        snake_x[i] = 10 - i;
        snake_y[i] = 5;
    }

    while (1) {
        set_cursor(0, 4);
        print("Pontos: ");
        char numbuf[10]; itoa(score, numbuf); print(numbuf);
        print("\r\n+----------------------------------------+\r\n");

        for (int y = 0; y < 12; y++) {
            print("|");
            for (int x = 0; x < 38; x++) {
                if (x == food_x && y == food_y) putchar('*');
                else {
                    int is_body = 0;
                    for (int k = 0; k < len; k++) {
                        if (snake_x[k] == x && snake_y[k] == y) { is_body = 1; break; }
                    }
                    if (is_body) putchar('O');
                    else putchar(' ');
                }
            }
            print("|\r\n");
        }
        print("+----------------------------------------+\r\n");

        unsigned short raw = get_key_nonblock_raw();
        unsigned char ascii = (unsigned char)(raw & 0xFF);
        unsigned char scan = (unsigned char)((raw >> 8) & 0xFF);

        if (ascii == 'w' || ascii == 'W' || scan == 0x48) { dir_x = 0; dir_y = -1; }
        else if (ascii == 's' || ascii == 'S' || scan == 0x50) { dir_x = 0; dir_y = 1; }
        else if (ascii == 'a' || ascii == 'A' || scan == 0x4B) { dir_x = -1; dir_y = 0; }
        else if (ascii == 'd' || ascii == 'D' || scan == 0x4D) { dir_x = 1; dir_y = 0; }
        else if (ascii == 'q' || ascii == 'Q') break;

        int next_x = snake_x[0] + dir_x;
        int next_y = snake_y[0] + dir_y;

        if (next_x < 0 || next_x >= 38 || next_y < 0 || next_y >= 12) {
            beep(200, 200);
            print("\r\nGAME OVER!");
            break;
        }

        if (next_x == food_x && next_y == food_y) {
            score += 10;
            beep(1400, 30);
            if (len < 59) len++;
            food_x = (food_x + 7) % 35 + 1;
            food_y = (food_y + 3) % 10 + 1;
        }

        for (int i = len - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        snake_x[0] = next_x;
        snake_y[0] = next_y;

        delay_ms(90);
    }

    print("\r\nPressione qualquer tecla para sair...");
    getchar();
    clear_screen();
}

/* --- OUTRAS FUNÇÕES E UTILITÁRIOS --- */

static void run_calculator(void) {
    char n1_str[16], n2_str[16], op_str[4];
    print("\r\n=== CALCULADORA ===\r\n");
    print("Primeiro numero: "); read_line(n1_str, 16);
    print("Operacao (+, -, *): "); read_line(op_str, 4);
    print("Segundo numero: "); read_line(n2_str, 16);

    int a = atoi(n1_str), b = atoi(n2_str), res = 0;
    if (op_str[0] == '+') res = a + b;
    else if (op_str[0] == '-') res = a - b;
    else if (op_str[0] == '*') res = a * b;

    char res_str[16]; itoa(res, res_str);
    print("Resultado: "); print(res_str); print("\r\n\r\n");
}

static void putchar(char c) {
    __asm__ __volatile__(
        "int $0x10"
        : : "a"((unsigned short)(0x0e00 | (unsigned char)c)), "b"(0)
    );
}

static unsigned short get_key_raw(void) {
    unsigned short ax;
    __asm__ __volatile__("movb $0x00, %%ah\n\tint $0x16" : "=a"(ax));
    return ax;
}

static char getchar(void) { return (char)(get_key_raw() & 0xFF); }

static unsigned short get_key_nonblock_raw(void) {
    unsigned short ax;
    __asm__ __volatile__(
        "movb $0x01, %%ah\n\tint $0x16\n\tjnz 1f\n\txor %%ax, %%ax\n\tjmp 2f\n\t1:\n\tmovb $0x00, %%ah\n\tint $0x16\n\t2:"
        : "=a"(ax)
    );
    return ax;
}

static void print(const char* s) { while (*s) putchar(*s++); }

static void set_cursor(unsigned char x, unsigned char y) {
    __asm__ __volatile__(
        "movb $0x02, %%ah\n\tmovb $0, %%bh\n\tint $0x10"
        : : "d"((unsigned short)((y << 8) | x))
    );
}

static void clear_screen(void) {
    __asm__ __volatile__(
        "movw $0x0600, %%ax\n\tmovb $0x07, %%bh\n\tmovw $0x0000, %%cx\n\tmovw $0x184f, %%dx\n\tint $0x10"
        : : : "ax", "bx", "cx", "dx"
    );
    set_cursor(0, 0);
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int atoi(const char* s) {
    int res = 0;
    while (*s >= '0' && *s <= '9') { res = res * 10 + (*s - '0'); s++; }
    return res;
}

static void itoa(int n, char* str) {
    int i = 0;
    if (n == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    char temp[16]; int j = 0;
    while (n > 0) { temp[j++] = (n % 10) + '0'; n /= 10; }
    while (j > 0) { str[i++] = temp[--j]; }
    str[i] = '\0';
}

static void read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = getchar();
        if (c == '\r' || c == '\n') { putchar('\r'); putchar('\n'); break; }
        else if (c == 8 || c == 127) { if (i > 0) { i--; print("\b \b"); } }
        else if (c >= 32 && c <= 126) { buffer[i++] = c; putchar(c); }
    }
    buffer[i] = '\0';
}