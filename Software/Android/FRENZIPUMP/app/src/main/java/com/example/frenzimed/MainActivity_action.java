package com.example.frenzimed;

import android.annotation.SuppressLint;
import android.app.TimePickerDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.NumberPicker;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TimePicker;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
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
    String readMessage = null;
    boolean newMessage = false;

    // Global Variables
    // These are the global variables
    int motorStartTimeHr = 12;
    int motorStartTimeMin = 0;
    int motorRunTimeHr = 12;
    int motorRunTimeMin = 0;
    int motorRunInterval = 10;
    int motorVoltage = 110;
    int motorCurrent = 10;
    int soilMoisturePercent = 10;
    int soilTemperature = 30;
    int airTemperature = 30;

    // These are the GUI variables
    EditText etStartTime;
    EditText etInterval;
    EditText etRunTimeHr;
    EditText  etRunTimeMin;
    EditText etCurrent;

    EditText etSoilMoisture;
    EditText etSoilTemperature;
    EditText etAirTemperature;
    RadioButton rd110v, rd220v, rd440v;
    RadioGroup voltRadioGroup;
    Button buttonSubmit, buttonReset, buttonManualStart, buttonManualStop, exitButton;

    TimePickerDialog timePickerDialog;
    String amPm;
    String name;

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
        // EditText fields
        etStartTime = findViewById(R.id.etStartTime);
        etRunTimeHr = findViewById(R.id.etRunTimeHr);
        //etStartTimeMin = (EditText) findViewById(R.id.etStartTimeMin);
        etRunTimeMin = findViewById(R.id.etRunTimeMin);
        etInterval = findViewById(R.id.etInterval);
        etCurrent = (EditText) findViewById(R.id.etCurrent);

        /*

        etSoilMoisture = (EditText) findViewById(R.id.etSoilMoisture);
        etSoilTemperature = (EditText) findViewById(R.id.etSoilTemperature);
        etAirTemperature = (EditText) findViewById(R.id.etAirTemperature);

        // RadioButtons
        voltRadioGroup = (RadioGroup) findViewById(R.id.voltRadioGroup);

        rd110v = (RadioButton) findViewById(R.id.rd110v);
        rd220v = (RadioButton) findViewById(R.id.rd220v);
        rd440v = (RadioButton) findViewById(R.id.rd440v);
*/
        //  Button
        buttonSubmit = findViewById(R.id.Submit);
        buttonReset = findViewById(R.id.cancel_button);
        exitButton = findViewById(R.id.exit_button);
        buttonManualStart = findViewById(R.id.ManualStart);
        buttonManualStop = findViewById(R.id.ManualStop);


        // get a handle on the bluetooth radio
        mBTAdapter = BluetoothAdapter.getDefaultAdapter();
        BluetoothManager btManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
//========================= PERMISSIONS ==============================

        mHandler = new Handler() {
            public void handleMessage(Message msg) {
                if (msg.what == MESSAGE_READ) {
                    readMessage = new String((byte[]) msg.obj, StandardCharsets.UTF_8);
                    newMessage = true;
                    Toast.makeText(getApplicationContext(), "Device Read: " + readMessage, Toast.LENGTH_SHORT).show();
                }
                if (msg.what == CONNECTING_STATUS) {
                    if (msg.arg1 == 1)
                        Toast.makeText(getApplicationContext(), "Connected to Device: " + msg.obj, Toast.LENGTH_SHORT).show();
                    else
                        Toast.makeText(getApplicationContext(), "Connection Failed", Toast.LENGTH_SHORT).show();
                }
            }
        };


        //=====================
        // Initialize
        initializeParam();
        //=====================

