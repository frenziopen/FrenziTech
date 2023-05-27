package com.example.frenzimed;

import android.Manifest;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;



public class MainActivity extends AppCompatActivity {
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;
    private static final int PERMISSION_REQUEST_BLUETOOTH_SCAN = 2;
    private static final int PERMISSION_REQUEST_FINE_LOCATION = 3;
    private static final int PERMISSION_REQUEST_BLUETOOTH_CONNECT = 4;
    //  private SubmitButton btnget;
    Button btnget;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.first);
        btnget = findViewById(R.id.btnget);

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



        //passing activity
        btnget.setOnClickListener(v -> {
            //Toast.makeText(getApplicationContext(), "PLEASE MAKE SURE DEVICE IS ON", Toast.LENGTH_LONG).show();
            Intent a = new Intent(MainActivity.this, MainActivity_action.class);
            startActivity(a);
        });
    }
}


 /*

import android.content.SharedPreferences;
        import android.os.CountDownTimer;
        import android.os.Bundle;
        import android.util.Log;
        import android.widget.TextView;

        import androidx.appcompat.app.AppCompatActivity;

        import java.util.Locale;

public class MainActivity2 extends AppCompatActivity {
    private static final long START_TIME_IN_MILLIS = 15000;
    private TextView mTextViewCountDown;
    private CountDownTimer mCountDownTimer;
    private boolean mTimerRunning;
    private long mTimeLeftInMillis;
    private long mEndTime;
    private long remainingTimeInMillis;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_new);

        mTextViewCountDown = findViewById(R.id.tv);
    }

    private void startTimer() {
        mCountDownTimer = new CountDownTimer(remainingTimeInMillis, 1000) {
            @Override
            public void onTick(long millisUntilFinished) {
                remainingTimeInMillis = millisUntilFinished;
                mTimeLeftInMillis = millisUntilFinished;
                updateCountDownText();
            }

            @Override
            public void onFinish() {
                //mTimerRunning = false;
                //updateButtons();

                updateCountDownText();
                resetTimer();
                startTimer();

            }
        }.start();

        //mTimerRunning = true;

    }


    private void resetTimer() {
        remainingTimeInMillis = START_TIME_IN_MILLIS;
        updateCountDownText();

    }

    private void updateCountDownText() {


        int minutes = (int) (remainingTimeInMillis / 1000) / 60;
        int seconds = (int) (remainingTimeInMillis / 1000) % 60;

        String timeLeftFormatted = String.format(Locale.getDefault(), "%02d:%02d", minutes, seconds);

        mTextViewCountDown.setText(timeLeftFormatted);
    }


    @Override
    protected void onStop() {
        super.onStop();

        SharedPreferences prefs = getSharedPreferences("prefs", MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        editor.putLong("millisLeft", mTimeLeftInMillis);
        editor.putBoolean("timerRunning", mTimerRunning);
        editor.putLong("endTime", System.currentTimeMillis());
        editor.apply();

    }

    @Override
    protected void onStart() {
        super.onStart();

        SharedPreferences prefs = getSharedPreferences("prefs", MODE_PRIVATE);

        mTimeLeftInMillis = prefs.getLong("millisLeft", START_TIME_IN_MILLIS);
        mTimerRunning = prefs.getBoolean("timerRunning", false);
        mEndTime = prefs.getLong("endTime", 0);
        if (mEndTime == 0L) {
            remainingTimeInMillis = (mTimeLeftInMillis);
        } else {
            Long timeDiff = (mEndTime - System.currentTimeMillis());
            //to convert into positive number
            timeDiff = Math.abs(timeDiff);

            long timeDiffInSeconds = (timeDiff / 1000) % 60;
            long timeDiffInMillis = timeDiffInSeconds * 1000;
            Long timeDiffInMillisPlusTimerRemaining = remainingTimeInMillis = mTimeLeftInMillis - timeDiffInMillis;

            if (timeDiffInMillisPlusTimerRemaining < 0) {
                timeDiffInMillisPlusTimerRemaining = Math.abs(timeDiffInMillisPlusTimerRemaining);
                remainingTimeInMillis = START_TIME_IN_MILLIS - timeDiffInMillisPlusTimerRemaining;
            }
        }
        updateCountDownText();
        startTimer();
    }
}
*/