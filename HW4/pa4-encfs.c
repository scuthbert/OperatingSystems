/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse
  --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(),
  write(), etc calls. As such, the functions that rely on maintaining file
  handles are not implmented (fgetattr(), etc). Those seeking a more efficient
  and more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "aes-crypt.h"

// Thanks Ahmed Almutawa for a boiler plate define
#define XMP_INFO ((struct xmp_private *)fuse_get_context()->private_data)
#define ENC_XATTR "user.pa4-encfs.encrypted"
struct xmp_private { // The struct in fuse context private_data
  char *base_path;
  char *password;
};

#define USAGE                                                                  \
  "usage: ./pa4-encfs <passphrase> <mirror directory> <mount point>\n"

static void new_path(char *full_path, const char *part_path) {
  strcpy(full_path, XMP_INFO->base_path);
  strncat(full_path, part_path, PATH_MAX);
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = lstat(npath, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_access(const char *path, int mask) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = access(npath, mask);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = readlink(npath, buf, size - 1);
  if (res == -1)
    return -errno;

  buf[res] = '\0';
  return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
  DIR *dp;
  struct dirent *de;

  (void)offset;
  (void)fi;

  char npath[PATH_MAX];
  new_path(npath, path);

  dp = opendir(npath);
  if (dp == NULL)
    return -errno;

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    if (filler(buf, de->d_name, &st, 0))
      break;
  }

  closedir(dp);
  return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  /* On Linux this could just be 'mknod(path, mode, rdev)' but this
     is more portable */
  if (S_ISREG(mode)) {
    res = open(npath, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (res >= 0)
      res = close(res);
  } else if (S_ISFIFO(mode))
    res = mkfifo(npath, mode);
  else
    res = mknod(npath, mode, rdev);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_mkdir(const char *path, mode_t mode) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = mkdir(npath, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_unlink(const char *path) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = unlink(npath);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_rmdir(const char *path) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = rmdir(npath);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_symlink(const char *from, const char *to) {
  int res;

  res = symlink(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_rename(const char *from, const char *to) {
  int res;

  res = rename(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_link(const char *from, const char *to) {
  int res;

  res = link(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_chmod(const char *path, mode_t mode) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = chmod(npath, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = lchown(npath, uid, gid);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_truncate(const char *path, off_t size) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = truncate(npath, size);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
  int res;
  struct timeval tv[2];

  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = utimes(npath, tv);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = open(npath, fi->flags);
  if (res == -1)
    return -errno;

  close(res);
  return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
  (void)fi;

  char npath[PATH_MAX];
  new_path(npath, path);

  int action = -1;

  char enc[5] = "fals";
  // NEW STYLE - w/ FILEs
  if (getxattr(npath, ENC_XATTR, enc, 5) == -1) {
    fprintf(stderr, "Read: Getting xattr (%s) failed %s.\n", enc, npath);
    // No error, as reading an unencrypted file would lead us here.
  }

  fprintf(stderr, "Read: Got xattr value as %s on %s.\n", enc, npath);

  char true[5] = "true";
  if (!strcmp(enc, true)) { // Is the file encrypted?
    // If so, open a tmp file and our file on disk
    action = 0;
  }

  FILE *tfile = tmpfile(); //fopen(tmpath, "wb+"); // Thanks stdio
  FILE *ffile = fopen(npath, "rb");

  if (tfile == NULL) {
    fprintf(stderr, "Read: Opening temp file failed %s.\n", npath);
    return -errno;
  }
  if (ffile == NULL) {
    fprintf(stderr, "Read: Opening file failed %s.\n", npath);
    return -errno;
  }

  if (do_crypt(ffile, tfile, action, XMP_INFO->password) == 0) {
    fprintf(stderr, "Read: Decrypting failed %s.\n", npath);
    return -errno;
  }

  // Check size of file
  fseek(tfile, 0, SEEK_END);
  size_t tfileLen = ftell(tfile);
  fseek(tfile, 0, SEEK_SET);
  fprintf(stderr, "Read: File size %lu bytes %s.\n", tfileLen, npath);

  // Second, read into our buf at the correct offset
  if (fseek(tfile, offset, SEEK_SET)) {
    fprintf(stderr, "Read: Seeking %lu bytes failed %s.\n", offset, npath);
    return -errno;
  }

  size_t readSize = fread(buf, sizeof(char), size, tfile);

  fclose(tfile);
  fclose(ffile);

  return readSize;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {

  (void)fi;
  int action = -1;

  char npath[PATH_MAX];
  new_path(npath, path);

  char enc[5] = "fals";
  // NEW STYLE - w/ FILEs
  if (getxattr(npath, ENC_XATTR, enc, 5) == -1) {
    fprintf(stderr, "Write: Getting xattr (%s) failed %s.\n", enc, npath);
    // No error, as reading an unencrypted file would lead us here.
  }

  fprintf(stderr, "Write: Got xattr value as %s on %s.\n", enc, npath);

  char true[5] = "true";
  if (!strcmp(enc, true)) { // Is the file encrypted?
    // If so, open a tmp file and our file on disk
    action = 1;

    FILE *tfile = tmpfile(); //fopen(tmpath, "wb+"); // Thanks stdio
    FILE *ffile = fopen(npath, "wb+");

    if (tfile == NULL) {
      fprintf(stderr, "Write: Opening temp file failed %s.\n", npath);
      return -errno;
    }
    if (ffile == NULL) {
      fprintf(stderr, "Write: Opening file failed %s.\n", npath);
      return -errno;
    }

    // Check size of file
    fseek(ffile, 0, SEEK_END);
    size_t fileLen = ftell(ffile);
    fseek(ffile, 0, SEEK_SET);
    fprintf(stderr, "Write: File size %lu bytes %s.\n", fileLen, npath);

    // First, unencrypt file on disk into our tmpfile
    if (fileLen > 0 && action != -1) {
      if (do_crypt(ffile, tfile, 0, XMP_INFO->password) == 0) {
        fprintf(stderr, "Write: Decrypting failed %s.\n", npath);
        return 0;
      }
    }

    // Second, write our buf at the correct offset
    if (fseek(tfile, offset, SEEK_SET)) {
      fprintf(stderr, "Write: Seeking %lu bytes failed %s.\n", offset, npath);
      return -errno;
    }

    if (fwrite(buf, sizeof(char), size, tfile) != size) {
      fprintf(stderr, "Write: Writing %lu bytes failed %s.\n", size, npath);
      return -errno;
    }

    fseek(ffile, 0, SEEK_SET);
    fseek(tfile, 0, SEEK_SET);

    // Third, re-encrypt our file
    if(do_crypt(tfile, ffile, action, XMP_INFO->password) == 0) {
      fprintf(stderr, "Write: Encrypting failed %s.\n", npath);
      return -errno;
    }

    // Last, close tfile and clean up
    fclose(tfile);
    fclose(ffile);

    return size;
  } else {
    int fd;
    int res;

    fd = open(path, O_WRONLY);
    if (fd == -1)
      return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
      res = -errno;

    close(fd);
    return res;
  }


}

static int xmp_statfs(const char *path, struct statvfs *stbuf) {
  int res;

  char npath[PATH_MAX];
  new_path(npath, path);

  res = statvfs(npath, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {
  (void)fi;
  (void)mode;

  char npath[PATH_MAX];
  new_path(npath, path);

  int res;
  res = creat(npath, mode);
  if (res == -1)
    return -errno;

  close(res);

  if (setxattr(npath, ENC_XATTR, "true", 5, 0)) {
    fprintf(stderr, "Create: Setting xattr (%s) failed %s.\n", ENC_XATTR,
            npath);
    return -errno;
  }

  fprintf(stderr, "Create: Set xattr (%s) on %s.\n", ENC_XATTR, npath);

  return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi) {
  /* Just a stub.	 This method is optional and can safely be left
     unimplemented */

  (void)path;
  (void)fi;
  return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi) {
  /* Just a stub.	 This method is optional and can safely be left
     unimplemented */

  (void)path;
  (void)isdatasync;
  (void)fi;
  return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
                        size_t size, int flags) {

  char npath[PATH_MAX];
  new_path(npath, path);

  int res = lsetxattr(npath, name, value, size, flags);
  if (res == -1)
    return -errno;
  return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
                        size_t size) {

  char npath[PATH_MAX];
  new_path(npath, path);

  int res = lgetxattr(npath, name, value, size);
  if (res == -1)
    return -errno;
  return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size) {

  char npath[PATH_MAX];
  new_path(npath, path);

  int res = llistxattr(npath, list, size);
  if (res == -1)
    return -errno;
  return res;
}

static int xmp_removexattr(const char *path, const char *name) {

  char npath[PATH_MAX];
  new_path(npath, path);

  int res = lremovexattr(npath, name);
  if (res == -1)
    return -errno;
  return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .access = xmp_access,
    .readlink = xmp_readlink,
    .readdir = xmp_readdir,
    .mknod = xmp_mknod,
    .mkdir = xmp_mkdir,
    .symlink = xmp_symlink,
    .unlink = xmp_unlink,
    .rmdir = xmp_rmdir,
    .rename = xmp_rename,
    .link = xmp_link,
    .chmod = xmp_chmod,
    .chown = xmp_chown,
    .truncate = xmp_truncate,
    .utimens = xmp_utimens,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .statfs = xmp_statfs,
    .create = xmp_create,
    .release = xmp_release,
    .fsync = xmp_fsync,
#ifdef HAVE_SETXATTR
    .setxattr = xmp_setxattr,
    .getxattr = xmp_getxattr,
    .listxattr = xmp_listxattr,
    .removexattr = xmp_removexattr,
#endif
};

int main(int argc, char *argv[]) {
  if (argc < 4) { // Check to see if we are using correctly
    fprintf(stderr, USAGE);
    return (errno);
  }

  struct xmp_private *xmp_data;
  xmp_data = malloc(sizeof(struct xmp_private)); // Mem leak - oh well?

  xmp_data->base_path = realpath(argv[2], NULL);
  xmp_data->password = argv[1];

  umask(0);
  return fuse_main(argc - 2, &argv[2], &xmp_oper, xmp_data);
}
