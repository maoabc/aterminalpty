
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <android/log.h>
#include <jni.h>
#include "utils/log.h"
#include "forkpty.h"

#define LOG_TAG "forkpty"

#ifdef __cplusplus
extern "C" {
#endif

//static jmethodID fd_ctor;

static void ThrowByName(JNIEnv *env, const char *name, const char *msg) {
    jclass cls = (*env)->FindClass(env, name);
    if (cls != NULL) {
        (*env)->ThrowNew(env, cls, msg);
        (*env)->DeleteLocalRef(env, cls);
    }
}

static void ThrowIOException(JNIEnv *env, const char *msg) {
    ThrowByName(env, "java/io/IOException", msg);
}

static void ThrowOutOfMemoryError(JNIEnv *env, const char *msg) {
    ThrowByName(env, "java/lang/OutOfMemoryError", msg);
}

static int
exec_in_pty(u_short rows, u_short cols, int *masterFd,
            const char *cmd, char *const *argv, char *const *envp) {
    struct termios termios;
    memset(&termios, 0, sizeof(termios));

    termios.c_iflag = ICRNL | IUTF8;
    termios.c_oflag = OPOST | ONLCR | NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
    termios.c_cflag = CS8 | CREAD;
    termios.c_lflag = ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK;

    cfsetispeed(&termios, B38400);
    cfsetospeed(&termios, B38400);

    termios.c_cc[VINTR] = 0x1f & 'C';
    termios.c_cc[VQUIT] = 0x1f & '\\';
    termios.c_cc[VERASE] = 0x7f;
    termios.c_cc[VKILL] = 0x1f & 'U';
    termios.c_cc[VEOF] = 0x1f & 'D';
    termios.c_cc[VSTART] = 0x1f & 'Q';
    termios.c_cc[VSTOP] = 0x1f & 'S';
    termios.c_cc[VSUSP] = 0x1f & 'Z';
    termios.c_cc[VREPRINT] = 0x1f & 'R';
    termios.c_cc[VWERASE] = 0x1f & 'W';
    termios.c_cc[VLNEXT] = 0x1f & 'V';
    termios.c_cc[VMIN] = 1;
    termios.c_cc[VTIME] = 0;

    struct winsize size = {rows, cols, 0, 0};


    int pid = forkpty(masterFd, NULL, &termios, &size);
    if (pid == 0) {//child process
        /* Restore the ISIG signals back to defaults */
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGSTOP, SIG_DFL);
        signal(SIGCONT, SIG_DFL);

//        clearenv();
        if (envp) {
            for (; *envp; ++envp) putenv(*envp);
        }

        execv(cmd, argv);
        _exit(1);
    }

    return pid;
}


static void aterm_pty_Pty_sendSignal(JNIEnv *env, jclass clazz,
                                     jint pid, jint signal) {
    kill(pid, signal);
}

static jint aterm_pty_Pty_waitFor(JNIEnv *env, jclass clazz, jint pid) {
    int status;
    int result = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        result = WEXITSTATUS(status);
    }
    return result;
}

static void aterm_pty_Pty_close(JNIEnv *env, jclass clazz, jint fd) {
    if (close(fd)) {
        ThrowIOException(env, "Failed to close fd");
    }
}

static void aterm_pty_Pty_setWindowSize(JNIEnv *env, jclass clazz,
                                        jint masterFd, jint row, jint col) {
    struct winsize sz = {
            .ws_row = (u_short) row,
            .ws_col = (u_short) col,
            .ws_xpixel = 0,
            .ws_ypixel = 0
    };

    if (ioctl(masterFd, TIOCSWINSZ, &sz)) {
        ThrowIOException(env,
                         "Failed to TIOCSWINSZ ioctl");
    }
}

static void
aterm_pty_Pty_getWindowSize(JNIEnv *env, jclass clazz,
                            jint masterFd, jintArray winsize) {

    struct winsize sz;
    jsize len = (*env)->GetArrayLength(env, winsize);
    if (len != 2) {
        ThrowIOException(env, "length != 2");
    }

    if (ioctl(masterFd, TIOCGWINSZ, &sz)) {
        ThrowIOException(env,
                         "Failed to TIOCGWINSZ ioctl");
    }
    jint *ss = (*env)->GetIntArrayElements(env, winsize, NULL);
    ss[0] = sz.ws_row;
    ss[1] = sz.ws_col;
    (*env)->ReleaseIntArrayElements(env, winsize, ss, 0);
}


