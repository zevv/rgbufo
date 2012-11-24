
package com.zevv.rgbufo;

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
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.Window;
import android.widget.TextView;
import android.widget.Toast;
import java.io.IOException;
import java.util.concurrent.ExecutionException;
import java.util.LinkedList;

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
	private double H = 0, S = 1.0, L = 1.0;
	private int srate = 22050;
	private int brate = 1200;


	private void do_draw()
	{
		int x, y;

		Log.i("rgbufo", "draw start");

		double h = H;
		double s = L;
		double v = 2 - L;

		s = (s>1) ? 1 : s;
		v = (v>1) ? 1 : v;

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
		canvas.drawRect(0, 0, cw, ch, paint);	
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
			//Log.i("rgbufo", "sizeChanged");
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
			int len = srate / brate;
			short[] buffer = new short[len];

			for(i=0; i<len; i++) {
				buffer[i] = (short)(Math.cos(t * 3.141592 * 2) * 20000.0);
				t += ((bit == 0) ? 2200.0 : 1200.0) / srate;
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


	/*
	 * Application handlers
	 */

	@Override
	public void onCreate(Bundle saved)
	{
		//Log.i("rgbufo", "onCreate");
		super.onCreate(saved);

		requestWindowFeature(Window.FEATURE_NO_TITLE);
			
		gv = new GraphView(this);
		setContentView(gv);

		canvas.drawColor(Color.BLACK);
		sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
	}


	@Override
	protected void onResume() {
		super.onResume();
		sensorManager.registerListener(this, 
				sensorManager.getDefaultSensor(Sensor.TYPE_ORIENTATION), 
				SensorManager.SENSOR_DELAY_GAME);
	}


	@Override
	public void onDestroy()
	{
		//Log.i("rgbufo", "onDestroy");
		super.onDestroy();
	}


	@Override
	public void onStart()
	{
		//Log.i("rgbufo", "onStart");
		super.onStart();
		
		/* Init audio */

		int buflen = AudioTrack.getMinBufferSize(
				srate, 
				AudioFormat.CHANNEL_CONFIGURATION_MONO,
				AudioFormat.ENCODING_PCM_16BIT);

		track = new AudioTrack(
				AudioManager.STREAM_MUSIC, 
				srate, 
				AudioFormat.CHANNEL_CONFIGURATION_MONO, 
				AudioFormat.ENCODING_PCM_16BIT, 
				buflen, 
				AudioTrack.MODE_STREAM);

		track.play();

		audiotask = new AudioTask();
		audiotask.execute();
	
	}

	private void send_color(int x, int y)
	{
		if(fifo.size() > 16) return;

		int c = bitmap.getPixel(x, y);

		/* Get pixel value from canvas */

		int r = (c >> 16) & 0xff;
		int g = (c >>  8) & 0xff;
		int b = (c >>  0) & 0xff;

		/* Gamma correction */

		r = (int)Math.pow(1.0218971486541166, r);
		g = (int)Math.pow(1.0218971486541166, g);
		b = (int)Math.pow(1.0218971486541166, b);

		String buf = String.format("c%02x%02x%02x\n", r, g, b);
		Log.i("rgbufo", "pixel: " + buf);

		byte[] bs = buf.getBytes();
		int i;
		for(i=0; i<bs.length; i++) {
			fifo.add((int)bs[i]);
		}
	}


	@Override
	public boolean onTouchEvent(MotionEvent ev) {
		int x = (int)ev.getX();
		int y = (int)ev.getY();
		send_color(x, y);
		return false;
	}


	private double ory = 0;
	private double orz = 0;
	

	@Override
	public void onSensorChanged(SensorEvent ev)
	{
		double ry = ev.values[0];
		double rz = ev.values[1];

		H += (ry - ory) / 80;
		L += (rz - orz) / 40;

		if(H < 0) H = 0;
		if(H > 1) H = 1;
		if(L < 0) L = 0;
		if(L > 2) L = 2;
		
		ory = ry;
		orz = rz;

		if(gv != null) gv.invalidate();
		send_color(0, 0);
	}


	@Override
	public void onAccuracyChanged(Sensor arg0, int arg1) {
	
	}


	@Override
	public void onStop()
	{
		//Log.i("rgbufo", "onStop");
		super.onStop();

		audiotask.cancel(true);

		track.stop(); 
		track.release();
		track = null;
	}
}

/* End */

