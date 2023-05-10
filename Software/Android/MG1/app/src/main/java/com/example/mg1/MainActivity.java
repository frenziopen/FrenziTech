package com.example.mg1;

import android.app.AlertDialog;
import android.app.TimePickerDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TimePicker;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    // These are the global variables
    int motorStartTimeHr    = 12;
    int motorStartTimeMin   = 0;
    int motorRunTimeHr      = 12;
    int motorRunTimeMin     = 0;
    int motorRunInterval    = 10;
    int motorVoltage        = 110;
    int motorCurrent        = 10;
    int soilMoisturePercent = 10;
    int soilTemperature     = 30;
    int airTemperature      = 30;

    // These are the GUI variables
    EditText etStartTime, etRunTimeHr, etRunTimeMin, etInterval, etCurrent, etSoilMoisture, etSoilTemperature, etAirTemperature;
    RadioButton rd110v, rd220v, rd440v;
    RadioGroup voltRadioGroup;
    Button buttonSubmit, buttonReset, buttonManualStart;


    TimePickerDialog timePickerDialog;
    String amPm;
    String name;

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
        /*
        etCurrent = (EditText) findViewById(R.id.etCurrent);

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
        buttonSubmit = (Button) findViewById(R.id.Submit);
        buttonReset = (Button) findViewById(R.id.cancel_button);
        buttonManualStart = (Button) findViewById(R.id.ManualStart);
        //=====================
        // Initialize
        initializeParam ();
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



        //====================
        buttonSubmit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                name = etStartTime.getText().toString();
                motorRunTimeHr      = Integer.parseInt(etRunTimeHr.getText().toString());
                motorRunTimeMin     = Integer.parseInt(etRunTimeMin.getText().toString());
                motorRunInterval    = Integer.parseInt(etInterval.getText().toString());
                switch (voltRadioGroup.getCheckedRadioButtonId()) {
                    case R.id.rd110v:
                        motorVoltage        = 110;
                        break;
                    case R.id.rd220v:
                        motorVoltage        = 220;
                        break;
                    case R.id.rd440v:
                        motorVoltage        = 440;
                        break;
                }

                motorCurrent        = Integer.parseInt(etCurrent.getText().toString());
                soilMoisturePercent = Integer.parseInt(etSoilMoisture.getText().toString());
                soilTemperature     = Integer.parseInt(etSoilTemperature.getText().toString());
                airTemperature      = Integer.parseInt(etAirTemperature.getText().toString());
                String displayValues  = 	"Sart time: " + motorStartTimeHr + ":" + motorStartTimeMin +
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
                                initializeParam ();
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
                initializeParam ();
            }
        });
    }

    void initializeParam () {
        etStartTime.setText(String.format("%02d:%02d ", motorStartTimeHr, motorStartTimeMin));
        etRunTimeHr.setText(String.format("%02d", motorRunTimeHr));
        etRunTimeMin.setText(String.format("%02d", motorRunTimeMin));
        etInterval.setText(String.format("%02d", motorRunInterval));
//        etCurrent.setText(String.format("%02d", motorCurrent));
//        etSoilMoisture.setText(String.format("%02d", soilMoisturePercent));
//        etSoilTemperature.setText(String.format("%02d", soilTemperature));
//        etAirTemperature.setText(String.format("%02d", airTemperature));
//        rd110v.setChecked(true);
    }
}





/*

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



