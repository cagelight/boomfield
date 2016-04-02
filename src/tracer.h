#ifndef TRACER_H
#define TRACER_H

#include <math.h>
#include <stdint.h>

#ifndef restrict
#define restrict __restrict__
#endif

typedef uint_fast64_t sample_rate_t;

//================================================================
//----------------------------------------------------------------
//============================ TRACER ============================

typedef double (*tracer_next_f) (void *);

typedef struct tracer_common_s {
	tracer_next_f next;
	double period;
	sample_rate_t * sample_rate_ptr;
	double volume;
	//---
	double period_adj, period_cache; //Don't touch these.
} tracer_common_t;

#define TRACER tracer_common_t common;

inline void tracer_common_init(tracer_common_t * tc, double period, sample_rate_t * sample_rate_ptr) {
	tc->next = 0;
	tc->period = period;
	tc->sample_rate_ptr = sample_rate_ptr;
	tc->period_adj = period / *sample_rate_ptr;
	tc->volume = 1.0;
}

inline double tracer_common_next(void * t) {
	return ((tracer_common_t *)(t))->volume * ((tracer_common_t *)(t))->next(t);
}

//================================================================

typedef struct tracer_mod_s {
	void * affector;
	double *target_data;
	double mult, add;
} tracer_mod_t;

inline void tracer_mod_init(tracer_mod_t * tm, void * affector, double * target_data, double min, double max) {
	tm->affector = affector;
	tm->target_data = target_data;
	tm->add = min;
	tm->mult = (max - min);
}

inline void tracer_mod_affect(tracer_mod_t * tm) {
	*tm->target_data = (tracer_common_next(tm->affector) * 0.5 + 0.5) * tm->mult + tm->add;
}

//================================================================

typedef struct tracer_sine_s {
	TRACER
	double theta;
} tracer_sine_t;

tracer_sine_t tracer_sine_create(double period, sample_rate_t * sample_rate_ptr);
double tracer_sine_next(tracer_sine_t *);

//================================================================

typedef struct tracer_saw_s {
	TRACER
	double lv;
} tracer_saw_t;

tracer_saw_t tracer_saw_create(double period, sample_rate_t * sample_rate_ptr);
double tracer_saw_next(tracer_saw_t *);

//================================================================

typedef struct tracer_freqnoise_subtracer_s {
	double period_adj_sub;
	double theta;
	double mult;
	int_fast8_t direc;
	double tmult;
} tracer_freqnoise_subtracer_t;

typedef struct tracer_freqnoise_s {
	TRACER
	tracer_freqnoise_subtracer_t * subtracers;
	uint_fast8_t subtracers_len;
	double freqmod;
	double freqdev;
} tracer_freqnoise_t;

tracer_freqnoise_t tracer_freqnoise_create(double period, sample_rate_t * sample_rate_ptr, uint_fast8_t subtracer_count, double subtracer_freqmod, double freqdev);
void tracer_freqnoise_destroy(tracer_freqnoise_t *);
double tracer_freqnoise_next(tracer_freqnoise_t *);

//================================================================

#endif
