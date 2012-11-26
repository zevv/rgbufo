
package com.zevv.rgbufo;

import java.io.IOException;
import java.util.concurrent.ExecutionException;
import java.util.LinkedList;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack; 
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.MotionEvent;
import android.view.View.OnTouchListener;
import android.widget.TextView;
import android.widget.Toast;

public class rgbufo extends Activity implements SensorEventListener
{
	private Canvas canvas = new Canvas();
	private int cw = 0, ch = 0;
	private GraphView gv;
	private Bitmap bitmap;
	private AudioTask audiotask;
	private AudioTrack track;
	private Paint paint = new Paint(0);
	private LinkedList<Integer> fifo = new LinkedList<Integer>();
	private double t = 0;
	private SensorManager sensorManager = null;

	private void do_draw()
	{
		int x, y;

		int d = 4;

		for(y=0; y<ch; y+=d) {
			for(x=0; x<cw; x+=d) {

				double h = (double)y / (double)ch;
				double s = (double)x / (double)cw;
				double v = 1.0;

				int xx = cw/2 - x;
				int yy = ch/2 - y;

				h = (Math.atan2(xx, yy) + 3.1415) / 6.2830;
				s = 1.2 * Math.hypot(xx, yy) / (cw/2) - 0.2;

				if(h < 0) h = 0;
				if(h > 1) h = 1;
				if(s < 0) s = 0;
				if(s > 1) s = 1;

				double r, g, b, f, m, n;
				int i;

				h *= 6.0;
				i = (int)Math.floor(h);
				f = (h) - i;
				if ((i & 1) != 1) f = 1 - f;
				m = v * (1 - s);
				n = v * (1 - s * f);
				if(i<0) i=0;
				if(i>6) i=6;
				switch (i) {
					case 6:
					case 0: r=v; g=n; b=m; break;
					case 1: r=n; g=v; b=m; break;
					case 2: r=m; g=v; b=n; break;
					case 3: r=m; g=n; b=v; break;
					case 4: r=n; g=m; b=v; break;
					case 5: r=v; g=m; b=n; break;
					default: r=g=b=1;
				}

				paint.setARGB(255, (int)(r*255), (int)(g*255), (int)(b*255));
				canvas.drawRect(x, y, x+d, y+d, paint);	
				canvas.drawPoint(x, y, paint);
			}
		}
	}


	/*
	 * View
	 */

	private class GraphView extends View
	{

		public GraphView(Context context) {
			super(context);
		}


		@Override
		protected void onSizeChanged(int nw, int nh, int pw, int ph) 
		{
			cw = nw;
			ch = nh;
			bitmap = Bitmap.createBitmap(nw, nh, Bitmap.Config.RGB_565);
			canvas.setBitmap(bitmap);
			super.onSizeChanged(cw, ch, pw, ph);
			do_draw();
		}


		@Override
		protected void onDraw(Canvas canvas) 
		{
			if(bitmap != null) {
				do_draw();
				canvas.drawBitmap(bitmap, 0, 0, null);
			}
		}

	}



	/*
	 * Audio task
	 */

	private class AudioTask extends AsyncTask<Void, Double, Integer> 
	{

		private void write_bit(int bit)
		{
			int i;
			int len = 48000 / 1200;
			short[] buffer = new short[len];

			for(i=0; i<len; i++) {
				buffer[i] = (short)(Math.cos(t * 3.141592 * 2) * 20000.0);
				t += ((bit == 0) ? 2200 : 1200) / 48000.0;
			}
			track.write(buffer, 0, len);
		}

		@Override
		protected Integer doInBackground(Void... params) 
		{
			while(! isCancelled() && track != null) {
				
				int i;

				if(fifo.isEmpty()) {
					for(i=0; i<8; i++) {
						write_bit(0);
					}
				} else {
					int c = fifo.removeFirst();
					int v = (((c ^ 0xff) << 1) | 0x1);

					int b;
					for(b=0; b<10; b++) {
						write_bit(v & 1);
						v >>= 1;
					}
				}
			}

			return 0;
		}


