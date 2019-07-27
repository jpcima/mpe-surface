package org.sdf1.jpcima;

import android.content.*;
import android.content.pm.*;
import android.media.midi.*;
import android.net.*;
import android.os.*;
import android.util.*;
import java.io.*;
import java.util.*;

class MidiDeviceInterface {
    private static final String TAG = "MidiDeviceInterface";

    private Context context = null;
    private MidiInputPort inPort = null;

    public MidiDeviceInterface(Context context)
    {
        this.context = context;
    }

    public void closePort() throws IOException
    {
        if (inPort == null)
            return;

        Log.i(TAG, "Close port: " + inPort);

        inPort.close();
        inPort = null;
    }

    public void openInputPort()
    {
        openInputPort((Uri)null);
    }

    public void openInputPort(String uriString)
    {
        openInputPort((uriString != null) ? Uri.parse(uriString) : null);
    }

    public void openInputPort(Uri uri)
    {
        try { closePort(); } catch (IOException ex) {}

        Log.i(TAG, "Open port");

        EndpointInfo ep = findEndpointUri(uri, MidiDeviceInfo.PortInfo.TYPE_INPUT);
        if (ep == null) {
            Log.e(TAG, "MIDI input endpoint not found");
            return;
        }

        openInputEndpoint(ep);
    }

    public String[] listInputPorts()
    {
        ArrayList<String> list = new ArrayList<String>();

        EndpointInfo eps[] = findAllEndpoints(null, null, MidiDeviceInfo.PortInfo.TYPE_INPUT);

        for (int i = 0, n = eps.length; i < n; ++i) {
            EndpointInfo ep = eps[i];
            list.add(ep.toUri().toString());
        }

        return list.toArray(new String[0]);
    }

    public void sendMidiMessage(byte[] message)
    {
        Log.d(TAG, "Send MIDI to port: " + inPort);

        if (inPort == null)
            return;

        try {
            inPort.send(message, 0, message.length);
        }
        catch (IOException ex) {
            Log.e(TAG, "Cannot send MIDI message", ex);
        }
    }

    private MidiManager getMidiManager()
    {
        PackageManager pm = context.getPackageManager();
        if (!pm.hasSystemFeature(PackageManager.FEATURE_MIDI))
            return null;
        return (MidiManager)context.getSystemService(Context.MIDI_SERVICE);
    }

    private class EndpointInfo {
        MidiDeviceInfo device;
        MidiDeviceInfo.PortInfo port;

        public static final String URI_SCHEME = "android";

        public Uri toUri()
        {
            Uri.Builder bld = new Uri.Builder();
            bld.scheme(URI_SCHEME);
            bld.appendPath(device.getProperties().getCharSequence(MidiDeviceInfo.PROPERTY_NAME).toString());
            bld.appendPath(port.getName());
            return bld.build();
        }
    }

    private EndpointInfo[] findAllEndpoints(String devName, String portName, int type)
    {
        ArrayList<EndpointInfo> eps = new ArrayList<EndpointInfo>();

        MidiManager mgr = getMidiManager();
        if (mgr == null)
            return eps.toArray(new EndpointInfo[0]);

        MidiDeviceInfo[] infos = mgr.getDevices();

        for (int i = 0, n = infos.length; i < n; ++i) {
            MidiDeviceInfo dev = infos[i];
            MidiDeviceInfo.PortInfo[] ports = dev.getPorts();

            if (devName != null && !devName.equals(dev.getProperties().getCharSequence(MidiDeviceInfo.PROPERTY_NAME)))
                continue;

            for (int j = 0, m = ports.length; j < m; ++j) {
                MidiDeviceInfo.PortInfo port = ports[j];

                if (port.getType() != type)
                    continue;

                if (portName == null || portName.equals(port.getName())) {
                    EndpointInfo ep = new EndpointInfo();
                    ep.device = dev;
                    ep.port = port;
                    eps.add(ep);
                }
            }
        }

        return eps.toArray(new EndpointInfo[0]);
    }

    private EndpointInfo findEndpoint(String devName, String portName, int type)
    {
        EndpointInfo eps[] = findAllEndpoints(devName, portName, type);
        return (eps.length > 0) ? eps[0] : null;
    }

    private EndpointInfo findEndpointUri(String uriString, int type)
    {
        return findEndpointUri((uriString != null) ? Uri.parse(uriString) : null, type);
    }

    private EndpointInfo findEndpointUri(Uri uri, int type)
    {
        if (uri == null)
            return findEndpoint(null, null, type);

        if (!uri.getScheme().equals(EndpointInfo.URI_SCHEME))
            return null;

        List<String> segments = uri.getPathSegments();
        int numSegments = segments.size();

        String devName = "";
        String portName = "";
        if (numSegments > 0)
            devName = segments.get(0);
        if (numSegments > 1)
            portName = segments.get(1);

        return findEndpoint(devName, portName, type);
    }

    private void openInputEndpoint(final EndpointInfo ep)
    {
        MidiManager mgr = getMidiManager();
        if (mgr == null)
            return;

        mgr.openDevice(ep.device, new MidiManager.OnDeviceOpenedListener() {
                @Override
                public void onDeviceOpened(MidiDevice device) {
                    if (device == null) {
                        Log.e(TAG, "Could not open device: " + ep.device);
                    } else {
                        Log.i(TAG, "Opened device: " + ep.device);
                        MidiInputPort port = device.openInputPort(ep.port.getPortNumber());
                        if (port == null) {
                            Log.e(TAG, "Could not open port: " + ep.port);
                            return;
                        }
                        Log.i(TAG, "Opened MIDI port: " + port);
                        inPort = port;
                    }
                }
            },
            new Handler(Looper.getMainLooper()));
    }
}
