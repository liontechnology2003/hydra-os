#ifndef INCLUDE_VFS_H
#define INCLUDE_VFS_H

#define VFS_NAME_MAX    32
#define VFS_CONTENT_MAX 512
#define VFS_MAX_NODES   64
#define VFS_PATH_MAX    128
#define VFS_EDIT_MAX    16

#define VFS_ERR_OK          0
#define VFS_ERR_NOT_FOUND   (-1)
#define VFS_ERR_NOT_DIR     (-2)
#define VFS_ERR_NOT_FILE    (-3)
#define VFS_ERR_EXISTS      (-4)
#define VFS_ERR_FULL        (-5)
#define VFS_ERR_NO_SPACE    (-6)
#define VFS_ERR_IS_DIR      (-7)

/** vfs_init:
 *  Initializes the virtual filesystem with a default directory tree.
 */
void vfs_init(void);

/** vfs_resolve:
 *  Resolves a path (absolute or relative) to a node index.
 *
 *  @param path  The path to resolve.  "~" expands to the user's home.
 *  @return      The node index, or VFS_ERR_NOT_FOUND
 */
int vfs_resolve(const char *path);

/** vfs_name:
 *  Returns the name of the given node index.
 */
char *vfs_name(int idx);

/** vfs_is_dir:
 *  Returns 1 if the node is a directory, 0 otherwise.
 */
int vfs_is_dir(int idx);

/** vfs_parent:
 *  Returns the parent node index of the given node.
 */
int vfs_parent(int idx);

/** vfs_content:
 *  Returns the content pointer and sets *size for a file node.
 *  Returns 0 on failure (not a file).
 */
char *vfs_content(int idx, int *size);

/** vfs_read_file:
 *  Reads the content of a file into buf, up to maxlen.
 *  Returns bytes written, or VFS_ERR_* on error.
 */
int vfs_read_file(const char *path, char *buf, int maxlen);

/** vfs_write_file:
 *  Creates or overwrites a file with the given content.
 *  Flags: 1 = truncate/overwrite, 2 = append.
 *  Returns VFS_ERR_OK on success.
 */
int vfs_write_file(const char *path, const char *content, int len, int flags);

/** vfs_edits_list:
 *  Lists files created or edited during this session into buf.
 *  Returns bytes written to buf.
 */
int vfs_edits_list(char *buf, int maxlen);

/** vfs_edits_clear:
 *  Clears the edited-file session memory.
 */
void vfs_edits_clear(void);

/** vfs_mkdir:
 *  Creates a directory.
 *  Returns VFS_ERR_OK on success.
 */
int vfs_mkdir(const char *path);

/** vfs_touch:
 *  Creates an empty file (or updates timestamps if extant).
 *  Returns VFS_ERR_OK on success.
 */
int vfs_touch(const char *path);

/** vfs_rm:
 *  Removes a file or empty directory.
 *  Returns VFS_ERR_OK on success.
 */
int vfs_rm(const char *path);

/** vfs_ls:
 *  Lists the contents of a directory into buf.
 *  Returns bytes written to buf.
 */
int vfs_ls(const char *path, char *buf, int maxlen);

/** vfs_cwd_path:
 *  Writes the current working directory path into buf.
 */
void vfs_cwd_path(char *buf, int maxlen);

/** vfs_set_cwd:
 *  Sets the current working directory.
 *  Returns VFS_ERR_OK on success.
 */
int vfs_set_cwd(const char *path);

#endif /* INCLUDE_VFS_H */
