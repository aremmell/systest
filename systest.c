#include "systest.h"
#include "macros.h"

#if defined(__WIN__)
# pragma comment(lib, "shlwapi.lib")
# pragma comment(lib, "ws2_32.lib")
# pragma comment(lib, "version.lib")
#endif

int num_attempted = 0;
int num_succeeded = 0;

void handle_result(bool pass, const char* desc) {
    /* print something reliable, so that if we need to use tools
     * to parse the output of this program to determine support,
     * it will be much easier. */
    if (pass) {
        printf("\t" LGREEN("PASS: %s") "\n", desc);
        num_succeeded++;
    } else {
        fprintf(stderr, "\t" RED("FAIL: %s") "\n", desc);
    }

    num_attempted++;
}

#if !defined(__WIN__)
bool call_sysconf(int val, const char* desc) {
    printf("checking sysconf(%d) (\"%s\")...\n", val, desc);

    long ret = sysconf(val);
    if (ret == -1) {
        printf("sysconf(%d) error: %s!\n", val, strerror(errno));
        return false;
    } else {
        printf("sysconf(%d) = %ld.\n", val, ret);
        return true;
    }

    return ret;
}
#endif

bool check_sysconf(void) {
#if !defined(__WIN__)
    bool retval = true;
    retval &= call_sysconf(_SC_2_VERSION, "popen() and pclose()");
    retval &= call_sysconf(_SC_HOST_NAME_MAX, "_SC_HOST_NAME_MAX");
    return retval;
#else
    /* https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen */
    printf("windows has _popen()/_pclose().\n");
    return true;
#endif
}

bool check_system(void) {
    printf("checking system(NULL)...\n");

    if (0 == system(NULL)) {
        printf("system() is NOT available to execute commands!\n");
        return false;
    } else {
        printf("system() is available to execute commands.\n");
        return true;
    }
}

bool check_z_printf(void) {
    char buf[256] = {0};
    size_t n = 10;
    snprintf(buf, 256, "printing a size_t with the value ten: '%zu'", n);
    printf("%s\n", buf);

    if (NULL != strstr(buf, "10")) {
        printf("Found '10' in the format string.\n");
        return true;
    }

    return false;
}

bool check_filesystem_api(void) {
    bool all_passed = true;

    /* ==== get app file name path (absolute path of binary file) ==== */
    char* appfilename = systest_getappfilename();
    all_passed &= _validptr(appfilename);
    if (_validptr(appfilename)) {
        printf("systest_getappfilename() = '%s'\n", appfilename);
        /* ==== */

        /* ==== get base name (file name component of a path)  ==== */
        char* basenametest = strdup(appfilename);
        if (!basenametest) {
            handle_error(errno, "strdup() failed!");
            return false;
        }

        char* basenameresult = systest_getbasename(basenametest);
        all_passed &= (strlen(basenameresult) > 0 && 0 != strcmp(basenameresult, "."));
        printf("systest_getbasename() = '%s'\n", basenameresult);
        systest_safefree(basenametest);
        /* ==== */

        /* ==== get dir name (directory structure component of a path)  ==== */
        char* dirnametest = strdup(appfilename);
        if (!dirnametest) {
            handle_error(errno, "strdup() failed!");
            return false;
        }

        char* dirnameresult = systest_getdirname(dirnametest);
        all_passed &= (strlen(dirnameresult) > 0 && 0 != strcmp(dirnameresult, "."));
        printf("systest_getdirname() = '%s'\n", dirnameresult);
        systest_safefree(dirnametest);
        /* ==== */

        systest_safefree(appfilename);
    } else {
        printf(RED("systest_getbasename() = skipped") "\n");
        printf(RED("systest_getdirname() = skipped") "\n");
    }

    /* ==== get app dir path (absolute path of directory containing binary file) ==== */
    char* appdir = systest_getappdir();
    all_passed &= _validptr(appdir);
    printf("systest_getappdir() = '%s'\n", prn_str(appdir));
    systest_safefree(appdir);
    /* ==== */

    /* ==== get binary file name with no path components ==== */
    char* appbname = systest_getappbasename();
    all_passed &= _validptr(appbname);
    printf("systest_getappbasename() = '%s'\n", prn_str(appbname));
    systest_safefree(appbname);
    /* ==== */

    /* ==== get current working directory (not necessarily the same ass app directory) ==== */
    char* cwd = systest_getcwd();
    all_passed &= _validptr(cwd);
    printf("systest_getcwd() = '%s'\n", prn_str(cwd));
    systest_safefree(cwd);
    /* ==== */

    /* file existence: some we know exist, and some we know don't. */
    static const struct { const char* const path; bool exists; } real_or_not[] = {
        {"../../LICENSE", true},
        {"./i_exist", true},
        {"a_s_d_f_foobar.baz", false},
        {"idontexist", false},
    };

    for (size_t n = 0; n < (sizeof(real_or_not) / sizeof(real_or_not[0])); n++) {
        bool exists = false;
        bool ret    = systest_pathexists(real_or_not[n].path, &exists,
            SYSTEST_PATH_REL_TO_APP);
        all_passed &= ret;
        if (!ret)
            continue;

        if (exists != real_or_not[n].exists) {
            all_passed = false;
            printf(RED("systest_pathexists('%s') = %s") "\n",
                real_or_not[n].path, exists ? "true" : "false");
        } else {
            printf("systest_pathexists('%s') = %s\n",
                real_or_not[n].path, exists ? "true" : "false");
        }
    }

    return all_passed;
}

