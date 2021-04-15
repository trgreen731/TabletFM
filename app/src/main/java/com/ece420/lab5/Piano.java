package com.ece420.lab5;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.Message;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import java.util.ArrayList;

public class Piano extends View {

    public static final int NB = 14;
    private double key_freq[];
    private Paint black, yellow, white;
    private ArrayList<Key> whites = new ArrayList<>();
    private ArrayList<Key> blacks = new ArrayList<>();
    private int keyWidth, height;
    public double freq;

    public Piano(Context context, AttributeSet attrs){
        super(context, attrs);
        black = new Paint();
        black.setColor(Color.BLACK);
        white = new Paint();
        white.setColor(Color.WHITE);
        white.setStyle(Paint.Style.FILL);
        yellow = new Paint();
        yellow.setColor(Color.YELLOW);
        yellow.setStyle(Paint.Style.FILL);
        freq = 0;
        key_freq = new double[]{130.81, 146.83, 138.59, 164.81, 155.56, 174.61, 195.99, 184.99, 220.00,
                                207.65, 246.94, 233.08, 261.63, 293.66, 277.18, 329.63, 311.13, 349.23,
                                391.99, 369.99, 440.00, 415.30, 493.88, 466.16};
        //make the envelope functions and wavetable right here
        initTable(0);
        initAmpEnv(4800, 4800, 70, 4800);
        initModEnv(4800, 4800, 70, 4800);
    }

    @Override
    protected void onSizeChanged(int w,int h, int oldw, int oldh){
        super.onSizeChanged(w, h, oldw, oldh);
        keyWidth = w/NB;
        height = h;
        int count = 0;

        for(int i=0; i<NB; i++){
            int left = i*keyWidth;
            int right = left+keyWidth;

            if(i == NB-1){
                right = w;
            }

            RectF rect = new RectF(left, 0, right, h);
            whites.add(new Key(rect, key_freq[count]));
            count++;

            if(i != 0 && i != 3 && i != 7 && i != 10){
                rect = new RectF((float)(i-1)*keyWidth + 0.5f*keyWidth + 0.25f*keyWidth, 0, (float)i*keyWidth + 0.25f*keyWidth, 0.67f*height);
                blacks.add(new Key(rect, key_freq[count]));
                count++;
            }
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        for (Key k : whites) {
            canvas.drawRect(k.rect, k.down ? yellow : white);
        }

        for (int i = 1; i < NB; i++) {
            canvas.drawLine(i * keyWidth, 0, i * keyWidth, height, black);
        }

        for (Key k : blacks) {
            canvas.drawRect(k.rect, k.down ? yellow : black);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getAction();
        boolean isDownAction = action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_MOVE;

        for (int touchIndex = 0; touchIndex < event.getPointerCount(); touchIndex++) {
            float x = event.getX(touchIndex);
            float y = event.getY(touchIndex);

            Key k = keyForCoords(x,y);

            if (k != null) {
                k.down = isDownAction;
            }
        }

        ArrayList<Key> tmp = new ArrayList<>(whites);
        tmp.addAll(blacks);

        int touched = 0;
        for (Key k : tmp) {
            if (k.down) {
                freq = k.sound;
                touched = 1;
                invalidate();
            }
            else{
                releaseKey(k);
            }
        }
        if (touched == 0){
            freq = 0;
        }
        writeNewFreq(freq);

        return true;
    }

    private Key keyForCoords(float x, float y) {
        for (Key k : blacks) {
            if (k.rect.contains(x,y)) {
                return k;
            }
        }

        for (Key k : whites) {
            if (k.rect.contains(x,y)) {
                return k;
            }
        }

        return null;
    }

    private void releaseKey(final Key k) {
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                k.down = false;
                handler.sendEmptyMessage(0);
            }
        }, 100);
    }

    private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            invalidate();
        }
    };

    public static native void writeNewFreq(double freq);
    public static native void initTable(int wave);
    public static native void initAmpEnv(int A, int D, int S, int R);
    public static native void initModEnv(int A, int D, int S, int R);
}
