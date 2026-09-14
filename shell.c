#include "shell.h"
#include "fb.h"
#include "keyboard.h"
#include "string.h"
#include "vfs.h"
#include "lineedit.h"
#include "snake.h"

/* =========================================================
 *  Stream abstraction (stdout redirection)
 * ========================================================= */

#define STREAM_FB     0
#define STREAM_FILE   1
#define STREAM_PIPE   2

static int   stream_target = STREAM_FB;
static char  pipe_buffer[4096];
static int   pipe_len = 0;
static int   pipe_active = 0;

static int last_status = 0;

void shell_putc(char c)
{
    if (stream_target == STREAM_FB) {
        fb_putc(c);
    } else if (stream_target == STREAM_PIPE) {
        if (pipe_len < 4095) {
            pipe_buffer[pipe_len++] = c;
            pipe_buffer[pipe_len] = '\0';
        }
    }
}

void shell_puts(const char *str)
{
    while (*str) {
        shell_putc(*str++);
    }
}

void shell_print_int(int val)
{
    char buf[16];
    itoa(buf, val);
    shell_puts(buf);
}

void shell_print_unsigned(unsigned int val)
{
    char buf[16];
    utoa(buf, val);
    shell_puts(buf);
}

/* =========================================================
 *  Environment table
 * ========================================================= */

#define ENV_MAX 32
#define ENV_LEN 128

static char env_table[ENV_MAX][ENV_LEN];
static int  env_count = 0;

static void env_init(void)
{
    env_count = 0;
    strcpy(env_table[env_count++], "USER=redlion");
    strcpy(env_table[env_count++], "HOME=/home/redlion");
    strcpy(env_table[env_count++], "HOSTNAME=redlion");
    strcpy(env_table[env_count++], "SHELL=/bin/bash");
    strcpy(env_table[env_count++], "PATH=/usr/bin:/bin");
    strcpy(env_table[env_count++], "TERM=linux");
    strcpy(env_table[env_count++], "LOGNAME=redlion");
}

static const char *env_get(const char *name)
{
    int i;
    int nlen = (int)strlen(name);

    for (i = 0; i < env_count; i++) {
        if (strncmp(env_table[i], name, nlen) == 0 && env_table[i][nlen] == '=') {
            return env_table[i] + nlen + 1;
        }
    }
    return 0;
}

static void env_set(const char *name, const char *value)
{
    int i;
    int nlen = (int)strlen(name);

    for (i = 0; i < env_count; i++) {
        if (strncmp(env_table[i], name, nlen) == 0 && env_table[i][nlen] == '=') {
            strncpy(env_table[i] + nlen + 1, value, ENV_LEN - nlen - 2);
            return;
        }
    }
    if (env_count < ENV_MAX) {
        strncpy(env_table[env_count], name, ENV_LEN - 1);
        env_table[env_count][nlen] = '=';
        strncpy(env_table[env_count] + nlen + 1, value, ENV_LEN - nlen - 2);
        env_count++;
    }
}

static void env_unset(const char *name)
{
    int i;
    int nlen = (int)strlen(name);

    for (i = 0; i < env_count; i++) {
        if (strncmp(env_table[i], name, nlen) == 0 && env_table[i][nlen] == '=') {
            for (; i < env_count - 1; i++) {
                strcpy(env_table[i], env_table[i + 1]);
            }
            env_count--;
            return;
        }
    }
}

static void env_list(void)
{
    int i;
    for (i = 0; i < env_count; i++) {
        shell_puts(env_table[i]);
        shell_putc('\n');
    }
}

/* =========================================================
 *  Variable expansion
 * ========================================================= */