#define INET_TEST_HOST "example.com"
#define INET_TEST_PORT "http"

bool systest_haveinetconn(void) {
    struct addrinfo hints = {
        .ai_flags = 0,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };

    struct addrinfo* result = NULL;
    int get = getaddrinfo(INET_TEST_HOST, INET_TEST_PORT, (const struct addrinfo*)&hints, &result);
    if (0 != get) {
        printf("getaddrinfo failed: %d (%s)!\n", get, gai_strerror(get));
        return false;
    }

    printf("getaddrinfo succeeded; creating a compatible socket...\n");

    bool conn_result = false;
    struct addrinfo* cur = result;
    do {
        printf("trying socket(%d, %d, %d)...\n", AF_INET, cur->ai_socktype, cur->ai_protocol);
        descriptor sock = socket(AF_INET, cur->ai_socktype, cur->ai_protocol);
        if (sock == BAD_SOCKET) {
            printf("socket failed: %d (%s)", errno, strerror(errno));
        } else {
            printf("got socket %d; trying connect...\n", (int)sock);
            const struct timeval timeout = {5, 0};
            int set = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval));
            if (set == -1) {
                printf("setsockopt(SO_SNDTIMEO) failed: %d (%s)\n", errno, strerror(errno));
            }
            set = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
            if (set == -1) {
                printf("setsockopt(SO_RCVTIMEO) failed: %d (%s)\n", errno, strerror(errno));
            }
            int conn = connect(sock, (const struct sockaddr*)cur->ai_addr, cur->ai_addrlen);
            if (conn == 0) {
                printf("connected successfully!\n");
                conn_result = true;
                break;
            } else {
                printf("connect failed: %d (%s); trying next address...\n", errno, strerror(errno));
            }
#if !defined(__WIN__)
            close(sock);
#else
            closesocket(sock);
#endif
        }
        cur = cur->ai_next;
    } while (cur != NULL);

    freeaddrinfo(result);
    result = NULL;

    return conn_result;
}

bool check_get_hostname(void) {
    char hname[SYSTEST_MAXHOST];
    if (!systest_gethostname(hname))
        return false;

    if (!_validstr(hname)) {
        printf(RED("gethostname returned an empty string!\n"));
        return false;
    }

    printf("hostname = '%s'\n", hname);
    return true;
}

bool check_get_uname(void) {
    struct utsname name;
    if (!systest_getuname(&name)) {
        printf(RED("Couldn't get uname data!\n"));
        return false;
    }

    printf("uname = '%s', '%s', '%s', '%s', '%s'\n", name.sysname, name.nodename,
        name.release, name.version, name.machine);
    return true;
}

void check_build_env(void) {
#if !defined(__WIN__)
# if defined(__STDC_LIB_EXT1__)
   printf("__STDC_LIB_EXT1__ is defined\n");
# else
   printf("__STDC_LIB_EXT1__ NOT defined\n");
# endif
# if defined(__GLIBC__)
    printf("Using GNU libc version: %d.%d\n", __GLIBC__, __GLIBC_MINOR__);
# else
    printf("Not using GNU libc\n");
# endif
#else // __WIN__
# if defined(__STDC_SECURE_LIB__)
   printf("__STDC_SECURE_LIB__ is defined\n");
# else
   printf("__STDC_SECURE_LIB__ NOT defined\n");
# endif
#endif
#if defined(__clang__)
    printf("Using Clang %s\n", __clang_version__);
#elif defined(__GNUC__)
    printf("Using GCC %d.%d\n", __GNUC__, __GNUC_MINOR__);
#elif defined(_MSC_VER)
    printf("Using MSVC %lld\n", (long long)_MSC_FULL_VER);
#else
    printf("Using unknown toolset\n");
#endif
}