		@Override
		protected void onProgressUpdate(Double... val)
		{

		}
	}
	

	private void send_byte(int b)
	{
		if(b >= 0xfe) {
			fifo.add(0xfe);
			fifo.add(b);
		} else {
			fifo.add(b);
		}
	}


	private void send(int data[])
	{
		int i;
		int sum = 0;

		for(i=0; i<data.length; i++) {
			int b = data[i] & 0xff;
			sum = (sum + b) & 0xff;
			send_byte(b);
		}
		send_byte(sum ^ 0xff);
		fifo.add(0xff);
	}


	private void send_color(int x, int y, double p)
	{
		if(p > 0 && fifo.size() > 0) return;

		if(x < 0) x = 0; if(x > cw) x = cw-1;
		if(y < 0) y = 0; if(y > ch) y = ch-1;

		int c = bitmap.getPixel(x, y);

		/* Get pixel value from canvas */

		int r = (int)(((c >> 16) & 0xff) * p);
		int g = (int)(((c >>  8) & 0xff) * p);
		int b = (int)(((c >>  0) & 0xff) * p);

		/* Gamma correction */

		r = (int)Math.pow(1.022, r) - 1;
		g = (int)Math.pow(1.022, g) - 1;
		b = (int)Math.pow(1.022, b) - 1;

		if(r > 255) r = 255;
		if(g > 255) g = 255;
		if(b > 255) b = 255;
		
		Log.i("rgbufo", String.format("pixel: #%02x%02x%02x", r, g, b));

		int[] data = { 'c', r, g, b };
		send(data);
	}


	/*
	 * Application handlers
	 */

	@Override
	public void onCreate(Bundle saved)
	{
		super.onCreate(saved);
		sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);

		requestWindowFeature(Window.FEATURE_NO_TITLE);
			
		gv = new GraphView(this);
		setContentView(gv);

		canvas.drawColor(Color.BLACK);
	}


	@Override
	public void onDestroy()
	{
		super.onDestroy();
	}


	@Override
	public void onStart()
	{
		super.onStart();
		
		/* Init audio */

		int buflen = AudioTrack.getMinBufferSize(
				48000, 
				AudioFormat.CHANNEL_CONFIGURATION_MONO,
				AudioFormat.ENCODING_PCM_16BIT);

		track = new AudioTrack(
				AudioManager.STREAM_MUSIC, 
				48000, 
				AudioFormat.CHANNEL_CONFIGURATION_MONO, 
				AudioFormat.ENCODING_PCM_16BIT, 
				buflen, 
				AudioTrack.MODE_STREAM);

		track.play();

		audiotask = new AudioTask();
		audiotask.execute();
	
	}


	private double pmax = 0.5;
	private double pavg = 0;

	@Override
	public boolean onTouchEvent(MotionEvent ev) {
		int x = (int)ev.getX();
		int y = (int)ev.getY();
		int a = ev.getAction();

		switch(a) {
			case MotionEvent.ACTION_MOVE:
				double p = ev.getPressure();
				if(p > pmax) pmax = p;
				pmax *= 0.99;
				p = p / pmax;
				if(p < 0) p = 0;
				if(p > 1) p = 1;
				pavg = pavg * 0.6 + p * 0.4;
				send_color(x, y, pavg);
				break;
			case MotionEvent.ACTION_UP:
				send_color(x, y, 0);
				break;
		}

		return false;
	}

	

	@Override
	public void onSensorChanged(SensorEvent ev)
	{
		double ry = ev.values[0];
		double rz = ev.values[1];
	}


	@Override
	public void onAccuracyChanged(Sensor arg0, int arg1) {
	
	}



	@Override
	public void onStop()
	{
		super.onStop();

		audiotask.cancel(true);

		track.stop(); 
		track.release();
		track = null;
	}
}

/*
 * vi: ft=java ts=3 sw=3
 */