//========================= FRENZIMED FIRST PAGE ==============================
// 1. BLUETOOTH_CONNECT
// 2. DURATION
// 3. STRENGTH
// 4. TRANSIT DATA ON START bluetoothConnect();
// 5. SHOW TIMER
// 6. ERROR CHECKING ...
// 6. EXIT
        // START Button
        buttonManualStart.setOnClickListener(v -> {
            startPump();
        });
        // STOP Button
        buttonManualStop.setOnClickListener(v -> {
            stopPump();
        });
        // Submit Config
        buttonSubmit.setOnClickListener(v -> {
            submitConfig();
        });
        // EXIT Button
        exitButton.setOnClickListener(v -> {
            try {
                if (mBTSocket != null) {
                    mBTSocket.close();
                    FRENZI_MED_CONNECT = false;
                }
            } catch (IOException e) {
            }
            finish();
        });

        //=====================
        // on below line we are adding
        // click listener for our edit text.
        etStartTime.setInputType(InputType.TYPE_NULL);
        etStartTime.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                etStartTime.requestFocus();
                timePickerDialog = new TimePickerDialog(MainActivity_action.this, new TimePickerDialog.OnTimeSetListener() {
                    @Override
                    public void onTimeSet(TimePicker timePicker, int hourOfDay, int minutes) {
                        if (hourOfDay >= 12) {
                            amPm = "PM";
                        } else {
                            amPm = "AM";
                        }
                        motorStartTimeHr = hourOfDay;
                        motorStartTimeMin = minutes;
                        etStartTime.setText(String.format("%02d:%02d ", hourOfDay, minutes) + amPm);
                    }
                }, 0, 0, false);
                timePickerDialog.show();
            }
        });

        //====================
    } // end oncreate


//--------Functions

    void initializeParam() {
        etStartTime.setText(String.format("%02d:%02d ", motorStartTimeHr, motorStartTimeMin));
        etRunTimeHr.setText(String.format("%02d", motorRunTimeHr));
        etRunTimeMin.setText(String.format("%02d", motorRunTimeMin));
        etInterval.setText(String.format("%02d", motorRunInterval));
        etCurrent.setText(String.format("%02d", motorCurrent));
//        etSoilMoisture.setText(String.format("%02d", soilMoisturePercent));
//        etSoilTemperature.setText(String.format("%02d", soilTemperature));
//        etAirTemperature.setText(String.format("%02d", airTemperature));
//        rd110v.setChecked(true);
         bluetoothConnect();
    }

    private void startPump() {
        mConnectedThread.write("StartPump"); // ON
        Toast.makeText(getBaseContext(), "Motor starting.", Toast.LENGTH_SHORT).show();
    }

    private void stopPump() {
        mConnectedThread.write("StopPump"); // OFF
        Toast.makeText(getBaseContext(), "Motor stop.", Toast.LENGTH_SHORT).show();
    }

    private void submitConfig() {


        mConnectedThread.write("ConfigStart"); // ConfigPump
        Toast.makeText(getBaseContext(), "Configure Start", Toast.LENGTH_SHORT).show();

        motorRunTimeHr = Integer.parseInt(etRunTimeHr.getText().toString());
        motorRunTimeMin = Integer.parseInt(etRunTimeMin.getText().toString());
        motorCurrent = Integer.parseInt(etCurrent.getText().toString());
        motorRunInterval = Integer.parseInt(etInterval.getText().toString());
        Toast.makeText(getBaseContext(), "Start time: " +  motorStartTimeHr + " Duration: " + motorRunTimeHr + " Cur I: " + motorCurrent, Toast.LENGTH_SHORT).show();
        mConnectedThread.write(String.format("%02d%02d%02d%02d%02d%02dX", motorStartTimeHr, motorStartTimeMin, motorRunTimeHr, motorRunTimeMin, motorCurrent, motorRunInterval));//((motorStartTimeHr) & 0xff));
        //Toast.makeText(getBaseContext(), "Configure End", Toast.LENGTH_SHORT).show();
        //mConnectedThread.write("ConfigEnd"); // ConfigPump
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
                    Toast.makeText(getApplicationContext(), "Paired FRENZIMED device: " + devices.getName(), Toast.LENGTH_SHORT).show();
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
                mConnectedThread.write("PhoneConnect");
                Toast.makeText(getApplicationContext(), "Phone Connected", Toast.LENGTH_SHORT).show();
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
                mmOutStream.flush();
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