void check_safefree(void) {
    int *ptr = malloc(sizeof(int));
    if (!ptr) {
        handle_error(errno, "malloc() failed!");
        return;
    }

    *ptr = 1234;
    systest_safefree(ptr);

    if (!ptr)
        printf("safe_free() does reset the pointer\n");
    else
        printf(RED("safe_free() does NOT reset the pointer!\n"));
}

int main(void) {

    printf("\t" BLUEB("~~~~~~~~~~ <systest> ~~~~~~~~~~") "\n");

    //
    // begin environment tests
    //

    check_build_env();

    //
    // begin curiosity tests
    //

    check_safefree();

    //
    // begin feature tests
    //

    // sysconf checks
    bool ret = check_sysconf();
    handle_result(ret, "sysconf()");

    // system()
    ret = check_system();
    handle_result(ret, "system()");

    // can you use a 'z' prefix in a printf-style specifier for size_t portably?
    ret = check_z_printf();
    handle_result(ret, "z prefix in *printf");

    //
    // begin portability tests
    //

    ret = check_filesystem_api();
    handle_result(ret, "filesystem api");

    ret = check_get_hostname();
    handle_result(ret, "get hostname");

    ret = check_get_uname();
    handle_result(ret, "get uname");

    ret = systest_haveinetconn();
    handle_result(ret, "test internet connection");

    if (num_succeeded != num_attempted)
        printf("\t" REDB("--- %d/%d tests passed ---\n"), num_succeeded, num_attempted);
    else
        printf("\t" LGREENB("--- all %d tests passed! ---\n"), num_attempted);

    printf("\t" BLUEB("~~~~~~~~~~ </systest> ~~~~~~~~~~") "\n");

    return num_succeeded > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}


//
// portability test implementations
//

bool systest_pathgetstat(const char* restrict path, struct stat* restrict st, systest_rel_to rel_to) {
    if (!_validstr(path) || !_validptr(st))
        return false;

    memset(st, 0, sizeof(struct stat));

    int stat_ret  = -1;
    bool relative = false;
    if (!systest_ispathrelative(path, &relative))
        return false;

    if (relative) {
        char* base_path = NULL;
        switch(rel_to) {
            case SYSTEST_PATH_REL_TO_APP: base_path = systest_getappdir(); break;
            case SYSTEST_PATH_REL_TO_CWD: base_path = systest_getcwd(); break;
            default: self_log("invalid enum!"); return false;
        }

        if (!base_path) {
            handle_error(errno, "couldn't get base path!");
            return false;
        }

#if !defined(__WIN__)
# if defined(__MACOS__)
        int open_flags = O_SEARCH;
# elif defined(__linux__)
        int open_flags = O_PATH | O_DIRECTORY;
# elif defined(__BSD__)
        int open_flags = O_EXEC | O_DIRECTORY;
# endif

        int fd = open(base_path, open_flags);
        if (-1 == fd) {
            handle_error(errno, "open() failed!");
            systest_safefree(base_path);
            return false;
        }

        stat_ret = fstatat(fd, path, st, AT_SYMLINK_NOFOLLOW);
        systest_safeclose(&fd);
        systest_safefree(base_path);
    } else {
        stat_ret = stat(path, st);
    }
#else // __WIN__
        char abs_path[SYSTEST_MAXPATH] = { 0 };
        snprintf(abs_path, SYSTEST_MAXPATH, "%s\\%s", base_path, path);

        stat_ret = stat(abs_path, st);
        systest_safefree(base_path);
    } else {
        stat_ret = stat(path, st);
    }
#endif
    if (-1 == stat_ret) {
        if (ENOENT == errno) {
            st->st_size = SYSTEST_STAT_NONEXISTENT;
            return true;
        } else {
            handle_error(errno, "stat() failed!");
            return false;
        }
    }

    char* as_str = systest_stattostring(st);
    if (as_str) {
        printf("%s = %s\n", path, as_str);
        systest_safefree(as_str);
    }

    return true;
}

bool systest_pathexists(const char* restrict path, bool* restrict exists, systest_rel_to rel_to) {
    if (!_validstr(path) || !_validptr(exists))
        return false;

    *exists = false;

    struct stat st = {0};
    bool stat_ret  = systest_pathgetstat(path, &st, rel_to);
    if (!stat_ret)
        return false;

    *exists = (st.st_size != SYSTEST_STAT_NONEXISTENT);
    return true;
}

