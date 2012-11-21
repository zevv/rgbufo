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
#define BRATE 1200.0
float FREQ_0 = 2200.0;
float FREQ_1 = 1200.0;

uint32_t hsv2rgb(double h, double s, double v);
int sound_open(char *dev);
void draw(void);
void send(char *buf);
void gen_audio(void *data, Uint8 *stream, int len);

struct rb *rb_new(size_t size);
void rb_push(struct rb *rb, int16_t v);
int rb_pop(struct rb *rb);
int rb_used(struct rb *rb);

SDL_Surface *screen;

double t = 0;
double h = 1;
double s = 1;
double v = 1;
int mx = 0;
int my = 0;

struct rb *rb_data;
struct rb *rb_audio;


void gen_audio(void *data, uint8_t *stream, int len)
{
        int i;
	int16_t *p = (void *)stream;
	int b;

	while(rb_used(rb_audio) < len) {

		int v = 0;

		if(rb_used(rb_data) > 0) {
			char c = rb_pop(rb_data);
			v = (((c ^ 0xff) << 1) | 0x1);
		}

		for(b=0; b<10; b++) {
			for(i=0; i<(SRATE / BRATE); i++) {
				int16_t y = cos(t * M_PI * 2) * 32000;
				rb_push(rb_audio, y);
				if(v & 1) {
					t += FREQ_1 / SRATE; 
				} else {
					t += FREQ_0 / SRATE; 
				}
			}
			v >>= 1;
		}
	}

        for(i=0; i<len/2; i++) {
		*p++ = rb_pop(rb_audio);
        }
}


int main(int argc, char **argv)
{
	SDL_AudioSpec fmt; 

	rb_data = rb_new(256);
	rb_audio = rb_new(48000);

	//int fd = sound_open("/dev/dsp");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	fmt.freq = SRATE;
	fmt.format = AUDIO_S16;
	fmt.channels = 1;
	fmt.samples = 256;
	fmt.callback = gen_audio;

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
					case SDLK_q:
						FREQ_0 += 10;
						break;
					case SDLK_a:
						FREQ_0 -= 10;
						break;
					case SDLK_w:
						FREQ_1 += 10;
						break;
					case SDLK_s:
						FREQ_1 -= 10;
						break;
					case 27:
						exit(0);
						break;
					default:
						break;
				}

				send("");
			}

			if(ev.type == SDL_MOUSEBUTTONDOWN) {
				if(ev.button.button == 0) {
					send("A");
				}
				if(ev.button.button == 4) {
					s += 0.05;
				}
				if(ev.button.button == 5) {
					s -= 0.05;
				}
				if(s < 0) s = 0;
				if(s > 1) s = 1;
				draw();
				send("A");
			}

			if(ev.type == SDL_VIDEORESIZE) {
				screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h, 32, SDL_HWSURFACE | SDL_RESIZABLE) ;
				draw();
				send("A");
			}

			if(ev.type == SDL_MOUSEMOTION) {
				mx = ev.motion.x;
				my = ev.motion.y;
				//send("A");
			}
		}
	}

	return 0;
}


struct rb *rb_new(size_t size)
{
	struct rb *rb;
        rb = calloc(sizeof *rb, 1);
        rb->size = size;
        rb->data = malloc(size * 2);
        rb->head = 0;
        rb->tail = 0;
	return rb;
}


int rb_used(struct rb *rb)
{
	if(rb->head >= rb->tail) {
		return rb->head - rb->tail;
	} else {
		return rb->size + rb->head - rb->tail;
	}
}


void rb_push(struct rb *rb, int16_t v)
{
	rb->data[rb->head] = v;
	rb->head = (rb->head + 1) % rb->size;
}


int rb_pop(struct rb *rb)
{
	int16_t v = rb->data[rb->tail];
	rb->tail = (rb->tail + 1) % rb->size;
	return v;
}


void send(char *_)
{
	uint32_t *v = screen->pixels + (my*screen->w + mx) * 4;

	double r = (*v >> 16) & 0xff;
	double g = (*v >>  8) & 0xff;
	double b = (*v >>  0) & 0xff;

	r = pow(1.0218971486541166, r);
	g = pow(1.0218971486541166, g);
	b = pow(1.0218971486541166, b);

	char buf[32];
	snprintf(buf, sizeof buf, "r%.0f\ng%.0f\nb%.0f\n", r, g, b);

	char *p = buf;
	while(*p) rb_push(rb_data, *p++);

}


void draw(void)
{
	int x, y;
	uint32_t *p = screen->pixels;

	for(y=0; y<screen->h; y++) {
		for(x=0; x<screen->w; x++) {

			h = (float)x / (float)screen->w;
			v = 1 - (float)y / (float)screen->h;

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

