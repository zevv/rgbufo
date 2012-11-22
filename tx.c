#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>

#define SRATE 48000.0
#define BRATE 1200.0
#define FREQ_0 2200.0
#define FREQ_1 1200.0

struct rb { 
        int16_t *data; 
        int size; 
        int head; 
        int tail; 
};


uint32_t hsv2rgb(double h, double s, double v);
Uint32 on_timer(Uint32 interval, void *_);
void draw(void);
void update_color(void);
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

int R, G, B, dirty;
int mx = 0;
int my = 0;
int run = 0;

struct rb *rb_data;
struct rb *rb_audio;



int main(int argc, char **argv)
{
	SDL_AudioSpec fmt; 

	rb_data = rb_new(256);
	rb_audio = rb_new(48000);

	//int fd = sound_open("/dev/dsp");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

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
	SDL_AddTimer(70, on_timer, NULL);

	for(;;) {

		while(SDL_WaitEvent(&ev)) {

			if(ev.type == SDL_KEYDOWN) {

				switch(ev.key.keysym.sym) {
					case 27:
						exit(0);
						break;
					default:
						{
							char buf[16];
							snprintf(buf, sizeof buf, "d%c\n", ev.key.keysym.sym);
							send(buf);
						}
						break;
				}
			}

			if(ev.type == SDL_MOUSEBUTTONDOWN) {
				run = 1;
			}

			if(ev.type == SDL_VIDEORESIZE) {
				screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h, 32, SDL_HWSURFACE | SDL_RESIZABLE) ;
				draw();
				update_color();
			}

			if(ev.type == SDL_MOUSEMOTION) {
				run = 0;
				mx = ev.motion.x;
				my = ev.motion.y;
				update_color();
			}
		}
	}

	return 0;
}


Uint32 on_timer(Uint32 interval, void *_)
{
	SDL_Event ev;

	if(run) {
		if(mx < screen->w-1) {
			mx ++;
			update_color();
		} else {
			run = 0;
		}
	}

	if(dirty) {
		char buf[32];
		snprintf(buf, sizeof buf, "c%02x%02x%02x\n", R, G, B);
		send(buf);
		dirty = 0;
	}

	ev.user.type = SDL_USEREVENT;
	ev.user.code = 1;
	SDL_PushEvent(&ev);
	return interval;
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


void update_color(void)
{
	uint32_t *v = screen->pixels + (my*screen->w + mx) * 4;

	double r = (*v >> 16) & 0xff;
	double g = (*v >>  8) & 0xff;
	double b = (*v >>  0) & 0xff;

	R = pow(1.0218971486541166, r);
	G = pow(1.0218971486541166, g);
	B = pow(1.0218971486541166, b);

	dirty = 1;
	
}


void send(char *buf)
{
	char *p = buf;
	while(*p) {
		putchar(*p);
		rb_push(rb_data, *p++);
	}
}


void draw(void)
{
	int x, y;
	uint32_t *p = screen->pixels;

	for(y=0; y<screen->h; y++) {
		for(x=0; x<screen->w; x++) {

			h = (float)x / (float)screen->w;
			s =     2 * (float)y / (float)screen->h;
			v = 2 - s;
			if(s > 1) s = 1;
			if(v > 1) v = 1;

			*p++ = hsv2rgb(h, s, v);
		}
	}

	SDL_Flip(screen);
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


void gen_audio(void *data, uint8_t *stream, int len)
{
        int i;
	int16_t *p = (void *)stream;
	int b;

	while(rb_used(rb_audio) < len/2) {

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



/*
 * End
 */

