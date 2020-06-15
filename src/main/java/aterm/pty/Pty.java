package aterm.pty;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Size;

import java.io.FileDescriptor;
import java.io.IOException;
import java.lang.reflect.Field;

public class Pty {
    static {
        System.loadLibrary("pty");
    }

    public static void setWindowSize(int masterFd, int rows, int cols) throws IOException {
        setWindowSize0(masterFd, rows, cols);
    }


    public static @Size(2)
    int[] getWindowSize(int masterFd) throws IOException {
        int[] winsize = new int[2];
        getWindowSize0(masterFd, winsize);
        return winsize;
    }

    public static void sendSignal(int pid, int signal) {
        sendSignal0(pid, signal);
    }

    public static int waitFor(int pid) {
        return waitFor0(pid);
    }

    public static void close(int fd) throws IOException {
        close0(fd);
    }

    public static int exec(@NonNull String cmd, @Nullable String[] argv, @Nullable String[] env,
                           int rows, int cols, @Size(1) int[] masterFd) throws IOException {
        return exec0(cmd, argv, env, rows, cols, masterFd);
    }

    public static FileDescriptor createFileDescriptor(int fd) {
        FileDescriptor fileDescriptor = new FileDescriptor();
        try {
            Field descriptorField = fileDescriptor.getClass().getDeclaredField("descriptor");
            descriptorField.setAccessible(true);
            descriptorField.set(fileDescriptor, fd);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return fileDescriptor;
    }


    @Keep
    private static native void setWindowSize0(int masterFd, int rows, int cols) throws IOException;

    @Keep
    private static native void getWindowSize0(int masterFd, @Size(2) int[] winsize) throws IOException;

    @Keep
    private static native void sendSignal0(int pid, int signal);

    @Keep
    private static native int waitFor0(int pid);

    @Keep
    private static native int exec0(@NonNull String cmd, String[] argv, String[] env,
                                    int rows, int cols, @Size(1) int[] fd) throws IOException;

    @Keep
    private static native void close0(int fd) throws IOException;

    //在某项机型上会报错
    @Keep
    private static native FileDescriptor createFileDescriptor0(int fd);

}