char* systest_getcwd(void) {
#if !defined(__WIN__)
# if defined(__linux__) && defined(_GNU_SOURCE)
    char* cur = get_current_dir_name();
    if (NULL == cur)
        handle_error(errno, "");
    return cur;
# else
    char* cur = getcwd(NULL, 0);
    if (NULL == cur)
        handle_error(errno, "");
    return cur;
# endif
#else // __WIN__
    char* cur = _getcwd(NULL, 0);
    if (NULL == cur)
        handle_error(errno, "");
    return cur;
#endif
}

char* systest_getappfilename(void) {

    char* buffer = (char*)calloc(SYSTEST_MAXPATH, sizeof(char));
    if (NULL == buffer) {
        handle_error(errno, "");
        return NULL;
    }

    bool resolved = false;
    size_t size   = SYSTEST_MAXPATH;
    bool grow     = false;

    do {
        if (grow) {
            systest_safefree(buffer);

            buffer = (char*)calloc(size, sizeof(char));
            if (NULL == buffer) {
                handle_error(errno, "");
                resolved = false;
                break;
            }

            grow = false;
        }

#if !defined(__WIN__)
# if defined(__linux__)
#  if defined(__HAVE_UNISTD_READLINK__)
        ssize_t read = readlink("/proc/self/exe", buffer, size);
        self_log("readlink() returned: %ld (size = %zu)", read, size);
        if (-1 != read && read < (ssize_t)size) {
            resolved = true;
            break;
        } else {
            if (-1 == read) {
                handle_error(errno, "");
                resolved = false;
                break;
            } else if (read >= (ssize_t)size) {
                /* readlink, like Windows' impl, doesn't have a concept
                * of letting you know how much larger your buffer needs
                * to be; it just truncates the string and returns how
                * many chars it wrote there. */
                size += SYSTEST_PATH_BUFFER_GROW_BY;
                grow = true;
                continue;
            }
        }
#  else
#   error "unable to resolve readlink(); see man readlink and its feature test macro requirements."
#  endif
# elif defined(__FreeBSD__)
        int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
        int ret = sysctl(mib, 4, buffer, &size, NULL, 0);
        self_log("sysctl() returned: %d (size = %zu)", ret, size);
        if (0 == ret) {
            resolved = true;
            break;
        } else {
            handle_error(errno, "");
            resolved = false;
            break;
        }
# elif defined(__APPLE__)
        int ret = _NSGetExecutablePath(buffer, (uint32_t*)&size);
        if (0 == ret) {
            resolved = true;
            break;
        } else if (-1 == ret) {
            grow = true;
            continue;
        } else {
            handle_error(errno, "");
            resolved = false;
            break;
        }
# else
#  error "no implementation for your platform; please contact the author."
# endif
#else // __WIN__
        DWORD ret = GetModuleFileNameA(NULL, buffer, (DWORD)size);
        if (0 != ret && ret < (DWORD)size) {
            resolved = true;
            break;
        } else {
            if (0 == ret) {
                handle_error(GetLastError(), "GetModuleFileNameA() failed!");
                resolved = false;
                break;
            } else if (ret == (DWORD)size || ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
                /* Windows has no concept of letting you know how much larger
                * your buffer needed to be; it just truncates the string and
                * returns size. So, we'll guess. */
                size += SYSTEST_PATH_BUFFER_GROW_BY;
                grow = true;
                continue;
            }
        }
#endif

    } while (true);

    if (!resolved) {
        systest_safefree(buffer);
        self_log("failed to resolve filename!");
    }

    return buffer;
}

char* systest_getappbasename(void) {
    char* filename = systest_getappfilename();
    if (!_validstr(filename))
        return NULL;

    char* retval = systest_getbasename(filename);
    char* bname  = strdup(retval);

    systest_safefree(filename);
    return bname;
}

char* systest_getappdir(void) {
    char* filename = systest_getappfilename();
    if (!_validstr(filename))
        return NULL;

    char* retval = systest_getdirname(filename);
    char* dname = strdup(retval);

    systest_safefree(filename);
    return dname;
}

char* systest_getbasename(char* restrict path) {
    if (!_validstr(path))
        return ".";

#if !defined(__WIN__)
    return basename(path);
#else
    return PathFindFileNameA(path);
#endif
}