static int expand_line(const char *in, char *out, int outsize)
{
    int pos = 0;

    while (*in && pos < outsize - 1) {
        if (*in == '$' && in[1] == '{') {
            const char *start = in + 2;
            const char *end = strchr(start, '}');
            char name[32];
            const char *val;
            int nlen;

            if (!end) {
                out[pos++] = '$';
                in++;
                continue;
            }
            nlen = (int)(end - start);
            if (nlen > 31) nlen = 31;
            strncpy(name, start, nlen);
            name[nlen] = '\0';
            val = env_get(name);
            if (val) {
                while (*val && pos < outsize - 1) {
                    out[pos++] = *val++;
                }
            }
            in = end + 1;
        } else if (*in == '$' && in[1] != '\0') {
            const char *start = in + 1;
            char name[32];
            const char *val;
            int nlen = 0;

            while (*start && *start != ' ' && *start != '"' &&
                   *start != '\'' && *start != '\n') {
                nlen++;
                start++;
            }
            if (nlen > 31) nlen = 31;
            strncpy(name, in + 1, nlen);
            name[nlen] = '\0';
            val = env_get(name);
            if (val) {
                while (*val && pos < outsize - 1) {
                    out[pos++] = *val++;
                }
            }
            in = start;
        } else if (*in == '\\' && in[1] == '$') {
            in++;
            out[pos++] = *in++;
        } else {
            out[pos++] = *in++;
        }
    }
    out[pos] = '\0';
    return pos;
}

/* =========================================================
 *  Tokenizer
 * ========================================================= */

#define MAX_TOKENS 64

static int tokenize(char *line, char **argv, int max_argv)
{
    int argc = 0;

    while (*line && argc < max_argv) {
        while (*line == ' ' || *line == '\t') {
            line++;
        }
        if (*line == '\0') {
            break;
        }

        if (*line == '#') {
            break;
        }

        if (line[0] == '&' && line[1] == '&') {
            argv[argc++] = "&&";
            line += 2;
            continue;
        }
        if (line[0] == '|' && line[1] == '|') {
            argv[argc++] = "||";
            line += 2;
            continue;
        }
        if (line[0] == '>' && line[1] == '>') {
            argv[argc++] = ">>";
            line += 2;
            continue;
        }
        if (line[0] == '|' || line[0] == ';' || line[0] == '>') {
            argv[argc] = line;
            argv[argc][1] = '\0';
            argc++;
            line++;
            continue;
        }

        if (*line == '"' || *line == '\'') {
            char quote = *line++;
            argv[argc] = line;
            while (*line && *line != quote) {
                line++;
            }
            if (*line == quote) {
                *line = '\0';
                line++;
            }
            argc++;
            continue;
        }

        argv[argc] = line;
        while (*line && *line != ' ' && *line != '\t' &&
               *line != '|' && *line != ';' && *line != '&' &&
               *line != '>' && *line != '<') {
            line++;
        }
        if (*line) {
            *line = '\0';
            line++;
        }
        argc++;
    }
    return argc;
}

/* =========================================================
 *  Command execution (recursive, needed by scripting)
 * ========================================================= */

static int execute_line(const char *raw_line);

/** is_operator:
 *  Returns 1 if the token is a shell operator.
 */
static int is_operator(const char *tok)
{
    return (strcmp(tok, "&&") == 0 || strcmp(tok, "||") == 0 ||
            strcmp(tok, "|") == 0 || strcmp(tok, ";") == 0 ||
            strcmp(tok, ">") == 0 || strcmp(tok, ">>") == 0);
}

/* =========================================================
 *  Builtins
 * ========================================================= */

static int builtin_help(char **argv, int argc)
{
    (void)argc; (void)argv;
    shell_puts("Available commands:\n");
    shell_puts("  help              Display this help message\n");
    shell_puts("  clear             Clear the screen\n");
    shell_puts("  echo              Print arguments (-n: no newline)\n");
    shell_puts("  about             Display OS information\n");
    shell_puts("  play              Play Snake game\n");
    shell_puts("  pwd               Print working directory\n");
    shell_puts("  ls [-l] [path]    List directory contents\n");
    shell_puts("  cd [path]         Change directory\n");
    shell_puts("  mkdir <name>      Create a directory\n");
    shell_puts("  touch <name>      Create an empty file\n");
    shell_puts("  rm <name>         Remove a file or empty directory\n");
    shell_puts("  cat <file>        Print file contents (or stdin)\n");
    shell_puts("  uname [-a]        Print system information\n");
    shell_puts("  hostname [name]   Print or set hostname\n");
    shell_puts("  whoami            Print current user\n");
    shell_puts("  version           Print version information\n");
    shell_puts("  history           Print command history\n");
    shell_puts("  env               Print environment variables\n");
    shell_puts("  export NAME=val   Set an environment variable\n");
    shell_puts("  unset NAME        Remove an environment variable\n");
    shell_puts("\n");
    shell_puts("Operators: ; && || | > >>\n");
    shell_puts("Scripting: if/then/else/fi  while/do/done  for/in/do/done\n");
    return 0;
}

