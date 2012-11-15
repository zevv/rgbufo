#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <linux/soundcard.h>

struct rb { 
        int16_t *data; 
        int size; 
        int head; 
        int tail; 
};


#define SRATE 48000.0
#define FREQ_0 1000.0
#define FREQ_1 2000.0

uint32_t hsv2rgb(double h, double s, double v);
int sound_open(char *dev);
void draw(void);
void send(struct rb *rb);
void gen_audio(void *data, Uint8 *stream, int len);


SDL_Surface *screen;

double v = 1;
int mx = 0;
int my = 0;

void gen_audio(void *data, uint8_t *stream, int len)
{
        struct rb *rb = data;
        int i;
	static double t = 0;
	int16_t *p = (void *)stream;

        for(i=0; i<len/2; i++) {
                if(rb->tail == rb->head) {
			int16_t val = cos(t * FREQ_0 * M_PI * 2) * 32000;
			*p++ = val;
			t += 1.0 / SRATE;
                } else {
                        *p++ = rb->data[rb->tail];
                        rb->tail = (rb->tail + 1) % rb->size;
                }
        }
}


int main(int argc, char **argv)
{
	SDL_AudioSpec fmt; 
	struct rb *rb;

        rb = calloc(sizeof *rb, 1);
        rb->size = 32e3;
        rb->data = malloc(rb->size * 2);
        rb->head = 0;
        rb->tail = 0;

	//int fd = sound_open("/dev/dsp");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	fmt.freq = SRATE;
	fmt.format = AUDIO_S16;
	fmt.channels = 1;
	fmt.samples = 256;
	fmt.callback = gen_audio;
	fmt.userdata = rb;

	if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
		fprintf(stderr, "Can't open audio: %s\n", SDL_GetError());
		exit(1);
	}


	SDL_Event ev;
	screen = SDL_SetVideoMode(16, 16, 32, SDL_HWSURFACE | SDL_RESIZABLE) ;
	screen = SDL_SetVideoMode(16, 16, 32, SDL_HWSURFACE | SDL_RESIZABLE) ;
	draw();

	SDL_PauseAudio(0);

	for(;;) {

		while(SDL_WaitEvent(&ev)) {

			if(ev.type == SDL_KEYDOWN) {

				switch(ev.key.keysym.sym) {
					case 27:
					case SDLK_q:
						exit(0);
						break;
					default:
						break;
				}
			}

			if(ev.type == SDL_MOUSEBUTTONDOWN) {
				if(ev.button.button == 0) {
					send(rb);
				}
				if(ev.button.button == 4) {
					v += 0.05;
				}
				if(ev.button.button == 5) {
					v -= 0.05;
				}
				if(v < 0) v = 0;
				if(v > 1) v = 1;
				draw();
				send(rb);
			}

			if(ev.type == SDL_VIDEORESIZE) {
				screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h, 32, SDL_HWSURFACE | SDL_RESIZABLE) ;
				draw();
				send(rb);
			}

			if(ev.type == SDL_MOUSEMOTION) {
				mx = ev.motion.x;
				my = ev.motion.y;
				send(rb);
			}
		}
	}

	return 0;
}


void rb_push(struct rb *rb, int16_t v)
{
	rb->data[rb->head] = v;
	rb->head = (rb->head + 1) % rb->size;
}


void send(struct rb *rb)
{
	char buf[32];
	char *p = buf;
	int b, i;
	static double t = 0;

	snprintf(buf, sizeof buf, "A");

	while(*p) {
		int v = (((*p) << 1) | 0x100);

		for(b=0; b<10; b++) {
			double f = (v & 1) ? FREQ_0 : FREQ_1;
			v >>= 1;
			for(i=0; i<160; i++) {
				int16_t v = cos(t * f * M_PI * 2) * 32000;
				rb_push(rb, v);
				t += 1.0 / SRATE;
			}
		}
		p++;
	}


//	uint32_t *p = screen->pixels;
//	p += mx * screen->w + my;
//	printf("%08x\n", *p);


}


void draw(void)
{
	int x, y;
	uint32_t *p = screen->pixels;

	for(y=0; y<screen->h; y++) {
		for(x=0; x<screen->w; x++) {

			double s = (float)y / (float)screen->h;
			double h = (float)x / (float)screen->w;

			*p++ = hsv2rgb(h, s, v);
		}
	}

	SDL_Flip(screen);
}


int sound_open(char *dev)
{
	int r;
	int fd;
	int i;
	int stereo = 0;
	int format = AFMT_S16_LE;
	int fragments = 0x00020002;
	int srate = SRATE;
	
	fd = open(dev, O_RDONLY);

	r=ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &fragments);
	if(r==-1) {
		printf("Error SNDCTL_DSP_SETFRAGMET : %s\n", strerror(errno));
		exit(1);
	}

	r=ioctl(fd, SOUND_PCM_SETFMT, &format);
	if(r==-1) {
		printf("Error SOUND_PCM_SETFMT : %s\n", strerror(errno));
		exit(1);
	}
	
	i=stereo;
	r=ioctl(fd, SNDCTL_DSP_STEREO, &stereo);
	if(i != stereo) {
		printf("Cant set channels to %d\n", i);
		exit(1);
	}
	if(r==-1) {
		printf("Error SNDCTL_DSP_STEREO : %s\n", strerror(errno));
		exit(1);
	}

	r=ioctl(fd, SNDCTL_DSP_SPEED, &srate);
	if(r==-1) {
		printf("Error SNDCTL_DSP_SPEED : %s\n", strerror(errno));
		exit(1);
	}
	
	return(fd);
}


uint32_t hsv2rgb(double h, double s, double v)
{	
	double r, g, b;
	double f, m, n;
	int i;

	h *= 6.0;
	i = floor(h);
	f = (h) - i;
	if (!(i & 1)) f = 1 - f;
	m = v * (1 - s);
	n = v * (1 - s * f);
	if(i<0) i=0;
	if(i>6) i=6;
	switch (i) {
		case 6:
		case 0: r=v; g=n, b=m; break;
		case 1: r=n; g=v, b=m; break;
		case 2: r=m; g=v, b=n; break;
		case 3: r=m; g=n, b=v; break;
		case 4: r=n; g=m, b=v; break;
		case 5: r=v; g=m, b=n; break;
		default: r=g=b=1;
	}

	return SDL_MapRGB(screen->format, r*255, g*255, b*255);
}

/*
 * End
 */