char* systest_getdirname(char* restrict path) {
    if (!_validstr(path))
        return ".";

#if !defined(__WIN__)
    return dirname(path);
#else
    if (!PathRemoveFileSpecA((LPSTR)path))
        handle_error(GetLastError(), "PathRemoveFileSpecA() failed!");
    return path;
#endif
}

bool systest_ispathrelative(const char* restrict path, bool* restrict relative) {
    if (!_validstr(path) || !_validptr(relative))
        return false;

#if !defined(__WIN__)
    if (path[0] == '/' || (path[0] == '~' && path[1] == '/'))
        *relative = false;
    else
        *relative = true;
    return true;
#else
    *relative = (TRUE == PathIsRelativeA(path));
    return true;
#endif
}

char* systest_stattostring(struct stat* restrict st) {
    if (!_validptr(st))
        return NULL;

    char* buffer = (char*)calloc(SYSTEST_STAT_BUFFER_SIZE, sizeof(char));
    if (!buffer) {
        handle_error(errno, "calloc() failed!");
        return NULL;
    }

#if defined(__WIN__)
# define S_IFBLK 0x0001
# define S_IFLNK 0x0002
# define S_IFSOCK 0x0003
# define S_IFIFO _S_IFIFO
# define S_IRUSR 0x0100
# define S_IWUSR 0x0080
# define S_IXUSR 0x0040
# define S_IRGRP 0x0010
# define S_IWGRP 0x0400
# define S_IXGRP 0x0004
# define S_IROTH 0x0020
# define S_IWOTH 0x0800
# define S_IXOTH 0x0008
#endif

    char* type = "";
    switch (st->st_mode & S_IFMT) {
        case S_IFBLK:  type = "block device"; break;
        case S_IFCHR:  type = "character special"; break;
        case S_IFDIR:  type = "directory"; break;
        case S_IFIFO:  type = "pipe (fifo)"; break;
        case S_IFLNK:  type = "symlink"; break;
        case S_IFREG:  type = "regular"; break;
        case S_IFSOCK: type = "socket"; break;
        default:       type = "<unknown>"; break;
    }

    char mode[32] = {0};
    snprintf(mode, sizeof(mode), "%c%c%c%c%c%c%c%c%c%c (%03o)",
        (systest_bittest(st->st_mode & S_IFMT, S_IFDIR) ? 'd' :
        (systest_bittest(st->st_mode & S_IFMT, S_IFSOCK) ? 's' : '-')),
        (systest_bittest(st->st_mode & 0x0FFF, S_IRUSR) ? 'r' : '-'),
        (systest_bittest(st->st_mode & 0x0FFF, S_IWUSR) ? 'w' : '-'),
        (systest_bittest(st->st_mode & 0x0FFF, S_IXUSR) ? 'x' : '-'),
        (systest_bittest(st->st_mode & 0x0FFF, S_IRGRP) ? 'r' : '-'),
        (systest_bittest(st->st_mode & 0x0FFF, S_IWGRP) ? 'w' : '-'),
        (systest_bittest(st->st_mode & 0x0FFF, S_IXGRP) ? 'x' : '-'),
        (systest_bittest(st->st_mode & 0x0FFF, S_IROTH) ? 'r' : '-'),
        (systest_bittest(st->st_mode & 0x0FFF, S_IWOTH) ? 'w' : '-'),
        (systest_bittest(st->st_mode & 0x0FFF, S_IXOTH) ? 'x' : '-'),
        ((st->st_mode & 0x0FFF) & ((S_IRUSR | S_IWUSR | S_IXUSR) |
            (S_IRGRP | S_IWGRP | S_IXGRP) |
            (S_IROTH | S_IWOTH | S_IXOTH))));

    snprintf(buffer, SYSTEST_STAT_BUFFER_SIZE, "{ type: %s, size: %ld, "
        "mode: %s }", type, (long)st->st_size, mode);

    return buffer;
}

bool systest_add_slash(char* restrict path) {
    if (!_validstr(path))
        return false;

    size_t path_len = strnlen(path, SYSTEST_MAXPATH);

    if (path_len > SYSTEST_MAXPATH - 2)
        return false;

    if (path[path_len - 1] != '/' && path[path_len - 1] != '\\') {
        path[path_len] = SYSTEST_PATH_SEP;
        path[path_len + 1] = '\0';
    }

    return true;
}