static int builtin_clear(char **argv, int argc)
{
    (void)argc; (void)argv;
    fb_clear();
    return 0;
}

static int builtin_echo(char **argv, int argc)
{
    int no_newline = 0;
    int i;

    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        no_newline = 1;
        i = 2;
    } else {
        i = 1;
    }

    for (; i < argc; i++) {
        shell_puts(argv[i]);
        if (i < argc - 1) {
            shell_putc(' ');
        }
    }
    if (!no_newline) {
        shell_putc('\n');
    }
    return 0;
}

static int builtin_about(char **argv, int argc)
{
    (void)argc; (void)argv;
    shell_puts("RedLion OS - A minimal x86 operating system\n");
    shell_puts("Shell: bash-like with scripting, pipes, and virtual filesystem\n");
    shell_puts("Built with clang/lld on MSYS2\n");
    return 0;
}

static int builtin_pwd(char **argv, int argc)
{
    char buf[VFS_PATH_MAX];
    (void)argc; (void)argv;
    vfs_cwd_path(buf, VFS_PATH_MAX);
    shell_puts(buf);
    shell_putc('\n');
    return 0;
}

static int builtin_cd(char **argv, int argc)
{
    const char *target;
    int r;

    if (argc < 2) {
        const char *home = env_get("HOME");
        target = home ? home : "/";
    } else {
        target = argv[1];
    }

    r = vfs_set_cwd(target);
    if (r == VFS_ERR_NOT_FOUND) {
        shell_puts("bash: cd: ");
        shell_puts(target);
        shell_puts(": No such file or directory\n");
        return 1;
    }
    if (r == VFS_ERR_NOT_DIR) {
        shell_puts("bash: cd: ");
        shell_puts(target);
        shell_puts(": Not a directory\n");
        return 1;
    }
    return 0;
}

static int builtin_ls(char **argv, int argc)
{
    char ls_buf[4096];
    int len;
    const char *path;
    int long_format = 0;

    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        long_format = 1;
        path = argc > 2 ? argv[2] : ".";
    } else {
        path = argc > 1 ? argv[1] : ".";
    }

    len = vfs_ls(path, ls_buf, 4096);
    if (len < 0) {
        shell_puts("bash: ls: ");
        shell_puts(path);
        shell_puts(": No such file or directory\n");
        return 1;
    }

    if (long_format) {
        shell_puts("total 0\n");
    }
    if (len > 0) {
        shell_puts(ls_buf);
    }
    return 0;
}

static int builtin_mkdir(char **argv, int argc)
{
    int r;

    if (argc < 2) {
        shell_puts("bash: mkdir: missing operand\n");
        return 1;
    }
    r = vfs_mkdir(argv[1]);
    if (r == VFS_ERR_EXISTS) {
        shell_puts("bash: mkdir: cannot create directory '");
        shell_puts(argv[1]);
        shell_puts("': File exists\n");
        return 1;
    }
    if (r == VFS_ERR_NOT_FOUND) {
        shell_puts("bash: mkdir: cannot create directory '");
        shell_puts(argv[1]);
        shell_puts("': No such file or directory\n");
        return 1;
    }
    return 0;
}

static int builtin_touch(char **argv, int argc)
{
    int r;

    if (argc < 2) {
        shell_puts("bash: touch: missing file operand\n");
        return 1;
    }
    r = vfs_touch(argv[1]);
    if (r == VFS_ERR_NOT_FOUND) {
        shell_puts("bash: touch: cannot touch '");
        shell_puts(argv[1]);
        shell_puts("': No such file or directory\n");
        return 1;
    }
    return 0;
}

static int builtin_rm(char **argv, int argc)
{
    int r;

    if (argc < 2) {
        shell_puts("bash: rm: missing operand\n");
        return 1;
    }
    r = vfs_rm(argv[1]);
    if (r == VFS_ERR_NOT_FOUND) {
        shell_puts("bash: rm: cannot remove '");
        shell_puts(argv[1]);
        shell_puts("': No such file or directory\n");
        return 1;
    }
    if (r == VFS_ERR_NO_SPACE) {
        shell_puts("bash: rm: cannot remove '");
        shell_puts(argv[1]);
        shell_puts("': Directory not empty\n");
        return 1;
    }
    return 0;
}