static char **jstr_arr2cstr_arr(JNIEnv *env, jobjectArray jstr_arr) {
    char **cstr_arr = NULL;

    if (jstr_arr == NULL) {
        return cstr_arr;
    }
    jsize size = (*env)->GetArrayLength(env, jstr_arr);
    if (size > 0) {
        cstr_arr = (char **) malloc((size + 1) * sizeof(char *));
        if (!cstr_arr) {
            ThrowOutOfMemoryError(env, "");
            return NULL;
        }
        for (int i = 0; i < size; ++i) {
            jstring jstr = (*env)->GetObjectArrayElement(env, jstr_arr, i);


            jsize length = (*env)->GetStringLength(env, jstr);

            jsize utfLength = (*env)->GetStringUTFLength(env, jstr);

            char *jstr_utf = malloc((size_t) (utfLength + 1));
            if (!jstr_utf) {
                ThrowOutOfMemoryError(env, "");
                return NULL;
            }
            (*env)->GetStringUTFRegion(env, jstr, 0, length, jstr_utf);
            jstr_utf[utfLength] = 0;

            cstr_arr[i] = jstr_utf;
        }
        cstr_arr[size] = NULL;
    }
    return cstr_arr;
}

static jint
aterm_pty_Pty_exec(JNIEnv *env, jclass clazz,
                   jstring cmd, jobjectArray args,
                   jobjectArray envVars,
                   jint rows, jint cols, jintArray outFd) {
    int masterFd;

    const char *cmd_utf;
    if (cmd == NULL || !(cmd_utf = (*env)->GetStringUTFChars(env, cmd, 0))) {
        ThrowIOException(env, "cmd == null");
        return -1;
    }

    char **argv = jstr_arr2cstr_arr(env, args);

    char **envp = jstr_arr2cstr_arr(env, envVars);


    int pid = exec_in_pty((u_short) rows, (u_short) cols, &masterFd, cmd_utf, argv,
                          envp);

    (*env)->ReleaseStringUTFChars(env, cmd, cmd_utf);

    if (argv) {
        for (char **tmp = argv; *tmp; ++tmp) free(*tmp);
        free(argv);
    }
    if (envp) {
        for (char **tmp = envp; *tmp; ++tmp) free(*tmp);
        free(envp);
    }

    (*env)->SetIntArrayRegion(env, outFd, 0, 1, &masterFd);

    return pid;
}

static jobject aterm_pty_Pty_createFileDescriptor(JNIEnv *env, jclass clazz,
                                                  jint fd) {

    return NULL;

}


#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

static jboolean
registerNativeMethods(JNIEnv *env, const char *cls_name, JNINativeMethod *methods, jint size) {
    jclass clazz = (*env)->FindClass(env, cls_name);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if ((*env)->RegisterNatives(env, clazz, methods, size) < 0) {
        (*env)->DeleteLocalRef(env, clazz);
        return JNI_FALSE;
    }
    (*env)->DeleteLocalRef(env, clazz);

    return JNI_TRUE;
}

static JNINativeMethod gMethods[] = {
        {"setWindowSize0",        "(III)V",                      (void *) aterm_pty_Pty_setWindowSize},

        {"getWindowSize0",        "(I[I)V",                      (void *) aterm_pty_Pty_getWindowSize},

        {"sendSignal0",           "(II)V",                       (void *) aterm_pty_Pty_sendSignal},

        {"waitFor0",              "(I)I",                        (void *) aterm_pty_Pty_waitFor},

        {"exec0",                 "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;II[I)I",
                                                                 (void *) aterm_pty_Pty_exec},
        {"close0",                "(I)V",                        (void *) aterm_pty_Pty_close},

        {"createFileDescriptor0", "(I)Ljava/io/FileDescriptor;", (void *) aterm_pty_Pty_createFileDescriptor},

};

int register_aterm_pty_Pty(JNIEnv *env) {
//    jclass fileDescClass = (*env)->FindClass(env, "java/io/FileDescriptor");
//    fd_ctor = (*env)->GetMethodID(env, fileDescClass, "<init>", "(I)V");
//    (*env)->DeleteLocalRef(env, fileDescClass);
    return registerNativeMethods(env, "aterm/pty/Pty", gMethods, NELEM(gMethods));
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed");
        return -1;
    }

    register_aterm_pty_Pty(env);

    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif