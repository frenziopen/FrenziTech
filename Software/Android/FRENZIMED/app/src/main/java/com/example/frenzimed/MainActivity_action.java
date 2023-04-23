package com.example.frenzimed;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Set;
import java.util.UUID;

public class MainActivity_action extends AppCompatActivity {

    private static final UUID BTMODULEUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // "random" unique identifier
    // #defines for identifying shared types between calling functions
    private final static int REQUEST_ENABLE_BT = 1; // used to identify adding bluetooth names
    private final static int MESSAGE_READ = 2; // used in bluetooth handler to identify message update
    private final static int CONNECTING_STATUS = 3; // used in bluetooth handler to identify message status
    private static boolean FRENZI_MED_DEVICE_FOUND = false;
    private static boolean FRENZI_MED_SOCKET = false;
    private static boolean FRENZI_MED_CONNECT = false;

    // Global Variables
    SeekBar sDuration;
    TextView sDurationView;
    SeekBar sStrength;
    TextView sStrengthView;
    TextView timerView;
    Button stopButton;
    Button startButton;
    Button exitButton;
    int THERAPY_DURATION = 10;
    int TIME_MULTIPLIER = 10;
    int THERAPY_STRENGTH = 25;
    int STRENGTH_MULTIPLIER = 25;
    boolean timerStarted = false;

    private Button mScanBtn;
    private BluetoothAdapter mBTAdapter;
    private Set<BluetoothDevice> mPairedDevices;
    private ArrayAdapter<String> mBTArrayAdapter;
    final BroadcastReceiver blReceiver = new BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                // add the name to the list
                mBTArrayAdapter.add(device.getName() + "\n" + device.getAddress());
                mBTArrayAdapter.notifyDataSetChanged();
            }
        }
    };
    private Handler mHandler; // Our main handler that will receive callback notifications
    private ConnectedThread mConnectedThread; // bluetooth background worker thread to send and receive data
    private BluetoothSocket mBTSocket = null; // bi-directional client-to-client data path
    private BluetoothDevice mBTDevice;
    private CountDownTimer mCountDownTimer;

    @SuppressLint("HandlerLeak")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.mainactivity_action);

        sDuration = findViewById(R.id.sDuration);
        sDurationView = findViewById(R.id.tvDuration);
        sStrength = findViewById(R.id.sStrength);
        sStrengthView = findViewById(R.id.tvStrength);
        timerView = findViewById(R.id.tvTimer);
        startButton = findViewById(R.id.bStart);
        stopButton = findViewById(R.id.bStop);
        exitButton = findViewById(R.id.bExit);
        // get a handle on the bluetooth radio
        mBTAdapter = BluetoothAdapter.getDefaultAdapter();
        BluetoothManager btManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
//========================= PERMISSIONS ==============================

        mHandler = new Handler() {
            public void handleMessage(Message msg) {
                if (msg.what == MESSAGE_READ) {
                    String readMessage = null;
                    readMessage = new String((byte[]) msg.obj, StandardCharsets.UTF_8);
                    //  Toast.makeText(getApplicationContext(), "Device: " + readMessage, Toast.LENGTH_SHORT).show();
                }
                if (msg.what == CONNECTING_STATUS) {
                    if (msg.arg1 == 1)
                        Toast.makeText(getApplicationContext(), "Connected to Device: " + msg.obj, Toast.LENGTH_SHORT).show();
                    else
                        Toast.makeText(getApplicationContext(), "Connection Failed", Toast.LENGTH_SHORT).show();
                }
            }
        };