static int builtin_cat(char **argv, int argc)
{
    char content[VFS_CONTENT_MAX + 1];
    int len;

    if (pipe_active) {
        shell_puts(pipe_buffer);
        return 0;
    }

    if (argc < 2) {
        shell_puts("bash: cat: missing file operand\n");
        return 1;
    }

    len = vfs_read_file(argv[1], content, VFS_CONTENT_MAX);
    if (len < 0) {
        shell_puts("bash: cat: ");
        shell_puts(argv[1]);
        shell_puts(": No such file or directory\n");
        return 1;
    }
    content[len] = '\0';
    shell_puts(content);
    return 0;
}

static int builtin_uname(char **argv, int argc)
{
    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        shell_puts("Linux redlion 1.0.0 #1 ");
        shell_puts(__DATE__);
        shell_puts(" i386 RedLionOS GNU/RedLion\n");
        return 0;
    }
    shell_puts("Linux\n");
    return 0;
}

static int builtin_hostname(char **argv, int argc)
{
    if (argc > 1) {
        env_set("HOSTNAME", argv[1]);
        return 0;
    }
    shell_puts(env_get("HOSTNAME"));
    shell_putc('\n');
    return 0;
}

static int builtin_whoami(char **argv, int argc)
{
    (void)argc; (void)argv;
    shell_puts(env_get("USER"));
    shell_putc('\n');
    return 0;
}

static int builtin_version(char **argv, int argc)
{
    (void)argc; (void)argv;
    shell_puts("RedLion OS 1.0.0\n");
    shell_puts("Kernel: i386 monolithic\n");
    shell_puts("Shell: bash-compatible\n");
    return 0;
}

static int builtin_env(char **argv, int argc)
{
    (void)argc; (void)argv;
    env_list();
    return 0;
}

static int builtin_export(char **argv, int argc)
{
    char name[32];
    const char *eq;
    int nlen;

    if (argc < 2) {
        env_list();
        return 0;
    }

    eq = strchr(argv[1], '=');
    if (!eq) {
        shell_puts("bash: export: '");
        shell_puts(argv[1]);
        shell_puts("': not a valid identifier\n");
        return 1;
    }
    nlen = (int)(eq - argv[1]);
    if (nlen > 31) nlen = 31;
    strncpy(name, argv[1], nlen);
    name[nlen] = '\0';
    env_set(name, eq + 1);
    return 0;
}

static int builtin_unset(char **argv, int argc)
{
    if (argc < 2) {
        shell_puts("bash: unset: missing operand\n");
        return 1;
    }
    env_unset(argv[1]);
    return 0;
}

static int builtin_play(char **argv, int argc)
{
    (void)argc; (void)argv;
    snake_game();
    fb_clear();
    return 0;
}

typedef struct {
    const char *name;
    int (*func)(char **argv, int argc);
} builtin_cmd;

static builtin_cmd builtins[] = {
    {"help",     builtin_help},
    {"clear",    builtin_clear},
    {"echo",     builtin_echo},
    {"about",    builtin_about},
    {"pwd",      builtin_pwd},
    {"cd",       builtin_cd},
    {"ls",       builtin_ls},
    {"mkdir",    builtin_mkdir},
    {"touch",    builtin_touch},
    {"rm",       builtin_rm},
    {"cat",      builtin_cat},
    {"uname",    builtin_uname},
    {"hostname", builtin_hostname},
    {"whoami",   builtin_whoami},
    {"version",  builtin_version},
    {"env",      builtin_env},
    {"export",   builtin_export},
    {"set",      builtin_env},
    {"unset",    builtin_unset},
    {"play",     builtin_play},
    {0, 0}
};

static int (*builtin_lookup(const char *name))(char **, int)
{
    int i;

    for (i = 0; builtins[i].name; i++) {
        if (strcmp(builtins[i].name, name) == 0) {
            return builtins[i].func;
        }
    }
    return 0;
}

