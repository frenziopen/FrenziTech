package com.example.mg1;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.TimePickerDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.InputType;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TimePicker;
import android.widget.Toast;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.UUID;
import java.util.WeakHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import javax.security.auth.callback.Callback;

public class MainActivity extends AppCompatActivity {

    // UUID for the ble serial port client characteristic which is necessary for notifications.
    public final static UUID CLIENT_UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");//00001801-0000-1000-8000-00805f9b34fb");//6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    public final static String ACTION_GATT_CONNECTED    =   "com.example.bluetooth.le.ACTION_GATT_CONNECTED";
    public final static String ACTION_GATT_DISCONNECTED =   "com.example.bluetooth.le.ACTION_GATT_DISCONNECTED";
    public final static String ACTION_GATT_SERVICES_DISCOVERED = "com.example.bluetooth.le.ACTION_GATT_SERVICES_DISCOVERED";
    public final static String ACTION_DATA_AVAILABLE =          "com.example.bluetooth.le.ACTION_DATA_AVAILABLE";
    public final static String EXTRA_DATA =                     "com.example.bluetooth.le.EXTRA_DATA";
    private final static int REQUEST_ENABLE_BT = 1;
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;
    private static final int PERMISSION_REQUEST_BLUETOOTH_SCAN = 2;
    private static final int PERMISSION_REQUEST_FINE_LOCATION = 3;
    private static final int PERMISSION_REQUEST_BLUETOOTH_CONNECT = 4;
    private final UUID UUID_AAF_READ = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    private final UUID UUID_AAF_WRITE = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    private final UUID UUID_AAF_CLIENT = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    public Map<String, String> uuids = new HashMap<String, String>();
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
    EditText etStartTime, etRunTimeHr, etRunTimeMin, etInterval, etCurrent, etSoilMoisture, etSoilTemperature, etAirTemperature;
    RadioButton rd110v, rd220v, rd440v;
    RadioGroup voltRadioGroup;
    Button buttonSubmit, buttonReset, buttonManualStart;
    // These are BLe
    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner bluetoothLeScanner;
    BluetoothGattCharacteristic TxChar;
    BluetoothGattCharacteristic RxChar;
    Boolean btScanning = false;
    int deviceIndex = 0;
    ArrayList<BluetoothDevice> devicesDiscovered = new ArrayList<BluetoothDevice>();
    EditText deviceIndexInput;
    Button connectToDevice;
    Button disconnectDevice;
    BluetoothGatt bluetoothGatt;
    TimePickerDialog timePickerDialog;
    String amPm;
    private boolean writeInProgress = false; // Flag to indicate a write is currently in progress
    private WeakHashMap<Callback, Object> callbacks;
    // Local
    private Queue<BluetoothGattCharacteristic> readQueue = new ConcurrentLinkedQueue<BluetoothGattCharacteristic>();
    // Stops scanning after 5 seconds.
    private Handler mHandler = new Handler();
    private static final long SCAN_PERIOD = 5000;

    // Create main window
    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // EditText fields
        etStartTime = (EditText) findViewById(R.id.etStartTime);
        etRunTimeHr = (EditText) findViewById(R.id.etRunTimeHr);
        //etStartTimeMin = (EditText) findViewById(R.id.etStartTimeMin);
        etRunTimeMin = (EditText) findViewById(R.id.etRunTimeMin);
        etInterval = (EditText) findViewById(R.id.etInterval);
        etCurrent = (EditText) findViewById(R.id.etCurrent);

        etSoilMoisture = (EditText) findViewById(R.id.etSoilMoisture);
        etSoilTemperature = (EditText) findViewById(R.id.etSoilTemperature);
        etAirTemperature = (EditText) findViewById(R.id.etAirTemperature);

        // RadioButtons
        voltRadioGroup = (RadioGroup) findViewById(R.id.voltRadioGroup);

        rd110v = (RadioButton) findViewById(R.id.rd110v);
        rd220v = (RadioButton) findViewById(R.id.rd220v);
        rd440v = (RadioButton) findViewById(R.id.rd440v);

        //  Button
        buttonSubmit = (Button) findViewById(R.id.Submit);
        buttonReset = (Button) findViewById(R.id.cancel_button);
        buttonManualStart = (Button) findViewById(R.id.ManualStart);

        // BLe
        btManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btManager.getAdapter();
        bluetoothLeScanner = btAdapter.getBluetoothLeScanner();

        if (btAdapter != null && !btAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            //startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        }

        // Make sure we have access coarse location enabled, if not, prompt the user to enable it
        //if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (this.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("This app needs scan location access");
                builder.setMessage("Please grant location access so this app can detect peripherals.");
                builder.setPositiveButton(android.R.string.ok, null);
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        requestPermissions(new String[]{Manifest.permission.BLUETOOTH_SCAN}, PERMISSION_REQUEST_BLUETOOTH_SCAN);
                    }
                });
                builder.show();
            }
            // Make sure we have access coarse location enabled, if not, prompt the user to enable it
            if (this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("This app needs location access");
                builder.setMessage("Please grant location access so this app can detect peripherals.");
                builder.setPositiveButton(android.R.string.ok, null);
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);
                    }
                });
                builder.show();
            }

            // Make sure we have access coarse location enabled, if not, prompt the user to enable it
            if (this.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("This app needs fine location access");
                builder.setMessage("Please grant location access so this app can detect peripherals.");
                builder.setPositiveButton(android.R.string.ok, null);
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, PERMISSION_REQUEST_FINE_LOCATION);
                    }
                });
                builder.show();
            }
            // Make sure we have access coarse location enabled, if not, prompt the user to enable it
            if (this.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("This app needs fine location access");
                builder.setMessage("Please grant location access so this app can detect peripherals.");
                builder.setPositiveButton(android.R.string.ok, null);
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT}, PERMISSION_REQUEST_BLUETOOTH_CONNECT);
                    }
                });
                builder.show();
            }
       // }

        //=====================
        // Initialize
        initializeParam();
        startScanning();
        //=====================

        // on below line we are adding
        // click listener for our edit text.
        etStartTime.setInputType(InputType.TYPE_NULL);
        etStartTime.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                etStartTime.requestFocus();
                timePickerDialog = new TimePickerDialog(MainActivity.this, new TimePickerDialog.OnTimeSetListener() {
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


        //====================
        buttonSubmit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                motorRunTimeHr = Integer.parseInt(etRunTimeHr.getText().toString());
                motorRunTimeMin = Integer.parseInt(etRunTimeMin.getText().toString());
                motorRunInterval = Integer.parseInt(etInterval.getText().toString());
                switch (voltRadioGroup.getCheckedRadioButtonId()) {
                    case R.id.rd110v:
                        motorVoltage = 110;
                        break;
                    case R.id.rd220v:
                        motorVoltage = 220;
                        break;
                    case R.id.rd440v:
                        motorVoltage = 440;
                        break;
                }

                motorCurrent = Integer.parseInt(etCurrent.getText().toString());
                soilMoisturePercent = Integer.parseInt(etSoilMoisture.getText().toString());
                soilTemperature = Integer.parseInt(etSoilTemperature.getText().toString());
                airTemperature = Integer.parseInt(etAirTemperature.getText().toString());
                String displayValues = "Sart time: " + motorStartTimeHr + ":" + motorStartTimeMin +
                        "\nRun time: " + motorRunTimeHr + ":" + motorRunTimeMin +
                        "\nInterval: " + motorRunInterval +
                        "\nCurrent (A): " + motorCurrent +
                        "\nVoltage (V): " + motorVoltage +
                        "\nSoil Moisture(%): " + soilMoisturePercent +
                        "\nSoil Temperature(F): " + soilTemperature +
                        "\nAir Temperature(F): " + airTemperature;

                //Toast.makeText(getApplicationContext(), displayValues, Toast.LENGTH_LONG).show();
                AlertDialog.Builder builder1 = new AlertDialog.Builder(MainActivity.this);
                builder1.setMessage(displayValues);
                builder1.setCancelable(true);

                builder1.setPositiveButton(
                        "Yes",
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                Toast.makeText(getApplicationContext(), "Programming Parameters ...1111111111111111111", Toast.LENGTH_LONG).show();
                                dialog.cancel();
                            }
                        });

                builder1.setNegativeButton(
                        "No",
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                Toast.makeText(getApplicationContext(), "Initializing Parameters", Toast.LENGTH_LONG).show();
                                initializeParam();
                                dialog.cancel();
                            }
                        });

                AlertDialog alert11 = builder1.create();
                alert11.show();
            }

        });

        buttonReset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // clearing out all the values
                Toast.makeText(getApplicationContext(), "Initializing Parameters", Toast.LENGTH_LONG).show();
                initializeParam();
            }
        });
    }
