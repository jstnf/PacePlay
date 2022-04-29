package dev.justinf.temposteps;

import com.fazecast.jSerialComm.SerialPort;

import java.util.concurrent.atomic.AtomicBoolean;

public class PortListener {

    private final TempoStepsApp app;
    private final SerialPort port;

    private AtomicBoolean running;

    // Store current string since communications can come with multiple buffers
    private boolean readMode;
    private String currentString;

    public PortListener(TempoStepsApp app, SerialPort port) {
        this.app = app;
        this.port = port;
        running = new AtomicBoolean(true);
        readMode = false;
    }

    public void listen() {
        try {
            while (running.get()) {
                while (port.bytesAvailable() == 0) {
                    Thread.sleep(20);
                }

                byte[] readBuffer = new byte[port.bytesAvailable()];
                int numRead = port.readBytes(readBuffer, readBuffer.length);
                // System.out.println("Read " + numRead + " bytes.");
                String s = new String(readBuffer);
                app.queueData(s);
            }
        } catch (Exception e) { e.printStackTrace(); }
    }

    public void close() {
        running.set(false);
    }
}