/* =========================================================
 *  Single command execution (with redirection)
 * ========================================================= */

static int run_single_command(char **argv, int argc)
{
    int (*func)(char **, int);
    int i;
    int new_argc;
    char *new_argv[MAX_TOKENS];
    const char *outfile = 0;
    int is_append = 0;
    int prev_target = stream_target;

    new_argc = 0;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], ">") == 0) {
            if (i + 1 < argc) {
                outfile = argv[++i];
                is_append = 0;
            }
        } else if (strcmp(argv[i], ">>") == 0) {
            if (i + 1 < argc) {
                outfile = argv[++i];
                is_append = 1;
            }
        } else if (strcmp(argv[i], "<") == 0) {
            i++;
        } else {
            if (new_argc < MAX_TOKENS) {
                new_argv[new_argc++] = argv[i];
            }
        }
    }
    new_argv[new_argc] = 0;

    if (new_argc == 0) {
        return 0;
    }

    /* Handle variable assignment: VAR=value */
    {
        const char *eq = strchr(new_argv[0], '=');
        if (eq && eq != new_argv[0] && eq[1] != '\0') {
            int nlen = (int)(eq - new_argv[0]);
            char vname[32];
            if (nlen > 31) nlen = 31;
            strncpy(vname, new_argv[0], nlen);
            vname[nlen] = '\0';
            env_set(vname, eq + 1);
            return 0;
        }
    }

    /* Set up output redirection */
    if (outfile) {
        vfs_write_file(outfile, "", 0, is_append ? 2 : 1);
        stream_target = STREAM_PIPE; /* reuse pipe buffer as write buffer */
        pipe_len = 0;
        pipe_buffer[0] = '\0';
    }

    func = builtin_lookup(new_argv[0]);
    if (func) {
        last_status = func(new_argv, new_argc);
    } else {
        shell_puts("bash: ");
        shell_puts(new_argv[0]);
        shell_puts(": command not found\n");
        last_status = 127;
    }

    /* Flush redirected output into the file */
    if (outfile) {
        vfs_write_file(outfile, pipe_buffer, pipe_len, is_append ? 2 : 1);
        stream_target = prev_target;
        pipe_len = 0;
        pipe_buffer[0] = '\0';
    }

    return last_status;
}

/* =========================================================
 *  Test conditions
 * ========================================================= */

static int test_condition_str(const char *cond)
{
    char expanded[256];
    char *argv[8];
    int argc;
    char *e;

    expand_line(cond, expanded, 256);
    e = expanded;
    argc = 0;
    while (*e && argc < 8) {
        while (*e == ' ') {
            *e++ = '\0';
        }
        if (*e) {
            argv[argc++] = e;
            while (*e && *e != ' ') {
                e++;
            }
        }
    }

    if (argc == 1) {
        return run_single_command(argv, argc);
    }

    if (argc == 3) {
        if (strcmp(argv[1], "=") == 0 || strcmp(argv[1], "==") == 0) {
            return strcmp(argv[0], argv[2]) != 0 ? 0 : 1;
        }
        if (strcmp(argv[1], "!=") == 0) {
            return strcmp(argv[0], argv[2]) == 0 ? 0 : 1;
        }
        if (strcmp(argv[0], "-e") == 0) {
            return vfs_resolve(argv[1]) < 0 ? 1 : 0;
        }
        if (strcmp(argv[0], "-f") == 0) {
            int idx = vfs_resolve(argv[1]);
            return (idx >= 0 && !vfs_is_dir(idx)) ? 0 : 1;
        }
        if (strcmp(argv[0], "-d") == 0) {
            int idx = vfs_resolve(argv[1]);
            return (idx >= 0 && vfs_is_dir(idx)) ? 0 : 1;
        }
        if (strcmp(argv[1], "-eq") == 0) {
            return atoi(argv[0]) == atoi(argv[2]) ? 0 : 1;
        }
        if (strcmp(argv[1], "-ne") == 0) {
            return atoi(argv[0]) != atoi(argv[2]) ? 0 : 1;
        }
        if (strcmp(argv[1], "-lt") == 0) {
            return atoi(argv[0]) < atoi(argv[2]) ? 0 : 1;
        }
        if (strcmp(argv[1], "-gt") == 0) {
            return atoi(argv[0]) > atoi(argv[2]) ? 0 : 1;
        }
        if (strcmp(argv[1], "-le") == 0) {
            return atoi(argv[0]) <= atoi(argv[2]) ? 0 : 1;
        }
        if (strcmp(argv[1], "-ge") == 0) {
            return atoi(argv[0]) >= atoi(argv[2]) ? 0 : 1;
        }
    }

    return run_single_command(argv, argc);
}