//=======

    // Device connect call back
    private final BluetoothGattCallback btleGattCallback = new BluetoothGattCallback() {
        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, final BluetoothGattCharacteristic characteristic) {
            // this will get called anytime you perform a read or write characteristic operation
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    //String data_in = new String(characteristic.getValue());
                    byte[] data_in = characteristic.getValue();
                    // int data_no = Integer.parseInt(data_in, 8);
                }
            });
        }
        @Override
        public void onConnectionStateChange(final BluetoothGatt gatt, final int status, final int newState) {
            // this will get called when a device connects or disconnects
            System.out.println(newState);
            switch (newState) {
                case 0:
                    MainActivity.this.runOnUiThread(new Runnable() {
                        public void run() {
                            connectToDevice.setVisibility(View.VISIBLE);
                            disconnectDevice.setVisibility(View.INVISIBLE);
                        }
                    });
                    break;
                case 2:
                    MainActivity.this.runOnUiThread(new Runnable() {
                        public void run() {
                            connectToDevice.setVisibility(View.INVISIBLE);
                            disconnectDevice.setVisibility(View.VISIBLE);
                        }
                    });

                    // discover services and characteristics for this device
                   // bluetoothGatt.discoverServices();

                    break;
                default:
                    MainActivity.this.runOnUiThread(new Runnable() {
                        public void run() {
                        }
                    });
                    break;
            }
        }

        @Override
        public void onServicesDiscovered(final BluetoothGatt gatt, final int status) {
            // this will get called after the client initiates a 			BluetoothGatt.discoverServices() call
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    //peripheralTextView.append("device services have been discovered\n");
                }
            });
            //displayGattServices(bluetoothGatt.getServices());
        }

        @Override
        // Result of a characteristic read operation
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            }
        }
    };
    private void broadcastUpdate(final String action,
                                 final BluetoothGattCharacteristic characteristic) {

        System.out.println(characteristic.getUuid());
       // System.out.println("hi"+ Arrays.toString(characteristic.getValue()));
    }
    //======
    void initializeParam() {
        etStartTime.setText(String.format("%02d:%02d ", motorStartTimeHr, motorStartTimeMin));
        etRunTimeHr.setText(String.format("%02d", motorRunTimeHr));
        etRunTimeMin.setText(String.format("%02d", motorRunTimeMin));
        etInterval.setText(String.format("%02d", motorRunInterval));
        etCurrent.setText(String.format("%02d", motorCurrent));
        etSoilMoisture.setText(String.format("%02d", soilMoisturePercent));
        etSoilTemperature.setText(String.format("%02d", soilTemperature));
        etAirTemperature.setText(String.format("%02d", airTemperature));
        rd110v.setChecked(true);
    }

    public static class CommunicationStatus {
        public static final long SEND_TIME_OUT_MILLIS = TimeUnit.SECONDS.toMillis(2);
        public static final int COMMUNICATION_SUCCESS = 0;
        public static final int COMMUNICATION_TIMEOUT = -1;
    }
  

    @SuppressLint("MissingPermission")
    public void disconnectDeviceSelected() {
        Toast.makeText(getApplicationContext(), "Disconnecting device at index: " + deviceIndexInput.getText() , Toast.LENGTH_LONG).show();
        bluetoothGatt.disconnect();
    }

    // Send data to connected ble serial port device.
    @SuppressLint("MissingPermission")
    public void send(byte[] data) {
        long beginMillis = System.currentTimeMillis();
        if (TxChar == null || data == null || data.length == 0) {
            // Do nothing if there is no connection or message to send.
            return;
        }
        // Update TX characteristic value.  Note the setValue overload that takes a byte array must be used.
        TxChar.setValue(data);
        writeInProgress = true; // Set the write in progress flag
        bluetoothGatt.writeCharacteristic(TxChar);
        while (writeInProgress) {
            if (System.currentTimeMillis() - beginMillis > CommunicationStatus.SEND_TIME_OUT_MILLIS) {
                notifyOnCommunicationError(CommunicationStatus.COMMUNICATION_TIMEOUT, null);
                break;
            }
        } ; // Wait for the flag to clear in onCharacteristicWrite
    }
    private void notifyOnCommunicationError(int status, String msg) {
        for (Callback cb : callbacks.keySet()) {
            if (cb != null) {
                cb.onCommunicationError(status, msg);
            }
        }
    }

    //


    // Device scan callback.
    private ScanCallback leScanCallback = new ScanCallback() {
        @SuppressLint("MissingPermission")
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            if (result.getDevice().getName() != null) {
                String deviceName = result.getDevice().getName();
                //  final ParcelUuid BASE_UUID = ParcelUuid.fromString("00000000-0000-1000-8000-00805F9B34FB");
                //  final ParcelUuid[] readuuids = result.getDevice().getUuids(); //.anyMatch(BASE_UUID);
                // peripheralTextView.append("Index: " + deviceName + "\n Device Name: " + deviceName + " rssi: " + result.getRssi() + "\n");
                if (deviceName.contains("UART")) {
			        Toast.makeText(getApplicationContext(), "Trying to connect to device: " + deviceName , Toast.LENGTH_LONG).show();
                    bluetoothGatt = result.getDevice().connectGatt(MainActivity.this, false, btleGattCallback);
                    List<BluetoothGattService> gattServices = bluetoothGatt.getServices();
                    // Loops through available GATT Services.
                    for (BluetoothGattService gattService : gattServices) {
                        final String uuid = gattService.getUuid().toString();
                        if (uuid.equalsIgnoreCase("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")) {
                            //peripheralTextView.append("---->Service: "+uuid+"\n");
                            stopScanning();
                            //BluetoothGattService SerialService = (BluetoothGattService) gattService;
                        } else {
                            bluetoothGatt.disconnect();
                        }
                    }
/*  @SuppressLint("MissingPermission")
    public void connectToDeviceSelected() {
        Toast.makeText(getApplicationContext(), "Trying to connect to device at index: " + deviceIndexInput.getText() , Toast.LENGTH_LONG).show();
        int deviceSelected = Integer.parseInt(deviceIndexInput.getText().toString());
        bluetoothGatt = devicesDiscovered.get(deviceSelected).connectGatt(this, false, btleGattCallback);
    }*/
	}
            }
        }
    };


    // Functions for Buttons
    @SuppressLint("MissingPermission")
    public void startScanning() {
        deviceIndex = 0;
        devicesDiscovered.clear();

        if (!btScanning) {
            // Stops scanning after a predefined scan period.
            mHandler.postDelayed(new Runnable() {
                @SuppressLint("MissingPermission")
                @Override
                public void run() {
                    btScanning = false;
                    bluetoothLeScanner.stopScan(leScanCallback);
                }
            }, SCAN_PERIOD);

            btScanning = true;
            bluetoothLeScanner.startScan(leScanCallback);
        } else {
            btScanning = false;
            bluetoothLeScanner.stopScan(leScanCallback);
        }
    }


    public void stopScanning() {
        btScanning = false;

        ExecutorService es = Executors.newSingleThreadExecutor();
        es.execute(new Runnable() {
            @SuppressLint("MissingPermission")
            @Override
            public void run() {
                bluetoothLeScanner.stopScan(leScanCallback);
            }
        });
    }


    // Interface for handler the serial port activity
    public interface Callback {
        public void onConnected(Context context);
        public void onConnectFailed(Context context);
        public void onDisconnected(Context context);
        public void onReceive(Context context, BluetoothGattCharacteristic rx);
        public void onDeviceFound(BluetoothDevice device);
        public void onDeviceInfoAvailable();
        public void onCommunicationError(int status, String msg);
    }
}





