package dev.justinf.temposteps;

import com.fazecast.jSerialComm.SerialPort;

public class SerialTerminal {

    private final TempoStepsApp app;
    private String port;
    private SerialPort currentPort;
    private PortListener currentListener;
    private boolean connecting;
    private boolean connected;
    private long lastHandshakeTimestamp;

    public SerialTerminal(TempoStepsApp app) {
        this.app = app;
    }

    public boolean establishConnection() {
        SerialPort[] ports = SerialPort.getCommPorts();
        for (int i = 0; i < ports.length; i++) {
            app.log(ports[i].getSystemPortName() + ": " + ports[i].getDescriptivePortName());
        }

        connecting = true;
        currentPort = SerialPort.getCommPort(port);
        currentPort.setComPortParameters(9600, 8, 1, 0);
        currentPort.setComPortTimeouts(SerialPort.TIMEOUT_WRITE_BLOCKING, 0, 0);
        if (!currentPort.openPort()) {
            app.log("Could not open port!");
            connecting = false;
            connected = false;
            return false;
        }

        lastHandshakeTimestamp = System.currentTimeMillis();
        app.log("Opened port!");

        connecting = false;
        connected = true;
        currentListener = new PortListener(app, currentPort);
        currentListener.listen();
        return true;
    }

    public String getPort() {
        return port;
    }

    public boolean isConnecting() {
        return connecting;
    }

    public boolean isConnected() {
        return connected;
    }

    public long getLastHandshakeTimestamp() {
        return lastHandshakeTimestamp;
    }

    public void setPort(String port) {
        this.port = port;
    }
}