/* =========================================================
 *  execute_line: main command dispatcher
 * ========================================================= */

static int execute_line(const char *raw_line)
{
    char line[512];
    char expanded[512];
    char *argv[MAX_TOKENS];
    int argc;
    int result = 0;
    int run_next = 1;
    int prev_was_and = 0;
    int prev_was_or = 0;
    int i;

    expand_line(raw_line, expanded, 512);
    strncpy(line, expanded, 511);
    line[511] = '\0';

    argc = tokenize(line, argv, MAX_TOKENS);
    if (argc == 0) {
        return 0;
    }

    i = 0;
    while (i < argc) {
        char *cmd_argv[MAX_TOKENS];
        int cmd_argc = 0;
        int is_pipe = 0;
        int is_and = 0;
        int is_or = 0;

        while (i < argc && !is_operator(argv[i])) {
            if (cmd_argc < MAX_TOKENS) {
                cmd_argv[cmd_argc++] = argv[i];
            }
            i++;
        }

        if (i < argc) {
            if (strcmp(argv[i], "&&") == 0) is_and = 1;
            else if (strcmp(argv[i], "||") == 0) is_or = 1;
            else if (strcmp(argv[i], "|") == 0) is_pipe = 1;
            i++;
        }

        if (prev_was_and && result != 0) {
            run_next = 0;
        } else if (prev_was_or && result == 0) {
            run_next = 0;
        } else {
            run_next = 1;
        }

        if (run_next && cmd_argc > 0) {
            if (is_pipe) {
                char *right_argv[MAX_TOKENS];
                int right_argc = 0;
                int saved_target = stream_target;

                pipe_len = 0;
                pipe_buffer[0] = '\0';
                stream_target = STREAM_PIPE;

                result = run_single_command(cmd_argv, cmd_argc);
                stream_target = saved_target;

                while (i < argc && !is_operator(argv[i])) {
                    if (right_argc < MAX_TOKENS) {
                        right_argv[right_argc++] = argv[i];
                    }
                    i++;
                }

                is_and = 0;
                is_or = 0;
                if (i < argc) {
                    if (strcmp(argv[i], "&&") == 0) is_and = 1;
                    else if (strcmp(argv[i], "||") == 0) is_or = 1;
                    i++;
                }

                if (right_argc > 0) {
                    pipe_active = 1;
                    result = run_single_command(right_argv, right_argc);
                    pipe_active = 0;
                }
            } else {
                result = run_single_command(cmd_argv, cmd_argc);
            }
        }

        prev_was_and = is_and;
        prev_was_or = is_or;
    }

    return result;
}

/* =========================================================
 *  Scripting blocks (if/while/for)
 * ========================================================= */

#define SCRIPT_BUF_SIZE 4096
#define SCRIPT_MAX_LINES 64

static char script_buf[SCRIPT_BUF_SIZE];
static int  script_collecting = 0;
static int  script_type = 0;   /* 0=if, 1=while, 2=for */
static char script_var[32];
static char script_values[16][128];
static int  script_values_count = 0;

/* State kept while a script runs so 'for' can iterate */
static int  script_lines[SCRIPT_MAX_LINES];
static int  script_line_count = 0;
static int  script_next_line = 0;

/** extract_condition:
 *  Extracts the condition from an if/while line.
 */
static const char *extract_condition(const char *line, char *buf, int bufsize)
{
    const char *p;
    int len;

    while (*line && *line != ' ') {
        line++;
    }
    while (*line == ' ') {
        line++;
    }

    if (*line == '[') {
        line++;
        while (*line == ' ') {
            line++;
        }
        p = line;
        while (*p && *p != ']') {
            p++;
        }
        len = (int)(p - line);
        if (len >= bufsize) len = bufsize - 1;
        strncpy(buf, line, len);
        buf[len] = '\0';
        while (len > 0 && buf[len - 1] == ' ') {
            buf[--len] = '\0';
        }
        return buf;
    }

    p = line;
    while (*p) {
        p++;
    }
    len = (int)(p - line);
    if (len >= bufsize) len = bufsize - 1;
    strncpy(buf, line, len);
    buf[len] = '\0';
    return buf;
}

