#include "vfs.h"
#include "string.h"

/* --- Internal node representation --- */

typedef struct vfs_node {
    char name[VFS_NAME_MAX];
    int parent;
    int is_dir;
    char content[VFS_CONTENT_MAX];
    int size;
} vfs_node;

static vfs_node nodes[VFS_MAX_NODES];
static int node_count = 0;
static int cwd = 0;   /* node index of current working directory */
static int home = 0;  /* node index of /home/redlion */

/** vfs_make_node:
 *  Allocates a new node, returning its index or -1 on failure.
 */
static int vfs_make_node(const char *name, int parent, int is_dir)
{
    if (node_count >= VFS_MAX_NODES) {
        return VFS_ERR_FULL;
    }
    memset(nodes[node_count].name, 0, VFS_NAME_MAX);
    strncpy(nodes[node_count].name, name, VFS_NAME_MAX - 1);
    nodes[node_count].parent = parent;
    nodes[node_count].is_dir = is_dir;
    nodes[node_count].size = 0;
    memset(nodes[node_count].content, 0, VFS_CONTENT_MAX);
    return node_count++;
}

/** vfs_find_child:
 *  Finds a direct child of parent_idx with the given name.
 *  Returns node index, or -1 if not found.
 */
static int vfs_find_child(int parent_idx, const char *name)
{
    int i;

    for (i = 0; i < node_count; i++) {
        if (nodes[i].parent == parent_idx && strcmp(nodes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/** vfs_path_depth:
 *  Returns the number of components in an absolute path.
 */
/** vfs_resolve:
 *  Resolves a path (absolute or relative) to a node index.
 *  "~" expands to /home/redlion.
 */
int vfs_resolve(const char *path)
{
    int cur;
    char token[VFS_NAME_MAX];
    char *tp;
    const char *p;
    int is_abs = 0;
    int is_home = 0;
    int len;

    if (*path == '~') {
        path++;
        is_home = 1;
    }

    if (*path == '/') {
        is_abs = 1;
        p = path + 1;
    } else {
        p = path;
    }

    cur = (is_abs || is_home) ? (is_home ? home : 0) : cwd;

    while (*p) {
        tp = token;
        len = 0;
        while (*p && *p != '/') {
            if (len < VFS_NAME_MAX - 1) {
                *tp++ = *p;
                len++;
            }
            p++;
        }
        *tp = '\0';
        while (*p == '/') {
            p++;
        }

        if (strcmp(token, ".") == 0) {
            continue;
        } else if (strcmp(token, "..") == 0) {
            cur = nodes[cur].parent;
            if (cur < 0) {
                cur = 0;
            }
        } else {
            cur = vfs_find_child(cur, token);
            if (cur < 0) {
                return VFS_ERR_NOT_FOUND;
            }
        }
    }

    return cur;
}

/** vfs_init:
 *  Initializes the virtual filesystem with a default directory tree.
 */
void vfs_init(void)
{
    node_count = 0;
    cwd = vfs_make_node("", -1, 1);
    home = vfs_make_node("home", 0, 1);
    vfs_mkdir("/home/redlion");
    vfs_mkdir("/root");
    vfs_mkdir("/bin");
    vfs_mkdir("/boot");
    vfs_mkdir("/dev");
    vfs_mkdir("/etc");
    vfs_mkdir("/proc");
    vfs_mkdir("/tmp");
    vfs_mkdir("/usr/bin");
    vfs_mkdir("/usr/lib");
    vfs_mkdir("/var/log");

    vfs_write_file("/etc/motd",
        "  _   _                       \n"
        " | | | | __ _ _ __   __ _ ___\n"
        " | |_| |/ _` | '_ \\ / _` / __|\n"
        " |  _  | (_| | | | | (_| \\__ \\\n"
        " |_| |_|\\__,_|_| |_|\\__, |___/\n"
        "                     |___/    \n",
        -1, 1);

    vfs_write_file("/etc/hostname",
        "redlion", -1, 1);

    vfs_write_file("/etc/os-release",
        "NAME=\"RedLion OS\"\n"
        "VERSION=\"1.0.0\"\n"
        "ID=redlion\n"
        "PRETTY_NAME=\"RedLion OS 1.0.0\"\n"
        "HOME_URL=\"https://github.com/anass/redlion-os\"\n",
        -1, 1);

    vfs_write_file("/home/redlion/welcome.txt",
        "Welcome to RedLion OS!\n"
        "Type 'help' for available commands.\n",
        -1, 1);

    vfs_write_file("/home/redlion/todo.txt",
        "1. Improve keyboard driver\n"
        "2. Add process scheduler\n"
        "3. Add ext2 filesystem\n",
        -1, 1);

    cwd = home;
}

/** vfs_name:
 *  Returns the name of the given node index.
 */
char *vfs_name(int idx)
{
    if (idx < 0 || idx >= node_count) {
        return "";
    }
    return nodes[idx].name;
}

/** vfs_is_dir:
 *  Returns 1 if the node is a directory, 0 otherwise.
 */
int vfs_is_dir(int idx)
{
    if (idx < 0 || idx >= node_count) {
        return 0;
    }
    return nodes[idx].is_dir;
}

/** vfs_parent:
 *  Returns the parent node index of the given node.
 */
int vfs_parent(int idx)
{
    if (idx < 0 || idx >= node_count) {
        return -1;
    }
    return nodes[idx].parent;
}

/** vfs_content:
 *  Returns the content pointer and sets *size for a file node.
 */
char *vfs_content(int idx, int *size)
{
    if (idx < 0 || idx >= node_count || nodes[idx].is_dir) {
        if (size) {
            *size = 0;
        }
        return 0;
    }
    if (size) {
        *size = nodes[idx].size;
    }
    return nodes[idx].content;
}

/** vfs_read_file:
 *  Reads the content of a file into buf, up to maxlen.
 */
int vfs_read_file(const char *path, char *buf, int maxlen)
{
    int idx;
    int sz;
    int to_copy;
    char *c;

    idx = vfs_resolve(path);
    if (idx < 0) {
        return idx;
    }
    if (nodes[idx].is_dir) {
        return VFS_ERR_NOT_FILE;
    }
    c = vfs_content(idx, &sz);
    if (!c) {
        return 0;
    }
    if (maxlen <= 0 || buf == 0) {
        return sz;
    }
    to_copy = sz < maxlen - 1 ? sz : maxlen - 1;
    memcpy(buf, c, to_copy);
    buf[to_copy] = '\0';
    return to_copy;
}

/** vfs_write_file:
 *  Creates or overwrites a file with the given content.
 */
int vfs_write_file(const char *path, const char *content, int len, int flags)
{
    int idx;
    int parent_idx;
    int new_idx;
    char name_buf[VFS_NAME_MAX];
    const char *basename;
    int i;

    /* Find the basename of the path */
    basename = path;
    i = strlen(path);
    while (i > 0 && path[i - 1] == '/') {
        i--;
    }
    i--;
    if (i > 0 && path[i - 1] == '/') {
        i--;
    }
    /* i now points to the last '/' before the name, or -1 */
    if (i < 0) {
        if (*path == '/') {
            return VFS_ERR_NOT_FOUND;
        }
        /* bare filename – relative to cwd */
        basename = path;
        parent_idx = cwd;
    } else {
        basename = path + i + 1;
        strncpy(name_buf, path, i + 1);
        name_buf[i + 1] = '\0';
        parent_idx = vfs_resolve(name_buf);
        if (parent_idx < 0) {
            return VFS_ERR_NOT_FOUND;
        }
        if (!nodes[parent_idx].is_dir) {
            return VFS_ERR_NOT_DIR;
        }
    }

    /* If the file exists, overwrite/append */
    idx = vfs_find_child(parent_idx, basename);
    if (idx >= 0 && !nodes[idx].is_dir) {
        if (flags == 2) {
            /* append */
            int space = VFS_CONTENT_MAX - nodes[idx].size;
            int to_write = len < 0 ? (int)strlen(content) : len;
            if (to_write > space) {
                to_write = space;
            }
            if (to_write > 0) {
                memcpy(nodes[idx].content + nodes[idx].size, content, to_write);
                nodes[idx].size += to_write;
            }
        } else {
            /* truncate and write */
            int to_write = len < 0 ? (int)strlen(content) : len;
            if (to_write > VFS_CONTENT_MAX) {
                to_write = VFS_CONTENT_MAX;
            }
            nodes[idx].size = 0;
            if (to_write > 0) {
                memcpy(nodes[idx].content, content, to_write);
                nodes[idx].size = to_write;
            }
        }
        return VFS_ERR_OK;
    }

    /* If path already exists as a directory, error */
    if (idx >= 0) {
        return VFS_ERR_IS_DIR;
    }

    /* Create a new file node */
    new_idx = vfs_make_node(basename, parent_idx, 0);
    if (new_idx < 0) {
        return new_idx;
    }
    if (len < 0) {
        len = (int)strlen(content);
    }
    if (len > VFS_CONTENT_MAX) {
        len = VFS_CONTENT_MAX;
    }
    if (len > 0) {
        memcpy(nodes[new_idx].content, content, len);
        nodes[new_idx].size = len;
    }
    return VFS_ERR_OK;
}

/** vfs_mkdir:
 *  Creates a directory.
 */
int vfs_mkdir(const char *path)
{
    int parent_idx;
    int new_idx;
    char name_buf[VFS_NAME_MAX];
    const char *basename;
    int i;

    if (strcmp(path, "/") == 0) {
        return VFS_ERR_EXISTS;
    }

    basename = path;
    i = strlen(path);
    while (i > 0 && path[i - 1] == '/') {
        i--;
    }
    i--;
    if (i > 0 && path[i - 1] == '/') {
        i--;
    }

    if (i < 0) {
        if (*path == '/') {
            return VFS_ERR_EXISTS;
        }
        basename = path;
        parent_idx = cwd;
    } else {
        basename = path + i + 1;
        strncpy(name_buf, path, i + 1);
        name_buf[i + 1] = '\0';
        parent_idx = vfs_resolve(name_buf);
        if (parent_idx < 0) {
            return VFS_ERR_NOT_FOUND;
        }
        if (!nodes[parent_idx].is_dir) {
            return VFS_ERR_NOT_DIR;
        }
    }

    if (vfs_find_child(parent_idx, basename) >= 0) {
        return VFS_ERR_EXISTS;
    }

    new_idx = vfs_make_node(basename, parent_idx, 1);
    if (new_idx < 0) {
        return new_idx;
    }
    return VFS_ERR_OK;
}

/** vfs_touch:
 *  Creates an empty file.
 */
int vfs_touch(const char *path)
{
    int idx;

    idx = vfs_resolve(path);
    if (idx >= 0) {
        return VFS_ERR_OK;
    }
    return vfs_write_file(path, "", 0, 1);
}

/** vfs_rm:
 *  Removes a file or empty directory.
 */
int vfs_rm(const char *path)
{
    int idx;
    int i;

    if (strcmp(path, "/") == 0) {
        return VFS_ERR_EXISTS;
    }

    idx = vfs_resolve(path);
    if (idx < 0) {
        return idx;
    }

    /* Cannot remove root */
    if (idx == 0) {
        return VFS_ERR_EXISTS;
    }

    /* Directory must be empty */
    if (nodes[idx].is_dir) {
        for (i = 0; i < node_count; i++) {
            if (nodes[i].parent == idx) {
                return VFS_ERR_NO_SPACE;
            }
        }
    }

    /* Remove by shifting all later nodes down */
    for (i = idx; i < node_count - 1; i++) {
        nodes[i] = nodes[i + 1];
    }
    node_count--;

    /* Fix parent references for children that shifted */
    for (i = 0; i < node_count; i++) {
        if (nodes[i].parent > idx) {
            nodes[i].parent--;
        }
    }

    /* Fix cwd/home if needed */
    if (cwd >= node_count) {
        cwd = home;
    }
    if (home >= node_count) {
        home = 0;
    }

    return VFS_ERR_OK;
}

/** vfs_ls:
 *  Lists the contents of a directory into buf.
 */
int vfs_ls(const char *path, char *buf, int maxlen)
{
    int idx;
    int i;
    int pos = 0;

    idx = vfs_resolve(path);
    if (idx < 0) {
        return idx;
    }
    if (!nodes[idx].is_dir) {
        return VFS_ERR_NOT_DIR;
    }

    for (i = 0; i < node_count; i++) {
        if (nodes[i].parent == idx) {
            if (pos + (int)strlen(nodes[i].name) + 1 < maxlen) {
                strcpy(buf + pos, nodes[i].name);
                pos += (int)strlen(nodes[i].name);
            }
            if (nodes[i].is_dir) {
                if (pos + 1 < maxlen) {
                    buf[pos++] = '/';
                }
            }
            if (pos < maxlen) {
                buf[pos++] = '\n';
            }
        }
    }

    if (buf != 0 && maxlen > 0) {
        buf[pos < maxlen ? pos : maxlen - 1] = '\0';
    }
    return pos;
}

/** vfs_cwd_path:
 *  Writes the current working directory path into buf.
 */
void vfs_cwd_path(char *buf, int maxlen)
{
    char parts[16][VFS_NAME_MAX];
    int idx;
    int depth = 0;
    int i;
    int pos = 0;

    if (maxlen <= 0) {
        return;
    }

    /* If cwd is home, display ~ */
    if (cwd == home) {
        buf[0] = '~';
        buf[1] = '\0';
        return;
    }

    idx = cwd;
    while (idx > 0 && depth < 16) {
        strncpy(parts[depth], nodes[idx].name, VFS_NAME_MAX - 1);
        depth++;
        idx = nodes[idx].parent;
    }

    if (depth == 0) {
        buf[0] = '\0';
        return;
    }

    buf[pos++] = '/';
    for (i = depth - 1; i >= 0; i--) {
        int nlen = (int)strlen(parts[i]);
        if (pos + nlen < maxlen) {
            strcpy(buf + pos, parts[i]);
            pos += nlen;
        }
        if (i > 0 && pos < maxlen - 1) {
            buf[pos++] = '/';
        }
    }
    buf[pos < maxlen ? pos : maxlen - 1] = '\0';
}

/** vfs_set_cwd:
 *  Sets the current working directory.
 */
int vfs_set_cwd(const char *path)
{
    int idx;

    idx = vfs_resolve(path);
    if (idx < 0) {
        return idx;
    }
    if (!nodes[idx].is_dir) {
        return VFS_ERR_NOT_DIR;
    }
    cwd = idx;
    return VFS_ERR_OK;
}