//========================= FRENZIMED FIRST PAGE ==============================
// 1. BLUETOOTH_CONNECT
// 2. DURATION
// 3. STRENGTH
// 4. TRANSIT DATA ON START bluetoothConnect();
// 5. SHOW TIMER
// 6. ERROR CHECKING ...
// 6. EXIT
        // THERAPY_DURATION
        sDuration.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                THERAPY_DURATION = TIME_MULTIPLIER * progress;
                sDurationView.setText("Minutes: " + THERAPY_DURATION);
                timerView.setText(THERAPY_DURATION + ":00");
                Toast.makeText(getApplicationContext(), "Duration changed:" + THERAPY_DURATION, Toast.LENGTH_SHORT).show();
                if (timerStarted) {
                    stopTimer();
                    Toast.makeText(getApplicationContext(), "Therapy Stopped Start again ", Toast.LENGTH_LONG).show();
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                //Toast.makeText(getApplicationContext(),"SeekBar Touch Started",Toast.LENGTH_SHORT).show();
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                //Toast.makeText(getApplicationContext(), "SeekBar Touch Stop ", Toast.LENGTH_SHORT).show();
            }
        });

        // STRENGTH DATA
        sStrength.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                THERAPY_STRENGTH = progress * STRENGTH_MULTIPLIER;
                sStrengthView.setText("Strength: " + THERAPY_STRENGTH);
                Toast.makeText(getApplicationContext(), "Strength changed:" + THERAPY_STRENGTH, Toast.LENGTH_SHORT).show();
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                //Toast.makeText(getApplicationContext(),"SeekBar Touch Started",Toast.LENGTH_SHORT).show();
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                //Toast.makeText(getApplicationContext(), "SeekBar Touch Stop ", Toast.LENGTH_SHORT).show();
            }
        });

        // START Button
        startButton.setOnClickListener(v -> {
            //turnOnLed();
            bluetoothConnect();
            startTimer();
        });
        // STOP Button
        stopButton.setOnClickListener(v -> {
            stopTimer();
        });
        // EXIT Button
        exitButton.setOnClickListener(v -> {
            try {
                if (mBTSocket != null) {
                    mBTSocket.close();
                }
            } catch (IOException e) {
            }
            finish();
        });
    }

    private void startTimer() {
        if (!timerStarted) {
            mCountDownTimer = new MyCountDownTimer((THERAPY_DURATION) * 1000 * 60, 1000);
            if (mConnectedThread != null) //First check to make sure thread created
            {
                if (THERAPY_STRENGTH == 25) mConnectedThread.write("2");
                if (THERAPY_STRENGTH == 50) mConnectedThread.write("3");
                if (THERAPY_STRENGTH == 75) mConnectedThread.write("4");
                mConnectedThread.write(String.valueOf(THERAPY_DURATION));
                mConnectedThread.write("1"); // ON
                mCountDownTimer.start();
                timerStarted = true;
                Toast.makeText(getBaseContext(), "Therapy started, stay cool.", Toast.LENGTH_SHORT).show();
            }
        }
    }

    private void stopTimer() {
        turnOffLED();
        timerStarted = false;
        mCountDownTimer.cancel();
    }

    private void turnOffLED() {
        if (mConnectedThread != null) //First check to make sure thread created
        {
            mConnectedThread.write("0"); //OFF
        }
    }

    //========================= FRENZIMED FIRST PAGE ==============================