/** split_script:
 *  Splits script_buf into lines.  Returns the number of lines.
 */
static int split_script(void)
{
    char *p = script_buf;
    int count = 0;

    while (*p && count < SCRIPT_MAX_LINES) {
        script_lines[count] = (int)(p - script_buf);
        while (*p && *p != '\n') {
            p++;
        }
        if (*p == '\n') {
            *p++ = '\0';
        }
        count++;
    }
    script_line_count = count;
    return count;
}

/** script_line_ptr:
 *  Returns the string for the nth script line.
 */
static char *script_line_ptr(int n)
{
    return script_buf + script_lines[n];
}

/** execute_script_block:
 *  Interprets the collected script block.
 */
static int execute_script_block(void)
{
    int count;
    int i;
    int result = 0;

    count = split_script();

    /* First line is the block header */
    if (count < 3) {
        script_collecting = 0;
        return 0;
    }

    if (script_type == 0) {
        /* IF block */
        char cond[256];
        int execute_body = 0;
        const char *c = extract_condition(script_line_ptr(0), cond, 256);
        int find_body = test_condition_str(c) == 0;
        int done = 0;

        for (i = 1; i < count && !done; i++) {
            char *line = script_line_ptr(i);

            if (strcmp(line, "then") == 0) {
                if (find_body) {
                    execute_body = 1;
                }
            } else if (strcmp(line, "else") == 0) {
                execute_body = !find_body;
            } else if (strcmp(line, "fi") == 0) {
                done = 1;
            } else if (execute_body) {
                char expanded[512];
                expand_line(line, expanded, 512);
                result = execute_line(expanded);
            }
        }
    } else if (script_type == 1) {
        /* WHILE block */
        char cond[256];
        int continue_loop = 1;

        while (continue_loop) {
            const char *c = extract_condition(script_line_ptr(0), cond, 256);
            continue_loop = (test_condition_str(c) == 0);

            if (continue_loop) {
                for (i = 1; i < count - 1; i++) {
                    char expanded[512];
                    expand_line(script_line_ptr(i), expanded, 512);
                    result = execute_line(expanded);
                }
            }
        }
    } else if (script_type == 2) {
        /* FOR block */
        int v;
        int body_start = 1;
        int body_end = count - 1;

        for (v = 0; v < script_values_count; v++) {
            env_set(script_var, script_values[v]);
            for (i = body_start; i < body_end; i++) {
                char expanded[512];
                expand_line(script_line_ptr(i), expanded, 512);
                result = execute_line(expanded);
            }
        }
    }

    script_collecting = 0;
    script_next_line = 0;
    script_line_count = 0;
    return result;
}

/* =========================================================
 *  Main shell logic
 * ========================================================= */

static char line_buf[256];
static char hist_buf[LINEEDIT_HISTORY_SIZE][256];
static int  prompt_len = 0;

/** draw_prompt:
 *  Prints the bash-style prompt: user@host:path$
 */
static void draw_prompt(void)
{
    char cwd_path[VFS_PATH_MAX];

    vfs_cwd_path(cwd_path, VFS_PATH_MAX);

    fb_set_color(FB_GREEN, FB_BLACK);
    shell_puts(env_get("USER"));
    shell_puts("@");
    shell_puts(env_get("HOSTNAME"));

    fb_set_color(FB_WHITE, FB_BLACK);
    shell_puts(":");

    fb_set_color(FB_LIGHT_BLUE, FB_BLACK);
    shell_puts(cwd_path);

    fb_set_color(FB_WHITE, FB_BLACK);
    shell_puts("$ ");

    prompt_len = (int)strlen(env_get("USER")) + 1 +
                 (int)strlen(env_get("HOSTNAME")) + 1 +
                 (int)strlen(cwd_path) + 2;
}

/** shell_init:
 *  Initializes the shell and draws the welcome banner.
 */
