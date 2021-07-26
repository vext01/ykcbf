// This is bf_base.c modified to replay the program via the JIT.

#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <yk.h>
#include <yk_testing.h>

#define CELLS_LEN 30000

void interp(char *prog, char *prog_end, char *cells, char *cells_end) {
    char *instr = prog;
    char *cell = cells;
    while (instr < prog_end) {
        switch (*instr) {
            case '>': {
                if (cell++ == cells_end)
                    errx(1, "out of memory");
                break;
            }
            case '<': {
                if (cell > cells)
                    cell--;
                break;
            }
            case '+': {
                (*cell)++;
                break;
            }
            case '-': {
                (*cell)--;
                break;
            }
            case '.': {
                if (putchar(*cell) == EOF)
                    err(1, "(stdout)");
                break;
            }
            case ',': {
                if (read(STDIN_FILENO, cell, 1) == -1)
                    err(1, "(stdin)");
                break;
            }
            case '[': {
                if (*cell == 0) {
                    int count = 0;
                    while (true) {
                        instr++;
                        if (*instr == ']') {
                            if (count == 0)
                                break;
                            count--;
                        } else if (*instr == '[')
                            count++;
                    }
                }
                break;
            }
            case ']': {
                if (*cell != 0) {
                    int count = 0;
                    while (true) {
                        instr--;
                        if (*instr == '[') {
                            if (count == 0)
                                break;
                            count--;
                        } else if (*instr == ']')
                            count++;
                    }
                }
                break;
            }
            default: break;
        }
        instr++;
    }
}

// Traces an entire execution of the program and then runs is a second time
// using JITted code. Expect all output twice in sequence.
void jit(char *prog, char *prog_end) {
    // First run collects a trace.
    char *cells = calloc(1, CELLS_LEN);
    if (cells == NULL)
        err(1, "out of memory");
    char *cells_end = cells + CELLS_LEN;

    void *tt = __yktrace_start_tracing(HW_TRACING, &prog, &prog_end, &cells, &cells_end);
    interp(prog, prog_end, cells, cells_end);
    void *tr = __yktrace_stop_tracing(tt);

    // Compile and run trace.
    void *ptr = __yktrace_irtrace_compile(tr);
    __yktrace_drop_irtrace(tr);

    memset(cells, '\0', CELLS_LEN);
    void (*func)(void *, void*, void *, void *) = (void (*)(void *, void *, void *, void *))ptr;
    func(&prog, &prog_end, &cells, &cells_end);
}

int main(int argc, char *argv[]) {
    if (argc < 2)
        errx(1, "<filename>");

    int fd = open(argv[1], O_RDONLY);
    struct stat sb;
    if (fstat(fd, &sb) != 0)
        err(1, "%s", argv[1]);
    char *prog = malloc(sb.st_size);
    if (read(fd, prog, sb.st_size) != sb.st_size)
        err(1, "%s", argv[1]);

    jit(prog, prog + sb.st_size);
}
