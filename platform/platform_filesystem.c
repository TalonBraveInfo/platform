/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <PL/platform_console.h>

#include "filesystem_private.h"
#include "platform_private.h"

#ifdef _WIN32

#include <afxres.h>
#include <secext.h>

#endif

/*	File System	*/

PLresult _InitIO(void) {
    return PL_RESULT_SUCCESS;
}

void _ShutdownIO(void) {

}

// Checks whether a file has been modified or not.
bool plIsFileModified(time_t oldtime, const char *path) {
    if (!oldtime) {
        ReportError(PL_RESULT_FILEERR, "invalid time, skipping check");
        return false;
    }

    struct stat attributes;
    if (stat(path, &attributes) == -1) {
        ReportError(PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror(errno));
        return false;
    }

    if (attributes.st_mtime > oldtime) {
        return true;
    }

    return false;
}

/**
 * returns the modified time of the given file.
 *
 * @param path
 * @return modification time in seconds. returns 0 upon fail.
 */
time_t plGetFileModifiedTime(const char *path) {
    struct stat attributes;
    if (stat(path, &attributes) == -1) {
        ReportError(PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror(errno));
        return 0;
    }
    return attributes.st_mtime;
}

// Creates a folder at the given path.
bool plCreateDirectory(const char *path) {
    if(plPathExists(path)) {
        return true;
    }

#ifdef _WIN32
    if(_mkdir(path) == 0) {
#else
    if(mkdir(path, 0777) == 0) {
#endif
        return true;
    }

    ReportError(PL_RESULT_FILEERR, "%s", strerror(errno));

    return false;
}

bool plCreatePath(const char *path) {
    size_t length = strlen(path);
    char dir_path[length + 1];
    memset(dir_path, 0, sizeof(dir_path));
    for(size_t i = 0; i < length; ++i) {
        dir_path[i] = path[i];
        if(i != 0 &&
            (path[i] == '\\' || path[i] == '/') &&
            (path[i - 1] != '\\' && path[i - 1] != '/')) {
            if(!plCreateDirectory(dir_path)) {
                return false;
            }
        }
    }

    return plCreateDirectory(dir_path);
}

// Returns the extension for the file.
const char *plGetFileExtension(const char *in) {
    if (plIsEmptyString(in)) {
        return "";
    }

    const char *s = strrchr(in, '.');
    if(!s || s == in) {
        return "";
    }

    return s + 1;
}

// Strips the extension from the filename.
void plStripExtension(char *dest, const char *in) {
    if (plIsEmptyString(in)) {
        *dest = 0;
        return;
    }

    const char *s = strrchr(in, '.');
    while (in < s) *dest++ = *in++;
    *dest = 0;
}

/**
 * returns pointer to the last component in the given filename.
 *
 * @param path
 * @return
 */
const char *plGetFileName(const char *path) {
    const char *lslash = strrchr(path, '/');
    if (lslash != NULL) {
        path = lslash + 1;
    }
    return path;
}

/**
 * returns the name of the systems current user.
 *
 * @param out
 */
void plGetUserName(char *out) {
#ifdef _WIN32

    char userstring[PL_SYSTEM_MAX_USERNAME];
    ULONG size = PL_SYSTEM_MAX_USERNAME;
    if (GetUserNameEx(NameDisplay, userstring, &size) == 0) {
        snprintf(userstring, sizeof(userstring), "user");
    }

#else   // Linux

    char *userstring = getenv("LOGNAME");
    if (userstring == NULL) {
        // If it fails, just set it to user.
        userstring = "user";
    }

#endif

    int i = 0, userlength = (int) strlen(userstring);
    while (i < userlength) {
        if (userstring[i] == ' ') {
            out[i] = '_';
        } else {
            out[i] = (char) tolower(userstring[i]);
        } i++;
    }
}

/**
 * scans the given directory.
 * on each file that's found, it calls the given function to handle the file.
 *
 * @param path path to directory.
 * @param extension the extension to scan for (exclude '.').
 * @param Function callback function to deal with the file.
 * @param recursive if true, also scans the contents of each sub-directory.
 */
void plScanDirectory(const char *path, const char *extension, void (*Function)(const char *), bool recursive) {
    DIR *directory = opendir(path);
    if (directory) {
        struct dirent *entry;
        while ((entry = readdir(directory))) {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char filestring[PL_SYSTEM_MAX_PATH + 1];
            snprintf(filestring, sizeof(filestring), "%s/%s", path, entry->d_name);

            struct stat st;
            if(stat(filestring, &st) == 0) {
                if(S_ISREG(st.st_mode)) {
                    if(pl_strcasecmp(plGetFileExtension(entry->d_name), extension) == 0) {
                        Function(filestring);
                    }
                } else if(S_ISDIR(st.st_mode) && recursive) {
                    plScanDirectory(filestring, extension, Function, recursive);
                }
            }
        }

        closedir(directory);
    } else {
        ReportError(PL_RESULT_FILEPATH, "opendir failed!");
    }
}

const char *plGetWorkingDirectory(void) {
    static char out[PL_SYSTEM_MAX_PATH] = { '\0' };
    if (getcwd(out, PL_SYSTEM_MAX_PATH) == NULL) {
        /* The MSDN documentation for getcwd() is gone, but it proooobably uses
         * errno and friends.
         */
        ReportError(PL_RESULT_SYSERR, "%s", strerror(errno));
        return NULL;
    }
    return out;
}

void plSetWorkingDirectory(const char *path) {
    if(chdir(path) != 0) {
        ReportError(PL_RESULT_SYSERR, "%s", strerror(errno));
        /* TODO: Return error condition */
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// FILE I/O

/* loads an entire file into the PLIOBuffer struct */
void plLoadFile(const char *path, PLIOBuffer *buffer) {
    if(plIsEmptyString(path)) {
        ReportError(PL_RESULT_FILEPATH, "invalid path");
        return;
    }

    memset(buffer, 0, sizeof(PLIOBuffer));

    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        ReportError(PL_RESULT_FILEREAD, strerror(errno));
        return;
    }

    strncpy(buffer->name, path, sizeof(buffer->name));

    buffer->size = plGetFileSize(path);
    buffer->data = calloc(buffer->size, sizeof(uint8_t));
    if(buffer->data != NULL) {
        if(fread(buffer->data, sizeof(uint8_t), buffer->size, fp) != buffer->size) {
            FSLog("failed to read complete file (%s)\n", path);
        }
    } else {
        ReportError(PL_RESULT_MEMORY_ALLOCATION, "failed to allocate %d bytes", buffer->size);
    }

    fclose(fp);
}

/**
 * checks whether or not the given file is accessible
 * or exists.
 *
 * @param path
 * @return false if the file wasn't accessible.
 */
bool plFileExists(const char *path) {
    struct stat buffer;
    return (bool) (stat(path, &buffer) == 0);
}

bool plPathExists(const char *path) {
    DIR *dir = opendir(path);
    if(dir) {
        closedir(dir);
        return true;
    }
    return false;
}

bool plDeleteFile(const char *path) {
    if(!plFileExists(path)) {
        return true;
    }

    int result = remove(path);
    if(result == 0) {
        return true;
    }

    ReportError(PL_RESULT_FILEREAD, strerror(errno));
    return false;
}

bool plCopyFile(const char *path, const char *dest) {
    size_t file_size = plGetFileSize(path);
    if(file_size == 0) {
        return false;
    }

    uint8_t *data = calloc(file_size, 1);
    if(data == NULL) {
        ReportError(PL_RESULT_MEMORY_ALLOCATION, "failed to allocate buffer for %s, with size %d", path, file_size);
        return false;
    }

    FILE *original = NULL;
    FILE *copy = NULL;

    // read in the original
    if((original = fopen(path, "rb")) == NULL) {
        ReportError(PL_RESULT_FILEREAD, "failed to open %s", path);
        goto BAIL;
    }
    if(fread(data, 1, file_size, original) != file_size) {
        ReportError(PL_RESULT_FILEREAD, "failed to read in %d bytes for %s", file_size, path);
        goto BAIL;
    }
    fclose(original); original = NULL;

    // write out the copy
    if((copy = fopen(dest, "wb")) == NULL) {
        ReportError(PL_RESULT_FILEWRITE, "failed to open %s for write", dest);
        goto BAIL;
    }
    if(fwrite(data, 1, file_size, copy) != file_size) {
        ReportError(PL_RESULT_FILEWRITE, "failed to write out %d bytes for %s", file_size, path);
        goto BAIL;
    }
    fclose(copy);

    return true;

    BAIL:

    free(data);

    if(original != NULL) {
        fclose(original);
    }

    if(copy != NULL) {
        fclose(copy);
    }

    return false;
}

size_t plGetFileSize(const char *path) {
    struct stat buf;
    if(stat(path, &buf) != 0) {
        ReportError(PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror(errno));
        return 0;
    }

    return (size_t)buf.st_size;
}

///////////////////////////////////////////

void plFileIOPush(PLIOBuffer *file) {}
void plFileIOPop(void) {}

int16_t plGetLittleShort(FILE *fin) {
    int b1 = fgetc(fin);
    int b2 = fgetc(fin);
    return (int16_t) (b1 + b2 * 256);
}

int32_t plGetLittleLong(FILE *fin) {
    int b1 = fgetc(fin);
    int b2 = fgetc(fin);
    int b3 = fgetc(fin);
    int b4 = fgetc(fin);
    return b1 + (b2 << 8) + (b3 << 16) + (b4 << 24);
}