// 1. BLUETOOTH_CONNECT
    @SuppressLint("MissingPermission")
    private void bluetoothConnect() {
        if (FRENZI_MED_CONNECT) {
            Toast.makeText(getApplicationContext(), "Bluetooth is already connected", Toast.LENGTH_SHORT).show();
            return;
        }
        if (mBTAdapter.isEnabled()) {
            mPairedDevices = mBTAdapter.getBondedDevices();
            FRENZI_MED_DEVICE_FOUND = false;
            // put it's one to the adapter
            for (BluetoothDevice devices : mPairedDevices) {
                if (devices.getName().toUpperCase().contains("FRENZI")) {
                    mBTDevice = devices;
                    FRENZI_MED_DEVICE_FOUND = true;
                    Toast.makeText(getApplicationContext(), "Paired FRENZIMED device: " + devices.getName(), Toast.LENGTH_LONG).show();
                }
            }
            // Required device found connect now
            //
            if (FRENZI_MED_DEVICE_FOUND) {
                try {
                    mBTSocket = createBluetoothSocket(mBTDevice);
                    FRENZI_MED_SOCKET = true;
                } catch (IOException e) {
                    FRENZI_MED_SOCKET = false;
                    Toast.makeText(getBaseContext(), "Socket creation failed", Toast.LENGTH_SHORT).show();
                }
                // Establish the Bluetooth socket connection.
                if (FRENZI_MED_SOCKET) {
                    try {
                        mBTSocket.connect();
                        Toast.makeText(getBaseContext(), "Socket connection", Toast.LENGTH_SHORT).show();
                        FRENZI_MED_CONNECT = true;
                    } catch (IOException e) {
                        try {
                            FRENZI_MED_CONNECT = false;
                            mBTSocket.close();
                            mHandler.obtainMessage(CONNECTING_STATUS, -1, -1).sendToTarget();
                        } catch (IOException e2) {
                            FRENZI_MED_CONNECT = false;
                            //insert code to deal with this
                            Toast.makeText(getBaseContext(), "Socket creation failed", Toast.LENGTH_SHORT).show();
                        }
                    }
                }

                if (FRENZI_MED_CONNECT) {
                    mConnectedThread = new ConnectedThread(mBTSocket);
                    mConnectedThread.start();
                    mHandler.obtainMessage(CONNECTING_STATUS, 1, -1, "FRENZI").sendToTarget();
                }
            } else if (!FRENZI_MED_DEVICE_FOUND) {
/*                final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("FRENZIMED device not found");
                builder.setMessage("Please make sure FRENZIMED device is connected and LED light on.");
                builder.setPositiveButton(android.R.string.ok, null);
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        Toast.makeText(getApplicationContext(), "Bluetooth not on", Toast.LENGTH_SHORT).show();
                    }
                });
                builder.show();*/
                Toast.makeText(getApplicationContext(), "Bluetooth not on", Toast.LENGTH_SHORT).show();
                FRENZI_MED_CONNECT = false;
            }

            if (mConnectedThread != null) //First check to make sure thread created
            {
                mConnectedThread.write("0");
                Toast.makeText(getApplicationContext(), "LED OFF", Toast.LENGTH_LONG).show();
            }
        } else {
            // if (!mBTAdapter.isEnabled()) {
            FRENZI_MED_CONNECT = false;
            Toast.makeText(getBaseContext(), "Bluetooth adapter is not on", Toast.LENGTH_SHORT).show();
            return;
        }
    }

    @SuppressLint("MissingPermission")
    private BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
        return device.createRfcommSocketToServiceRecord(BTMODULEUUID);
        //creates secure outgoing connection with BT device using UUID
    }

    public class MyCountDownTimer extends CountDownTimer {
        public MyCountDownTimer(long millisInFuture, long countDownInterval) {
            super(millisInFuture, countDownInterval);
        }

        @Override
        public void onTick(long millisUntilFinished) {
            int progress = (int) (millisUntilFinished / 1000);
            NumberFormat f = new DecimalFormat("00");
            long min = (millisUntilFinished / 60000) % 60;
            long sec = (millisUntilFinished / 1000) % 60;
            timerView.setText(f.format(min) + ":" + f.format(sec));
        }

        @Override
        public void onFinish() {
            ToneGenerator toneG = new ToneGenerator(AudioManager.STREAM_ALARM, 100);
            toneG.startTone(ToneGenerator.TONE_CDMA_CALL_SIGNAL_ISDN_PING_RING, 500);
            toneG.startTone(ToneGenerator.TONE_CDMA_ALERT_CALL_GUARD, 1500);
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            toneG.startTone(ToneGenerator.TONE_CDMA_ALERT_CALL_GUARD, 1500);
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            //toneG.startTone(ToneGenerator.TONE_CDMA_CALL_SIGNAL_ISDN_PING_RING, 500);
            toneG.startTone(ToneGenerator.TONE_CDMA_ALERT_CALL_GUARD, 1500);
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            mCountDownTimer.cancel();
            stopTimer();
        }
    }


    private class ConnectedThread extends Thread {
        private final BluetoothSocket mmSocket;
        private final InputStream mmInStream;
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket) {
            mmSocket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            // Get the input and output streams, using temp objects because
            // member streams are final
            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) {
            }
            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run() {
            byte[] buffer = new byte[1024];  // buffer store for the stream
            int bytes; // bytes returned from read()
            // Keep listening to the InputStream until an exception occurs
            while (true) {
                try {
                    // Read from the InputStream
                    bytes = mmInStream.read(buffer);
                    if (bytes != 0) {
                        SystemClock.sleep(100);
                        mmInStream.read(buffer);
                    }
                    // Send the obtained bytes to the UI activity
                    mHandler.obtainMessage(MESSAGE_READ, bytes, -1, buffer).sendToTarget();
                } catch (IOException e) {
                    break;
                }
            }
        }

        /* Call this from the main activity to send data to the remote device */
        public void write(String input) {
            byte[] bytes = input.getBytes();           //converts entered String into bytes
            try {
                mmOutStream.write(bytes);
            } catch (IOException e) {
            }
        }

        /* Call this from the main activity to shutdown the connection */
        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) {
            }
        }
    }


}