/*
    etRunTimeMin.addTextChangedListener(new TextWatcher() {

            public void onTextChanged(CharSequence s, int start, int before, int count) {

                if (etRunTimeMin.getText().toString().length() == 2) {
                    int min = Integer.parseInt(etRunTimeMin.getText().toString());
                    if(min > 59) {
                        Toast.makeText(getApplicationContext(), "Minutes < 60 : " + min, Toast.LENGTH_LONG).show();
                        etRunTimeMin.setText("");
                        etRunTimeMin.requestFocus();
                    }    else {
                        etInterval.requestFocus();
                    }
                }
            }

            @Override
            public void afterTextChanged(Editable arg0) {
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }
        });


buttonReset.setOnClickListener(new View.OnClickListener() {

    @Override
    public void onClick(View v) {
        // clearing out all the values
        editName.setText("");
        editPassword.setText("");
        result.setText("");
        editName.requestFocus();
    }
});

NumberPicker np;


        startTime.setInputType(InputType.TYPE_NULL);
        runTime.setInputType(InputType.TYPE_NULL);

        startTime.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                calendar = Calendar.getInstance();
                currentHour = calendar.get(Calendar.HOUR_OF_DAY);
                currentMinute = calendar.get(Calendar.MINUTE);

                startTimePickerDialog = new TimePickerDialog(MainActivity.this, new TimePickerDialog.OnTimeSetListener() {
                    @Override
                    public void onTimeSet(TimePicker timePicker, int hourOfDay, int minutes) {
                        if (hourOfDay >= 12) {
                            amPm = "PM";
                        } else {
                            amPm = "AM";
                        }
                        startTime.setText(String.format("%02d:%02d", hourOfDay, minutes) + amPm);
                    }
                }, currentHour, currentMinute, false);

                startTimePickerDialog.show();
            }
        });
        startTime = (EditText) findViewById(R.id.etRunTime);

        startTime.setOnEditorActionListener(new TextView.OnEditorActionListener() {
                public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                    if ((event != null && (event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) || (actionId == EditorInfo.IME_ACTION_DONE)) {

                    }
                    return false;
                }
            });*/