void shell_init(void)
{
    env_init();
    vfs_init();

    fb_clear();
    fb_set_color(FB_LIGHT_GREEN, FB_BLACK);
    shell_puts("Linux redlion 1.0.0 #1 i386 RedLionOS GNU/RedLion\n");
    shell_puts("\n");
    fb_set_color(FB_LIGHT_CYAN, FB_BLACK);
    shell_puts(" ____          _ _     _             \n");
    shell_puts("|  _ \\ ___  __| | |   (_) ___  _ __ \n");
    shell_puts("| |_) / _ \\/ _` | |   | |/ _ \\| '_ \\\n");
    shell_puts("|  _ <  __/ (_| | |___| | (_) | | | |\n");
    shell_puts("|_| \\_\\___|\\__,_|_____|_|\\___/|_| |_|\n");
    fb_set_color(FB_WHITE, FB_BLACK);
    shell_puts("\nType 'help' for available commands.\n\n");

    script_collecting = 0;
    script_line_count = 0;

    draw_prompt();
    lineedit_init(prompt_len, hist_buf);
}

/** shell_update:
 *  Updates the shell (call this in main loop).
 */
void shell_update(void)
{
    int ready;
    char trimmed[256];
    int len;

    ready = lineedit_update(line_buf, 256);
    if (!ready) {
        return;
    }

    shell_putc('\n');

    if (line_buf[0] == '\0') {
        draw_prompt();
        lineedit_init(prompt_len, hist_buf);
        return;
    }

    lineedit_history_add(line_buf, hist_buf);

    strncpy(trimmed, line_buf, 255);
    trimmed[255] = '\0';
    len = (int)strlen(trimmed);

    /* --- Script collection mode --- */
    if (script_collecting) {
        int slen = (int)strlen(script_buf);

        if (strcmp(trimmed, "fi") == 0 || strcmp(trimmed, "done") == 0) {
            if (slen + len + 2 < SCRIPT_BUF_SIZE) {
                strcpy(script_buf + slen, trimmed);
            }
            execute_script_block();
            draw_prompt();
            lineedit_init(prompt_len, hist_buf);
            return;
        }

        if (slen + len + 2 < SCRIPT_BUF_SIZE) {
            strcpy(script_buf + slen, trimmed);
            strcat(script_buf, "\n");
        }
        draw_prompt();
        lineedit_init(prompt_len, hist_buf);
        return;
    }

    /* --- Script block header detection --- */
    if (strncmp(trimmed, "if ", 3) == 0) {
        script_collecting = 1;
        script_type = 0;
        strcpy(script_buf, trimmed);
        strcat(script_buf, "\n");
        draw_prompt();
        lineedit_init(prompt_len, hist_buf);
        return;
    }

    if (strncmp(trimmed, "while ", 6) == 0) {
        script_collecting = 1;
        script_type = 1;
        strcpy(script_buf, trimmed);
        strcat(script_buf, "\n");
        draw_prompt();
        lineedit_init(prompt_len, hist_buf);
        return;
    }

    if (strncmp(trimmed, "for ", 4) == 0) {
        char *p = trimmed + 4;
        int tpos = 0;

        while (*p && *p != ' ') {
            script_var[tpos++] = *p++;
        }
        script_var[tpos] = '\0';
        while (*p == ' ') {
            p++;
        }
        if (strncmp(p, "in ", 3) == 0) {
            p += 3;
        }

        script_values_count = 0;
        while (*p && script_values_count < 16) {
            char token_buf[128];
            tpos = 0;
            while (*p && *p != ' ') {
                token_buf[tpos++] = *p++;
            }
            token_buf[tpos] = '\0';
            if (tpos > 0) {
                expand_line(token_buf, script_values[script_values_count], 128);
                script_values_count++;
            }
            while (*p == ' ') {
                p++;
            }
        }

        script_collecting = 1;
        script_type = 2;
        strcpy(script_buf, trimmed);
        strcat(script_buf, "\n");
        draw_prompt();
        lineedit_init(prompt_len, hist_buf);
        return;
    }

    /* --- Normal command execution --- */
    execute_line(trimmed);

    draw_prompt();
    lineedit_init(prompt_len, hist_buf);
}