bool systest_gethostname(char hname[SYSTEST_MAXHOST]) {
    hname[0] = '\0';
#if !defined(__WIN__)
    if (-1 == gethostname(hname, SYSTEST_MAXHOST - 1)) {
        handle_error(errno, "gethostname() failed!");
        return false;
    }
#else
    WSADATA wsad = {0};
    int ret = WSAStartup(MAKEWORD(2, 2), &wsad);
    if (0 != ret) {
        handle_error(ret, "WSAStartup() failed!");
        return false;
    }

    if (SOCKET_ERROR == gethostname(hname, SYSTEST_MAXHOST)) {
        handle_error(WSAGetLastError(), "gethostname() failed!");
        WSACleanup();
        return false;
    }

    WSACleanup();
#endif // !__WIN__
    return true;
}

bool systest_getuname(struct utsname* name) {
    if (!_validptr(name))
        return false;

    memset(name, 0, sizeof(struct utsname));

#if !defined(__WIN__)
    if (-1 == uname(name)) {
        handle_error(errno, "uname() failed!");
        return false;
    }
#else
    static const char* sys_name = "Windows";
    static const char* kernel_dll = "kernel32.dll";

    strncpy(name->sysname, sys_name, strnlen(sys_name, _SYS_NAMELEN));

    char tmp[SYSTEST_MAXHOST];
    if (!systest_gethostname(tmp))
        return false;

    strncpy(name->nodename, tmp, strnlen(tmp, _SYS_NAMELEN));

    char path[SYSTEST_MAXPATH];
    if (0 == GetSystemDirectoryA(path, SYSTEST_MAXPATH)) {
        handle_error(GetLastError(), "GetSystemDirectoryA failed!");
        return false;
    }

    if (!PathAppendA(path, kernel_dll)) {
        handle_error(GetLastError(), "PathAppendA failed!");
        return false;
    }

    self_log("kernel dll is at: %s; extracting version...", path);

    DWORD vsize = GetFileVersionInfoSizeA(path, NULL);
    if (0 == vsize) {
        handle_error(GetLastError(), "GetFileVersionInfoSizeA failed!");
        return false;
    }

    VS_FIXEDFILEINFO* fvi = calloc(1, vsize);
    if (!fvi) {
        handle_error(errno, "calloc failed!");
        return false;
    }

    if (!GetFileVersionInfoA(path, 0, vsize, fvi)) {
        handle_error(GetLastError(), "GetFileVersionInfoA failed!");
        systest_safefree(fvi);
        return false;
    }

    UINT blk_size = 0;
    VS_FIXEDFILEINFO* blk = NULL;
    if (!VerQueryValueA(fvi, "\\", &blk, &blk_size) || blk_size < sizeof(VS_FIXEDFILEINFO)) {
        handle_error(GetLastError(), "VerQueryValueA failed!");
        systest_safefree(fvi);
        return false;
    }

    const WORD verMajor = HIWORD(blk->dwProductVersionMS);
    const WORD verMinor = LOWORD(blk->dwProductVersionMS);
    const WORD verBuild = HIWORD(blk->dwProductVersionLS);

    systest_safefree(fvi);

    int prn = sprintf(name->release, "%hu.%hu.%hu", verMajor, verMinor, verBuild);
    assert(prn > 0);

    SYSTEM_INFO si = {0};
    GetNativeSystemInfo(&si);

    const char* mach_hw_str = "";
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL: mach_hw_str = "x86"; break;
        case PROCESSOR_ARCHITECTURE_IA64: mach_hw_str = "IA64"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: mach_hw_str = "aarch64"; break;
        case PROCESSOR_ARCHITECTURE_ARM: mach_hw_str = "armhf"; break;
        case PROCESSOR_ARCHITECTURE_AMD64: mach_hw_str = "x86_64"; break;
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
        default:
            mach_hw_str = "<unknown>";
            break;
    }

    prn = snprintf(name->version, _SYS_NAMELEN, "%s %hu.%hu.%hu-%s", sys_name, verMajor,
        verMinor, verBuild, mach_hw_str);
    assert(prn > 0);

    strncpy(name->machine, mach_hw_str, strnlen(mach_hw_str, _SYS_NAMELEN));

#endif // !__WIN__
    return true;
}


//
// utility functions
//


void _handle_error(int err, const char* msg, char* file, int line, const char* func) {
    fprintf(stderr, RED("ERROR: %s (%s:%d): %s (%d, %s)") "\n",
        func, file, line, msg, err, strerror(err));
}

void _self_log(const char* msg, char* file, int line, const char* func) {
    fprintf(stderr, WHITE("%s (%s:%d): %s") "\n", func, file, line, msg);
}
