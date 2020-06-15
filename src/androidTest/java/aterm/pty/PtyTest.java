package aterm.pty;

import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;

import static org.junit.Assert.assertEquals;

@RunWith(AndroidJUnit4.class)
public class PtyTest {

    @Test
    public void testExec() throws IOException {
        final int[] fd = new int[1];
        final int pid = Pty.exec("/system/bin/sh", null, null, 25, 30, fd);
        int[] windowSize = Pty.getWindowSize(fd[0]);
        assertEquals(windowSize[0], 25);
        assertEquals(windowSize[1], 30);

        Pty.setWindowSize(fd[0], 300, 500);
        int[] windowSize1 = Pty.getWindowSize(fd[0]);
        assertEquals(windowSize1[0], 300);
        assertEquals(windowSize1[1], 500);

        System.out.println("file desc " + fd[0] + "  " + windowSize[0] + "  " + windowSize[1]);
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(3000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
//                Pty.sendSignal(pid,9);
                try {
                    Pty.close(fd[0]);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }).start();
        Pty.waitFor(pid);
    }
}